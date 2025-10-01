#pragma once

#include <iostream>
#include <cstring>
#include <fstream>
#include <vector>
#include <memory>
#include <deque>

#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#define QUEUE_SIZE 20
#define IO_BUFFER_SIZE 1024
#define CLIENT_HEADER_LIMIT 1024
#define MAX_EVENTS 10
#define DEFAULT_EPOLL_SIZE 10

#define HTTP_STATUS "HTTP/1.1 200\r\n\r\n"
#define CRLF "\r\n"
#define DOUBLE_CRLF "\r\n\r\n"

// default conf_file
#define PORT "8080"
#define HOST "0.0.0.0"
#define DEFAULT_CONF "web/default_conf"
#define STATIC_SITE "web/www/index.html"

class Server;
class ClientHandler;
class IpPort;
class Client;

using ServerPtr  = std::shared_ptr<Server>;
using ServerDeq = std::deque<ServerPtr>;

using ClientHandlerPtr = std::shared_ptr<ClientHandler>;
using ClientHandlerDeq = std::deque<ClientHandlerPtr>;

using IpPortPtr = std::unique_ptr<IpPort>;

using ClientPtr = std::shared_ptr<Client>;
using ClientDeq = std::deque<ClientPtr>;

#include "CustomException.hpp"
#include "Server.hpp"
#include "IEpollFdOwner.hpp"
#include "ClientHanlder.hpp"

#include "Program.hpp"
#include "Client.hpp"

typedef struct	s_request
{
	std::string	method;
	char	*path;
	char	*proocol;

	std::string	body;
}				t_request;

typedef struct	s_response
{
	char	*protocol;
	int		status_code;
	char	*optional_phrase;

	char	*content_length;
	char	*body;
}				t_response;

typedef struct	s_server
{
	// conf
	char	*host_address;
	int		port;

	char	*server_name;
	size_t	client_body_size;
	char	*error_pages_path[10];

	char	*accepted_HTTP_methods;
	char	*HTTP_redirection;
	char	*root_directory;

	bool	directory_listing;
	char	*default_direcory_file;

	// runtime
	char	*clients;


}		t_server;

typedef struct	s_data
{
		ServerDeq	servers;

		epoll_event	ev;
		epoll_event	events[MAX_EVENTS];
		int			epoll_fd;

}		Data;

void	parser(Data &data, char *conf_file);
void	init_servers(Data &data);
void	accepting_loop(Data &data);
void	init_epoll(Data &data);
