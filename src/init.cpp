#include "webserv.hpp"

void	init_servers(addrinfo *&server, int &sockfd)
{
	int			err;
	addrinfo	hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;


	err = getaddrinfo(DOMAIN, PORT, &hints, &server);
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

	err = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (err)
		THROW_ERRNO("setsockopt(SO_REUSEADDR)");

	err = bind(sockfd, server->ai_addr, server->ai_addrlen);
	if (err)
		THROW_ERRNO("bind");

	err = listen(sockfd, QUEUE_SIZE);
	if (err)
		THROW_ERRNO("listen");
}
