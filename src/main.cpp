#include "webserv.hpp"

int	main(int ac, char **av)
{
	(void)ac;(void)av;


	int			err;
	addrinfo	hints;
	addrinfo	*server;
	int			sockfd = -1;

	int					client_sockfd = -1;
	sockaddr_storage	client_addr;
	socklen_t			client_addr_len = sizeof(client_addr);


	// (void)av;
	// if (ac != 2)
	// 	return 0;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	try
	{
		err = getaddrinfo(IN_DOMAIN, IN_PORT, &hints, &server);
		if (err)
			THROW(gai_strerror(err));

		int i = 0;
		for (addrinfo *p = server; p; p = p->ai_next, i++)
		{}
		std::cout << "addrinfo nodes: " << i << std::endl;

		sockfd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
		if (sockfd == -1)
			THROW_ERRNO("socket");

		int opt = 1;
		#ifdef DDEBUG
			err = setsockopt(sockfd, SOL_SOCKET, SO_DEBUG, &opt, sizeof(opt));
			if (err)
				THROW_ERRNO("setsockopt(SO_DEBUG)");
		#endif
			err = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
			if (err)
				THROW_ERRNO("setsockopt(SO_REUSEADDR)");

		err = bind(sockfd, server->ai_addr, server->ai_addrlen);
		if (err)
			THROW_ERRNO("bind");

		err = listen(sockfd, QUEUE_SIZE);
		if (err)
			THROW_ERRNO("listen");

		while (1)
		{
			client_sockfd = accept(sockfd, (sockaddr *)&client_addr, &client_addr_len);
			if (client_sockfd == -1)
				THROW_ERRNO("listen");


			char	buffer[1024] = {0};
			read(client_sockfd, buffer, sizeof(buffer));
			if (err)
				THROW_ERRNO("read");

			std::cout << "CLIENT REQUEST:\n" << buffer << std::endl;

			bzero(buffer, sizeof(buffer));
			strcpy(buffer, HTTP_STATUS);

			int html_fd = open(STATIC_SITE, O_RDWR);
			if (err)
				THROW_ERRNO("open");

			int len = read(html_fd, &buffer[sizeof(HTTP_STATUS) - 1], sizeof(buffer) / 2);
			if (len < 0)
				THROW_ERRNO("read");

			std::cout << "HTML_BUFFER:\n" << buffer << std::endl;

			send(client_sockfd, buffer, sizeof(buffer), 0);
			if (err)
				THROW_ERRNO("send");

			// send(client_sockfd, CRLF, sizeof(CRLF), 0);
			// if (err)
			// 	THROW_ERRNO("send");

			close(client_sockfd);
		}
	}
	catch (std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	close(client_sockfd);
	close(sockfd);
	freeaddrinfo(server);

	std::cout << "END" << std::endl;

	return 0;
}
