#include "webserv.hpp"

void parser(Data& data, char* conf_file) {
	try {
		ConfigParser configParser;
		
		if (conf_file) {
			configParser.parseConfig(std::string(conf_file));
		} else {
			// Try default configuration file
			configParser.parseConfig(std::string(DEFAULT_CONF));
		}
		
		configParser.createServersFromConfig(data);
		
		// If no servers were created, create a default one
		if (data.servers.empty()) {
			auto default_server = std::make_unique<Server>();
			default_server->setHost(std::string(HOST));
			default_server->setPort(std::string(PORT));
			data.servers.push_back(std::move(default_server));
		}
	} catch (const std::exception& e) {
		std::cerr << "Configuration parsing error: " << e.what() << std::endl;
		std::cerr << "Using default configuration..." << std::endl;
		
		// Create default server as fallback
		auto default_server = std::make_unique<Server>();
		default_server->setHost(std::string(HOST));
		default_server->setPort(std::string(PORT));
		data.servers.push_back(std::move(default_server));
	}
}

