# Webserv Basic Testing Guide

## 📋 Overview

This guide provides comprehensive instructions for testing the basic functionality of the webserv HTTP server implementation. These tests verify that all core HTTP methods work correctly and that the server handles various scenarios gracefully.

## 🚀 Prerequisites

Before running tests, ensure:

1. **Server is built**: Run `make clean && make` to compile the server
2. **Configuration file exists**: Default test configuration is `web/test.conf`
3. **Required tools**: `curl` command-line tool (standard on most systems)
4. **Port availability**: Ensure port 8080 is available (or modify configuration)

## 🔧 Test Setup

### 1. Start the Server

```bash
cd /path/to/webserv
./webserv web/test.conf &
```

### 2. Verify Server is Running

```bash
# Check if server process is running
jobs

# Test basic connectivity
curl -s -o /dev/null -w "Status: %{http_code}\n" http://localhost:8080/
```

Expected output: `Status: 200`

## 🧪 Basic Test Suite

### Test 1: GET Requests ✅

GET requests are the most common HTTP method for retrieving resources.

#### Test 1.1: Root Path
```bash
curl -v http://localhost:8080/
```
**Expected**: HTTP 200 OK with main page content

#### Test 1.2: Static File
```bash
curl -s -o /dev/null -w "Status: %{http_code}\n" http://localhost:8080/about.html
```
**Expected**: `Status: 200`

#### Test 1.3: Specific File in Directory
```bash
curl -s -o /dev/null -w "Status: %{http_code}\n" http://localhost:8080/test/test.html
```
**Expected**: `Status: 200`

#### Test 1.4: Non-existent File (404 Test)
```bash
curl -s -o /dev/null -w "Status: %{http_code}\n" http://localhost:8080/nonexistent.html
```
**Expected**: `Status: 404`

### Test 2: POST Requests ✅

POST requests are used for sending data to the server.

#### Test 2.1: Basic POST with Data
```bash
curl -X POST -d "test data" -H "Content-Type: text/plain" \
     -s -o /dev/null -w "Status: %{http_code}\n" http://localhost:8080/
```
**Expected**: `Status: 200`

#### Test 2.2: File Upload Simulation
```bash
echo "Hello World! Test content." > test_upload.txt
curl -X POST -F "file=@test_upload.txt" \
     -s -o /dev/null -w "Status: %{http_code}\n" http://localhost:8080/upload
```
**Expected**: `Status: 200`

#### Test 2.3: POST with Form Data
```bash
curl -X POST -d "name=test&value=data" \
     -H "Content-Type: application/x-www-form-urlencoded" \
     -s -o /dev/null -w "Status: %{http_code}\n" http://localhost:8080/
```
**Expected**: `Status: 200`

### Test 3: DELETE Requests ✅

DELETE requests are used for removing resources.

#### Test 3.1: DELETE File
```bash
curl -X DELETE -s -o /dev/null -w "Status: %{http_code}\n" \
     http://localhost:8080/test/sample.txt
```
**Expected**: `Status: 200`

#### Test 3.2: DELETE Directory
```bash
curl -X DELETE -s -o /dev/null -w "Status: %{http_code}\n" \
     http://localhost:8080/test/
```
**Expected**: `Status: 200`

### Test 4: Unknown/Unsupported Methods ✅

Test server's handling of unsupported HTTP methods.

#### Test 4.1: PATCH Method
```bash
curl -X PATCH -s -o /dev/null -w "Status: %{http_code}\n" http://localhost:8080/
```
**Expected**: `Status: 405` (Method Not Allowed)

#### Test 4.2: TRACE Method
```bash
curl -X TRACE -s -o /dev/null -w "Status: %{http_code}\n" http://localhost:8080/
```
**Expected**: `Status: 405` (Method Not Allowed)

#### Test 4.3: OPTIONS Method
```bash
curl -X OPTIONS -s -o /dev/null -w "Status: %{http_code}\n" http://localhost:8080/
```
**Expected**: `Status: 405` (Method Not Allowed)

### Test 5: Server Stability ✅

Test server stability under multiple requests.

#### Test 5.1: Rapid Consecutive Requests
```bash
for i in {1..5}; do 
    echo "Request $i:"
    curl -s -o /dev/null -w "Status: %{http_code}\n" http://localhost:8080/
done
```
**Expected**: All requests return `Status: 200`

#### Test 5.2: Mixed Method Sequence
```bash
curl -X POST -d "data1" -s -o /dev/null -w "POST: %{http_code}\n" http://localhost:8080/ && \
curl -X GET -s -o /dev/null -w "GET: %{http_code}\n" http://localhost:8080/about.html && \
curl -X DELETE -s -o /dev/null -w "DELETE: %{http_code}\n" http://localhost:8080/test/
```
**Expected**: 
- `POST: 200`
- `GET: 200` 
- `DELETE: 200`

