#include "webserv.hpp"

int	parseRequest(t_request &req, std::string& use_buf)
{
	int		leftBytes = 0;

	if (req.method.empty())
	{
		req.method = use_buf.substr(0, 3);
		if (req.method != "GET"
			&& req.method != "POST"
			&& req.method != "DELETE")
		{
			THROW("unknown method in request");
		}
		use_buf.erase(0, 3);
	}

	return leftBytes;
}

// void	readRequest(t_request &req, int client_sockfd)
// {
// 	char		buffer[1024];
// 	std::string	use_buf(0);
// 	int		res;
// 	int		leftBytes;

// 	while(true)
// 	{
// 		res = read(client_sockfd, buffer, sizeof(buffer));
// 		if (res > 0)
// 		{
// 			buffer[res] = 0;
// 			use_buf += buffer;
// 		}
// 		else if (res == 0)
// 		{} // connection was closed
// 		else
// 		{} // error
// 	}
// 	leftBytes = parseRequest(req, use_buf);
// }

void	init_epoll(GlobalData &data)
{
	int			err;
	epoll_event	&ev = data.ev;

	data.epoll_fd = epoll_create(DEFAULT_EPOLL_SIZE);
	if (data.epoll_fd == -1)
		THROW_ERRNO("epoll_create");

	ev.events = EPOLLIN;
	for (ServerPtr srv : data.servers)
	{
		ev.data.ptr = static_cast<void*>(srv.get());
		err = epoll_ctl(data.epoll_fd, EPOLL_CTL_ADD, srv->getSockfd(), &ev);
		if (err)
			THROW_ERRNO("epoll_ctl");
	}
}

void	accepting_loop(GlobalData &data)
{
	int			nfds;

	init_epoll(data);
	while (true)
	{
		nfds = epoll_wait(data.epoll_fd, data.events, MAX_EVENTS, -1);
		if (nfds == -1)
			THROW_ERRNO("epoll_wait");

		for (int i = 0; i < nfds; ++i)
		{
			std::cout << "Erpoll fd: " << data.epoll_fd << std::endl;
			std::cout << "Server sockfd: " << data.servers.front()->getSockfd() << std::endl;

			IEpollFdOwner *owner = static_cast<IEpollFdOwner*>(data.events[i].data.ptr);
			owner->handleEpollEvent(data.ev, data.epoll_fd);
		}
	}
	close(data.epoll_fd);

	return ;
}
