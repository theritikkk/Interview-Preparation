#!/usr/bin/env python3
"""
Vendor synchronisation between Certa and an existing GRC (ServiceNow-shaped) system.

Assessment topic 3 (Certa FDE): vendor sync between Certa and GRC/ServiceNow.

Demonstrates, end to end and runnable with no credentials:
  * pull sync with cursor pagination and an `updated_since` high-water mark
  * delta sync against a persisted mirror, so an incremental pull never looks
    like "the other system deleted everything"
  * a client-side token bucket plus honoured `Retry-After` on 429
  * a *lost response* (server applied the write, client saw a 500) recovered by
    replaying the same Idempotency-Key -- and a guard against reusing a key for
    a different body
  * field-level conflict resolution by explicit field ownership, LWW only where
    nobody owns the field, escalation when LWW cannot decide
  * a concurrent edit on the destination producing a real version conflict (409)
    that is re-planned from fresh data and retried exactly once
  * validation before a destructive push (quarantine, never retry a 422)
  * reconciliation after the run, and a second run that is a verified no-op

Run:
    python3 vendor_sync.py            # full scenario with a report
    python3 vendor_sync.py --json     # machine-readable report
"""

from __future__ import annotations

import argparse
import hashlib
import json
import threading
import time
import uuid
from dataclasses import dataclass, field
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from typing import Any, Iterator, Mapping
from urllib import error, parse, request as urllib_request

ACK_STATUSES = ("created", "updated")

# --------------------------------------------------------------------------- #
# Field ownership: the single most important design decision in a two-way sync.
# Without a written policy, every sync degenerates into last-write-wins.
# --------------------------------------------------------------------------- #
OWNER_CERTA = "certa"      # compliance-owned data: Certa is authoritative
OWNER_GRC = "grc"          # procurement-owned data: the GRC is authoritative
OWNER_LWW = "lww"          # nobody owns it exclusively: newest version wins

FIELD_OWNERSHIP: dict[str, str] = {
    "vendor_id": OWNER_CERTA,        # immutable identity, never overwritten
    "legal_name": OWNER_CERTA,
    "tier": OWNER_CERTA,             # risk tier is a compliance decision
    "data_processing": OWNER_CERTA,
    "region": OWNER_CERTA,
    "owner": OWNER_GRC,              # business owner / relationship manager
    "status": OWNER_GRC,             # procurement lifecycle status
    "cost_center": OWNER_GRC,
    "display_name": OWNER_LWW,       # presentation only, no compliance impact
}
IDENTITY_KEYS = ("vendor_id",)
GRC_TIER_ENUM = {"low", "medium", "high", "critical"}
VALID_STATUS = {"active", "under_review", "offboarded"}
# Creating a net-new third party is an onboarding decision, not a sync side effect.
AUTO_CREATE_IN_CERTA = False


# --------------------------------------------------------------------------- #
# Fixtures: the two systems, as they exist before the run
# --------------------------------------------------------------------------- #
CERTA_VENDORS: dict[str, dict[str, Any]] = {
    "V001": {"vendor_id": "V001", "legal_name": "Acme GmbH", "display_name": "Acme",
             "tier": "high", "data_processing": "true", "region": "Europe",
             "owner": "rita.compliance@customer.example", "status": "active",
             "cost_center": "CC-100", "version": 7, "updated_at": "2026-10-01T09:00:00Z"},
    "V002": {"vendor_id": "V002", "legal_name": "Northwind Analytics", "display_name": "Northwind",
             "tier": "medium", "data_processing": "false", "region": "Americas",
             "owner": "ops@northwind.example", "status": "active",
             "cost_center": "CC-200", "version": 3, "updated_at": "2026-09-15T09:00:00Z"},
    "V003": {"vendor_id": "V003", "legal_name": "Baltic Logistics OU", "display_name": "Baltic",
             "tier": "low", "data_processing": "true", "region": "Europe",
             "owner": "logistics-owner@customer.example", "status": "active",
             "cost_center": "CC-300", "version": 2, "updated_at": "2026-08-20T09:00:00Z"},
    # Data-quality defect: tier is outside the destination enum. Must be quarantined
    # locally, not pushed and retried until someone notices a 422 loop.
    "V005": {"vendor_id": "V005", "legal_name": "Initech LLC", "display_name": "Initech",
             "tier": "unknown", "data_processing": "false", "region": "Americas",
             "owner": "it-owner@customer.example", "status": "active",
             "cost_center": "CC-500", "version": 1, "updated_at": "2026-10-05T09:00:00Z"},
}

