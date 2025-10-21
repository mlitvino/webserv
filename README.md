# Webserv - C++20 HTTP Server

A modern HTTP/1.1 server implementation in C++20 with comprehensive configuration management and robust error handling.

## 🚀 Features

- **HTTP/1.1 Protocol Support** - Full HTTP/1.1 implementation
- **Multiple Server Configurations** - Support for multiple virtual servers
- **Port Conflict Prevention** - Early detection and prevention of port binding conflicts
- **Modern C++20** - Smart pointers, RAII, and modern C++ best practices
- **Epoll Event Handling** - High-performance event-driven architecture
- **Configuration Parser** - Flexible nginx-style configuration format
- **Error Handling** - Graceful error handling with detailed diagnostics

## 🛠️ Quick Start

```bash
# Build the server
make clean && make

# Run with default configuration
./webserv

# Run with custom configuration
./webserv conf/test.conf

# Test the server
curl http://localhost:8080/
```

## ⚡ Key Features Fixed

### POST Method Connection Handling
Fixed "Connection reset by peer" errors in POST requests:

- **Proper HTTP Headers**: Complete HTTP/1.1 compliant responses
- **Connection Management**: Explicit connection lifecycle handling
- **Body Data Extraction**: Proper parsing and display of POST data
- **Error Resilience**: Graceful error handling prevents crashes

```
