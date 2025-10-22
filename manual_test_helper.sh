#!/bin/bash
# Manual HTTP Methods Testing Helper Script

echo "🧪 Manual HTTP Methods Testing Helper"
echo "====================================="

SERVER="http://localhost:8080"
TIMEOUT=5

# Check if server is running
check_server() {
    if ! curl -s --connect-timeout $TIMEOUT $SERVER/ > /dev/null 2>&1; then
        echo "❌ Server not responding at $SERVER"
        echo "Please start your server with: ./webserv conf/default.conf"
        exit 1
    fi
    echo "✅ Server is responding at $SERVER"
}

# Test function
test_method() {
    local method="$1"
    local path="$2"  
    local data="$3"
    local desc="$4"
    
    echo ""
    echo "Testing: $desc"
    echo "Command: curl -v -X $method ${data:+-d '$data'} $SERVER$path"
    echo "Press ENTER to execute, or 's' to skip:"
    read -r response
    
    if [ "$response" != "s" ]; then
        if [ -n "$data" ]; then
            curl -v -X "$method" -d "$data" "$SERVER$path"
        else
            curl -v -X "$method" "$SERVER$path"
        fi
        echo ""
        echo "Result above. Press ENTER to continue..."
        read -r
    fi
}

# Create test files
setup_test_files() {
    echo ""
    echo "Setting up test files..."
    mkdir -p web/upload 2>/dev/null
    echo "This is a test file for deletion" > web/upload/delete-me.txt
    echo "Test file content" > web/upload/test-file.txt
    echo "✅ Test files created"
}

check_server

echo ""
echo "This script will guide you through manual HTTP method testing."
echo "Each test will show the curl command before executing."
echo ""
echo "Press ENTER to start, or Ctrl+C to exit..."
read -r

setup_test_files

echo ""
echo "🟢 === TESTING GET REQUESTS ==="

test_method "GET" "/" "" "Basic GET request to root"
test_method "GET" "/index.html" "" "GET specific file"  
test_method "GET" "/nonexistent.html" "" "GET non-existent file (should be 404)"
test_method "GET" "/upload/" "" "GET directory (may show listing)"

echo ""
echo "🟡 === TESTING POST REQUESTS ==="

test_method "POST" "/upload/" "name=test&value=data" "POST with form data"
test_method "POST" "/upload/" "This is raw post data" "POST with raw text data"
test_method "POST" "/cgi-bin/demo.py" "name=John&age=30" "POST to CGI script"

echo ""
echo "🔴 === TESTING DELETE REQUESTS ==="

test_method "DELETE" "/upload/delete-me.txt" "" "DELETE existing file"
test_method "DELETE" "/upload/nonexistent.txt" "" "DELETE non-existent file (should be 404)"
test_method "DELETE" "/upload/" "" "DELETE on directory"

echo ""
echo "⚠️ === TESTING UNKNOWN/INVALID METHODS ==="

echo ""
echo "⚠️  The following tests use invalid HTTP methods."
echo "⚠️  Your server should NOT crash - it should return 405 Method Not Allowed"
echo ""

test_method "PATCH" "/" "" "PATCH method (invalid)"
test_method "PUT" "/" "" "PUT method (might be invalid)" 
test_method "OPTIONS" "/" "" "OPTIONS method (might be invalid)"
test_method "INVALID" "/" "" "Completely invalid method"
test_method "FOOBAR" "/" "" "Made-up method name"

echo ""
echo "🔍 === SERVER HEALTH CHECK ==="

echo ""
echo "Checking if server is still running after all tests..."

if curl -s --connect-timeout $TIMEOUT $SERVER/ > /dev/null 2>&1; then
    echo "✅ SUCCESS: Server is still responding!"
    echo "✅ Server handled invalid methods without crashing"
else
    echo "❌ FAILURE: Server is not responding"
    echo "❌ Server may have crashed during testing"
fi

echo ""
echo "Checking server process..."
if ps aux | grep webserv | grep -v grep > /dev/null; then
    echo "✅ Server process is still running"
else
    echo "❌ Server process not found"
fi

echo ""
echo "🏁 Testing complete!"
echo ""
echo "Summary of what should work:"
echo "✅ GET requests: 200 OK for existing files, 404 for missing"
echo "✅ POST requests: Should work for uploads and CGI"  
echo "✅ DELETE requests: Should work for existing files, 404 for missing"
echo "✅ Invalid methods: Should return 405/501, NOT crash server"
echo "✅ Server stability: Should keep running after all tests"
