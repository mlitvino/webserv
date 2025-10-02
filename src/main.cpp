#include "webserv.hpp"

int	main(int ac, char **av)
{
	Program	program;

	if (ac > 2)
	{
		std::cout << "Usage: ./webserver [conf_file]" << std::endl;
		return 0;
	}

	try
	{
		program.parseConfigFile(av[1]);
		program.initSockets();
		program.waitEpollEvent();
	}
	catch (std::exception& e)
	{
		std::cerr << "Fatal Error: " << e.what() << std::endl;
	}

	return 0;
}
