# 🛣️ Route Testing Guide for Webserv

## Overview
This guide helps you test different route configurations that map URLs to different directories in your webserver.

## Current Route Configuration

### 🏠 localhost Server Routes
```
Route: /           → Document Root: ./web/www
Route: /upload     → Document Root: ./web/upload  
Route: /cgi-bin    → Document Root: ./web/cgi-bin
```

### 🌐 example.com Server Routes  
```
Route: /           → Document Root: ./web/example
Route: /api        → Document Root: ./web/example/api
Route: /uploads    → Document Root: ./web/example/uploads
```

### 🧪 test.local Server Routes
```
Route: /           → Document Root: ./web/test
Route: /files      → Document Root: ./web/test/files
Route: /cgi-bin    → Document Root: ./web/cgi-bin (shared)
```

### 💻 dev.webserv.com Server Routes
```
Route: /           → Document Root: ./web/dev
Route: /admin      → Document Root: ./web/dev/admin
```

## Testing Commands

### 1. Test localhost Routes
```bash
# Root route
curl http://localhost:8080/

# Upload route  
curl http://localhost:8080/upload/

# CGI route
curl http://localhost:8080/cgi-bin/
```

### 2. Test example.com Routes
```bash
# Root route
curl --resolve example.com:8080:127.0.0.1 http://example.com:8080/

# API route
curl --resolve example.com:8080:127.0.0.1 http://example.com:8080/api/

# API JSON endpoint
curl --resolve example.com:8080:127.0.0.1 http://example.com:8080/api/status.json

# Uploads route
curl --resolve example.com:8080:127.0.0.1 http://example.com:8080/uploads/

# Uploads file
curl --resolve example.com:8080:127.0.0.1 http://example.com:8080/uploads/sample.txt
```

### 3. Test test.local Routes
```bash
# Root route
curl --resolve test.local:8080:127.0.0.1 http://test.local:8080/

# Files route
curl --resolve test.local:8080:127.0.0.1 http://test.local:8080/files/

# Files content
curl --resolve test.local:8080:127.0.0.1 http://test.local:8080/files/testfile.txt

# CGI route (shared)
curl --resolve test.local:8080:127.0.0.1 http://test.local:8080/cgi-bin/hello.py
```

### 4. Test dev.webserv.com Routes  
```bash
# Root route
curl --resolve dev.webserv.com:8080:127.0.0.1 http://dev.webserv.com:8080/

# Admin route
curl --resolve dev.webserv.com:8080:127.0.0.1 http://dev.webserv.com:8080/admin/
```

## Automated Route Testing Script

### Quick Test All Routes
```bash
#!/bin/bash
echo "🛣️ Testing All Routes"
echo "===================="

# Function to test a route
test_route() {
    local desc="$1"
    local url="$2" 
    local resolve="$3"
    
    echo ""
    echo "Testing: $desc"
    echo "URL: $url"
    
    if [ -z "$resolve" ]; then
        result=$(curl -s -w "Status: %{http_code}" "$url" | tail -1)
    else
        result=$(curl -s -w "Status: %{http_code}" --resolve "$resolve" "$url" | tail -1)
    fi
    
    if echo "$result" | grep -q "Status: 200"; then
        echo "✅ SUCCESS - Route working"
    else
        echo "❌ FAILED - $result"
    fi
}

# Test localhost routes
echo ""
echo "=== LOCALHOST ROUTES ==="
test_route "Root (/)" "http://localhost:8080/" ""
test_route "Upload (/upload)" "http://localhost:8080/upload/" ""
test_route "CGI (/cgi-bin)" "http://localhost:8080/cgi-bin/" ""

# Test example.com routes
echo ""
echo "=== EXAMPLE.COM ROUTES ==="
test_route "Root (/)" "http://example.com:8080/" "example.com:8080:127.0.0.1"
test_route "API (/api)" "http://example.com:8080/api/" "example.com:8080:127.0.0.1"
test_route "API JSON" "http://example.com:8080/api/status.json" "example.com:8080:127.0.0.1"
test_route "Uploads (/uploads)" "http://example.com:8080/uploads/" "example.com:8080:127.0.0.1"

# Test test.local routes  
echo ""
echo "=== TEST.LOCAL ROUTES ==="
test_route "Root (/)" "http://test.local:8080/" "test.local:8080:127.0.0.1"
test_route "Files (/files)" "http://test.local:8080/files/" "test.local:8080:127.0.0.1"
test_route "Test File" "http://test.local:8080/files/testfile.txt" "test.local:8080:127.0.0.1"

# Test dev.webserv.com routes
echo ""
echo "=== DEV.WEBSERV.COM ROUTES ==="
test_route "Root (/)" "http://dev.webserv.com:8080/" "dev.webserv.com:8080:127.0.0.1"
test_route "Admin (/admin)" "http://dev.webserv.com:8080/admin/" "dev.webserv.com:8080:127.0.0.1"

echo ""
echo "Route testing complete!"
```

