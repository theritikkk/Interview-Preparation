#!/usr/bin/env python3
"""
Soaring Prices -- "return the three most expensive products per city" from a REST API.

Assessment topic 2 (Certa FDE). Fully runnable, stdlib + optional `requests`.

Run:
    python3 soaring_prices.py --selftest        # offline assertions, no network
    python3 soaring_prices.py --mock            # starts a local mock API, then solves it
    python3 soaring_prices.py --url https://... # solve against a real endpoint

Design notes (the things an interviewer is listening for):
  * prices compared numerically (Decimal), never as strings
  * deterministic tie-break so repeated runs give identical output
  * one network call; parsing is pure and unit-testable
  * malformed rows are skipped and counted, not silently dropped
  * "top 3" must be defined when a city has fewer than 3 products
"""

from __future__ import annotations

import argparse
import json
import threading
import time
from decimal import Decimal, InvalidOperation
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from typing import Any, Callable, Iterable, Mapping, Sequence

try:  # optional: production code normally uses requests
    import requests
except ImportError:  # pragma: no cover
    requests = None  # type: ignore[assignment]


# --------------------------------------------------------------------------- #
# 1. Fetch layer
# --------------------------------------------------------------------------- #
class TransientHTTPError(RuntimeError):
    """Retryable: timeouts, connection resets, 429, 5xx."""


class PermanentHTTPError(RuntimeError):
    """Not retryable: 400/401/403/404/422 and other contract errors."""


