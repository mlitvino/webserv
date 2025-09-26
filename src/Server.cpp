#include "Server.hpp"

void	Server::RemoveClientHandler(ClientHandler& handler, size_t index)
{
	auto temp = std::move(_clients.back());
	// add numbers checking
	temp->setIndex(index);
	_clients.erase(_clients.begin() + index);
	if (index != _clients.size())
		_clients.insert(_clients.begin() + index, std::move(temp));

	std::cout << "Size after removing: " << _clients.size() << std::endl;
}

void	Server::handleEpollEvent(epoll_event &ev, int epoll_fd)
{
	std::cout << "Accepting new connection..." << std::endl;

	try
	{
		ClientHandlerPtr	new_client = std::make_unique<ClientHandler>(*this);
		new_client->acceptConnect(_sockfd, epoll_fd);
		_clients.push_back(std::move(new_client));
		std::cout << "New connection was accepted" << std::endl;
	}
	catch(const std::exception& e)
	{
		std::cout << "Connection failed: " << e.what() << std::endl;
	}
}

void	Server::prepareSockFd(addrinfo &hints, addrinfo *server)
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

size_t	Server::getSizeClients()
{
	return _clients.size();
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

