import json
import re
import sys
from http.server import BaseHTTPRequestHandler, HTTPServer

def get_handler_class(files, endpoints):
    class Server(BaseHTTPRequestHandler):
        def do_GET(self):
            self.handle_request('GET')

        def do_POST(self):
            read = self.rfile.read(int(self.headers['content-length']))
            body = json.loads(read)
            self.handle_request('POST', body)


        def handle_request(self, method, body = None):
            try:
                if method == 'GET':
                    for file in files:
                        if file.path == self.path:                            
                            self.send_response(200)
                            self.send_header('Content-type', file.content_type)
                            self.end_headers()
                            f = open(file.name, 'r')
                            self.wfile.write(f.read().encode('utf-8'))
                            f.close()
                            return
                for endpoint in endpoints:
                    if endpoint.id == f'{method}:{self.path}':
                        response = endpoint.handle_request(body)
                        self.send_response(200)
                        response_json = json.dumps(response, allow_nan=False)
                        self.send_header('Content-type', 'application/json')
                        self.end_headers()
                        self.wfile.write(response_json.encode('utf-8'))
                        return
            except Exception as e:
                self.send_response(500) # internal server error
                self.send_header('Content-type', 'text/plain')
                self.end_headers()
                self.wfile.write(str(e).encode('utf-8'))
                raise e

    return Server

class File:
    def __init__(self, name, content_type = 'text/plain'):
        self.name = name
        self.path = f'/{name}'
        self.content_type = content_type


class SavePattern:
    id = 'POST:/save-pattern'
    def handle_request(self, body):
        with open('pattern.txt', 'w') as f:
            f.write(body['pattern'])
        return {}

pattern_editor_page = File('pattern-editor.html', 'text/html')
pattern_editor_page.path = '/'

server_address = ('', int(sys.argv[-1]))
httpd = HTTPServer(server_address, get_handler_class([pattern_editor_page], [SavePattern()]))
httpd.serve_forever()
httpd.server_close()
