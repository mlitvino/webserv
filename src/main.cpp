#include "webserv.hpp"

int	main(int ac, char **av)
{
	int			res;
	addrinfo	hints;
	addrinfo	*server;
	int			sockfd;

	// (void)av;
	// if (ac != 2)
	// 	return 0;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	try
	{
		res = getaddrinfo(IN_DOMAIN, IN_PORT, &hints, &server);
		if (res)
			THROW(gai_strerror(res));

		sockfd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
		if (sockfd == -1)
			THROW_ERRNO("socket");

		res = bind(sockfd, server->ai_addr, server->ai_addrlen);
		if (res)
			THROW_ERRNO("bind");

	}
	catch (std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	return 0;
}
