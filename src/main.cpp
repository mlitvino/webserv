#include "webserv.hpp"

int	main(int ac, char **av)
{
	Server	Server;
	std::string	host = HOST;
	std::string port = PORT;

	if (ac > 2)
	{
		std::cout << "Usage: ./webserver [conf_file]" << std::endl;
		return 0;
	}

	try
	{
		parser(av[1]);
		//init_servers(server, sockfd);
		Server.setHost(host);
		Server.setPort(port);
		Server.prepareServer();
		accepting_loop(Server.getSockfd());
	}
	catch (std::exception& e)
	{
		std::cerr << "Fatal Error: " << e.what() << std::endl;
	}

	return 0;
}