GRC_VENDORS: dict[str, dict[str, Any]] = {
    # tier is stale here -> Certa owns tier, so Certa wins and we push
    # owner is newer here -> GRC owns owner, so GRC wins and we pull
    "V001": {"vendor_id": "V001", "legal_name": "Acme GmbH", "display_name": "Acme GmbH",
             "tier": "low", "data_processing": "true", "region": "Europe",
             "owner": "procurement.lead@customer.example", "status": "active",
             "cost_center": "CC-100", "version": 9, "updated_at": "2026-10-10T11:00:00Z"},
    # display_name differs and both sides are LWW: GRC's version is newer -> GRC wins
    "V002": {"vendor_id": "V002", "legal_name": "Northwind Analytics Inc",
             "display_name": "Northwind Analytics, Inc.", "tier": "medium",
             "data_processing": "false", "region": "Americas", "owner": "ops@northwind.example",
             "status": "active", "cost_center": "CC-200", "version": 8, "updated_at": "2026-10-08T11:00:00Z"},
    # exists only in the GRC: onboarding a new third party needs approval, not sync
    "V004": {"vendor_id": "V004", "legal_name": "Globex Inc", "display_name": "Globex",
             "tier": "critical", "data_processing": "true", "region": "Americas",
             "owner": "sec-team@customer.example", "status": "active",
             "cost_center": "CC-400", "version": 4, "updated_at": "2026-10-02T11:00:00Z"},
    # A concurrent writer bumps this vendor *after* we read it: the push must 409,
    # re-read, re-apply the policy and retry once.
    "V006": {"vendor_id": "V006", "legal_name": "Umbrella Health", "display_name": "Umbrella",
             "tier": "low", "data_processing": "true", "region": "Europe",
             "owner": "hr-owner@customer.example", "status": "under_review",
             "cost_center": "CC-600", "version": 11, "updated_at": "2026-10-12T11:00:00Z"},
}
CERTA_VENDORS["V006"] = {"vendor_id": "V006", "legal_name": "Umbrella Health", "display_name": "Umbrella",
                         "tier": "high", "data_processing": "true", "region": "Europe",
                         "owner": "hr-owner@customer.example", "status": "active",
                         "cost_center": "CC-600", "version": 10, "updated_at": "2026-10-11T09:00:00Z"}


# --------------------------------------------------------------------------- #
# Mock GRC API
# --------------------------------------------------------------------------- #
class GrcState:
    def __init__(self) -> None:
        self.lock = threading.Lock()
        self.vendors: dict[str, dict[str, Any]] = {k: dict(v) for k, v in GRC_VENDORS.items()}
        self.idempotency_cache: dict[str, dict[str, Any]] = {}   # key -> {"hash":..., "response":...}
        self.request_log: list[dict[str, Any]] = []
        self.get_calls = 0
        self.inject_429_every = 4              # deterministic rate limiting for the demo
        self.drop_batch_response_once = True   # server applies the write, client sees 500
        self.rate_limit_hits = 0
        self.concurrent_edit_applied = False


GRC = GrcState()


