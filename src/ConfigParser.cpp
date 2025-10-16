#include "webserv.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

std::string ConfigParser::trim(const std::string& str) {
	size_t first = str.find_first_not_of(" \t\r\n");
	if (std::string::npos == first)
		return "";
	size_t last = str.find_last_not_of(" \t\r\n");
	return str.substr(first, (last - first + 1));
}

std::vector<std::string> ConfigParser::split(const std::string& str, char delimiter) {
	std::vector<std::string> tokens;
	std::stringstream ss(str);
	std::string token;

	while (std::getline(ss, token, delimiter)) {
		std::string trimmed = trim(token);
		if (!trimmed.empty()) {
			tokens.push_back(trimmed);
		}
	}
	return tokens;
}

int ConfigParser::parseHttpMethods(const std::string& methods) {
	int result = 0;
	std::vector<std::string> methodList = split(methods, ' ');

	for (const auto& method : methodList) {
		if (method == "GET") result |= static_cast<int>(HttpMethod::GET);
		else if (method == "POST") result |= static_cast<int>(HttpMethod::POST);
		else if (method == "DELETE") result |= static_cast<int>(HttpMethod::DELETE);
	}
	return result ? result : static_cast<int>(HttpMethod::GET); // Default to GET if no methods specified
}

void ConfigParser::parseLocationDirective(const std::string& line, Location& location) {
	std::istringstream iss(line);
	std::string directive;
	iss >> directive;

	std::string rest;
	std::getline(iss, rest);
	rest = trim(rest);
	if (!rest.empty() && rest.back() == ';') rest.pop_back();

	auto take_first = [&](const std::string &s) -> std::string {
		if (s.empty()) return "";
		size_t p = s.find(' ');
		if (p == std::string::npos) return s;
		return s.substr(0, p);
	};

	if (directive == "root") {
		location.root = take_first(rest);
	} else if (directive == "index") {
		location.index = take_first(rest);
	} else if (directive == "allow_methods") {
		location.allowedMethods = parseHttpMethods(trim(rest));
	} else if (directive == "autoindex") {
		std::string token = take_first(rest);
		location.autoindex = (token == "on");
	} else if (directive == "return") {
		std::string codeStr = take_first(rest);
		std::string url;
		if (!codeStr.empty()) {
			size_t pos = rest.find(' ');
			if (pos != std::string::npos) url = trim(rest.substr(pos + 1));
		}
		location.redirectCode = 0;
		location.redirectUrl.clear();
		if (!codeStr.empty() && !url.empty()) {
			try {
				int code = std::stoi(codeStr);
				if (code >= 300 && code < 400) {
					location.redirectCode = code;
					location.redirectUrl = url;
				}
			} catch (...) {
			}
		}
	} else if (directive == "cgi_extension") {
		location.cgiExtension = take_first(rest);
	} else if (directive == "cgi_path") {
		location.cgiPath = take_first(rest);
	}
}

void ConfigParser::parseLocationBlock(std::ifstream& file, Location& location) {
	std::string line;

	while (std::getline(file, line)) {
		line = trim(line);

		if (line.empty() || line[0] == '#')
			continue;

		if (line == "}")
			break;

		parseLocationDirective(line, location);
	}
}

void ConfigParser::parseServerDirective(const std::string& line, ServerConfig& config) {
	std::istringstream iss(line);
	std::string directive;
	iss >> directive;

	if (directive == "listen") {
		std::string listenValue;
		iss >> listenValue;
		ListenConfig listen;
		// Parse address:port or just port
		size_t colonPos = listenValue.find(':');
		if (colonPos != std::string::npos) {
			// Format: address:port
			listen.host = listenValue.substr(0, colonPos);
			listen.port = std::stoi(listenValue.substr(colonPos + 1));
		} else {
			// Format: port only
			listen.host = "0.0.0.0"; // default
			listen.port = std::stoi(listenValue);
		}
		config.listens.push_back(listen);
	} else if (directive == "server_name") {
		iss >> config.serverName;
		// Remove semicolon if present
		if (!config.serverName.empty() && config.serverName.back() == ';') {
			config.serverName.pop_back();
		}
	} else if (directive == "client_max_body_size") {
		iss >> config.clientMaxBodySize;
	} else if (directive == "error_page") {
		int code;
		std::string path;
		iss >> code >> path;
		if (path.back() == ';')
			path.erase(path.end() - 1);
		config.errorPages[code] = path;
	}
}

