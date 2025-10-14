#pragma once

#include "webserv.hpp"

enum class BodyReadStatus {
	NEED_MORE,
	COMPLETE,
	ERROR
};

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
		void			handlePostRequest(ClientPtr &client, const std::string& path);
		void			handleDeleteRequest(ClientPtr &client);
		void			generateResponse(ClientPtr &client, std::string path, int statusCode);

		BodyReadStatus	getContentLengthBody(ClientPtr &client);
		BodyReadStatus	getChunkedBody(ClientPtr &client);
		bool			getMultiPart(ClientPtr &client);
	
		bool			extractFilename(ClientPtr &client, std::string &dashBoundary);
		std::string		composeUploadPath(ClientPtr &client);
		void			writeBodyPart(ClientPtr &client, std::string &uploadPath, size_t tailSize);
		void			getLastBoundary(ClientPtr &client, std::string &boundaryMarker);

		void			processCgi(ClientPtr &client);

		void			setAddrPort(std::string addrPort);
		int				getSockFd();

		~IpPort();
		IpPort(Program &program);


};
