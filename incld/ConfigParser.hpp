#pragma once

#include "webserv.hpp"

#include <map>
#include <set>
#include <vector>
#include <string>
#include <memory>

struct Location {
	std::string path;
	std::string root;
	std::string index;
	int allowedMethods = static_cast<int>(HttpMethod::GET);
	bool autoindex = false;
	int redirectCode = 0;
	std::string redirectUrl;
	std::string cgiExtension;
	std::string cgiPath;
};

struct ListenConfig {
	std::string host = "localhost";
	int port = 8080;
	std::string getAddressPort() const { return host + ":" + std::to_string(port); }
};

struct ServerConfig {
	std::vector<ListenConfig> listens;
	std::string serverName;
	size_t clientMaxBodySize = 1000000;
	std::map<int, std::string> errorPages;
	std::vector<Location> locations;
	
	std::string getHost() const { return listens.empty() ? "localhost" : listens[0].host; }
	int getPort() const { return listens.empty() ? 8080 : listens[0].port; }
};

class ConfigParser {
private:
	std::vector<ServerConfig> _serverConfigs;
	std::string _configFile;

	void parseServerBlock(std::ifstream& file, ServerConfig& config);
	void parseLocationBlock(std::ifstream& file, Location& location);
	void parseServerDirective(const std::string& line, ServerConfig& config);
	void parseLocationDirective(const std::string& line, Location& location);

	std::string trim(const std::string& str);
	std::vector<std::string> split(const std::string& str, char delimiter);
	int parseHttpMethods(const std::string& methods);
	void validatePortConflicts();

public:
	ConfigParser() = default;
	~ConfigParser() = default;

	void parseConfig(const std::string& configFile);
	const std::vector<ServerConfig>& getServerConfigs() const;
	void createServersAndIpPortsFromConfig(Program &program);
};
