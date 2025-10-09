# Step-by-Step: How the Enhanced ConfigParser Works

## Overview
The ConfigParser reads nginx-style configuration files and creates server configurations with multiple listen directives, then automatically creates IP Port objects during parsing.

## Step-by-Step Parsing Process

### Step 1: File Opening and Initial Setup
```cpp
void ConfigParser::parseConfig(const std::string& configFile) {
    _configFile = configFile;
    _serverConfigs.clear(); // Clear previous configurations
    std::ifstream file(configFile);

    if (!file.is_open()) {
        throw std::runtime_error("Cannot open configuration file: " + configFile);
    }
```
**What happens:**
- Opens the configuration file (e.g., `web/comprehensive_listen_test.conf`)
- Clears any previous configurations
- Throws error if file can't be opened

### Step 2: Main Parsing Loop
```cpp
std::string line;
while (std::getline(file, line)) {
    line = trim(line);

    if (line.empty() || line[0] == '#')
        continue;

    if (line.find("server") == 0) {
        ServerConfig config;
        parseServerBlock(file, config);
        _serverConfigs.push_back(config);
    }
}
```
**What happens:**
- Reads file line by line
- Skips empty lines and comments (`#`)
- When it finds `server {`, creates a new `ServerConfig` object
- Calls `parseServerBlock()` to parse the entire server block

### Step 3: Parsing Server Block
```cpp
void ConfigParser::parseServerBlock(std::ifstream& file, ServerConfig& config) {
    std::string line;

    while (std::getline(file, line)) {
        line = trim(line);

        if (line.empty() || line[0] == '#')
            continue;

        if (line == "}")
            break; // End of server block

        if (line.find("location") == 0) {
            // Parse location block
            Location location;
            std::istringstream iss(line);
            std::string directive;
            iss >> directive >> location.path;

            std::string braceLine;
            std::getline(file, braceLine); // Skip opening brace

            parseLocationBlock(file, location);
            config.locations.push_back(location);
        } else {
            parseServerDirective(line, config); // Parse server-level directives
        }
    }
}
```
**What happens:**
- Continues reading until it finds the closing `}` of the server block
- If it finds `location /path {`, it parses a location block
- Otherwise, it parses server-level directives (like `listen`, `server_name`)

### Step 4: Parsing Server Directives (Including Listen)
```cpp
void ConfigParser::parseServerDirective(const std::string& line, ServerConfig& config) {
    std::istringstream iss(line);
    std::string directive;
    iss >> directive;

    if (directive == "listen") {
        std::string listenValue;
        iss >> listenValue;
        
        ListenConfig listen;
        
        // Parse address:port or just port
        size_t colonPos = listenValue.find(':');
        if (colonPos != std::string::npos) {
            // Format: address:port (e.g., "127.0.0.1:8082")
            listen.host = listenValue.substr(0, colonPos);
            listen.port = std::stoi(listenValue.substr(colonPos + 1));
        } else {
            // Format: port only (e.g., "8080")
            listen.host = "localhost"; // default
            listen.port = std::stoi(listenValue);
        }
        
        config.listens.push_back(listen); // Add to server's listen list
    }
    // ... handle other directives (server_name, client_max_body_size, etc.)
}
```
**What happens:**
- Extracts the directive name (`listen`, `server_name`, etc.)
- For `listen` directives:
  - Parses both `8080` and `127.0.0.1:8080` formats
  - Creates a `ListenConfig` object
  - Adds it to the server's `listens` vector
- **Key Enhancement**: Multiple `listen` directives in one server are all stored

### Step 5: Parsing Location Blocks
```cpp
void ConfigParser::parseLocationBlock(std::ifstream& file, Location& location) {
    std::string line;

    while (std::getline(file, line)) {
        line = trim(line);

        if (line.empty() || line[0] == '#')
            continue;

        if (line == "}")
            break; // End of location block

        parseLocationDirective(line, location);
    }
}
```
**What happens:**
- Parses location-specific directives (`root`, `index`, `allow_methods`, etc.)
- Continues until closing `}` of location block

