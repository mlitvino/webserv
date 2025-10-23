#include "Client.hpp"

bool	Client::readRequest()
{
	char	buffer[IO_BUFFER_SIZE];
	int		bytesRead = read(_clientFd, buffer, sizeof(buffer) - 1);

	if (bytesRead > 0)
	{
		buffer[bytesRead] = '\0';
		_buffer.append(buffer, bytesRead);
		_lastActivity = g_current_time;
		return true;
	}
	else if (bytesRead == 0)
		return false;
	else if (bytesRead == -1)
	{
		THROW_ERRNO("read");
	}

	return true;
}

void	Client::sendResponse()
{
	std::cout << "Sending response..." << std::endl;
	int		bytesSent = 0;

	if (_responseOffset < _responseBuffer.size())
	{
		const char* buf = _responseBuffer.c_str() + _responseOffset;
		size_t toSend = _responseBuffer.size() - _responseOffset;
		bytesSent = send(_clientFd, buf, toSend, 0);
		if (bytesSent > 0)
			_responseOffset += static_cast<size_t>(bytesSent);
	}
	else if (_fileOffset < _fileSize && _fileFd >= 0)
	{
		off_t offset = static_cast<off_t>(_fileOffset);
		bytesSent = sendfile(_clientFd, _fileFd, &offset, _fileSize - offset);
		if (bytesSent > 0)
			_fileOffset += static_cast<size_t>(bytesSent);
	}

	if (_responseOffset >= _responseBuffer.size()
		&& _fileOffset >= _fileSize)
	{
		_fileOffset = 0;
		_fileSize = 0;
		_responseOffset = 0;
		_responseBuffer.clear();

		if (_keepAlive == false)
			return _ipPort.closeConnection(_clientFd);

		_postHandler.resetBodyState();
		closeFile();
		_state = ClientState::READING_REQUEST;
		utils::changeEpollHandler(_handlersMap, _clientFd, &_ipPort);
		return ;
	}

	if (bytesSent > 0)
	{
		_lastActivity = g_current_time;
	}
	else if (bytesSent == 0)
	{
		_ipPort.closeConnection(_clientFd);
	}
	else if (bytesSent == -1)
	{
		THROW_ERRNO("send");
	}
}

void	Client::closeFile()
{
	if (_fileFd != -1)
		close(_fileFd);
	_fileFd = -1;
	_fileSize = 0;
	_fileOffset = 0;
}

void	Client::openFile(std::string &filePath)
{
	int			err;
	struct stat	fileInfo;

	_fileFd = open(filePath.c_str(), O_RDWR | O_NONBLOCK, 667);
	if (_fileFd < 0)
		return ;
	utils::makeFdNoninheritable(_fileFd);
	err = fstat(_fileFd, &fileInfo);
	if (err == -1)
	{
		close(_fileFd);
		_fileFd = -1;
	}
	_fileSize = fileInfo.st_size;
	_fileOffset = 0;
}

void	Client::handleEpollEvent(epoll_event &ev, int eventFd)
{
	try
	{
		if (ev.events & (EPOLLIN | EPOLLHUP))
		{
			if (eventFd == _cgi.getStdoutFd()  && _state == ClientState::READING_CGI_OUTPUT)
			{
				handleCgiStdoutEvent();
				return;
			}
		}
		if (ev.events & EPOLLOUT)
		{
			if (eventFd == _clientFd && _state == ClientState::SENDING_RESPONSE)
			{
				sendResponse();
			}
			else if (eventFd == _cgi.getStdinFd() && _state == ClientState::WRITING_CGI_INPUT)
			{
				handleCgiStdinEvent();
			}
		}
	}
	catch (HttpException &e)
	{
		resetRequestData();
		_buffer.clear();
		auto FdclientPtr = _ipPort.getClientsMap().find(_clientFd);
		_ipPort.generateResponse(FdclientPtr->second, "", e.getStatusCode());
	}
	catch (std::exception &e)
	{
		_ipPort.closeConnection(eventFd);
	}
}

