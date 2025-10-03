#include "Server.hpp"
#include "ClientHanlder.hpp"
#include <sstream>

void Server::RemoveClientHandler(size_t index) {
	if (index >= _clients.size()) return;

	// Simply remove the client at the index - unique_ptr will automatically clean up
	_clients.erase(_clients.begin() + index);

	// Update indices for remaining clients
	for (size_t i = index; i < _clients.size(); ++i) {
		_clients[i]->setIndex(i);
	}

	std::cout << "Size after removing: " << _clients.size() << std::endl;
}

void Server::handleEpollEvent(epoll_event& ev, int epoll_fd, int eventFd) {
	std::cout << "Accepting new connection..." << std::endl;

	try {
		auto new_client = std::make_unique<ClientHandler>(*this);
		new_client->acceptConnect(_sockfd, epoll_fd);
		new_client->setIndex(_clients.size());
		_clients.push_back(std::move(new_client));
		std::cout << "New connection was accepted" << std::endl;
	}
	catch(const std::exception& e) {
		std::cout << "Connection failed: " << e.what() << std::endl;
	}
}

void Server::prepareSockFd(addrinfo& hints, addrinfo* server) {
	int err = getaddrinfo(_host.c_str(), _port.c_str(), &hints, &server);
	if (err)
		THROW(gai_strerror(err));

	int i = 0;
	for (addrinfo* p = server; p; p = p->ai_next, i++) {}
	std::cout << "addrinfo nodes: " << i << std::endl;

	_sockfd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
	if (_sockfd == -1)
		THROW_ERRNO("socket");

	int opt = 1;
	err = setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (err)
		THROW_ERRNO("setsockopt(SO_REUSEADDR)");

	err = bind(_sockfd, server->ai_addr, server->ai_addrlen);
	if (err)
		THROW_ERRNO("bind");

	err = listen(_sockfd, QUEUE_SIZE);
	if (err)
		THROW_ERRNO("listen");
}

// Setters
void Server::setHost(std::string host) {
	_host = std::move(host);
}

void Server::setPort(std::string port) {
	_port = std::move(port);
}

void Server::setServerName(const std::string& name) {
	_serverName = name;
}

void Server::setClientBodySize(size_t size) {
	_clientBodySize = size;
}

void Server::setErrorPages(const std::map<int, std::string>& errorPages) {
	_errorPages = errorPages;
}

void Server::setLocations(const std::vector<Location>& locations) {
	_locations = locations;
}

// Getters
std::string& Server::getHost() {
	return _host;
}

std::string& Server::getPort() {
	return _port;
}

const std::string& Server::getServerName() const {
	return _serverName;
}

size_t Server::getClientBodySize() const {
	return _clientBodySize;
}

const std::map<int, std::string>& Server::getErrorPages() const {
	return _errorPages;
}

const std::vector<Location>& Server::getLocations() const {
	return _locations;
}

int Server::getSockfd() const {
	return _sockfd;
}

size_t Server::getSizeClients() const {
	return _clients.size();
}

// Helper function to convert int to string
static std::string intToString(int value) {
	return std::to_string(value);
}

// Constructors
Server::Server(const ServerConfig& config)
	: _serverName(config.serverName),
	_host(config.host),
	_port(intToString(config.port)),
	_clientBodySize(config.clientMaxBodySize),
	_errorPages(config.errorPages),
	_locations(config.locations) {
}

// Move constructor
Server::Server(Server&& other) noexcept
	: _serverName(std::move(other._serverName)),
	_host(std::move(other._host)),
	_port(std::move(other._port)),
	_clientBodySize(other._clientBodySize),
	_errorPages(std::move(other._errorPages)),
	_locations(std::move(other._locations)),
	_clients(std::move(other._clients)),
	_sockfd(other._sockfd) {
	other._sockfd = -1;
}

// Move assignment
Server& Server::operator=(Server&& other) noexcept {
	if (this != &other) {
		// Close current socket if open
		if (_sockfd != -1) {
			close(_sockfd);
		}

		_serverName = std::move(other._serverName);
		_host = std::move(other._host);
		_port = std::move(other._port);
		_clientBodySize = other._clientBodySize;
		_errorPages = std::move(other._errorPages);
		_locations = std::move(other._locations);
		_clients = std::move(other._clients);
		_sockfd = other._sockfd;

		other._sockfd = -1;
	}
	return *this;
}

Server::~Server() {
	// Smart pointers automatically clean up when vector is destroyed
	if (_sockfd != -1)
		close(_sockfd);
}

