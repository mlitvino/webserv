#include "Server.hpp"
#include "ClientHanlder.hpp"
#include <sstream>

// Helper function to convert int to string
static std::string intToString(int value) {
	return std::to_string(value);
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

// Constructors + Destructor

Server::~Server() {
	if (_sockfd != -1)
		close(_sockfd);
}

Server::Server(const ServerConfig& config)
	: _serverName(config.serverName),
	_host(config.host),
	_port(intToString(config.port)),
	_clientBodySize(config.clientMaxBodySize),
	_errorPages(config.errorPages),
	_locations(config.locations) {
}

