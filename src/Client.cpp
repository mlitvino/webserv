#include "Client.hpp"

bool	Client::readRequest()
{
	char	buffer[IO_BUFFER_SIZE];
	int		bytesRead = read(_clientFd, buffer, sizeof(buffer) - 1);

	if (bytesRead > 0)
	{
		buffer[bytesRead] = '\0';
		_buffer.append(buffer, bytesRead);
		return true;
	}
	else if (bytesRead == 0)
	{
		std::cout << "client disconnected" << std::endl;
		return false;
	}
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

	std::cout << "bytes was send: " << bytesSent << std::endl;

	if (_responseOffset >= _responseBuffer.size()
		&& _fileOffset >= _fileSize)
	{
		std::cout << "responseSize: " << _responseBuffer.size() << std::endl;
		std::cout << "fileSize: " << _fileSize << std::endl;
		std::cout << "IF BUFFER EMPTY: " << (_buffer.empty() ? "YES" : "NO") << std::endl;
		std::cout << "Sending response is done" << std::endl;
		_fileOffset = 0;
		_fileSize = 0;
		_responseOffset = 0;
		_responseBuffer.clear();

		if (_keepAlive == false)
			return _ipPort.closeConnection(_clientFd);

		_postHandler.resetBodyState();
		if (_fileFd != -1)
		{
			close(_fileFd);
			_fileFd = -1;
		}
		_state = ClientState::READING_REQUEST;
		utils::changeEpollHandler(_handlersMap, _clientFd, &_ipPort);
		return ;
	}

	if (bytesSent == 0)
	{
		std::cout << "Connection was closed in resndResponse" << std::endl;
		_ipPort.closeConnection(_clientFd);
	}
	else if (bytesSent == -1)
	{
		THROW_ERRNO("send");
	}

	std::cout << "Sending part of response is done" << std::endl;
}

void	Client::closeFile(epoll_event &ev, int epollFd, int eventFd)
{
	close(_fileFd);
	_fileBuffer.clear();
	_fileSize = 0;
	_fileOffset = 0;
}

void	Client::openFile(std::string &filePath)
{
	int			err;
	struct stat	fileInfo;
	_filePath = filePath;

	_fileFd = open(_filePath.c_str(), O_RDWR | O_NONBLOCK, 667);
	if (_fileFd < 0)
		return ;

	err = fstat(_fileFd, &fileInfo);
	if (err == -1)
	{
		close(_fileFd);
		_fileFd = -1;
	}
	_fileSize = fileInfo.st_size;
	_fileOffset = 0;
}

void	Client::handleEpollEvent(epoll_event &ev, int epollFd, int eventFd)
{
	try
	{
		if (ev.events & (EPOLLIN | EPOLLHUP))
		{
			if (eventFd == _cgi.getStdoutFd()  && _state == ClientState::READING_CGI_OUTPUT)
			{
				handleCgiStdoutEvent(ev);
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
				handleCgiStdinEvent(ev);
			}
		}
	}
	catch (HttpException &e)
	{
		resetRequestData();
		_buffer.clear();
		std::cout << "HttpException: " << e.what() << ", statusCode " << e.getStatusCode() << std::endl;
		auto FdclientPtr = _ipPort._clientsMap.find(_clientFd);
		_ipPort.generateResponse(FdclientPtr->second, "", e.getStatusCode());
	}
	catch (std::exception &e)
	{
		std::cout << "Exception: " << e.what() << std::endl;
		_ipPort.closeConnection(eventFd);
	}
}

void	Client::handleCgiStdoutEvent(epoll_event &ev)
{
	std::cout << "Client: cgi stdout" << std::endl;
	char	buf[IO_BUFFER_SIZE];

	ssize_t n = read(_cgi.getStdoutFd(), buf, sizeof(buf));
	if (n > 0)
	{
		std::cout << "handleCgiOut, n>0" << std::endl;
		_cgiBuffer.append(buf, static_cast<size_t>(n));
	}
	else if (n == 0)
	{
		std::cout << "handleCgiOut, n=0" << std::endl;
		// EOF from CGI stdout
		// Parse headers if not yet done and prepare final response
		int status = _cgi.reapChild();
		if (status != 0)
		{
			if (_fileFd != -1)
			{
				close(_fileFd);
				_fileFd = -1;
			}
			THROW_HTTP(500, "Child failed");
		}
		if (!parseCgiHeadersAndPrepareResponse())
		{
			_responseBuffer = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
			_state = ClientState::SENDING_RESPONSE;
		}
		epoll_ctl(_ipPort._epollFd, EPOLL_CTL_DEL, _cgi.getStdoutFd(), 0);
		_handlersMap.erase(_cgi.getStdoutFd());
		close(_cgi.getStdoutFd());
		utils::changeEpollHandler(_handlersMap, _clientFd, this);
		return;
	}
	else
	{
		std::cout << "handleCgiOut, n<0" << std::endl;
		if (errno == EWOULDBLOCK)
		{
			std::cout << "handleCgiOut, errno == EWOULDBLOCK" << std::endl;
		}
		if (errno == EAGAIN)
		{
			std::cout << "handleCgiOut, errno == EAGAIN" << std::endl;
		}
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return;
		std::cout << "NOOO" << std::endl;
		THROW_ERRNO("read CGI stdout");
	}
}

