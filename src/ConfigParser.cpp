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

	if (directive == "root") {
		iss >> location.root;
	} else if (directive == "index") {
		iss >> location.index;
	} else if (directive == "allow_methods") {
		std::string methods;
		std::getline(iss, methods);
		location.allowedMethods = parseHttpMethods(trim(methods));
	} else if (directive == "autoindex") {
		std::string value;
		iss >> value;
		location.autoindex = (value == "on");
	} else if (directive == "return") {
		std::string code, url;
		iss >> code >> url;
		location.redirect = code + " " + url;
	} else if (directive == "cgi_extension") {
		iss >> location.cgiExtension;
	} else if (directive == "cgi_path") {
		iss >> location.cgiPath;
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
		iss >> config.port;
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

			// Skip the opening brace line
			std::string braceLine;
			std::getline(file, braceLine);

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

void ConfigParser::createServersFromConfig(Program &program) {
	for (const auto& config : _serverConfigs) {
		auto server = std::make_unique<Server>(config);
		program._servers.push_back(std::move(server));
	}
}

void ConfigParser::validatePortConflicts() {
	std::map<int, std::string> usedPorts;

	for (const auto& config : _serverConfigs) {
		auto it = usedPorts.find(config.port);
		if (it != usedPorts.end()) {
			throw std::runtime_error("Port conflict detected: Port " +
				std::to_string(config.port) + " is used by multiple servers (" +
				it->second + " and " + config.serverName + ")");
		}
		usedPorts[config.port] = config.serverName.empty() ?
			("server_" + std::to_string(config.port)) : config.serverName;
	}
}
