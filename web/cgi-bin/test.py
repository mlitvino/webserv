#!/usr/bin/env python3

import os
import datetime
import sys

sys.stdout.write("Content-Type: text/html\r\n")
sys.stdout.write("\r\n")  # Empty line with CRLF required between headers and body

sys.stdout.write("<!DOCTYPE html>\n")
sys.stdout.write("<html>\n")
sys.stdout.write("<head>\n")
sys.stdout.write("    <title>CGI Test Script</title>\n")
sys.stdout.write("    <style>\n")
sys.stdout.write("        body { font-family: Arial, sans-serif; margin: 40px; }\n")
sys.stdout.write("        .info { background: #f0f0f0; padding: 20px; margin: 10px 0; border-radius: 5px; }\n")
sys.stdout.write("        .env-var { background: #e8f4fd; padding: 5px; margin: 2px; border-radius: 3px; }\n")
sys.stdout.write("    </style>\n")
sys.stdout.write("</head>\n")
sys.stdout.write("<body>\n")
sys.stdout.write("    <h1>🧪 CGI Test Script (test.py)</h1>\n")
sys.stdout.write("\n")
sys.stdout.write("    <div class='info'>\n")
sys.stdout.write("        <h2>🕐 Current Time</h2>\n")
sys.stdout.write(f"        <p>{datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>\n")
sys.stdout.write("    </div>\n")
sys.stdout.write("\n")
sys.stdout.write("    <div class='info'>\n")
sys.stdout.write("        <h2>🌍 Environment Variables</h2>\n")

# Print important CGI environment variables
env_vars = [
    'REQUEST_METHOD', 'CONTENT_LENGTH', 'CONTENT_TYPE', 
    'REQUEST_URI', 'QUERY_STRING', 'SERVER_PROTOCOL',
    'SCRIPT_FILENAME', 'PATH_INFO', 'GATEWAY_INTERFACE',
    'REDIRECT_STATUS', 'UPLOAD_DIR'
]

for var in env_vars:
    value = os.environ.get(var, 'Not set')
    sys.stdout.write(f'        <div class="env-var"><strong>{var}:</strong> {value}</div>\n')

sys.stdout.write("    </div>\n")
sys.stdout.write("\n")
sys.stdout.write("    <div class='info'>\n")
sys.stdout.write("        <h2>📁 Working Directory</h2>\n")
sys.stdout.write(f"        <p>{os.getcwd()}</p>\n")
sys.stdout.write("    </div>\n")
sys.stdout.write("\n")
sys.stdout.write("    <div class='info'>\n")
sys.stdout.write("        <h2>🐍 Python Version</h2>\n")
sys.stdout.write(f"        <p>{sys.version}</p>\n")
sys.stdout.write("    </div>\n")
sys.stdout.write("\n")
sys.stdout.write("</body>\n")
sys.stdout.write("</html>\n")
