#pragma once

#include <memory>
#include <vector>
#include <string>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

class Client;
class IpPort;
using ClientPtr = std::shared_ptr<Client>;
enum class CgiType;

#define PYTHON_PATH "/usr/bin/python3"
#define PYTHON_EXT ".py"
#define PHP_PATH "/usr/bin/php-cgi"
#define PHP_EXT ".php"

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
		std::vector<char*>			_argv;
		std::vector<std::string>	_envStorage;
		std::vector<char*>			_envp;

		bool	prepareScript();
		void	buildArgv();
		void	buildEnv();

		bool	createPipes(int inPipe[2], int outPipe[2]);
		void	configureParentFds(int stdinWriteFd, int stdoutReadFd, pid_t pid);
		bool	registerWithEpoll();
		void	cleanupCgiFds();

	public:
		std::string					_uploadDir;

		Cgi(Client &client);
		~Cgi();

		bool				init();

		const std::string&	defaultContentType() const;

		int					getStdinFd() const { return _stdinFd; }
		int					getStdoutFd() const { return _stdoutFd; }
};
