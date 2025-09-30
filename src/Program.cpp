#include "Program.hpp"

void	Program::parseConfigFile(char *config_file)
{
	IpPortPtr	addr_port = std::make_unique<IpPort>();
	ServerPtr	new_server = std::make_shared<Server>();

	new_server->setHost(HOST);
	new_server->setPort(PORT);

	addr_port->_servers.push_back(new_server);
	_addrPortVec.push_back(std::move(addr_port));
}

void	Program::initServers()
{
	addrinfo	hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	for (IpPortPtr &ipPort: _addrPortVec)
	{
		ipPort->OpenSocket(hints, _servInfo);
		free(_servInfo);
		_servInfo = nullptr;
	}
}


// Canonical Form

Program::Program()
{
	_servInfo = nullptr;
}

Program::~Program()
{
	free(_servInfo);
}