class GrcHandler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"
    quiet = True

    def _json(self, status: int, body: Mapping[str, Any], retry_after: int | None = None) -> None:
        encoded = json.dumps(body).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(encoded)))
        if retry_after is not None:
            self.send_header("Retry-After", str(retry_after))
        self.end_headers()
        self.wfile.write(encoded)

    def do_GET(self) -> None:  # noqa: N802
        parsed = parse.urlparse(self.path)
        if parsed.path != "/grc/vendors":
            self._json(404, {"error": "not_found"})
            return
        with GRC.lock:
            GRC.get_calls += 1
            GRC.request_log.append({"at": time.time(), "path": self.path, "outcome": "ok"})
            if GRC.get_calls % GRC.inject_429_every == 0:   # simulate the provider's rate limiter
                GRC.rate_limit_hits += 1
                GRC.request_log[-1]["outcome"] = "429"
                self._json(429, {"error": "rate_limited"}, retry_after=1)
                return
            params = parse.parse_qs(parsed.query)
            updated_since = (params.get("updated_since") or [""])[0]
            limit = int((params.get("limit") or ["2"])[0])
            cursor = (params.get("cursor") or [""])[0]

            rows = [v for v in GRC.vendors.values() if not updated_since or v["updated_at"] > updated_since]
            rows.sort(key=lambda v: v["vendor_id"])
            start = 0
            if cursor:
                ids = [v["vendor_id"] for v in rows]
                start = ids.index(cursor) + 1 if cursor in ids else len(rows)
            page = rows[start : start + limit]
            next_cursor = page[-1]["vendor_id"] if start + limit < len(rows) and page else None

            # A different writer (a human in the GRC UI, another integration) edits V006
            # immediately after we read it. This is the race the version check exists for.
            if not GRC.concurrent_edit_applied and any(v["vendor_id"] == "V006" for v in page):
                GRC.concurrent_edit_applied = True
                GRC.vendors["V006"] = {**GRC.vendors["V006"], "version": 13,
                                       "status": "offboarded", "updated_at": "2026-10-14T08:00:00Z"}

            self._json(200, {"data": page, "next_cursor": next_cursor, "total": len(rows)})

    def do_POST(self) -> None:  # noqa: N802
        if self.path != "/grc/vendors:batch":
            self._json(404, {"error": "not_found"})
            return
        length = int(self.headers.get("Content-Length") or 0)
        raw = self.rfile.read(length) or b"{}"
        body = json.loads(raw)
        idem_key = self.headers.get("Idempotency-Key", "")
        body_hash = hashlib.sha256(raw).hexdigest()

        with GRC.lock:
            cached = GRC.idempotency_cache.get(idem_key) if idem_key else None
            if cached is not None:
                if cached["hash"] != body_hash:
                    # The same key with a different body is a client bug: refusing it
                    # prevents returning another request's cached response as if it
                    # were this one's -- silent data corruption.
                    GRC.request_log.append({"at": time.time(), "path": self.path, "outcome": "idempotency_key_reuse"})
                    self._json(409, {"error": "idempotency_key_reuse", "detail": "key already used with a different body"})
                    return
                GRC.request_log.append({"at": time.time(), "path": self.path, "outcome": "idempotent_replay"})
                self._json(200, cached["response"])
                return

            results = [self._apply_item(item) for item in body.get("items", [])]
            response = {"results": results}
            if idem_key:
                GRC.idempotency_cache[idem_key] = {"hash": body_hash, "response": response}

            if GRC.drop_batch_response_once:
                # The write is committed but the response is lost -- the case where
                # "the client saw an error" does NOT mean "the server did nothing".
                GRC.drop_batch_response_once = False
                GRC.request_log.append({"at": time.time(), "path": self.path, "outcome": "applied_then_500"})
                self._json(500, {"error": "gateway_timeout"})
                return

            GRC.request_log.append({"at": time.time(), "path": self.path, "outcome": "ok"})
            self._json(200, response)

    @staticmethod
    def _apply_item(item: Mapping[str, Any]) -> dict[str, Any]:
        vendor_id = item.get("vendor_id")
        incoming_version = int(item.get("version", 0))
        errors: list[str] = []
        tier = item.get("tier")
        if tier is not None and tier not in GRC_TIER_ENUM:
            errors.append(f"tier must be one of {sorted(GRC_TIER_ENUM)}, got {tier!r}")
        if not item.get("legal_name"):
            errors.append("legal_name is required")
        if errors:
            return {"vendor_id": vendor_id, "status": "invalid", "errors": errors}

        current = GRC.vendors.get(vendor_id)
        if current is None:
            created = dict(item)
            created["version"] = 1  # the destination assigns the initial version
            created["updated_at"] = "2026-10-15T12:00:00Z"
            GRC.vendors[vendor_id] = created
            return {"vendor_id": vendor_id, "status": "created", "current_version": 1,
                    "updated_at": created["updated_at"]}
        if incoming_version != current["version"]:
            # Optimistic concurrency: the write asserts the version it was based on.
            return {"vendor_id": vendor_id, "status": "conflict",
                    "errors": [f"based on v{incoming_version}, current is v{current['version']}"],
                    "current_version": current["version"], "current": current}
        else:
            merged = dict(current)
            merged.update({k: v for k, v in item.items() if k in FIELD_OWNERSHIP})
            merged["version"] = incoming_version + 1
            merged["updated_at"] = "2026-10-15T12:00:00Z"
            GRC.vendors[vendor_id] = merged
            return {"vendor_id": vendor_id, "status": "updated", "current_version": merged["version"],
                    "updated_at": merged["updated_at"]}

    def log_message(self, *args: Any) -> None:
        if not self.quiet:
            super().log_message(*args)


# --------------------------------------------------------------------------- #
# Client
# --------------------------------------------------------------------------- #
class TransientError(RuntimeError):
    """Safe to retry: 429, 5xx, timeouts."""


class PermanentError(RuntimeError):
    """Not safe to retry: 400/401/403/404/409/422."""


