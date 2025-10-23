#pragma once

#include <sys/epoll.h>

struct IEpollFdOwner
{
	virtual void handleEpollEvent(epoll_event &ev, int eventFd) = 0;
	virtual ~IEpollFdOwner() {};
};
