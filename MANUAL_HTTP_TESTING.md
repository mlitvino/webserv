# 🧪 Manual HTTP Methods Testing Guide

## Overview
This guide provides step-by-step manual testing instructions for all HTTP methods using curl and telnet to verify your webserver handles requests properly and doesn't crash on unknown methods.

## Prerequisites
1. Start your webserver: `./webserv conf/virtual-hosts.conf` (or `conf/default.conf`)
2. Server should be running on `localhost:8080`
3. Ensure test files exist in web directories

## 1. Testing GET Requests

### Using curl
```bash
# Basic GET request
curl -v http://localhost:8080/

# GET with specific file
curl -v http://localhost:8080/index.html

# GET non-existent file (should return 404)
curl -v http://localhost:8080/nonexistent.html

# GET with different Host headers
curl -v -H "Host: example.com" http://localhost:8080/
curl -v --resolve example.com:8080:127.0.0.1 http://example.com:8080/

# Expected Result: 200 OK with file content, or 404 for missing files
```

### Using telnet
```bash
# Connect to server
telnet localhost 8080

# Type this (press Enter after each line, double Enter after last header):
GET / HTTP/1.1
Host: localhost
Connection: close

# Expected: HTTP/1.1 200 OK response with HTML content
```

### Manual telnet GET example:
```
$ telnet localhost 8080
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
GET / HTTP/1.1
Host: localhost
Connection: close

HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: 1234
Connection: close

<!DOCTYPE html>
<html>...
```

## 2. Testing POST Requests

### Using curl
```bash
# POST with simple data
curl -v -X POST -d "test data" http://localhost:8080/upload

# POST with form data
curl -v -X POST -F "name=test" -F "data=value" http://localhost:8080/upload

# POST with file upload
echo "test file content" > test.txt
curl -v -X POST -F "file=@test.txt" http://localhost:8080/upload

# POST with JSON data
curl -v -X POST -H "Content-Type: application/json" -d '{"key":"value"}' http://localhost:8080/upload

# POST to CGI endpoint
curl -v -X POST -d "name=John&age=30" http://localhost:8080/cgi-bin/demo.py

# Expected Result: 200 OK, 201 Created, or 303 See Other
```

### Using telnet
```bash
# Connect to server
telnet localhost 8080

# Type this (calculate Content-Length manually):
POST /upload HTTP/1.1
Host: localhost
Content-Type: text/plain
Content-Length: 9
Connection: close

test data

# Expected: HTTP/1.1 200 OK or appropriate response
```

### Manual telnet POST example:
```
$ telnet localhost 8080
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
POST /upload HTTP/1.1
Host: localhost
Content-Type: text/plain
Content-Length: 9
Connection: close

test data
HTTP/1.1 200 OK
Content-Type: text/html
...
```

## 3. Testing DELETE Requests

### Using curl
```bash
# Create a test file first
echo "delete me" > web/upload/deleteme.txt

# DELETE existing file
curl -v -X DELETE http://localhost:8080/upload/deleteme.txt

# DELETE non-existent file (should return 404)
curl -v -X DELETE http://localhost:8080/upload/nonexistent.txt

# DELETE with different paths
curl -v -X DELETE http://localhost:8080/upload/
curl -v -X DELETE http://localhost:8080/test/file.txt

# DELETE on different routes/servers
curl -v --resolve example.com:8080:127.0.0.1 -X DELETE http://example.com:8080/uploads/file.txt

# Expected Result: 200 OK, 204 No Content, or 404 Not Found
```

### Using telnet
```bash
# Connect to server
telnet localhost 8080

# Type this:
DELETE /upload/testfile.txt HTTP/1.1
Host: localhost
Connection: close

# Expected: HTTP/1.1 200 OK or 404 Not Found
```

### Manual telnet DELETE example:
```
$ telnet localhost 8080
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
DELETE /upload/deleteme.txt HTTP/1.1
Host: localhost
Connection: close

HTTP/1.1 200 OK
Content-Length: 0
Connection: close
```

## 4. Testing UNKNOWN/Invalid Methods

### Using curl
```bash
# Test invalid HTTP methods (should NOT crash server)
curl -v -X PATCH http://localhost:8080/
curl -v -X PUT http://localhost:8080/
curl -v -X OPTIONS http://localhost:8080/
curl -v -X HEAD http://localhost:8080/
curl -v -X TRACE http://localhost:8080/
curl -v -X CONNECT http://localhost:8080/

# Test completely invalid methods
curl -v -X INVALID http://localhost:8080/
curl -v -X FOOBAR http://localhost:8080/
curl -v -X "WEIRD METHOD" http://localhost:8080/

# Expected Result: 405 Method Not Allowed (server should NOT crash)
```

### Using telnet
```bash
# Test 1: Invalid method
telnet localhost 8080

INVALID / HTTP/1.1
Host: localhost
Connection: close

# Test 2: Malformed method
telnet localhost 8080

SUPERLONGMETHODNAME / HTTP/1.1
Host: localhost
Connection: close

# Test 3: Non-standard method  
telnet localhost 8080

PATCH / HTTP/1.1
Host: localhost
Connection: close

# Expected: HTTP/1.1 405 Method Not Allowed (NO crash)
```