void	Client::handleCgiStdoutEvent()
{
	char	buf[IO_BUFFER_SIZE];

	int readBytes = read(_cgi.getStdoutFd(), buf, sizeof(buf));
	if (readBytes > 0)
	{
		_cgiBuffer.append(buf, readBytes);
		_lastActivity = g_current_time;
	}
	else
	{
		epoll_ctl(_ipPort.getEpollFd(), EPOLL_CTL_DEL, _cgi.getStdoutFd(), 0);
		close(_cgi.getStdoutFd());
		_handlersMap.erase(_cgi.getStdoutFd());
		if (readBytes == 0)
		{
			int status = _cgi.reapChild();
			if (status != 0)
			{
				closeFile();
				std::string	errorPage = _ownerServer->getCustomErrorPage(status);
				ClientPtr	self = _clientsMap.at(_clientFd);
				_ipPort.generateResponse(self, errorPage, status);
				return;
			}
			parseCgiOutput();
			utils::changeEpollHandler(_handlersMap, _clientFd, this);
			return;
		}
		else
			THROW_ERRNO("read CGI stdout");
	}
}

void	Client::handleCgiStdinEvent()
{
	char buf[IO_BUFFER_SIZE];

	int	readBytes = read(_fileFd, buf, sizeof(buf));
	if (readBytes > 0)
	{
		int	wroteBytes = write(_cgi.getStdinFd(), buf, readBytes);
		if (wroteBytes == -1)
		{
			THROW_HTTP(500, "write failed in CGI");
		}
		else if (wroteBytes < readBytes)
		{
			lseek(_fileFd, static_cast<off_t>(wroteBytes - readBytes), SEEK_CUR);
			return;
		}
		_lastActivity = g_current_time;
	}
	else
	{
		epoll_ctl(_ipPort.getEpollFd(), EPOLL_CTL_DEL, _cgi.getStdinFd(), 0);
		close(_cgi.getStdinFd());
		_handlersMap.erase(_cgi.getStdinFd());
		if (readBytes == 0)
		{
			closeFile();
			_state = ClientState::READING_CGI_OUTPUT;
			return;
		}
		else
		{
			THROW_HTTP(500, "read temp body file");
		}
	}

}

bool	Client::parseCgiOutput()
{
	_fileSize = 0;
	_fileOffset = 0;

	std::string::size_type	pos = _cgiBuffer.find("\r\n\r\n");
	std::string				headers;
	std::string				body;
	int						code = 200;
	if (pos != std::string::npos)
	{
		headers = _cgiBuffer.substr(0, pos);
		body = _cgiBuffer.substr(pos + 4);
	}
	else
		body = _cgiBuffer;

	std::string	statusLine = "HTTP/1.1 200 OK\r\n";
	std::string	outHeaders;
	if (headers.empty())
		THROW_HTTP(500, "Invalid CGI Status header");
	std::istringstream iss(headers);
	std::string line;
	while (std::getline(iss, line))
	{
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		std::string	status = "Status:";
		if (line.rfind(status, 0) == 0)
		{
			std::string	val = line.substr(status.size());
			try {
				code = std::stoi(val);
			}
			catch(std::exception& e) {
				THROW_HTTP(500, "Inernal Error");
			}
			if (code >= 400)
				THROW_HTTP(code, "Cgi returned error");
			statusLine = "HTTP/1.1 " + std::to_string(code) + " " + _ipPort.getStatusText(code) + "\r\n";
		}
		else
			outHeaders += line + "\r\n";
	}
	_keepAlive = false;
	_responseBuffer = statusLine;
	_responseBuffer += outHeaders;
	_responseBuffer += "Content-Length: " + std::to_string(body.size()) + "\r\n";
	_responseBuffer += "Connection: close\r\n\r\n";
	_responseBuffer += body;
	_responseOffset = 0;
	_state = ClientState::SENDING_RESPONSE;
	_cgiBuffer.clear();
	std::cout << "HTTP code for client: " << code << std::endl;
	return true;
}

void	Client::resetRequestData()
{
	_postHandler.resetBodyState();
	_contentLen = 0;
	_chunked = false;
	_contentType.clear();
	_query.clear();
	_redirectedUrl.clear();
	_fileType = FileType::REGULAR;
	_cgiBuffer.clear();
	_keepAlive = false;
	closeFile();
}

// Getters + Setters

int				Client::getFd() { return _clientFd; }

