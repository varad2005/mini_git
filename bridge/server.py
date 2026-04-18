import http.server
import socketserver
import subprocess
import json
import urllib.parse
import os

PORT = 8000
# Use absolute path for backend
BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BACKEND_PATH = os.path.join(BASE_DIR, "backend", "mygit")

class MyHandler(http.server.BaseHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        super().end_headers()

    def do_OPTIONS(self):
        self.send_response(200)
        self.end_headers()

    def do_GET(self):
        parsed_path = urllib.parse.urlparse(self.path)
        path = parsed_path.path

        # Serve static files from frontend directory
        if path == "/" or path == "/index.html" or path == "/style.css" or path == "/app.js":
            file_path = os.path.join(BASE_DIR, "frontend", path[1:] if path != "/" else "index.html")
            if os.path.exists(file_path):
                self.send_response(200)
                if file_path.endswith(".html"): self.send_header('Content-type', 'text/html')
                elif file_path.endswith(".css"): self.send_header('Content-type', 'text/css')
                elif file_path.endswith(".js"): self.send_header('Content-type', 'application/javascript')
                self.end_headers()
                with open(file_path, 'rb') as f:
                    self.wfile.write(f.read())
                return

        response_data = {"error": "Invalid endpoint"}
        status_code = 404

        if path == "/log":
            result = subprocess.run([BACKEND_PATH, "log"], cwd=os.path.join(BASE_DIR, "backend"), capture_output=True, text=True)
            response_data = {"output": result.stdout, "stderr": result.stderr}
            status_code = 200
        elif path == "/graph":
            result = subprocess.run([BACKEND_PATH, "graph"], cwd=os.path.join(BASE_DIR, "backend"), capture_output=True, text=True)
            response_data = {"output": result.stdout, "stderr": result.stderr}
            status_code = 200
        elif path.startswith("/diff/"):
            parts = path.split("/")
            if len(parts) >= 4:
                c1, c2 = parts[2], parts[3]
                result = subprocess.run([BACKEND_PATH, "diff", c1, c2], cwd=os.path.join(BASE_DIR, "backend"), capture_output=True, text=True)
                response_data = {"output": result.stdout, "stderr": result.stderr}
                status_code = 200
        elif path == "/init":
            result = subprocess.run([BACKEND_PATH, "init"], cwd=os.path.join(BASE_DIR, "backend"), capture_output=True, text=True)
            response_data = {"output": result.stdout, "stderr": result.stderr}
            status_code = 200
        elif path == "/status":
            result = subprocess.run([BACKEND_PATH, "status"], cwd=os.path.join(BASE_DIR, "backend"), capture_output=True, text=True)
            response_data = {"output": result.stdout, "stderr": result.stderr}
            status_code = 200

        self.send_response(status_code)
        self.send_header('Content-type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps(response_data).encode())

    def do_POST(self):
        content_length = int(self.headers.get('Content-Length', 0))
        post_data = self.rfile.read(content_length)
        data = json.loads(post_data) if post_data else {}

        parsed_path = urllib.parse.urlparse(self.path)
        path = parsed_path.path

        response_data = {"error": "Invalid endpoint"}
        status_code = 404

        args = []
        if path == "/add":
            args = ["add", data.get("filename", "")]
        elif path == "/commit":
            args = ["commit", data.get("message", "")]
        elif path == "/branch":
            args = ["branch", data.get("name", "")]
        elif path == "/checkout":
            args = ["checkout", data.get("name", "")]
        elif path == "/tag":
            args = ["tag", data.get("name", ""), data.get("commitId", "")]
            if args[2] == "": args.pop() # Remove empty commitId if not provided
        elif path == "/stash":
            args = ["stash", data.get("action", "push")]
        elif path == "/reset":
            args = ["reset", data.get("commitId", ""), "--hard" if data.get("hard", True) else "--soft"]

        if args:
            result = subprocess.run([BACKEND_PATH] + args, cwd=os.path.join(BASE_DIR, "backend"), capture_output=True, text=True)
            response_data = {"output": result.stdout, "stderr": result.stderr}
            status_code = 200

        self.send_response(status_code)
        self.send_header('Content-type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps(response_data).encode())

if __name__ == "__main__":
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("", PORT), MyHandler) as httpd:
        print(f"Server running on http://localhost:{PORT}")
        httpd.serve_forever()
