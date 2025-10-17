#pragma once

#include "webserv.hpp"
#include "IEpollFdOwner.hpp"
#include "utils.hpp"

class IpPort : public IEpollFdOwner
{
	public:
		FdClientMap		&_clientsMap;
		FdEpollOwnerMap	&_handlersMap;

		ServerDeq		_servers;
		std::string		_addrPort;

		int				_sockFd;
		int				&_epollFd;

		void			OpenSocket(addrinfo &hints, addrinfo *_servInfo);

		void			handleEpollEvent(epoll_event &ev, int epoll_fd, int eventFd);

		void			acceptConnection(epoll_event &ev, int epollFd, int eventFd);
		void			closeConnection(int clientFd);

		void			parseRequest(epoll_event &ev, int epollFd, int eventFd);
		void			parseHeaders(ClientPtr &client);
		void			assignServerToClient(ClientPtr &client);

		void			handleGetRequest(ClientPtr &client);
		void			handleDeleteRequest(ClientPtr &client);
		void			generateResponse(ClientPtr &client, std::string path, int statusCode);
		void			listDirectory(ClientPtr &client, std::string &listingBuffer);
		std::string		formHeaders(ClientPtr &client, std::string &filePath, size_t contentLength);

		void			processCgi(ClientPtr &client);

		void			setAddrPort(std::string addrPort);
		int				getSockFd();

		~IpPort();
		IpPort(Program &program);

};
