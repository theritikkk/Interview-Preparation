#!/usr/bin/env python3
"""
Webhook receiver with the properties the Certa FDE assessment asks about:
signature verification, duplicate delivery, ordering, at-least-once delivery,
backpressure, replay, plus crash-safe persistence and a DLQ.

Assessment topic 4 (Certa FDE). Stdlib only -- no framework, no broker, so every
mechanism is visible: SQLite WAL for durability, an inbox table for dedupe, an
outbox table as the work queue, and a version guard for ordering.

Run:
    python3 webhook_receiver.py          # starts the server and runs the full scenario
    python3 webhook_receiver.py --json   # machine-readable result
"""

from __future__ import annotations

import argparse
import hashlib
import hmac
import json
import os
import random
import sqlite3
import sys
import threading
import time
from contextlib import contextmanager
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from typing import Any, Iterator, Mapping

SECRET = b"whsec_demo_do_not_hardcode_in_production"
SIGNATURE_TOLERANCE_SECONDS = 300
MAX_ATTEMPTS = 3
BACKOFF_BASE_SECONDS = 0.05  # 50ms so the demo finishes fast; production: seconds
BACKOFF_CAP_SECONDS = 2.0
MAX_QUEUE_DEPTH = 50          # beyond this we return 429 instead of accepting work
WORKER_COUNT = 2


# --------------------------------------------------------------------------- #
# Storage
# --------------------------------------------------------------------------- #
SCHEMA = """
PRAGMA journal_mode=WAL;

-- inbox: one row per delivery attempt outcome. The PRIMARY KEY is the dedupe.
CREATE TABLE IF NOT EXISTS inbox (
    event_id     TEXT PRIMARY KEY,
    event_type   TEXT NOT NULL,
    vendor_id    TEXT NOT NULL,
    version      INTEGER NOT NULL,
    payload      TEXT NOT NULL,
    payload_hash TEXT NOT NULL,
    received_at  REAL NOT NULL,
    status       TEXT NOT NULL,           -- accepted | duplicate | rejected
    http_status  INTEGER NOT NULL
);

-- outbox/queue: written in the SAME transaction as the inbox row, so an accepted
-- event can never be lost even if the process dies right after the 202.
CREATE TABLE IF NOT EXISTS jobs (
    job_id          INTEGER PRIMARY KEY AUTOINCREMENT,
    event_id        TEXT NOT NULL REFERENCES inbox(event_id),
    status          TEXT NOT NULL,        -- pending | processing | done | dead
    attempts        INTEGER NOT NULL DEFAULT 0,
    next_attempt_at REAL NOT NULL DEFAULT 0,
    last_error      TEXT,
    updated_at      REAL NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_jobs_claimable ON jobs(status, next_attempt_at);

-- business state, with the version kept for out-of-order protection
CREATE TABLE IF NOT EXISTS vendor_state (
    vendor_id  TEXT PRIMARY KEY,
    name       TEXT NOT NULL,
    tier       TEXT NOT NULL,
    version    INTEGER NOT NULL,
    updated_at REAL NOT NULL
);

CREATE TABLE IF NOT EXISTS dead_letters (
    event_id      TEXT PRIMARY KEY,
    attempts      INTEGER NOT NULL,
    last_error    TEXT NOT NULL,
    moved_at      REAL NOT NULL,
    replayed_at   REAL
);

CREATE TABLE IF NOT EXISTS audit_log (
    id       INTEGER PRIMARY KEY AUTOINCREMENT,
    event_id TEXT,
    action   TEXT NOT NULL,
    detail   TEXT,
    at       REAL NOT NULL
);

CREATE TABLE IF NOT EXISTS service_flags (key TEXT PRIMARY KEY, value TEXT NOT NULL);
"""