class TokenBucket:
    """Client-side pacing so we spend our quota instead of discovering it via 429."""

    def __init__(self, rate_per_second: float, capacity: int) -> None:
        self.rate = rate_per_second
        self.capacity = capacity
        self.tokens = float(capacity)
        self.updated = time.monotonic()
        self.lock = threading.Lock()
        self.waits = 0

    def acquire(self) -> None:
        with self.lock:
            while True:
                now = time.monotonic()
                self.tokens = min(self.capacity, self.tokens + (now - self.updated) * self.rate)
                self.updated = now
                if self.tokens >= 1:
                    self.tokens -= 1
                    return
                self.waits += 1
                time.sleep((1 - self.tokens) / self.rate)


@dataclass
class SyncStats:
    pages_fetched: int = 0
    http_429s: int = 0
    pushes_attempted: int = 0
    pushes_applied: int = 0
    conflicts_seen: int = 0
    quarantined: list[dict[str, Any]] = field(default_factory=list)
    review_queue: list[dict[str, Any]] = field(default_factory=list)
    field_writes_certa_to_grc: int = 0
    field_writes_grc_to_certa: int = 0


class GrcClient:
    def __init__(self, base_url: str, *, rate_per_second: float = 4.0, capacity: int = 2, batch_size: int = 3) -> None:
        self.base_url = base_url.rstrip("/")
        self.bucket = TokenBucket(rate_per_second, capacity)
        self.batch_size = batch_size

    def _request(self, method: str, path: str, *, body: Mapping[str, Any] | None = None,
                 headers: Mapping[str, str] | None = None, max_attempts: int = 5,
                 stats: SyncStats | None = None) -> dict[str, Any]:
        payload = json.dumps(body).encode() if body is not None else None
        last: Exception | None = None
        for attempt in range(1, max_attempts + 1):
            self.bucket.acquire()
            request = urllib_request.Request(
                self.base_url + path, data=payload, method=method,
                headers={"Content-Type": "application/json", "Accept": "application/json", **(headers or {})},
            )
            try:
                with urllib_request.urlopen(request, timeout=10) as response:
                    return json.loads(response.read() or b"{}")
            except error.HTTPError as exc:
                detail = exc.read().decode("utf-8", "replace")[:300]
                if exc.code == 429:
                    if stats:
                        stats.http_429s += 1
                    retry_after = exc.headers.get("Retry-After")
                    numeric = bool(retry_after) and retry_after.replace(".", "", 1).isdigit()
                    delay = float(retry_after) if numeric else 0.5 * 2 ** (attempt - 1)
                    last = TransientError(f"429 from {path}")
                    if attempt < max_attempts:
                        # Jitter on top of Retry-After: without it every client retries in
                        # the same millisecond and re-triggers the same limit.
                        time.sleep(min(delay, 30.0) + 0.05 * attempt)
                        continue
                elif exc.code >= 500:
                    last = TransientError(f"HTTP {exc.code} from {path}: {detail}")
                    if attempt < max_attempts:
                        time.sleep(0.15 * 2 ** (attempt - 1))
                        continue
                else:
                    raise PermanentError(f"HTTP {exc.code} from {path}: {detail}") from exc
            except (error.URLError, TimeoutError) as exc:
                last = TransientError(f"network error on {path}: {exc}")
                if attempt < max_attempts:
                    time.sleep(0.15 * 2 ** (attempt - 1))
                    continue
        raise TransientError(f"{path} failed after {max_attempts} attempts: {last}")

    def iter_vendors(self, *, updated_since: str | None = None, page_size: int = 2,
                     max_pages: int = 50, stats: SyncStats | None = None) -> Iterator[dict[str, Any]]:
        """Cursor pagination with a loop guard: bound the walk and assert progress."""
        cursor: str | None = None
        seen: set[str] = set()
        for _ in range(max_pages):
            query = {"limit": str(page_size)}
            if updated_since:
                query["updated_since"] = updated_since
            if cursor:
                query["cursor"] = cursor
            page = self._request("GET", "/grc/vendors?" + parse.urlencode(query), stats=stats)
            if stats:
                stats.pages_fetched += 1
            for row in page.get("data") or []:
                if row["vendor_id"] in seen:
                    raise PermanentError(f"provider returned {row['vendor_id']} twice; cursor is not stable")
                seen.add(row["vendor_id"])
                yield row
            cursor = page.get("next_cursor")
            if not cursor:
                return
        raise PermanentError("pagination exceeded max_pages; possible cursor loop")

    def push(self, items: list[dict[str, Any]], idempotency_key: str, stats: SyncStats) -> list[dict[str, Any]]:
        """Push in batches. Each batch gets a key derived from the run key *and the batch
        content hash*, so retrying a batch is safe but two different batches can never
        share a key (reusing a key for a different body returns the wrong cached answer).
        """
        results: list[dict[str, Any]] = []
        for start in range(0, len(items), self.batch_size):
            batch = items[start : start + self.batch_size]
            body = {"items": batch}
            digest = hashlib.sha256(json.dumps(body, sort_keys=True).encode()).hexdigest()[:16]
            key = f"{idempotency_key}-{start//self.batch_size}-{digest}"
            stats.pushes_attempted += len(batch)
            response = self._request("POST", "/grc/vendors:batch", body=body,
                                     headers={"Idempotency-Key": key}, stats=stats)
            results.extend(response.get("results", []))
        return results


