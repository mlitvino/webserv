#include "Cgi.hpp"
#include "utils.hpp"
#include <cstring>
#include <cerrno>

std::string Cgi::buildScriptPath(std::string &interpreter)
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
			interpreter = loc.cgiPath;
			break;
		}
	}
	if (scriptPath.empty())
		scriptPath = reqPath;
	if (!scriptPath.empty() && scriptPath[0] != '/')
		scriptPath = "web/" + scriptPath;
	return scriptPath;
}

std::vector<char*> Cgi::buildArgv(const std::string &interpreter, const std::string &script, std::vector<std::string> &storage)
{
	if (!interpreter.empty()) {
		storage.push_back(interpreter);
		storage.push_back(script);
	} else {
		storage.push_back(script);
	}
	std::vector<char*> argv;
	for (size_t i = 0; i < storage.size(); ++i) argv.push_back(const_cast<char*>(storage[i].c_str()));
	argv.push_back(nullptr);
	return argv;
}

std::vector<char*> Cgi::buildEnv(std::vector<std::string> &storage, const std::string &body)
{
	storage.push_back("REQUEST_METHOD=" + _client._httpMethod);
	storage.push_back("CONTENT_LENGTH=" + std::to_string(body.size()));
	storage.push_back("SERVER_PROTOCOL=" + _client._httpVersion);
	std::vector<char*> envp;
	for (size_t i = 0; i < storage.size(); ++i) envp.push_back(const_cast<char*>(storage[i].c_str()));
	envp.push_back(nullptr);
	return envp;
}

bool Cgi::start()
{
	size_t headersEnd = _client._buffer.find("\r\n\r\n");
	std::string body;
	if (headersEnd != std::string::npos)
		body = _client._buffer.substr(headersEnd + 4);

	int inPipe[2];
	int outPipe[2];
	if (pipe(inPipe) == -1)
		return false;
	if (pipe(outPipe) == -1)
	{
		close(inPipe[0]);
		close(inPipe[1]);
		return false;
	}

	std::string interpreter;
	std::string script = buildScriptPath(interpreter);
	std::cout << "DEBUG: cgi script " << script << std::endl;
	std::vector<std::string> argvStorage; std::vector<char*> argv = buildArgv(interpreter, script, argvStorage);
	std::vector<std::string> envStorage; std::vector<char*> envp = buildEnv(envStorage, body);

	pid_t pid = fork();
	if (pid == -1) {
		close(inPipe[0]); close(inPipe[1]);
		close(outPipe[0]); close(outPipe[1]);
		return false;
	}
	if (pid == 0) {
		if (dup2(inPipe[0], STDIN_FILENO) == -1) _exit(127);
		if (dup2(outPipe[1], STDOUT_FILENO) == -1) _exit(127);
		close(inPipe[0]); close(inPipe[1]);
		close(outPipe[0]); close(outPipe[1]);
		if (!interpreter.empty())
			execve(interpreter.c_str(), argv.data(), envp.data());
		else
			execve(script.c_str(), argv.data(), envp.data());
		_exit(127);
	}
	// Parent
	close(inPipe[0]);
	close(outPipe[1]);
	utils::makeFdNonBlocking(inPipe[1]);
	utils::makeFdNonBlocking(outPipe[0]);
	_client._cgiInFd = inPipe[1];
	_client._cgiOutFd = outPipe[0];
	_client._cgiPid = pid;
	_client._cgiHeadersParsed = false;
	_contentType = "text/html";
	if (!body.empty()) _client._cgiBuffer = body; // pending stdin
	return true;
}

// Constructors + Destructors

Cgi::Cgi(Client &client)
	: _client(client)
	, _contentType("text/html")
{}

Cgi::~Cgi() {}
