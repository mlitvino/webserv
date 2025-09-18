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
	ev.events = EPOLLIN | EPOLLET;
	ev.data.ptr = static_cast<void*>(this);
	err = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, _sockFd, &ev);
	if (err)
		THROW_ERRNO("epoll_ctl");
}

void	ClientHandler::handleEpollEvent(epoll_event &ev, int epoll_fd)
{
	std::cout << "Accepting new data from " << std::endl;

	char	buffer[1024] = {0};
	int		err;

	err = read(_sockFd, buffer, sizeof(buffer));
	if (err == -1)
		THROW_ERRNO("read");

	std::cout << "ClientHandler REQUEST:\n" << buffer << std::endl;

	bzero(buffer, sizeof(buffer));
	strcpy(buffer, HTTP_STATUS);

	int html_fd = open(STATIC_SITE, O_RDWR);
	if (html_fd == -1)
		THROW_ERRNO("open");

	int len = read(html_fd, &buffer[sizeof(HTTP_STATUS) - 1], sizeof(buffer) / 2);
	if (len == -1)
		THROW_ERRNO("read");

	close(html_fd);

	std::cout << "SERVER RESPONSE:\n" << buffer << std::endl;

	err = send(_sockFd, buffer, sizeof(buffer), 0);
	if (err == -1)
		THROW_ERRNO("send");

	close(_sockFd);
}

ClientHandler::ClientHandler()
	: _clientAddrLen{sizeof(_clientAddr)}
	, _sockFd{-1}
{
	
}

ClientHandler::~ClientHandler()
{
	if (_sockFd != -1)
		close(_sockFd);
}