void	Client::handleCgiStdinEvent(epoll_event &ev)
{
	std::cout << "Client: cgi stdin" << std::endl;
	// Drain request body temp file into CGI stdin
	char buf[IO_BUFFER_SIZE];

	ssize_t n = read(_fileFd, buf, sizeof(buf));
	if (n > 0)
	{
		ssize_t wr = write(_cgi.getStdinFd(), buf, static_cast<size_t>(n));
		if (wr == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				// put back read data into file by seeking back
				lseek(_fileFd, -n, SEEK_CUR);
				return;
			}
			THROW_ERRNO("write CGI stdin");
		}
		else if (wr < n)
		{
			// partial write; seek back the unwritten part
			lseek(_fileFd, static_cast<off_t>(wr - n), SEEK_CUR);
			return;
		}
	}
	else if (n == 0)
	{
		// Finished sending body; close CGI stdin and stop monitoring
		epoll_ctl(_ipPort._epollFd, EPOLL_CTL_DEL, _cgi.getStdinFd(), 0);
		_handlersMap.erase(_cgi.getStdinFd());
		close(_cgi.getStdinFd());
		// We can close the temp file as well
		if (_fileFd != -1)
		{
			close(_fileFd);
			_fileFd = -1;
		}
		return;
	}
	else
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return;
		THROW_ERRNO("read temp body file");
	}
}

bool	Client::parseCgiHeadersAndPrepareResponse()
{
	// Find the end of CGI headers: a blank line CRLF CRLF
	_fileSize = 0;
	_fileOffset = 0;


	std::string::size_type pos = _cgiBuffer.find("\r\n\r\n");
	std::string headers;
	std::string body;
	if (pos != std::string::npos)
	{
		headers = _cgiBuffer.substr(0, pos);
		body = _cgiBuffer.substr(pos + 4);
	}
	else
	{
		// No headers delimiter found; assume entire buffer is body with default content-type
		body = _cgiBuffer;
	}

	std::string statusLine = "HTTP/1.1 200 OK\r\n";
	std::string outHeaders;
	// Parse CGI-style headers (e.g., Status:, Content-Type:)
	if (!headers.empty())
	{
		std::istringstream iss(headers);
		std::string line;
		while (std::getline(iss, line))
		{
			if (!line.empty() && line.back() == '\r')
				line.pop_back();
			if (line.rfind("Status:", 0) == 0)
			{
				std::string val = line.substr(7);
				while (!val.empty() && isspace(val.front())) val.erase(0,1);
				outHeaders += "Status: " + val + "\r\n"; // we'll convert to HTTP status below
				// Extract code
				std::istringstream cs(val);
				int code; cs >> code;
				if (!cs.fail())
				{
					// Minimal mapping to status text
					std::string text = (code==200?"OK": code==302?"Found": code==404?"Not Found": code==500?"Internal Server Error":"");
					statusLine = "HTTP/1.1 " + std::to_string(code) + " " + (text.empty()?"":text) + "\r\n";
				}
			}
			else if (line.rfind("Content-Type:", 0) == 0)
			{
				outHeaders += line + "\r\n";
			}
			else if (line.rfind("Location:", 0) == 0)
			{
				// Support CGI redirect
				outHeaders += line + "\r\n";
				statusLine = "HTTP/1.1 302 Found\r\n";
			}
		}
	}
	else
	{
		// No headers; set default content-type
		outHeaders += "Content-Type: " + _cgi.defaultContentType() + "\r\n";
	}

	_responseBuffer = statusLine;
	_responseBuffer += outHeaders;
	_responseBuffer += "Content-Length: " + std::to_string(body.size()) + "\r\n";
	_responseBuffer += "Connection: close\r\n\r\n";
	_responseBuffer += body;
	_responseOffset = 0;
	_state = ClientState::SENDING_RESPONSE;
	_cgiBuffer.clear();
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


	if (_fileFd != -1)
	{
		close(_fileFd);
		_fileFd = -1;
	}
}

// Getters + Setters

int		Client::getFd()
{
	return _clientFd;
}

// Constructors + Destructor

Client::Client(sockaddr_storage clientAddr, socklen_t	clientAddrLen, int	clientFd, IpPort &owner)
	: _buffer()
	, _responseOffset{0}
	, _state(ClientState::READING_REQUEST)
	, _clientsMap(owner._clientsMap)
	, _handlersMap(owner._handlersMap)
	, _ipPort(owner)
	, _chunked(false)
	, _keepAlive(false)
	, _hostHeader()
	, _clientAddr{clientAddr}
	, _clientAddrLen{clientAddrLen}
	, _clientFd{clientFd}
	, _fileFd{-1}
	, _fileBuffer()
	, _fileSize{0}
	, _fileOffset{0}
	, _cgi{*this}
	, _postHandler{_ipPort}
{

}

Client::~Client()
{
	if (_clientFd != -1)
		close(_clientFd);
	if (_fileFd != -1)
		close(_fileFd);
}

