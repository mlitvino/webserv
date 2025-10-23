#pragma once

#include <iostream>

#include <netdb.h>

#include "webserv.hpp"
#include "Program.hpp"
#include "HttpException.hpp"
#include "IEpollFdOwner.hpp"
#include "utils.hpp"
#include "Client.hpp"

#define QUEUE_SIZE 20

class IpPort : public IEpollFdOwner
{
	private:
		FdClientMap		&_clientsMap;
		FdEpollOwnerMap	&_handlersMap;

		ServerDeq		_servers;
		std::string		_addrPort;

		int				_sockFd;
		int				&_epollFd;

		void		parseRequest(epoll_event &ev, int epollFd, int eventFd);
		void		parseHeaders(ClientPtr &client);
		void		parseQuery(ClientPtr &client, const std::string &pathAndQuery);
		void		assignServerToClient(ClientPtr &client);

		void		handleGetRequest(ClientPtr &client);
		void		handleDeleteRequest(ClientPtr &client);
		bool		listDirectory(ClientPtr &client, std::string &listingBuffer);
		std::string	formHeaders(ClientPtr &client, std::string &filePath, size_t contentLength, int statusCode);
	public:
		~IpPort();
		IpPort(Program &program);

		void			OpenSocket(addrinfo &hints, addrinfo *_servInfo);
		void			handleEpollEvent(epoll_event &ev, int epoll_fd, int eventFd);
		void			acceptConnection(epoll_event &ev, int epollFd, int eventFd);
		void			closeConnection(int &clientFd);
		std::string		getStatusText(int statusCode);
		void			generateResponse(ClientPtr &client, std::string path, int statusCode);

		int					getSockFd();
		FdClientMap&		getClientsMap();
		FdEpollOwnerMap&	getHandlersMap();
		ServerDeq&			getServers();
		const std::string&	getAddrPort();
		int&				getEpollFd();

		void				setSockFd(int fd);
		void				setAddrPort(const std::string& addrPort);
};
