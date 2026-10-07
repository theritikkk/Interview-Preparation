#!/usr/bin/env python3
"""inventory_service.py -- same dual 2PC/Saga pattern as account_service.py, on a stock count."""
import json
import socket
import threading

PORT = 8102
stock = 5
reserved = 0
lock = threading.Lock()


def log(msg):
    print("[inventory] %s" % msg, flush=True)


def handle(msg):
    global stock, reserved
    op = msg["op"]
    with lock:
        if op == "prepare_reserve":
            qty = msg["qty"]
            if stock - reserved >= qty:
                reserved += qty
                log("PREPARE reserve %d: OK (stock=%d, now reserved=%d)" % (qty, stock, reserved))
                return {"vote": "yes"}
            log("PREPARE reserve %d: NO -- insufficient stock (stock=%d, reserved=%d)" % (qty, stock, reserved))
            return {"vote": "no"}
        if op == "commit_reserve":
            qty = msg["qty"]
            stock -= qty
            reserved -= qty
            log("COMMIT reserve %d -> stock now %d" % (qty, stock))
            return {"ok": True}
        if op == "abort_reserve":
            qty = msg["qty"]
            reserved -= qty
            log("ABORT reserve %d -> reservation released, stock still %d" % (qty, stock))
            return {"ok": True}
        if op == "reserve":
            qty = msg["qty"]
            if stock >= qty:
                stock -= qty
                log("SAGA reserve %d (IMMEDIATE) -> stock now %d" % (qty, stock))
                return {"ok": True}
            log("SAGA reserve %d FAILED -- insufficient stock (stock=%d)" % (qty, stock))
            return {"ok": False}
        if op == "compensate_reserve":
            qty = msg["qty"]
            stock += qty
            log("COMPENSATE reserve %d -> releasing back, stock now %d" % (qty, stock))
            return {"ok": True}
        if op == "status":
            return {"stock": stock, "reserved": reserved}
    return {"error": "unknown op"}


def serve():
    srv = socket.socket()
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(("127.0.0.1", PORT))
    srv.listen(32)
    log("listening on %d, starting stock=%d" % (PORT, stock))
    while True:
        conn, _ = srv.accept()
        data = b""
        while not data.endswith(b"\n"):
            data += conn.recv(4096)
        reply = handle(json.loads(data))
        conn.sendall((json.dumps(reply) + "\n").encode())
        conn.close()


if __name__ == "__main__":
    serve()
