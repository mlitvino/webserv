# Webserv Configuration Parser

This document describes the modern C++20 configuration parser implemented for the webserv HTTP server project.

## Features

The parser supports all the mandatory features required for the webserv project:

### Server Configuration
- Multiple server blocks
- Listen ports
- Server names
- Client max body size limits
- Custom error pages

### Location Blocks
- Path-based routing
- Root directory specification
- Index files
- HTTP method restrictions (GET, POST, DELETE, PUT, HEAD)
- Directory listing (autoindex)
- HTTP redirections
- CGI support (extension and path configuration)

### File Format

The configuration file uses a simple block-based syntax similar to NGINX:

```conf
server {
	listen 8080;
	server_name localhost;
	client_max_body_size 1048576;
	
	error_page 404 /error_pages/404.html;
	error_page 500 /error_pages/500.html;
	
	location / {
		root /web/www;
		index index.html;
		allow_methods GET POST DELETE;
		autoindex on;
	}
	
	location /upload {
		root /web/uploads;
		allow_methods POST;
		autoindex off;
	}
	
	location /cgi-bin {
		root /web/cgi-bin;
		allow_methods GET POST;
		cgi_extension .py;
		cgi_path /usr/bin/python3;
	}
}
```

### Supported Directives

#### Server Block Directives:
- `listen <port>` - Port to listen on
- `server_name <name>` - Server name
- `client_max_body_size <size>` - Maximum request body size in bytes
- `error_page <code> <path>` - Custom error page for HTTP status code

#### Location Block Directives:
- `root <path>` - Root directory for this location
- `index <file>` - Default index file
- `allow_methods <methods>` - Space-separated list of allowed HTTP methods
- `autoindex <on|off>` - Enable/disable directory listing
- `return <code> <url>` - HTTP redirection
- `cgi_extension <ext>` - File extension for CGI scripts
- `cgi_path <path>` - Path to CGI interpreter

## Implementation Details

### Classes and Structures

#### `ConfigParser`
Main parser class that handles configuration file parsing.

#### `ServerConfig`
Structure containing server configuration data:
- Host and port information
- Server name
- Client body size limit
- Error pages mapping
- Location configurations

#### `Location`
Structure containing location-specific configuration:
- Path pattern
- Root directory
- Index file
- Allowed HTTP methods (as type-safe enum class bitfield)
- Autoindex setting
- Redirect configuration
- CGI settings

### Modern C++20 Features

#### Smart Pointers
- `std::unique_ptr<Server>` for automatic server memory management
- `std::unique_ptr<ClientHandler>` for client connection management
- Eliminates memory leaks and manual resource cleanup

#### Type Safety
- `enum class HttpMethod` prevents implicit conversions
- `enum class ClientState` for connection state management
- Modern `using` type aliases for clarity

#### Performance Optimizations
- Move semantics for efficient resource transfers
- Range-based loops for cleaner iteration
- Standard library algorithms for optimized operations

### C++20 Modern Implementation

The parser leverages modern C++20 features for improved safety and performance:
- **Smart Pointers**: Uses `std::unique_ptr` for automatic memory management
- **Enum Classes**: Type-safe enums (`enum class HttpMethod`, `enum class ClientState`)
- **Modern Type Aliases**: Uses `using` declarations instead of `typedef`
- **Range-based Loops**: Modern iteration syntax for cleaner code
- **Move Semantics**: Efficient resource management with move constructors
- **RAII**: Resource Acquisition Is Initialization throughout
- **Standard Library**: Uses `std::to_string` and modern standard algorithms

### Memory Management

The parser creates Server objects from the configuration using modern C++20 smart pointers (`std::unique_ptr`) for automatic memory management. No manual memory cleanup is required - RAII ensures proper resource deallocation.

### Error Handling

- Configuration parsing errors are caught and logged
- Fallback to default configuration if parsing fails
- Validation of configuration values
- Proper cleanup on errors

## Usage

```cpp
// In main function
parser(data, av[1]);    // Parse configuration file
initServers(data);      // Initialize servers from config
acceptingLoop(data);    // Start server loop
```

The parser automatically handles:
- Missing configuration files (uses default)
- Invalid configuration syntax
- Multiple server blocks
- Default values for optional directives
- Automatic memory management with smart pointers
- Exception safety with RAII principles

## Build Requirements

- **C++20 Compiler**: GCC 8+ or Clang 7+ with C++20 support
- **Standard Library**: Modern C++20 standard library implementation
- **Build Command**: `make` (uses `-std=c++20` flag)

## Example Configurations

### Simple Single Server
```conf
server {
	listen 8080;
	server_name localhost;
	
	location / {
		root /web/www;
		index index.html;
		allow_methods GET;
	}
}
```

### Multiple Servers with Different Configurations
```conf
server {
	listen 8080;
	server_name localhost;
	client_max_body_size 1000000;
	
	location / {
		root /web/www;
		allow_methods GET POST DELETE;
		autoindex on;
	}
}

server {
	listen 8081;
	server_name api.localhost;
	client_max_body_size 500000;
	
	location /api {
		root /web/api;
		allow_methods GET POST;
	}
	
	location /upload {
		root /web/uploads;
		allow_methods POST;
	}
}
```

## Modern Implementation Benefits

### Memory Safety
- **Zero Memory Leaks**: Smart pointers ensure automatic cleanup
- **Exception Safety**: RAII guarantees resource cleanup even during exceptions
- **No Manual Memory Management**: Eliminates `new`/`delete` errors

### Code Quality
- **Type Safety**: Enum classes prevent common programming errors
- **Readability**: Modern syntax is more expressive and maintainable
- **Performance**: Move semantics reduce unnecessary copying
- **Standards Compliance**: Follows modern C++ best practices

### Development Experience
- **Easier Debugging**: Better error messages and cleaner stack traces
- **Maintainability**: Modern patterns make code easier to understand and modify
- **Future-Proof**: Uses contemporary C++ features and idioms

This modern C++20 parser implementation provides a robust, safe, and efficient foundation for the webserv project's configuration system while leveraging the latest C++ standards and best practices.
