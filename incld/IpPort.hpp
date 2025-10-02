#pragma once

#include "webserv.hpp"

class IpPort : public IEpollFdOwner2, public IEpollFdOwner
{
	public:
		FdClientMap		&_clientsMap;
		FdEpollOwnerMap	&_handlersMap;


		std::string		_addrPort;
		std::string		_buffer;
		ServerDeq		_servers;
		ClientDeq		_unsortedClients;

		int				_sockFd;
		IEpollInfo		*_epollInfo;

		void			OpenSocket(addrinfo &hints, addrinfo *_servInfo);

		void			handleEpollEvent(epoll_event &ev, int epoll_fd, int eventFd);

		void			handleEpollEvent(IEpollInfo *epollInfo, int epoll_fd, epoll_event event) {};

		void			acceptConnection(epoll_event &ev, int epollFd, int eventFd);

		void			parseRequest(epoll_event &ev, int epollFd, int eventFd);
		//void			parseRequest(ClientPtr	client);

		void			setEpollInfo(IEpollInfo *epollInfo);
		void			setAddrPort(std::string addrPort);
		int				getSockFd();

		~IpPort();
		IpPort(FdClientMap	&clientsMap, FdEpollOwnerMap &handlersMap);
};
