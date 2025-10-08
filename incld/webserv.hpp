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
#include <algorithm>

#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <sys/wait.h>

#define QUEUE_SIZE 20
#define IO_BUFFER_SIZE 1024
#define MAX_EVENTS 10
#define DEFAULT_EPOLL_SIZE 10
#define CLIENT_HEADER_LIMIT 1024

#define DEFAULT_CONF "conf/default.conf"

class		Program;
class		Server;
struct		IEpollFdOwner;
class		IpPort;
class		Client;
enum class	HttpMethod;
enum class	ClientState;

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
#include "utils.hpp"

enum class HttpMethod {
	GET = 1,
	POST = 2,
	DELETE = 4,
};

#include "ConfigParser.hpp"
