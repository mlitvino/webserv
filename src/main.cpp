#include "webserv.hpp"

int	main(int ac, char **av)
{
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

		sockfd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
		if (sockfd == -1)
			THROW_ERRNO("socket");

		err = bind(sockfd, server->ai_addr, server->ai_addrlen);
		if (err)
			THROW_ERRNO("bind");

		err = listen(sockfd, QUEUE_SIZE);
		if (err)
			THROW_ERRNO("listen");

		client_sockfd = accept(sockfd, (sockaddr *)&client_addr, &client_addr_len);
		if (client_sockfd == -1)
			THROW_ERRNO("listen");


		char buffer[1024] = {0};
		read(client_sockfd, buffer, sizeof(buffer));

		std::cout << "CONTENT: " << "(" << buffer << ")" << std::endl;
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
