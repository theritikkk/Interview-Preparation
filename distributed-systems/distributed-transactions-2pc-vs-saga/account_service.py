#!/usr/bin/env python3
"""
account_service.py -- a tiny "bank account" resource, deliberately
supporting BOTH transaction styles on the SAME underlying balance, so
the two orchestrators below can be compared fairly:

  2PC style:  prepare_debit (tentatively RESERVE, don't touch balance
              yet) -> commit_debit (actually subtract) / abort_debit
              (release the reservation, balance untouched)

  Saga style: debit (subtract IMMEDIATELY, no tentative phase at all)
              -> compensate_debit (credit back, run LATER if a
              downstream step fails)
"""
import json
import socket
import threading

PORT = 8101
balance = 100
reserved = 0          # 2PC: funds tentatively held, not yet subtracted
lock = threading.Lock()


def log(msg):
    print("[account] %s" % msg, flush=True)


def handle(msg):
    global balance, reserved
    op = msg["op"]
    with lock:
        if op == "prepare_debit":
            amt = msg["amount"]
            if balance - reserved >= amt:
                reserved += amt
                log("PREPARE debit %d: OK (balance=%d, now reserved=%d)" % (amt, balance, reserved))
                return {"vote": "yes"}
            log("PREPARE debit %d: NO -- insufficient available funds (balance=%d, reserved=%d)" %
                (amt, balance, reserved))
            return {"vote": "no"}
        if op == "commit_debit":
            amt = msg["amount"]
            balance -= amt
            reserved -= amt
            log("COMMIT debit %d -> balance now %d" % (amt, balance))
            return {"ok": True}
        if op == "abort_debit":
            amt = msg["amount"]
            reserved -= amt
            log("ABORT debit %d -> reservation released, balance still %d" % (amt, balance))
            return {"ok": True}
        if op == "debit":          # saga: immediate, real, no tentative phase
            amt = msg["amount"]
            if balance >= amt:
                balance -= amt
                log("SAGA debit %d (IMMEDIATE) -> balance now %d" % (amt, balance))
                return {"ok": True}
            log("SAGA debit %d FAILED -- insufficient funds (balance=%d)" % (amt, balance))
            return {"ok": False}
        if op == "compensate_debit":
            amt = msg["amount"]
            balance += amt
            log("COMPENSATE debit %d -> crediting back, balance now %d" % (amt, balance))
            return {"ok": True}
        if op == "status":
            return {"balance": balance, "reserved": reserved}
    return {"error": "unknown op"}


def serve():
    srv = socket.socket()
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(("127.0.0.1", PORT))
    srv.listen(32)
    log("listening on %d, starting balance=%d" % (PORT, balance))
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
