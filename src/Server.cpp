#include "Server.hpp"
#include "ClientHanlder.hpp"
#include <sstream>

std::string Server::findIndexFile(ClientPtr &client, const std::string& path)
{
	// Get server configuration
	const std::vector<Location>& locations = client->_ownerServer->getLocations();

	// Find matching location
	const Location* matchedLocation = nullptr;
	size_t longestMatch = 0;

	for (const auto& location : locations) {
		if (path.find(location.path) == 0 && location.path.length() > longestMatch) {
			matchedLocation = &location;
			longestMatch = location.path.length();
		}
	}

	std::string indexFile = "index.html"; // Default
	if (matchedLocation && !matchedLocation->index.empty()) {
		// Parse the first index file from the space-separated list
		std::string indexFiles = matchedLocation->index;

		// Remove trailing semicolon if present
		if (!indexFiles.empty() && indexFiles.back() == ';') {
			indexFiles.pop_back();
		}

		// Find first space or use entire string
		size_t spacePos = indexFiles.find(' ');
		if (spacePos != std::string::npos) {
			indexFile = indexFiles.substr(0, spacePos);
		} else {
			indexFile = indexFiles;
		}

		// Trim any remaining whitespace
		while (!indexFile.empty() && (indexFile.front() == ' ' || indexFile.front() == '\t')) {
			indexFile.erase(0, 1);
		}
		while (!indexFile.empty() && (indexFile.back() == ' ' || indexFile.back() == '\t')) {
			indexFile.pop_back();
		}
	}

	return indexFile;
}

bool	Server::isBodySizeValid(ClientPtr &client)
{
	size_t headerStart = client->_buffer.find("Content-Length: ");
	if (headerStart == std::string::npos) {
		return false; // No Content-Length header, assume valid
	}

	headerStart += 16; // Move past "Content-Length: "
	size_t headerEnd = client->_buffer.find("\r\n", headerStart);

	std::string contentLengthStr = client->_buffer.substr(headerStart, headerEnd - headerStart);
	try
	{
		size_t contentLength = std::stoul(contentLengthStr);
		size_t maxBodySize = getClientBodySize();

		std::cout << "DEBUG: Content-Length: " << contentLength << ", Max allowed: " << maxBodySize << std::endl;
		return contentLength <= maxBodySize;
	} catch (const std::exception& e)
	{
		std::cout << "DEBUG: Error parsing Content-Length: " << e.what() << std::endl;
		return false;
	}
}

bool	Server::isMethodAllowed(ClientPtr &client, std::string& path)
{
	const std::vector<Location>& locations = getLocations();
	std::cout << "DEBUG: Found " << locations.size() << " locations" << std::endl;

	const Location* matchedLocation = nullptr;
	size_t longestMatch = 0;

	for (const auto& location : locations)
	{
		std::cout << "DEBUG: Checking location: " << location.path << " against path: " << path << std::endl;
		if (path.find(location.path) == 0 && location.path.length() > longestMatch) {
			matchedLocation = &location;
			longestMatch = location.path.length();
			std::cout << "DEBUG: Matched location: " << location.path << std::endl;
			break;
		}
	}

	if (!matchedLocation)
	{
		std::cout << "DEBUG: No specific location found, allowing GET by default" << std::endl;
		return client->_httpMethod == "GET";
	}

	std::cout << "DEBUG: Using location: " << matchedLocation->path << " with allowedMethods: " << matchedLocation->allowedMethods << std::endl;

	int methodFlag = 0;
	if (client->_httpMethod == "GET") methodFlag = static_cast<int>(HttpMethod::GET);
	else if (client->_httpMethod == "POST") methodFlag = static_cast<int>(HttpMethod::POST);
	else if (client->_httpMethod == "DELETE") methodFlag = static_cast<int>(HttpMethod::DELETE);
	else {
		std::cout << "DEBUG: Unknown method: " << client->_httpMethod << std::endl;
		return false;
	}

	bool allowed = (matchedLocation->allowedMethods & methodFlag) != 0;
	std::cout << "DEBUG: Method " << client->_httpMethod << " (flag: " << methodFlag << ") allowed: " << (allowed ? "YES" : "NO") << std::endl;

	return allowed;
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
// Constructors + Destructor

Server::~Server() {
	if (_sockfd != -1)
		close(_sockfd);
}

Server::Server(const ServerConfig& config)
	: _serverName(config.serverName),
	_host(config.host),
	_port(std::to_string(config.port)),
	_clientBodySize(config.clientMaxBodySize),
	_errorPages(config.errorPages),
	_locations(config.locations),
	_sockfd{-1} {
}

