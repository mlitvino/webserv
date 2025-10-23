#!/usr/bin/env python3
import sys
# Prevent creation of __pycache__ and .pyc files
sys.dont_write_bytecode = True

import os
import datetime
import tempfile
import uuid
import cgi
from urllib.parse import parse_qs

sys.stdout.write("Content-Type: text/html\r\n\r\n")

print("""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>File Upload CGI - Webserv</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 800px; margin: 0 auto; padding: 20px; background-color: #f5f5f5; }
        .container { background-color: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        h1 { color: #333; border-bottom: 2px solid #007acc; padding-bottom: 10px; }
        .section { background-color: #f8f9fa; padding: 15px; margin: 15px 0; border-left: 4px solid #007acc; border-radius: 5px; }
        .success { background-color: #d4edda; border-left-color: #28a745; color: #155724; }
        .error { background-color: #f8d7da; border-left-color: #dc3545; color: #721c24; }
        .nav { background-color: #007acc; padding: 15px; border-radius: 5px; margin: 20px 0; }
        .nav a { color: white; text-decoration: none; margin-right: 20px; padding: 5px 10px; border-radius: 3px; }
        .nav a:hover { background-color: #005a8c; }
        .upload-form { margin: 20px 0; }
        .form-group { margin-bottom: 15px; }
        label { display: block; margin-bottom: 5px; font-weight: bold; }
        input[type="file"], input[type="text"], textarea { width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px; }
        button { background-color: #007acc; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; }
        button:hover { background-color: #005a8c; }
        pre { background-color: #f1f3f4; padding: 10px; border-radius: 5px; overflow-x: auto; }
    </style>
</head>
<body>
    <div class="container">
        <nav class="nav">
            <a href="/">Home</a>
            <a href="/cgi.html">CGI Demo</a>
            <a href="/cgi-bin/upload.py">File Upload</a>
            <a href="/upload.html">Upload</a>
        </nav>

        <h1>📁 File Upload CGI</h1>
        <p>This CGI script demonstrates file upload functionality with proper multipart/form-data handling.</p>
""")

request_method = os.environ.get('REQUEST_METHOD', '')

if request_method == 'POST':
    try:
        # Parse the request while keeping FieldStorage alive during processing
        form = cgi.FieldStorage(fp=sys.stdin.buffer, environ=os.environ, keep_blank_values=True)

        # Resolve upload directory from environment, fallback to project default
        upload_dir = os.environ.get('UPLOAD_DIR', 'web/upload')
        try:
            os.makedirs(upload_dir, exist_ok=True)
        except Exception as e:
            print('<div class="section error">')
            print('<h2>❌ Upload Error</h2>')
            print('<p>Failed to create upload directory.</p>')
            print(f'<pre>{upload_dir} -> {e}</pre>')
            print('</div>')
            form = None  # explicit to avoid lints
        else:
            saved_any = False
            invalid_found = False
            print('<div class="section success">')
            print('<h2>✅ File Upload Result</h2>')

            # Accept single or multiple file inputs under the name "file"
            items = []
            if 'file' in form:
                field = form['file']
                items = field if isinstance(field, list) else [field]

            for item in items:
                filename = getattr(item, 'filename', None)
                if not filename:
                    continue
                # Use original filename from Content-Disposition (sanitized)
                safe_filename = os.path.basename(filename)
                # Reject filenames containing space or '#'
                if (' ' in safe_filename) or ('#' in safe_filename):
                    print(f'<p class="error">❌ Invalid filename "{filename}": spaces and # characters are not allowed. Please rename your file and try again.</p>')
                    invalid_found = True
                    continue
                file_path = os.path.join(upload_dir, safe_filename)

                # Stream to disk and count bytes
                total_bytes = 0
                try:
                    with open(file_path, 'wb') as out:
                        while True:
                            chunk = item.file.read(65536)
                            if not chunk:
                                break
                            out.write(chunk)
                            total_bytes += len(chunk)

                    print(f'<p><strong>File:</strong> {filename}</p>')
                    print(f'<p><strong>Size:</strong> {total_bytes} bytes</p>')
                    print(f'<p><strong>Saved as:</strong> {safe_filename}</p>')
                    print(f'<p><strong>Upload time:</strong> {datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")}</p>')
                    saved_any = True
                except Exception as e:
                    print(f'<p class="error">Error saving file {filename}: {e}</p>')

            if not saved_any:
                if invalid_found:
                    print('<p>No files were saved because one or more filenames contain invalid characters (spaces or #). Please rename your files and try again.</p>')
                else:
                    print('<p>No files were found in the upload request.</p>')
            print('</div>')

    except Exception as e:
        print('<div class="section error">')
        print('<h2>❌ Upload Error</h2>')
        print(f'<p>Error processing upload: {str(e)}</p>')
        print('</div>')

try:
    import shutil
    pycache_dir = os.path.join(os.path.dirname(__file__), "__pycache__")
    if os.path.isdir(pycache_dir):
        shutil.rmtree(pycache_dir, ignore_errors=True)
except Exception:
    pass

print(f"""
        <div class="section">
            <h2>📊 Request Information</h2>
            <p><strong>Request Method:</strong> {request_method}</p>
            <p><strong>Content Type:</strong> {os.environ.get('CONTENT_TYPE', 'Not specified')}</p>
            <p><strong>Content Length:</strong> {os.environ.get('CONTENT_LENGTH', '0')} bytes</p>
            <p><strong>Server Time:</strong> {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>
        </div>

        <div class="section">
            <h2>📤 Upload File</h2>
            <form method="POST" action="/cgi-bin/upload.py" enctype="multipart/form-data" class="upload-form">
                <div class="form-group">
                    <label for="file">Select File:</label>
                    <input type="file" id="file" name="file" required>
                </div>

                <button type="submit">Upload File</button>
            </form>
        </div>

        <div class="section">
            <h2>🔧 CGI Upload Features</h2>
            <ul>
                <li>✅ Multipart/form-data parsing</li>
                <li>✅ File content extraction</li>
                <li>✅ File size reporting</li>
                <li>✅ Upload directory management</li>
            </ul>
        </div>

        <div class="section">
            <h2>📋 Technical Details</h2>
            <p>This CGI script demonstrates:</p>
            <ul>
                <li><strong>Multipart Form Data:</strong> Proper parsing of multipart/form-data requests</li>
                <li><strong>File Handling:</strong> Binary and text file processing</li>
                <li><strong>Path Management:</strong> Correct directory handling for file operations</li>
                <li><strong>CGI Environment:</strong> Full use of CGI environment variables</li>
            </ul>
        </div>

        <p><a href="/">← Back to Home</a></p>
    </div>
</body>
</html>""")
