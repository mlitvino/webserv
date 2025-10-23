#pragma once

#include <chrono>

#include "webserv.hpp"
#include "ConfigParser.hpp"
#include "CustomException.hpp"
#include "ChildFailedException.hpp"
#include "Client.hpp"

#define DEFAULT_CONF "conf/default.conf"
#define MAX_EVENTS 10
#define DEFAULT_EPOLL_SIZE 10
#define TIMEOUT_SECONDS 360

class Program
{
	private:
		int						_epollFd;
		IpPortDeq	_addrPortVec;
		addrinfo				*_servInfo;

		ServerDeq				_servers;

		epoll_event				_ev;
		epoll_event				_events[MAX_EVENTS];

		FdClientMap				_clientsMap;
		FdEpollOwnerMap			_handlersMap;
		Time					_nextTimeoutCheck;
	public:
		Program();
		~Program();

		void	parseConfFile(char *conf_file);
		void	initSockets();
		void	waitEpollEvent();
		void	checkTimeOut();

		int				&getEpollFd();
		FdClientMap		&getClientsMap();
		FdEpollOwnerMap	&getHandlersMap();
		IpPortDeq		&getAddrPortVec();
		ServerDeq 		&getServers();
};
