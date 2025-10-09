/*
 * Example demonstrating the new Listen Directive and IP Port functionality
 * 
 * This shows how the enhanced ConfigParser creates IP Port objects during parsing
 * and assigns servers to them based on their listen directives.
 */

#include "webserv.hpp"
#include <iostream>

void demonstrateListenDirectiveParsing() {
    std::cout << "=== Listen Directive and IP Port Management Demo ===" << std::endl;
    
    /*
     * EXAMPLE 1: Multi-Port Server
     * 
     * Configuration:
     *   server {
     *       listen 8080;
     *       listen 8081;
     *       listen 127.0.0.1:8082;
     *       server_name multi-port-server;
     *   }
     * 
     * Result: Creates 3 IP Port objects:
     *   - localhost:8080 (1 server)
     *   - localhost:8081 (1 server)  
     *   - 127.0.0.1:8082 (1 server)
     */
    std::cout << "\nExample 1: Multi-Port Server" << std::endl;
    std::cout << "One server listening on multiple addresses/ports" << std::endl;
    std::cout << "Creates: 3 IP Port objects, 1 server" << std::endl;
    
    /*
     * EXAMPLE 2: Virtual Hosting
     * 
     * Configuration:
     *   server {
     *       listen 8080;
     *       server_name site1.example.com;
     *   }
     *   server {
     *       listen 8080;
     *       server_name site2.example.com;
     *   }
     * 
     * Result: Creates 1 IP Port object:
     *   - localhost:8080 (2 servers)
     */
    std::cout << "\nExample 2: Virtual Hosting" << std::endl;
    std::cout << "Multiple servers sharing the same port" << std::endl;
    std::cout << "Creates: 1 IP Port object, 2 servers" << std::endl;
    
    /*
     * EXAMPLE 3: Mixed Configuration
     * 
     * Configuration:
     *   server {
     *       listen 8080;
     *       listen 8081;
     *       server_name server-a;
     *   }
     *   server {
     *       listen 8080;
     *       server_name server-b;
     *   }
     * 
     * Result: Creates 2 IP Port objects:
     *   - localhost:8080 (2 servers: server-a, server-b)
     *   - localhost:8081 (1 server: server-a)
     */
    std::cout << "\nExample 3: Mixed Configuration" << std::endl;
    std::cout << "Combination of multi-port and virtual hosting" << std::endl;
    std::cout << "Creates: 2 IP Port objects, 2 servers" << std::endl;
    
    std::cout << "\n=== Key Implementation Points ===" << std::endl;
    std::cout << "1. IP Ports are created during parsing, not manually" << std::endl;
    std::cout << "2. Each unique address:port gets one IP Port object" << std::endl;
    std::cout << "3. Multiple servers can share the same IP Port" << std::endl;
    std::cout << "4. Server selection uses Host header for virtual hosting" << std::endl;
    std::cout << "5. Backward compatibility maintained with existing configs" << std::endl;
}

/*
 * Code Structure Overview:
 * 
 * ConfigParser::createServersAndIpPortsFromConfig() is the main method that:
 * 
 * 1. Iterates through parsed ServerConfig objects
 * 2. For each server's listen directives:
 *    - Creates IP Port object if address:port doesn't exist
 *    - Adds server to the appropriate IP Port
 * 3. Sets up handler mappings
 * 
 * Key data structures:
 * 
 * struct ListenConfig {
 *     std::string host;
 *     int port;
 *     std::string getAddressPort() const;
 * };
 * 
 * struct ServerConfig {
 *     std::vector<ListenConfig> listens;  // Multiple listen directives
 *     std::string serverName;
 *     // ... other server configuration
 * };
 * 
 * class IpPort {
 *     ServerDeq _servers;          // Servers assigned to this IP Port
 *     std::string _addrPort;       // Address:port string (e.g., "localhost:8080")
 *     // ... socket and epoll handling
 * };
 */

int main() {
    demonstrateListenDirectiveParsing();
    return 0;
}
