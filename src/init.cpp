#include "webserv.hpp"

void	init_servers(GlobalData &data)
{
	addrinfo	hints;
	addrinfo	*srv_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	for (ServerPtr srv : data.servers)
	{
		try
		{
			srv_info = nullptr;
			srv->prepareServer(hints, srv_info);
			freeaddrinfo(srv_info);
		}
		catch (std::exception &e)
		{
			freeaddrinfo(srv_info);
			throw;
		}
	}
}
