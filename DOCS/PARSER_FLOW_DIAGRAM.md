# ConfigParser Flow Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                    ConfigParser::parseConfig()                  │
│                                                                 │
│  1. Open config file                                            │
│  2. Clear previous configurations                               │
│  3. Read line by line                                           │
└─────────────┬───────────────────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────────────────────────────────┐
│                     Main Parsing Loop                           │
│                                                                 │
│  While reading lines:                                           │
│  • Skip empty lines and comments (#)                            │
│  • Look for "server {" blocks                                   │
└─────────────┬───────────────────────────────────────────────────┘
              │
              ▼ Found "server {"
┌─────────────────────────────────────────────────────────────────┐
│                  parseServerBlock()                             │
│                                                                 │
│  For each line in server block:                                 │
│  • Skip empty lines/comments                                    │
│  • Check for "}" (end of block)                                 │
│  • Check for "location" directives → parseLocationBlock()       │
│  • Otherwise → parseServerDirective()                           │
└─────────────┬───────────────────────────────────────────────────┘
              │
              ▼
┌─────────────┬───────────────────────────────────────────────────┐
│ parseServerDirective()            │  parseLocationBlock()       │
│                                   │                             │
│ Handles:                          │  Handles:                   │
│ • listen 8080                     │  • root web/www             │
│ • listen 127.0.0.1:8082           │  • index index.html         │
│ • server_name example.com         │  • allow_methods GET POST   │
│ • client_max_body_size 1000000    │  • autoindex on             │
│ • error_page 404 /error.html      │  • return 301 /redirect     │
│                                   │  • cgi_extension .py        │
│ Creates ListenConfig objects      │  • cgi_path /usr/bin/python │
│ and adds to ServerConfig          │                             │
└─────────────┬───────────────────────────────────────────────────┘
              │
              ▼ All servers parsed
┌─────────────────────────────────────────────────────────────────┐
│            createServersAndIpPortsFromConfig()                  │
│                                                                 │
│  1. Create Server objects from ServerConfig                     │
│  2. For each server's listen directives:                        │
│     • Generate address:port string                              │
│     • Create IpPort if doesn't exist                            │
│     • Add server to IpPort                                      │
│  3. Set up handler mappings                                     │
└─────────────────────────────────────────────────────────────────┘
```

## Listen Directive Parsing Detail

```
"listen 127.0.0.1:8082"
         │
         ▼
┌─────────────────────────────────────┐
│     parseServerDirective()          │
│                                     │
│  1. Extract "listen" directive      │
│  2. Extract "127.0.0.1:8082"        │
│  3. Find ':' position               │
│  4. Split into host and port        │
│     host = "127.0.0.1"              │
│     port = 8082                     │
│  5. Create ListenConfig object      │
│  6. Add to config.listens vector    │
└─────────────────────────────────────┘
```

## Example: Multi-Server Configuration Processing

```
Input Config:
┌──────────────────────────┐    ┌──────────────────────────┐
│ server {                 │    │ server {                 │
│   listen 8080;           │    │   listen 8080;           │
│   listen 8081;           │    │   server_name site2;     │
│   server_name site1;     │    │   location / { ... }     │
│   location / { ... }     │    │ }                        │
│ }                        │    │                          │
└──────────────────────────┘    └──────────────────────────┘
         │                               │
         ▼                               ▼
┌──────────────────────────┐    ┌──────────────────────────┐
│ ServerConfig 1:          │    │ ServerConfig 2:          │
│ • listens: [             │    │ • listens: [             │
│     {localhost:8080},    │    │     {localhost:8080}     │
│     {localhost:8081}     │    │   ]                      │
│   ]                      │    │ • serverName: "site2"    │
│ • serverName: "site1"    │    │ • locations: [...]       │
│ • locations: [...]       │    │                          │
└──────────────────────────┘    └──────────────────────────┘
         │                               │
         └───────────┬───────────────────┘
                     ▼
    ┌─────────────────────────────────────────┐
    │    createServersAndIpPortsFromConfig    │
    │                                         │
    │  Process Server1:                       │
    │  • Create IpPort "localhost:8080"       │
    │  • Create IpPort "localhost:8081"       │
    │  • Add Server1 to both IpPorts          │
    │                                         │
    │  Process Server2:                       │
    │  • Find existing IpPort "localhost:8080"│
    │  • Add Server2 to existing IpPort       │
    └─────────────────────────────────────────┘
                     │
                     ▼
    ┌─────────────────────────────────────────┐
    │             Final Result:               │
    │                                         │
    │  IpPort "localhost:8080":               │
    │    servers: [Server1, Server2]          │
    │                                         │
    │  IpPort "localhost:8081":               │
    │    servers: [Server1]                   │
    │                                         │
    │  program._addrPortVec: 2 IpPorts        │
    │  program._servers: 2 Servers            │
    └─────────────────────────────────────────┘
```

## Data Flow Summary

```
Config File → parseConfig() → ServerConfig objects → createServersAndIpPortsFromConfig() → IpPort objects + Server objects
     │              │                │                            │                              │
     │              │                │                            │                              │
Raw text      Line-by-line     Structured data           Smart grouping                  Ready for
parsing      directive parsing   with multiple           by address:port                socket binding
                                 listen configs                                         and request handling
```