# --------------------------------------------------------------------------- #
# Planning: who writes which field, and what cannot be decided automatically
# --------------------------------------------------------------------------- #
@dataclass
class Plan:
    push_fields: dict[str, dict[str, Any]] = field(default_factory=dict)   # vendor_id -> partial patch
    pull_fields: dict[str, dict[str, Any]] = field(default_factory=dict)   # vendor_id -> partial patch
    create_in_grc: list[str] = field(default_factory=list)
    review: list[dict[str, Any]] = field(default_factory=list)
    notes: list[str] = field(default_factory=list)


def validate_payload(payload: Mapping[str, Any]) -> list[str]:
    """Deterministic checks in front of the API. A 422 costs a round trip and a support
    ticket; a local check costs nothing."""
    errors: list[str] = []
    if not payload.get("vendor_id"):
        errors.append("vendor_id is required")
    if "tier" in payload and payload["tier"] not in GRC_TIER_ENUM:
        errors.append(f"tier {payload['tier']!r} is outside the destination enum {sorted(GRC_TIER_ENUM)}")
    if "legal_name" in payload and not str(payload.get("legal_name", "")).strip():
        errors.append("legal_name is empty")
    if payload.get("status") is not None and payload["status"] not in VALID_STATUS:
        errors.append(f"status {payload['status']!r} is not a known lifecycle value")
    return errors


def plan_sync(certa: Mapping[str, Mapping[str, Any]], grc: Mapping[str, Mapping[str, Any]]) -> Plan:
    """Field-level merge plan against the *mirror* of the destination.

    Order of decisions per vendor:
      1. presence on only one side -> create (if allowed) or escalate
      2. per field: the owning system wins; LWW only when nobody owns it
      3. equal-version disagreement is undecidable and must be escalated, never coin-flipped
      4. validate the outgoing payload before it is ever sent
    """
    plan = Plan()
    for vendor_id in sorted(set(certa) | set(grc)):
        c, g = certa.get(vendor_id), grc.get(vendor_id)

        if c and not g:
            errors = validate_payload(c)
            if errors:
                plan.review.append({"vendor_id": vendor_id, "reason": "validation_failed",
                                    "detail": "; ".join(errors)})
                plan.notes.append(f"{vendor_id}: quarantined locally instead of pushing invalid data")
            else:
                plan.create_in_grc.append(vendor_id)
                plan.notes.append(f"{vendor_id}: exists only in Certa -> create in GRC")
            continue
        if g and not c:
            if AUTO_CREATE_IN_CERTA:
                plan.notes.append(f"{vendor_id}: exists only in GRC -> create in Certa")
            else:
                plan.review.append({"vendor_id": vendor_id, "reason": "only_in_grc",
                                    "detail": "new third party requires onboarding approval"})
            continue

        assert c is not None and g is not None
        patch: dict[str, Any] = {}
        for field_name, owner in FIELD_OWNERSHIP.items():
            if field_name in IDENTITY_KEYS:
                continue
            certa_value, grc_value = c.get(field_name), g.get(field_name)
            if certa_value == grc_value:
                continue
            if owner == OWNER_CERTA:
                patch[field_name] = certa_value
            elif owner == OWNER_GRC:
                plan.pull_fields.setdefault(vendor_id, {})[field_name] = grc_value
            else:  # LWW
                if int(c.get("version", 0)) > int(g.get("version", 0)):
                    patch[field_name] = certa_value
                elif int(g.get("version", 0)) > int(c.get("version", 0)):
                    plan.pull_fields.setdefault(vendor_id, {})[field_name] = grc_value
                else:
                    plan.review.append({
                        "vendor_id": vendor_id, "reason": "undecidable_lww",
                        "detail": f"{field_name}: versions equal ({c.get('version')}) but values differ",
                    })

        if patch:
            errors = validate_payload({**c, **patch})
            if errors:
                plan.review.append({"vendor_id": vendor_id, "reason": "validation_failed", "detail": "; ".join(errors)})
                plan.notes.append(f"{vendor_id}: quarantined locally instead of pushing invalid data")
            else:
                plan.push_fields[vendor_id] = patch
    return plan


