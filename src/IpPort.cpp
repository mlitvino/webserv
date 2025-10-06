#include "IpPort.hpp"

void	IpPort::OpenSocket(addrinfo &hints, addrinfo *_servInfo)
{
	int	err;

	size_t		delim = _addrPort.find_last_of(":");
	std::string	host = _addrPort.substr(0, delim);
	std::string	port = _addrPort.substr(delim + 1, _addrPort.size() - delim);

	err = getaddrinfo(host.c_str(), port.c_str(), &hints, &_servInfo);
	if (err)
		THROW(gai_strerror(err));

	_sockFd = socket(_servInfo->ai_family, _servInfo->ai_socktype, _servInfo->ai_protocol);
	if (_sockFd == -1)
		THROW_ERRNO("socket");

	int opt = 1;

	err = setsockopt(_sockFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (err)
		THROW_ERRNO("setsockopt(SO_REUSEADDR)");

	err = bind(_sockFd, _servInfo->ai_addr, _servInfo->ai_addrlen);
	if (err)
		THROW_ERRNO("bind");

	err = listen(_sockFd, QUEUE_SIZE);
	if (err)
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
		std::cout << "Existing client" << std::endl;
		ClientPtr	client = (*_clientsMap.find(eventFd)).second;
		if (ev.events & EPOLLIN)
		{
			if (client->_state == ClientState::READING_REQUEST)
			{
				std::cout << "Accepting data from existing client..." << std::endl;
				if (!readRequest(client, eventFd))
					return closeConnection(ev, epollFd, eventFd);
				if (client->_buffer.find("\r\n\r\n") != std::string::npos)
				{
					std::cout << "Full request recieved" << std::endl;
					parseRequest(ev, epollFd, eventFd);
				}
				else if (client->_buffer.size() > CLIENT_HEADER_LIMIT)
				{
					std::cout << "Client's header is too large" << std::endl;
					closeConnection(ev, epollFd, eventFd);
				}
				std::cout << "Accepting data is done" << std::endl;
			}
		}
	}
}
std::string		IpPort::parseRequestLine(ClientPtr &client, std::string& line)
{
	size_t firstSpace = line.find(' ');
	size_t secondSpace = line.find(' ', firstSpace + 1);

	if (firstSpace == std::string::npos || secondSpace == std::string::npos)
		return "";

	client->_httpMethod = line.substr(0, firstSpace);
	client->_httpPath = line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
	client->_httpVersion = line.substr(secondSpace + 1);

	// Remove \r\n from version
	size_t crPos = client->_httpVersion.find('\r');
	if (crPos != std::string::npos)
		client->_httpVersion = client->_httpVersion.substr(0, crPos);

	return client->_httpPath;
}

void	IpPort::parseRequest(epoll_event &ev, int epollFd, int eventFd)
{
	ClientPtr	client = (*_clientsMap.find(eventFd)).second;

	std::cout << "DEBUG: Buffer content: " << client->_buffer << std::endl;

	size_t		firstLine = client->_buffer.find("\r\n");
	std::string	requestLine = client->_buffer.substr(0, firstLine);
	std::cout << "DEBUG: Request line: " << requestLine << std::endl;

	std::string	requestedPath = parseRequestLine(client, requestLine);

	std::cout << "Method: " << client->_httpMethod << ", Path: " << client->_httpPath << ", Version: " << client->_httpVersion << std::endl;

	assignServerToClient(client);

	if (!isMethodAllowed(client, requestedPath))
	{
		std::cout << "Invalid Method" << std::endl;
		closeConnection(ev, epollFd, eventFd);
		//_responseBuffer = generateHttpResponse("", 405);
		return;
	}

	if (client->_httpMethod == "POST")
	{
		if (!isBodySizeValid(client))
		{
			std::cout << "Invalid Content-Length" << std::endl;
			closeConnection(ev, epollFd, eventFd);
			//_responseBuffer = generateHttpResponse("", 413);
			return;
		}
	}

	if (client->_httpMethod == "GET")
	{
		//handleGetRequest(requestedPath);
	}
	else if (client->_httpMethod == "POST")
	{
		//handlePostRequest(requestedPath);
	}
	else if (client->_httpMethod == "DELETE")
	{
		//handleDeleteRequest(requestedPath);
	}
}

bool	IpPort::isBodySizeValid(ClientPtr &client)
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
		size_t maxBodySize = client->_ownerServer->getClientBodySize();

		std::cout << "DEBUG: Content-Length: " << contentLength << ", Max allowed: " << maxBodySize << std::endl;
		return contentLength <= maxBodySize;
	} catch (const std::exception& e)
	{
		std::cout << "DEBUG: Error parsing Content-Length: " << e.what() << std::endl;
		return false;
	}
}

