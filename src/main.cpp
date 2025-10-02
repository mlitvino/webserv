#include "webserv.hpp"

void cleanup_data(Data& data) {
	// Smart pointers automatically clean up when the vector is destroyed
	data.servers.clear();
}

int main(int ac, char** av) {
	Data data;

	if (ac > 2) {
		std::cout << "Usage: ./webserver [conf_file]" << std::endl;
		return 0;
	}

	try {
		parser(data, av[1]);
		initServers(data);
		acceptingLoop(data);
	}
	catch (const std::exception& e) {
		std::cerr << "Fatal Error: " << e.what() << std::endl;
	}
	
	cleanup_data(data);
	return 0;
}