def _decode_json(raw: bytes, url: str) -> Any:
    try:
        return json.loads(raw.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise PermanentHTTPError(f"{url} returned a non-JSON body: {exc}") from exc


def fetch_json_urllib(
    url: str,
    *,
    timeout: tuple[float, float] = (3.0, 15.0),
    max_attempts: int = 4,
    base_delay: float = 0.5,
    sleep: Callable[[float], None] = time.sleep,
    headers: Mapping[str, str] | None = None,
) -> Any:
    """GET a URL and parse JSON, with timeouts, bounded retries and jitter-free backoff.

    Retries only what is safe to retry: a GET is idempotent, so re-issuing it is fine.
    """
    import urllib.error
    import urllib.request

    request = urllib.request.Request(url, headers=dict(headers or {}), method="GET")
    last_error: Exception | None = None
    for attempt in range(1, max_attempts + 1):
        try:
            with urllib.request.urlopen(request, timeout=timeout[1]) as response:
                body = response.read()
                if response.status >= 500:
                    raise TransientHTTPError(f"{url} -> HTTP {response.status}")
                return _decode_json(body, url)
        except urllib.error.HTTPError as exc:
            if exc.code == 429 or exc.code >= 500:
                last_error = TransientHTTPError(f"{url} -> HTTP {exc.code}")
                retry_after = exc.headers.get("Retry-After")
                delay = float(retry_after) if retry_after and retry_after.isdigit() else base_delay * 2 ** (attempt - 1)
            else:
                raise PermanentHTTPError(f"{url} -> HTTP {exc.code}") from exc
        except (urllib.error.URLError, TimeoutError, TransientHTTPError) as exc:
            last_error = exc
            delay = base_delay * 2 ** (attempt - 1)
        if attempt == max_attempts:
            break
        sleep(min(delay, 30.0))
    raise TransientHTTPError(f"{url} failed after {max_attempts} attempts: {last_error}")


def fetch_json_requests(
    url: str,
    *,
    timeout: tuple[float, float] = (3.0, 15.0),
    max_attempts: int = 4,
    session: "requests.Session | None" = None,
) -> Any:
    """Same contract using `requests` (connection reuse, tuple timeouts)."""
    if requests is None:
        raise RuntimeError("pip install requests")
    http = session or requests.Session()
    for attempt in range(1, max_attempts + 1):
        try:
            response = http.get(url, timeout=timeout, headers={"Accept": "application/json"})
        except requests.Timeout as exc:  # connect or read timeout: outcome unknown
            if attempt == max_attempts:
                raise TransientHTTPError(f"timeout after {max_attempts} attempts") from exc
            time.sleep(0.5 * 2 ** (attempt - 1))
            continue
        except requests.RequestException as exc:
            if attempt == max_attempts:
                raise TransientHTTPError(str(exc)) from exc
            time.sleep(0.5 * 2 ** (attempt - 1))
            continue

        if response.status_code == 429 or response.status_code >= 500:
            if attempt == max_attempts:
                raise TransientHTTPError(f"HTTP {response.status_code}")
            retry_after = response.headers.get("Retry-After")
            delay = float(retry_after) if retry_after and retry_after.isdigit() else 0.5 * 2 ** (attempt - 1)
            time.sleep(min(delay, 30.0))
            continue
        if response.status_code >= 400:
            raise PermanentHTTPError(f"HTTP {response.status_code}: {response.text[:200]}")
        try:
            return response.json()
        except ValueError as exc:
            raise PermanentHTTPError("response body was not JSON") from exc
    raise TransientHTTPError("unreachable")


# --------------------------------------------------------------------------- #
# 2. Response-shape discovery (data mapping)
# --------------------------------------------------------------------------- #
_ENVELOPE_KEYS = ("data", "results", "items", "products", "payload")


def extract_city_groups(payload: Any) -> dict[str, list[dict[str, Any]]]:
    """Normalise the three response shapes a vendor API realistically returns.

    A) flat list of products, each carrying its city   [{"city": "Paris", ...}, ...]
    B) envelope object                                  {"data": [...A...]}
    C) map of city -> list of products                  {"Paris": [{...}, ...]}
    D) list of city objects with nested products        [{"city": "Paris", "products": [...]}]
    """
    if isinstance(payload, list):
        if payload and all(isinstance(row, Mapping) for row in payload) and any(
            isinstance(row.get("products"), list) or isinstance(row.get("items"), list) for row in payload
        ):
            groups: dict[str, list[dict[str, Any]]] = {}
            for row in payload:  # shape D
                city = str(row.get("city") or row.get("name") or "UNKNOWN")
                items = row.get("products") if isinstance(row.get("products"), list) else row.get("items", [])
                groups.setdefault(city, []).extend(item for item in items if isinstance(item, Mapping))
            return groups
        groups = {}
        for row in payload:  # shape A
            if not isinstance(row, Mapping):
                continue
            city = str(row.get("city") or "UNKNOWN")
            groups.setdefault(city, []).append(row)
        return groups

    if isinstance(payload, Mapping):
        for key in _ENVELOPE_KEYS:
            if key in payload:
                return extract_city_groups(payload[key])
        groups = {}
        for city, items in payload.items():  # shape C
            if isinstance(items, Mapping):  # tolerate {"Paris": {"products": [...]}}
                items = items.get("products", items.get("items", []))
            if not isinstance(items, Iterable) or isinstance(items, (str, bytes)):
                continue
            groups[str(city)] = [item for item in items if isinstance(item, Mapping)]
        return groups

    raise PermanentHTTPError(f"unsupported response shape: {type(payload).__name__}")


# --------------------------------------------------------------------------- #
# 3. Normalisation + ranking
# --------------------------------------------------------------------------- #
_PRICE_NOISE = "$€£₹ ,"


def to_decimal(value: Any) -> Decimal | None:
    """Parse 1299, '1,299.00', '$1299', 1299.5 into a Decimal. None if unusable."""
    if isinstance(value, bool) or value is None:
        return None
    if isinstance(value, Decimal):
        return value
    if isinstance(value, (int, float)):
        return Decimal(str(value))
    if isinstance(value, str):
        cleaned = value.strip()
        for ch in _PRICE_NOISE:
            cleaned = cleaned.replace(ch, "")
        if not cleaned:
            return None
        try:
            return Decimal(cleaned)
        except InvalidOperation:
            return None
    return None


def top_products_per_city(
    payload: Any,
    limit: int = 3,
    *,
    strict: bool = False,
) -> tuple[dict[str, list[dict[str, Any]]], dict[str, int]]:
    """Return ({city: top-N products}, warnings) from a raw API payload.

    Ordering: price DESC, then name ASC, then id ASC -- deterministic on ties.
    Rows without a usable numeric price are skipped and counted (or raise in strict mode).
    """
    groups = extract_city_groups(payload)
    warnings = {"rows_skipped_missing_price": 0, "rows_skipped_missing_name": 0, "cities_with_fewer_than_limit": 0}
    result: dict[str, list[dict[str, Any]]] = {}

    for city, rows in groups.items():
        parsed: list[dict[str, Any]] = []
        for row in rows:
            price = to_decimal(row.get("price"))
            name = row.get("name") or row.get("product") or row.get("title")
            if price is None:
                if strict:
                    raise ValueError(f"{city}: row has no usable price: {row!r}")
                warnings["rows_skipped_missing_price"] += 1
                continue
            if not isinstance(name, str) or not name.strip():
                if strict:
                    raise ValueError(f"{city}: row has no usable name: {row!r}")
                warnings["rows_skipped_missing_name"] += 1
                name = f"(unnamed:{row.get('id', '?')})"
            parsed.append(
                {
                    "id": row.get("id"),
                    "name": name.strip(),
                    "price": price,
                }
            )

        parsed.sort(key=lambda item: (-item["price"], item["name"].casefold(), str(item["id"])))
        if len(parsed) < limit:
            warnings["cities_with_fewer_than_limit"] += 1
        result[city] = parsed[:limit]

    return result, warnings


def as_jsonable(result: Mapping[str, Sequence[Mapping[str, Any]]]) -> dict[str, list[dict[str, Any]]]:
    """Decimal is not JSON serialisable: render money as a 2dp string."""
    return {
        city: [
            {"id": item.get("id"), "name": item["name"], "price": f"{item['price']:.2f}"}
            for item in items
        ]
        for city, items in result.items()
    }


# --------------------------------------------------------------------------- #
# 4. Mock API (so the example runs with zero credentials)
# --------------------------------------------------------------------------- #
MOCK_PAYLOAD: dict[str, list[dict[str, Any]]] = {
    "Amsterdam": [
        {"id": "p1", "name": "Laptop Stand", "price": 1299},
        {"id": "p2", "name": "Laptop Sleeve", "price": "1,299.00"},
        {"id": "p3", "name": "Notebook", "price": "$45.50"},
        {"id": "p4", "name": "Monitor", "price": 899.99},
    ],
    "Paris": [
        {"id": "p5", "name": "Printer", "price": "not-a-price"},
        {"id": "p6", "name": "Desk Lamp", "price": 75},
        {"id": "p7", "name": "Chair", "price": 450.0},
    ],
    "Tokyo": [
        {"id": "p8", "name": "Keyboard", "price": 120.0},
        {"id": "p9", "name": "Keyboard Pro", "price": 120.0},
        {"id": "p10", "name": "Mouse", "price": 60},
    ],
}


class _MockHandler(BaseHTTPRequestHandler):
    payload: Any = {"data": []}
    fail_first: int = 0
    calls = 0

    def do_GET(self) -> None:  # noqa: N802
        type(self).calls += 1
        if type(self).fail_first > 0:  # simulate provider flakiness once
            type(self).fail_first -= 1
            self.send_response(503)
            self.send_header("Retry-After", "1")
            self.end_headers()
            self.wfile.write(b'{"error":"temporarily unavailable"}')
            return
        body = json.dumps(type(self).payload).encode()
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, *args: Any) -> None:  # keep the demo output clean
        return


