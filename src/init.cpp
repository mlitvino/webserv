#include "webserv.hpp"

void	init_servers(t_data &data)
{
	addrinfo	hints;
	addrinfo	*server;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	for (int i = 0; i < data.server_amount; ++i)
	{
		try
		{
			server = nullptr;
			data.serverArray[i].prepareServer(hints, server);
			freeaddrinfo(server);
		}
		catch (std::exception &e)
		{
			freeaddrinfo(server);
			throw;
		}
	}
}