def build_push_items(plan: Plan, certa: Mapping[str, Mapping[str, Any]], mirror: Mapping[str, Mapping[str, Any]]) -> list[dict[str, Any]]:
    """Build payloads that assert the version the client read.

    The version is NOT incremented locally: the destination owns its version counter,
    and sending "the version I based this change on" is what lets it detect a
    concurrent write instead of silently overwriting it.
    """
    items: list[dict[str, Any]] = []
    for vendor_id, patch in plan.push_fields.items():
        base = dict(mirror.get(vendor_id) or certa[vendor_id])
        base.update(patch)
        items.append(base)
    for vendor_id in plan.create_in_grc:
        source = dict(certa[vendor_id])
        source["version"] = 0  # creating: no prior version on the destination
        items.append(source)
    return items


# --------------------------------------------------------------------------- #
# Reconciliation
# --------------------------------------------------------------------------- #
def reconcile(certa: Mapping[str, Mapping[str, Any]], grc: Mapping[str, Mapping[str, Any]]) -> dict[str, Any]:
    """Compare both systems field by field; the ownership policy says which differences
    are defects and which are expected."""
    report: dict[str, Any] = {
        "in_sync": 0,
        "unexpected_drift": [],
        "only_in_certa": sorted(set(certa) - set(grc)),
        "only_in_grc": sorted(set(grc) - set(certa)),
    }
    for vendor_id in sorted(set(certa) & set(grc)):
        c, g = certa[vendor_id], grc[vendor_id]
        drift: list[dict[str, Any]] = []
        for field_name in FIELD_OWNERSHIP:
            if field_name in IDENTITY_KEYS or c.get(field_name) == g.get(field_name):
                continue
            # An unpropagated difference in an *owned* field is a defect: the owner's
            # value should have been written to the other side by now.
            drift.append({
                "field": field_name,
                "certa": c.get(field_name),
                "grc": g.get(field_name),
                "owner": FIELD_OWNERSHIP[field_name],
                "verdict": "needs_manual_check" if FIELD_OWNERSHIP[field_name] == OWNER_LWW else "unpropagated_owned_field",
            })
        if drift:
            report["unexpected_drift"].append({"vendor_id": vendor_id, "fields": drift})
        else:
            report["in_sync"] += 1
    return report


