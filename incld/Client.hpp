#pragma once

#include "webserv.hpp"

enum clientState
{
	READING_CLIENT_HEADER,
	READING_CLIENT_BODY,
	WRITING_RESPONSE,
	READING_FILE,
	WRITING_FILE,

	SENDING_RESPONSE,
	SENDING_FILE,
	GETTING_FILE,
};

class Client : public IEpollFdOwner
{
	public:
		std::string			_buffer;

		std::string			_responseBuffer;
		size_t				_responseOffset;

		ClientState			_state;
		FdClientMap			&_clientsMap;
		FdEpollOwnerMap		&_handlersMap;
		IpPort				&_ipPort;
		ServerPtr			_ownerServer;

		// HTTP request data
		std::string			_httpMethod;
		std::string			_httpPath;
		std::string			_httpVersion;


		sockaddr_storage	_clientAddr;
		socklen_t			_clientAddrLen;
		int					_clientFd;

		int					_fileFd;
		std::string			_filePath;
		std::string			_fileBuffer;
		int					_fileSize;
		int					_fileOffset;
	private:






		// response?
		// request?
		// buffer?
	public:
		Client(sockaddr_storage clientAddr, socklen_t	clientAddrLen, int	clientFd, IpPort &owner);
		~Client();

		int		getFd();
		void	handleEpollEvent(epoll_event &ev, int epollFd, int eventFd);

		void	sendResponse(epoll_event &ev, int epollFd, int eventFd);
		void	sendFile(epoll_event &ev, int epollFd, int eventFd);

		void	closeFile(epoll_event &ev, int epollFd, int eventFd);
		void	readFile(epoll_event &ev, int epollFd, int eventFd);
		void	openFile(epoll_event &ev, int epollFd, int eventFd);
		void	openFile(std::string &filePath);
};
