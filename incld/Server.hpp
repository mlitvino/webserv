#pragma once

#include "webserv.hpp"
#include <cstring>

typedef enum s_err_pages
{
	ERR_404,
	MAX_ERRS
}	t_err_pages;

class Server
{
	private:
		std::string	_serverName;
		std::string	_host;
		std::string	_port;

		size_t		_clientBodySize;
		std::string	_errPagePath[MAX_ERRS];

		int			_sockfd;
	public:
		Server();
		~Server();

		void		prepareServer(addrinfo &hints, addrinfo *server);
		void		setHost(std::string &host);
		void		setPort(std::string &port);
		std::string	&getHost();
		std::string	&getPort();
		int			getSockfd();
};

