#include "webserv.hpp"
#include "Server.hpp"

void	Server::prepareServer(addrinfo &hints, addrinfo *server)
{
	int	err;

	err = getaddrinfo(_host.c_str(), _port.c_str(), &hints, &server);
	if (err)
		THROW(gai_strerror(err));

	int i = 0;
	for (addrinfo *p = server; p; p = p->ai_next, i++)
	{}
	std::cout << "addrinfo nodes: " << i << std::endl;

	_sockfd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
	if (_sockfd == -1)
		THROW_ERRNO("socket");

	int opt = 1;

	err = setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (err)
		THROW_ERRNO("setsockopt(SO_REUSEADDR)");

	err = bind(_sockfd, server->ai_addr, server->ai_addrlen);
	if (err)
		THROW_ERRNO("bind");

	err = listen(_sockfd, QUEUE_SIZE);
	if (err)
		THROW_ERRNO("listen");
}

// Setters, Getters

void	Server::setHost(std::string &host)
{
	_host = host;
}

void	Server::setPort(std::string &port)
{
	_port = port;
}

std::string	&Server::getHost()
{
	return _host;
}

std::string	&Server::getPort()
{
	return _port;
}

int	Server::getSockfd()
{
	return _sockfd;
}

// Constructors

Server::Server()
{
	_sockfd = -1;

}

Server::~Server()
{
	if (_sockfd != -1)
		close(_sockfd);
}

