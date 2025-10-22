#pragma once

#include <sstream>
#include <cctype>

#include <sys/stat.h>
#include <sys/sendfile.h>

#include "webserv.hpp"
#include "IEpollFdOwner.hpp"
#include "utils.hpp"
#include "Cgi.hpp"
#include "Server.hpp"
#include "PostRequestHandler.hpp"

enum class ClientState
{
	READING_REQUEST,
	SENDING_RESPONSE,
	GETTING_BODY,

	GETTING_FILE,
	WRITING_CGI_INPUT,
	READING_CGI_OUTPUT,
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
		std::string			_cgiBuffer;
		size_t				_cgiHeaderEndPos;

		ClientState			_state;
		FdClientMap			&_clientsMap;
		FdEpollOwnerMap		&_handlersMap;
		IpPort				&_ipPort;
		ServerPtr			_ownerServer;

		std::string			_httpMethod;
		std::string			_httpPath;
		std::string			_query;
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

		Cgi					_cgi;
		PostRequestHandler	_postHandler;

		Client(sockaddr_storage clientAddr, socklen_t	clientAddrLen, int	clientFd, IpPort &owner);
		~Client();

		int		getFd();
		void	handleEpollEvent(epoll_event &ev, int epollFd, int eventFd);

		void	sendResponse();
		bool	readRequest();

		void	closeFile();
		void	openFile(std::string &filePath);

		void	handleCgiStdoutEvent(epoll_event &ev);
		void	handleCgiStdinEvent(epoll_event &ev);
		bool	parseCgiOutput();
		void	resetRequestData();
};
