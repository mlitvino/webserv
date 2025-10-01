#include "Client.hpp"

void	Client::handleEpollEvent(IEpollInfo *epollInfo, int epollFd, epoll_event event)
{

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

Client::Client(sockaddr_storage clientAddr, socklen_t clientAddrLen, int clientFd)
	: _clientAddr{clientAddr}
	, _clientAddrLen{clientAddrLen}
	, _clientFd{clientFd}
	, _epollInfo{nullptr}
{}

Client::~Client()
{
	delete _epollInfo;
	if (_clientFd != -1)
		close(_clientFd);
}

