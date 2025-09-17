#include "webserv.hpp"

void	parser(t_data &data, char *conf_file)
{
	//test conf
	std::string	host = HOST;
	std::string	port = PORT;
	data.server_amount = 1;

	data.serverArray = new class Server[data.server_amount];

	data.serverArray[0].setHost(host);
	data.serverArray[0].setPort(port);

	if (conf_file)
	{

	}
}

