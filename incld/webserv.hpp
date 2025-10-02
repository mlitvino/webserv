#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <deque>
#include <memory>
#include <map>
#include <unordered_map>
#include <sstream>

#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#define QUEUE_SIZE 20
#define IO_BUFFER_SIZE 1024
#define MAX_EVENTS 10
#define DEFAULT_EPOLL_SIZE 10

#define HTTP_STATUS "HTTP/1.1 200\r\n\r\n"
#define CRLF "\r\n"
#define DBLE_CRLF "\r\n\r\n"

// default conf_file
#define PORT "8080"
#define HOST "localhost"
#define DEFAULT_CONF "web/default.conf"
#define STATIC_SITE "web/www/index.html"

class Server;
class ClientHandler;

using ServerPtr = std::unique_ptr<Server>;
using ServerContainer = std::vector<ServerPtr>;
using ClientHandlerPtr = std::unique_ptr<ClientHandler>;
using ClientHandlerContainer = std::vector<ClientHandlerPtr>;

#include "CustomException.hpp"
#include "Server.hpp"
#include "IEpollFdOwner.hpp"
#include "ClientHanlder.hpp"

enum class HttpMethod {
	GET = 1,
	POST = 2,
	DELETE = 4,
	PUT = 8,
	HEAD = 16
};

struct HttpRequest {
	HttpMethod method;
	std::string path;
	std::string protocol;
	std::unordered_map<std::string, std::string> headers;
	std::string body;
};

struct HttpResponse {
	std::string protocol = "HTTP/1.1";
	int statusCode = 200;
	std::string statusText = "OK";
	std::unordered_map<std::string, std::string> headers;
	std::string body;
};

struct Data {
	ServerContainer servers;
	epoll_event ev;
	epoll_event events[MAX_EVENTS];
	int epollFd;
	
	Data() : epollFd(-1) {}
	~Data() {
		if (epollFd != -1) {
			close(epollFd);
		}
	}
};

#include "ConfigParser.hpp"

void parser(Data& data, char* confFile);
void initServers(Data& data);
void acceptingLoop(Data& data);
void initEpoll(Data& data);
