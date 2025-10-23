#include "Cgi.hpp"
#include "ConfigParser.hpp"

void	Cgi::buildArgv()
{
	_argv.clear();

	_script = _client._resolvedPath;
	size_t		dot = _script.find_last_of(".");
	std::string	ext = _script.substr(dot);

	if (ext == PYTHON_EXT)
	{
		_interpreter = PYTHON_PATH;
	}
	else if (ext == PHP_EXT)
	{
		_interpreter = PHP_PATH;
	}
	else
	{
		_interpreter = _script;
	}
	_argv.push_back(const_cast<char*>(_interpreter.c_str()));
	if (ext == PHP_EXT)
		_argv.push_back(const_cast<char*>("-f"));
	_argv.push_back(const_cast<char*>(_script.c_str()));
	_argv.push_back(nullptr);
}

void	Cgi::buildEnv()
{
	_envStorage.clear();

	_envStorage.push_back("REQUEST_METHOD=" + _client._httpMethod);
	_envStorage.push_back(std::string("CONTENT_LENGTH=") + std::to_string(_client._fileSize));
	_envStorage.push_back("SERVER_PROTOCOL=" + _client._httpVersion);
	_envStorage.push_back(std::string("SCRIPT_FILENAME=") + _script);
	_envStorage.push_back("CONTENT_TYPE=" + _client._contentType);
	_envStorage.push_back(std::string("REQUEST_URI=") + _client._httpPath);
	_envStorage.push_back(std::string("PATH_INFO=") + _script);
	_envStorage.push_back("GATEWAY_INTERFACE=CGI/1.1");
	_envStorage.push_back("REDIRECT_STATUS=200");
	_envStorage.push_back("QUERY_STRING=" + _client._query);

	if (_client._httpMethod == "POST")
		_envStorage.push_back(std::string("UPLOAD_DIR=") + _uploadDir);

	_envp.clear();
	for (auto &envLine : _envStorage)
		_envp.push_back(const_cast<char*>(envLine.c_str()));
	_envp.push_back(nullptr);
}

bool Cgi::createPipes(int inPipe[2], int outPipe[2])
{
	if (pipe(inPipe) == -1)
		return false;
	if (pipe(outPipe) == -1)
	{
		close(inPipe[STDIN_FILENO]);
		close(inPipe[STDOUT_FILENO]);
		return false;
	}
	return true;
}


void	Cgi::configureParentFds(int stdinWriteFd, int stdoutReadFd, pid_t pid)
{
	utils::makeFdNonBlocking(stdinWriteFd);
	utils::makeFdNonBlocking(stdoutReadFd);
	_stdinFd = stdinWriteFd;
	_stdoutFd = stdoutReadFd;
	_pid = pid;
	_headersParsed = false;
	_contentType = "text/html";
}

void	Cgi::cleanupCgiFds()
{
	if (_stdoutFd != -1)
		close(_stdoutFd);
	if (_stdinFd != -1)
		close(_stdinFd);
	_stdoutFd = -1;
	_stdinFd = -1;
}

bool	Cgi::registerWithEpoll()
{
	epoll_event evIn = {0};
	evIn.events = EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLERR;
	evIn.data.fd = _stdoutFd;
	if (epoll_ctl(_client.getIpPort().getEpollFd(), EPOLL_CTL_ADD, _stdoutFd, &evIn) == -1)
	{
		cleanupCgiFds();
		return false;
	}
	_client.getHandlersMap().emplace(_stdoutFd, &_client);

	epoll_event evOut = {0};
	evOut.events = EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLERR;
	evOut.data.fd = _stdinFd;
	if (epoll_ctl(_client.getIpPort().getEpollFd(), EPOLL_CTL_ADD, _stdinFd, &evOut) == -1)
	{
		_client.getHandlersMap().erase(_stdoutFd);
		cleanupCgiFds();
		return false;
	}
	_client.getHandlersMap().emplace(_stdinFd, &_client);
	return true;
}

bool	Cgi::init()
{
	int	inPipe[2] = {-1, -1};
	int	outPipe[2] = {-1, -1};

	if (!createPipes(inPipe, outPipe))
		return false;

	buildArgv();
	buildEnv();

	pid_t pid = fork();
	if (pid == -1)
	{
		close(inPipe[STDIN_FILENO]);
		close(inPipe[STDOUT_FILENO]);
		close(outPipe[STDIN_FILENO]);
		close(outPipe[STDOUT_FILENO]);
		return false;
	}
	if (pid == 0)
	{
		if (dup2(inPipe[STDIN_FILENO], STDIN_FILENO) == -1
			|| dup2(outPipe[STDOUT_FILENO], STDOUT_FILENO) == -1)
		{
			THROW_CHILD("dup2");
		}
		close(inPipe[STDIN_FILENO]);
		close(inPipe[STDOUT_FILENO]);
		close(outPipe[STDIN_FILENO]);
		close(outPipe[STDOUT_FILENO]);
		execve(_interpreter.c_str(), _argv.data(), _envp.data());
		THROW_CHILD("execve");
	}

	close(inPipe[STDIN_FILENO]);
	close(outPipe[STDOUT_FILENO]);
	configureParentFds(inPipe[STDOUT_FILENO], outPipe[STDIN_FILENO], pid);
	if (!registerWithEpoll())
	{
		kill(_pid, SIGKILL);
		int status;
		waitpid(_pid, &status, 0);
		_pid = -1;
		_pid = -1;
		return false;
	}
	_client.setState(ClientState::READING_CGI_OUTPUT);
	std::cout << "Client in cgi init was changed" << std::endl;
	return true;
}

int	Cgi::reapChild()
{
	int	status;
	waitpid(_pid, &status, 0);
	_pid = -1;
	return status;
}

int	Cgi::killChild()
{
	int status = 0;
	if (_pid != -1)
	{
		kill(_pid, SIGKILL);
		waitpid(_pid, &status, 0);
	}
	_pid = -1;
	return status;
}

// Getters + Setters

int	Cgi::getStdinFd()
{
	return _stdinFd;
}

int	Cgi::getStdoutFd()
{
	return _stdoutFd;
}

void	Cgi::setUploadDir(const std::string &uploadDir)
{
	_uploadDir = uploadDir;
}

// Constructors + Destructors

Cgi::Cgi(Client &client)
	: _client(client)
	, _contentType("text/html")
	, _stdinFd(-1)
	, _stdoutFd(-1)
	, _pid(-1)
	, _headersParsed(false)
	, _interpreter()
	, _script()
	, _argv()
	, _envStorage()
	, _envp()
{}

Cgi::~Cgi()
{
	if (_stdinFd != -1)
	{
		close(_stdinFd);
	}
	if (_stdoutFd != -1)
	{
		close(_stdoutFd);
	}
	killChild();
}

