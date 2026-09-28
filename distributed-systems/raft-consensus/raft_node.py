#!/usr/bin/env python3
"""
raft_node.py -- ONE node of a Raft cluster, run as its own OS process.

Implements (following the Raft paper, Ongaro & Ousterhout 2014):
  * Leader election      (RequestVote RPC, randomized election timeouts, terms)
  * Log replication      (AppendEntries RPC, consistency check, conflict repair)
  * Commit rule          (majority replication AND entry from the leader's own term)
  * Persistence          (currentTerm, votedFor, log -> fsync'd JSON file)
  * A no-op entry at the start of every leadership (paper section 8)
  * A tiny key/value state machine

Deliberately NOT implemented (named honestly in the accompanying doc):
  membership changes, log compaction/snapshots, PreVote, CheckQuorum,
  linearizable reads.

Wire format: one JSON object per line, one request + one reply per TCP
connection -- the same "one short-lived connection per call" shape as the
earlier no-pool RPC demos, kept simple on purpose.

Extra, demo-only "admin" messages let the driver ask for status and
simulate a network partition (drop all traffic to/from chosen peers).
"""
import argparse
import json
import os
import random
import socket
import threading
import time

HEARTBEAT_S = 0.10           # leader -> followers, much shorter than election timeout
ELECTION_MIN_S = 0.50        # randomized per node, per reset
ELECTION_MAX_S = 1.00
RPC_TIMEOUT_S = 0.30
COMMIT_WAIT_S = 3.0          # how long a client write waits for commit


def fmt_entry(e):
    if e["cmd"] is None:
        return "t%d:noop" % e["term"]
    return "t%d:%s=%s" % (e["term"], e["cmd"][0], e["cmd"][1])


