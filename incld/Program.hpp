#pragma once

#include <vector>

#include "webserv.hpp"
#include "IpPort.hpp"

struct Program
{
	public:
		int						_epollFd;
		std::vector<IpPortPtr>	_addrPortVec;
		addrinfo				*_servInfo;

		epoll_event				_ev;
		epoll_event				_events[MAX_EVENTS];

		FdClientMap				_clientMap;
		FdEpollOwnerMap			_handlersMap;

		void	parseConfigFile(char *config_file);
		void	initSockets();
		void	waitEpollEvent();

		Program();
		~Program();
};
