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
#include <fstream>

#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define QUEUE_SIZE 20
#define IO_BUFFER_SIZE 1024
#define MAX_EVENTS 10
#define DEFAULT_EPOLL_SIZE 10

#define CLIENT_HEADER_LIMIT 1024

#define HTTP_STATUS "HTTP/1.1 200 OK\r\nContent-Length: 131\r\n\r\n"
#define CRLF "\r\n"
#define DOUBLE_CRLF "\r\n\r\n"

// default conf_file
#define PORT "8080"
#define HOST "0.0.0.0"
#define DEFAULT_CONF "web/default.conf"
#define STATIC_SITE "web/www/index.html"

struct IEpollFdOwner;
class Server;
class IpPort;
class Client;
struct HttRequest;
enum class HttpMethod;
struct HttpResponse;
struct Data;
enum class ClientState;
class Program;
class ClientHandler;

using ServerPtr  = std::shared_ptr<Server>;
using ServerDeq = std::deque<ServerPtr>;

using IpPortPtr = std::shared_ptr<IpPort>;

using ClientPtr = std::shared_ptr<Client>;
using ClientDeq = std::deque<ClientPtr>;

using FdClientMap = std::unordered_map<int, ClientPtr>;
using FdEpollOwnerMap = std::unordered_map<int, IEpollFdOwner*>;

#include "IEpollFdOwner.hpp"
#include "CustomException.hpp"
#include "Server.hpp"
#include "IpPort.hpp"
#include "Program.hpp"
#include "Client.hpp"

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

#include "ClientHanlder.hpp"
#include "ConfigParser.hpp"
