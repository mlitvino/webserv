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

void	accepting_loop(Data &data)
{
	int	nbr_events;

	init_epoll(data);
	std::cout << "Erpoll fd: " << data.epoll_fd << std::endl;
	while (true)
	{
		nbr_events = epoll_wait(data.epoll_fd, data.events, MAX_EVENTS, -1);
		if (nbr_events == -1)
			THROW_ERRNO("epoll_wait");

		for (int i = 0; i < nbr_events; ++i)
		{
			if (data.events[i].events == EPOLLIN)
				std::cout << "READING EPOLL EVENT" << std::endl;
			else if (data.events[i].events == EPOLLOUT)
				std::cout << "WRITING EPOLL EVENT" << std::endl;


			IEpollFdOwner *owner = static_cast<IEpollFdOwner*>(data.events[i].data.ptr);
			owner->handleEpollEvent(data.ev, data.epoll_fd);
		}
	}
	close(data.epoll_fd);

	return ;
}
