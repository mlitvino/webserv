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

## 📚 Documentation

- **[TESTING_SUITE_DOCUMENTATION.md](./TESTING_SUITE_DOCUMENTATION.md)** - Comprehensive testing suite and verification
- **[PORT_CONFLICT_PREVENTION.md](./PORT_CONFLICT_PREVENTION.md)** - Comprehensive guide to port conflict prevention
- **[POST_METHOD_FIX.md](./POST_METHOD_FIX.md)** - POST method connection reset fix documentation
- **[TESTING_GUIDE.md](./TESTING_GUIDE.md)** - Testing procedures and examples
- **[HTTP_IMPLEMENTATION.md](./HTTP_IMPLEMENTATION.md)** - HTTP protocol implementation details
- **[PARSER_README.md](./PARSER_README.md)** - Configuration parser documentation

## 🛠️ Quick Start

```bash
# Build the server
make clean && make

# Run with default configuration
./webserv

# Run with custom configuration
./webserv web/test.conf

# Test the server
curl http://localhost:8080/
```

## ⚡ Key Features Fixed

### Port Conflict Prevention
This webserv implementation includes robust port conflict detection:

- **Early Detection**: Conflicts detected during configuration parsing
- **Clear Error Messages**: Detailed information about conflicting servers
- **Graceful Fallback**: Automatic fallback to default configuration
- **No Runtime Failures**: Eliminates binding-time crashes

### POST Method Connection Handling
Fixed "Connection reset by peer" errors in POST requests:

- **Proper HTTP Headers**: Complete HTTP/1.1 compliant responses
- **Connection Management**: Explicit connection lifecycle handling
- **Body Data Extraction**: Proper parsing and display of POST data
- **Error Resilience**: Graceful error handling prevents crashes

See documentation files for detailed technical information.

## 🧪 Testing

### Quick Demo (Interactive)
```bash
./master_demo.sh
```

### Comprehensive Testing
```bash
./comprehensive_test.sh    # Full automated test suite
./status_code_test.sh      # Status code verification
./telnet_test.sh          # Raw HTTP testing
./file_test.sh            # File upload/download
```

### Manual Testing
```bash
# Test basic functionality
./test_webserv.sh

# Test port conflicts
./webserv web/conflict_test.conf
```

See **[TESTING_SUITE_DOCUMENTATION.md](./TESTING_SUITE_DOCUMENTATION.md)** for complete testing documentation.