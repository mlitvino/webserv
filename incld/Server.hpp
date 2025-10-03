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
	std::string _serverName;
	std::string _host;
	std::string _port;

	size_t _clientBodySize = 1024 * 1024; // 1MB default
	std::map<int, std::string> _errorPages;
	std::vector<Location> _locations;

	std::vector<std::unique_ptr<ClientHandler>> _clients;

	int _sockfd = -1;

public:
	Server() = default;
	Server(const ServerConfig& config);
	~Server();

	// Delete copy constructor and assignment to prevent accidental copying
	Server(const Server&) = delete;
	Server& operator=(const Server&) = delete;

	// Move constructor and assignment
	// Server(Server&& other) noexcept;
	// Server& operator=(Server&& other) noexcept;

	void prepareSockFd(addrinfo& hints, addrinfo* server);
	void setHost(std::string host);
	void setPort(std::string port);
	void setServerName(const std::string& name);
	void setClientBodySize(size_t size);
	void setErrorPages(const std::map<int, std::string>& errorPages);
	void setLocations(const std::vector<Location>& locations);

	std::string& getHost();
	std::string& getPort();
	const std::string& getServerName() const;
	size_t getClientBodySize() const;
	const std::map<int, std::string>& getErrorPages() const;
	const std::vector<Location>& getLocations() const;
	int getSockfd() const;
	size_t getSizeClients() const;
	void RemoveClientHandler(size_t index);

};

