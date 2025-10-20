#pragma once

#include <fcntl.h>
#include <string.h>

#include "webserv.hpp"
#include "CustomException.hpp"

namespace utils
{

inline void	makeFdNonBlocking(int fd)
{
	int	flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		THROW_ERRNO("fcntl(F_GETFL)");
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		THROW_ERRNO("fcntl(F_SETFL)");
}

inline void	changeEpollHandler(FdEpollOwnerMap &map, int fd, IEpollFdOwner *newHandler)
{
	auto elem = map.find(fd);
	if (elem == map.end())
		THROW("Uknown fd in FdEpollOwnerMap");
	elem->second = newHandler;
}

}


