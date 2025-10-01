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

	int i = 0;
	for (addrinfo *p = _servInfo; p; p = p->ai_next, i++)
	{}
	std::cout << "addrinfo nodes: " << i << std::endl;

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

void	IpPort::handleEpollEvent(IEpollInfo *epollInfo, int epollFd, epoll_event event)
{
	// if (epollInfo->state == SENDING_ERROR)
	// {

	// }
	if (epollInfo->_fd == getSockFd())
	{
		std::cout << "Accepting new connection..." << std::endl;
		acceptConnection(epollInfo, epollFd, event);
		std::cout << "Connection was accepted" << std::endl;
	}
	else
	{
		std::cout << "Parsing request of existed client..." << std::endl;
		parseRequest(epollInfo, epollFd, event);
		std::cout << "Parsing is done" << std::endl;
	}
}

void	IpPort::parseRequest(IEpollInfo *epollInfo, int epoll_fd, epoll_event event)
{
	char		read_buf[IO_BUFFER_SIZE];
	int			leftBytes;
	ClientPtr	client = epollInfo->_client;

	std::cout << "Accepting new data from " << std::endl;
	leftBytes = read(client->getFd(), read_buf, sizeof(read_buf) - 1);
	if (leftBytes > 0)
	{
		read_buf[leftBytes] = 0;
		client->_buffer += read_buf;

		std::cout << "ClientHandler REQUEST:\n" << client->_buffer << std::endl;

		if (client->_buffer.find("close\r\n") != std::string::npos)
		{
			//return CloseConnection(epoll_fd);
		}
		else if (client->_buffer.find(DOUBLE_CRLF) != std::string::npos)
		{
			// parseRequest

			//_state = WRITING_RESPONSE;
		}
		else if (client->_buffer.size() > CLIENT_HEADER_LIMIT)
		{
			// too large header
			// sendError();
			THROW("too large client's header");
		}

		client->_buffer.clear();
	}
	else if (leftBytes == 0)
	{
		//client->CloseConnection(epoll_fd);
	}
	else if (leftBytes < 0)
	{
		//client->CloseConnection(epoll_fd);
		THROW_ERRNO("read");
	}
}

void	IpPort::acceptConnection(IEpollInfo *epollInfo, int epollFd, epoll_event event)
{
	int					clientFd;
	sockaddr_storage	clientAddr;
	socklen_t			clientAddrLen;

	int	err;
	epoll_event ev;

	try
	{
		clientFd = accept(_sockFd, (sockaddr *)&clientAddr, &clientAddrLen);
		if (clientFd == -1)
			THROW_ERRNO("accept");
		int flags = fcntl(clientFd, F_GETFL, 0);
		fcntl(clientFd, F_SETFL, flags | O_NONBLOCK);

		ClientPtr	newClient = std::make_shared<Client>(clientAddr, clientAddrLen, clientFd);
		IEpollInfo	*newEpollInfo = new IEpollInfo;

		newEpollInfo->_owner = this;
		newEpollInfo->_fd = clientFd;
		newEpollInfo->_client = newClient;
		_unsortedClients.push_back(newClient);

		ev.events = EPOLLIN | EPOLLOUT;
		ev.data.ptr = static_cast<void*>(newEpollInfo);
		err = epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &ev);
		if (err)
			THROW_ERRNO("epoll_ctl");
	}
	catch(const std::exception& e)
	{
		close(clientFd);
		std::cerr << "Exception:" << e.what() << std::endl;
	}
}

// Getters + Setters

int	IpPort::getSockFd()
{
	return _sockFd;
}

void	IpPort::setEpollInfo(IEpollInfo *epollInfo)
{
	_epollInfo = epollInfo;
}

void	IpPort::setAddrPort(std::string addrPort)
{
	_addrPort = addrPort;
}

// Constructors + Destructor

IpPort::~IpPort()
{
	delete _epollInfo;
	if (_sockFd != -1)
		close(_sockFd);
}

IpPort::IpPort()
	: _sockFd{-1}
	, _epollInfo{nullptr}
{}
