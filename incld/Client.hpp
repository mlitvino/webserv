#pragma once

#include "webserv.hpp"

enum class ClientState {
	READING_REQUEST,
	SENDING_RESPONSE,
	GETTING_BODY,

	GETTING_FILE,
	CGI_READING_OUTPUT,
};

class Client : public IEpollFdOwner
{
	private:

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
		size_t				_contentLen;
		bool				_chunked;
		bool				_keepAlive;
		std::string			_hostHeader;

		std::string			_contentType;
		std::string			_multipartBoundary;

		std::string			_resolvedPath;

		std::string			_uploadFilename;
		// Request body tracking
		std::string			_bodyBuffer;
		size_t				_bodyBytesExpected;
		size_t				_bodyBytesReceived;
		bool				_bodyProcessingInitialized;
		size_t				_currentChunkSize;
		size_t				_currentChunkRead;
		bool				_readingChunkSize;
		bool				_parsingChunkTrailers;
		bool				_chunkedFinished;

		sockaddr_storage	_clientAddr;
		socklen_t			_clientAddrLen;
		int					_clientFd;

		int					_fileFd;
		std::string			_filePath;
		std::string			_fileBuffer;
		int					_fileSize;
		int					_fileOffset;

		int					_cgiInFd;
		int					_cgiOutFd;
		pid_t				_cgiPid;
		std::string			_cgiBuffer;
		bool				_cgiHeadersParsed;


		Client(sockaddr_storage clientAddr, socklen_t	clientAddrLen, int	clientFd, IpPort &owner);
		~Client();

		int		getFd();
		void	handleEpollEvent(epoll_event &ev, int epollFd, int eventFd);

		void	sendResponse();
		bool	readRequest();

		void	closeFile(epoll_event &ev, int epollFd, int eventFd);
		void	openFile(std::string &filePath);
		void	resetBodyTracking();
};
