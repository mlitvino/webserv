# Listen Directive and IP Port Management Documentation

## Overview

The webserv HTTP server now supports advanced listen directive parsing that automatically creates IP Port objects during configuration parsing. This enhancement allows servers to listen on multiple addresses and ports, and supports virtual hosting where multiple servers can share the same listening socket.

## Key Features

- **Multiple Listen Directives**: Servers can have multiple `listen` directives
- **Automatic IP Port Creation**: IP Ports are created during parsing, not manually
- **Virtual Hosting Support**: Multiple servers can share the same address:port combination
- **Flexible Address Formats**: Supports both port-only and address:port formats
- **Backward Compatibility**: Existing configurations continue to work unchanged

## Configuration Syntax

### Basic Listen Directive Formats

```nginx
# Port only (defaults to localhost)
listen 8080;

# Address and port
listen 127.0.0.1:8080;
listen 192.168.1.100:8080;
listen 0.0.0.0:8080;

# IPv6 (if supported)
listen [::1]:8080;
```

### Multiple Listen Directives

A single server can have multiple listen directives:

```nginx
server {
	listen 8080;
	listen 8081;
	listen 127.0.0.1:8082;
	server_name multi-port-server;
	
	location / {
		root /web/www;
		index index.html;
	}
}
```

This configuration creates **3 separate IP Port objects**:
- `localhost:8080`
- `localhost:8081` 
- `127.0.0.1:8082`

The same server will be available on all three addresses.

### Virtual Hosting (Shared Ports)

Multiple servers can share the same listening address and port:

```nginx
server {
	listen 8080;
	server_name site1.example.com;
	
	location / {
		root /web/site1;
		index index.html;
	}
}

server {
	listen 8080;
	server_name site2.example.com;
	
	location / {
		root /web/site2;
		index index.html;
	}
}
```

This configuration creates **1 IP Port object** (`localhost:8080`) with **2 servers** assigned to it.

## Implementation Details

### Data Structures

#### ListenConfig Structure
```cpp
struct ListenConfig {
	std::string host = "localhost";
	int port = 8080;
	std::string getAddressPort() const { return host + ":" + std::to_string(port); }
};
```

#### Enhanced ServerConfig Structure
```cpp
struct ServerConfig {
	std::vector<ListenConfig> listens;  // Multiple listen directives
	std::string serverName;
	size_t clientMaxBodySize = 1000000;
	std::map<int, std::string> errorPages;
	std::vector<Location> locations;
	
	// Backward compatibility methods
	std::string getHost() const { return listens.empty() ? "localhost" : listens[0].host; }
	int getPort() const { return listens.empty() ? 8080 : listens[0].port; }
};
```

### Parsing Process

1. **Configuration Parsing**: Each `listen` directive creates a `ListenConfig` object
2. **Server Creation**: Servers are created from `ServerConfig` objects
3. **IP Port Management**: 
- Unique address:port combinations create new IP Port objects
- Servers are assigned to appropriate IP Port objects
- Multiple servers can share the same IP Port
4. **Handler Setup**: Each server gets references to the global handler maps

### IP Port Creation Algorithm

```cpp
void ConfigParser::createServersAndIpPortsFromConfig(Program &program) {
	std::map<std::string, IpPortPtr> ipPortMap;
	
	for (const auto& config : _serverConfigs) {
		auto server = std::make_shared<Server>(config);
		
		// Handle default listen if none specified
		std::vector<ListenConfig> listens = config.listens;
		if (listens.empty()) {
			ListenConfig defaultListen;
			listens.push_back(defaultListen);
		}
		
		// Assign server to IP Ports
		for (const auto& listen : listens) {
			std::string addrPort = listen.getAddressPort();
			
			// Create IP Port if it doesn't exist
			if (ipPortMap.find(addrPort) == ipPortMap.end()) {
				auto ipPort = std::make_shared<IpPort>(program);
				ipPort->setAddrPort(addrPort);
				ipPortMap[addrPort] = ipPort;
				program._addrPortVec.push_back(ipPort);
			}
			
			// Add server to this IP Port
			ipPortMap[addrPort]->_servers.push_back(server);
		}
		
		program._servers.push_back(server);
	}
}
```

## Configuration Examples

### Example 1: Multi-Port Server

```nginx
server {
	listen 8080;
	listen 8081;
	listen 127.0.0.1:8082;
	server_name multi-port-server;
	client_max_body_size 1000000;
	
	error_page 404 /error_pages/404.html;
	
	location / {
		root /web/www;
		index index.html;
		allow_methods GET POST DELETE;
		autoindex on;
	}
}
```

