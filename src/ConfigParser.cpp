#include "ConfigParser.hpp"

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
	return result;
}

void ConfigParser::parseLocationDirective(const std::string& line, Location& location) {
	std::istringstream iss(line);
	std::string directive;
	iss >> directive;

	std::string rest;
	std::getline(iss, rest);
	rest = trim(rest);
	if (!rest.empty() && rest.back() == ';') rest.pop_back();

	auto getFirstToken = [&](const std::string &s) -> std::string {
		if (s.empty()) return "";
		size_t pos = s.find(' ');
		return (pos == std::string::npos) ? s : s.substr(0, pos);
	};

	if (directive == "root") {
		location.root = getFirstToken(rest);
		if (location.root.empty())
			throw std::runtime_error("Empty root");
	} else if (directive == "upload_dir") {
		location.uploadDir = getFirstToken(rest);
		if (location.uploadDir.empty())
			throw std::runtime_error("Empty upload_dir");
	} else if (directive == "index") {
		location.index = getFirstToken(rest);
	} else if (directive == "allow_methods") {
		location.allowedMethods = parseHttpMethods(rest);
	} else if (directive == "autoindex") {
		location.autoindex = (getFirstToken(rest) == "on");
	} else if (directive == "return") {
		std::istringstream returnStream(rest);
		std::string codeStr, url;
		returnStream >> codeStr >> url;

		try {
			int code = std::stoi(codeStr);
			if (code >= 300 && code < 400 && !url.empty()) {
				location.redirectCode = code;
				location.redirectUrl = url;
				location.isRedirected = true;
			}
		} catch (...) {
				throw std::runtime_error("Invalid redirection");
		}
	} else if (directive == "cgi") {
		std::string token = getFirstToken(rest);
		if (!token.empty() && token.back() == ';')
			token.pop_back();
		if (token == "on") {
			location.isCgi = true;
		}
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

		size_t colonPos = listenValue.find(':');
		if (colonPos != std::string::npos) {
			listen.host = listenValue.substr(0, colonPos);
			listen.port = std::stoi(listenValue.substr(colonPos + 1));
		} else {
			listen.host = "0.0.0.0";
			listen.port = std::stoi(listenValue);
		}
		if (listen.port <= 0)
			throw std::runtime_error("Invalid port");
		config.listens.push_back(listen);
	} else if (directive == "server_name") {
		iss >> config.serverName;
		if (!config.serverName.empty() && config.serverName.back() == ';')
			config.serverName.pop_back();
	} else if (directive == "client_max_body_size") {
		long long temp;
		iss >> temp;
		if (temp <= 0)
			throw std::runtime_error("Invalid body size");
		config.clientMaxBodySize = temp;
	} else if (directive == "error_page") {
		int code;
		std::string path;
		iss >> code >> path;
		if (!path.empty() && path.back() == ';')
			path.pop_back();
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

			if (line.find('{') == std::string::npos) {
				std::string braceLine;
				while (std::getline(file, braceLine)) {
					if (trim(braceLine).find('{') != std::string::npos)
						break;
				}
			}

			parseLocationBlock(file, location);
			config.locations.push_back(location);
		} else {
			fulfillDefaultErrorPages(config);
			parseServerDirective(line, config);
		}
	}
}

void ConfigParser::parseConfig(const std::string& configFile) {
	_serverConfigs.clear();
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
			parseServerBlock(file, config);
			_serverConfigs.push_back(config);
		}
	}

	file.close();

	if (_serverConfigs.empty()) {
		throw std::runtime_error("No server configurations found in file: " + configFile);
	}

}

const std::vector<ServerConfig>& ConfigParser::getServerConfigs() const {
	return _serverConfigs;
}

void ConfigParser::createServersAndIpPortsFromConfig(Program &program) {
	std::map<std::string, IpPortPtr> ipPortMap;

	for (const auto& config : _serverConfigs) {
		auto server = std::make_shared<Server>(config);

		std::vector<ListenConfig> listens = config.listens;
		if (listens.empty()) {
			ListenConfig defaultListen;
			defaultListen.host = "0.0.0.0";
			defaultListen.port = 8080;
			listens.push_back(defaultListen);
		}

		for (const auto& listen : listens) {
			std::string addrPort = listen.getAddressPort();

			if (ipPortMap.find(addrPort) == ipPortMap.end()) {
				auto ipPort = std::make_shared<IpPort>(program);
				ipPort->setAddrPort(addrPort);
				ipPortMap[addrPort] = ipPort;
				program.getAddrPortVec().push_back(ipPort);
			}
			else
			{
				throw std::runtime_error("Several the same configuration");
			}

			ipPortMap[addrPort]->getServers().push_back(server);
		}
		program.getServers().push_back(server);
	}
}

void ConfigParser::fulfillDefaultErrorPages(ServerConfig& config) {
	static const int codes[] = {400, 404, 405, 408, 413, 415, 500, 501, 505};
	for (int code : codes) {
		if (config.errorPages.find(code) == config.errorPages.end()) {
			config.errorPages[code] = DEFAULT_ERROR_DIR + std::to_string(code) + ".html";
		}
	}
}


