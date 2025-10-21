#!/usr/bin/env python3
import os
import sys
import datetime
import json
from urllib.parse import parse_qs

print("Content-Type: text/html")
print()

print("""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>CGI Demo - Webserv</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 800px; margin: 0 auto; padding: 20px; background-color: #f5f5f5; }
        .container { background-color: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        h1 { color: #333; border-bottom: 2px solid #007acc; padding-bottom: 10px; }
        .section { background-color: #f8f9fa; padding: 15px; margin: 15px 0; border-left: 4px solid #007acc; border-radius: 5px; }
        .env-var { font-family: monospace; background-color: #e9ecef; padding: 2px 5px; border-radius: 3px; }
        pre { background-color: #f1f3f4; padding: 10px; border-radius: 5px; overflow-x: auto; }
        .nav { background-color: #007acc; padding: 15px; border-radius: 5px; margin: 20px 0; }
        .nav a { color: white; text-decoration: none; margin-right: 20px; padding: 5px 10px; border-radius: 3px; }
        .nav a:hover { background-color: #005a8c; }
    </style>
</head>
<body>
    <div class="container">
        <nav class="nav">
            <a href="/">Home</a>
            <a href="/cgi-bin/demo.py">CGI Demo</a>
            <a href="/upload.html">Upload</a>
        </nav>
        
        <h1>🐍 CGI Demonstration</h1>
        <p>This page demonstrates Common Gateway Interface (CGI) functionality using Python.</p>
""")

print(f"""
        <div class="section">
            <h2>📊 Server Information</h2>
            <p><strong>Server Time:</strong> {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>
            <p><strong>CGI Script:</strong> {os.environ.get('SCRIPT_NAME', 'Unknown')}</p>
            <p><strong>Request Method:</strong> {os.environ.get('REQUEST_METHOD', 'Unknown')}</p>
            <p><strong>Query String:</strong> {os.environ.get('QUERY_STRING', 'None')}</p>
            <p><strong>Remote Address:</strong> {os.environ.get('REMOTE_ADDR', 'Unknown')}</p>
        </div>
""")

print("""
        <div class="section">
            <h2>🔧 CGI Environment Variables</h2>
            <p>These environment variables are set by the web server:</p>
            <pre>""")

important_vars = [
    'REQUEST_METHOD', 'SCRIPT_NAME', 'QUERY_STRING', 'CONTENT_TYPE',
    'CONTENT_LENGTH', 'SERVER_NAME', 'SERVER_PORT', 'REMOTE_ADDR',
    'HTTP_HOST', 'HTTP_USER_AGENT', 'PATH_INFO'
]

for var in sorted(os.environ.keys()):
    if var in important_vars or var.startswith('HTTP_'):
        print(f"{var} = {os.environ[var]}")

print("""</pre>
        </div>""")

request_method = os.environ.get('REQUEST_METHOD', '')
content_length = os.environ.get('CONTENT_LENGTH', '')

if request_method == 'POST' and content_length:
    try:
        content_length = int(content_length)
        post_data = sys.stdin.read(content_length)
        
        print("""
        <div class="section">
            <h2>📥 POST Data Received</h2>
            <p>The following data was sent via POST request:</p>
            <pre>""")
        
        print(f"Raw POST data: {post_data}")
        
        if post_data:
            try:
                parsed_data = parse_qs(post_data)
                print(f"Parsed data: {parsed_data}")
            except:
                print("Could not parse POST data as form data")
        
        print("""</pre>
        </div>""")
    except:
        print("""
        <div class="section">
            <h2>⚠️ POST Data Error</h2>
            <p>Error reading POST data</p>
        </div>""")

query_string = os.environ.get('QUERY_STRING', '')
if query_string:
    print("""
        <div class="section">
            <h2>🔍 Query Parameters</h2>
            <p>Parameters from the URL query string:</p>
            <pre>""")
    
    try:
        params = parse_qs(query_string)
        for key, values in params.items():
            print(f"{key}: {values}")
    except:
        print(f"Raw query string: {query_string}")
    
    print("""</pre>
        </div>""")

print("""
        <div class="section">
            <h2>🧪 Test CGI Functionality</h2>
            <p>Test different CGI features:</p>
            
            <h3>GET Request with Parameters</h3>
            <form method="GET" action="/cgi-bin/demo.py">
                <input type="text" name="test_param" placeholder="Enter test value" />
                <input type="submit" value="Send GET Request" />
            </form>
            
            <h3>POST Request with Data</h3>
            <form method="POST" action="/cgi-bin/demo.py">
                <textarea name="post_data" placeholder="Enter POST data here..." rows="3" cols="50"></textarea><br>
                <input type="text" name="name" placeholder="Your name" />
                <input type="submit" value="Send POST Request" />
            </form>
        </div>

        <div class="section">
            <h2>📋 CGI Technical Information</h2>
            <ul>
                <li><strong>Script Language:</strong> Python 3</li>
                <li><strong>CGI Version:</strong> Common Gateway Interface 1.1</li>
                <li><strong>File Uploads:</strong> Supported via POST requests</li>
                <li><strong>Environment Variables:</strong> Full CGI environment available</li>
                <li><strong>Standard Input:</strong> POST data reading supported</li>
                <li><strong>Standard Output:</strong> HTML generation with proper headers</li>
            </ul>
        </div>

        <div class="section">
            <h2>🔗 CGI Features Demonstrated</h2>
            <ul>
                <li>✅ Environment variable access (REQUEST_METHOD, QUERY_STRING, etc.)</li>
                <li>✅ GET parameter parsing</li>
                <li>✅ POST data handling</li>
                <li>✅ HTTP header generation (Content-Type)</li>
                <li>✅ Dynamic content generation</li>
                <li>✅ Server information display</li>
                <li>✅ Form processing</li>
            </ul>
        </div>

        <p><a href="/">← Back to Home</a></p>
    </div>
</body>
</html>""")
