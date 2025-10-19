#pragma once

#include <vector>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <memory>

class Client;
class IpPort;
using ClientPtr = std::shared_ptr<Client>;

class Cgi
{
	private:
		Client						&_client;
		std::string					_contentType;


		int							_stdinFd;
		int							_stdoutFd;
		pid_t						_pid;
		bool						_headersParsed;

		std::string					_interpreter;
		std::string					_script;
		std::vector<std::string>	_argvStorage;
		std::vector<char*>			_argv;
		std::vector<std::string>	_envStorage;
		std::vector<char*>			_envp;

		void	prepareScript();
		void	buildArgv();
		void	buildEnv();

		// Helpers to keep init() small and readable
		bool	createPipes(int inPipe[2], int outPipe[2]);
		void	configureParentFds(int stdinWriteFd, int stdoutReadFd, pid_t pid);
		bool	registerWithEpoll();
		void	cleanupCgiFds();

	public:
		Cgi(Client &client);
		~Cgi();

		bool				init();

		const std::string&	defaultContentType() const;
};
