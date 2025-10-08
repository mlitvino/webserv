#pragma once

#include "webserv.hpp"

class Program
{
	public:
		int						_epollFd;
		std::vector<IpPortPtr>	_addrPortVec;
		addrinfo				*_servInfo;

		ServerDeq				_servers;

		epoll_event				_ev;
		epoll_event				_events[MAX_EVENTS];

		FdClientMap				_clientsMap;
		FdEpollOwnerMap			_handlersMap;

		void	parseConfFile(char *conf_file);
		void	initSockets();
		void	waitEpollEvent();

		Program();
		~Program();
};
