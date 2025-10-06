#pragma once

#include "webserv.hpp"

class IpPort : public IEpollFdOwner
{
	public:
		FdClientMap		&_clientsMap;
		FdEpollOwnerMap	&_handlersMap;

		ServerDeq		_servers;


		std::string		_addrPort;
		std::string		_buffer;
		ClientDeq		_unsortedClients;

		int				_sockFd;

		void			OpenSocket(addrinfo &hints, addrinfo *_servInfo);

		void			handleEpollEvent(epoll_event &ev, int epoll_fd, int eventFd);

		void			acceptConnection(epoll_event &ev, int epollFd, int eventFd);
		void			closeConnection(epoll_event &ev, int epollFd, int eventFd);

		void			parseRequest(epoll_event &ev, int epollFd, int eventFd);
		//void			parseRequest(ClientPtr	client);

		void			setAddrPort(std::string addrPort);
		int				getSockFd();

		~IpPort();
		IpPort(FdClientMap	&clientsMap, FdEpollOwnerMap &handlersMap);
};
