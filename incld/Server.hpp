#pragma once

#include <cstring>
#include <map>
#include <vector>
#include <memory>
#include <sstream>
#include <fstream>

#include <sys/stat.h>

#include "webserv.hpp"
#include "HttpException.hpp"
#include "CustomException.hpp"
#include "IEpollFdOwner.hpp"
#include "utils.hpp"
#include "IpPort.hpp"
#include "Client.hpp"

#define HTTP_VERSION "HTTP/1.1"

class Server
{
	private:
		std::string							_serverName;
		std::string							_host;
		std::string							_port;

		size_t								_clientBodySize;
		std::map<int, std::string>			_errorPages;
		std::vector<Location>				_locations;

		const Location*						findLocationForPath(std::string& path);

		bool								isMethodAllowed(ClientPtr &client, const Location* matchedLocation);
		bool								isRedirected(ClientPtr &client, const Location* matchedLocation);
		bool								isBodySizeValid(ClientPtr &client);
	public:
		Server(const ServerConfig& config);
		~Server();

		bool								areHeadersValid(ClientPtr &client);
		std::string							findFile(ClientPtr &client, const std::string& path, const Location* matchedLocation);
		std::string							getCustomErrorPage(int statusCode);

		void								setHost(std::string host);
		void								setPort(std::string port);
		void								setServerName(const std::string& name);
		void								setClientBodySize(size_t size);
		void								setErrorPages(const std::map<int, std::string>& errorPages);
		void								setLocations(const std::vector<Location>& locations);

		std::string&						getHost();
		std::string&						getPort();
		const std::string&					getServerName();
		size_t								getClientBodySize();
		const std::map<int, std::string>&	getErrorPages();
		const std::vector<Location>&		getLocations();
};

