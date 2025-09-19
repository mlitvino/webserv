#include "webserv.hpp"

void	parser(Data &data, char *conf_file)
{
	// do it in loop for all servers in conf file
	ServerPtr	new_server = std::make_unique<Server>();
	new_server->setHost(std::string(HOST));
	new_server->setPort(std::string(PORT));
	data.servers.push_back(std::move(new_server));

	if (conf_file)
	{

	}
}