bool	IpPort::isMethodAllowed(ClientPtr &client, std::string& path)
{
	const std::vector<Location>& locations = client->_ownerServer->getLocations();
	std::cout << "DEBUG: Found " << locations.size() << " locations" << std::endl;

	// Find matching location
	const Location* matchedLocation = nullptr;
	size_t longestMatch = 0;

	for (const auto& location : locations) {
		std::cout << "DEBUG: Checking location: " << location.path << " against path: " << path << std::endl;
		if (path.find(location.path) == 0 && location.path.length() > longestMatch) {
			matchedLocation = &location;
			longestMatch = location.path.length();
			std::cout << "DEBUG: Matched location: " << location.path << std::endl;
		}
	}

	if (!matchedLocation) {
		std::cout << "DEBUG: No specific location found, allowing GET by default" << std::endl;
		return client->_httpMethod == "GET";
	}

	std::cout << "DEBUG: Using location: " << matchedLocation->path << " with allowedMethods: " << matchedLocation->allowedMethods << std::endl;

	// Check if method is allowed in this location
	int methodFlag = 0;
	if (client->_httpMethod == "GET") methodFlag = static_cast<int>(HttpMethod::GET);
	else if (client->_httpMethod == "POST") methodFlag = static_cast<int>(HttpMethod::POST);
	else if (client->_httpMethod == "DELETE") methodFlag = static_cast<int>(HttpMethod::DELETE);
	else {
		std::cout << "DEBUG: Unknown method: " << client->_httpMethod << std::endl;
		return false; // Unknown method
	}

	bool allowed = (matchedLocation->allowedMethods & methodFlag) != 0;
	std::cout << "DEBUG: Method " << client->_httpMethod << " (flag: " << methodFlag << ") allowed: " << (allowed ? "YES" : "NO") << std::endl;

	return allowed;
}

void	IpPort::assignServerToClient(ClientPtr &client)
{
	client->_ownerServer = _servers.front();

	size_t		headersEnd = client->_buffer.find("\r\n\r\n");
	std::string	headersSlice = client->_buffer.substr(0, headersEnd);
	size_t		hostPos = headersSlice.find("Host:");

	std::string hostValue;
	if (hostPos != std::string::npos)
	{
		size_t	lineEnd = headersSlice.find('\r', hostPos);
		size_t	valueStart = headersSlice.find(':', hostPos);
		if (valueStart != std::string::npos)
		{
			valueStart += 1;
			while (valueStart < headersSlice.size() && isspace(headersSlice[valueStart]))
				++valueStart;
			size_t	valueLen = lineEnd - valueStart;
			hostValue = headersSlice.substr(valueStart, valueLen);

			size_t colon = hostValue.find(':');
			if (colon != std::string::npos)
				hostValue = hostValue.substr(0, colon);
		}
	}

	if (!hostValue.empty())
	{
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
	char buffer[IO_BUFFER_SIZE];
	int bytesRead = read(clientFd, buffer, sizeof(buffer) - 1);

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
			if (errno != EAGAIN && errno != EWOULDBLOCK)
			{
				std::cout << "DEBUG: Read error: " << strerror(errno) << std::endl;
				THROW_ERRNO("read");
			}
			else
			{
				std::cout << "DEBUG: Read would block (EAGAIN/EWOULDBLOCK)" << std::endl;
			}
			return true;
		}
	}
	catch (std::exception& e)
	{
		std::cout << "Exception: " << e.what() << std::endl;
		return false;
	}

	return true;
}

void	IpPort::acceptConnection(epoll_event &ev, int epollFd, int eventFd)
{
	int					err;
	epoll_event			newEv;
	int					clientFd;
	sockaddr_storage	clientAddr;
	socklen_t			clientAddrLen;

	try
	{
		clientFd = accept(_sockFd, (sockaddr *)&clientAddr, &clientAddrLen);
		if (clientFd == -1)
			THROW_ERRNO("accept");
		int flags = fcntl(clientFd, F_GETFL, 0);
		fcntl(clientFd, F_SETFL, flags | O_NONBLOCK);

		ClientPtr	newClient = std::make_shared<Client>(clientAddr, clientAddrLen, clientFd, *this);
		_clientsMap.emplace(clientFd, newClient);
		_handlersMap.emplace(clientFd, this);
		newEv.events = EPOLLIN | EPOLLOUT;
		newEv.data.fd = clientFd;
		err = epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &newEv);
		if (err)
			THROW_ERRNO("epoll_ctl");
	}
	catch (const std::exception& e)
	{
		if (clientFd != -1)
			close(clientFd);
		std::cerr << "Exception:" << e.what() << std::endl;
	}
}

void	IpPort::closeConnection(int epollFd, int clientFd)
{
	std::cout << "Closing client..." << std::endl;

	_handlersMap.erase(clientFd);
	_clientsMap.erase(clientFd);

	int err = epoll_ctl(clientFd, EPOLL_CTL_DEL, clientFd, 0);
	if (err)
	{
		THROW("epoll_ctl(EPOLL_CTL_DEL)");
	}
}

void	IpPort::closeConnection(epoll_event &ev, int epollFd, int eventFd)
{
	std::cout << "Closing client..." << std::endl;

	_handlersMap.erase(eventFd);
	_clientsMap.erase(eventFd);

	int err = epoll_ctl(epollFd, EPOLL_CTL_DEL, eventFd, 0);
	if (err)
	{
		THROW("epoll_ctl(EPOLL_CTL_DEL)");
	}
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

IpPort::IpPort(FdClientMap	&clientsMap, FdEpollOwnerMap &handlersMap)
	: _clientsMap{clientsMap}
	, _handlersMap{handlersMap}
	, _sockFd{-1}
{};
