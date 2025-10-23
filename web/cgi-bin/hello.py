#!/usr/bin/env python3

import sys

# PHP is not installed, so this is a Python version
sys.stdout.write("Content-Type: text/html\r\n")
sys.stdout.write("\r\n")  # Empty line with CRLF required between headers and body
sys.stdout.write("""<html>
<head><title>Hello CGI (PHP converted to Python)</title></head>
<body>
<h2>Hello from Python CGI! (hello.php converted)</h2>
<p><strong>Note:</strong> PHP-CGI is not installed on this system, so this script was converted to Python.</p>
</body>
</html>""")
