#pragma once

#include "webserv.hpp"
#include "Client.hpp"
#include "ConfigParser.hpp"
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

class Cgi {
private:
	Client &_client;
	std::string _contentType;

	std::string buildScriptPath(std::string &interpreter);
	std::vector<char*> buildArgv(const std::string &interpreter, const std::string &script, std::vector<std::string> &storage);
	std::vector<char*> buildEnv(std::vector<std::string> &storage, const std::string &body);
public:
	Cgi(Client &client);
	~Cgi();

	bool start();

	const std::string &defaultContentType() const { return _contentType; }
};
