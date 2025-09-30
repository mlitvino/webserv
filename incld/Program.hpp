#pragma once

#include <vector>

#include "webserv.hpp"
#include "IpPort.hpp"

class Program
{
	public:
		std::vector<IpPortPtr>	_addrPortVec;
		addrinfo				*_servInfo;

		epoll_event	ev;
		epoll_event	events[MAX_EVENTS];
		int			epoll_fd;

		void	parseConfigFile(char *config_file);
		void	initServers();

		Program();
		~Program();
};
