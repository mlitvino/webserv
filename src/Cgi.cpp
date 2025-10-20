#include "Cgi.hpp"

#include "webserv.hpp"
#include "utils.hpp"
#include "ConfigParser.hpp"

void Cgi::prepareScript()
{
	std::string reqPath = _client._httpPath;
	const std::vector<Location> &locs = _client._ownerServer->getLocations();

	if (!reqPath.empty() && reqPath[0] == '/')
		reqPath = reqPath.substr(1);

	std::string scriptPath;
	for (size_t i = 0; i < locs.size(); ++i)
	{
		const Location &loc = locs[i];
		if (_client._httpPath.rfind(loc.path, 0) == 0)
		{
			std::string root = loc.root;
			if (!root.empty() && root[0] == '/') root = root.substr(1);
			std::string rel = _client._httpPath.substr(loc.path.size());
			if (!rel.empty() && rel[0] == '/') rel = rel.substr(1);
			if (!root.empty()) {
				scriptPath = root;
				if (!scriptPath.empty() && scriptPath.back() != '/' && !rel.empty()) scriptPath += '/';
				scriptPath += rel;
			} else {
				scriptPath = reqPath;
			}
			break;
		}
	}
	if (scriptPath.empty())
		scriptPath = reqPath;
	if (!scriptPath.empty() && scriptPath[0] != '/')
		scriptPath = "web/" + scriptPath;
	_script = scriptPath;

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
			_interpreter.clear();
			break;
	}
}

void Cgi::buildArgv()
{
	_argvStorage.clear();
	if (!_interpreter.empty()) {
		_argvStorage.push_back(_interpreter);
		_argvStorage.push_back(_script);
	} else {
		_argvStorage.push_back(_script);
	}
	_argv.clear();
	for (size_t i = 0; i < _argvStorage.size(); ++i) _argv.push_back(const_cast<char*>(_argvStorage[i].c_str()));
	_argv.push_back(nullptr);
}

void Cgi::buildEnv()
{
	_envStorage.clear();
	_envStorage.push_back("REQUEST_METHOD=" + _client._httpMethod);
	// CONTENT_LENGTH can be filled later when known (stdin until EOF if unknown)
	_envStorage.push_back("SERVER_PROTOCOL=" + _client._httpVersion);
	_envp.clear();
	for (size_t i = 0; i < _envStorage.size(); ++i) _envp.push_back(const_cast<char*>(_envStorage[i].c_str()));
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
	, _argvStorage()
	, _argv()
	, _envStorage()
	, _envp()
	, _cgiType(CgiType::NONE)
{}

Cgi::~Cgi() {}

const std::string& Cgi::defaultContentType() const
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

// forkExec inlined inside init() for clarity

void Cgi::configureParentFds(int stdinWriteFd, int stdoutReadFd, pid_t pid)
{
	utils::makeFdNonBlocking(stdinWriteFd);
	utils::makeFdNonBlocking(stdoutReadFd);
	_stdinFd = stdinWriteFd;
	_stdoutFd = stdoutReadFd;
	_pid = pid;
	_headersParsed = false;
	_contentType = "text/html";
}

void Cgi::cleanupCgiFds()
{
	if (_stdoutFd != -1)
		close(_stdoutFd);
	if (_stdinFd != -1)
		close(_stdinFd);
	_stdoutFd = -1;
	_stdinFd = -1;
}

bool Cgi::registerWithEpoll()
{
	epoll_event evIn = {0};
	evIn.events = EPOLLIN | EPOLLET;
	evIn.data.fd = _stdoutFd;
	if (epoll_ctl(_client._ipPort._epollFd, EPOLL_CTL_ADD, _stdoutFd, &evIn) == -1)
	{
		cleanupCgiFds();
		return false;
	}
	_client._handlersMap.emplace(_stdoutFd, &_client);

	epoll_event evOut = {0};
	evOut.events = EPOLLOUT | EPOLLET;
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

bool Cgi::init()
{
	int inPipe[2] = {-1, -1};
	int outPipe[2] = {-1, -1};
	if (!createPipes(inPipe, outPipe))
		return false;

	prepareScript();
	buildArgv();
	buildEnv();

	pid_t pid = fork();
	if (pid == -1)
	{
		close(inPipe[STDIN_FILENO]); close(inPipe[STDOUT_FILENO]);
		close(outPipe[STDIN_FILENO]); close(outPipe[STDOUT_FILENO]);
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
		THROW_CHILD("execve");
	}

	// Parent
	close(inPipe[STDIN_FILENO]);
	close(outPipe[STDOUT_FILENO]);
	configureParentFds(inPipe[STDOUT_FILENO], outPipe[STDIN_FILENO], pid);
	if (!registerWithEpoll())
		return false;

	_client._state = ClientState::CGI_READING_OUTPUT;
	return true;
}