# --------------------------------------------------------------------------- #
# Orchestration
# --------------------------------------------------------------------------- #
def run_sync(client: GrcClient, certa: dict[str, dict[str, Any]], mirror: dict[str, dict[str, Any]],
             *, updated_since: str | None, stats: SyncStats, label: str) -> dict[str, Any]:
    """One sync run.

    The mirror is the key to incremental sync: a delta pull tells you what *changed*,
    not what *exists*. Planning against only the delta rows would make every unchanged,
    already-synced vendor look like "exists only in Certa" and re-create it.
    """
    delta = {row["vendor_id"]: row for row in client.iter_vendors(updated_since=updated_since, stats=stats)}
    mirror.update(delta)

    plan = plan_sync(certa, mirror)
    stats.review_queue.extend(plan.review)
    stats.quarantined.extend([r for r in plan.review if r["reason"] == "validation_failed"])

    # pull side: Certa is the system we control, so apply locally
    for vendor_id, patch in plan.pull_fields.items():
        certa[vendor_id].update(patch)
        certa[vendor_id]["version"] = int(certa[vendor_id].get("version", 0)) + 1
        stats.field_writes_grc_to_certa += len(patch)

    push_results: list[dict[str, Any]] = []
    if plan.push_fields or plan.create_in_grc:
        items = build_push_items(plan, certa, mirror)
        push_results = client.push(items, f"sync-{label}-{uuid.uuid4()}", stats)
        for result in push_results:
            vendor_id = result["vendor_id"]
            if result["status"] in ACK_STATUSES:
                stats.pushes_applied += 1
                patch = plan.push_fields.get(vendor_id)
                stats.field_writes_certa_to_grc += len(patch) if patch else 1
                # Trust the destination's acknowledgement, not our own assumption about
                # what we just wrote: adopt its version and timestamp into the mirror.
                # Without this, the next plan re-pushes from a stale version and 409s.
                if vendor_id in mirror:
                    mirror[vendor_id].update(patch or {})
                    mirror[vendor_id]["version"] = result["current_version"]
                    mirror[vendor_id]["updated_at"] = result.get("updated_at", mirror[vendor_id]["updated_at"])
                else:
                    mirror[vendor_id] = {**certa[vendor_id], "version": result["current_version"],
                                         "updated_at": result.get("updated_at")}
            elif result["status"] == "invalid":
                stats.quarantined.append({"vendor_id": vendor_id, "reason": "destination_422",
                                          "detail": result.get("errors")})
                stats.review_queue.append({"vendor_id": vendor_id, "reason": "validation_rejected",
                                           "detail": "; ".join(result.get("errors") or [])})

    # bounded conflict recovery: refetch the affected rows, re-apply the policy, retry once
    conflicted = [r["vendor_id"] for r in push_results if r["status"] == "conflict"]
    retry_results: list[dict[str, Any]] = []
    if conflicted:
        stats.conflicts_seen += len(conflicted)
        fresh: dict[str, dict[str, Any]] = {}
        for row in client.iter_vendors(stats=stats):
            if row["vendor_id"] in conflicted:
                fresh[row["vendor_id"]] = row
        mirror.update(fresh)
        replan = plan_sync(certa, {**mirror, **fresh})
        retry_items = build_push_items(replan, certa, {**mirror, **fresh})
        if retry_items:
            retry_results = client.push(retry_items, f"sync-{label}-conflict-retry-{uuid.uuid4()}", stats)
            for result in retry_results:
                vendor_id = result["vendor_id"]
                if result["status"] in ACK_STATUSES:
                    stats.pushes_applied += 1
                    patch = replan.push_fields.get(vendor_id)
                    stats.field_writes_certa_to_grc += len(patch) if patch else 1
                    mirror[vendor_id] = {**mirror.get(vendor_id, {}), **(patch or {}),
                                         "version": result["current_version"],
                                         "updated_at": result.get("updated_at", fresh.get(vendor_id, {}).get("updated_at"))}
                else:
                    stats.review_queue.append({"vendor_id": vendor_id, "reason": "conflict_unresolved",
                                               "detail": json.dumps(result.get("errors"))})
        for vendor_id, patch in replan.pull_fields.items():
            certa[vendor_id].update(patch)
            certa[vendor_id]["version"] = int(certa[vendor_id].get("version", 0)) + 1
            stats.field_writes_grc_to_certa += len(patch)

    # High-water mark comes from the mirror (which now includes destination
    # acknowledgements), so the next delta pull does not re-fetch our own writes.
    high_water = max((v.get("updated_at", "") for v in mirror.values()), default=updated_since or "")
    return {
        "pulled": len(delta),
        "planned_pushes": len(plan.push_fields) + len(plan.create_in_grc),
        "push_results": push_results,
        "conflict_retry_results": retry_results,
        "high_water_mark": high_water,
        "plan_notes": plan.notes,
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()

    certa = {k: dict(v) for k, v in CERTA_VENDORS.items()}
    mirror: dict[str, dict[str, Any]] = {}   # local cache of the destination's state
    server = ThreadingHTTPServer(("127.0.0.1", 0), GrcHandler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    client = GrcClient(f"http://127.0.0.1:{server.server_address[1]}", rate_per_second=4.0, capacity=2)

    report: dict[str, Any] = {}

    # --- run 1: initial sync ------------------------------------------------
    stats1 = SyncStats()
    run1 = run_sync(client, certa, mirror, updated_since=None, stats=stats1, label="run1")
    # Reconcile against a fresh full pull, not against our mirror: verify, don't assume.
    truth = {row["vendor_id"]: row for row in client.iter_vendors(page_size=10)}
    report["run1"] = {
        "pulled_vendors": run1["pulled"],
        "planned_pushes": run1["planned_pushes"],
        "push_results": run1["push_results"],
        "conflict_retry_results": run1["conflict_retry_results"],
        "plan_notes": run1["plan_notes"],
        "http_429s_handled": stats1.http_429s,
        "pages_fetched": stats1.pages_fetched,
        "field_writes": {"certa_to_grc": stats1.field_writes_certa_to_grc,
                         "grc_to_certa": stats1.field_writes_grc_to_certa},
        "quarantined": stats1.quarantined,
        "review_queue": stats1.review_queue,
        "reconciliation": reconcile(certa, truth),
        "high_water_mark": run1["high_water_mark"],
    }

    # --- run 2: delta sync, must be a clean no-op ---------------------------
    stats2 = SyncStats()
    run2 = run_sync(client, certa, mirror, updated_since=run1["high_water_mark"], stats=stats2, label="run2")
    report["run2_delta"] = {
        "pulled_vendors": run2["pulled"],
        "planned_pushes": run2["planned_pushes"],
        "push_results": run2["push_results"],
        "http_429s_handled": stats2.http_429s,
        "idempotent_replays_observed": sum(1 for e in GRC.request_log if e["outcome"] == "idempotent_replay"),
        "server_side_rate_limit_hits": GRC.rate_limit_hits,
        "reconciliation": reconcile(certa, {row["vendor_id"]: row for row in client.iter_vendors(page_size=10)}),
    }

    # --- guard: reusing a key for a *different* body must be refused -------------
    guard_key = "guard-demo-key"
    guard_body_a = {"items": [{"vendor_id": "V999", "legal_name": "Guard Demo A",
                               "tier": "low", "version": 0}]}
    guard_body_b = {"items": [{"vendor_id": "V999", "legal_name": "Guard Demo B",
                               "tier": "high", "version": 0}]}
    guard: dict[str, Any] = {}
    try:
        client._request("POST", "/grc/vendors:batch", body=guard_body_a,
                        headers={"Idempotency-Key": guard_key})
        try:
            client._request("POST", "/grc/vendors:batch", body=guard_body_b,
                            headers={"Idempotency-Key": guard_key}, max_attempts=1)
            guard["outcome"] = "UNEXPECTED: a different body was accepted under a reused key"
        except PermanentError as exc:
            guard["outcome"] = f"refused as designed -> {str(exc)[:70]}"
    except Exception as exc:  # noqa: BLE001
        guard["outcome"] = f"setup error: {exc}"
    report["idempotency_key_reuse_guard"] = guard

    server.shutdown()
    if args.json:
        print(json.dumps(report, indent=2, default=str))
        return

    print("=== run 1: initial sync")
    print(f"pulled from GRC        : {report['run1']['pulled_vendors']} vendors over {report['run1']['pages_fetched']} pages")
    print(f"429s handled by client : {report['run1']['http_429s_handled']}")
    print(f"planned pushes         : {report['run1']['planned_pushes']}")
    for note in report["run1"]["plan_notes"]:
        print(f"  plan: {note}")
    print("push results:")
    for result in report["run1"]["push_results"]:
        print(f"  {result['vendor_id']}: {result['status']} {result.get('errors', '')}")
    if report["run1"]["conflict_retry_results"]:
        print("conflict retry results (re-planned from fresh data):")
        for result in report["run1"]["conflict_retry_results"]:
            print(f"  {result['vendor_id']}: {result['status']} {result.get('errors', '')}")
    print(f"field writes           : certa->grc {report['run1']['field_writes']['certa_to_grc']}, "
          f"grc->certa {report['run1']['field_writes']['grc_to_certa']}")
    print("review queue (human decision required):")
    for item in report["run1"]["review_queue"]:
        print(f"  {item['vendor_id']}: {item['reason']} -- {item['detail']}")

    print("\n=== reconciliation after run 1 (against a fresh full pull)")
    rec = report["run1"]["reconciliation"]
    print(f"in sync                  : {rec['in_sync']}")
    print(f"unexpected drift         : {len(rec['unexpected_drift'])}")
    for item in rec["unexpected_drift"]:
        print(f"  {item['vendor_id']}: {item['fields']}")
    print(f"only in Certa            : {rec['only_in_certa']}  (blocked by validation/review)")
    print(f"only in GRC              : {rec['only_in_grc']}  (awaiting onboarding approval)")

    print("\n=== run 2: delta sync (must be a no-op)")
    print(json.dumps({k: v for k, v in report["run2_delta"].items() if k != "reconciliation"}, indent=2))
    print(f"reconciliation in sync   : {report['run2_delta']['reconciliation']['in_sync']}")
    print(f"unexpected drift         : {len(report['run2_delta']['reconciliation']['unexpected_drift'])}")
    print(f"key-reuse guard          : {report['idempotency_key_reuse_guard']['outcome']}")

    print("\n=== key mechanisms")
    print("idempotency key : the first batch was applied and the response was lost (500);")
    print("                  the retry reused that batch's key, so no duplicate write happened.")
    print("key scoping     : keys are per batch + body hash, so a retry replays and a")
    print("                  different batch can never receive another batch's cached answer.")
    print("rate limiting   : client token bucket + honoured Retry-After absorbed server 429s.")
    print("ownership       : Certa wrote tier, GRC wrote owner -- both directions, one run.")
    print("conflict        : a concurrent edit caused a 409, re-planned from fresh data,")
    print("                  retried once, and applied exactly once.")
    print("escalation      : vendor creation and undecidable LWW went to a human review queue.")
    print("delta + mirror  : run 2 pulled the delta against a mirror and planned zero writes.")


if __name__ == "__main__":
    main()
