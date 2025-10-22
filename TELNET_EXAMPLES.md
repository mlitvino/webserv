# 📡 Telnet Manual Testing Examples

## Quick Telnet Commands Reference

### 1. Basic GET Request
```
telnet localhost 8080
GET / HTTP/1.1
Host: localhost
Connection: close

(press Enter twice after the last line)
```

### 2. POST Request with Data
```
telnet localhost 8080
POST /upload HTTP/1.1
Host: localhost
Content-Type: text/plain
Content-Length: 13
Connection: close

Hello Server!
(press Enter twice after the data)
```

### 3. DELETE Request  
```
telnet localhost 8080
DELETE /upload/test.txt HTTP/1.1
Host: localhost
Connection: close

(press Enter twice)
```

### 4. Invalid Method (Should NOT crash)
```
telnet localhost 8080
INVALID / HTTP/1.1
Host: localhost
Connection: close

(press Enter twice)
```

### 5. Malformed Request (Crash test)
```
telnet localhost 8080
GET
(just press Enter and close connection)
```

## Expected Responses:

**Valid requests:** HTTP/1.1 200 OK (or appropriate status)
**Invalid methods:** HTTP/1.1 405 Method Not Allowed  
**Missing files:** HTTP/1.1 404 Not Found
**Malformed:** Server should not crash, may close connection

## Copy-Paste Ready Commands

Save these to separate files and copy-paste into telnet:

**get_test.txt:**
```
GET / HTTP/1.1
Host: localhost
Connection: close


```

**post_test.txt:**
```
POST /upload HTTP/1.1
Host: localhost
Content-Type: text/plain
Content-Length: 9
Connection: close

test data

```

**delete_test.txt:**
```
DELETE /upload/file.txt HTTP/1.1
Host: localhost
Connection: close


```

**invalid_test.txt:**
```
INVALID / HTTP/1.1
Host: localhost  
Connection: close


```

Then use: `telnet localhost 8080 < get_test.txt`
