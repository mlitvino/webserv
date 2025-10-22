#pragma once

#include <deque>
#include <memory>
#include <unordered_map>

#define IO_BUFFER_SIZE 1024
#define CONTENT_TYPE_MULTIPART "multipart/form-data"
#define CONTENT_TYPE_APP_FORM "application/x-www-form-urlencoded"

class		Program;
class		PostRequestHandler;
class		Server;
struct		IEpollFdOwner;
class		IpPort;
class		Client;
enum class	HttpMethod;
enum class	ClientState;
class		Cgi;
class		ConfigParser;
struct		ServerConfig;
struct		Location;

using		IpPortPtr = std::shared_ptr<IpPort>;

using		ServerPtr  = std::shared_ptr<Server>;
using		ServerDeq = std::deque<ServerPtr>;

using		ClientPtr = std::shared_ptr<Client>;
using		ClientDeq = std::deque<ClientPtr>;

using		FdClientMap = std::unordered_map<int, ClientPtr>;
using		FdEpollOwnerMap = std::unordered_map<int, IEpollFdOwner*>;
