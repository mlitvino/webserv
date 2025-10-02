#!/bin/sh

# Comprehensive Configuration Testing Script for Webserv
# Tests all requested configuration features with proper validation

echo "=========================================================="
echo "WEBSERV COMPREHENSIVE CONFIGURATION TESTING"
echo "=========================================================="

# Test configuration
CONFIG_FILE="web/config_simple.conf"
WEBSERV_BINARY="./webserv"
TEST_FILE="web/www/test/sample.txt"
UPLOAD_FILE="test_upload.txt"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
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
	echo "\n${YELLOW}========== $1 ==========${NC}"
}

# Function to check if webserv is running
check_webserv_running() {
	if ! pgrep -f "$WEBSERV_BINARY" > /dev/null; then
		print_fail "Webserv is not running! Please start it first."
		echo "Run: $WEBSERV_BINARY $CONFIG_FILE"
		exit 1
	fi
	print_pass "Webserv is running"
}

# Function to test HTTP status codes
test_status_codes() {
	print_section "HTTP STATUS CODES TESTING"
	
	# Test 200 OK - Valid file
	print_test "Testing 200 OK - Valid file request"
	response=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/)
	if [ "$response" = "200" ]; then
		print_pass "200 OK: Valid file request works"
	else
		print_fail "200 OK: Expected 200, got $response"
	fi
	
	# Test 404 Not Found - Non-existent file
	print_test "Testing 404 Not Found - Non-existent file"
	response=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/nonexistent.html)
	if [ "$response" = "404" ]; then
		print_pass "404 Not Found: Non-existent file correctly returns 404"
	else
		print_fail "404 Not Found: Expected 404, got $response"
	fi
	
	# Test 405 Method Not Allowed - DELETE on restricted route
	print_test "Testing 405 Method Not Allowed - DELETE on restricted route"
	response=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE http://localhost:8081/readonly/)
	if [ "$response" = "405" ]; then
		print_pass "405 Method Not Allowed: DELETE on restricted route correctly returns 405"
	else
		print_fail "405 Method Not Allowed: Expected 405, got $response"
	fi
	
	# Test 413 Content Too Large - Large body on size-limited server
	print_test "Testing 413 Content Too Large - Large body on port 8082 (512 byte limit)"
	large_data=$(printf 'a%.0s' $(seq 1 1000))  # 1000 bytes, exceeds 512 byte limit
	response=$(curl -s -o /dev/null -w "%{http_code}" -X POST -H "Content-Type: text/plain" --data "$large_data" http://localhost:8082/)
	if [ "$response" = "413" ]; then
		print_pass "413 Content Too Large: Large body correctly rejected"
	else
		print_fail "413 Content Too Large: Expected 413, got $response"
	fi
}

# Function to test multiple servers with different ports
test_multiple_ports() {
	print_section "MULTIPLE SERVERS - DIFFERENT PORTS"
	
	# Test all configured ports
	ports="8080 8081 8082 8083"
	for port in $ports; do
		print_test "Testing server on port $port"
		response=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:$port/)
		if [ "$response" = "200" ]; then
			print_pass "Port $port: Server responding correctly"
		else
			print_fail "Port $port: Server not responding (got $response)"
		fi
	done
}

