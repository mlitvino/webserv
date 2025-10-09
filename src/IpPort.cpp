#include "IpPort.hpp"
#include "Cgi.hpp"

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
			if (client->_state == ClientState::READING_REQUEST)
			{
				std::cout << "Accepting data from existing client..." << std::endl;
				if (!readRequest(client, eventFd))
					return closeConnection(eventFd);
				parseRequest(ev, epollFd, eventFd);
				std::cout << "Accepting data is done" << std::endl;
			}
		}
		else if (ev.events & EPOLLOUT)
		{
			if (client->_state == ClientState::SENDING_RESPONSE)
			{
				std::cout << "Sending response..." << std::endl;
				sendResponse(client, eventFd);
				std::cout << "Sending response is done" << std::endl;
			}
		}
	}
}

void	IpPort::sendResponse(ClientPtr &client, int clientFd)
{
	int		bytesSent = 0;

	if (client->_responseOffset < client->_responseBuffer.size())
	{
		const char* buf = client->_responseBuffer.c_str() + client->_responseOffset;
		size_t toSend = client->_responseBuffer.size() - client->_responseOffset;
		bytesSent = send(clientFd, buf, toSend, 0);
		if (bytesSent > 0)
			client->_responseOffset += static_cast<size_t>(bytesSent);
	}
	else if (client->_fileOffset < client->_fileSize && client->_fileFd >= 0)
	{
		off_t offset = static_cast<off_t>(client->_fileOffset);
		bytesSent = sendfile(clientFd, client->_fileFd, &offset, client->_fileSize - offset);
		if (bytesSent > 0)
			client->_fileOffset += static_cast<size_t>(bytesSent);
	}

	if (client->_responseOffset >= client->_responseBuffer.size()
		&& client->_fileOffset >= client->_fileSize)
	{
		client->_responseBuffer.clear();
		size_t	endRequest = client->_buffer.find("\r\n\r\n") + 4;
		client->_buffer.erase(0, endRequest);
		if (client->_fileFd != -1) {
			close(client->_fileFd);
			client->_fileFd = -1;
		}
		client->_state = ClientState::READING_REQUEST;
		return;
	}

	if (bytesSent == 0)
	{
		std::cout << "Connection was closed in resndResponse" << std::endl;
		closeConnection(clientFd);
	}
	else if (bytesSent == -1)
	{
		THROW_ERRNO("send");
	}
}

void	IpPort::parseRequest(epoll_event &ev, int epollFd, int eventFd)
{
	ClientPtr	client = (*_clientsMap.find(eventFd)).second;

	if (client->_buffer.find("\r\n\r\n") == std::string::npos)
		return ;

	std::cout << "DEBUG: Buffer content: " << client->_buffer << std::endl;
	parseHeaders(client);
	std::cout << "Method: " << client->_httpMethod << ", Path: " << client->_httpPath << ", Version: " << client->_httpVersion << std::endl;

	assignServerToClient(client);

	if (!client->_ownerServer->isMethodAllowed(client, client->_httpPath))
	{
		std::cout << "Invalid Method" << std::endl;
		generateResponse(client, "", 405);
		return;
	}

	// isPathAllowed

	if (client->_httpMethod == "GET")
	{
		handleGetRequest(client, client->_httpPath);
	}
	else if (client->_httpMethod == "POST")
	{
		if (!client->_ownerServer->isBodySizeValid(client))
		{
			std::cout << "Invalid Content-Length" << std::endl;
			generateResponse(client, "", 413);
			return;
		}
		//handlePostRequest(client->_httpPath);
	}
	else if (client->_httpMethod == "DELETE")
	{
		//handleDeleteRequest(client->_httpPath);
	}
}