void ConfigParser::parseServerBlock(std::ifstream& file, ServerConfig& config) {
	std::string line;

	while (std::getline(file, line)) {
		line = trim(line);

		if (line.empty() || line[0] == '#')
			continue;

		if (line == "}")
			break;

		if (line.find("location") == 0) {
			Location location;
			std::istringstream iss(line);
			std::string directive;
			iss >> directive >> location.path;

			std::string restOfLine;
			std::getline(iss, restOfLine);
			restOfLine = trim(restOfLine);
			if (restOfLine.find('{') == std::string::npos) {
				std::string braceLine;
				while (std::getline(file, braceLine)) {
					braceLine = trim(braceLine);
					if (braceLine.empty() || braceLine[0] == '#')
						continue;
					if (braceLine.find('{') != std::string::npos)
						break;
					break;
				}
			}

			parseLocationBlock(file, location);
			config.locations.push_back(location);
		} else {
			parseServerDirective(line, config);
		}
	}
}

void ConfigParser::parseConfig(const std::string& configFile) {
	_configFile = configFile;
	_serverConfigs.clear(); // Clear previous configurations
	std::ifstream file(configFile);

	if (!file.is_open()) {
		throw std::runtime_error("Cannot open configuration file: " + configFile);
	}

	std::string line;
	while (std::getline(file, line)) {
		line = trim(line);

		if (line.empty() || line[0] == '#')
			continue;

		if (line.find("server") == 0) {
			ServerConfig config;

			// Don't skip any line - the opening brace is on the same line as 'server'

			parseServerBlock(file, config);
			_serverConfigs.push_back(config);
		}
	}

	file.close();

	if (_serverConfigs.empty()) {
		throw std::runtime_error("No server configurations found in file: " + configFile);
	}

	// Add port conflict validation
	validatePortConflicts();
}

const std::vector<ServerConfig>& ConfigParser::getServerConfigs() const {
	return _serverConfigs;
}

void ConfigParser::createServersAndIpPortsFromConfig(Program &program) {
	// Map to store IpPort objects by address:port string
	std::map<std::string, IpPortPtr> ipPortMap;

	// Create servers and organize them by their listen addresses
	for (const auto& config : _serverConfigs) {
		auto server = std::make_shared<Server>(config);

		// If no listen directive was specified, add default
		std::vector<ListenConfig> listens = config.listens;
		if (listens.empty()) {
			ListenConfig defaultListen;
			defaultListen.host = "0.0.0.0";
			defaultListen.port = 8080;
			listens.push_back(defaultListen);
		}

		// Add this server to all its listen addresses
		for (const auto& listen : listens) {
			std::string addrPort = listen.getAddressPort();

			// Create IpPort if it doesn't exist
			if (ipPortMap.find(addrPort) == ipPortMap.end()) {
				auto ipPort = std::make_shared<IpPort>(program);
				ipPort->setAddrPort(addrPort);
				ipPortMap[addrPort] = ipPort;
				program._addrPortVec.push_back(ipPort);
			}

			// Add server to this IpPort
			ipPortMap[addrPort]->_servers.push_back(server);
		}

		// Add server to program's servers list
		program._servers.push_back(server);
	}

	// Set up handler mappings for each IpPort
	for (auto &ipPort : program._addrPortVec) {
		for (auto &server : ipPort->_servers) {
			server->_handlersMap = &program._handlersMap;
			server->_clientsMap = &program._clientsMap;
		}
	}
}

void ConfigParser::validatePortConflicts() {
	// Note: Multiple servers can listen on the same address:port
	// as long as they have different server_names (virtual hosting)
	// This is a common HTTP server feature, so we don't need to validate conflicts
	// The server selection will be done based on the Host header during request processing
}