# Function to test multiple servers with different hostnames
test_multiple_hostnames() {
	print_section "MULTIPLE SERVERS - DIFFERENT HOSTNAMES"
	
	# Test localhost (default)
	print_test "Testing localhost hostname"
	response=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/)
	if [ "$response" = "200" ]; then
		print_pass "localhost: Working correctly"
	else
		print_fail "localhost: Not working (got $response)"
	fi
	
	# Test example.com (using --resolve to simulate DNS)
	print_test "Testing example.com hostname"
	response=$(curl -s -o /dev/null -w "%{http_code}" --resolve "example.com:8081:127.0.0.1" http://example.com:8081/)
	if [ "$response" = "200" ]; then
		print_pass "example.com: Virtual host working correctly"
	else
		print_fail "example.com: Virtual host not working (got $response)"
	fi
	
	# Test testsite.local
	print_test "Testing testsite.local hostname"
	response=$(curl -s -o /dev/null -w "%{http_code}" --resolve "testsite.local:8082:127.0.0.1" http://testsite.local:8082/)
	if [ "$response" = "200" ]; then
		print_pass "testsite.local: Virtual host working correctly"
	else
		print_fail "testsite.local: Virtual host not working (got $response)"
	fi
	
	# Test virtual1.test
	print_test "Testing virtual1.test hostname"
	response=$(curl -s -o /dev/null -w "%{http_code}" --resolve "virtual1.test:8083:127.0.0.1" http://virtual1.test:8083/)
	if [ "$response" = "200" ]; then
		print_pass "virtual1.test: Virtual host working correctly"
	else
		print_fail "virtual1.test: Virtual host not working (got $response)"
	fi
}

# Function to test custom error pages
test_custom_error_pages() {
	print_section "CUSTOM ERROR PAGES"
	
	# Test custom 404 page on localhost:8080
	print_test "Testing custom 404 page on localhost:8080"
	response=$(curl -s http://localhost:8080/nonexistent.html)
	if echo "$response" | grep -q "Page Not Found"; then
		print_pass "Custom 404 page: Working correctly on localhost:8080"
	else
		print_fail "Custom 404 page: Not working on localhost:8080"
	fi
	
	# Test custom 404 page on testsite.local:8082
	print_test "Testing custom 404 page on testsite.local:8082"
	response=$(curl -s --resolve "testsite.local:8082:127.0.0.1" http://testsite.local:8082/nonexistent.html)
	if echo "$response" | grep -q "File not found"; then
		print_pass "Custom 404 page: Working correctly on testsite.local"
	else
		print_fail "Custom 404 page: Not working on testsite.local"
	fi
}

# Function to test client body size limits
test_body_size_limits() {
	print_section "CLIENT BODY SIZE LIMITS"
	
	# Test small body within limit (port 8082 has 512 byte limit)
	print_test "Testing small body within 512 byte limit (port 8082)"
	small_data=$(printf 'a%.0s' $(seq 1 400))  # 400 bytes, within limit
	response=$(curl -s -o /dev/null -w "%{http_code}" -X POST -H "Content-Type: text/plain" --data "$small_data" http://localhost:8082/)
	if [ "$response" = "200" ]; then
		print_pass "Small body: Accepted correctly within limit"
	else
		print_fail "Small body: Expected 200, got $response"
	fi
	
	# Test large body exceeding limit (port 8082 has 512 byte limit)
	print_test "Testing large body exceeding 512 byte limit (port 8082)"
	large_data=$(printf 'a%.0s' $(seq 1 600))  # 600 bytes, exceeds limit
	response=$(curl -s -o /dev/null -w "%{http_code}" -X POST -H "Content-Type: text/plain" --data "$large_data" http://localhost:8082/)
	if [ "$response" = "413" ]; then
		print_pass "Large body: Correctly rejected with 413"
	else
		print_fail "Large body: Expected 413, got $response"
	fi
	
	# Test medium body on higher limit server (port 8083 has 4KB limit)
	print_test "Testing medium body within 4KB limit (port 8083)"
	medium_data=$(printf 'a%.0s' $(seq 1 2000))  # 2KB, within 4KB limit
	response=$(curl -s -o /dev/null -w "%{http_code}" -X POST -H "Content-Type: text/plain" --data "$medium_data" http://localhost:8083/)
	if [ "$response" = "200" ]; then
		print_pass "Medium body: Accepted correctly within 4KB limit"
	else
		print_fail "Medium body: Expected 200, got $response"
	fi
}

# Function to test routes to different directories
test_routes_directories() {
	print_section "ROUTES TO DIFFERENT DIRECTORIES"
	
	# Test /test/ route pointing to www/test/
	print_test "Testing /test/ route to www/test/ directory"
	response=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/test/)
	if [ "$response" = "200" ]; then
		print_pass "Route /test/: Working correctly"
	else
		print_fail "Route /test/: Not working (got $response)"
	fi
	
	# Test /test/sample.txt file in routed directory
	print_test "Testing file access in routed directory /test/sample.txt"
	response=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/test/sample.txt)
	if [ "$response" = "200" ]; then
		print_pass "File in routed directory: Accessible correctly"
	else
		print_fail "File in routed directory: Not accessible (got $response)"
	fi
	
	# Test /readonly/ route (different from main root)
	print_test "Testing /readonly/ route"
	response=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:8081/readonly/)
	if [ "$response" = "200" ]; then
		print_pass "Route /readonly/: Working correctly"
	else
		print_fail "Route /readonly/: Not working (got $response)"
	fi
}

# Function to test default files for directories
test_default_files() {
	print_section "DEFAULT FILES FOR DIRECTORIES"
	
	# Test default index.html when accessing directory
	print_test "Testing default index.html for directory access"
	response=$(curl -s http://localhost:8080/)
	if echo "$response" | grep -q -i "html"; then
		print_pass "Default file: index.html served correctly for directory"
	else
		print_fail "Default file: index.html not served for directory"
	fi
	
	# Test explicit index.html access
	print_test "Testing explicit index.html access"
	response=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/index.html)
	if [ "$response" = "200" ]; then
		print_pass "Explicit index.html: Accessible correctly"
	else
		print_fail "Explicit index.html: Not accessible (got $response)"
	fi
	
	# Test directory without trailing slash (should redirect or serve index)
	print_test "Testing directory access without trailing slash"
	response=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/test)
	if [ "$response" = "200" ] || [ "$response" = "301" ] || [ "$response" = "302" ]; then
		print_pass "Directory without slash: Handled correctly (got $response)"
	else
		print_fail "Directory without slash: Not handled correctly (got $response)"
	fi
}

# Function to test method permissions
test_method_permissions() {
	print_section "METHOD PERMISSIONS"
	
	# Test allowed methods
	print_test "Testing allowed GET method on main location"
	response=$(curl -s -o /dev/null -w "%{http_code}" -X GET http://localhost:8080/)
	if [ "$response" = "200" ]; then
		print_pass "GET method: Allowed correctly"
	else
		print_fail "GET method: Not working (got $response)"
	fi
	
	print_test "Testing allowed POST method on main location"
	response=$(curl -s -o /dev/null -w "%{http_code}" -X POST -H "Content-Type: text/plain" --data "test" http://localhost:8080/)
	if [ "$response" = "200" ]; then
		print_pass "POST method: Allowed correctly"
	else
		print_fail "POST method: Not working (got $response)"
	fi
	
	# Test restricted methods on /readonly/ location (only GET allowed)
	print_test "Testing GET method on restricted /readonly/ location"
	response=$(curl -s -o /dev/null -w "%{http_code}" -X GET http://localhost:8081/readonly/)
	if [ "$response" = "200" ]; then
		print_pass "GET on restricted location: Allowed correctly"
	else
		print_fail "GET on restricted location: Not working (got $response)"
	fi
	
	print_test "Testing POST method on restricted /readonly/ location (should be denied)"
	response=$(curl -s -o /dev/null -w "%{http_code}" -X POST -H "Content-Type: text/plain" --data "test" http://localhost:8081/readonly/)
	if [ "$response" = "405" ]; then
		print_pass "POST on restricted location: Correctly denied with 405"
	else
		print_fail "POST on restricted location: Expected 405, got $response"
	fi
	
	print_test "Testing DELETE method on restricted /readonly/ location (should be denied)"
	response=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE http://localhost:8081/readonly/)
	if [ "$response" = "405" ]; then
		print_pass "DELETE on restricted location: Correctly denied with 405"
	else
		print_fail "DELETE on restricted location: Expected 405, got $response"
	fi
}

# Function to test comprehensive functionality
test_comprehensive_functionality() {
	print_section "COMPREHENSIVE FUNCTIONALITY TEST"
	
	# Create test upload file
	echo "This is a test upload file for webserv configuration testing" > "$UPLOAD_FILE"
	
	# Test file upload
	print_test "Testing file upload functionality"
	response=$(curl -s -o /dev/null -w "%{http_code}" -X POST -F "file=@$UPLOAD_FILE" http://localhost:8080/upload.html)
	if [ "$response" = "200" ]; then
		print_pass "File upload: Working correctly"
	else
		print_fail "File upload: Not working (got $response)"
	fi
	
	# Test unknown method
	print_test "Testing unknown HTTP method (PATCH)"
	response=$(curl -s -o /dev/null -w "%{http_code}" -X PATCH http://localhost:8080/)
	if [ "$response" = "405" ] || [ "$response" = "501" ]; then
		print_pass "Unknown method: Correctly handled with $response"
	else
		print_fail "Unknown method: Expected 405 or 501, got $response"
	fi
	
	# Clean up
	rm -f "$UPLOAD_FILE"
}

# Function to show configuration summary
show_config_summary() {
	print_section "CONFIGURATION SUMMARY"
	
	print_info "Server Configurations:"
	print_info "  Port 8080: localhost, no body limit, all methods"
	print_info "  Port 8081: example.com, 1KB body limit, restricted /readonly/"
	print_info "  Port 8082: testsite.local, 512B body limit, custom 404"
	print_info "  Port 8083: virtual1.test, 4KB body limit, all methods"
	print_info ""
	print_info "Custom Error Pages:"
	print_info "  localhost:8080 -> custom_errors/not_found.html"
	print_info "  testsite.local:8082 -> test_errors/missing.html"
	print_info ""
	print_info "Location Configurations:"
	print_info "  /test/ -> www/test/ directory"
	print_info "  /readonly/ -> GET only (on port 8081)"
	print_info "  / -> www/ with index.html default"
}

# Main execution
main() {
	print_section "WEBSERV CONFIGURATION TESTING STARTED"
	
	# Check if webserv is running
	check_webserv_running
	
	# Show configuration summary
	show_config_summary
	
	# Run all tests
	test_status_codes
	test_multiple_ports
	test_multiple_hostnames
	test_custom_error_pages
	test_body_size_limits
	test_routes_directories
	test_default_files
	test_method_permissions
	test_comprehensive_functionality
	
	print_section "CONFIGURATION TESTING COMPLETE"
	print_info "All configuration features have been tested."
	print_info "Check the results above for any failures."
}

# Run main function
main
