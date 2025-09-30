#pragma once

#include "webserv.hpp"

struct IEpollFdOwner
{
	virtual void handleEpollEvent(epoll_event &ev, int epoll_fd) = 0;
	virtual ~IEpollFdOwner() {};
};

struct IEpollCtx
{
	IEpollFdOwner*	owner;
	int				fd;
	int				type;
	int				index;
};