## Testing Different HTTP Methods per Route

### Example.com API Route (supports GET, POST, DELETE)
```bash
# GET request
curl --resolve example.com:8080:127.0.0.1 -X GET http://example.com:8080/api/

# POST request  
curl --resolve example.com:8080:127.0.0.1 -X POST -d "test data" http://example.com:8080/api/

# DELETE request
curl --resolve example.com:8080:127.0.0.1 -X DELETE http://example.com:8080/api/
```

### Test.local Files Route (supports GET, POST, DELETE)
```bash
# GET request
curl --resolve test.local:8080:127.0.0.1 -X GET http://test.local:8080/files/

# POST request
curl --resolve test.local:8080:127.0.0.1 -X POST -d "file data" http://test.local:8080/files/

# DELETE request  
curl --resolve test.local:8080:127.0.0.1 -X DELETE http://test.local:8080/files/testfile.txt
```

### Dev.webserv.com Admin Route (supports GET, POST only)
```bash
# GET request (should work)
curl --resolve dev.webserv.com:8080:127.0.0.1 -X GET http://dev.webserv.com:8080/admin/

# POST request (should work)
curl --resolve dev.webserv.com:8080:127.0.0.1 -X POST -d "admin data" http://dev.webserv.com:8080/admin/

# DELETE request (should fail with 405 Method Not Allowed)
curl --resolve dev.webserv.com:8080:127.0.0.1 -X DELETE http://dev.webserv.com:8080/admin/
```

## Expected Behavior

### ✅ Success Cases (200 OK)
- All routes serve content from their configured directories
- Each route shows different styling/content  
- Files are served from correct directory paths
- Allowed HTTP methods work properly

### ❌ Error Cases
- **404 Not Found**: Route exists but file doesn't exist
- **405 Method Not Allowed**: Using unsupported HTTP method
- **403 Forbidden**: Directory access denied (if autoindex off and no index file)

## Autoindex Testing

### Routes with Autoindex ON (directory listing enabled)
```bash
# Should show directory contents when no index.html
curl --resolve example.com:8080:127.0.0.1 http://example.com:8080/uploads/
curl --resolve test.local:8080:127.0.0.1 http://test.local:8080/files/
```

### Routes with Autoindex OFF (no directory listing)
```bash
# Should show 403 or custom error if no index.html
curl --resolve example.com:8080:127.0.0.1 http://example.com:8080/api/
curl --resolve dev.webserv.com:8080:127.0.0.1 http://dev.webserv.com:8080/admin/
```

## Troubleshooting Routes

### If routes don't work:
1. **Check server is running**: `ps aux | grep webserv`
2. **Verify configuration**: Routes defined correctly in config
3. **Check file permissions**: Files readable by server process
4. **Test with verbose curl**: Add `-v` flag to see full request/response

### Common Issues:
- **Always serves root**: Route matching not implemented  
- **Wrong content**: Incorrect document root paths
- **404 on valid routes**: File doesn't exist in target directory
- **405 Method errors**: HTTP method not allowed for route

## Directory Structure Verification

Before testing, ensure these directories exist:
```bash
ls -la web/www/          # localhost root
ls -la web/upload/       # localhost /upload  
ls -la web/example/      # example.com root
ls -la web/example/api/  # example.com /api
ls -la web/example/uploads/ # example.com /uploads
ls -la web/test/         # test.local root  
ls -la web/test/files/   # test.local /files
ls -la web/dev/          # dev.webserv.com root
ls -la web/dev/admin/    # dev.webserv.com /admin
```
