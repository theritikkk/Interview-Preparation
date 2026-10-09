#!/usr/bin/env python3
"""
saga_orchestrator.py -- the alternative to 2PC: each step commits
REAL state IMMEDIATELY, one after another. There is no tentative
"reservation" phase and no participant is ever blocked waiting on
another. If a LATER step fails, the orchestrator runs COMPENSATING
actions for every EARLIER step that already succeeded, in REVERSE
order -- undoing them, rather than having prevented them in advance.

The trade-off versus 2PC, made concrete by this exact demo: no
participant is ever blocked holding a reservation (better
throughput, better availability if one service is slow) -- but for
a real window of time, the system is in a state where account and
inventory HAVE been debited/reserved even though the order will
ultimately fail. A concurrent read during that window sees a state
that will later be undone -- 2PC never allows that to be observable.
"""
import sys
from rpc_helper import call, ACCOUNT, INVENTORY, SHIPPING


def run(order_id, amount, qty=2, ship_fail=False):
    print("\n=== SAGA TRANSACTION order=%s (amount=%d, qty=%d, ship_fail=%s) ===" %
          (order_id, amount, qty, ship_fail))

    completed = []   # (name, port, compensate_msg) for each step that actually succeeded

    print("--- STEP 1: debit account (REAL, immediate) ---")
    r = call(ACCOUNT, {"op": "debit", "amount": amount})
    print("  orchestrator -> account   debit -> %s" % r)
    if not r["ok"]:
        print("RESULT: saga FAILED at step 1, nothing to compensate.")
        return
    completed.append(("account", ACCOUNT, {"op": "compensate_debit", "amount": amount}))

    print("--- STEP 2: reserve inventory (REAL, immediate) ---")
    r = call(INVENTORY, {"op": "reserve", "qty": qty})
    print("  orchestrator -> inventory reserve -> %s" % r)
    if not r["ok"]:
        print("--- STEP 2 FAILED -> running compensations for completed steps, in REVERSE order ---")
        compensate(completed)
        print("RESULT: saga FAILED, but all prior real effects were undone.")
        return
    completed.append(("inventory", INVENTORY, {"op": "compensate_reserve", "qty": qty}))

    print("--- STEP 3: ship order (REAL, immediate) ---")
    r = call(SHIPPING, {"op": "ship", "order_id": order_id, "fail": ship_fail})
    print("  orchestrator -> shipping  ship -> %s" % r)
    if not r["ok"]:
        print("--- STEP 3 FAILED -> running compensations for completed steps, in REVERSE order ---")
        compensate(completed)
        print("RESULT: saga FAILED, but all prior real effects were undone.")
        return

    print("RESULT: saga COMPLETED, all three steps committed immediately as they ran.")


def compensate(completed):
    for name, port, msg in reversed(completed):
        r = call(port, msg)
        print("  orchestrator -> %-9s %s -> %s" % (name, msg["op"], r))


if __name__ == "__main__":
    order_id = sys.argv[1]
    ship_fail = sys.argv[2] == "fail"
    qty = int(sys.argv[3]) if len(sys.argv) > 3 else 2
    run(order_id, amount=30, qty=qty, ship_fail=ship_fail)