## 5. Crash Testing - Malformed Requests

### Using telnet (these should NOT crash the server)
```bash
# Test 1: Malformed HTTP request
telnet localhost 8080
GET
(press Enter and close)

# Test 2: Invalid HTTP version
telnet localhost 8080
GET / HTTP/2.0
Host: localhost

# Test 3: Missing required headers
telnet localhost 8080
GET / HTTP/1.1

# Test 4: Very long request line
telnet localhost 8080
GET /very/long/path/that/goes/on/and/on/and/might/cause/buffer/overflow/issues HTTP/1.1
Host: localhost

# Test 5: Invalid characters in method
telnet localhost 8080
G€T / HTTP/1.1
Host: localhost

# Test 6: Incomplete request
telnet localhost 8080
POST / HTTP/1.1
Host: localhost
Content-Length: 100

short data
(close connection without sending full content)
```

## 6. Testing Scripts

### Automated Testing Script
```bash
#!/bin/bash
echo "🧪 Testing HTTP Methods"
echo "======================"

SERVER="http://localhost:8080"

echo "1. Testing GET..."
curl -s -o /dev/null -w "GET: %{http_code}\n" $SERVER/
curl -s -o /dev/null -w "GET (404): %{http_code}\n" $SERVER/nonexistent

echo "2. Testing POST..."
curl -s -o /dev/null -w "POST: %{http_code}\n" -X POST -d "data" $SERVER/upload/
curl -s -o /dev/null -w "POST CGI: %{http_code}\n" -X POST -d "test=value" $SERVER/cgi-bin/demo.py

echo "3. Testing DELETE..."
curl -s -o /dev/null -w "DELETE: %{http_code}\n" -X DELETE $SERVER/upload/test.txt
curl -s -o /dev/null -w "DELETE (404): %{http_code}\n" -X DELETE $SERVER/nonexistent.txt

echo "4. Testing INVALID methods (should not crash)..."
curl -s -o /dev/null -w "INVALID: %{http_code}\n" -X INVALID $SERVER/
curl -s -o /dev/null -w "PATCH: %{http_code}\n" -X PATCH $SERVER/
curl -s -o /dev/null -w "FOOBAR: %{http_code}\n" -X FOOBAR $SERVER/

echo "5. Server crash test..."
ps aux | grep webserv | grep -v grep > /dev/null
if [ $? -eq 0 ]; then
    echo "✅ Server still running after tests"
else
    echo "❌ Server crashed!"
fi
```

### Stress Testing with Multiple Methods
```bash
#!/bin/bash
echo "Stress testing HTTP methods..."

for i in {1..10}; do
    curl -s $SERVER/ > /dev/null &
    curl -s -X POST -d "test$i" $SERVER/upload/ > /dev/null &
    curl -s -X DELETE $SERVER/upload/test$i.txt > /dev/null &
    curl -s -X INVALID $SERVER/ > /dev/null &
    sleep 0.1
done

wait
echo "Stress test complete"
```

## 7. Expected Results

### Successful Requests:
- **GET /**: `200 OK` with HTML content
- **POST /upload**: `200 OK`, `201 Created`, or `303 See Other`  
- **DELETE /upload/file**: `200 OK` or `204 No Content`

### Error Responses:
- **GET /nonexistent**: `404 Not Found`
- **POST** to invalid location: `404 Not Found` or `405 Method Not Allowed`
- **DELETE /nonexistent**: `404 Not Found`
- **INVALID method**: `405 Method Not Allowed` or `501 Not Implemented`

### Critical Requirements:
- ✅ Server must NOT crash on any request
- ✅ Unknown methods return 405/501, not crash
- ✅ Malformed requests handled gracefully
- ✅ Server continues accepting new connections

## 8. Monitoring Server Health

### Check if server is still running:
```bash
# Check process
ps aux | grep webserv

# Test basic connectivity
curl -I http://localhost:8080/

# Check for core dumps
ls -la core* 2>/dev/null || echo "No core dumps found"

# Monitor server logs (if available)
tail -f server.log  # if your server logs to file
```

### Signs of proper behavior:
- Server process remains running
- Returns appropriate HTTP status codes
- No segmentation faults or crashes
- Continues accepting new connections after invalid requests

## 9. Manual Verification Checklist

- [ ] GET requests return 200 OK for existing files
- [ ] GET requests return 404 for missing files  
- [ ] POST requests work for file uploads
- [ ] POST requests work for CGI scripts
- [ ] DELETE requests work for existing files
- [ ] DELETE requests return 404 for missing files
- [ ] Invalid methods return 405 (not crash)
- [ ] Malformed requests don't crash server
- [ ] Server continues running after all tests
- [ ] New connections accepted after error requests

Run through this checklist manually to verify your webserver handles all HTTP methods correctly and robustly!
