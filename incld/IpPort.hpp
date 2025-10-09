#pragma once

#include "webserv.hpp"

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

		bool			readRequest(ClientPtr &client, int clientFd);
		void			parseRequest(epoll_event &ev, int epollFd, int eventFd);
		void			parseHeaders(ClientPtr &client);
		void			assignServerToClient(ClientPtr &client);

		void			handleGetRequest(ClientPtr &client, const std::string& path);
		void			handlePostRequest(ClientPtr &client, const std::string& path);
		void			handleDeleteRequest(ClientPtr &client, const std::string& path);
		void			generateResponse(ClientPtr &client, std::string path, int statusCode);
		std::string		getMimeType(const std::string& filePath);
		std::string		getCustomErrorPage(ServerPtr& server, int statusCode);

		void			processCgi(ClientPtr &client);

		void			sendResponse(ClientPtr &client, int clientFd);

		void			setAddrPort(std::string addrPort);
		int				getSockFd();

		~IpPort();
		IpPort(Program &program);
};