class Store:
    """SQLite with WAL. One connection per operation keeps it thread-safe."""

    def __init__(self, path: str) -> None:
        self.path = path
        with self._connect() as conn:
            conn.executescript(SCHEMA)

    @contextmanager
    def _connect(self) -> Iterator[sqlite3.Connection]:
        conn = sqlite3.connect(self.path, timeout=10.0, isolation_level=None)
        conn.row_factory = sqlite3.Row
        conn.execute("PRAGMA foreign_keys=ON")
        try:
            yield conn
        finally:
            conn.close()

    # --- ingest -----------------------------------------------------------
    def record_delivery(self, event: Mapping[str, Any], payload_hash: str, raw: str) -> tuple[str, int]:
        """Insert the inbox row and the queue job atomically. Returns (status, http).

        The unique key on event_id is what makes duplicate detection correct under
        concurrency: two simultaneous deliveries race on the INSERT and exactly one
        wins, rather than both passing a "does it exist?" check.
        """
        now = time.time()
        with self._connect() as conn:
            conn.execute("BEGIN IMMEDIATE")  # single writer: no interleaving
            try:
                conn.execute(
                    """INSERT INTO inbox (event_id, event_type, vendor_id, version, payload,
                                          payload_hash, received_at, status, http_status)
                       VALUES (?,?,?,?,?,?,?,?,?)""",
                    (
                        event["event_id"], event["event_type"], event["vendor_id"], event["version"],
                        raw, payload_hash, now, "accepted", 202,
                    ),
                )
                conn.execute(
                    """INSERT INTO jobs (event_id, status, attempts, next_attempt_at, updated_at)
                       VALUES (?, 'pending', 0, 0, ?)""",
                    (event["event_id"], now),
                )
                conn.execute(
                    "INSERT INTO audit_log (event_id, action, detail, at) VALUES (?,?,?,?)",
                    (event["event_id"], "accepted", None, now),
                )
                conn.execute("COMMIT")
                return "accepted", 202
            except sqlite3.IntegrityError:
                conn.execute("ROLLBACK")
                # Duplicate delivery: same event_id already durable. Compare the body
                # hash so a reused id with different content is visible, not silent.
                with self._connect() as check:
                    row = check.execute("SELECT payload_hash FROM inbox WHERE event_id = ?", (event["event_id"],)).fetchone()
                if row and row["payload_hash"] != payload_hash:
                    return "conflict_same_id_different_body", 409
                return "duplicate", 200

    def queue_depth(self) -> int:
        with self._connect() as conn:
            row = conn.execute("SELECT COUNT(*) AS n FROM jobs WHERE status IN ('pending','processing')").fetchone()
        return int(row["n"])

    # --- worker side ------------------------------------------------------
    def claim_job(self) -> sqlite3.Row | None:
        """Atomically take one due job. The UPDATE ... WHERE status='pending' guard is
        the lease: two workers cannot claim the same row."""
        now = time.time()
        with self._connect() as conn:
            conn.execute("BEGIN IMMEDIATE")
            row = conn.execute(
                """SELECT j.job_id, j.event_id, j.attempts, i.payload, i.version, i.vendor_id, i.event_type
                   FROM jobs AS j JOIN inbox AS i ON i.event_id = j.event_id
                   WHERE j.status = 'pending' AND j.next_attempt_at <= ?
                   ORDER BY j.job_id LIMIT 1""",
                (now,),
            ).fetchone()
            if row is None:
                conn.execute("COMMIT")
                return None
            updated = conn.execute(
                "UPDATE jobs SET status='processing', attempts = attempts + 1, updated_at = ? WHERE job_id = ? AND status='pending'",
                (now, row["job_id"]),
            ).rowcount
            conn.execute("COMMIT")
            return row if updated == 1 else None

    def complete_job(self, job_id: int, event_id: str) -> None:
        now = time.time()
        with self._connect() as conn:
            conn.execute("BEGIN IMMEDIATE")
            conn.execute("UPDATE jobs SET status='done', updated_at=? WHERE job_id=?", (now, job_id))
            conn.execute("UPDATE inbox SET status='processed' WHERE event_id=?", (event_id,))
            conn.execute("COMMIT")

    def fail_job(self, job_id: int, event_id: str, error: str, attempts: int) -> bool:
        """Backoff, or move to the DLQ. Returns True if dead-lettered."""
        now = time.time()
        if attempts >= MAX_ATTEMPTS:
            with self._connect() as conn:
                conn.execute("BEGIN IMMEDIATE")
                conn.execute("UPDATE jobs SET status='dead', last_error=?, updated_at=? WHERE job_id=?", (error, now, job_id))
                conn.execute(
                    """INSERT INTO dead_letters (event_id, attempts, last_error, moved_at)
                       VALUES (?,?,?,?)
                       ON CONFLICT(event_id) DO UPDATE SET attempts=excluded.attempts,
                           last_error=excluded.last_error, moved_at=excluded.moved_at""",
                    (event_id, attempts, error, now),
                )
                conn.execute("INSERT INTO audit_log (event_id, action, detail, at) VALUES (?,?,?,?)", (event_id, "dead_lettered", error, now))
                conn.execute("COMMIT")
            return True

        # Full jitter: avoids a synchronised retry stampede across workers/tenants.
        delay = min(BACKOFF_CAP_SECONDS, BACKOFF_BASE_SECONDS * (2 ** (attempts - 1)))
        delay = random.uniform(0, delay)
        with self._connect() as conn:
            conn.execute("BEGIN IMMEDIATE")
            conn.execute(
                "UPDATE jobs SET status='pending', next_attempt_at=?, last_error=?, updated_at=? WHERE job_id=?",
                (now + delay, error, now, job_id),
            )
            conn.execute("INSERT INTO audit_log (event_id, action, detail, at) VALUES (?,?,?,?)", (event_id, "retry_scheduled", f"{error} in {delay:.3f}s", now))
            conn.execute("COMMIT")
        return False

    # --- business effect --------------------------------------------------
    def apply_event(self, payload: Mapping[str, Any], event_id: str) -> str:
        """Idempotent, order-safe state transition. Raises to simulate failure."""
        if payload.get("poison") and not self.flag("poison_fixed"):
            raise RuntimeError("downstream vendor API returned 500 (simulated outage)")

        vendor_id, version = payload["vendor_id"], int(payload["version"])
        now = time.time()
        with self._connect() as conn:
            conn.execute("BEGIN IMMEDIATE")
            current = conn.execute("SELECT version FROM vendor_state WHERE vendor_id=?", (vendor_id,)).fetchone()
            if current is not None and version <= current["version"]:
                # Out-of-order or replayed event: keep the newer state, keep the audit trail.
                conn.execute(
                    "INSERT INTO audit_log (event_id, action, detail, at) VALUES (?,?,?,?)",
                    (event_id, "stale_ignored", f"incoming v{version} <= stored v{current['version']}", now),
                )
                conn.execute("COMMIT")
                return "stale_ignored"
            conn.execute(
                """INSERT INTO vendor_state (vendor_id, name, tier, version, updated_at) VALUES (?,?,?,?,?)
                   ON CONFLICT(vendor_id) DO UPDATE SET name=excluded.name, tier=excluded.tier,
                       version=excluded.version, updated_at=excluded.updated_at""",
                (vendor_id, payload["name"], payload["tier"], version, now),
            )
            conn.execute(
                "INSERT INTO audit_log (event_id, action, detail, at) VALUES (?,?,?,?)",
                (event_id, "applied", f"v{version}", now),
            )
            conn.execute("COMMIT")
        return "applied"

    # --- operator actions -------------------------------------------------
    def replay(self, event_id: str | None = None) -> int:
        """Requeue dead jobs. Idempotent by construction: apply_event guards on version,
        and the inbox primary key blocks re-ingest, so a replay can never double-apply."""
        now = time.time()
        with self._connect() as conn:
            conn.execute("BEGIN IMMEDIATE")
            if event_id:
                rows = conn.execute("SELECT job_id FROM jobs WHERE event_id=? AND status='dead'", (event_id,)).fetchall()
            else:
                rows = conn.execute("SELECT job_id FROM jobs WHERE status='dead'").fetchall()
            for row in rows:
                conn.execute(
                    "UPDATE jobs SET status='pending', attempts=0, next_attempt_at=?, last_error=NULL, updated_at=? WHERE job_id=?",
                    (now, now, row["job_id"]),
                )
            conn.execute("UPDATE dead_letters SET replayed_at=? WHERE replayed_at IS NULL", (now,))
            conn.execute("INSERT INTO audit_log (event_id, action, detail, at) VALUES (?,?,?,?)", (event_id, "replay_requested", f"{len(rows)} job(s)", now))
            conn.execute("COMMIT")
        return len(rows)

    def set_flag(self, key: str, value: bool) -> None:
        with self._connect() as conn:
            conn.execute("INSERT INTO service_flags (key, value) VALUES (?,?) ON CONFLICT(key) DO UPDATE SET value=excluded.value", (key, "1" if value else "0"))

    def flag(self, key: str) -> bool:
        with self._connect() as conn:
            row = conn.execute("SELECT value FROM service_flags WHERE key=?", (key,)).fetchone()
        return bool(row and row["value"] == "1")

    def audit(self, event_id: str | None, action: str, detail: str | None = None) -> None:
        with self._connect() as conn:
            conn.execute(
                "INSERT INTO audit_log (event_id, action, detail, at) VALUES (?,?,?,?)",
                (event_id, action, detail, time.time()),
            )

    def audit_for(self, event_id: str) -> list[dict[str, Any]]:
        with self._connect() as conn:
            return [dict(r) for r in conn.execute(
                "SELECT action, detail FROM audit_log WHERE event_id=? ORDER BY id", (event_id,)
            )]

    def metrics(self) -> dict[str, Any]:
        with self._connect() as conn:
            inbox = {r["status"]: r["n"] for r in conn.execute("SELECT status, COUNT(*) AS n FROM inbox GROUP BY status")}
            jobs = {r["status"]: r["n"] for r in conn.execute("SELECT status, COUNT(*) AS n FROM jobs GROUP BY status")}
            actions = {r["action"]: r["n"] for r in conn.execute("SELECT action, COUNT(*) AS n FROM audit_log GROUP BY action")}
            oldest = conn.execute("SELECT MIN(next_attempt_at) AS t FROM jobs WHERE status='pending'").fetchone()["t"]
            vendors = [dict(r) for r in conn.execute("SELECT vendor_id, name, tier, version FROM vendor_state ORDER BY vendor_id")]
        return {
            "inbox_by_status": inbox,
            "jobs_by_status": jobs,
            "audit_actions": actions,
            "queue_depth": sum(jobs.get(k, 0) for k in ("pending", "processing")),
            "oldest_pending_lag_seconds": round(max(0.0, time.time() - oldest), 3) if oldest else 0.0,
            "vendors": vendors,
        }


