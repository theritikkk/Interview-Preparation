#!/usr/bin/env python3
import subprocess
import sys
import time
from rpc_helper import call, ACCOUNT, INVENTORY, SHIPPING

PY = sys.executable
procs = []


def start(script):
    f = open(script.replace(".py", ".out"), "w")
    p = subprocess.Popen([PY, script], stdout=f, stderr=subprocess.STDOUT)
    procs.append(p)


def status():
    a = call(ACCOUNT, {"op": "status"})
    i = call(INVENTORY, {"op": "status"})
    s = call(SHIPPING, {"op": "status"})
    print("  STATE: account.balance=%d inventory.stock=%d shipping.shipments=%s" %
          (a["balance"], i["stock"], s["shipments"]))


def section(t):
    print("\n" + "#" * 78)
    print("# " + t)
    print("#" * 78)


def main():
    for s in ("account_service.py", "inventory_service.py", "shipping_service.py"):
        start(s)
    time.sleep(0.5)

    section("STARTING STATE")
    status()

    section("SCENARIO 1: 2PC, everything succeeds")
    subprocess.run([PY, "twopc_coordinator.py", "order-A", "succeed"])
    status()

    section("SCENARIO 2: 2PC, shipping will vote NO during PREPARE")
    subprocess.run([PY, "twopc_coordinator.py", "order-B", "fail"])
    print("\n  Check: did account/inventory change AT ALL from the failed attempt?")
    status()

    section("SCENARIO 3: SAGA, everything succeeds")
    subprocess.run([PY, "saga_orchestrator.py", "order-C", "succeed"])
    status()

    section("SCENARIO 4: SAGA, shipping fails AFTER account+inventory already committed")
    subprocess.run([PY, "saga_orchestrator.py", "order-D", "fail", "1"])  # qty=1: enough stock remains
    print("\n  Check: account/inventory were REALLY debited, then REALLY compensated back")
    status()

    section("cleanup")
    for p in procs:
        p.kill()
    print("  all service processes killed")


if __name__ == "__main__":
    try:
        main()
    finally:
        for p in procs:
            p.kill()
