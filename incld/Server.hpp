#pragma once

#include <cstring>
#include <map>
#include <vector>
#include <memory>

#include "webserv.hpp"
#include "IEpollFdOwner.hpp"

enum class ErrorPage {
	ERR_404,
	MAX_ERRS
};

struct ServerConfig;
struct Location;
class ClientHandler;

class Server {
private:

	std::string					_serverName;
	std::string					_host;
	std::string					_port;

	size_t						_clientBodySize;
	std::map<int, std::string>	_errorPages;
	std::vector<Location>		_locations;

	int _sockfd;

	const Location* findLocationForPath(const std::string& path) const;

public:
	FdClientMap					*_clientsMap;
	FdEpollOwnerMap				*_handlersMap;

	Server(const ServerConfig& config);
	~Server();

	Server(const Server&) = delete;
	Server& operator=(const Server&) = delete;

	bool		areHeadersValid(ClientPtr &client);
	bool		isMethodAllowed(ClientPtr &client, const Location* matchedLocation);
	bool		isBodySizeValid(ClientPtr &client);
	std::string	findFile(ClientPtr &client, const std::string& path, const Location* matchedLocation);
	std::string	getCustomErrorPage(int statusCode);

	void	setHost(std::string host);
	void	setPort(std::string port);
	void	setServerName(const std::string& name);
	void	setClientBodySize(size_t size);
	void	setErrorPages(const std::map<int, std::string>& errorPages);
	void	setLocations(const std::vector<Location>& locations);

	std::string&						getHost();
	std::string&						getPort();
	const std::string&					getServerName() const;
	size_t								getClientBodySize() const;
	const std::map<int, std::string>&	getErrorPages() const;
	const std::vector<Location>&		getLocations() const;
	int									getSockfd() const;
};

