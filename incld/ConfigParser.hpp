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
	std::string redirect;
	std::string cgiExtension;
	std::string cgiPath;
};

struct ServerConfig {
	std::string host = "localhost";
	int port = 8080;
	std::string serverName;
	size_t clientMaxBodySize = 1000000;
	std::map<int, std::string> errorPages;
	std::vector<Location> locations;
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
	void	createServersFromConfig(Program &program);
};
