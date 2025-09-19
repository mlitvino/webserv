#pragma once

#include <cstring>

#include "webserv.hpp"
#include "IEpollFdOwner.hpp"

class ClientHandler : public IEpollFdOwner
{
	private:
		sockaddr_storage	_clientAddr;
		socklen_t			_clientAddrLen;
		Server				&_owner;
		size_t				_index;

		int					_sockFd;
	public:
		ClientHandler() = delete;
		~ClientHandler();
		ClientHandler(Server& owner);

		void		setIndex(size_t index);
		void		CloseConnection(int epoll_fd);

		void	acceptConnect(int srvSockFd, int epoll_fd);
		void	handleEpollEvent(epoll_event &ev, int epoll_fd);
};


