#!/bin/bash

echo "=== FILE UPLOAD/DOWNLOAD TESTING ==="
echo ""

# Create test files with different content types
echo "Creating test files..."

# Text file
echo "Hello World!
This is a test file for webserv upload.
Line 3 with special chars: àáâãäåæç
Numbers: 1234567890
Symbols: !@#$%^&*()_+-={}[]|\\:;\"'<>?,./
End of test file." > upload_test.txt

# Binary-like file
dd if=/dev/urandom of=binary_test.bin bs=1024 count=1 2>/dev/null

# Large text file
for i in {1..100}; do
    echo "Line $i: Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua."
done > large_test.txt

# JSON file
echo '{
  "test": "file_upload",
  "server": "webserv",
  "timestamp": "'$(date -Iseconds)'",
  "data": {
    "number": 42,
    "boolean": true,
    "array": [1, 2, 3, "test"],
    "null_value": null
  }
}' > json_test.json

echo "Created test files:"
ls -la *test*

echo ""
echo "=== UPLOADING FILES ==="

echo ""
echo "1. Uploading text file:"
curl -X POST -F 'file=@upload_test.txt' -F 'description=Text file test' http://localhost:8080/upload

echo ""
echo "2. Uploading binary file:"
curl -X POST -F 'file=@binary_test.bin' -F 'description=Binary file test' http://localhost:8080/upload

echo ""
echo "3. Uploading large text file:"
curl -X POST -F 'file=@large_test.txt' -F 'description=Large text file test' http://localhost:8080/upload

echo ""
echo "4. Uploading JSON file:"
curl -X POST -F 'file=@json_test.json' -F 'description=JSON file test' http://localhost:8080/upload

echo ""
echo "=== TESTING FILE SERVING ==="

echo ""
echo "5. Attempting to download uploaded files (if implemented):"
echo "Trying to GET files from server..."

echo ""
echo "5.1 Getting existing file /test/sample.txt:"
curl -v http://localhost:8080/test/sample.txt

echo ""
echo "5.2 Getting existing file /test/data.json:"
curl -v http://localhost:8080/test/data.json

echo ""
echo "=== CLEANUP ==="
echo "Removing test files..."
rm -f upload_test.txt binary_test.bin large_test.txt json_test.json

echo ""
echo "=== FILE UPLOAD/DOWNLOAD TESTING COMPLETE ==="
