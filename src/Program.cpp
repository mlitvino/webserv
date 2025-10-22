#include "Program.hpp"

Time	g_current_time = std::chrono::steady_clock::now();

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
		g_current_time = std::chrono::steady_clock::now();
		int timeoutMs = 0;
		if (g_current_time < _nextTimeoutCheck)
		{
			auto tempTime = std::chrono::duration_cast<std::chrono::milliseconds>(_nextTimeoutCheck - g_current_time).count();
			timeoutMs = static_cast<int>(tempTime);
		}

		int	nbr_events = epoll_wait(_epollFd, _events, MAX_EVENTS, timeoutMs);
		if (nbr_events == -1)
			THROW_ERRNO("epoll_wait");

		for (int i = 0; i < nbr_events; ++i)
		{
			int		eventFd = _events[i].data.fd;
			auto	fdHandlerPair = _handlersMap.find(eventFd);
			if (fdHandlerPair == _handlersMap.end())
				THROW("Unknown fd in map");
			(*fdHandlerPair).second->handleEpollEvent(_events[i], _epollFd, eventFd);
		}

		g_current_time = std::chrono::steady_clock::now();
		if (g_current_time >= _nextTimeoutCheck)
		{
			checkTimeOut();
			_nextTimeoutCheck = g_current_time + std::chrono::minutes(1);
		}
	}
}

void	Program::checkTimeOut()
{
	try
	{
		std::vector<int> toClose;
		toClose.reserve(_clientsMap.size());
		for (auto &fdClient : _clientsMap)
		{
			int clientFd = fdClient.first;
			const ClientPtr &client = fdClient.second;
			auto time = std::chrono::duration_cast<std::chrono::minutes>(g_current_time - client->_lastActivity).count();
			if (time >= TIMEOUT_MINUTES)
				toClose.push_back(clientFd);
		}
		for (auto &clientFd : toClose)
		{
			if (_clientsMap.find(clientFd) != _clientsMap.end())
				_addrPortVec.front()->closeConnection(clientFd);
		}
	}
	catch (std::exception &e)
	{
		std::cerr << "checkTimeOut error: " << e.what() << std::endl;
	}
}

// Getters + Setters

int	&Program::getEpollFd()
{
	return _epollFd;
}

FdClientMap	&Program::getClientsMap()
{
	return _clientsMap;
}

FdEpollOwnerMap	&Program::getHandlersMap()
{
	return _handlersMap;
}

IpPortDeq &Program::getAddrPortVec()
{
	return _addrPortVec;
}

ServerDeq &Program::getServers()
{
	return _servers;
}

// Constructors + Destructor

Program::Program()
	: _epollFd{-1}
	, _servInfo{nullptr}
	, _nextTimeoutCheck{g_current_time + std::chrono::minutes(1)}
{}

Program::~Program()
{
	free(_servInfo);
	close(_epollFd);
}

