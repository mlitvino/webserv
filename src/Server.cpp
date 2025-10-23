#include "Server.hpp"

bool	Server::areHeadersValid(ClientPtr &client)
{
	std::cout << "Validating headers..." << std::endl;
	const Location	*matchedLocation = findLocationForPath(client->getHttpPath());

	if (!matchedLocation)
		THROW_HTTP(400, "No matched location");

	if (client->getHttpVersion() != HTTP_VERSION)
		THROW_HTTP(505, "HTTP Version Not Supported");

	if (isRedirected(client, matchedLocation))
	{
		client->getIpPort().generateResponse(client, "", client->getRedirectCode());
		return true;
	}

	if (client->getContentLen() > 0 && client->isChunked())
		THROW_HTTP(415, "Chunked body and content-length are presented");

	if (client->getContentLen()< 0)
		THROW_HTTP(400, "Invalid content-length");

	if (!isMethodAllowed(client, matchedLocation))
		THROW_HTTP(405, "Method not allowed");

	if (!isBodySizeValid(client))
		THROW_HTTP(413, "Content too large");

	if (client->getContentType().find(CONTENT_TYPE_MULTIPART) != std::string::npos
		&& client->getMultipartBoundary().empty())
	{
		THROW_HTTP(400, "Multipart boundary missing");
	}

	if (client->getHttpMethod()== "POST"
		&& client->getContentType().find(CONTENT_TYPE_MULTIPART) == std::string::npos
		&& client->getContentType().find(CONTENT_TYPE_APP_FORM) == std::string::npos)
	{
		THROW_HTTP(415, "Unsupported media type");
	}

	std::string	path = findFile(client, client->getHttpPath(), matchedLocation);
	client->setResolvedPath(path);

	std::cout << "CONETENT TYPE: " <<  client->getContentType() << std::endl;
	std::cout << "DEBUG: type file: " << (client->getFileType() == FileType::CGI_SCRIPT ? "Cgi" : "not cgi") << std::endl;

	if (client->getResolvedPath().empty())
		THROW_HTTP(404, "Not Found");

	if (client->getFileType() == FileType::CGI_SCRIPT)
	{
		size_t		dot = client->getResolvedPath().find_last_of(".");
		std::string	ext = client->getResolvedPath().substr(dot);

		if (ext != PYTHON_EXT && ext != PHP_EXT)
			THROW_HTTP(400, "Unsupported cgi");
		client->getCgi().setUploadDir(matchedLocation->uploadDir);
	}

	std::cout << "Validating headers is done" << std::endl;
	return true;
}

bool	Server::isRedirected(ClientPtr &client, const Location* matchedLocation)
{
	int					code = matchedLocation->redirectCode;
	const std::string	&url = matchedLocation->redirectUrl;

	if (!matchedLocation->isRedirected)
		return false;

	if (code <= 300 || code > 307 || url.empty())
		THROW_HTTP(400, "Bad redirection");

	client->setRedirectedUrl(matchedLocation->redirectUrl);
	client->setRedirectCode(matchedLocation->redirectCode);
	return true;
}

const Location* Server::findLocationForPath(std::string& path)
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

	if (client->getHttpMethod()== "POST" && matched->isCgi == false)
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
				if (matched->isCgi == true)
				{
					client->setFileType(FileType::CGI_SCRIPT);
				}
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
		client->setFileType(FileType::DIRECTORY);
		return fsDir;
	}

	return "";
}

bool	Server::isBodySizeValid(ClientPtr &client)
{
	return client->getContentLen()<= getClientBodySize();
}

bool	Server::isMethodAllowed(ClientPtr &client, const Location* matchedLocation)
{
	std::cout << "DEBUG: Using location: " << matchedLocation->path << " with allowedMethods: " << matchedLocation->allowedMethods << std::endl;

	int methodFlag = 0;
	if (client->getHttpMethod()== "GET")
		methodFlag = static_cast<int>(HttpMethod::GET);
	else if (client->getHttpMethod()== "POST")
		methodFlag = static_cast<int>(HttpMethod::POST);
	else if (client->getHttpMethod()== "DELETE")
		methodFlag = static_cast<int>(HttpMethod::DELETE);
	else
	{
		std::cout << "DEBUG: Unknown method: " << client->getHttpMethod()<< std::endl;
		return false;
	}

	bool allowed = (matchedLocation->allowedMethods & methodFlag) != 0;
	std::cout << "DEBUG: Method " << client->getHttpMethod()<< " (flag: " << methodFlag << ") allowed: " << (allowed ? "YES" : "NO") << std::endl;

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

const std::string& Server::getServerName() {
	return _serverName;
}

size_t Server::getClientBodySize() {
	return _clientBodySize;
}

const std::map<int, std::string>& Server::getErrorPages() {
	return _errorPages;
}

const std::vector<Location>& Server::getLocations() {
	return _locations;
}

// Constructors + Destructor

Server::~Server()
{}

Server::Server(const ServerConfig& config)
	: _serverName(config.serverName),
	_host(config.getHost()),
	_port(std::to_string(config.getPort())),
	_clientBodySize(config.clientMaxBodySize),
	_errorPages(config.errorPages),
	_locations(config.locations)

{}

