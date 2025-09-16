#include "webserv.hpp"

int	parseRequest(t_request &req, std::string& use_buf)
{
	int		leftBytes;

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

int	handle_new_data(epoll_event *events, int i)
{
	char	buffer[1024] = {0};
	int		err;

	err = read(events[i].data.fd, buffer, sizeof(buffer));
	if (err == -1)
		THROW_ERRNO("read");

	std::cout << "CLIENT REQUEST:\n" << buffer << std::endl;

	int flag = 0;
	if (strcmp("ex\r\n", buffer) == 0)
		flag++;

	bzero(buffer, sizeof(buffer));
	strcpy(buffer, HTTP_STATUS);

	int html_fd = open(STATIC_SITE, O_RDWR);
	if (html_fd == -1)
		THROW_ERRNO("open");

	int len = read(html_fd, &buffer[sizeof(HTTP_STATUS) - 1], sizeof(buffer) / 2);
	if (len == -1)
		THROW_ERRNO("read");

	close(html_fd);

	std::cout << "SERVER RESPONSE:\n" << buffer << std::endl;

	err = send(events[i].data.fd, buffer, sizeof(buffer), 0);
	if (err == -1)
		THROW_ERRNO("send");

	close(events[i].data.fd);

	return flag;
}

void	accepting_loop(int sockfd)
{
	int					err;
	int					client_sockfd = -1;
	sockaddr_storage	client_addr;
	socklen_t			client_addr_len = sizeof(client_addr);

	epoll_event	ev;
	epoll_event	events[MAX_EVENTS];
	int			epoll_fd;
	int			nfds;

	t_request	req;
	bzero(&req, sizeof(req));

	epoll_fd = epoll_create(DEFAULT_EPOLL_SIZE);
	if (epoll_fd == -1)
		THROW_ERRNO("epoll_create");

	ev.events = EPOLLIN;
	ev.data.fd = sockfd;
	err = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &ev);
	if (err)
		THROW_ERRNO("epoll_ctl");

	while (true)
	{
		nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
		if (nfds == -1)
			THROW_ERRNO("epoll_wait");

		for (int i = 0; i < nfds; ++i)
		{
			if (events[i].data.fd == sockfd)
			{
				// accept new connection
				int client_sockfd = accept(sockfd, (sockaddr *)&client_addr, &client_addr_len);
				if (client_sockfd == -1)
					THROW_ERRNO("accept");

				int flags = fcntl(client_sockfd, F_GETFL, 0);
				fcntl(client_sockfd, F_SETFL, flags | O_NONBLOCK);
				ev.events = EPOLLIN | EPOLLET;
				ev.data.fd = client_sockfd;
				err = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_sockfd, &ev);
				if (err)
					THROW_ERRNO("epoll_ctl");
			}
			else
			{
				// handle new data in old connection
				if (handle_new_data(events, i) == 1)
					goto fin;
			}
		}
	}
	fin:
	close(epoll_fd);

	return ;
}
