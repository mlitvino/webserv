#include "webserv.hpp"

void	init_servers(Data &data)
{
	addrinfo	hints;
	addrinfo	*srv_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	for (ServerPtr &srv : data.servers)
	{
		try
		{
			srv_info = nullptr;
			srv->prepareSockFd(hints, srv_info);
			freeaddrinfo(srv_info);
		}
		catch (std::exception &e)
		{
			freeaddrinfo(srv_info);
			throw;
		}
	}
}

void	init_epoll(Data &data)
{
	int			err;
	epoll_event	&ev = data.ev;

	data.epoll_fd = epoll_create(DEFAULT_EPOLL_SIZE);
	if (data.epoll_fd == -1)
		THROW_ERRNO("epoll_create");

	ev.events = EPOLLIN;
	for (ServerPtr &srv : data.servers)
	{
		ev.data.ptr = static_cast<void*>(srv.get());
		err = epoll_ctl(data.epoll_fd, EPOLL_CTL_ADD, srv->getSockfd(), &ev);
		if (err)
			THROW_ERRNO("epoll_ctl");
	}
}
