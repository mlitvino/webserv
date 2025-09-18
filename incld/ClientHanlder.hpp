#pragma once

#include <cstring>

#include "webserv.hpp"
#include "IEpollFdOwner.hpp"

class ClientHandler : public IEpollFdOwner
{
	private:
		sockaddr_storage	_clientAddr;
		socklen_t			_clientAddrLen;

		int					_sockFd;
	public:
		ClientHandler();
		~ClientHandler();

		void	acceptConnect(int srvSockFd, int epoll_fd);
		void	handleEpollEvent(epoll_event &ev, int epoll_fd);
};


