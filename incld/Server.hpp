#pragma once

#include <cstring>

#include "webserv.hpp"
#include "IEpollFdOwner.hpp"

typedef enum s_err_pages
{
	ERR_404,
	MAX_ERRS
}	t_err_pages;

class Server : public IEpollFdOwner
{
	private:
		std::string			_serverName;
		std::string			_host;
		std::string			_port;

		size_t				_clientBodySize;
		std::string			_errPagePath[MAX_ERRS];

		ClientHandlerDeq	_clients;

		int					_sockfd;
	public:
		Server();
		~Server();

		void		prepareSockFd(addrinfo &hints, addrinfo *server);
		void		setHost(std::string host);
		void		setPort(std::string port);
		std::string	&getHost();
		std::string	&getPort();
		int			getSockfd();
		size_t		getSizeClients();
		void		RemoveClientHandler(ClientHandler& handler, size_t index);

		void	handleEpollEvent(epoll_event &ev, int epoll_fd, int eventFd);
};

