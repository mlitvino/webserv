#include "Program.hpp"

void	Program::parseConfigFile(char *config_file)
{
	IpPortPtr	addr_port = std::make_unique<IpPort>();
	ServerPtr	new_server = std::make_shared<Server>();

	new_server->setHost(HOST);
	new_server->setPort(PORT);

	addr_port->_servers.push_back(new_server);
	addr_port->setAddrPort(new_server->getHost() + ":" + new_server->getPort());
	_addrPortVec.push_back(std::move(addr_port));
}

void	Program::initServers()
{
	addrinfo	hints;
	int			err;
	epoll_event	&ev = _ev;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	_epollFd = epoll_create(DEFAULT_EPOLL_SIZE);
	if (_epollFd == -1)
		THROW_ERRNO("epoll_create");

	for (IpPortPtr &ipPort: _addrPortVec)
	{
		ipPort->OpenSocket(hints, _servInfo);

		IEpollInfo *epollInfo = new IEpollInfo;
		epollInfo->_owner = ipPort.get();
		epollInfo->_fd = ipPort->getSockFd();
		ev.data.ptr = static_cast<void*>(epollInfo);
		ev.events = EPOLLIN;
		ipPort->setEpollInfo(epollInfo);

		err = epoll_ctl(_epollFd, EPOLL_CTL_ADD, ipPort->getSockFd(), &ev);
		if (err)
			THROW_ERRNO("epoll_ctl");
		free(_servInfo);
		_servInfo = nullptr;
	}
}

void	Program::waitEpollEvent()
{
	int	nbr_events;

	std::cout << "Erpoll fd: " << _epollFd << std::endl;
	while (true)
	{
		nbr_events = epoll_wait(_epollFd, _events, MAX_EVENTS, -1);
		if (nbr_events == -1)
			THROW_ERRNO("epoll_wait");

		for (int i = 0; i < nbr_events; ++i)
		{
			IEpollInfo	*epollInfo = static_cast<IEpollInfo*>(_events[i].data.ptr);
			epollInfo->_owner->handleEpollEvent(epollInfo, _events[i].events, _events[i]);
		}
	}
}


// Constructors + Destructor

Program::Program()
{
	_servInfo = nullptr;
}

Program::~Program()
{
	free(_servInfo);
	close(_epollFd);
}

