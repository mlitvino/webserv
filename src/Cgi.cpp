#include "Cgi.hpp"

#include "webserv.hpp"
#include "utils.hpp"
#include "ConfigParser.hpp"

bool	Cgi::prepareScript()
{

	_script = _client._resolvedPath;
	switch (_cgiType)
	{
		case CgiType::PYTHON:
			_interpreter = PYTHON_PATH;
			break;
		case CgiType::PHP:
			_interpreter = PHP_PATH;
			break;
		case CgiType::NONE:
		default:
			return false;
	}
	return true;
}

void	Cgi::buildArgv()
{
	if (!_interpreter.empty())
	{
		_argv.push_back(const_cast<char*>(_interpreter.c_str()));
		_argv.push_back(const_cast<char*>(_script.c_str()));
	}
	else
	{
		_argv.push_back(const_cast<char*>(_script.c_str()));
	}
	_argv.push_back(nullptr);
}

void	Cgi::buildEnv()
{
	_envStorage.clear();
	_envStorage.push_back("REQUEST_METHOD=" + _client._httpMethod);
	_envStorage.push_back(std::string("CONTENT_LENGTH=") + std::to_string(_client._fileSize));
	_envStorage.push_back("SERVER_PROTOCOL=" + _client._httpVersion);
	_envStorage.push_back(std::string("SCRIPT_FILENAME=") + _script);
	_envStorage.push_back(std::string("REQUEST_URI=") + _client._httpPath);
	_envStorage.push_back("GATEWAY_INTERFACE=CGI/1.1");
	_envp.clear();
	for (auto &envLine : _envStorage)
		_envp.push_back(const_cast<char*>(envLine.c_str()));
	_envp.push_back(nullptr);
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
	, _cgiType(CgiType::NONE)
{}

Cgi::~Cgi() {}

const	std::string& Cgi::defaultContentType() const
{
	return _contentType;
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
	evIn.events = EPOLLIN | EPOLLOUT;
	evIn.data.fd = _stdoutFd;
	if (epoll_ctl(_client._ipPort._epollFd, EPOLL_CTL_ADD, _stdoutFd, &evIn) == -1)
	{
		cleanupCgiFds();
		return false;
	}
	_client._handlersMap.emplace(_stdoutFd, &_client);

	epoll_event evOut = {0};
	evOut.events = EPOLLIN | EPOLLOUT;
	evOut.data.fd = _stdinFd;
	if (epoll_ctl(_client._ipPort._epollFd, EPOLL_CTL_ADD, _stdinFd, &evOut) == -1)
	{
		epoll_ctl(_client._ipPort._epollFd, EPOLL_CTL_DEL, _stdoutFd, 0);
		_client._handlersMap.erase(_stdoutFd);
		cleanupCgiFds();
		return false;
	}
	_client._handlersMap.emplace(_stdinFd, &_client);
	return true;
}

bool	Cgi::init()
{
	int	inPipe[2] = {-1, -1};
	int	outPipe[2] = {-1, -1};

	if (!prepareScript())
		return false;
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
		return false;

	_client._state = ClientState::READING_CGI_OUTPUT;

	return true;
}
