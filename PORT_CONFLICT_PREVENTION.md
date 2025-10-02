# Webserv Port Conflict Prevention Documentation

## Overview

This document describes how the webserv HTTP server prevents and handles port conflicts when multiple server configurations attempt to bind to the same port.

## Table of Contents

1. [Problem Description](#problem-description)
2. [Solution Implementation](#solution-implementation)
3. [Configuration Validation](#configuration-validation)
4. [Error Handling](#error-handling)
5. [Testing](#testing)
6. [Best Practices](#best-practices)
7. [Troubleshooting](#troubleshooting)

## Problem Description

### What is a Port Conflict?

A port conflict occurs when multiple server instances attempt to bind to the same network port simultaneously. In TCP networking, only one process can bind to a specific port at a time.

### Original Issue in Webserv

Before the fix, webserv had the following behavior:

1. **Configuration Parsing**: Multiple server blocks could specify the same port
2. **Runtime Binding**: Each server would attempt to bind independently
3. **Failure Mode**: The second server would fail with "Address already in use" (EADDRINUSE)
4. **Result**: Application crash or partial functionality

```bash
# Example problematic configuration
server {
    listen 8080;        # First server binds successfully
    server_name server1;
}

server {
    listen 8080;        # Second server fails with EADDRINUSE
    server_name server2;
}
```

### Why SO_REUSEADDR Doesn't Help

The webserv implementation uses `SO_REUSEADDR`:

```cpp
int opt = 1;
setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
```

However, `SO_REUSEADDR` only helps with:
- ✅ Reusing ports after server restart (TIME_WAIT state)
- ❌ **Does NOT allow** multiple active sockets on the same port

## Solution Implementation

### Architecture Overview

The solution implements **early detection** during configuration parsing rather than runtime binding failure:

```
Configuration File → Parser → Validation → Server Creation → Binding
                              ↑
                         Port Conflict
                         Detection Here
```

### Code Implementation

#### 1. Header Declaration (`ConfigParser.hpp`)

```cpp
class ConfigParser {
private:
    // ... existing members ...
    void validatePortConflicts();  // Added method
    
public:
    // ... existing methods ...
};
```

#### 2. Validation Logic (`ConfigParser.cpp`)

```cpp
void ConfigParser::validatePortConflicts() {
    std::map<int, std::string> usedPorts;
    
    for (const auto& config : _serverConfigs) {
        auto it = usedPorts.find(config.port);
        if (it != usedPorts.end()) {
            throw std::runtime_error("Port conflict detected: Port " + 
                std::to_string(config.port) + " is used by multiple servers (" + 
                it->second + " and " + config.serverName + ")");
        }
        usedPorts[config.port] = config.serverName.empty() ? 
            ("server_" + std::to_string(config.port)) : config.serverName;
    }
}
```

#### 3. Integration in Parse Flow

```cpp
void ConfigParser::parseConfig(const std::string& configFile) {
    // ... parse configuration blocks ...
    
    if (_serverConfigs.empty()) {
        throw std::runtime_error("No server configurations found");
    }
    
    // Validate port conflicts after parsing
    validatePortConflicts();
}
```

## Configuration Validation

### Validation Rules

1. **Uniqueness**: Each server must have a unique port number
2. **Early Detection**: Validation occurs during configuration parsing
3. **Descriptive Errors**: Clear error messages identify conflicting servers
4. **Graceful Fallback**: System falls back to default configuration on conflicts

### Validation Algorithm

```
For each server configuration:
1. Check if port is already used by another server
2. If yes → Throw detailed error with server names
3. If no → Register port as used by this server
4. Continue to next server
```

### Error Message Format

```
Port conflict detected: Port 8080 is used by multiple servers (server1 and server2)
```

## Error Handling

### Exception Flow

```cpp
try {
    parser(data, av[1]);           // Parse with validation
    initServers(data);             // Initialize validated servers
    acceptingLoop(data);           // Start server loop
}
catch (const std::exception& e) {
    std::cerr << "Configuration parsing error: " << e.what() << std::endl;
    std::cerr << "Using default configuration..." << std::endl;
    // Falls back to default single server
}
```

### Fallback Mechanism

When port conflicts are detected:

1. **Error Reporting**: Descriptive error message to stderr
2. **Graceful Fallback**: Creates default single-server configuration
3. **Continued Operation**: Server starts with default settings
4. **User Notification**: Clear indication of fallback mode

## Testing

### Test Configuration for Port Conflicts

Create `web/conflict_test.conf`:

```conf
server {
    listen 8080;
    server_name server1;
    location / {
        root web/www;
        index index.html;
    }
}

server {
    listen 8080;    # Intentional conflict
    server_name server2;
    location / {
        root web/www;
        index index.html;
    }
}
```

### Test Execution

```bash
./webserv web/conflict_test.conf
```

### Expected Output

```
Configuration parsing error: Port conflict detected: Port 8080 is used by multiple servers (server1 and server2)
Using default configuration...
addrinfo nodes: 1
Epoll fd: 4
```

### Test Results Interpretation

- ✅ **Conflict Detected**: Error message identifies the problem
- ✅ **Graceful Handling**: Server doesn't crash
- ✅ **Fallback Working**: Default configuration loads successfully
- ✅ **Clear Communication**: User understands the issue

## Best Practices

### 1. Port Planning

```conf
# Recommended port allocation strategy
server {
    listen 8080;           # Main web server
    server_name main.localhost;
}

server {
    listen 8081;           # API server
    server_name api.localhost;
}

server {
    listen 8082;           # File server
    server_name files.localhost;
}

server {
    listen 8083;           # Development server
    server_name dev.localhost;
}
```

### 2. Port Ranges

- **8080-8089**: Web servers
- **8090-8099**: API servers
- **8100-8109**: File/upload servers
- **8110-8119**: Development/testing

### 3. Configuration Validation

Always test configuration files before deployment:

```bash
# Test configuration syntax
./webserv config_file.conf

# Check for port conflicts
grep -n "listen" config_file.conf | sort
```

### 4. Documentation Standards

Document each server's purpose and port in configuration comments:

```conf
# Main production web server - serves static content
server {
    listen 8080;
    server_name production.example.com;
    # ... configuration ...
}

# API endpoint server - handles REST API requests
server {
    listen 8081;
    server_name api.example.com;
    # ... configuration ...
}
```

## Troubleshooting

### Common Issues

#### 1. "Port conflict detected" Error

**Symptom**: 
```
Port conflict detected: Port 8080 is used by multiple servers
```

**Solution**:
- Check configuration file for duplicate `listen` directives
- Assign unique ports to each server block
- Use port planning strategy

#### 2. Server Falls Back to Default Configuration

**Symptom**:
```
Using default configuration...
addrinfo nodes: 1
```

**Causes**:
- Port conflicts in configuration
- Invalid configuration syntax
- Missing configuration file

**Solution**:
- Fix port conflicts
- Validate configuration syntax
- Ensure configuration file exists and is readable

#### 3. "Address already in use" Runtime Error

**Note**: This should NOT occur with the new validation, but if it does:

**Possible Causes**:
- Another process using the port
- Previous server instance still running
- System-level port binding issues

**Solutions**:
```bash
# Check what's using the port
netstat -tlnp | grep 8080
lsof -i :8080

# Kill existing processes
pkill -f webserv

# Wait for port to be released
sleep 2
```

### Debug Commands

```bash
# Check server process
ps aux | grep webserv

# Check port usage
netstat -tlnp | grep 8080
ss -tlnp | grep 8080

# Test connectivity
telnet localhost 8080
curl -I http://localhost:8080

# Configuration validation
./webserv config_file.conf
```

### Logging and Monitoring

For production environments, consider adding:

1. **Configuration Logging**: Log successful and failed configuration loads
2. **Port Monitoring**: Monitor port binding status
3. **Health Checks**: Regular connectivity tests
4. **Error Alerting**: Notifications for configuration failures

## Implementation Details

### Memory Management

The solution uses modern C++20 features:

```cpp
// Smart pointers for automatic cleanup
std::vector<std::unique_ptr<Server>> servers;

// RAII principles ensure proper resource management
// No manual memory management required
```

### Performance Impact

- **Validation Time**: O(n) where n = number of servers
- **Memory Usage**: Minimal - temporary map for port tracking
- **Startup Delay**: Negligible (microseconds for typical configurations)

### Thread Safety

The validation occurs during single-threaded configuration parsing phase, so no additional synchronization is required.

## Future Enhancements

### Potential Improvements

1. **Port Range Validation**: Ensure ports are in valid range (1-65535)
2. **Reserved Port Checking**: Warn about well-known ports (< 1024)
3. **Host:Port Combinations**: Allow same port on different interfaces
4. **Configuration Hot-Reload**: Runtime configuration updates with validation
5. **Advanced Error Recovery**: Attempt automatic port assignment

### Example Future Configuration

```conf
server {
    listen 0.0.0.0:8080;      # Specific interface
    server_name main.localhost;
}

server {
    listen 127.0.0.1:8080;    # Different interface, same port - could be allowed
    server_name local.localhost;
}
```

## Conclusion

The port conflict prevention system in webserv provides:

- ✅ **Early Detection**: Problems found during configuration parsing
- ✅ **Clear Error Messages**: Descriptive feedback for troubleshooting
- ✅ **Graceful Fallback**: System remains operational despite configuration issues
- ✅ **Best Practices**: Encourages proper configuration management
- ✅ **Zero Runtime Failures**: Eliminates binding-time crashes

This implementation ensures robust and reliable server operation while maintaining clear communication about configuration issues.
