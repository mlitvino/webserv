# Listen Directive Quick Reference

## Syntax
```nginx
listen <port>;
listen <address>:<port>;
```

## Examples

### Single Port
```nginx
server {
	listen 8080;
	server_name example.com;
}
```
→ Creates: `localhost:8080` IP Port with 1 server

### Multiple Ports
```nginx
server {
	listen 8080;
	listen 8081;
	listen 127.0.0.1:8082;
	server_name example.com;
}
```
→ Creates: 3 IP Ports, each with 1 server

### Virtual Hosting
```nginx
server {
	listen 8080;
	server_name site1.com;
}
server {
	listen 8080;
	server_name site2.com;
}
```
→ Creates: 1 IP Port with 2 servers

## IP Port Creation Rules

| Configuration | IP Ports Created | Servers per IP Port |
|---------------|------------------|-------------------|
| 1 server, 1 listen | 1 | 1 |
| 1 server, 3 listens | 3 | 1 each |
| 2 servers, same listen | 1 | 2 |
| 2 servers, different listens | 2 | 1 each |

## Test Files

- `web/multi_listen_test.conf` - Multi-port example
- `web/shared_port_test.conf` - Virtual hosting example  
- `web/comprehensive_listen_test.conf` - Complete demo

## Code Changes Summary

1. **ConfigParser.hpp**: Added `ListenConfig` struct, enhanced `ServerConfig`
2. **ConfigParser.cpp**: New parsing logic for multiple listens, IP Port creation
3. **Program.cpp**: Simplified to use new parsing method
4. **Server.cpp**: Updated constructor for new config structure

## Backward Compatibility

✅ All existing configurations work unchanged
✅ Default behavior preserved (single listen → single IP Port)
✅ No breaking changes to existing APIs
