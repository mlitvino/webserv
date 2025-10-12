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

		void			handleGetRequest(ClientPtr &client, const std::string& path);
		void			handlePostRequest(ClientPtr &client, const std::string& path);
		void			handleDeleteRequest(ClientPtr &client, const std::string& path);
		void			generateResponse(ClientPtr &client, std::string path, int statusCode);
		std::string		getMimeType(const std::string& filePath);

		BodyReadStatus	getContentLengthBody(Client &client, int &errorStatus);
		BodyReadStatus	getChunkedBody(Client &client, int &errorStatus);
		bool			drainMultipartFirstPartToFile(Client &client,
											const std::string &boundary,
											const std::string &uploadDir,
											bool &finished,
											int &errorStatus);
		bool			parseMultipartFirstPart(const std::string &body,
										const std::string &boundary,
										std::string &outFilename,
										std::string &outData);
		static bool		ensureDirExists(const std::string &dir);

		void			processCgi(ClientPtr &client);

		void			setAddrPort(std::string addrPort);
		int				getSockFd();

		~IpPort();
		IpPort(Program &program);
};
