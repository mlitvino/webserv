# HTTP Status Codes Reference for Webserv Testing

## Standard HTTP Status Codes (RFC 7231)

### 1xx Informational
- **100 Continue**: Server received headers, client should send body
- **101 Switching Protocols**: Server switching protocols per client request

### 2xx Success  
- **200 OK**: Request succeeded
- **201 Created**: Request succeeded, resource created
- **202 Accepted**: Request accepted for processing
- **204 No Content**: Request succeeded, no response body
- **206 Partial Content**: Server delivering part of resource (range requests)

### 3xx Redirection
- **301 Moved Permanently**: Resource permanently moved to new URI
- **302 Found**: Resource temporarily moved  
- **304 Not Modified**: Resource not modified since last request
- **307 Temporary Redirect**: Temporary redirect, method must not change
- **308 Permanent Redirect**: Permanent redirect, method must not change

### 4xx Client Error
- **400 Bad Request**: Malformed request syntax
- **401 Unauthorized**: Authentication required
- **403 Forbidden**: Server understood but refuses authorization
- **404 Not Found**: Resource not found
- **405 Method Not Allowed**: Method not supported for resource
- **406 Not Acceptable**: Content not acceptable per Accept headers
- **408 Request Timeout**: Client timeout
- **409 Conflict**: Request conflicts with server state
- **410 Gone**: Resource no longer available
- **411 Length Required**: Content-Length header required
- **413 Payload Too Large**: Request entity too large
- **414 URI Too Long**: Request URI too long
- **415 Unsupported Media Type**: Media type not supported
- **422 Unprocessable Entity**: Syntactically correct but semantically incorrect
- **429 Too Many Requests**: Rate limit exceeded

### 5xx Server Error
- **500 Internal Server Error**: Generic server error
- **501 Not Implemented**: Method not implemented
- **502 Bad Gateway**: Invalid response from upstream server
- **503 Service Unavailable**: Server temporarily unavailable
- **504 Gateway Timeout**: Upstream server timeout
- **505 HTTP Version Not Supported**: HTTP version not supported

## Webserv Expected Status Codes

Based on the configuration and requirements, your webserver should handle:

### Must Support:
- **200 OK**: Successful GET requests
- **301 Moved Permanently**: Redirects configured in location blocks
- **404 Not Found**: Missing files/resources
- **405 Method Not Allowed**: Unsupported HTTP methods for specific routes
- **413 Payload Too Large**: Body size exceeding client_max_body_size
- **500 Internal Server Error**: Server-side errors

### Should Support:
- **403 Forbidden**: Access denied (file permissions, etc.)
- **501 Not Implemented**: Unsupported HTTP methods globally
- **400 Bad Request**: Malformed HTTP requests

### Testing Commands:

```bash
# Test 200 OK
curl -v http://localhost:8080/

# Test 404 Not Found  
curl -v http://localhost:8080/nonexistent.html

# Test 405 Method Not Allowed
curl -v -X POST http://localhost:8080/  # If only GET allowed

# Test 413 Payload Too Large
curl -v -X POST -d "$(printf 'A%.0s' {1..2000})" http://localhost:8080/upload

# Test 301 Redirect
curl -v http://localhost:8080/redirect

# Test with different methods
curl -v -X DELETE http://localhost:8080/upload/
curl -v -X PUT http://localhost:8080/api/
```

## Configuration Requirements Checklist

✅ **HTTP Status Codes**: Implement correct status codes per RFC
✅ **Multiple Ports**: Different servers on different ports  
✅ **Multiple Hostnames**: Virtual hosting with server_name
✅ **Custom Error Pages**: error_page directive for 404, 500, etc.
✅ **Body Size Limits**: client_max_body_size directive
✅ **Route Directories**: location blocks with different root paths
✅ **Default Index**: index directive for directory requests
✅ **Method Restrictions**: allow_methods directive per location
