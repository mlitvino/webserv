#include "Server.hpp"
#include <sstream>
#include <sys/stat.h>
#include <fstream>

bool	Server::areHeadersValid(ClientPtr &client)
{
	std::cout << "Validating headers..." << std::endl;

	if (client->_contentLen != 0 && client->_chunked)
	{
		std::cout << "Chunked body and content's length are presented at the same time" << std::endl;
		//generateResponse(client, "", 405);
		return false;
	}

	if (!isMethodAllowed(client, client->_httpPath))
	{
		std::cout << "Invalid Method" << std::endl;
		//generateResponse(client, "", 405);
		return false;
	}

	if (!isBodySizeValid(client))
	{
		std::cout << "Invalid Content-Length" << std::endl;
		//generateResponse(client, "", 413);
		return false;
	}

	// isPathAllowed

	std::cout << "Validating headers is done" << std::endl;
	return true;
}

std::string	Server::findFile(ClientPtr &client, const std::string& path)
{
	const std::vector<Location>& locations = getLocations();
	const Location* matched = nullptr;
	size_t bestLen = 0;

	for (auto it = locations.begin(); it != locations.end(); ++it)
	{
		const std::string &locPath = it->path;
		if (path.size() >= locPath.size() &&
			path.compare(0, locPath.size(), locPath) == 0)
		{
			bool validBoundary = (locPath == "/") ||
				(path.size() == locPath.size()) ||
				(path.size() > locPath.size() && (path[locPath.size()] == '/'));
			if (validBoundary && locPath.length() > bestLen)
			{
				matched = &(*it);
				bestLen = it->path.length();
			}
		}
	}

	if (!matched)
		return "";

	std::vector<std::string> indexFiles;
	if (!matched->index.empty())
	{
		std::istringstream iss(matched->index);
		std::string token;
		while (iss >> token)
		{
			if (!token.empty() && token.back() == ';') token.pop_back();
			indexFiles.push_back(token);
		}
	}
	else
		indexFiles.push_back("index.html");

	std::string	docRoot = matched->root;
	if (docRoot.empty())
		docRoot = "web/www";
	while (!docRoot.empty() && (docRoot.back() == '/' || docRoot.back() == '\\'))
		docRoot.pop_back();

	std::cout << "FindFile: path " << path << std::endl;
	std::cout << "FindFile: matched path " << matched->path << std::endl;

	std::string suffix = path.substr(matched->path.size());
	while (!suffix.empty() && (suffix.front() == '/' || suffix.front() == '\\'))
		suffix.erase(0, 1);

	std::string fsPath = docRoot;
	if (!suffix.empty())
		fsPath += "/" + suffix;

	std::cout << "FindFile: fsPath " << fsPath << std::endl;

	if (!suffix.empty() && path.back() != '/')
	{
		struct stat st;
		if (stat(fsPath.c_str(), &st) == 0)
		{
			if (S_ISREG(st.st_mode))
			{
				return fsPath;
			}
			else if (!S_ISDIR(st.st_mode))
			{
				THROW("not regular file or directory");
				return "";
			}
		}
		else if (errno == ENOENT && client->_httpMethod == "POST")
		{
			return fsPath;
		}
		else
		{
			THROW_ERRNO("stat");
			return "";
		}
	}

	std::string fsDir = fsPath.empty() ? docRoot : fsPath;
	if (fsDir.empty() || fsDir.back() != '/')
		fsDir += '/';

	for (auto it = indexFiles.begin(); it != indexFiles.end(); ++it)
	{
		std::string candidate = fsDir + *it;
		struct stat st;
		if (stat(candidate.c_str(), &st) == 0 && S_ISREG(st.st_mode))
			return candidate;
	}

	return "";
}

bool	Server::isBodySizeValid(ClientPtr &client)
{
	return client->_contentLen <= getClientBodySize();
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
		const std::string &locPath = location.path;
		if (path.size() >= locPath.size() &&
			path.compare(0, locPath.size(), locPath) == 0)
		{
			bool validBoundary = (locPath == "/") ||
				(path.size() == locPath.size()) ||
				(path.size() > locPath.size() && (path[locPath.size()] == '/'));
			if (validBoundary && locPath.length() > longestMatch) {
				matchedLocation = &location;
				longestMatch = location.path.length();
				std::cout << "DEBUG: Matched location: " << location.path << std::endl;
			}
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
	else
	{
		std::cout << "DEBUG: Unknown method: " << client->_httpMethod << std::endl;
		return false;
	}

	bool allowed = (matchedLocation->allowedMethods & methodFlag) != 0;
	std::cout << "DEBUG: Method " << client->_httpMethod << " (flag: " << methodFlag << ") allowed: " << (allowed ? "YES" : "NO") << std::endl;

	return allowed;
}

std::string	Server::getCustomErrorPage(int statusCode)
{
	const std::map<int, std::string>& errorPages = getErrorPages();
	auto it = errorPages.find(statusCode);
	if (it != errorPages.end())
	{
		return it->second;
	}
	return "";
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

