# Understanding std::istringstream

## What is std::istringstream?

`std::istringstream` (input string stream) is a C++ class from the `<sstream>` header that allows you to read data from a string using the same operations you would use to read from `std::cin` or a file.

## Basic Concept

Think of it as converting a string into a "fake input stream" that you can read from word by word, number by number, etc.

```cpp
#include <sstream>
#include <iostream>
#include <string>

// Instead of reading from keyboard:
// std::cin >> word1 >> word2 >> number;

// You can read from a string:
std::string data = "hello world 42";
std::istringstream iss(data);
std::string word1, word2;
int number;
iss >> word1 >> word2 >> number;
// word1 = "hello", word2 = "world", number = 42
```

## How It's Used in ConfigParser

In your ConfigParser code, `std::istringstream` is used to parse configuration lines by breaking them into individual components:

### Example 1: Parsing Server Directives
```cpp
void ConfigParser::parseServerDirective(const std::string& line, ServerConfig& config) {
    std::istringstream iss(line);  // Create stream from the line
    std::string directive;
    iss >> directive;              // Read first word (the directive name)

    if (directive == "listen") {
        std::string listenValue;
        iss >> listenValue;        // Read second word (the value)
        // Now directive = "listen", listenValue = "8080" or "127.0.0.1:8082"
    }
}
```

**Input line**: `"listen 127.0.0.1:8082"`
**After parsing**:
- `directive` = `"listen"`
- `listenValue` = `"127.0.0.1:8082"`

### Example 2: Parsing Location Directives
```cpp
void ConfigParser::parseLocationDirective(const std::string& line, Location& location) {
    std::istringstream iss(line);  // Create stream from the line
    std::string directive;
    iss >> directive;              // Read directive name

    if (directive == "allow_methods") {
        std::string methods;
        std::getline(iss, methods);  // Read the rest of the line
        location.allowedMethods = parseHttpMethods(trim(methods));
    }
}
```

**Input line**: `"allow_methods GET POST DELETE"`
**After parsing**:
- `directive` = `"allow_methods"`
- `methods` = `" GET POST DELETE"` (note the leading space)

### Example 3: Parsing Multiple Values
```cpp
if (directive == "error_page") {
    int code;
    std::string path;
    iss >> code >> path;  // Read two values in sequence
    config.errorPages[code] = path;
}
```

**Input line**: `"error_page 404 /errors/not_found.html"`
**After parsing**:
- `directive` = `"error_page"`
- `code` = `404`
- `path` = `"/errors/not_found.html"`

## Key Operations

### 1. `>>` Operator (Extraction)
Reads whitespace-separated tokens:
```cpp
std::string line = "listen 8080 localhost";
std::istringstream iss(line);
std::string cmd, port, host;
iss >> cmd >> port >> host;
// cmd = "listen", port = "8080", host = "localhost"
```

### 2. `std::getline()` 
Reads the rest of the line (including spaces):
```cpp
std::string line = "allow_methods GET POST DELETE";
std::istringstream iss(line);
std::string directive, methods;
iss >> directive;           // directive = "allow_methods"
std::getline(iss, methods); // methods = " GET POST DELETE"
```

### 3. Type Conversion
Automatically converts string to appropriate types:
```cpp
std::string line = "client_max_body_size 1000000";
std::istringstream iss(line);
std::string directive;
size_t size;
iss >> directive >> size;   // size is automatically converted to size_t
```

## Advantages of Using istringstream

### 1. **Automatic Whitespace Handling**
```cpp
// These all work the same:
"listen 8080"
"listen    8080"      // Multiple spaces
"\tlisten\t8080\t"    // Tabs
```

### 2. **Type Safety**
```cpp
std::string line = "port 8080";
std::istringstream iss(line);
std::string directive;
int port;
iss >> directive >> port;  // Automatic string-to-int conversion
```

### 3. **Error Handling**
```cpp
std::istringstream iss("port invalid_number");
std::string directive;
int port;
iss >> directive >> port;
if (iss.fail()) {
    // Conversion failed - port couldn't be parsed as integer
}
```

## Alternative Approaches (Why istringstream is Better)

### Manual String Parsing (More Error-Prone)
```cpp
// Instead of istringstream:
std::string line = "listen 127.0.0.1:8080";
size_t spacePos = line.find(' ');
std::string directive = line.substr(0, spacePos);
std::string value = line.substr(spacePos + 1);
// But what if there are multiple spaces? Leading/trailing spaces?
```

### String Splitting (More Complex)
```cpp
// Instead of istringstream:
std::vector<std::string> tokens = split(line, ' ');
std::string directive = tokens[0];
std::string value = tokens[1];
// But what about empty tokens from multiple spaces?
```

## Real ConfigParser Examples

### Parsing Location Blocks
```cpp
// Input: "location /api {"
std::istringstream iss(line);
std::string directive;
iss >> directive >> location.path;
// directive = "location", location.path = "/api"
```

### Parsing Return Directives
```cpp
// Input: "return 301 https://example.com"
if (directive == "return") {
    std::string code, url;
    iss >> code >> url;
    location.redirect = code + " " + url;
}
// code = "301", url = "https://example.com"
// redirect = "301 https://example.com"
```

## Summary

`std::istringstream` is a powerful tool for parsing structured text because it:

✅ **Handles whitespace automatically**
✅ **Converts types automatically** (string to int, etc.)
✅ **Reads data sequentially** like a real input stream
✅ **Provides error checking** for failed conversions
✅ **Makes code cleaner** than manual string manipulation

In your ConfigParser, it's essential for breaking down configuration lines like:
- `"listen 127.0.0.1:8082"` → `["listen", "127.0.0.1:8082"]`
- `"error_page 404 /not_found.html"` → `["error_page", "404", "/not_found.html"]`
- `"client_max_body_size 1000000"` → `["client_max_body_size", 1000000]`

This makes parsing configuration files much more reliable and maintainable!
