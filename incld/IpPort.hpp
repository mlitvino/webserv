#pragma once

#include <utility>

#include "webserv.hpp"

class IpPort
{
	public:
		std::string		_addrPort;
		ServerDeq		_servers;
		int				_sockFd;

		void	OpenSocket(addrinfo &hints, addrinfo *_servInfo);
};