Time			Client::getLastActivity() { return _lastActivity; }
void			Client::setLastActivity(Time t) { _lastActivity = t; }

std::string&	Client::getBuffer() { return _buffer; }
void			Client::setBuffer(const std::string &v) { _buffer = v; }

std::string&	Client::getResponseBuffer() { return _responseBuffer; }
void			Client::setResponseBuffer(const std::string &v) { _responseBuffer = v; }

size_t			Client::getResponseOffset() { return _responseOffset; }
void			Client::setResponseOffset(size_t v) { _responseOffset = v; }

ClientState		Client::getState() { return _state; }
void			Client::setState(ClientState s) { _state = s; }

ServerPtr&		Client::getOwnerServer() { return _ownerServer; }
void			Client::setOwnerServer(const ServerPtr &srv) { _ownerServer = srv; }

std::string&	Client::getHttpMethod() { return _httpMethod; }
void			Client::setHttpMethod(const std::string &v) { _httpMethod = v; }

std::string&	Client::getHttpPath() { return _httpPath; }
void			Client::setHttpPath(const std::string &v) { _httpPath = v; }

std::string&	Client::getQuery() { return _query; }
void			Client::setQuery(const std::string &v) { _query = v; }

std::string&	Client::getHttpVersion() { return _httpVersion; }
void			Client::setHttpVersion(const std::string &v) { _httpVersion = v; }

size_t			Client::getContentLen() { return _contentLen; }
void			Client::setContentLen(size_t v) { _contentLen = v; }

bool			Client::isChunked() { return _chunked; }
void			Client::setChunked(bool v) { _chunked = v; }

bool			Client::isKeepAlive() { return _keepAlive; }
void			Client::setKeepAlive(bool v) { _keepAlive = v; }

std::string&	Client::getHostHeader() { return _hostHeader; }
void			Client::setHostHeader(const std::string &v) { _hostHeader = v; }

std::string&	Client::getContentType() { return _contentType; }
void			Client::setContentType(const std::string &v) { _contentType = v; }

std::string&	Client::getMultipartBoundary() { return _multipartBoundary; }
void			Client::setMultipartBoundary(const std::string &v) { _multipartBoundary = v; }

std::string&	Client::getResolvedPath() { return _resolvedPath; }
void			Client::setResolvedPath(const std::string &v) { _resolvedPath = v; }

FileType		Client::getFileType() { return _fileType; }
void			Client::setFileType(FileType t) { _fileType = t; }

std::string&	Client::getRedirectedUrl() { return _redirectedUrl; }
void			Client::setRedirectedUrl(const std::string &v) { _redirectedUrl = v; }

int				Client::getRedirectCode() { return _redirectCode; }
void			Client::setRedirectCode(int v) { _redirectCode = v; }

int				Client::getClientFd() { return _clientFd; }
void			Client::setClientFd(int fd) { _clientFd = fd; }

int				Client::getFileFd() { return _fileFd; }
void			Client::setFileFd(int fd) { _fileFd = fd; }

int				Client::getFileSize() { return _fileSize; }
void			Client::setFileSize(int sz) { _fileSize = sz; }

Cgi&			Client::getCgi() { return _cgi; }
PostRequestHandler&	Client::getPostRequestHandler() { return _postHandler; }

FdClientMap&		Client::getClientsMap() { return _clientsMap; }
FdEpollOwnerMap&	Client::getHandlersMap() { return _handlersMap; }
IpPort&				Client::getIpPort() { return _ipPort; }

// Constructors + Destructor

Client::Client(int clientFd, IpPort &owner)
	: _clientFd{clientFd}
	, _lastActivity{g_current_time}
	, _buffer()
	, _responseOffset{0}
	, _state(ClientState::READING_REQUEST)
	, _clientsMap(owner.getClientsMap())
	, _handlersMap(owner.getHandlersMap())
	, _ipPort(owner)
	, _chunked(false)
	, _keepAlive(false)
	, _hostHeader()
	, _fileFd{-1}
	, _fileSize{0}
	, _fileOffset{0}
	, _cgi{*this}
	, _postHandler{_ipPort}
{}

Client::~Client()
{
	if (_clientFd != -1)
		close(_clientFd);
	if (_fileFd != -1)
		close(_fileFd);
}

