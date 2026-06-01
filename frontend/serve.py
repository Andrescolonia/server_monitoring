from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer


HOST = "127.0.0.1"
PORT = 5173


if __name__ == "__main__":
    server = ThreadingHTTPServer((HOST, PORT), SimpleHTTPRequestHandler)
    print(f"Serving frontend at http://{HOST}:{PORT}")
    server.serve_forever()