def serve_mock(payload: Any = None, fail_first: int = 0) -> tuple[str, ThreadingHTTPServer]:
    _MockHandler.payload = payload if payload is not None else {"data": MOCK_PAYLOAD}
    _MockHandler.fail_first = fail_first
    server = ThreadingHTTPServer(("127.0.0.1", 0), _MockHandler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    host, port = server.server_address[:2]
    return f"http://{host}:{port}/products", server


# --------------------------------------------------------------------------- #
# 5. Tests
# --------------------------------------------------------------------------- #
def selftest() -> None:
    payload = {"data": MOCK_PAYLOAD}
    result, warnings = top_products_per_city(payload)

    # 1299 == "1299.00" is a real tie, so the name tie-break decides: "sleeve" < "stand"
    assert [p["name"] for p in result["Amsterdam"]] == ["Laptop Sleeve", "Laptop Stand", "Monitor"], result["Amsterdam"]
    assert [p["price"] for p in result["Amsterdam"]] == [Decimal(1299), Decimal("1299.00"), Decimal("899.99")]
    # tie on price -> name breaks it deterministically
    assert [p["name"] for p in result["Tokyo"]] == ["Keyboard", "Keyboard Pro", "Mouse"]
    # unusable price skipped, warning counted; a 2-product city is allowed
    assert [p["name"] for p in result["Paris"]] == ["Chair", "Desk Lamp"]
    assert warnings == {
        "rows_skipped_missing_price": 1,
        "rows_skipped_missing_name": 0,
        "cities_with_fewer_than_limit": 1,
    }, warnings

    # string prices must never be compared as strings: "9" > "1000"
    assert to_decimal("9") < to_decimal("1000")

    # flat list shape and map shape produce the same ranking
    flat = [{"city": c, **row} for c, rows in MOCK_PAYLOAD.items() for row in rows]
    by_map, _ = top_products_per_city(MOCK_PAYLOAD)
    by_flat, _ = top_products_per_city(flat)
    assert as_jsonable(by_map) == as_jsonable(by_flat)

    # nested city objects
    nested = [{"city": c, "products": rows} for c, rows in MOCK_PAYLOAD.items()]
    by_nested, _ = top_products_per_city(nested)
    assert as_jsonable(by_nested) == as_jsonable(by_map)

    # strict mode surfaces bad data instead of hiding it
    try:
        top_products_per_city(MOCK_PAYLOAD, strict=True)
    except ValueError as exc:
        assert "Paris" in str(exc), exc
    else:  # pragma: no cover
        raise AssertionError("strict mode should have raised")

    # empty payload is not a crash
    assert top_products_per_city({"data": []})[0] == {}
    print("selftest: all assertions passed")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--url", help="endpoint returning products")
    parser.add_argument("--mock", action="store_true", help="start a local mock API and query it")
    parser.add_argument("--selftest", action="store_true")
    parser.add_argument("--limit", type=int, default=3)
    args = parser.parse_args()

    if args.selftest:
        selftest()
        return

    server = None
    url = args.url
    if args.mock or not url:
        url, server = serve_mock(fail_first=1)  # first call returns 503 so retries are exercised
        print(f"mock API listening on {url}")

    try:
        payload = fetch_json_urllib(url, timeout=(3.0, 10.0), max_attempts=4)
        result, warnings = top_products_per_city(payload, limit=args.limit)
        print(json.dumps({"result": as_jsonable(result), "warnings": warnings}, indent=2))
    finally:
        if server is not None:
            server.shutdown()


if __name__ == "__main__":
    main()
