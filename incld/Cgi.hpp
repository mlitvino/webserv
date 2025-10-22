#pragma once

#include <memory>
#include <vector>
#include <string>

#include "webserv.hpp"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define PYTHON_PATH "/usr/bin/python3"
#define PYTHON_EXT ".py"
#define PHP_PATH "/usr/bin/php-cgi"
#define PHP_EXT ".php"

class Cgi
{
	private:
		Client						&_client;
		std::string					_contentType;
		std::string					_uploadDir;

		int							_stdinFd;
		int							_stdoutFd;
		pid_t						_pid;
		bool						_headersParsed;

		std::string					_interpreter;
		std::string					_script;
		std::vector<char*>			_argv;
		std::vector<std::string>	_envStorage;
		std::vector<char*>			_envp;

		void	buildArgv();
		void	buildEnv();

		bool	createPipes(int inPipe[2], int outPipe[2]);
		void	configureParentFds(int stdinWriteFd, int stdoutReadFd, pid_t pid);
		bool	registerWithEpoll();
		void	cleanupCgiFds();

	public:
		Cgi(Client &client);
		~Cgi();

		bool				init();
		int					reapChild();
		int					killChild();

		int					getStdinFd();
		int					getStdoutFd();

		void				setUploadDir(const std::string &uploadDir);
};