# --------------------------------------------------------------------------- #
# Signature verification
# --------------------------------------------------------------------------- #
def sign(secret: bytes, body: bytes, timestamp: int | None = None) -> str:
    """Provider-side helper: sign the timestamp and the raw body together."""
    timestamp = timestamp or int(time.time())
    mac = hmac.new(secret, f"{timestamp}.".encode() + body, hashlib.sha256).hexdigest()
    return f"t={timestamp},v1={mac}"


def verify_signature(secret: bytes, header: str, body: bytes, *, now: float | None = None, tolerance: int = SIGNATURE_TOLERANCE_SECONDS) -> tuple[bool, str]:
    """Verify over the RAW body, inside a timestamp window, in constant time.

    Returns (ok, reason). Every rejection is explicit so the 401 is diagnosable.
    """
    now = time.time() if now is None else now
    parts: dict[str, str] = {}
    for item in header.split(","):
        if "=" in item:
            key, _, value = item.partition("=")
            parts[key.strip()] = value.strip()
    if "t" not in parts or "v1" not in parts:
        return False, "malformed signature header"
    try:
        timestamp = int(parts["t"])
    except ValueError:
        return False, "non-integer timestamp"
    if abs(now - timestamp) > tolerance:
        return False, f"timestamp outside {tolerance}s tolerance (possible replay)"
    expected = hmac.new(secret, f"{timestamp}.".encode() + body, hashlib.sha256).hexdigest()
    if not hmac.compare_digest(expected, parts["v1"]):
        return False, "signature mismatch"
    return True, "ok"


