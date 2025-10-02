#!/bin/bash

echo "=== Starting Debug Session ==="

# Kill any existing processes
pkill -f webserv 2>/dev/null
sleep 1

# Start server in background
echo "Starting webserv with debug output..."
cd /home/riamaev/Downloads/webserv-main
./webserv web/test.conf &
SERVER_PID=$!

# Wait for server to start
sleep 2

echo "Server PID: $SERVER_PID"

# Test with a simple HTTP request
echo ""
echo "Sending test request..."
echo -e "GET / HTTP/1.1\r\nHost: localhost:8080\r\nConnection: close\r\n\r\n" | nc localhost 8080

# Wait a bit
sleep 2

# Kill server
echo ""
echo "Stopping server..."
kill $SERVER_PID 2>/dev/null

echo "Debug session complete."
