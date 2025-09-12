#include "webserv.hpp"

int	main(int ac, char **av)
{
	if (ac > 2)
	{
		std::cout << "Usage: ./webserver [conf_file]" << std::endl;
		return 0;
	}

	addrinfo	*server = 0;
	int			sockfd = -1;

	try
	{
		parser(av[1]);
		init_servers(server, sockfd);
		accepting_loop(sockfd);
	}
	catch (std::exception& e)
	{
		std::cout << "Fatal Error: " << e.what() << std::endl;
	}

	close(sockfd);

	return 0;
}
