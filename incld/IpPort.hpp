#pragma once

#include "webserv.hpp"

class IpPort : public IEpollFdOwner
{
	public:
		FdClientMap		&_clientsMap;
		FdEpollOwnerMap	&_handlersMap;

		ServerDeq		_servers;


		std::string		_addrPort;
		//std::string		_buffer;
		ClientDeq		_unsortedClients;

		int				_sockFd;

		void			OpenSocket(addrinfo &hints, addrinfo *_servInfo);

		void			handleEpollEvent(epoll_event &ev, int epoll_fd, int eventFd);

		void			acceptConnection(epoll_event &ev, int epollFd, int eventFd);
		void			closeConnection(epoll_event &ev, int epollFd, int eventFd);
		void			closeConnection(int epollFd, int clientFd);

		bool			readRequest(ClientPtr &client, int clientFd);
		void			parseRequest(epoll_event &ev, int epollFd, int eventFd);
		std::string		parseRequestLine(ClientPtr &client, std::string& line);
		void			assignServerToClient(ClientPtr &client);
		bool			isMethodAllowed(ClientPtr &client, std::string& path);
		bool			isBodySizeValid(ClientPtr &client);

		void			setAddrPort(std::string addrPort);
		int				getSockFd();

		~IpPort();
		IpPort(FdClientMap	&clientsMap, FdEpollOwnerMap &handlersMap);
};
