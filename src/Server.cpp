#include "Server.hpp"

// Setters, Getters

void	Server::setHost(std::string host)
{
	_host = host;
}

void	Server::setPort(std::string port)
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

