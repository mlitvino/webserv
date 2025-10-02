# Webserv Testing Guide

## Overview

This guide provides comprehensive instructions for testing the Webserv HTTP server implementation.

## Web Pages Created

We have created a complete testing website structure:

### 1. Main Pages

#### `/web/www/index.html` - Home Page
- **URL:** `http://localhost:8080/`
- **Features:** Welcome page with server information, navigation, and feature overview
- **Tests:** Basic GET request handling, HTML serving, CSS styling

#### `/web/www/about.html` - About Page  
- **URL:** `http://localhost:8080/about.html`
- **Features:** Technical details, project information, testing instructions
- **Tests:** Static HTML file serving, internal navigation

#### `/web/www/upload.html` - Upload Form
- **URL:** `http://localhost:8080/upload.html`
- **Features:** File upload forms, curl testing examples, POST method testing
- **Tests:** Form handling, POST requests, multipart form data

### 2. Test Directory Structure

#### `/web/www/test/` - Test Directory
- **URL:** `http://localhost:8080/test/`
- **Features:** Directory listing (autoindex), multiple file types
- **Tests:** Directory browsing, autoindex functionality

#### Test Files:
- `test.html` - HTML file in subdirectory
- `sample.txt` - Plain text file for GET testing
- `data.json` - JSON data file for content-type testing

### 3. Error Pages

#### `/web/www/errors/404.html` - Custom 404 Page
- **URL:** Triggered on non-existent URLs
- **Features:** Custom styled error page with navigation
- **Tests:** Error handling, custom error page serving

## Configuration Files

### 1. `web/test.conf` - Basic Test Configuration
```conf
server {
	listen 8080;
	server_name localhost;
	client_max_body_size 1048576;
	
	error_page 404 /errors/404.html;
	error_page 500 /errors/500.html;
	
	location / {
		root web/www;
		index index.html;
		allow_methods GET POST DELETE;
		autoindex on;
	}
	
	location /upload {
		root web/www;
		allow_methods GET POST;
		autoindex off;
	}
	
	location /test {
		root web/www;
		allow_methods GET POST DELETE;
		autoindex on;
	}
}
```

### 2. `web/comprehensive.conf` - Full Feature Configuration
- Multiple server blocks
- Different ports (8080, 8081, 8082)
- Various location configurations
- CGI support configuration
- File upload areas
- Redirections

## Testing Scripts

### 1. `test_server.sh` - Automated Testing Script
**Usage:** `./test_server.sh`

**Features:**
- Server connectivity testing
- HTTP method testing (GET, POST, DELETE)
- Error handling verification
- Manual testing instructions
- Performance testing examples

**Test Categories:**
1. **Server Availability** - Basic connectivity
2. **HTTP Methods** - GET requests to all pages
3. **Error Handling** - 404 error testing
4. **Manual Testing** - Browser-based testing instructions
5. **Advanced Testing** - curl command examples

## Manual Testing Procedures

### 1. Basic Server Testing

#### Start the Server:
```bash
cd /home/riamaev/Downloads/webserv-main
./webserv web/test.conf
```

#### Test with Browser:
- Open browser and navigate to `http://localhost:8080/`
- Test all navigation links
- Try uploading files using the upload form
- Test directory browsing at `/test/`
- Try accessing non-existent pages for 404 testing

### 2. Command Line Testing

#### GET Requests:
```bash
curl http://localhost:8080/                    # Home page
curl http://localhost:8080/about.html          # About page  
curl http://localhost:8080/test/sample.txt     # Text file
curl http://localhost:8080/test/data.json      # JSON file
curl http://localhost:8080/test/               # Directory listing
```

#### POST Requests (if implemented):
```bash
# Simple POST data
curl -X POST -d 'test=data' http://localhost:8080/upload

# File upload
echo "Hello Webserv!" > test.txt
curl -X POST -F 'file=@test.txt' http://localhost:8080/upload
```

#### DELETE Requests (if implemented):
```bash
curl -X DELETE http://localhost:8080/test/sample.txt
```

#### Error Testing:
```bash
curl http://localhost:8080/nonexistent         # Should return 404
curl http://localhost:8080/fake/path           # Should return 404
```

### 3. Advanced Testing

#### Headers Testing:
```bash
curl -H "User-Agent: WebservTester/1.0" http://localhost:8080/
curl -H "Accept: application/json" http://localhost:8080/test/data.json
```

#### Method Testing:
```bash
curl -X HEAD http://localhost:8080/            # HEAD request
curl -X OPTIONS http://localhost:8080/         # OPTIONS request (if supported)
```

#### Performance Testing:
```bash
# Multiple concurrent requests
for i in {1..10}; do curl -s http://localhost:8080/ & done; wait
```

## Expected Behaviors

### 1. Successful Operations
- **200 OK** for existing files and pages
- **Proper HTML rendering** in browsers
- **CSS styling** applied correctly
- **Navigation links** working
- **Directory listing** (if autoindex enabled)

### 2. Error Handling
- **404 Not Found** for non-existent resources
- **Custom error pages** displayed
- **Graceful error recovery**

### 3. Configuration Features
- **Multiple locations** working correctly
- **Method restrictions** enforced
- **File size limits** respected
- **Custom error pages** served

## Troubleshooting

### Common Issues:

1. **Server not starting:**
- Check configuration file syntax
- Ensure port 8080 is available
- Check file permissions

2. **Pages not loading:**
- Verify file paths in configuration
- Check document root settings
- Ensure files exist

3. **404 errors for existing files:**
- Check location block configuration
- Verify root directory paths
- Check file permissions

### Debug Commands:

```bash
# Check if server is running
ps aux | grep webserv

# Check port availability  
netstat -tlnp | grep 8080

# Check server logs (if logging implemented)
tail -f server.log

# Test basic connectivity
telnet localhost 8080
```

## Success Criteria

### Parser Implementation ✅
- [x] Configuration file parsing
- [x] Multiple server blocks
- [x] Location blocks
- [x] Error page configuration
- [x] Method restrictions
- [x] Directory settings

### Web Structure ✅
- [x] Complete HTML website
- [x] Multiple page types
- [x] File upload forms
- [x] Test files and directories
- [x] Custom error pages
- [x] Professional styling

### Testing Infrastructure ✅
- [x] Automated test script
- [x] Manual testing procedures
- [x] Multiple configuration files
- [x] Comprehensive documentation
- [x] Troubleshooting guide

## Next Steps

The configuration parser and web structure are complete. The next development phase should focus on:

1. **HTTP Request/Response Handling**
- Complete HTTP protocol implementation
- Request parsing and response generation
- Method-specific handlers

2. **Feature Implementation**
- File upload processing
- CGI script execution
- Directory listing generation
- Error page serving

3. **Testing Integration**
- Run the automated test suite
- Validate all HTTP methods
- Performance and stress testing

This testing infrastructure provides a solid foundation for validating the webserv implementation as development continues.