# --------------------------------------------------------------------------- #
# HTTP surface
# --------------------------------------------------------------------------- #
class Api:
    def __init__(self, store: Store) -> None:
        self.store = store
        self.stop = threading.Event()

    # POST /webhooks/vendor
    def ingest(self, raw: bytes, signature_header: str) -> tuple[int, dict[str, Any]]:
        ok, reason = verify_signature(SECRET, signature_header, raw)
        if not ok:
            self.store.audit(None, "signature_rejected", reason)  # failed auth attempts are auditable
            return 401, {"error": "invalid_signature", "reason": reason}

        try:
            event = json.loads(raw.decode("utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError):
            return 400, {"error": "invalid_json"}

        missing = [k for k in ("event_id", "event_type", "vendor_id", "version") if k not in event]
        if missing:
            return 422, {"error": "validation_failed", "missing": missing}

        # Backpressure: refuse new work instead of growing an unbounded queue.
        if self.store.queue_depth() >= MAX_QUEUE_DEPTH or self.store.flag("throttled"):
            return 429, {"error": "too_many_requests", "retry_after_seconds": 5}

        payload_hash = hashlib.sha256(raw).hexdigest()
        status, http = self.store.record_delivery(event, payload_hash, json.dumps(event, sort_keys=True))
        return http, {"status": status, "event_id": event["event_id"]}

    # POST /admin/replay
    def replay(self, body: Mapping[str, Any]) -> tuple[int, dict[str, Any]]:
        count = self.store.replay(body.get("event_id"))
        return 200, {"requeued": count}


class Handler(BaseHTTPRequestHandler):
    api: Api
    quiet = True

    def _respond(self, status: int, body: Mapping[str, Any]) -> None:
        encoded = json.dumps(body).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(encoded)))
        if status == 429:
            self.send_header("Retry-After", "5")
        self.end_headers()
        self.wfile.write(encoded)

    def do_POST(self) -> None:  # noqa: N802
        length = int(self.headers.get("Content-Length") or 0)
        raw = self.rfile.read(length)
        if self.path == "/webhooks/vendor":
            status, body = self.api.ingest(raw, self.headers.get("X-Signature", ""))
        elif self.path == "/admin/replay":
            try:
                body = json.loads(raw or b"{}")
            except json.JSONDecodeError:
                status, body = 400, {"error": "invalid_json"}
                self._respond(status, body)
                return
            status, body = self.api.replay(body)
        else:
            status, body = 404, {"error": "not_found"}
        self._respond(status, body)

    def do_GET(self) -> None:  # noqa: N802
        if self.path == "/metrics":
            self._respond(200, self.api.store.metrics())
        else:
            self._respond(404, {"error": "not_found"})

    def log_message(self, *args: Any) -> None:
        if not self.quiet:
            super().log_message(*args)


