#pragma once

#include "webserv.hpp"

struct IEpollFdOwner
{
	virtual void handleEpollEvent(epoll_event &ev, int epoll_fd, int eventFd) = 0;
	virtual ~IEpollFdOwner() {};
};
