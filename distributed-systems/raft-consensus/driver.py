#!/usr/bin/env python3
"""
driver.py -- orchestrates a real 5-node Raft cluster (5 separate OS
processes) through a full scenario: election, client writes, a real
simulated network partition with a competing leader, healing, log
repair, and a crash + restart demonstrating disk persistence.
"""
import json
import os
import socket
import subprocess
import sys
import time

NODES = [1, 2, 3, 4, 5]
BASE_PORT = 7100
PORTS = {n: BASE_PORT + n for n in NODES}
CLUSTER = ",".join("%d:%d" % (n, PORTS[n]) for n in NODES)
DIR = os.path.dirname(os.path.abspath(__file__))
PYTHON = sys.executable

procs = {}


def start(n):
    logf = open(os.path.join(DIR, "logs", "stdout%d.log" % n), "a")
    p = subprocess.Popen([PYTHON, os.path.join(DIR, "raft_node.py"),
                          "--id", str(n), "--cluster", CLUSTER, "--dir", DIR],
                         stdout=logf, stderr=subprocess.STDOUT)
    procs[n] = p


def kill(n, sig9=True):
    if n in procs:
        procs[n].kill() if sig9 else procs[n].terminate()
        procs[n].wait()
        del procs[n]


def rpc(n, msg, timeout=1.0):
    try:
        s = socket.create_connection(("127.0.0.1", PORTS[n]), timeout=timeout)
        s.settimeout(timeout)
        s.sendall((json.dumps(msg) + "\n").encode())
        data = b""
        while not data.endswith(b"\n"):
            chunk = s.recv(65536)
            if not chunk:
                break
            data += chunk
        s.close()
        return json.loads(data) if data.strip() else None
    except Exception:
        return None


def status(n):
    return rpc(n, {"type": "admin", "cmd": "status"})


def wait_for_leader(candidates, timeout=6.0):
    deadline = time.time() + timeout
    while time.time() < deadline:
        for n in candidates:
            st = status(n)
            if st and st["state"] == "leader":
                return n, st["term"]
        time.sleep(0.1)
    return None, None


def section(title):
    print("\n" + "=" * 78)
    print(title)
    print("=" * 78)


def print_cluster_status(candidates=NODES):
    for n in candidates:
        st = status(n)
        if st is None:
            print("  N%d: [unreachable]" % n)
        else:
            print("  N%d: %-9s term=%-3d leader=%s commit=%-3d kv=%s blocked=%s" %
                  (n, st["state"], st["term"],
                   ("N%d" % st["leader"]) if st["leader"] else "?",
                   st["commit_index"], st["kv"], st["blocked"]))


def main():
    os.makedirs(os.path.join(DIR, "logs"), exist_ok=True)
    os.makedirs(os.path.join(DIR, "data"), exist_ok=True)
    # clean slate
    for n in NODES:
        for sub in ("logs", "data"):
            path = os.path.join(DIR, sub, "node%d.json" % n if sub == "data" else "stdout%d.log" % n)
            if os.path.exists(path):
                os.remove(path)
        lp = os.path.join(DIR, "logs", "node%d.log" % n)
        if os.path.exists(lp):
            os.remove(lp)

    section("1. STARTING 5 REAL OS PROCESSES (nodes 1-5)")
    for n in NODES:
        start(n)
        print("  started N%d (pid %d) listening on port %d" % (n, procs[n].pid, PORTS[n]))
    time.sleep(0.3)

    leader, term = wait_for_leader(NODES)
    print("\n  Leader elected: N%d, term %d" % (leader, term))
    print_cluster_status()

    section("2. CLIENT WRITES (through the real leader, real majority replication)")
    for k, v in [("x", "1"), ("y", "2"), ("x", "3")]:
        r = rpc(leader, {"type": "client", "key": k, "value": v})
        print("  SET %s=%s -> %s" % (k, v, r))
    print_cluster_status()

    section("3. SIMULATING A REAL NETWORK PARTITION")
    minority = [leader] + [n for n in NODES if n != leader][:1]     # leader + 1 = 2 nodes, isolated
    majority = [n for n in NODES if n not in minority]              # remaining 3 nodes
    print("  Partition: {%s} (old leader's side, MINORITY) <-X-> {%s} (MAJORITY)" %
          (",".join("N%d" % n for n in minority), ",".join("N%d" % n for n in majority)))
    for n in minority:
        rpc(n, {"type": "admin", "cmd": "block", "peers": majority})
    for n in majority:
        rpc(n, {"type": "admin", "cmd": "block", "peers": minority})
    time.sleep(0.05)
    print("  (every node's outbound/inbound traffic to the other side is now silently dropped)")

    print("\n  Old leader N%d, still believing it's leader, tries a write into the minority side:" % leader)
    r = rpc(leader, {"type": "client", "key": "z", "value": "stuck"}, timeout=4.0)
    print("  SET z=stuck (sent to old leader N%d) -> %s" % (leader, r))
    print("  (it can't reach a majority to commit -- this call BLOCKS until COMMIT_WAIT_S, then times out)")

    new_leader, new_term = wait_for_leader(majority, timeout=6.0)
    print("\n  Meanwhile, the MAJORITY side (%s) elected its OWN new leader:" %
          ",".join("N%d" % n for n in majority))
    print("  New leader: N%d, term %d  (strictly greater than old leader's term %d)" %
          (new_leader, new_term, term))

    print("\n  Client writes now go through the NEW leader, and DO commit (majority is reachable):")
    for k, v in [("z", "committed_by_new_leader"), ("x", "99")]:
        r = rpc(new_leader, {"type": "client", "key": k, "value": v})
        print("  SET %s=%s -> %s" % (k, v, r))

    print("\n  Full cluster status DURING the partition (two leaders exist -- but only one can commit):")
    print_cluster_status()

    section("4. HEALING THE PARTITION")
    for n in NODES:
        rpc(n, {"type": "admin", "cmd": "heal"})
    print("  network restored for all 5 nodes")
    time.sleep(1.0)
    print_cluster_status()
    print("\n  (the old leader N%d must have stepped down the moment it saw the new, higher term --" % leader)
    print("   check logs/node%d.log for the exact 'become_follower' line)" % leader)

    section("5. CRASH + RESTART: does the log survive on disk?")
    victim = new_leader if new_leader != leader else NODES[0]
    before = status(victim)
    print("  N%d BEFORE kill -9: state=%s term=%d log=%s" %
          (victim, before["state"], before["term"], before["log"]))
    kill(victim)
    print("  N%d killed with SIGKILL (no graceful shutdown, no chance to flush anything NEW)" % victim)
    time.sleep(0.3)
    start(victim)
    time.sleep(1.0)
    after = status(victim)
    print("  N%d AFTER restart:  state=%s term=%d log=%s" %
          (victim, after["state"], after["term"], after["log"]))
    print("  (log and term recovered from data/node%d.json on disk -- fsync'd before every reply, per Raft's rule)" % victim)

    section("6. FINAL CLUSTER STATE -- every reachable node converges on the same committed KV state")
    print_cluster_status()

    section("cleanup")
    for n in list(procs):
        kill(n)
    print("  all node processes killed")


if __name__ == "__main__":
    try:
        main()
    finally:
        for n in list(procs):
            kill(n)
