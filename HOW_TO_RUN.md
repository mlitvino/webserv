# How to Run Webserv and View in Chrome

## 🚀 **Step-by-Step Instructions**

### 1. **Start the Webserv Server**

Open a terminal and run:

```bash
cd /home/riamaev/Downloads/webserv-main
./webserv web/test.conf
```

You should see output like:
```
addrinfo nodes: 1
Epoll fd: 4
```

This means the server is running and listening on port 8080.

### 2. **Open Chrome and Navigate to the Website**

Open Chrome (or any web browser) and go to:

```
http://localhost:8080
```

Or try these specific pages:

- **Home Page**: `http://localhost:8080/`
- **About Page**: `http://localhost:8080/about.html`
- **Index Page**: `http://localhost:8080/index.html`

### 3. **Available Features**

Your modernized webserv supports:

- ✅ **GET Requests**: View web pages
- ✅ **POST Requests**: Submit forms
- ✅ **DELETE Requests**: Delete resources
- ✅ **Static File Serving**: HTML, CSS, JS, images
- ✅ **Error Pages**: Custom 404 and 500 pages
- ✅ **Directory Listing**: When autoindex is enabled

### 4. **Troubleshooting**

If the website doesn't load:

1. **Check if server is running**:
   ```bash
   ps aux | grep webserv
   ```

2. **Check if port 8080 is open**:
   ```bash
   netstat -tlnp | grep 8080
   ```

3. **Test with curl**:
   ```bash
   curl http://localhost:8080/
   ```

4. **Check server logs** by looking at the terminal where you started the server.

### 5. **Server Configuration**

The server uses `web/test.conf` which configures:
- **Port**: 8080
- **Document Root**: `web/www/`
- **Default Index**: `index.html`
- **Allowed Methods**: GET, POST, DELETE

### 6. **Stop the Server**

To stop the server, use Ctrl+C in the terminal where it's running, or:

```bash
pkill -f webserv
```

## 🎯 **Quick Start Commands**

```bash
# Navigate to project directory
cd /home/riamaev/Downloads/webserv-main

# Start the server
./webserv web/test.conf

# In another terminal, test it works:
curl http://localhost:8080/

# Open in Chrome:
# Navigate to http://localhost:8080
```

## 📁 **Available Web Content**

Your server serves files from `web/www/`:
- `index.html` - Main homepage
- `about.html` - About page  
- `favicon_io/` - Favicon files
- Any other files you place in this directory

The modernized C++20 webserv is now ready to serve your web content! 🌐
