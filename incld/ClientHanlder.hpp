#pragma once

#include <cstring>

#include "webserv.hpp"
#include "IEpollFdOwner.hpp"

typedef enum
{
	READING_CLIENT_HEADER,
	READING_CLIENT_BODY,
	WRITING_RESPONSE,
	READING_FILE,
	WRITING_FILE,

}	t_state;

class ClientHandler : public IEpollFdOwner
{
	private:
		sockaddr_storage	_clientAddr;
		socklen_t			_clientAddrLen;
		Server				&_owner;
		size_t				_index;

		int					_sockFd;
		int					_fileFd;

		int					_state;

		std::string			_buffer;
		std::string			_headRequest;
		std::string			_body;
	public:
		ClientHandler() = delete;
		~ClientHandler();
		ClientHandler(Server& owner);

		void		setIndex(size_t index);
		void		CloseConnection(int epoll_fd);

		void	acceptConnect(int srvSockFd, int epoll_fd);
		void	readRequest(int epoll_fd);

		void	handleEpollEvent(epoll_event &ev, int epoll_fd);
};


