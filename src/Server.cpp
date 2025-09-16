#include "webserv.hpp"
#include "Server.hpp"

void	Server::prepareServer()
{
	int	err;

	err = getaddrinfo(_host.c_str(), _port.c_str(), &_hints, &_server);
	if (err)
		THROW(gai_strerror(err));

	int i = 0;
	for (addrinfo *p = _server; p; p = p->ai_next, i++)
	{}
	std::cout << "addrinfo nodes: " << i << std::endl;

	_sockfd = socket(_server->ai_family, _server->ai_socktype, _server->ai_protocol);
	if (_sockfd == -1)
		THROW_ERRNO("socket");

	int opt = 1;

	err = setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (err)
		THROW_ERRNO("setsockopt(SO_REUSEADDR)");

	err = bind(_sockfd, _server->ai_addr, _server->ai_addrlen);
	if (err)
		THROW_ERRNO("bind");

	err = listen(_sockfd, QUEUE_SIZE);
	if (err)
		THROW_ERRNO("listen");

	freeaddrinfo(_server);
	_server = nullptr;
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
	_server = nullptr;
	_sockfd = -1;

	memset(&_hints, 0, sizeof(_hints));
	_hints.ai_family = AF_INET;
	_hints.ai_socktype = SOCK_STREAM;
	_hints.ai_flags = AI_PASSIVE;
}

Server::~Server()
{
	freeaddrinfo(_server);
	_server = nullptr;

	if (_sockfd != -1)
		close(_sockfd);
}

