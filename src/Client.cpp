#include "Client.hpp"

void	Client::sendResponse(epoll_event &ev, int epollFd, int eventFd)
{
	int flags = MSG_NOSIGNAL;

	int err = send(_clientFd, _fileBuffer.c_str(), _fileBuffer.size(), flags);
	if (err)
	{
		THROW_ERRNO("send");
	}

	if (_fileSize == _readFileBytes)
	{
		_state = clientState::READING_CLIENT_HEADER;
		auto it = _handlersMap.find(_clientFd);
		if (it == _handlersMap.end())
		{
			THROW_ERRNO("Unknown fd in map");
		}
		(*it).second = &_ipPort;
	}
}

void	Client::closeFileDelEpoll(epoll_event &ev, int epollFd, int eventFd)
{
	epoll_ctl(epollFd, EPOLL_CTL_DEL, _fileFd, 0);
	_handlersMap.erase(_fileFd);


	close(_fileFd);
	_fileBuffer.clear();
}

void	Client::openFileAddEpoll(epoll_event &ev, int epollFd, int eventFd)
{
	int			err;
	epoll_event	newEv;
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
	_readFileBytes = 0;

	newEv.data.fd = _fileFd;
	newEv.events = EPOLLIN | EPOLLOUT;

	_handlersMap.emplace(_fileFd, this);
	newEv.events = EPOLLIN | EPOLLOUT;
	newEv.data.fd = _fileFd;
	err = epoll_ctl(epollFd, EPOLL_CTL_ADD, _fileFd, &newEv);
	if (err)
		THROW_ERRNO("epoll_ctl");
	_state = clientState::READING_FILE;
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
		_fileBuffer += read_buf;
		_readFileBytes += readBytes;

		std::cout << "FILE BUFFER:\n" << _fileBuffer << std::endl;
		_state = clientState::SENDING_RESPONSE;
	}
	else if (readBytes == 0)
	{
		THROW_ERRNO("read");
	}
	else if (readBytes < 0)
	{
		THROW_ERRNO("read");
	}

	if (_fileSize == _readFileBytes)
	{
		closeFileDelEpoll(ev, epollFd, eventFd);
		_state = clientState::SENDING_RESPONSE;
	}
}

void	Client::handleEpollEvent(epoll_event &ev, int epollFd, int eventFd)
{
	if (ev.events == EPOLLIN)
	{
		if (_state == clientState::READING_FILE)
		{
			if (eventFd == _fileFd)
			{
				readFile(ev, epollFd, eventFd);
			}
		}
	}
	else
	{
		if (_state == clientState::SENDING_RESPONSE)
		{
			if (eventFd == _clientFd)
			{
				sendResponse(ev, epollFd, eventFd);
			}
		}
	}
}

void	Client::setEpollInfo(IEpollInfo *epollInfo)
{
	_epollInfo = epollInfo;
}

// Getters + Setters

int		Client::getFd()
{
	return _clientFd;
}

// Constructors + Destructor

Client::Client(sockaddr_storage clientAddr, socklen_t	clientAddrLen, int	clientFd, IpPort &owner)
	: _buffer()
	, _state(READING_CLIENT_HEADER)
	, _clientsMap(owner._clientsMap)
	, _handlersMap(owner._handlersMap)
	, _ipPort(owner)
	, _clientAddr{clientAddr}
	, _clientAddrLen{clientAddrLen}
	, _clientFd{clientFd}
	, _fileFd{-1}
	, _filePath{STATIC_SITE}
	, _fileBuffer()
	, _fileSize{0}
	, _readFileBytes{0}
	, _epollInfo{nullptr}
{}

Client::~Client()
{
	if (_clientFd != -1)
		close(_clientFd);
	if (_fileFd != -1)
		close(_fileFd);
}