# --------------------------------------------------------------------------- #
# Worker
# --------------------------------------------------------------------------- #
def worker_loop(api: Api) -> None:
    store = api.store
    while not api.stop.is_set():
        job = store.claim_job()
        if job is None:
            time.sleep(0.02)
            continue
        try:
            store.apply_event(json.loads(job["payload"]), job["event_id"])
            store.complete_job(job["job_id"], job["event_id"])
        except Exception as exc:  # noqa: BLE001 - any handler failure is a job failure
            store.fail_job(job["job_id"], job["event_id"], f"{type(exc).__name__}: {exc}", job["attempts"])


def wait_until_idle(store: Store, timeout: float = 8.0) -> bool:
    """Test helper: async assertions need a deterministic point to observe.
    Production equivalents: a metric/alarm on queue depth, or a polling sync API."""
    deadline = time.time() + timeout
    while time.time() < deadline:
        if store.queue_depth() == 0:
            time.sleep(0.05)  # let the worker finish its commit
            return True
        time.sleep(0.02)
    return False


# --------------------------------------------------------------------------- #
# Demo scenario: duplicates, tampering, ordering, failure, DLQ, replay, throttling
# --------------------------------------------------------------------------- #
def run_scenario(db_path: str) -> dict[str, Any]:
    if os.path.exists(db_path):
        os.remove(db_path)
    store = Store(db_path)
    api = Api(store)
    Handler.api = api
    server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    hub = threading.Thread(target=server.serve_forever, daemon=True)
    hub.start()
    workers = [threading.Thread(target=worker_loop, args=(api,), daemon=True) for _ in range(WORKER_COUNT)]
    for w in workers:
        w.start()

    base = f"http://127.0.0.1:{server.server_address[1]}"

    def deliver(event: Mapping[str, Any], *, tamper: bool = False, skew: int = 0) -> tuple[int, dict[str, Any]]:
        raw = json.dumps(event).encode()
        header = sign(SECRET, raw, int(time.time()) + skew)
        if tamper:
            raw = raw.replace(b"high", b"low")  # body changed after signing
        req = urllib_request(base + "/webhooks/vendor", raw, header)
        return req

    results: dict[str, Any] = {}

    # 1. happy path
    results["1_valid_event"] = deliver({
        "event_id": "evt_1", "event_type": "vendor.updated", "vendor_id": "V001",
        "version": 5, "name": "Acme GmbH", "tier": "high",
    })
    wait_until_idle(store)

    # 2. exact duplicate: provider retried because our 202 was slow to arrive
    results["2_duplicate_delivery"] = deliver({
        "event_id": "evt_1", "event_type": "vendor.updated", "vendor_id": "V001",
        "version": 5, "name": "Acme GmbH", "tier": "high",
    })

    # 3. same event_id, different body: a genuine conflict, not a benign retry
    results["3_same_id_different_body"] = deliver({
        "event_id": "evt_1", "event_type": "vendor.updated", "vendor_id": "V001",
        "version": 9, "name": "Impostor Inc", "tier": "low",
    })
    results["3b_evt_1_audit_trail"] = store.audit_for("evt_1")

    # 4. tampered body, valid-looking signature
    results["4_tampered_body"] = deliver({
        "event_id": "evt_tamper", "event_type": "vendor.updated", "vendor_id": "V001",
        "version": 99, "name": "Acme GmbH", "tier": "high",
    }, tamper=True)

    # 5. timestamp outside tolerance (replay of an old capture)
    results["5_stale_timestamp"] = deliver({
        "event_id": "evt_old", "event_type": "vendor.updated", "vendor_id": "V001",
        "version": 99, "name": "Acme GmbH", "tier": "high",
    }, skew=-3600)

    # 6. out-of-order: v7 is fully applied before v6 arrives
    results["6_newer_version_v7"] = deliver({
        "event_id": "evt_7", "event_type": "vendor.updated", "vendor_id": "V002",
        "version": 7, "name": "Northwind Analytics", "tier": "high",
    })
    wait_until_idle(store)
    results["6b_stale_version_v6"] = deliver({
        "event_id": "evt_6", "event_type": "vendor.updated", "vendor_id": "V002",
        "version": 6, "name": "Northwind Analytics", "tier": "medium",
    })
    wait_until_idle(store)
    results["6c_evt_6_audit_trail"] = store.audit_for("evt_6")
    results["6d_v002_state"] = [v for v in store.metrics()["vendors"] if v["vendor_id"] == "V002"]

    # 7. poison event: downstream fails 3 times -> DLQ
    results["7_poison_event"] = deliver({
        "event_id": "evt_poison", "event_type": "vendor.updated", "vendor_id": "V003",
        "version": 1, "name": "Baltic Logistics OU", "tier": "medium", "poison": True,
    })

    time.sleep(1.2)  # let retries exhaust
    results["7b_metrics_after_poison"] = store.metrics()

    # 8. operator fixes the downstream and replays the DLQ
    store.set_flag("poison_fixed", True)
    results["8_replay"] = api.replay({})
    wait_until_idle(store)
    results["8b_metrics_after_replay"] = store.metrics()

    # 9. replay again: must be a no-op (nothing dead left)
    results["9_replay_idempotent"] = api.replay({})

    # 10. backpressure: service sheds load instead of queueing forever
    store.set_flag("throttled", True)
    results["10_throttled_429"] = deliver({
        "event_id": "evt_throttled", "event_type": "vendor.updated", "vendor_id": "V004",
        "version": 1, "name": "Globex Inc", "tier": "low",
    })
    store.set_flag("throttled", False)

    # 11. crash simulation: an unprocessed pending job survives a process restart,
    #     which is exactly what the inbox+outbox transaction buys us.
    with store._connect() as conn:
        conn.execute("BEGIN IMMEDIATE")
        conn.execute(
            """INSERT INTO inbox (event_id, event_type, vendor_id, version, payload, payload_hash,
                                  received_at, status, http_status)
               VALUES ('evt_crash','vendor.updated','V005',3,
                       '{"event_id":"evt_crash","event_type":"vendor.updated","vendor_id":"V005","version":3,"name":"Initech LLC","tier":"low"}',
                       'hash','1','accepted',202)"""
        )
        conn.execute("INSERT INTO jobs (event_id, status, attempts, next_attempt_at, updated_at) VALUES ('evt_crash','pending',0,0,1)")
        conn.execute("COMMIT")
    wait_until_idle(store)  # a live worker drains it, as it would after a restart
    results["11_after_crash_recovery"] = store.metrics()

    api.stop.set()
    server.shutdown()
    final = store.metrics()
    return {"steps": results, "final_metrics": final}


