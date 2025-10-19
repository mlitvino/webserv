#include "IpPort.hpp"
#include "Cgi.hpp"
#include "PostRequestHandler.hpp"
#include <dirent.h>
#include <sys/stat.h>
#include <sstream>
#include <algorithm>
#include <cctype>

void	IpPort::OpenSocket(addrinfo &hints, addrinfo *_servInfo)
{
	int	err;

	size_t		delim = _addrPort.find_last_of(":");
	std::string	host = _addrPort.substr(0, delim);
	std::string	port = _addrPort.substr(delim + 1, _addrPort.size() - delim);

	err = getaddrinfo(host.c_str(), port.c_str(), &hints, &_servInfo);
	if (err == -1)
		THROW(gai_strerror(err));

	_sockFd = socket(_servInfo->ai_family, _servInfo->ai_socktype, _servInfo->ai_protocol);
	if (_sockFd == -1)
		THROW_ERRNO("socket");

	int opt = 1;

	err = setsockopt(_sockFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (err == -1)
		THROW_ERRNO("setsockopt(SO_REUSEADDR)");

	err = bind(_sockFd, _servInfo->ai_addr, _servInfo->ai_addrlen);
	if (err == -1)
		THROW_ERRNO("bind");

	err = listen(_sockFd, QUEUE_SIZE);
	if (err == -1)
		THROW_ERRNO("listen");
}

void	IpPort::handleEpollEvent(epoll_event &ev, int epollFd, int eventFd)
{
	if (eventFd == getSockFd())
	{
		std::cout << "Accepting new connection..." << std::endl;
		acceptConnection(ev, epollFd, eventFd);
		std::cout << "Connection was accepted" << std::endl;
	}
	else
	{
		ClientPtr	client = (*_clientsMap.find(eventFd)).second;
		if (ev.events & EPOLLIN)
		{
			try
			{
				if (client->_state == ClientState::READING_REQUEST)
				{
					//std::cout << "Accepting data from existing client->.." << std::endl;
					if (!client->readRequest())
						return closeConnection(eventFd);
					parseRequest(ev, epollFd, eventFd);
					//std::cout << "Accepting data is done" << std::endl;
				}
				else if (client->_state == ClientState::GETTING_BODY)
				{
					std::cout << "Continuing to read POST body..." << std::endl;
					if (!client->readRequest())
					{
						client->_postHandler.resetBodyState();
						return closeConnection(eventFd);
					}
					client->_postHandler.handlePostRequest(client, client->_httpPath);
				}
			}
			catch (std::bad_alloc &e)
			{
				std::cout << "ERROR: Infficient memory space" << std::endl;
				closeConnection(eventFd);
			}
			catch (HttpException &e)
			{
				client->_postHandler.resetBodyState();
				std::cout << "HttpException: " << e.what() << ", statusCode " << e.getStatusCode() << std::endl;
				generateResponse(client, "", e.getStatusCode());
			}
			catch (std::exception &e)
			{
				std::cout << "Exception: " << e.what() << std::endl;
				closeConnection(eventFd);
			}
		}
	}
}

void	IpPort::parseRequest(epoll_event &ev, int epollFd, int eventFd)
{
	ClientPtr	client = (*_clientsMap.find(eventFd)).second;

	std::cout << "---------" << std::endl;
	std::cout << "DEBUG: Buffer content: " << client->_buffer << std::endl;
	parseHeaders(client);
	std::cout << "Method: " << client->_httpMethod << ", Path: " << client->_httpPath << ", Version: " << client->_httpVersion << std::endl;
	assignServerToClient(client);

	client->_ownerServer->areHeadersValid(client);

	if (client->_state == ClientState::SENDING_RESPONSE)
	{
		std::cout << "Redirecting..." << std::endl;
		return;
	}

	if (client->_httpMethod == "GET")
	{
		handleGetRequest(client);
	}
	else if (client->_httpMethod == "POST")
	{
		client->_postHandler.resetBodyState();
		client->_postHandler.handlePostRequest(client, client->_httpPath);
	}
	else if (client->_httpMethod == "DELETE")
	{
		handleDeleteRequest(client);
	}
}

void	IpPort::parseHeaders(ClientPtr &client)
{
	size_t	headersEnd = client->_buffer.find("\r\n\r\n");
	if (headersEnd == std::string::npos)
		return;

	std::string			headers = client->_buffer.substr(0, headersEnd);
	std::istringstream	iss(headers);
	std::string			line;

	client->_contentLen = 0;
	client->_chunked = false;
	client->_keepAlive = false;
	client->_contentType.clear();
	client->_redirectedUrl.clear();

	std::getline(iss, line);
	size_t firstSpace = line.find(' ');
	size_t secondSpace = line.find(' ', firstSpace + 1);
	client->_httpMethod = line.substr(0, firstSpace);

	std::string	pathAndQuery = line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
	client->_httpPath = pathAndQuery;
	size_t		qpos = pathAndQuery.find("?_method=DELETE");
	if (qpos != std::string::npos)
	{
		client->_httpPath = pathAndQuery.substr(0, qpos);
		client->_httpMethod = "DELETE";
	}
	client->_httpVersion = line.substr(secondSpace + 1);

	while (std::getline(iss, line))
	{
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		size_t colon = line.find(':');
		if (colon == std::string::npos)
			continue ;
		std::string	name = line.substr(0, colon);
		std::string	value = line.substr(colon + 1);

		while (!value.empty() && isspace(value.front()))
			value.erase(0, 1);
		while (!value.empty() && isspace(value.back()))
			value.pop_back();

		if (name == "Host")
		{
			client->_hostHeader = value;
		}
		else if (name == "Content-Length")
		{
			client->_contentLen = std::stoul(value);
		}
		else if (name == "Content-Type")
		{
			client->_contentType = value;
			if (value.find(CONTENT_TYPE_MULTIPART) != std::string::npos)
			{
				size_t bpos = value.find("boundary=");
				if (bpos != std::string::npos)
				{
					size_t valpos = bpos + strlen("boundary=");
					client->_multipartBoundary = value.substr(valpos);
				}
			}
		}
		else if (name == "Transfer-Encoding")
		{
			if (value.find("chunked") != std::string::npos)
				client->_chunked = true;
		}
		else if (name == "Connection")
		{
			if (value.find("keep-alive") != std::string::npos)
				client->_keepAlive = true;
		}
	}

	client->_buffer.erase(0, headersEnd + 4);
}

void IpPort::handleGetRequest(ClientPtr &client)
{
	std::cout << "DEBUG: _resolvedPath returned: " << client->_resolvedPath << std::endl;
	generateResponse(client, client->_resolvedPath, 303);
}

void	IpPort::handleDeleteRequest(ClientPtr &client)
{
	std::cout << "DEBUG: _resolvedPath: " << client->_resolvedPath << std::endl;

	if (std::remove(client->_resolvedPath.c_str()) == 0)
	{
		std::cout << "DEBUG: File deleted successfully, generating 303 response" << std::endl;
		std::string dirPath = client->_httpPath.substr(0, client->_httpPath.find_last_of("/"));
		client->_redirectedUrl = "http://localhost:8080" + dirPath + ".html";
		generateResponse(client, "", 303);
	}
	else
	{
		THROW_HTTP(505, "Failed to delete file");
	}
}

void	IpPort::generateResponse(ClientPtr &client, std::string filePath, int statusCode)
{
	std::string	statusText;
	std::string	response;

	switch (statusCode)
	{
		case 200: statusText = "OK"; break;

		case 301: statusText = "Moved Permanently"; break;
		case 302: statusText = "Found"; break;
		case 303: statusText = "See Other"; break;
		case 307: statusText = "Temporary Redirect"; break;
		case 308: statusText = "Permanent Redirect"; break;

		case 404: statusText = "Not Found"; break;
		case 405: statusText = "Method Not Allowed"; break;
		case 413: statusText = "Payload Too Large"; break;

		case 500: statusText = "Internal Server Error"; break;
		default: statusText = "Unknown"; break;
	}

	if (statusCode >= 400)
		filePath = client->_ownerServer->getCustomErrorPage(statusCode);

	std::string	listingBuffer;
	size_t		contentLentgh = 0;
	if (!filePath.empty())
	{
		std::cout << "DEBUG: filepath name -> " << filePath << std::endl;
		if (client->_fileType == FileType::DIRECTORY)
		{
			listDirectory(client, listingBuffer);
			contentLentgh = listingBuffer.size();
		}
		else
		{
			client->openFile(filePath);
			if (client->_fileFd < 0)
			{
				std::cout << "DEBUG: Coudln't open filePath" << std::endl;
				statusCode = 500;
				statusText = "Internal Server Error";
			}
			contentLentgh = client->_fileSize;
		}
	}
	else
	{
		std::cout << "DEBUG: filePath is empty" << std::endl;
	}

	response = "HTTP/1.1 " + std::to_string(statusCode) + " " + statusText + "\r\n";
	response += formHeaders(client, filePath, contentLentgh, statusCode);

	if (!listingBuffer.empty())
		response += listingBuffer;

	client->_state = ClientState::SENDING_RESPONSE;
	utils::changeEpollHandler(_handlersMap, client->_clientFd, client.get());
	client->_responseBuffer = std::move(response);

	std::cout << "DEBUG: Response buffer size after generation: " << client->_responseBuffer.size() << std::endl;
	std::cout << "DEBUG: Response buffer: " << client->_responseBuffer << std::endl;
}


std::string	IpPort::formHeaders(ClientPtr &client, std::string &filePath, size_t contentLength, int statusCode)
{
	std::string	contentType;
	std::string	hdrs;

	if (client->_fileType == FileType::DIRECTORY)
	{
		contentType = "text/html";
	}
	else if (!filePath.empty() && client->_httpMethod == "GET")
	{
		size_t		lastSlash = filePath.find_last_of("/\\");
		std::string	fileName = (lastSlash == std::string::npos) ? filePath : filePath.substr(lastSlash + 1);
		size_t dot = fileName.find_last_of('.');
		std::string ext;
		if (dot != std::string::npos && dot + 1 < fileName.size())
			ext = fileName.substr(dot + 1);

		if (ext == "html" || ext == "htm")
			contentType = "text/html";
		else if (ext == "css")
			contentType = "text/css";
		else if (ext == "png")
			contentType = "image/png";
		else if (ext == "gif")
			contentType = "image/gif";
		else if (ext == "pdf")
			contentType = "application/pdf";
		else if (ext == "txt")
			contentType = "text/plain";

		if (!contentType.empty())
			hdrs += "Content-Type: " + contentType + "\r\n";

		if (ext != "html" && ext != "htm" && ext != "cgi" && ext != "ico")
			hdrs = "Content-Disposition: attachment; filename=\"" + fileName + "\"" + "\r\n";
	}
	if (!client->_redirectedUrl.empty())
		hdrs += "Location: " + client->_redirectedUrl + "\r\n";
	hdrs += "Content-Length: " + std::to_string(contentLength) + "\r\n";
	hdrs += "Server: webserv/1.0\r\n";
	hdrs += "Connection: " + std::string((client->_keepAlive ? "keep-alive" : "close")) + "\r\n";
	hdrs += "Cache-Control: no-cache\r\n";
	hdrs += "\r\n";
	return hdrs;
}

void	IpPort::assignServerToClient(ClientPtr &client)
{
	client->_ownerServer = _servers.front();

	std::string hostValue = client->_hostHeader;
	if (!hostValue.empty())
	{
		size_t colon = hostValue.find(':');
		if (colon != std::string::npos)
			hostValue = hostValue.substr(0, colon);

		for (auto &server : _servers)
		{
			if (server->getServerName() == hostValue)
			{
				client->_ownerServer = server;
				return ;
			}
		}
	}
}

void	IpPort::listDirectory(ClientPtr &client, std::string &listingBuffer)
{
	std::string	dirPath = client->_resolvedPath;
	DIR *dir = opendir(dirPath.c_str());
	if (!dir)
		THROW_HTTP(500, "Failed to open directory");

	std::vector<std::string>	entries;
	struct dirent *ent;
	while ((ent = readdir(dir)) != NULL)
	{
		std::string name = ent->d_name;
		if (name == "." || name == "..")
			continue;
		entries.push_back(name);
	}
	closedir(dir);
	std::sort(entries.begin(), entries.end());
	std::ostringstream	oss;
	oss << "<html><head><meta charset=\"utf-8\"><title>Index of " << client->_httpPath << "</title></head>";
	oss << "<body><h1>Index of " << client->_httpPath << "</h1><ul>";
	for (auto &name : entries)
	{
		std::string href = client->_httpPath;
		if (href.empty() || href.back() != '/')
			href += '/';
		href += name;
		oss << "<li><a href=\"" << href << "\">" << name << "</a>";
		oss << " <a href=\"" << href << "?_method=DELETE\" title=\"Delete\" onclick=\"return confirm('Delete file?');\"";
		oss << "style=\"color:red;margin-left:8px;text-decoration:none\">&#10005;</a>";
		oss << "</li>";
	}
	oss << "</ul><hr><address>webserv/1.0</address></body></html>";
	listingBuffer = oss.str();
}

// void	IpPort::processCgi(ClientPtr &client)
// {
// 	try {
// 		Cgi cgi(*client);
// 		if (!cgi.start()) {
// 			generateResponse(client, "", 500);
// 			return;
// 		}
// 		client->_state = ClientState::CGI_READING_OUTPUT;
// 	}
// 	catch (const std::exception &e) {
// 		std::cerr << "CGI start exception: " << e.what() << std::endl;
// 		generateResponse(client, "", 500);
// 	}
// }

void	IpPort::acceptConnection(epoll_event &ev, int epollFd, int eventFd)
{
	int					err;
	epoll_event			newEv;
	int					clientFd;
	sockaddr_storage	clientAddr;
	socklen_t			clientAddrLen = sizeof(clientAddr);

	try
	{
		clientFd = accept(_sockFd, (sockaddr *)&clientAddr, &clientAddrLen);
		if (clientFd == -1)
			THROW_ERRNO("accept");
		utils::makeFdNonBlocking(clientFd);
		ClientPtr	newClient = std::make_shared<Client>(clientAddr, clientAddrLen, clientFd, *this);
		_clientsMap.emplace(clientFd, newClient);
		_handlersMap.emplace(clientFd, this);
		newEv.events = EPOLLIN | EPOLLOUT;
		newEv.data.fd = clientFd;
		err = epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &newEv);
		if (err)
		{
			_handlersMap.erase(clientFd);
			_clientsMap.erase(clientFd);
			close(clientFd);
			THROW_ERRNO("epoll_ctl");
		}
	}
	catch (const std::exception& e)
	{
		if (clientFd != -1)
		{
			_handlersMap.erase(clientFd);
			_clientsMap.erase(clientFd);
			close(clientFd);
		}
		std::cerr << "Failed to accept new connection:" << e.what() << std::endl;
	}
}

void	IpPort::closeConnection(int clientFd)
{
	std::cout << "Closing connection..." << std::endl;

	_handlersMap.erase(clientFd);
	_clientsMap.erase(clientFd);

	if (clientFd != -1)
	{
		int err = epoll_ctl(_epollFd, EPOLL_CTL_DEL, clientFd, 0);
		if (err == -1)
			THROW_ERRNO("epoll_ctl(EPOLL_CTL_DEL)");
	}
	exit(1);

	std::cout << "Closing connection is done" << std::endl;
}

// Getters + Setters

int	IpPort::getSockFd()
{
	return _sockFd;
}

void	IpPort::setAddrPort(std::string addrPort)
{
	_addrPort = addrPort;
}

// Constructors + Destructor

IpPort::~IpPort()
{
	if (_sockFd != -1)
		close(_sockFd);
}

IpPort::IpPort(Program &program)
	: _clientsMap{program._clientsMap}
	, _handlersMap{program._handlersMap}
	, _sockFd{-1}
	, _epollFd{program._epollFd}
{
}
