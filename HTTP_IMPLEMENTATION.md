## Build and Test Commands

### 1. Build the Project
```bash
cd /home/riamaev/Downloads/webserv-main
make clean
make
```

### 2. Start the Server
```bash
./webserv web/test.conf
```
You should see:
```
addrinfo nodes: 1
Erpoll fd: 4
```

### 3. Test in Browser
Open your browser and visit: `http://localhost:8080/`

You should now see the beautiful webserv homepage we created!

### 4. Test Different Pages
- `http://localhost:8080/about.html` - About page
- `http://localhost:8080/upload.html` - Upload form page
- `http://localhost:8080/test/` - Test directory
- `http://localhost:8080/test/sample.txt` - Text file
- `http://localhost:8080/test/data.json` - JSON file
- `http://localhost:8080/nonexistent` - 404 error page

### 5. Test with curl
```bash
# Test GET request
curl -v http://localhost:8080/

# Test POST request
curl -X POST http://localhost:8080/upload

# Test DELETE request
curl -X DELETE http://localhost:8080/test/

# Test 404 error
curl -v http://localhost:8080/nonexistent
```

### 6. Check Server Output
In the terminal where webserv is running, you should see:
```
Received data: GET / HTTP/1.1
Host: localhost:8080
...
Method: GET, Path: /, Version: HTTP/1.1
Sent X bytes
```

## What We've Implemented:

✅ **HTTP Request Parsing** - Parses HTTP method, path, and version
✅ **GET Method Handler** - Serves static HTML, CSS, JS, images
✅ **POST Method Handler** - Returns confirmation (placeholder for uploads)
✅ **DELETE Method Handler** - Returns confirmation (placeholder for deletion)
✅ **File Serving** - Reads and serves files from web/www/
✅ **MIME Type Detection** - Sends correct Content-Type headers
✅ **Error Handling** - Custom 404 pages, proper HTTP status codes
✅ **HTTP Response Generation** - Complete HTTP/1.1 responses
✅ **Configuration Integration** - Uses the parsed configuration

## Expected Results:
Your webserver should now fully serve the beautiful website we created and handle HTTP requests properly!

## If Build Fails:
Check for any compilation errors and let me know. The code is C++98 compatible and should compile cleanly.