### Step 6: Creating Servers and IP Ports
```cpp
void ConfigParser::createServersAndIpPortsFromConfig(Program &program) {
    std::map<std::string, IpPortPtr> ipPortMap;
    
    // For each parsed server configuration
    for (const auto& config : _serverConfigs) {
        auto server = std::make_shared<Server>(config);
        
        // Handle default listen if none specified
        std::vector<ListenConfig> listens = config.listens;
        if (listens.empty()) {
            ListenConfig defaultListen;
            defaultListen.host = "localhost";
            defaultListen.port = 8080;
            listens.push_back(defaultListen);
        }
        
        // For each listen directive in this server
        for (const auto& listen : listens) {
            std::string addrPort = listen.getAddressPort(); // "localhost:8080"
            
            // Create IpPort if it doesn't exist
            if (ipPortMap.find(addrPort) == ipPortMap.end()) {
                auto ipPort = std::make_shared<IpPort>(program);
                ipPort->setAddrPort(addrPort);
                ipPortMap[addrPort] = ipPort;
                program._addrPortVec.push_back(ipPort);
            }
            
            // Add server to this IpPort
            ipPortMap[addrPort]->_servers.push_back(server);
        }
        
        program._servers.push_back(server);
    }
}
```
**What happens:**
- Creates `Server` objects from parsed configurations
- For each server's listen directives, creates or finds the corresponding `IpPort`
- Assigns servers to the appropriate `IpPort` objects
- **Key Innovation**: Multiple servers can share the same `IpPort` (virtual hosting)

## Example Walkthrough

Let's trace through parsing this configuration:

```nginx
server {
    listen 8080;
    listen 8081;
    listen 127.0.0.1:8082;
    server_name multi-port-server;
    
    location / {
        root web/www;
        index index.html;
    }
}

server {
    listen 8080;
    server_name another-server;
    
    location / {
        root web/www;
        index index.html;
    }
}
```

### Parsing Sequence:

1. **Read first `server {`**
   - Create `ServerConfig config1`

2. **Parse first server directives:**
   - `listen 8080` → Create `ListenConfig{host: "localhost", port: 8080}`
   - `listen 8081` → Create `ListenConfig{host: "localhost", port: 8081}` 
   - `listen 127.0.0.1:8082` → Create `ListenConfig{host: "127.0.0.1", port: 8082}`
   - `server_name multi-port-server` → Set server name
   - **Result**: `config1.listens` has 3 `ListenConfig` objects

3. **Parse location block:**
   - `location / {` → Create `Location` with path "/"
   - `root web/www` → Set location root
   - `index index.html` → Set location index
   - Add location to `config1.locations`

4. **Read second `server {`**
   - Create `ServerConfig config2`

5. **Parse second server directives:**
   - `listen 8080` → Create `ListenConfig{host: "localhost", port: 8080}`
   - `server_name another-server` → Set server name
   - **Result**: `config2.listens` has 1 `ListenConfig` object

6. **Create IP Ports:**
   - Process `config1`:
     - Server1 → IpPort "localhost:8080" (created)
     - Server1 → IpPort "localhost:8081" (created)  
     - Server1 → IpPort "127.0.0.1:8082" (created)
   - Process `config2`:
     - Server2 → IpPort "localhost:8080" (exists, add server to it)

### Final Result:
- **3 IP Port objects created:**
  - `"localhost:8080"` → [Server1, Server2] (2 servers)
  - `"localhost:8081"` → [Server1] (1 server)
  - `"127.0.0.1:8082"` → [Server1] (1 server)

## Data Structures

### ListenConfig
```cpp
struct ListenConfig {
    std::string host = "localhost";
    int port = 8080;
    std::string getAddressPort() const { return host + ":" + std::to_string(port); }
};
```

### ServerConfig
```cpp
struct ServerConfig {
    std::vector<ListenConfig> listens;  // NEW: Multiple listen directives
    std::string serverName;
    size_t clientMaxBodySize = 1000000;
    std::map<int, std::string> errorPages;
    std::vector<Location> locations;
};
```

## Key Enhancements

1. **Multiple Listen Support**: One server can have multiple `listen` directives
2. **Flexible Address Parsing**: Supports both `port` and `address:port` formats
3. **Automatic IP Port Creation**: No manual IP Port management required
4. **Virtual Hosting**: Multiple servers can share the same address:port
5. **Configuration-Driven**: Uses actual config values instead of hardcoded paths

## Benefits

- ✅ **Standard HTTP Server Behavior**: Matches nginx/Apache functionality
- ✅ **Virtual Hosting**: Multiple sites on same port with different server names
- ✅ **Multi-Port Servers**: One server accessible on multiple addresses/ports
- ✅ **Clean Architecture**: Parsing logic separated from socket management
- ✅ **Backward Compatible**: Existing configurations continue to work