## 📊 Expected Status Codes

| Scenario | Expected Status Code | Description |
|----------|---------------------|-------------|
| Valid GET request | 200 | OK - Resource found and served |
| Valid POST request | 200 | OK - Data received and processed |
| Valid DELETE request | 200 | OK - Delete request processed |
| File not found | 404 | Not Found - Resource doesn't exist |
| Unsupported method | 405 | Method Not Allowed - Method not supported |
| Server error | 500 | Internal Server Error - Server-side issue |

## 🔍 Debugging and Troubleshooting

### Common Issues and Solutions

#### 1. Connection Refused
```
* connect to 127.0.0.1 port 8080 failed: Connection refused
```
**Solution**: Ensure server is running on the correct port

#### 2. Server Not Starting
```
Fatal Error: bind: Address already in use
```
**Solution**: Kill existing processes or change port in configuration

#### 3. Permission Denied
```
Fatal Error: bind: Permission denied
```
**Solution**: Use ports > 1024 or run with appropriate permissions

### Debug Output Analysis

The server provides detailed debug output showing:
- Connection acceptance
- Request parsing
- Method handling
- Response generation
- Connection cleanup

Example debug output:
```
READING EPOLL EVENT
Accepting new connection...
DEBUG: handleEpollEvent called with events: 1
DEBUG: Current state: 0
DEBUG: EPOLLIN event - reading request
DEBUG: Received 78 bytes
DEBUG: Request line: GET / HTTP/1.1
Method: GET, Path: /, Version: HTTP/1.1
```

## 🛠️ Advanced Testing

### Custom Test Script

Create a comprehensive test script:

```bash
#!/bin/bash

echo "=========================================="
echo "WEBSERV BASIC TESTING SUITE"
echo "=========================================="

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

# Test function
test_request() {
    local method=$1
    local url=$2
    local expected=$3
    local description=$4
    
    echo -n "Testing $description: "
    status=$(curl -X "$method" -s -o /dev/null -w "%{http_code}" "$url")
    
    if [ "$status" = "$expected" ]; then
        echo -e "${GREEN}PASS${NC} (HTTP $status)"
        return 0
    else
        echo -e "${RED}FAIL${NC} (Expected: $expected, Got: $status)"
        return 1
    fi
}

# Start server
echo "Starting server..."
./webserv web/test.conf &
SERVER_PID=$!
sleep 2

# Run tests
test_request "GET" "http://localhost:8080/" "200" "GET root"
test_request "GET" "http://localhost:8080/about.html" "200" "GET static file"
test_request "GET" "http://localhost:8080/nonexistent" "404" "GET non-existent"
test_request "POST" "http://localhost:8080/" "200" "POST request"
test_request "DELETE" "http://localhost:8080/test/" "200" "DELETE request"
test_request "PATCH" "http://localhost:8080/" "405" "Unsupported method"

# Cleanup
echo "Cleaning up..."
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null

echo "Testing complete!"
```

## 📋 Test Checklist

Use this checklist to verify all tests pass:

- [ ] **GET /**: Returns HTTP 200 with main page
- [ ] **GET /about.html**: Returns HTTP 200 with about page
- [ ] **GET /nonexistent**: Returns HTTP 404 Not Found
- [ ] **POST /** with data: Returns HTTP 200
- [ ] **POST /upload** with file: Returns HTTP 200
- [ ] **DELETE /test/file**: Returns HTTP 200
- [ ] **PATCH /**: Returns HTTP 405 Method Not Allowed
- [ ] **TRACE /**: Returns HTTP 405 Method Not Allowed
- [ ] **Multiple rapid requests**: All return HTTP 200
- [ ] **Server stability**: No crashes during testing
- [ ] **Proper connection handling**: Connections close cleanly

## 🔄 Cleanup

After testing, clean up:

```bash
# Stop server
kill %1  # or use specific PID

# Remove test files
rm -f test_upload.txt

# Verify server stopped
jobs
```

## 📝 Notes

1. **Configuration**: Tests assume default `web/test.conf` configuration
2. **Port**: Default tests use port 8080 - modify if needed
3. **Files**: Some tests reference files in `web/www/` directory
4. **Debug**: Server runs with debug output enabled for troubleshooting
5. **Non-destructive**: Tests don't permanently modify server files

## 🎯 Success Criteria

A successful test run should show:
- All HTTP status codes match expected values
- No server crashes or unexpected terminations
- Proper handling of both valid and invalid requests
- Stable performance under multiple requests
- Graceful error handling for unsupported methods

This testing suite verifies that your webserv implementation correctly handles the core HTTP/1.1 functionality required for a robust web server.
