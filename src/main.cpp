#include "webserv.hpp"

int	main(int ac, char **av)
{
	GlobalData	data;

	if (ac > 2)
	{
		std::cout << "Usage: ./webserver [conf_file]" << std::endl;
		return 0;
	}

	try
	{
		parser(data, av[1]);
		init_servers(data);
		accepting_loop(data);
	}
	catch (std::exception& e)
	{
		std::cerr << "Fatal Error: " << e.what() << std::endl;
	}

	return 0;
}
