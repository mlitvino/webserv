#!/bin/bash

echo "=== STATUS CODE VERIFICATION TEST ==="
echo ""

# Function to test and verify status codes
test_status_code() {
    local method=$1
    local url=$2
    local expected=$3
    local description=$4
    local extra_args=$5
    
    echo "Testing: $description"
    echo "Command: curl -s -o /dev/null -w \"%{http_code}\" $extra_args -X $method $url"
    
    status=$(curl -s -o /dev/null -w "%{http_code}" $extra_args -X $method $url 2>/dev/null)
    
    if [ "$status" = "$expected" ]; then
        echo "✅ PASS: Got $status (expected $expected)"
    else
        echo "❌ FAIL: Got $status (expected $expected)"
    fi
    echo ""
}

# Test various scenarios
echo "Starting status code verification tests..."
echo ""

# GET tests
test_status_code "GET" "http://localhost:8080/" "200" "GET root page"
test_status_code "GET" "http://localhost:8080/index.html" "200" "GET index.html"
test_status_code "GET" "http://localhost:8080/about.html" "200" "GET about.html"
test_status_code "GET" "http://localhost:8080/upload.html" "200" "GET upload.html"
test_status_code "GET" "http://localhost:8080/test/" "200" "GET directory listing"
test_status_code "GET" "http://localhost:8080/test/sample.txt" "200" "GET text file"
test_status_code "GET" "http://localhost:8080/test/data.json" "200" "GET JSON file"
test_status_code "GET" "http://localhost:8080/nonexistent.html" "404" "GET non-existent file"
test_status_code "GET" "http://localhost:8080/missing/path/" "404" "GET non-existent path"

# POST tests
test_status_code "POST" "http://localhost:8080/upload" "200" "POST to upload" "-d 'test=data'"
test_status_code "POST" "http://localhost:8080/" "200" "POST to root" "-d 'name=test'"
test_status_code "POST" "http://localhost:8080/api" "200" "POST to API" "-H 'Content-Type: application/json' -d '{\"test\":\"json\"}'"

# DELETE tests  
test_status_code "DELETE" "http://localhost:8080/test/file" "200" "DELETE file"
test_status_code "DELETE" "http://localhost:8080/upload/" "200" "DELETE directory"

# Method not allowed tests
test_status_code "PUT" "http://localhost:8080/" "405" "PUT method (not allowed)"
test_status_code "PATCH" "http://localhost:8080/" "405" "PATCH method (not allowed)"
test_status_code "HEAD" "http://localhost:8080/" "405" "HEAD method (not allowed)"
test_status_code "OPTIONS" "http://localhost:8080/" "405" "OPTIONS method (not allowed)"

echo "=== DETAILED RESPONSE TESTING ==="
echo ""

echo "1. Testing GET response headers:"
curl -I http://localhost:8080/
echo ""

echo "2. Testing POST response with body:"
curl -X POST -d 'testdata=status_verification' http://localhost:8080/upload
echo ""

echo "3. Testing 404 response:"
curl -v http://localhost:8080/this_file_does_not_exist 2>&1 | head -20
echo ""

echo "4. Testing method not allowed response:"
curl -v -X PUT http://localhost:8080/ 2>&1 | head -20
echo ""

echo "=== STATUS CODE VERIFICATION COMPLETE ==="