class RaftNode:
    def __init__(self, nid, ports, base_dir):
        self.id = nid
        self.ports = ports
        self.peers = sorted(p for p in ports if p != nid)
        self.n = len(ports)
        self.lock = threading.RLock()

        os.makedirs(os.path.join(base_dir, "data"), exist_ok=True)
        os.makedirs(os.path.join(base_dir, "logs"), exist_ok=True)
        self.data_path = os.path.join(base_dir, "data", "node%d.json" % nid)
        self.log_path = os.path.join(base_dir, "logs", "node%d.log" % nid)

        # ---- persistent state (must survive a crash) ----
        self.current_term = 0
        self.voted_for = None
        self.log = []                 # entries: {"term": t, "cmd": [k, v] or None}; index is 1-based

        # ---- volatile state ----
        self.state = "follower"
        self.commit_index = 0
        self.last_applied = 0
        self.kv = {}
        self.leader_id = None
        self.next_index = {}
        self.match_index = {}
        self.inflight = {}
        self.blocked = set()          # simulated partition: peers we drop traffic with
        self.next_heartbeat = 0.0

        loaded = self.load()
        self.reset_election_deadline()
        self.event("started as FOLLOWER, term=%d, log_len=%d%s" %
                   (self.current_term, len(self.log), " (recovered from disk)" if loaded else ""))

    # ------------------------------------------------------------ helpers
    def event(self, msg):
        t = time.time()
        ts = time.strftime("%H:%M:%S", time.localtime(t)) + ".%03d" % int((t * 1000) % 1000)
        with open(self.log_path, "a") as f:
            f.write("%s [N%d] %s\n" % (ts, self.id, msg))

    def persist(self):
        tmp = self.data_path + ".tmp"
        with open(tmp, "w") as f:
            json.dump({"current_term": self.current_term,
                       "voted_for": self.voted_for,
                       "log": self.log}, f)
            f.flush()
            os.fsync(f.fileno())      # Raft requires durability BEFORE replying
        os.replace(tmp, self.data_path)

    def load(self):
        if not os.path.exists(self.data_path):
            return False
        with open(self.data_path) as f:
            d = json.load(f)
        self.current_term = d["current_term"]
        self.voted_for = d["voted_for"]
        self.log = d["log"]
        return True

    def last_log_index(self):
        return len(self.log)

    def last_log_term(self):
        return self.log[-1]["term"] if self.log else 0

    def term_at(self, i):
        return 0 if i == 0 else self.log[i - 1]["term"]

    def reset_election_deadline(self):
        # monotonic clock: immune to wall-clock jumps (see the clock-skew discussion in the RPC docs)
        self.election_deadline = time.monotonic() + random.uniform(ELECTION_MIN_S, ELECTION_MAX_S)

    # ------------------------------------------------------- role changes
    def become_follower(self, term, why):
        with self.lock:
            term_changed = term > self.current_term
            if term_changed:
                self.current_term = term
                self.voted_for = None
            if self.state != "follower" or term_changed:
                self.event("-> FOLLOWER (term %d): %s" % (self.current_term, why))
            self.state = "follower"
            if term_changed:
                self.persist()

    def start_election(self):                      # lock held by caller
        self.current_term += 1
        self.state = "candidate"
        self.voted_for = self.id                   # vote for myself
        self.leader_id = None
        self.persist()
        self.reset_election_deadline()
        term = self.current_term
        lli, llt = self.last_log_index(), self.last_log_term()
        self.event("ELECTION TIMEOUT -> CANDIDATE, term %d (my last log: idx=%d term=%d)" % (term, lli, llt))
        votes = {"n": 1}
        for p in self.peers:
            threading.Thread(target=self._request_vote, args=(p, term, lli, llt, votes), daemon=True).start()

    def _request_vote(self, p, term, lli, llt, votes):
        resp = self.rpc(p, {"type": "request_vote", "from": self.id, "term": term,
                            "candidate": self.id, "last_log_index": lli, "last_log_term": llt})
        if resp is None:
            return
        with self.lock:
            if resp["term"] > self.current_term:
                self.become_follower(resp["term"], "N%d replied with higher term %d" % (p, resp["term"]))
                return
            if self.state != "candidate" or self.current_term != term:
                return
            if resp["granted"]:
                votes["n"] += 1
                self.event("vote GRANTED by N%d (%d/%d votes, need %d)" % (p, votes["n"], self.n, self.n // 2 + 1))
                if votes["n"] > self.n // 2:
                    self.become_leader()

    def become_leader(self):                       # lock held by caller
        self.state = "leader"
        self.leader_id = self.id
        for p in self.peers:
            self.next_index[p] = self.last_log_index() + 1
            self.match_index[p] = 0
            self.inflight[p] = False
        # No-op entry: lets the new leader commit older-term entries promptly (paper section 8)
        self.log.append({"term": self.current_term, "cmd": None})
        self.persist()
        self.next_heartbeat = 0.0
        self.event("*** became LEADER for term %d *** (appended no-op at idx %d)" %
                   (self.current_term, self.last_log_index()))

    # ------------------------------------------------------------ ticking
    def ticker(self):
        while True:
            time.sleep(0.02)
            with self.lock:
                now = time.monotonic()
                if self.state == "leader":
                    if now >= self.next_heartbeat:
                        self.next_heartbeat = now + HEARTBEAT_S
                        for p in self.peers:
                            if not self.inflight.get(p) and p not in self.blocked:
                                self.inflight[p] = True
                                threading.Thread(target=self.replicate_to, args=(p,), daemon=True).start()
                elif now >= self.election_deadline:
                    self.start_election()

    # -------------------------------------------------- leader replication
    def replicate_to(self, p):
        try:
            with self.lock:
                if self.state != "leader":
                    return
                term = self.current_term
                ni = self.next_index[p]
                pli = ni - 1
                plt = self.term_at(pli)
                entries = list(self.log[ni - 1: ni - 1 + 50])
                commit = self.commit_index
            resp = self.rpc(p, {"type": "append_entries", "from": self.id, "term": term,
                                "leader": self.id, "prev_log_index": pli, "prev_log_term": plt,
                                "entries": entries, "leader_commit": commit})
            if resp is None:
                return
            with self.lock:
                if resp["term"] > self.current_term:
                    self.become_follower(resp["term"], "N%d replied with higher term %d (I was a stale leader)" %
                                         (p, resp["term"]))
                    return
                if self.state != "leader" or self.current_term != term:
                    return
                if resp["success"]:
                    self.match_index[p] = max(self.match_index[p], pli + len(entries))
                    self.next_index[p] = self.match_index[p] + 1
                    self.advance_commit()
                else:
                    hint = resp.get("conflict_index", ni - 1)
                    self.next_index[p] = max(1, min(ni - 1, hint))
        finally:
            with self.lock:
                self.inflight[p] = False

    def advance_commit(self):                      # lock held by caller
        for N in range(self.last_log_index(), self.commit_index, -1):
            if self.log[N - 1]["term"] != self.current_term:
                break          # never commit older-term entries by counting replicas (paper figure 8)
            count = 1 + sum(1 for p in self.peers if self.match_index.get(p, 0) >= N)
            if count > self.n // 2:
                self.commit_index = N
                self.event("COMMIT idx=%d (%d/%d replicas)" % (N, count, self.n))
                self.apply_committed()
                break

    def apply_committed(self):                     # lock held by caller
        while self.last_applied < self.commit_index:
            self.last_applied += 1
            e = self.log[self.last_applied - 1]
            if e["cmd"] is not None:
                self.kv[e["cmd"][0]] = e["cmd"][1]
                self.event("APPLY idx=%d %s=%s" % (self.last_applied, e["cmd"][0], e["cmd"][1]))

    # ------------------------------------------------------------ RPC out
    def rpc(self, peer, msg):
        if peer in self.blocked:
            return None
        try:
            s = socket.create_connection(("127.0.0.1", self.ports[peer]), timeout=RPC_TIMEOUT_S)
            s.settimeout(RPC_TIMEOUT_S)
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

    # ---------------------------------------------------------- RPC in
    def on_request_vote(self, m):
        with self.lock:
            if m["term"] > self.current_term:
                self.become_follower(m["term"], "saw higher term %d in RequestVote from N%d" % (m["term"], m["candidate"]))
            granted = False
            if m["term"] == self.current_term and self.voted_for in (None, m["candidate"]):
                # Election restriction: candidate's log must be at least as up-to-date as mine
                if (m["last_log_term"], m["last_log_index"]) >= (self.last_log_term(), self.last_log_index()):
                    granted = True
                    self.voted_for = m["candidate"]
                    self.persist()
                    self.reset_election_deadline()
            self.event("RequestVote from N%d term %d -> %s" %
                       (m["candidate"], m["term"], "GRANTED" if granted else "DENIED"))
            return {"term": self.current_term, "granted": granted}

    def on_append_entries(self, m):
        with self.lock:
            if m["term"] < self.current_term:
                return {"term": self.current_term, "success": False}
            if m["term"] > self.current_term or self.state != "follower":
                self.become_follower(m["term"], "AppendEntries from leader N%d (term %d)" % (m["leader"], m["term"]))
            self.leader_id = m["leader"]
            self.reset_election_deadline()

            pli, plt = m["prev_log_index"], m["prev_log_term"]
            if pli > self.last_log_index():
                return {"term": self.current_term, "success": False, "conflict_index": self.last_log_index() + 1}
            if self.term_at(pli) != plt:
                return {"term": self.current_term, "success": False, "conflict_index": pli}

            idx = pli
            changed = False
            for e in m["entries"]:
                idx += 1
                if idx <= self.last_log_index():
                    if self.log[idx - 1]["term"] != e["term"]:
                        dropped = [fmt_entry(x) for x in self.log[idx - 1:]]
                        self.event("LOG CONFLICT at idx %d: truncating my entries [%s] to match leader N%d" %
                                   (idx, " ".join(dropped), m["leader"]))
                        del self.log[idx - 1:]
                        self.log.append(e)
                        changed = True
                else:
                    self.log.append(e)
                    changed = True
            if changed:
                self.persist()
            if m["leader_commit"] > self.commit_index:
                self.commit_index = min(m["leader_commit"], pli + len(m["entries"]))
                self.apply_committed()
            return {"term": self.current_term, "success": True}

    def on_client_set(self, key, value):
        with self.lock:
            if self.state != "leader":
                return {"ok": False, "error": "not_leader", "leader": self.leader_id}
            term = self.current_term
            self.log.append({"term": term, "cmd": [key, value]})
            self.persist()
            idx = self.last_log_index()
            self.event("client SET %s=%s appended at idx %d (term %d), waiting for majority" % (key, value, idx, term))
        deadline = time.monotonic() + COMMIT_WAIT_S
        while time.monotonic() < deadline:
            with self.lock:
                if self.current_term != term or self.state != "leader":
                    return {"ok": False, "error": "lost_leadership_outcome_unknown", "index": idx}
                if self.commit_index >= idx:
                    return {"ok": True, "index": idx, "term": term}
            time.sleep(0.02)
        return {"ok": False, "error": "timeout_not_committed", "index": idx}

    def on_admin(self, m):
        with self.lock:
            if m["cmd"] == "status":
                return {"id": self.id, "state": self.state, "term": self.current_term,
                        "leader": self.leader_id, "commit_index": self.commit_index,
                        "kv": dict(self.kv), "log": [fmt_entry(e) for e in self.log],
                        "voted_for": self.voted_for, "blocked": sorted(self.blocked)}
            if m["cmd"] == "block":
                self.blocked = set(m["peers"])
                self.event("!!! PARTITION: dropping all traffic with %s" % sorted(self.blocked))
                return {"ok": True}
            if m["cmd"] == "heal":
                self.blocked = set()
                self.event("!!! HEAL: network restored")
                return {"ok": True}
        return {"ok": False}

    def dispatch(self, m):
        t = m.get("type")
        if t in ("request_vote", "append_entries") and m.get("from") in self.blocked:
            return None                                   # simulated packet loss: no reply at all
        if t == "request_vote":
            return self.on_request_vote(m)
        if t == "append_entries":
            return self.on_append_entries(m)
        if t == "client":
            return self.on_client_set(m["key"], m["value"])
        if t == "admin":
            return self.on_admin(m)
        return None

    # ---------------------------------------------------------- server
    def handle_conn(self, conn):
        try:
            f = conn.makefile("rwb")
            line = f.readline()
            if line:
                reply = self.dispatch(json.loads(line))
                if reply is not None:
                    f.write((json.dumps(reply) + "\n").encode())
                    f.flush()
        except Exception:
            pass
        finally:
            conn.close()

    def serve(self):
        srv = socket.socket()
        srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srv.bind(("127.0.0.1", self.ports[self.id]))
        srv.listen(64)
        while True:
            conn, _ = srv.accept()
            threading.Thread(target=self.handle_conn, args=(conn,), daemon=True).start()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--id", type=int, required=True)
    ap.add_argument("--cluster", required=True, help="1:7101,2:7102,...")
    ap.add_argument("--dir", default=".")
    a = ap.parse_args()
    ports = {int(x.split(":")[0]): int(x.split(":")[1]) for x in a.cluster.split(",")}
    node = RaftNode(a.id, ports, a.dir)
    threading.Thread(target=node.ticker, daemon=True).start()
    node.serve()


if __name__ == "__main__":
    main()
