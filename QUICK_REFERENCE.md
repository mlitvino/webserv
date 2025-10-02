# Port Conflict Prevention - Quick Reference

## TL;DR

Webserv now prevents multiple servers from using the same port through early configuration validation.

## What Changed

✅ **Before**: Runtime `bind()` failures  
✅ **After**: Configuration-time validation with clear error messages

## Error Example

```bash
$ ./webserv web/conflict_test.conf
Configuration parsing error: Port conflict detected: Port 8080 is used by multiple servers (server1 and server2)
Using default configuration...
```

## Fix Your Config

```conf
# ❌ BAD - Port conflict
server {
    listen 8080;
    server_name server1;
}
server {
    listen 8080;  # Same port!
    server_name server2;
}

# ✅ GOOD - Unique ports
server {
    listen 8080;
    server_name server1;
}
server {
    listen 8081;  # Different port
    server_name server2;
}
```

## Test Your Config

```bash
# This will show port conflicts immediately
./webserv your_config.conf

# If no errors, server starts normally
# If conflicts found, falls back to default config
```

## Port Planning

- 8080: Main web server
- 8081: API server  
- 8082: File server
- 8083: Development

## More Info

See [PORT_CONFLICT_PREVENTION.md](./PORT_CONFLICT_PREVENTION.md) for complete documentation.
