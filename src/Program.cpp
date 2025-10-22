#include "Program.hpp"

void	Program::parseConfFile(char *conf_file)
{
	try {
		ConfigParser configParser;

		if (conf_file) {
			configParser.parseConfig(std::string(conf_file));
		} else {
			// Try default configuration file
			configParser.parseConfig(std::string(DEFAULT_CONF));
		}

		configParser.createServersAndIpPortsFromConfig(*this);

		// If no servers were created, create a default one
		if (_servers.empty()) {
			THROW("none of servers was created");
		}
	} catch (const std::exception& e) {
		std::string err = std::string("parsing error: ") + e.what();
		THROW(err.c_str());
	}
}

void	Program::initSockets()
{
	addrinfo	hints;
	int			err;
	epoll_event	&ev = _ev;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;


	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGPIPE, &sa, NULL);


	_epollFd = epoll_create(DEFAULT_EPOLL_SIZE);
	if (_epollFd == -1)
		THROW_ERRNO("epoll_create");

	for (IpPortPtr &ipPort: _addrPortVec)
	{
		ipPort->OpenSocket(hints, _servInfo);
		ev.events = EPOLLIN;
		ev.data.fd = ipPort->getSockFd();
		_handlersMap.emplace(ipPort->getSockFd(), ipPort.get());
		err = epoll_ctl(_epollFd, EPOLL_CTL_ADD, ipPort->getSockFd(), &ev);
		if (err)
			THROW_ERRNO("epoll_ctl");
		free(_servInfo);
		_servInfo = nullptr;
	}
}

void	Program::waitEpollEvent()
{
	std::cout << "Waiting for epoll event..." << std::endl;
	while (true)
	{
		int	nbr_events = epoll_wait(_epollFd, _events, MAX_EVENTS, -1);
		if (nbr_events == -1)
			THROW_ERRNO("epoll_wait");

		// check time_out clients and close them
		// checkTimeOut

		for (int i = 0; i < nbr_events; ++i)
		{
			int		eventFd = _events[i].data.fd;
			auto	fdHandlerPair = _handlersMap.find(eventFd);
			if (fdHandlerPair == _handlersMap.end())
				THROW("Unknown fd in map");
			(*fdHandlerPair).second->handleEpollEvent(_events[i], _epollFd, eventFd);
		}
	}
}


// Constructors + Destructor

Program::Program()
	: _epollFd{-1}
	, _servInfo{nullptr}
{}

Program::~Program()
{
	free(_servInfo);
	close(_epollFd);
}

