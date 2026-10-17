#!/usr/bin/env python3
"""A tiny backend 'service' for the mesh demo. Identifies itself (v1/v2) in
every response so traffic-split tests can count the distribution. If
FLAKY_PCT is set, fails that percentage of requests with a 503 so the
retry-policy demo has something real to recover from."""
import http.server
import os
import random
import socketserver
import sys
import time

NAME = os.environ.get("SERVICE_NAME", "unknown")
PORT = int(os.environ.get("PORT", "9001"))
FLAKY_PCT = int(os.environ.get("FLAKY_PCT", "0"))
DELAY_MS = int(os.environ.get("DELAY_MS", "0"))

class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        if DELAY_MS:
            time.sleep(DELAY_MS / 1000.0)
        if FLAKY_PCT and random.randint(1, 100) <= FLAKY_PCT:
            self.send_response(503)
            self.send_header("Content-Type", "text/plain")
            self.end_headers()
            self.wfile.write(f"{NAME}: injected failure\n".encode())
            return
        self.send_response(200)
        self.send_header("Content-Type", "text/plain")
        self.end_headers()
        self.wfile.write(f"{NAME}\n".encode())

    def log_message(self, fmt, *args):
        sys.stderr.write(f"[{NAME}] {fmt % args}\n")

class ReuseTCPServer(socketserver.ThreadingTCPServer):
    allow_reuse_address = True

if __name__ == "__main__":
    srv = ReuseTCPServer(("127.0.0.1", PORT), Handler)
    print(f"[{NAME}] listening on 127.0.0.1:{PORT} flaky={FLAKY_PCT}%")
    srv.serve_forever()
