#!/bin/bash

echo "=== Testing Modernized Webserv C++20 Server ==="
echo ""

# Kill any existing webserv processes
pkill -f webserv 2>/dev/null
sleep 1

# Start the server in background
echo "Starting webserv server on port 8080..."
cd /home/riamaev/Downloads/webserv-main
./webserv web/test.conf &
SERVER_PID=$!

# Wait for server to start
sleep 2

# Test if server is listening
echo "Checking if server is listening on port 8080..."
if netstat -tlnp 2>/dev/null | grep -q ":8080"; then
    echo "✅ Server is listening on port 8080"
else
    echo "❌ Server is not listening on port 8080"
    kill $SERVER_PID 2>/dev/null
    exit 1
fi

# Test GET request to root
echo ""
echo "Testing GET request to root (/)..."
if curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/ | grep -q "200"; then
    echo "✅ GET / returns HTTP 200"
else
    echo "❌ GET / failed"
fi

# Test GET request to index.html
echo ""
echo "Testing GET request to /index.html..."
response=$(curl -s http://localhost:8080/index.html)
if echo "$response" | grep -q "Webserv"; then
    echo "✅ GET /index.html returns valid content"
else
    echo "❌ GET /index.html failed or returned unexpected content"
fi

# Test GET request to non-existent file (should return 404)
echo ""
echo "Testing 404 error handling..."
status_code=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/nonexistent.html)
if [ "$status_code" = "404" ]; then
    echo "✅ GET /nonexistent.html returns HTTP 404"
else
    echo "❌ 404 handling failed (returned $status_code)"
fi

# Clean up
echo ""
echo "Stopping server..."
kill $SERVER_PID 2>/dev/null
sleep 1

echo ""
echo "=== Test Complete ==="
