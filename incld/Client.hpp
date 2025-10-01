#pragma once

#include "webserv.hpp"

class Client : public IEpollFdOwner2
{
	public:
		std::string			_buffer;
	private:
		sockaddr_storage	_clientAddr;
		socklen_t			_clientAddrLen;
		int					_clientFd;

		IEpollInfo			*_epollInfo;

		// response?
		// request?
		// buffer?
	public:
		Client(sockaddr_storage clientAddr, socklen_t	clientAddrLen, int	clientFd);
		~Client();

		void	setEpollInfo(IEpollInfo *epollInfo);
		int		getFd();
		void	handleEpollEvent(IEpollInfo *epollInfo, int epollFd, epoll_event event);
};
