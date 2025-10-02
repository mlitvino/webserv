#!/bin/bash

echo "=== Testing POST Method Fixes ==="
echo ""

# Kill any existing webserv processes
pkill -f webserv 2>/dev/null
sleep 1

# Start the server in background
echo "Starting webserv server..."
cd /home/riamaev/Downloads/webserv-r_branch
./webserv web/test.conf &
SERVER_PID=$!

# Wait for server to start
sleep 2

echo "Server started with PID: $SERVER_PID"
echo ""

# Test POST with simple data
echo "1. Testing POST with simple form data:"
echo "curl -X POST -d 'name=test&data=hello' http://localhost:8080/upload"
curl -X POST -d 'name=test&data=hello' http://localhost:8080/upload
echo ""
echo ""

# Test POST with JSON data
echo "2. Testing POST with JSON data:"
echo "curl -X POST -H 'Content-Type: application/json' -d '{\"message\":\"Hello Webserv\"}' http://localhost:8080/api"
curl -X POST -H 'Content-Type: application/json' -d '{"message":"Hello Webserv"}' http://localhost:8080/api
echo ""
echo ""

# Test POST with multipart form data
echo "3. Testing POST with file upload:"
echo "Creating test file..."
echo "Hello from test file!" > test_upload.txt
echo "curl -X POST -F 'file=@test_upload.txt' http://localhost:8080/upload"
curl -X POST -F 'file=@test_upload.txt' http://localhost:8080/upload
echo ""
echo ""

# Test GET request to make sure it still works
echo "4. Testing GET request (should still work):"
echo "curl http://localhost:8080/"
curl http://localhost:8080/
echo ""
echo ""

# Test DELETE request
echo "5. Testing DELETE request:"
echo "curl -X DELETE http://localhost:8080/test/sample.txt"
curl -X DELETE http://localhost:8080/test/sample.txt
echo ""
echo ""

# Clean up
echo "Stopping server..."
kill $SERVER_PID 2>/dev/null
sleep 1
rm -f test_upload.txt

echo ""
echo "=== Test Complete ==="
echo "If you see proper HTML responses above without 'Connection reset by peer' errors,"
echo "then the POST method fixes are working correctly!"