void	IpPort::parseHeaders(ClientPtr &client)
{
	size_t headersEnd = client->_buffer.find("\r\n\r\n");
	if (headersEnd == std::string::npos)
		return;

	std::string			headers = client->_buffer.substr(0, headersEnd);
	std::istringstream	iss(headers);
	std::string			line;

	client->_contentLen = 0;
	client->_chunked = false;
	client->_keepAlive = false;

	std::getline(iss, line);
	size_t firstSpace = line.find(' ');
	size_t secondSpace = line.find(' ', firstSpace + 1);
	client->_httpMethod = line.substr(0, firstSpace);
	client->_httpPath = line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
	client->_httpVersion = line.substr(secondSpace + 1);

	while (std::getline(iss, line))
	{
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		size_t colon = line.find(':');
		if (colon == std::string::npos)
			continue;
		std::string name = line.substr(0, colon);
		std::string value = line.substr(colon + 1);

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
}

void IpPort::handleGetRequest(ClientPtr &client, const std::string& path)
{
	std::cout << "DEBUG: handleGetRequest() called with path: " << path << std::endl;

	std::string fullPath = client->_ownerServer->findFile(client, path);
	std::cout << "DEBUG: findFile returned: " << fullPath << std::endl;

	if (fullPath.empty())
	{
		std::cout << "DEBUG: File not found, generating 404 response" << std::endl;
		generateResponse(client, "", 404);
		return;
	}

	std::ifstream file(fullPath);
	if (file.good())
	{
		std::cout << "DEBUG: File exists, generating 200 response" << std::endl;
		file.close();
		generateResponse(client, fullPath, 200);
	}
	else
	{
		std::cout << "DEBUG: File not found after stat, generating 404 response" << std::endl;
		generateResponse(client, "", 404);
	}

	std::cout << "DEBUG: Response buffer size after generation: " << client->_responseBuffer.size() << std::endl;
}

void	IpPort::generateResponse(ClientPtr &client, std::string filePath, int statusCode)
{
	std::string	statusText;
	std::string	response;

	switch (statusCode)
	{
		case 200: statusText = "OK"; break;
		case 404: statusText = "Not Found"; break;
		case 405: statusText = "Method Not Allowed"; break;
		case 413: statusText = "Payload Too Large"; break;
		case 500: statusText = "Internal Server Error"; break;
		default: statusText = "Unknown"; break;
	}

	if (statusCode != 200)
		filePath = client->_ownerServer->getCustomErrorPage(statusCode);

	if (!filePath.empty())
	{
		std::cout << "DEBUG: filepath name -> " << filePath << std::endl;
		client->openFile(filePath);
		if (client->_fileFd < 0)
		{
			THROW_ERRNO("open");
			std::cout << "DEBUG: Coudln't open filePath" << std::endl;
			statusCode = 500;
			statusText = "Internal Server Error";
		}
	}
	else
	{
		std::cout << "DEBUG: filePath is empty" << std::endl;
	}

	// add MIME TYPE
	response = "HTTP/1.1 " + std::to_string(statusCode) + " " + statusText + "\r\n";
	response += "Content-Type: text/html\r\n";
	response += "Content-Length: " + std::to_string(client->_fileSize) + "\r\n";
	response += "Server: webserv/1.0\r\n";
	response += "Connection: close\r\n";
	response += "Cache-Control: no-cache\r\n";
	response += "\r\n";

	client->_state = ClientState::SENDING_RESPONSE;
	utils::changeEpollHandler(_handlersMap, client->_clientFd, client.get());
	client->_responseBuffer = std::move(response);
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

bool IpPort::readRequest(ClientPtr &client, int clientFd)
{
	char	buffer[IO_BUFFER_SIZE];
	int		bytesRead = read(clientFd, buffer, sizeof(buffer) - 1);

	try
	{
		if (bytesRead > 0)
		{
			buffer[bytesRead] = '\0';
			client->_buffer.append(buffer, bytesRead);
			return true;
		}
		else if (bytesRead == 0)
		{
			std::cout << "DEBUG: Client disconnected" << std::endl;
			return false;
		}
		else if (bytesRead == -1)
		{
			THROW_ERRNO("read");
		}
	}
	catch (std::exception& e)
	{
		std::cout << "Exception: " << e.what() << std::endl;
		return false;
	}

	return true;
}

std::string	IpPort::getMimeType(const std::string& filePath)
{
	if (filePath.empty())
		return "text/html";

	size_t dotPos = filePath.find_last_of('.');
	if (dotPos == std::string::npos)
		return "text/plain";

	std::string extension = filePath.substr(dotPos + 1);

	// Convert to lowercase
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

	if (extension == "html" || extension == "htm")
		return "text/html";
	else if (extension == "css")
		return "text/css";
	else if (extension == "js")
		return "application/javascript";
	else if (extension == "json")
		return "application/json";
	else if (extension == "txt")
		return "text/plain";
	else if (extension == "png")
		return "image/png";
	else if (extension == "jpg" || extension == "jpeg")
		return "image/jpeg";
	else if (extension == "gif")
		return "image/gif";
	else if (extension == "ico")
		return "image/x-icon";
	else
		return "application/octet-stream";
}

void	IpPort::processCgi(ClientPtr &client)
{
	try {
		Cgi cgi(*client);
		if (!cgi.start()) {
			generateResponse(client, "", 500);
			return;
		}
		client->_state = ClientState::CGI_READING_OUTPUT;
	}
	catch (const std::exception &e) {
		std::cerr << "CGI start exception: " << e.what() << std::endl;
		generateResponse(client, "", 500);
	}
}

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
		std::cerr << "Exception:" << e.what() << std::endl;
	}
}

void	IpPort::closeConnection(int clientFd)
{
	std::cout << "Closing connection..." << std::endl;

	_handlersMap.erase(clientFd);
	_clientsMap.erase(clientFd);

	int err = epoll_ctl(_epollFd, EPOLL_CTL_DEL, clientFd, 0);
	if (err)
	{
		THROW("epoll_ctl(EPOLL_CTL_DEL)");
	}

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
{};