**Result**: 3 IP Port objects created, same server accessible on all three addresses.

### Example 2: Virtual Hosting

```nginx
server {
	listen 9090;
	server_name first-vhost.example.com;
	
	location / {
		root /web/vhost1;
		index index.html;
		allow_methods GET;
	}
}

server {
	listen 9090;
	server_name second-vhost.example.com;
	
	location / {
		root /web/vhost2;
		index index.html;
		allow_methods GET POST;
	}
}
```

**Result**: 1 IP Port object with 2 servers. Server selection based on Host header.

### Example 3: Mixed Configuration

```nginx
server {
	listen 8080;
	listen 8081;
	server_name server-a;
	
	location / {
		root /web/a;
	}
}

server {
	listen 8080;
	server_name server-b;
	
	location / {
		root /web/b;
	}
}

server {
	listen 192.168.1.1:8080;
	server_name server-c;
	
	location / {
		root /web/c;
	}
}
```

**Result**: 3 IP Port objects:
- `localhost:8080`: servers A and B
- `localhost:8081`: server A only  
- `192.168.1.1:8080`: server C only

## Server Selection for Virtual Hosts

When multiple servers share the same IP Port, server selection is performed during request processing based on the HTTP Host header:

1. **Host Header Matching**: The incoming request's Host header is compared against each server's `server_name`
2. **Default Server**: If no exact match is found, the first server in the IP Port is used as default
3. **Fallback**: If no Host header is present, the default server handles the request

## Backward Compatibility

### Legacy Configurations

Old configurations continue to work without modification:

```nginx
server {
	listen 8080;
	server_name localhost;
	# ... rest of configuration
}
```

This creates a single IP Port object at `localhost:8080` with one server, exactly as before.

### Migration Path

To migrate existing configurations to use multiple listen directives:

**Before:**
```nginx
server {
	listen 8080;
	server_name example.com;
	# configuration...
}

server {
	listen 8081;
	server_name example.com;
	# same configuration...
}
```

**After:**
```nginx
server {
	listen 8080;
	listen 8081;
	server_name example.com;
	# configuration...
}
```

## Testing

### Test Configurations

The repository includes several test configurations:

- `web/multi_listen_test.conf`: Multi-port server example
- `web/shared_port_test.conf`: Virtual hosting example  
- `web/comprehensive_listen_test.conf`: Complete feature demonstration

### Verification

To verify the parsing is working correctly, run the server with debug output:

```bash
./webserv web/comprehensive_listen_test.conf
```

The server will display which IP Ports are created and how many servers are assigned to each.

## Error Handling

### Invalid Listen Directives

- **Invalid Port Numbers**: Ports must be valid integers (1-65535)
- **Invalid IP Addresses**: Malformed IP addresses will cause parsing errors
- **Missing Values**: `listen` directive requires at least a port number

### Configuration Validation

The server performs basic validation:
- Port numbers must be within valid range
- IP addresses must be properly formatted
- Listen directives must have valid syntax

## Performance Considerations

### Memory Usage

- Each unique address:port combination creates one IP Port object
- Servers are shared pointers, so memory usage is optimized
- Virtual hosting adds minimal overhead

### Socket Management

- Each IP Port creates one listening socket
- Multiple servers sharing an IP Port share the same socket
- Socket creation happens during `initSockets()` phase

## Future Enhancements

Potential future improvements:

1. **IPv6 Support**: Full IPv6 address parsing and binding
2. **Unix Socket Support**: Support for Unix domain sockets
3. **SSL/TLS Configuration**: Per-listen directive SSL settings
4. **Advanced Binding Options**: SO_REUSEPORT and other socket options

## Troubleshooting

### Common Issues

1. **Address Already in Use**: Another process is using the port
2. **Permission Denied**: Binding to privileged ports (< 1024) requires root
3. **Invalid Address**: IP address not available on the system

### Debug Information

Enable debug output by adding temporary logging to see:
- How many IP Ports are created
- Which servers are assigned to each IP Port  
- Address:port strings for each binding

## API Reference

### ConfigParser Methods

- `parseConfig(const std::string& configFile)`: Parse configuration file
- `createServersAndIpPortsFromConfig(Program &program)`: Create servers and IP Ports
- `getServerConfigs()`: Get parsed server configurations

### Key Classes

- `ListenConfig`: Represents a single listen directive
- `ServerConfig`: Enhanced server configuration with multiple listens
- `IpPort`: Represents a listening socket with associated servers
- `Program`: Main program class managing IP Ports and servers

---

*This documentation covers the enhanced listen directive functionality implemented in webserv. For additional information or support, refer to the source code comments and test configurations.*
