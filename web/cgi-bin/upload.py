#!/usr/bin/env python3
import os
import sys
import datetime
import tempfile
import uuid
from urllib.parse import parse_qs

def parse_multipart_form_data():
    """Parse multipart/form-data from stdin"""
    content_type = os.environ.get('CONTENT_TYPE', '')
    if 'multipart/form-data' not in content_type:
        return {}
    
    try:
        boundary = content_type.split('boundary=')[1]
        boundary = '--' + boundary
    except:
        return {}
    
    content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    if content_length == 0:
        return {}
    
    data = sys.stdin.buffer.read(content_length)
    
    files = {}
    form_data = {}
    
    parts = data.split(boundary.encode())
    
    for part in parts:
        if b'Content-Disposition' in part:
            lines = part.split(b'\r\n')
            headers = {}
            content = b''
            
            in_headers = True
            for line in lines:
                if in_headers:
                    if line == b'':
                        in_headers = False
                        continue
                    if b':' in line:
                        key, value = line.decode().split(':', 1)
                        headers[key.lower().strip()] = value.strip()
                else:
                    if content:
                        content += b'\r\n'
                    content += line
            
            disposition = headers.get('content-disposition', '')
            if 'form-data' in disposition:
                if 'filename=' in disposition:
                    name = disposition.split('name="')[1].split('"')[0]
                    filename = disposition.split('filename="')[1].split('"')[0]
                    files[name] = {
                        'filename': filename,
                        'content': content.rstrip(b'\r\n')
                    }
                else:
                    name = disposition.split('name="')[1].split('"')[0]
                    form_data[name] = content.decode().rstrip('\r\n')
    
    return {'files': files, 'form': form_data}

print("Content-Type: text/html")
print()

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
            <a href="/cgi-bin/demo.py">CGI Demo</a>
            <a href="/cgi-bin/upload.py">File Upload</a>
            <a href="/upload.html">Upload</a>
        </nav>
        
        <h1>📁 File Upload CGI</h1>
        <p>This CGI script demonstrates file upload functionality with proper multipart/form-data handling.</p>
""")

request_method = os.environ.get('REQUEST_METHOD', '')

if request_method == 'POST':
    try:
        parsed_data = parse_multipart_form_data()
        files = parsed_data.get('files', {})
        form_data = parsed_data.get('form', {})
        
        if files:
            print('<div class="section success">')
            print('<h2>✅ File Upload Successful</h2>')
            
            for field_name, file_info in files.items():
                filename = file_info['filename']
                content = file_info['content']
                
                # Save file to upload directory
                upload_dir = '/home/riamaev/Downloads/webserv-main21-10/webserv-main/web/upload'
                if not os.path.exists(upload_dir):
                    os.makedirs(upload_dir)
                
                # Generate unique filename to prevent conflicts
                file_id = str(uuid.uuid4())[:8]
                safe_filename = f"{file_id}_{filename}"
                file_path = os.path.join(upload_dir, safe_filename)
                
                try:
                    with open(file_path, 'wb') as f:
                        f.write(content)
                    
                    print(f'<p><strong>File:</strong> {filename}</p>')
                    print(f'<p><strong>Size:</strong> {len(content)} bytes</p>')
                    print(f'<p><strong>Saved as:</strong> {safe_filename}</p>')
                    print(f'<p><strong>Upload time:</strong> {datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")}</p>')
                    
                    # Show file content preview if it's text
                    if len(content) < 1000 and filename.endswith(('.txt', '.html', '.css', '.js', '.py', '.cpp', '.c')):
                        try:
                            text_content = content.decode('utf-8')
                            print('<p><strong>Content preview:</strong></p>')
                            print(f'<pre>{text_content[:500]}{"..." if len(text_content) > 500 else ""}</pre>')
                        except:
                            print('<p>Binary file - content preview not available</p>')
                    else:
                        print('<p>Large or binary file - content preview not available</p>')
                        
                except Exception as e:
                    print(f'<p class="error">Error saving file: {str(e)}</p>')
            
            if form_data:
                print('<p><strong>Additional form data:</strong></p>')
                print('<pre>')
                for key, value in form_data.items():
                    print(f"{key}: {value}")
                print('</pre>')
            
            print('</div>')
        else:
            print('<div class="section error">')
            print('<h2>❌ No Files Received</h2>')
            print('<p>No files were found in the upload request.</p>')
            print('</div>')
            
    except Exception as e:
        print('<div class="section error">')
        print('<h2>❌ Upload Error</h2>')
        print(f'<p>Error processing upload: {str(e)}</p>')
        print('</div>')

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
                
                <div class="form-group">
                    <label for="description">Description (optional):</label>
                    <textarea id="description" name="description" rows="3" placeholder="Enter a description for the file..."></textarea>
                </div>
                
                <div class="form-group">
                    <label for="uploader">Uploader Name (optional):</label>
                    <input type="text" id="uploader" name="uploader" placeholder="Your name">
                </div>
                
                <button type="submit">Upload File</button>
            </form>
        </div>

        <div class="section">
            <h2>🔧 CGI Upload Features</h2>
            <ul>
                <li>✅ Multipart/form-data parsing</li>
                <li>✅ File content extraction</li>
                <li>✅ Automatic file saving</li>
                <li>✅ Unique filename generation</li>
                <li>✅ File size reporting</li>
                <li>✅ Text file preview</li>
                <li>✅ Upload directory management</li>
                <li>✅ Error handling</li>
            </ul>
        </div>

        <div class="section">
            <h2>📋 Technical Details</h2>
            <p>This CGI script demonstrates:</p>
            <ul>
                <li><strong>Multipart Form Data:</strong> Proper parsing of multipart/form-data requests</li>
                <li><strong>File Handling:</strong> Binary and text file processing</li>
                <li><strong>Path Management:</strong> Correct directory handling for file operations</li>
                <li><strong>Security:</strong> Unique filename generation to prevent conflicts</li>
                <li><strong>CGI Environment:</strong> Full use of CGI environment variables</li>
            </ul>
        </div>

        <p><a href="/">← Back to Home</a></p>
    </div>
</body>
</html>""")
