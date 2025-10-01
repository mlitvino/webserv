#pragma once

#include "webserv.hpp"

class IpPort : public IEpollFdOwner2
{
	public:
		std::string		_addrPort;
		std::string		_buffer;
		ServerDeq		_servers;
		ClientDeq		_unsortedClients;

		int				_sockFd;
		IEpollInfo		*_epollInfo;

		void			OpenSocket(addrinfo &hints, addrinfo *_servInfo);
		void			handleEpollEvent(IEpollInfo *epollInfo, int epoll_fd, epoll_event event);
		void			acceptConnection(IEpollInfo *epollInfo, int epoll_fd, epoll_event event);

		void			parseRequest(IEpollInfo *epollInfo, int epoll_fd, epoll_event event);
		//void			parseRequest(ClientPtr	client);

		void			setEpollInfo(IEpollInfo *epollInfo);
		void			setAddrPort(std::string addrPort);
		int				getSockFd();

		~IpPort();
		IpPort();
};
