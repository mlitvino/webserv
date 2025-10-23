#pragma once

#include <chrono>
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
#include "Program.hpp"

extern Time g_current_time;

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
		int					_clientFd;
		sockaddr_storage	_clientAddr;
		socklen_t			_clientAddrLen;
	public:
		std::string			_cgiBuffer;
		size_t				_cgiHeaderEndPos;

		Time				_lastActivity;
		std::string			_buffer;

		std::string			_responseBuffer;
		size_t				_responseOffset;

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

		int					_fileFd;
		int					_fileSize;
		int					_fileOffset;

		Cgi					_cgi;
		PostRequestHandler	_postHandler;

		Client(sockaddr_storage clientAddr, socklen_t clientAddrLen, int clientFd, IpPort &owner);
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

		Time			getLastActivity();
		void			setLastActivity(Time t);

		std::string&	getBuffer();
		void			setBuffer(const std::string &v);

		std::string&	getResponseBuffer();
		void			setResponseBuffer(const std::string &v);

		size_t			getResponseOffset();
		void			setResponseOffset(size_t v);

		ClientState		getState();
		void			setState(ClientState s);

		FdClientMap&		getClientsMap();
		FdEpollOwnerMap&	getHandlersMap();
		IpPort&			getIpPort();

		ServerPtr&		getOwnerServer();
		void			setOwnerServer(const ServerPtr &srv);

		std::string&	getHttpMethod();
		void			setHttpMethod(const std::string &v);

		std::string&	getHttpPath();
		void			setHttpPath(const std::string &v);

		std::string&	getQuery();
		void			setQuery(const std::string &v);

		std::string&	getHttpVersion();
		void			setHttpVersion(const std::string &v);

		size_t			getContentLen();
		void			setContentLen(size_t v);

		bool			getChunked();
		void			setChunked(bool v);

		bool			getKeepAlive();
		void			setKeepAlive(bool v);

		std::string&	getHostHeader();
		void			setHostHeader(const std::string &v);

		std::string&	getContentType();
		void			setContentType(const std::string &v);

		std::string&	getMultipartBoundary();
		void			setMultipartBoundary(const std::string &v);

		std::string&	getResolvedPath();
		void			setResolvedPath(const std::string &v);

		FileType		getFileType();
		void			setFileType(FileType t);

		std::string&	getRedirectedUrl();
		void			setRedirectedUrl(const std::string &v);

		int				getRedirectCode();
		void			setRedirectCode(int v);

		int				getClientFd();
		void			setClientFd(int fd);

		int				getFileFd();
		void			setFileFd(int fd);

		int				getFileSize();
		void			setFileSize(int sz);

		Cgi&			getCgi();
		PostRequestHandler&	getPostRequestHandler();
};
