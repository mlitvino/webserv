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

#define IN_PORT "8080"
#define IN_DOMAIN "localhost"
#define QUEUE_SIZE 20

#define HTTP_STATUS "HTTP/1.1 200 OK\r\n\r\n"
#define CRLF "\r\n"


#define DEFAULT_CONF "web/default_conf"
#define STATIC_SITE "web/index.html"

struct	s_server
{
		char	*host_address;
		int		port;

		char	*server_name;
		size_t	client_body_size;
		char	*error_pages_path[10];

		// routes
		char	*accepted_HTTP_methods;
		char	*HTTP_redirection;
		char	*root_directory;

		bool	directory_listing;
		char	*default_direcory_file;
}		t_server;
