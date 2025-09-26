#include "ClientHanlder.hpp"

void	ClientHandler::acceptConnect(int srvSockFd, int epoll_fd)
{
	int	err;
	epoll_event ev;

	_sockFd = accept(srvSockFd, (sockaddr *)&_clientAddr, &_clientAddrLen);
	if (_sockFd == -1)
		THROW_ERRNO("accept");

	int flags = fcntl(_sockFd, F_GETFL, 0);
	fcntl(_sockFd, F_SETFL, flags | O_NONBLOCK);
	ev.events = EPOLLIN | EPOLLOUT;
	ev.data.ptr = static_cast<void*>(this);
	err = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, _sockFd, &ev);
	if (err)
		THROW_ERRNO("epoll_ctl");
	_state = READING_REQUEST;
}

void	ClientHandler::readAll()
{

}

void	ClientHandler::handleEpollEvent(epoll_event &ev, int epoll_fd)
{
	switch (_state)
	{
		case READING_REQUEST:
		{
			if (ev.events == EPOLLIN)
			{

			}
			break;
		}
		case WRITING_RESPONSE:
		{
			if (ev.events == EPOLLOUT)
			{

			}
			break;
		}
		case READING_FILE:
		{
			if (ev.events == EPOLLIN)
			{

			}
			break;
		}
		case WRITING_FILE:
		{
			if (ev.events == EPOLLOUT)
			{

			}
			break;
		}
		default:
		{
			THROW("unknown ClientHandler state");
			break;
		}
	}

	// std::cout << "Accepting new data from " << std::endl;

	// char	tmp[IO_BUFFER_SIZE];

	// int len = read(_sockFd, tmp, sizeof(tmp));

	// if (len > 0)
	// 	_buffer.append(tmp, len);
	// else if (len == 0)
	// 	return CloseConnection(epoll_fd);
	// else
	// 	THROW_ERRNO("read");

	// std::cout << "ClientHandler REQUEST:\n" << _buffer << std::endl;

	// if (_buffer.find("ex\r\n") != std::string::npos)
	// 	return CloseConnection(epoll_fd);
	// if (_buffer.find("ec\r\n") == std::string::npos)
	// 	return ;

	// _buffer = HTTP_STATUS;

	// std::fstream infile(STATIC_SITE);

	// if (infile.fail())
	// 	THROW_ERRNO("std::fstream failed to open file");

	// infile.readsome(&_buffer[_buffer.size()], _buffer.capacity() - _buffer.size());

	// infile.close();

	// std::cout << "SERVER RESPONSE:\n" << _buffer << std::endl;

	// len = send(_sockFd, &_buffer[0], _buffer.size(), 0);
	// if (len == -1)
	// 	THROW_ERRNO("send");
}
void	ClientHandler::CloseConnection(int epoll_fd)
{
	if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, _sockFd, 0))
		THROW("epoll_ctl(DEL)");
	close(_sockFd);

	_owner.RemoveClientHandler(*this, _index);

	std::cout << "CLOSED" << std::endl;
}

void	ClientHandler::setIndex(size_t index)
{
	_index = index;
}

ClientHandler::ClientHandler(Server& owner)
	: _clientAddrLen{sizeof(_clientAddr)}
	, _owner{owner}
	, _index{owner.getSizeClients()}
{
	_buffer.reserve(IO_BUFFER_SIZE);
}

// ClientHandler::ClientHandler()
// 	: _clientAddrLen{sizeof(_clientAddr)}
// 	, _sockFd{-1}
// {

// }

ClientHandler::~ClientHandler()
{
	if (_sockFd != -1)
		close(_sockFd);
}
