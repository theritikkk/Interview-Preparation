#!/usr/bin/env python3
"""
twopc_coordinator.py -- a real Two-Phase Commit coordinator across
three independent services (Account, Inventory, Shipping).

Phase 1 (PREPARE): ask every participant "can you do this?" Each one
  tentatively reserves the resource (does NOT mutate real state yet)
  and votes yes/no.

Phase 2a (COMMIT): if EVERY participant voted yes, tell all of them
  to commit -- now they actually mutate state. By construction, this
  cannot fail: each participant already proved during PREPARE that
  it could do this, and is holding the reservation.

Phase 2b (ABORT): if ANY participant voted no, tell EVERY participant
  (including the ones that voted yes) to abort -- release whatever
  they tentatively reserved. Real state is untouched anywhere.

The key property, worth internalizing: between phase 1 and phase 2,
every "yes" voter is BLOCKED holding its reservation, unable to use
that resource for anything else, until the coordinator decides. This
is 2PC's central cost -- it buys strong consistency by making every
participant wait on the single slowest one.
"""
import sys
from rpc_helper import call, ACCOUNT, INVENTORY, SHIPPING


def run(order_id, amount, qty, ship_fail):
    print("\n=== 2PC TRANSACTION order=%s (amount=%d, qty=%d, ship_fail=%s) ===" %
          (order_id, amount, qty, ship_fail))

    participants = [
        ("account", ACCOUNT, {"op": "prepare_debit", "amount": amount},
         {"op": "commit_debit", "amount": amount}, {"op": "abort_debit", "amount": amount}),
        ("inventory", INVENTORY, {"op": "prepare_reserve", "qty": qty},
         {"op": "commit_reserve", "qty": qty}, {"op": "abort_reserve", "qty": qty}),
        ("shipping", SHIPPING, {"op": "prepare_ship", "order_id": order_id, "fail": ship_fail},
         {"op": "commit_ship", "order_id": order_id}, {"op": "abort_ship", "order_id": order_id}),
    ]

    print("--- PHASE 1: PREPARE ---")
    votes = {}
    for name, port, prepare_msg, _, _ in participants:
        resp = call(port, prepare_msg)
        votes[name] = resp["vote"]
        print("  coordinator -> %-9s PREPARE -> vote=%s" % (name, resp["vote"]))

    all_yes = all(v == "yes" for v in votes.values())

    if all_yes:
        print("--- ALL VOTES YES -> PHASE 2: GLOBAL COMMIT ---")
        for name, port, _, commit_msg, _ in participants:
            call(port, commit_msg)
            print("  coordinator -> %-9s COMMIT" % name)
        print("RESULT: transaction COMMITTED everywhere.")
    else:
        no_voters = [n for n, v in votes.items() if v == "no"]
        print("--- %s VOTED NO -> PHASE 2: GLOBAL ABORT ---" % ",".join(no_voters))
        for name, port, _, _, abort_msg in participants:
            call(port, abort_msg)
            print("  coordinator -> %-9s ABORT (even services that voted yes release their hold)" % name)
        print("RESULT: transaction ABORTED everywhere -- real state untouched anywhere.")


if __name__ == "__main__":
    order_id = sys.argv[1]
    ship_fail = sys.argv[2] == "fail"
    run(order_id, amount=30, qty=2, ship_fail=ship_fail)
