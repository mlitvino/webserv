#include "Server.hpp"
#include <sstream>
#include <sys/stat.h>
#include <fstream>

bool	Server::areHeadersValid(ClientPtr &client)
{
	std::cout << "Validating headers..." << std::endl;
	const Location	*matchedLocation = findLocationForPath(client->_httpPath);

	// optional: check HTTP version here

	if (!matchedLocation)
		THROW_HTTP(400, "No matched location");

	if (isRedirected(client, matchedLocation))
		return true;

	if (client->_httpPath.find("%") != std::string::npos)
		THROW_HTTP(400, "Unsupported encoded request");

	if (client->_contentLen != 0 && client->_chunked)
		THROW_HTTP(400, "Chunked body and content-length are presented");

	if (!isMethodAllowed(client, matchedLocation))
		THROW_HTTP(405, "Method not allowed");

	if (!isBodySizeValid(client))
		THROW_HTTP(413, "Content too large");

	if (client->_contentType.find(CONTENT_TYPE_MULTIPART) != std::string::npos
		&& client->_multipartBoundary.empty())
	{
		THROW_HTTP(400, "Multipart boundary missing");
	}

	if (client->_httpMethod == "POST"
		&& client->_contentType.find(CONTENT_TYPE_MULTIPART) == std::string::npos
		&& client->_contentType.find(CONTENT_TYPE_APP_FORM) == std::string::npos)
	{
		THROW_HTTP(415, "Unsupported media type");
	}

	client->_resolvedPath = findFile(client, client->_httpPath, matchedLocation);
	if (client->_resolvedPath.empty())
		THROW_HTTP(404, "Not Found");

	std::cout << "Validating headers is done" << std::endl;
	return true;
}

bool	Server::isRedirected(ClientPtr &client, const Location* matchedLocation)
{
	int					code = matchedLocation->redirectCode;
	const std::string	&url = matchedLocation->redirectUrl;
	if (code == 0 || url.empty())
		return false;

	std::string statusText;
	switch (code)
	{
		case 301: statusText = "Moved Permanently"; break;
		case 302: statusText = "Found"; break;
		case 303: statusText = "See Other"; break;
		case 307: statusText = "Temporary Redirect"; break;
		case 308: statusText = "Permanent Redirect"; break;
		default: statusText = "Redirect"; break;
	}

	std::string	response;
	response = "HTTP/1.1 " + std::to_string(code) + " " + statusText + "\r\n";
	response += "Location: " + url + "\r\n";
	response += "Connection: close\r\n";
	response += "Server: webserv/1.0\r\n\r\n";

	client->_state = ClientState::SENDING_RESPONSE;
	utils::changeEpollHandler(client->_handlersMap, client->_clientFd, client.get());
	client->_responseBuffer = std::move(response);
	return true;
}

const Location* Server::findLocationForPath(const std::string& path) const
{
	const std::vector<Location>&	locations = getLocations();
	const Location*					matched = nullptr;
	size_t							bestLen = 0;

	for (auto &location : locations)
	{
		const std::string &locPath = location.path;
		if (path.size() >= locPath.size() &&
			path.compare(0, locPath.size(), locPath) == 0)
		{
			bool validBoundary = (locPath == "/") || (path.size() == locPath.size())
								|| (path.size() > locPath.size() && (path[locPath.size()] == '/'));
			if (validBoundary && locPath.length() > bestLen)
			{
				matched = &location;
				bestLen = location.path.length();
			}
		}
	}
	return matched;
}

std::string	Server::findFile(ClientPtr &client, const std::string& path, const Location* matched)
{
	client->_isTargetDir = false;

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

	if (client->_httpMethod == "POST")
	{
		std::string fsDir = fsPath.empty() ? docRoot : fsPath;
		struct stat st{};
		if (stat(fsDir.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
			return fsDir;
		return "";
	}

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
				THROW_HTTP(400, "Not regular file or directory");
				return "";
			}
		}
		else
		{
			THROW_HTTP(404, "Not Found");
			return "";
		}
	}

	std::vector<std::string> indexFiles;
	if (!matched->index.empty())
	{
		std::istringstream iss(matched->index);
		std::string token;
		while (iss >> token)
		{
			if (!token.empty() && token.back() == ';')
				token.pop_back();
			indexFiles.push_back(token);
		}
	}
	else
	{
		indexFiles.push_back("index.html");
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

	if (matched->autoindex)
	{
		client->_isTargetDir = true;
		return fsDir;
	}

	return "";
}

bool	Server::isBodySizeValid(ClientPtr &client)
{
	return client->_contentLen <= getClientBodySize();
}

bool	Server::isMethodAllowed(ClientPtr &client, const Location* matchedLocation)
{
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
	_host(config.getHost()),
	_port(std::to_string(config.getPort())),
	_clientBodySize(config.clientMaxBodySize),
	_errorPages(config.errorPages),
	_locations(config.locations),
	_sockfd{-1} {
}

