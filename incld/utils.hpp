#pragma once

#include <fcntl.h>

namespace utils
{

inline void	makeFdNonBlocking(int fd)
{
	int	flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

}


