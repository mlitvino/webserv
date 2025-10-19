#pragma once

#include "webserv.hpp"
#include "IEpollFdOwner.hpp"
#include "utils.hpp"

enum class ClientState
{
	READING_REQUEST,
	SENDING_RESPONSE,
	GETTING_BODY,

	GETTING_FILE,
	CGI_READING_OUTPUT,
};

enum class FileType
{
	REGULAR,
	DIRECTORY,
	CGI_SCRIPT,
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

		FileType			_fileType;
		std::string			_redirectedUrl;
		int					_redirectCode;

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

		PostRequestHandler	_postHandler;


		Client(sockaddr_storage clientAddr, socklen_t	clientAddrLen, int	clientFd, IpPort &owner);
		~Client();

		int		getFd();
		void	handleEpollEvent(epoll_event &ev, int epollFd, int eventFd);

		void	sendResponse();
		bool	readRequest();

		void	closeFile(epoll_event &ev, int epollFd, int eventFd);
		void	openFile(std::string &filePath);
};
