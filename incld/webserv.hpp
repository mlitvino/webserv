#pragma once

#include <iostream>
#include <cstring>
#include <fstream>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#include "CustomException.hpp"

#define QUEUE_SIZE 20

#define HTTP_STATUS "HTTP/1.1 200\r\n\r\n"
#define CRLF "\r\n"

// default conf_file
#define IN_PORT "8080"
#define IN_DOMAIN "localhost"
#define DEFAULT_CONF "web/default_conf"
#define STATIC_SITE "web/www/index.html"

typedef struct	s_request
{
	char	*method;
	char	*path;
	char	*proocol;

	char	*body;
}				t_request;

typedef struct	s_response
{
	char	*protocol;
	int		status_code;
	char	*optional_phrase;

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
		t_server	*server_array;
		int			server_amount;

}		t_data;

void	parser(char *conf_file);
void	init_servers(addrinfo *&server, int &sockfd);
void	accepting_loop(int &sockfd);
