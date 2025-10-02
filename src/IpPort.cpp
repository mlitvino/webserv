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

void	IpPort::handleEpollEvent(epoll_event &ev, int epollFd, int eventFd)
{
	if (ev.events & (EPOLLIN | EPOLLHUP))
		std::cout << "READING EPOLL EVENT" << std::endl;
	// if (ev.events == EPOLLOUT)
	// 	std::cout << "WRITING EPOLL EVENT" << std::endl;

	// if (epollInfo->state == SENDING_ERROR)
	// {

	// }
	if (eventFd == getSockFd())
	{
		std::cout << "Accepting new connection..." << std::endl;
		acceptConnection(ev, epollFd, eventFd);
		std::cout << "Connection was accepted" << std::endl;
	}
	else
	{

		ClientPtr	client = (*_clientsMap.find(eventFd)).second;

		if (ev.events & (EPOLLIN | EPOLLHUP))
		{
			std::cout << "EXISTING CLIENT" << std::endl;


			if (client->_state == clientState::READING_CLIENT_HEADER)
			{
				std::cout << "Parsing request of existed client..." << std::endl;
				parseRequest(ev, epollFd, eventFd);
				std::cout << "Parsing is done" << std::endl;
			}
		}
		else
		{
			if (client->_state == clientState::SENDING_RESPONSE)
			{
				std::cout << "Sending response..." << std::endl;
				//parseRequest(ev, epollFd, eventFd);
				std::cout << "Sending response is done" << std::endl;
			}
		}
	}
}

void	IpPort::parseRequest(epoll_event &ev, int epollFd, int eventFd)
{
	char		read_buf[IO_BUFFER_SIZE];
	int			leftBytes;
	ClientPtr	client = (*_clientsMap.find(eventFd)).second;

	std::cout << "Accepting new data from " << std::endl;
	leftBytes = read(eventFd, read_buf, sizeof(read_buf) - 1);
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

			client->openFileAddEpoll(ev, epollFd, eventFd);
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

void	IpPort::acceptConnection(epoll_event &ev, int epollFd, int eventFd)
{
	int					clientFd;
	sockaddr_storage	clientAddr;
	socklen_t			clientAddrLen;

	int	err;
	epoll_event newEv;

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
	catch(const std::exception& e)
	{
		if (clientFd != -1)
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

IpPort::IpPort(FdClientMap	&clientsMap, FdEpollOwnerMap &handlersMap)
	: _clientsMap{clientsMap}
	, _handlersMap{handlersMap}
	, _sockFd{-1}
	, _epollInfo{nullptr}
{};
