# Listen Directive and IP Port Management

## Quick Start

webserv now supports advanced listen directive parsing with automatic IP Port creation during configuration parsing.

### Multiple Ports per Server

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

**Result**: Server accessible on 3 different addresses (creates 3 IP Port objects)

### Virtual Hosting (Multiple Servers, Same Port)

```nginx
server {
	listen 8080;
	server_name site1.example.com;
	location / { root /web/site1; }
}

server {
	listen 8080;
	server_name site2.example.com;
	location / { root /web/site2; }
}
```

**Result**: Both servers share port 8080 (creates 1 IP Port object with 2 servers)

## Key Features

- ✅ Multiple `listen` directives per server
- ✅ Automatic IP Port creation during parsing
- ✅ Virtual hosting support (multiple servers per port)
- ✅ Both `port` and `address:port` formats supported
- ✅ Backward compatibility with existing configs

## Listen Directive Formats

```nginx
listen 8080;                    # localhost:8080
listen 127.0.0.1:8080;         # 127.0.0.1:8080
listen 192.168.1.100:9090;     # 192.168.1.100:9090
```

## Testing

Test configurations are provided in the `web/` directory:
- `multi_listen_test.conf` - Multiple ports example
- `shared_port_test.conf` - Virtual hosting example
- `comprehensive_listen_test.conf` - Complete feature demo

```bash
./webserv web/comprehensive_listen_test.conf
```

For detailed documentation, see [LISTEN_DIRECTIVE_DOCUMENTATION.md](LISTEN_DIRECTIVE_DOCUMENTATION.md).
