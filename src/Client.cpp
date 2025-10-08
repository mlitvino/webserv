#include "Client.hpp"

void	Client::sendFile(epoll_event &ev, int epollFd, int eventFd)
{
	std::cout << "Sending file..." << std::endl;
	int flags = MSG_NOSIGNAL | MSG_MORE;

	if (!_fileBuffer.empty())
	{
		int err = send(_clientFd, _fileBuffer.c_str(), _fileBuffer.size(), flags);
		if (err < 0)
		{
			THROW_ERRNO("send");
		}
	}

	if (_fileSize == _fileOffset)
	{
		closeFile(ev, epollFd, eventFd);
		_state = ClientState::READING_REQUEST;
		auto it = _handlersMap.find(_clientFd);
		if (it == _handlersMap.end())
		{
			THROW_ERRNO("Unknown fd in map");
		}
		(*it).second = &_ipPort;
	}
	std::cout << "Sending file is done" << std::endl;
}

void	Client::sendResponse(epoll_event &ev, int epollFd, int eventFd)
{
	int flags = MSG_NOSIGNAL | MSG_MORE;

	std::ostringstream	oss;
	oss << "HTTP/1.1 200 OK\r\n"
		<< "Content-Length: " << _fileSize << "\r\n"
		<< "Content-Type: text/html\r\n"  // or detect MIME type
		<< "\r\n";
	std::string header = oss.str();

	int err = send(_clientFd, header.c_str(), header.size(), flags);
	if (err < 0)
	{
		THROW_ERRNO("send");
	}
	_state = ClientState::READING_FILE;
}

void	Client::closeFile(epoll_event &ev, int epollFd, int eventFd)
{
	close(_fileFd);
	_fileBuffer.clear();
	_fileSize = 0;
	_fileOffset = 0;
}

void	Client::openFile(epoll_event &ev, int epollFd, int eventFd)
{
	int			err;
	struct stat	fileInfo;

	_fileFd = open(_filePath.c_str(), O_RDWR, 667);
	if (_fileFd < 0)
		THROW_ERRNO("open");

	err = fstat(_fileFd, &fileInfo);
	if (err)
	{
		THROW_ERRNO("fstat");
	}
	_fileSize = fileInfo.st_size;
	_fileOffset = 0;


	_state = ClientState::SENDING_RESPONSE;
	auto it = _handlersMap.find(_clientFd);
	if (it == _handlersMap.end())
	{
		THROW_ERRNO("Unknown fd in map");
	}
	(*it).second = this;
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


	// auto it = _handlersMap.find(_clientFd);
	// if (it == _handlersMap.end())
	// {
	// 	THROW_ERRNO("Unknown fd in map");
	// }
	// (*it).second = this;
}

void	Client::readFile(epoll_event &ev, int epollFd, int eventFd)
{
	char	read_buf[IO_BUFFER_SIZE];
	int		readBytes;

	std::cout << "Reading file " << _filePath << std::endl;
	readBytes = read(_fileFd, read_buf, sizeof(read_buf) - 1);
	if (readBytes > 0)
	{
		read_buf[readBytes] = 0;
		_fileBuffer = read_buf;
		_fileOffset += readBytes;

		std::cout << "FILE BUFFER:\n" << _fileBuffer << std::endl;
		_state = ClientState::SENDING_FILE;
	}
	else if (readBytes == 0)
	{
		std::cout << "file size: " << _fileSize << ", readByes" << _fileOffset << std::endl;
		THROW_ERRNO("read");
	}
	else if (readBytes < 0)
	{
		THROW_ERRNO("read");
	}
}

void	Client::handleEpollEvent(epoll_event &ev, int epollFd, int eventFd)
{
	if (ev.events & EPOLLIN)
	{
		if (_state == ClientState::GETTING_FILE)
		{

		}
	}
	else if (ev.events & (EPOLLOUT | EPOLLHUP))
	{
		if (_state == ClientState::SENDING_RESPONSE)
		{
			sendResponse(ev, epollFd, eventFd);
		}

		if (_state == ClientState::SENDING_FILE)
		{
			sendFile(ev, epollFd, eventFd);
		}
		if (_state == ClientState::READING_FILE)
		{
			readFile(ev, epollFd, eventFd);
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

