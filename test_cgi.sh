#!/bin/bash

echo "🚀 Starting Webserver CGI Test..."
echo ""

# Navigate to project directory
cd /home/riamaev/webservNEW

# Check if webserver exists
if [ ! -f "./webserv" ]; then
    echo "❌ webserv binary not found. Compiling..."
    make
    if [ $? -ne 0 ]; then
        echo "❌ Compilation failed!"
        exit 1
    fi
fi

# Check CGI scripts permissions
echo "🔍 Checking CGI script permissions..."
chmod +x web/cgi-bin/*.py

# Start webserver in background
echo "🌐 Starting webserver on port 8080..."
./webserv conf/default.conf > server.log 2>&1 &
SERVER_PID=$!
echo "Server PID: $SERVER_PID"

# Wait for server to start
sleep 3

# Test CGI scripts
echo ""
echo "🧪 Testing CGI Scripts..."
echo ""

echo "1️⃣  Testing hello.py:"
curl -s http://localhost:8080/cgi-bin/hello.py | head -5
echo ""

echo "2️⃣  Testing test.py:"
curl -s http://localhost:8080/cgi-bin/test.py | grep -A2 "CGI Test Script"
echo ""

echo "3️⃣  Testing demo.py:"
curl -s http://localhost:8080/cgi-bin/demo.py | grep -A2 "CGI Demo"
echo ""

echo "4️⃣  Testing upload.py:"
curl -s http://localhost:8080/cgi-bin/upload.py | head -3
echo ""

# Stop server
echo "🛑 Stopping webserver..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo "✅ CGI test completed!"
echo ""
echo "📊 To start manually:"
echo "   ./webserv conf/default.conf"
echo "   Then visit: http://localhost:8080/cgi-bin/hello.py"
