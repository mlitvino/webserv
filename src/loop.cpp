#include "webserv.hpp"

void	accepting_loop(int &sockfd)
{
	int					err;
	int					client_sockfd = -1;
	sockaddr_storage	client_addr;
	socklen_t			client_addr_len = sizeof(client_addr);

	while (1)
	{
		client_sockfd = accept(sockfd, (sockaddr *)&client_addr, &client_addr_len);
		if (client_sockfd == -1)
			THROW_ERRNO("listen");

		char	buffer[1024] = {0};
		err = read(client_sockfd, buffer, sizeof(buffer));
		if (err == -1)
			THROW_ERRNO("read");

		std::cout << "CLIENT REQUEST:\n" << buffer << std::endl;

		bzero(buffer, sizeof(buffer));
		strcpy(buffer, HTTP_STATUS);

		int html_fd = open(STATIC_SITE, O_RDWR);
		if (html_fd == -1)
			THROW_ERRNO("open");

		int len = read(html_fd, &buffer[sizeof(HTTP_STATUS) - 1], sizeof(buffer) / 2);
		if (len < 0)
			THROW_ERRNO("read");

		close(html_fd);

		std::cout << "SERVER RESPONSE:\n" << buffer << std::endl;

		err = send(client_sockfd, buffer, sizeof(buffer), 0);
		if (err == -1)
			THROW_ERRNO("send");

		close(client_sockfd);
	}
}
