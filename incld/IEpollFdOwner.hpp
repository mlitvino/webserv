#pragma once

#include "webserv.hpp"

struct IEpollInfo;

struct IEpollFdOwner
{
	virtual void handleEpollEvent(epoll_event &ev, int epoll_fd) = 0;
	virtual ~IEpollFdOwner() {};
};

struct IEpollFdOwner2
{
	virtual void handleEpollEvent(IEpollInfo *epollInfo, int epollFd, epoll_event event) = 0;
	virtual ~IEpollFdOwner2() {};
};

struct IEpollInfo
{
	IEpollFdOwner2*	_owner;
	int				_fd;
	ClientPtr		_client;
	int				_type;
};