def urllib_request(url: str, raw: bytes, signature: str) -> tuple[int, dict[str, Any]]:
    import urllib.error
    import urllib.request

    request = urllib.request.Request(
        url, data=raw, method="POST",
        headers={"Content-Type": "application/json", "X-Signature": signature},
    )
    try:
        with urllib.request.urlopen(request, timeout=10) as response:
            return response.status, json.loads(response.read() or b"{}")
    except urllib.error.HTTPError as exc:
        return exc.code, json.loads(exc.read() or b"{}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--db", default="webhooks.db")
    args = parser.parse_args()

    outcome = run_scenario(args.db)
    if args.json:
        print(json.dumps(outcome, indent=2, default=str))
        return

    for name, result in outcome["steps"].items():
        if name.endswith("metrics") or name.endswith("recovery"):
            continue
        print(f"{name:<32} -> {result}")
    print("\n--- final metrics ---")
    print(json.dumps(outcome["final_metrics"], indent=2))
    print("\n--- takeaway ---")
    print("inbox PK  : duplicate delivery cannot double-apply business state")
    print("outbox    : accepted events survive a crash between 202 and processing")
    print("version   : v6 arriving after v7 is recorded as stale_ignored, not applied")
    print("DLQ+replay: a poisoned event is quarantined, then reprocessed safely after a fix")
    print("throttle  : 429 + Retry-After instead of unbounded queue growth")


if __name__ == "__main__":
    sys.exit(main())
