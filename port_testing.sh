#!/bin/sh

echo "==============================================="
echo "WEBSERV PORT TESTING COMPREHENSIVE SUITE"
echo "==============================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
NC='\033[0m' # No Color

print_test() {
    echo "${BLUE}[TEST]${NC} $1"
}

print_pass() {
    echo "${GREEN}[PASS]${NC} $1"
}

print_fail() {
    echo "${RED}[FAIL]${NC} $1"
}

print_info() {
    echo "${YELLOW}[INFO]${NC} $1"
}

print_section() {
    echo "\n${PURPLE}========== $1 ==========${NC}"
}

# Test 1: Multiple ports with different websites
test_multiple_ports_different_websites() {
    print_section "TEST 1: MULTIPLE PORTS WITH DIFFERENT WEBSITES"
    
    print_test "Testing port 8080 (Main site)"
    response=$(curl -s http://localhost:8080/ | head -1)
    if echo "$response" | grep -q "DOCTYPE\|html"; then
        print_pass "Port 8080: Serving main website"
        print_info "Content preview: $(echo "$response" | cut -c1-50)..."
    else
        print_fail "Port 8080: Not serving expected content"
    fi
    
    print_test "Testing port 8081 (Example.com site)"
    response=$(curl -s --resolve "example.com:8081:127.0.0.1" http://example.com:8081/ | head -1)
    if echo "$response" | grep -q "DOCTYPE\|html"; then
        print_pass "Port 8081: Serving example.com website"
        print_info "Content preview: $(echo "$response" | cut -c1-50)..."
    else
        print_fail "Port 8081: Not serving expected content"
    fi
    
    print_test "Testing port 8082 (Testsite.local)"
    response=$(curl -s --resolve "testsite.local:8082:127.0.0.1" http://testsite.local:8082/ | head -1)
    if echo "$response" | grep -q "DOCTYPE\|html"; then
        print_pass "Port 8082: Serving testsite.local website"
        print_info "Content preview: $(echo "$response" | cut -c1-50)..."
    else
        print_fail "Port 8082: Not serving expected content"
    fi
    
    print_test "Testing port 8083 (Virtual1.test)"
    response=$(curl -s --resolve "virtual1.test:8083:127.0.0.1" http://virtual1.test:8083/ | head -1)
    if echo "$response" | grep -q "DOCTYPE\|html"; then
        print_pass "Port 8083: Serving virtual1.test website"
        print_info "Content preview: $(echo "$response" | cut -c1-50)..."
    else
        print_fail "Port 8083: Not serving expected content"
    fi
}

# Test 2: Same port multiple times (should fail)
test_same_port_multiple_times() {
    print_section "TEST 2: SAME PORT MULTIPLE TIMES (SHOULD FAIL)"
    
    print_test "Testing configuration with duplicate ports"
    timeout 3 ./webserv web/conflict_test.conf > conflict_output.log 2>&1 &
    TEST_PID=$!
    sleep 1
    
    if grep -q "Port conflict detected" conflict_output.log; then
        print_pass "Duplicate ports correctly detected and rejected"
        print_info "Error message: $(grep "Port conflict" conflict_output.log)"
        print_info "Server gracefully fell back to default configuration"
    else
        print_fail "Duplicate ports not detected - this is a problem!"
    fi
    
    kill $TEST_PID 2>/dev/null
    rm -f conflict_output.log
}

# Test 3: Multiple server instances with common ports
test_multiple_servers_common_ports() {
    print_section "TEST 3: MULTIPLE SERVER INSTANCES WITH COMMON PORTS"
    
    print_test "Starting first server instance on port 8080"
    ./webserv web/default.conf > server1.log 2>&1 &
    SERVER1_PID=$!
    sleep 2
    
    if ps -p $SERVER1_PID > /dev/null; then
        print_pass "First server started successfully"
        
        print_test "Testing first server response"
        response=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/)
        if [ "$response" = "200" ]; then
            print_pass "First server responding on port 8080"
        else
            print_fail "First server not responding properly"
        fi
        
        print_test "Attempting to start second server on same port 8080"
        timeout 5 ./webserv web/test.conf > server2.log 2>&1 &
        SERVER2_PID=$!
        sleep 2
        
        # Check if second server failed to bind
        if grep -q "Address already in use\|bind.*failed" server2.log 2>/dev/null; then
            print_pass "Second server correctly failed with 'Address already in use'"
            print_info "Error: $(grep -i "address\|bind" server2.log 2>/dev/null | head -1)"
        elif ps -p $SERVER2_PID > /dev/null 2>&1; then
            print_fail "Second server should NOT have started on same port!"
            print_fail "This indicates a problem with port exclusivity"
            kill $SERVER2_PID 2>/dev/null
        else
            print_info "Second server failed to start (expected behavior)"
        fi
        
        kill $SERVER1_PID 2>/dev/null
    else
        print_fail "First server failed to start"
    fi
    
    # Clean up
    pkill -f webserv 2>/dev/null
    rm -f server1.log server2.log
}

# Test 4: Browser-like testing with different content per port
test_browser_simulation() {
    print_section "TEST 4: BROWSER SIMULATION - DIFFERENT CONTENT PER PORT"
    
    print_test "Fetching complete HTML from port 8080"
    response=$(curl -s http://localhost:8080/)
    if echo "$response" | grep -q "<title>"; then
        title=$(echo "$response" | grep -o "<title>[^<]*</title>" | sed 's/<[^>]*>//g')
        print_pass "Port 8080: Found HTML with title: '$title'"
    else
        print_fail "Port 8080: No valid HTML title found"
    fi
    
    print_test "Fetching complete HTML from port 8081 (example.com)"
    response=$(curl -s --resolve "example.com:8081:127.0.0.1" http://example.com:8081/)
    if echo "$response" | grep -q "<title>"; then
        title=$(echo "$response" | grep -o "<title>[^<]*</title>" | sed 's/<[^>]*>//g')
        print_pass "Port 8081: Found HTML with title: '$title'"
    else
        print_fail "Port 8081: No valid HTML title found"
    fi
    
    print_test "Checking if different ports serve different content"
    content1=$(curl -s http://localhost:8080/ | md5sum)
    content2=$(curl -s --resolve "example.com:8081:127.0.0.1" http://example.com:8081/ | md5sum)
    
    if [ "$content1" != "$content2" ]; then
        print_pass "Different ports serve different content (as expected)"
    else
        print_info "Same content served on different ports (could be intentional)"
    fi
}

# Test 5: Port accessibility verification
test_port_accessibility() {
    print_section "TEST 5: PORT ACCESSIBILITY VERIFICATION"
    
    ports="8080 8081 8082 8083"
    for port in $ports; do
        print_test "Testing port $port accessibility"
        if timeout 3 bash -c "</dev/tcp/localhost/$port" 2>/dev/null; then
            print_pass "Port $port: Accessible and listening"
        else
            print_fail "Port $port: Not accessible or not listening"
        fi
    done
}

# Main execution
main() {
    print_section "STARTING WEBSERV PORT TESTING"
    
    # Start webserv with our comprehensive configuration
    print_info "Starting webserv with config_simple.conf..."
    ./webserv web/config_simple.conf > test_server.log 2>&1 &
    MAIN_SERVER_PID=$!
    
    sleep 3
    
    if ps -p $MAIN_SERVER_PID > /dev/null; then
        print_pass "Main server started successfully"
        
        # Run all tests
        test_multiple_ports_different_websites
        test_port_accessibility
        test_browser_simulation
        
        # Kill main server for conflict tests
        kill $MAIN_SERVER_PID
        sleep 1
        
        test_same_port_multiple_times
        test_multiple_servers_common_ports
        
    else
        print_fail "Failed to start main server"
        cat test_server.log
    fi
    
    # Cleanup
    pkill -f webserv 2>/dev/null
    rm -f test_server.log
    
    print_section "PORT TESTING COMPLETE"
    print_info "Check results above for any issues"
}

# Run the main function
main
