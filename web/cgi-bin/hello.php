#!/usr/bin/env python3

# PHP is not installed, so this is a Python version
print("Content-Type: text/html")
print()  # Empty line required between headers and body
print("""<html>
<head><title>Hello CGI (PHP converted to Python)</title></head>
<body>
<h2>Hello from Python CGI! (hello.php converted)</h2>
<p><strong>Note:</strong> PHP-CGI is not installed on this system, so this script was converted to Python.</p>
</body>
</html>""")
