#!/usr/bin/env python3
"""
shipping_service.py -- same dual pattern, but this is the service
we'll deliberately make FAIL on command (simulating a real carrier
API being down / an invalid address), via an explicit "fail" flag in
the request -- so the SAME running process can demonstrate both the
happy path and the failure path, under the orchestrators' control.
"""
import json
import socket

PORT = 8103
shipments = {}


def log(msg):
    print("[shipping] %s" % msg, flush=True)


def handle(msg):
    op = msg["op"]
    order = msg.get("order_id")
    fail = msg.get("fail", False)

    if op == "prepare_ship":
        if fail:
            log("PREPARE ship order=%s: NO -- carrier API unreachable (simulated)" % order)
            return {"vote": "no"}
        log("PREPARE ship order=%s: OK (address validated, carrier slot held)" % order)
        return {"vote": "yes"}
    if op == "commit_ship":
        shipments[order] = "shipped"
        log("COMMIT ship order=%s -> SHIPPED" % order)
        return {"ok": True}
    if op == "abort_ship":
        log("ABORT ship order=%s -> no shipment created" % order)
        return {"ok": True}
    if op == "ship":
        if fail:
            log("SAGA ship order=%s FAILED -- carrier API unreachable (simulated)" % order)
            return {"ok": False}
        shipments[order] = "shipped"
        log("SAGA ship order=%s (IMMEDIATE) -> SHIPPED" % order)
        return {"ok": True}
    if op == "compensate_ship":
        shipments.pop(order, None)
        log("COMPENSATE ship order=%s -> shipment cancelled" % order)
        return {"ok": True}
    if op == "status":
        return {"shipments": dict(shipments)}
    return {"error": "unknown op"}


def serve():
    srv = socket.socket()
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(("127.0.0.1", PORT))
    srv.listen(32)
    log("listening on %d" % PORT)
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
