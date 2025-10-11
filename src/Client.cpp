#include "Client.hpp"

bool	Client::readRequest()
{
	char	buffer[IO_BUFFER_SIZE];
	int		bytesRead = read(_clientFd, buffer, sizeof(buffer) - 1);

	try
	{
		if (bytesRead > 0)
		{
			buffer[bytesRead] = '\0';
			_buffer.append(buffer, bytesRead);
			return true;
		}
		else if (bytesRead == 0)
		{
			std::cout << "DEBUG: Client disconnected" << std::endl;
			return false;
		}
		else if (bytesRead == -1)
		{
			THROW_ERRNO("read");
		}
	}
	catch (std::exception& e)
	{
		std::cout << "Exception: " << e.what() << std::endl;
		return false;
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
		_responseBuffer.clear();
		size_t	endRequest = _buffer.find("\r\n\r\n") + 4;
		_buffer.erase(0, endRequest);
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

	std::cout << "Sending response is done" << std::endl;
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
	if (err)
	{
		THROW_ERRNO("fstat");
	}
	_fileSize = fileInfo.st_size;
	_fileOffset = 0;
}

void	Client::handleEpollEvent(epoll_event &ev, int epollFd, int eventFd)
{
	if (ev.events & EPOLLIN)
	{
		if (_state == ClientState::GETTING_FILE)
		{

		}
	}
	else if (ev.events & EPOLLOUT)
	{
		if (_state == ClientState::SENDING_RESPONSE)
		{
			sendResponse();
		}
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
{}

Client::~Client()
{
	if (_clientFd != -1)
		close(_clientFd);
	if (_fileFd != -1)
		close(_fileFd);
}

