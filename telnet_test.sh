#!/bin/bash

echo "=== TELNET TESTING SCRIPT ==="
echo ""

# Test basic GET request
echo "1. Testing basic GET request with telnet:"
echo "Command: telnet localhost 8080"
echo "Sending: GET / HTTP/1.1"
echo ""

(
echo "GET / HTTP/1.1"
echo "Host: localhost:8080"
echo "User-Agent: Telnet-Test/1.0"
echo "Connection: close"
echo ""
) | telnet localhost 8080 2>/dev/null

echo ""
echo "=================================="
echo ""

# Test POST request
echo "2. Testing POST request with telnet:"
echo "Sending: POST /upload HTTP/1.1 with form data"
echo ""

test_data="username=telnetuser&message=Hello+from+telnet&test=true"
content_length=${#test_data}

(
echo "POST /upload HTTP/1.1"
echo "Host: localhost:8080"
echo "Content-Type: application/x-www-form-urlencoded"
echo "Content-Length: $content_length"
echo "User-Agent: Telnet-Test/1.0"
echo "Connection: close"
echo ""
echo "$test_data"
) | telnet localhost 8080 2>/dev/null

echo ""
echo "=================================="
echo ""

# Test DELETE request
echo "3. Testing DELETE request with telnet:"
echo "Sending: DELETE /test/file.txt HTTP/1.1"
echo ""

(
echo "DELETE /test/file.txt HTTP/1.1"
echo "Host: localhost:8080"
echo "User-Agent: Telnet-Test/1.0"
echo "Connection: close"
echo ""
) | telnet localhost 8080 2>/dev/null

echo ""
echo "=================================="
echo ""

# Test invalid method
echo "4. Testing invalid HTTP method with telnet:"
echo "Sending: INVALID /test HTTP/1.1"
echo ""

(
echo "INVALID /test HTTP/1.1"
echo "Host: localhost:8080"
echo "Connection: close"
echo ""
) | telnet localhost 8080 2>/dev/null

echo ""
echo "=== TELNET TESTING COMPLETE ==="
