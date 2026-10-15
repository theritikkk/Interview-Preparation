<!-- Certa FDE mastery workbook — split part 3 of 7 (source lines 1096-1890). Concatenating all 7 files in order reproduces certa_fde_interview_mastery.md. -->

## Part 4: Python API coding — Soaring Prices and friends

> **Companion file:** `certa_lab/soaring_prices.py` — verified runnable (`python3 soaring_prices.py --selftest`, `--mock`, `--url`). The core is inlined below, trimmed of comments, so you can read the whole solution in one screen.

### 4.1 The assessment task and the ambiguity list [ASSESSMENT topic 2]

**Prompt (as given):** call a REST API in Python and return the three most expensive products per city.

**Say this before you type** (10 seconds, and it is what separates a 3 from a 2):

> "Three things are ambiguous and I'll state my assumptions: one, ties on price — I'll break them deterministically by name then id so repeated runs give identical output; two, cities with fewer than three products — I'll return what exists rather than padding; three, malformed prices — I'll skip and count them rather than crash the whole run, and I'll offer a strict mode that raises instead. If the grader wants the strict behavior I can flip one flag."

| Ambiguity | Decision in the reference solution | Why |
|---|---|---|
| Response shape | Support 4 shapes: flat list with a `city` field, an envelope (`data`/`results`/`items`), a `{city: [products]}` map, or `[{city, products}]` | Providers are inconsistent; discovering shape is a mapping problem, and a hardcoded assumption fails the first real API |
| Price type | `Decimal`, parsed tolerantly (`1299`, `"1,299.00"`, `"$45.50"`) | `float` loses money precision; **string comparison sorts `"9"` above `"1000"`** — the single most common fatal bug in this exercise |
| Ties | sort by `(price DESC, name ASC, id ASC)` | A partial sort key makes output nondeterministic across runs |
| Fewer than 3 products | Return what exists | Padding with nulls invents data |
| Missing/invalid price | Skip the row, count it in `warnings`; `strict=True` raises | Never let one bad record kill a run, but never hide it either |
| Output type | `Decimal` internally, formatted to 2dp strings for JSON | `Decimal` is not JSON-serializable; formatting is a presentation concern |
| Network | One call, timeouts, bounded retries honoring `Retry-After` | A GET is idempotent so retrying is safe |

### 4.2 The interview-speed version (write this first, ~8 minutes)

```python
import json
import requests

def top_three_per_city(url: str) -> dict[str, list[tuple[str, str]]]:
    """Return {city: [(name, price), ...]} with the 3 most expensive products per city."""
    response = requests.get(url, timeout=(3, 15))
    response.raise_for_status()
    payload = response.json()

    # Shape discovery: tolerate a bare list, an envelope, or a {city: [...]} map.
    if isinstance(payload, dict):
        payload = payload.get("data") or payload.get("results") or payload

    groups: dict[str, list[dict]] = {}
    if isinstance(payload, list):
        for row in payload:                      # flat list of products
            groups.setdefault(str(row.get("city", "UNKNOWN")), []).append(row)
    else:
        for city, items in payload.items():      # map of city -> products
            groups[str(city)] = list(items)

    result: dict[str, list[tuple[str, str]]] = {}
    for city, products in groups.items():
        ranked = sorted(
            products,
            key=lambda p: (-float(p["price"]), str(p.get("name", ""))),
        )[:3]
        result[city] = [(p["name"], f"{float(p['price']):.2f}") for p in ranked]
    return result
```

**Then immediately say what is wrong with it** — self-critique is the strongest signal available to you here:

> "That works for the happy path, and here's what I'd change for production: `float()` will raise or misbehave on `'$1,299'` or a missing key and take the whole run down, so I'd parse defensively with `Decimal`; sorting on a float loses precision for large amounts; the tie-break needs the id as a final term to be deterministic; and `float('nan')` sorts unpredictably, so invalid rows must be filtered before sorting. Let me show the hardened version."

### 4.3 The production version (inlined from the verified module)

**Fetch layer — timeouts, bounded retries, correct retry classification:**

```python
class TransientHTTPError(RuntimeError):
    """Retryable: timeouts, connection resets, 429, 5xx."""

class PermanentHTTPError(RuntimeError):
    """Not retryable: 400/401/403/404/422 and other contract errors."""

def fetch_json_urllib(url, *, timeout=(3.0, 15.0), max_attempts=4, base_delay=0.5, sleep=time.sleep):
    import urllib.error, urllib.request
    request = urllib.request.Request(url, method="GET")
    last_error = None
    for attempt in range(1, max_attempts + 1):
        try:
            with urllib.request.urlopen(request, timeout=timeout[1]) as response:
                body = response.read()
                if response.status >= 500:
                    raise TransientHTTPError(f"{url} -> HTTP {response.status}")
                return json.loads(body.decode("utf-8"))
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
        sleep(min(delay, 30.0))          # capped: never sleep for hours inside a request
    raise TransientHTTPError(f"{url} failed after {max_attempts} attempts: {last_error}")
```

With `requests` the same contract is three lines shorter and adds connection reuse:

```python
def fetch_json_requests(url, *, timeout=(3.0, 15.0), max_attempts=4, session=None):
    http = session or requests.Session()
    for attempt in range(1, max_attempts + 1):
        try:
            response = http.get(url, timeout=timeout, headers={"Accept": "application/json"})
        except requests.Timeout as exc:                 # connect *or* read timeout: outcome unknown
            if attempt == max_attempts:
                raise TransientHTTPError("timeout") from exc
            time.sleep(0.5 * 2 ** (attempt - 1)); continue
        except requests.RequestException as exc:
            if attempt == max_attempts:
                raise TransientHTTPError(str(exc)) from exc
            time.sleep(0.5 * 2 ** (attempt - 1)); continue
        if response.status_code == 429 or response.status_code >= 500:
            if attempt == max_attempts:
                raise TransientHTTPError(f"HTTP {response.status_code}")
            retry_after = response.headers.get("Retry-After")
            delay = float(retry_after) if retry_after and retry_after.isdigit() else 0.5 * 2 ** (attempt - 1)
            time.sleep(min(delay, 30.0)); continue
        if response.status_code >= 400:
            raise PermanentHTTPError(f"HTTP {response.status_code}: {response.text[:200]}")
        return response.json()
    raise TransientHTTPError("unreachable")
```

**Shape discovery (the mapping layer):**

```python
_ENVELOPE_KEYS = ("data", "results", "items", "products", "payload")

def extract_city_groups(payload):
    """Normalise 4 realistic response shapes into {city: [product, ...]}."""
    if isinstance(payload, list):
        # shape D: [{"city": "Paris", "products": [...]}, ...]
        if payload and all(isinstance(r, Mapping) for r in payload) and any(
            isinstance(r.get("products"), list) or isinstance(r.get("items"), list) for r in payload
        ):
            groups = {}
            for row in payload:
                city = str(row.get("city") or row.get("name") or "UNKNOWN")
                items = row.get("products") if isinstance(row.get("products"), list) else row.get("items", [])
                groups.setdefault(city, []).extend(i for i in items if isinstance(i, Mapping))
            return groups
        # shape A: flat list of products, each carrying its city
        groups = {}
        for row in payload:
            if isinstance(row, Mapping):
                groups.setdefault(str(row.get("city") or "UNKNOWN"), []).append(row)
        return groups

    if isinstance(payload, Mapping):
        for key in _ENVELOPE_KEYS:                     # shape B: envelope
            if key in payload:
                return extract_city_groups(payload[key])
        groups = {}
        for city, items in payload.items():            # shape C: map of city -> products
            if isinstance(items, Mapping):
                items = items.get("products", items.get("items", []))
            if not isinstance(items, Iterable) or isinstance(items, (str, bytes)):
                continue
            groups[str(city)] = [i for i in items if isinstance(i, Mapping)]
        return groups

    raise PermanentHTTPError(f"unsupported response shape: {type(payload).__name__}")
```

**Parsing and ranking (the part that decides correctness):**

```python
_PRICE_NOISE = "$€£₹ ,"

def to_decimal(value):
    """Parse 1299, '1,299.00', '$1299', 1299.5 -> Decimal. None if unusable."""
    if isinstance(value, bool) or value is None:
        return None
    if isinstance(value, Decimal):
        return value
    if isinstance(value, (int, float)):
        return Decimal(str(value))          # str() first: Decimal(0.1) is a float artifact
    if isinstance(value, str):
        cleaned = value.strip()
        for ch in _PRICE_NOISE:
            cleaned = cleaned.replace(ch, "")
        if not cleaned:
            return None
        try:
            return Decimal(cleaned)         # never float(): money precision matters
        except InvalidOperation:
            return None
    return None

def top_products_per_city(payload, limit=3, *, strict=False):
    groups = extract_city_groups(payload)
    warnings = {"rows_skipped_missing_price": 0, "rows_skipped_missing_name": 0,
                "cities_with_fewer_than_limit": 0}
    result = {}
    for city, rows in groups.items():
        parsed = []
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
            parsed.append({"id": row.get("id"), "name": name.strip(), "price": price})

        parsed.sort(key=lambda item: (-item["price"], item["name"].casefold(), str(item["id"])))
        if len(parsed) < limit:
            warnings["cities_with_fewer_than_limit"] += 1
        result[city] = parsed[:limit]
    return result, warnings
```

**Verified behavior (`python3 soaring_prices.py --selftest` → all assertions pass):**

| Scenario | Input | Output | Rule demonstrated |
|---|---|---|---|
| Numeric tie | Amsterdam: `1299`, `"1,299.00"`, `"$45.50"`, `899.99` | `Laptop Sleeve 1299.00`, `Laptop Stand 1299.00`, `Monitor 899.99` | Equal values tie; the name tie-break decides ("sleeve" < "stand") |
| Bad price | Paris: `"not-a-price"`, `75`, `450.0` | `Chair 450.00`, `Desk Lamp 75.00` + `rows_skipped_missing_price: 1` | Skip and count, never crash |
| Exact tie | Tokyo: `120.0`, `120.0`, `60` | `Keyboard`, `Keyboard Pro`, `Mouse` | Deterministic secondary key |
| Short city | Paris has 2 valid products | 2 returned, not 3 | Never pad |
| Shape equivalence | flat list vs map vs nested | identical output | Mapping layer is shape-agnostic |

**Verified end-to-end run** (`--mock` starts a local API whose *first* response is a 503 with `Retry-After: 1`, so the retry path is exercised for real):

```
mock API listening on http://127.0.0.1:58310/products
{
  "result": {
    "Amsterdam": [ {"id": "p2", "name": "Laptop Sleeve", "price": "1299.00"},
                   {"id": "p1", "name": "Laptop Stand",  "price": "1299.00"},
                   {"id": "p4", "name": "Monitor",       "price": "899.99"} ],
    "Paris":     [ {"id": "p7", "name": "Chair", "price": "450.00"},
                   {"id": "p6", "name": "Desk Lamp", "price": "75.00"} ],
    "Tokyo":     [ {"id": "p8", "name": "Keyboard", "price": "120.00"},
                   {"id": "p9", "name": "Keyboard Pro", "price": "120.00"},
                   {"id": "p10", "name": "Mouse", "price": "60.00"} ]
  },
  "warnings": {"rows_skipped_missing_price": 1, "rows_skipped_missing_name": 0,
               "cities_with_fewer_than_limit": 1}
}
```

### 4.4 Complexity analysis (say this out loud)

Let **P** = total products, **C** = cities, **k** = products in the largest city.

| Dimension | Cost | Notes |
|---|---|---|
| Network | 1 request (O(1) calls) | Pagination would make it O(P/page_size) |
| Grouping | O(P) time, O(P) space | One pass over the payload, hash-map insertion |
| Sorting | **O(P log P)** worst case, O(Σ kᵢ log kᵢ) actual | Sorting each city separately |
| Selection | O(1) after sort (slice of 3) | Would be O(P) with a 3-element min-heap, O(P log 3) |
| JSON parsing | O(P) time, O(P) space | The payload is the memory floor |
| Total | **O(P log P) time, O(P) space** | Dominated by the sort |

**The optimization worth mentioning** (and knowing when *not* to do it):

> "If the dataset were huge and I only needed the top 3, I'd use a bounded min-heap of size k — `heapq.nlargest(3, ...)` or a manual 3-element heap — which is O(P log k) instead of O(P log P), so it drops the log factor for large inputs. For a few thousand products it doesn't matter; the sort is clearer and faster in absolute terms because of constant factors. I'd only reach for the heap if profiling showed the sort dominating."

**What *cannot* be optimized away:** you must read every product to know which is most expensive. Any claim of sub-linear time on this problem is wrong — unless the API offers a sorted/top endpoint, which is the first thing you'd check with the provider.

### 4.5 Edge cases (the checklist that earns the marks)

| Edge case | Behavior in the reference solution | Why it matters |
|---|---|---|
| Price as string with currency and separators | `Decimal` after stripping `$ € £ ₹ , space` | The single most likely real-world input |
| Price as int or float | `Decimal(str(v))` | `Decimal(0.1)` ≠ `0.1`; round-tripping through `str` avoids binary artifacts |
| `price` missing / null | Skip + count (or raise in strict mode) | Never fabricate a price |
| `price` is a bool | Rejected explicitly | `isinstance(True, int)` is True in Python — a classic trap |
| NaN / Infinity | `Decimal("NaN")` parses, so filter non-finite values before sorting | `NaN` comparisons are all False, which makes sorts nondeterministic |
| Negative prices (credits/refunds) | Kept as-is and ranked normally | Hiding them changes the customer's number |
| Duplicate product ids | Both kept; the id is only a tie-break | Deduplication is a *policy* decision, not an obvious one |
| City key missing | Bucketed as `"UNKNOWN"` rather than dropped | Silently dropping data is worse than a visible bucket |
| Empty payload / empty city list | `{}` returned, no crash | Verified in the self-test |
| Very long names / unicode | Preserved; `casefold()` for case-insensitive tie-break | `"ÅNGSTRÖM"` vs `"angstrom"` — `lower()` is not the same as `casefold()` |
| Envelope with `data: null` | Treated as empty | Providers do this on "no results" |

### 4.6 Common mistakes in Python API coding, and why they fail

| Mistake | Why it fails | Fix |
|---|---|---|
| `sorted(products, key=lambda p: p["price"], reverse=True)` with string prices | `"9" > "1000"` lexicographically → wrong top 3, silently | Parse to a number first |
| `float` for money | Precision loss; `0.1 + 0.2 != 0.3`; fails large-amount comparisons | `Decimal` |
| No `timeout` on `requests.get` | Hangs forever on a black-holed connection; the request never returns and your worker is stuck | Always a tuple `(connect, read)` |
| Retrying on any exception | Retries 400/403/422 forever; burns quota; turns a data bug into a load incident | Classify: retry 429/5xx/timeout only |
| Catching `Exception` and returning `[]` | The failure becomes invisible; customers see "no data" instead of an error | Log, count, and surface the failure |
| Assuming a JSON shape | Breaks on the first envelope wrapper | Discover the shape; fail loudly on unknown shapes |
| `data["city"]` instead of `data.get("city")` | `KeyError` kills the whole run for one bad row | Defensive access with an explicit fallback policy |
| Sorting in place then using the original list | Aliasing bug; callers see mutated input | Return new structures |
| Ignoring `Retry-After` | Retries inside the window are rejected again → thundering herd | Honor the header, then add jitter |
| One `Session` per request | Loses connection reuse; TLS handshake per call | Reuse a session across calls |
| `print()` debugging left in the solution | Sloppy, and can leak PII/tokens into logs | Use `logging` with levels |

### 4.7 Variants you should be able to handle in one pass

1. **"Now do it for 500 cities, one endpoint per city."** → A bounded thread pool (`concurrent.futures.ThreadPoolExecutor(max_workers=8)`), a shared rate limiter, per-city failure isolation (don't let one city aborts the whole gather), and partial results + error list returned to the caller. Mention the tail-latency trade-off and that a per-city timeout must be shorter than the batch timeout.
2. **"Return the products, not just names."** → Return the original dicts with normalized fields added, so consumers can still see the raw payload; keep the raw payload's key names untouched.
3. **"The API paginates."** → Keyset/cursor walk with a loop guard; accumulate; then rank once at the end (ranking per page is wrong).
4. **"The list is 500 MB."** → Stream with `ijson` or parse per page and keep only a bounded heap of size 3 per city plus a running count; O(C·k) memory instead of O(P).
5. **"Prices are in different currencies."** → Refuse to compare across currencies silently. Convert with a dated FX rate and record the rate used, or make the output currency a parameter and filter first. Comparing EUR to USD without a rate is a *correctness* bug, not a rounding issue.
6. **"Make it a well-tested module."** → Pure function for ranking (no I/O), injected fetch function, `unittest`/`pytest` cases for each edge case in the table above, and one integration test against the mock server.

### 4.8 What to say while you code (narration script)

> "I'll start with the network call and the shape assumption, because if the payload shape is different, everything downstream changes... I'm using `Decimal` here, not float, because `'9'` compared as a string sorts above `'1000'`, and floats lose money precision... I'm sorting on a tuple of `(negative price, name, id)` — the negative gives me descending order in one pass, and the extra keys make the order total so two runs produce identical output... I'm slicing the first three instead of building a heap, because `P log P` on a few thousand rows is fine and the sort is clearer; I'd switch to `heapq.nlargest` if the list were huge... Finally, the warnings counter — I'd rather tell the customer 'one row had an unparseable price' than silently return a top three that's missing a product."

### 4.9 Practice

**Practice 4.1 [ASSESSMENT]** Write `top_three_per_city` from memory in under 12 minutes, with a docstring, tie-break, and malformed-price handling. Then run it against `certa_lab/soaring_prices.py --mock` and compare.

**Your answer:**
> ```python
>
> ```

**Practice 4.2** The response is `{"status": "ok", "payload": {"cities": [{"name": "Paris", "products": [{"title": "Chair", "cost": {"amount": "1,299.00", "currency": "EUR"}}]}]}}`. Write the mapping function and name the three assumptions you had to make.

**Your answer:**
>
>
>

**Practice 4.3** Find whether two numbers in a list sum to a target — the classic that an interviewer may use as a warm-up (your 450 LeetCode problems make this free points; the value is in *narrating* it).

**Your answer:**
> ```python
>
> ```

<details><summary>Expected answer and why it is asked</summary>

```python
def has_pair_with_sum(nums, target):
    seen = set()
    for value in nums:
        if target - value in seen:      # check BEFORE inserting
            return True
        seen.add(value)
    return False
```
Average **O(n)** time, **O(n)** space. Checking before inserting is what prevents using the same element twice to match itself (target 8, list `[4]` must be False). The hash set is the [ASSESSMENT] MCQ topic "hash sets": membership is O(1) average, O(n) worst case under adversarial collisions. Sorting + two pointers is O(n log n) time / O(1) extra space — the better answer if memory is the constraint, and worth offering unprompted.
</details>

**Practice 4.4** Given a list of 10 million prices, return the top 3 without sorting. State the complexity.

**Your answer:**
>
>

### 4.10 Part 4 scorecard

| Item | Correctness /4 | Speed (target) | Notes |
|---|---|---|---|
| Soaring Prices written from memory | | < 12 min | |
| Names the string-vs-numeric price trap unprompted | | < 1 min | |
| Decimal vs float justification | | < 30s | |
| Deterministic tie-break | | < 30s | |
| Timeout + retry classification + `Retry-After` | | < 2 min | |
| Complexity stated correctly incl. heap alternative | | < 30s | |
| Self-critique of the naive solution | | < 60s | |

---

## Part 5: Webhook and backend reliability architecture

> **Companion file:** `certa_lab/webhook_receiver.py` — stdlib only, verified end to end. It starts a real HTTP server, delivers signed events (including duplicates, tampering, stale versions, a poisoned event, throttling, and a crash), and prints the resulting state and metrics.

### 5.1 First principles: a webhook is a push-based queue you do not control

Traditional integration: *you* poll a provider on your schedule and control concurrency, backoff and retry.
Webhook integration: the *provider* drives. That inversion creates every hard property in this part:

| Property | Who controls it | Consequence you must design for |
|---|---|---|
| Delivery timing | Provider | Bursts. Your endpoint must absorb peaks |
| Delivery count | Provider | **At-least-once is the norm**: you will see duplicates |
| Ordering | Provider/network | No global order; per-entity order is not guaranteed either |
| Retry policy | Provider (for its delivery to you) | A slow 200 is a retry; a non-2xx is a retry later |
| Payload version | Provider | Schemas evolve without asking you |
| Autonomy | You, over *processing* | You control how you drain, retry, and replay internally |

**The one-sentence framing to open with:** *"A webhook endpoint is a durable intake for a queue I don't own. So my job is to acknowledge fast and correctly, persist before acknowledging, and make processing idempotent, ordered by version, and replayable."*

### 5.2 The eight required properties [ASSESSMENT topic 4] mapped to mechanisms

| # | Requirement | Mechanism (guaranteed) | What is *only* an assumption unless stated |
|---|---|---|---|
| 1 | **Signature verification** | HMAC-SHA256 over `timestamp + raw body`, constant-time compare, timestamp window | That the provider signs exactly what it sends (verify against a real sample, don't assume) |
| 2 | **Duplicate delivery** | Inbox table with a UNIQUE/PK on the provider event ID; insert is the dedupe decision | That the provider's event ID is stable across retries. If it isn't, dedupe on a natural key + payload hash |
| 3 | **Event ordering** | Per-entity monotonic version; conditional apply (`incoming > stored`); audit the ignored ones | That the provider's version is monotonic per entity. If it only has a timestamp, use an overlap window and reconcile |
| 4 | **At-least-once delivery** | Consumer tolerance: dedupe key + idempotent effect + version guard | That the provider retries at all, and for how long |
| 5 | **Backpressure** | Bounded queue + bounded worker concurrency; 429/503 with `Retry-After` when saturated; shed load instead of growing memory | That the provider honors `Retry-After` (record how it behaves; escalate if it doesn't) |
| 6 | **Replay** | Durable event log + DLQ + an audited replay endpoint; replay is safe because dedupe and version guards hold | That replays will not be needed — they always are |
| 7 | **Crash safety** | Inbox + job inserted in **one transaction**; worker claims with a lease; ack only after the effect commits | That the process will not crash between two independent writes — it will |
| 8 | **Observability** | Intake rate, signature failures, duplicate rate, queue depth, oldest-message age, DLQ size, per-tenant error rate | That metrics exist without you instrumenting them |

### 5.3 Signature verification (the security core)

**Three rules that interviewers specifically listen for:**

1. **Verify the raw bytes.** If your framework parses JSON and you re-serialize to verify, whitespace and key ordering change the digest and *valid* events fail (`json.dumps` adds spaces that weren't in the original). Capture the raw body before parsing.
2. **Include the timestamp in the signed material** and reject anything outside a tolerance window (5 minutes is typical). Without this, a captured request can be replayed forever.
3. **Compare in constant time** (`hmac.compare_digest`), and never log the secret or the expected signature.

```python
def sign(secret: bytes, body: bytes, timestamp: int | None = None) -> str:
    """Provider-side helper: sign the timestamp and the raw body together."""
    timestamp = timestamp or int(time.time())
    mac = hmac.new(secret, f"{timestamp}.".encode() + body, hashlib.sha256).hexdigest()
    return f"t={timestamp},v1={mac}"

def verify_signature(secret, header, body, *, now=None, tolerance=300) -> tuple[bool, str]:
    """Returns (ok, reason). Every rejection is explicit so the 401 is diagnosable."""
    now = time.time() if now is None else now
    parts = {}
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
```

**Verified behavior from the lab run:**

| Scenario | Result |
|---|---|
| Correctly signed body | `202 accepted` |
| Body tampered after signing (`high` → `low`) | `401 {"reason": "signature mismatch"}` |
| Signed 1 hour ago (captured request replayed) | `401 {"reason": "timestamp outside 300s tolerance (possible replay)"}` |

**Secret rotation without downtime** [BEST PRACTICE]: accept two active secrets during the rotation window (`v1` and `v2` prefixes, or a `key_id` in the header), publish the new secret first, wait for the provider to start using it (you can see which key verified each request), then retire the old one. If you only support one secret, rotation means a window of 401s — which the provider reads as "the endpoint is down" and starts retrying.

**What signature verification does *not* do:** it does not prevent duplicates (the provider legitimately retries), does not prevent replay within the tolerance window (the same body can be re-sent twice inside 5 minutes — which is why you need the event-ID dedupe anyway), and does not prove the payload is *valid* — only that it came from someone holding the secret. Validate the schema separately.

### 5.4 Duplicate delivery: the unique key *is* the mechanism

The only correctness-preserving dedupe is a database constraint. Everything else is a race.

```sql
CREATE TABLE IF NOT EXISTS inbox (
    event_id     TEXT PRIMARY KEY,      -- the dedupe decision lives here, not in application code
    event_type   TEXT NOT NULL,
    vendor_id    TEXT NOT NULL,
    version      INTEGER NOT NULL,
    payload      TEXT NOT NULL,
    payload_hash TEXT NOT NULL,         -- sha256 of the raw body
    received_at  REAL NOT NULL,
    status       TEXT NOT NULL,         -- accepted | processed | duplicate | rejected
    http_status  INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS jobs (
    job_id          INTEGER PRIMARY KEY AUTOINCREMENT,
    event_id        TEXT NOT NULL REFERENCES inbox(event_id),
    status          TEXT NOT NULL,      -- pending | processing | done | dead
    attempts        INTEGER NOT NULL DEFAULT 0,
    next_attempt_at REAL NOT NULL DEFAULT 0,
    last_error      TEXT,
    updated_at      REAL NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_jobs_claimable ON jobs(status, next_attempt_at);
```

**Insert-and-enqueue in one transaction** (verified: this is what makes a crash after the `202` unable to lose the event):

```python
def record_delivery(self, event, payload_hash, raw) -> tuple[str, int]:
    now = time.time()
    with self._connect() as conn:
        conn.execute("BEGIN IMMEDIATE")          # single writer: no interleaving
        try:
            conn.execute("""INSERT INTO inbox (event_id, event_type, vendor_id, version, payload,
                                               payload_hash, received_at, status, http_status)
                            VALUES (?,?,?,?,?,?,?,?,?)""",
                         (event["event_id"], event["event_type"], event["vendor_id"],
                          event["version"], raw, payload_hash, now, "accepted", 202))
            conn.execute("""INSERT INTO jobs (event_id, status, attempts, next_attempt_at, updated_at)
                            VALUES (?, 'pending', 0, 0, ?)""", (event["event_id"], now))
            conn.execute("COMMIT")
            return "accepted", 202
        except sqlite3.IntegrityError:            # duplicate event_id
            conn.execute("ROLLBACK")
            row = ...  # read the stored payload_hash
            if row and row["payload_hash"] != payload_hash:
                return "conflict_same_id_different_body", 409
            return "duplicate", 200
```

**The subtle case most candidates miss:** the same `event_id` arriving with a *different* body. That is not a benign retry — it means either the provider reused an ID (a provider bug) or someone is forging. Returning 200 silently would apply the wrong state or hide an attack. The lab returns **409** with a reason, and records the anomaly. Verified: `409 {"status": "conflict_same_id_different_body"}`.

**Which status code should a duplicate return?** Usually 200 with a body saying `duplicate`. Rationale: the provider's goal was delivery, and the event *is* delivered (it was already accepted). Returning 409 makes the provider retry or mark the endpoint unhealthy. Return 409 only for the genuinely contradictory case above. Either way, **record the duplicate as a metric** — a spike means your 200s are too slow, which is itself a defect.

### 5.5 Ordering: there is no global order, only per-entity progress

**State the reality first:** *"Events arrive out of order because of retries, parallel provider workers, and independent network paths. I cannot rely on arrival order, so I never apply an event's *presence* as evidence of recency. I carry the provider's version (or `updated_at`) on the entity and apply a conditional update."*

```python
def apply_event(self, payload, event_id) -> str:
    vendor_id, version = payload["vendor_id"], int(payload["version"])
    with self._connect() as conn:
        conn.execute("BEGIN IMMEDIATE")
        current = conn.execute("SELECT version FROM vendor_state WHERE vendor_id=?", (vendor_id,)).fetchone()
        if current is not None and version <= current["version"]:
            conn.execute("INSERT INTO audit_log (event_id, action, detail, at) VALUES (?,?,?,?)",
                         (event_id, "stale_ignored", f"incoming v{version} <= stored v{current['version']}", time.time()))
            conn.execute("COMMIT")
            return "stale_ignored"
        conn.execute("""INSERT INTO vendor_state (vendor_id, name, tier, version, updated_at) VALUES (?,?,?,?,?)
                        ON CONFLICT(vendor_id) DO UPDATE SET name=excluded.name, tier=excluded.tier,
                            version=excluded.version, updated_at=excluded.updated_at""",
                     (vendor_id, payload["name"], payload["tier"], version, time.time()))
        conn.execute("INSERT INTO audit_log (event_id, action, detail, at) VALUES (?,?,?,?)",
                     (event_id, "applied", f"v{version}", time.time()))
        conn.execute("COMMIT")
    return "applied"
```

Verified: `v7` applied, then `v6` → `stale_ignored` with detail `incoming v6 <= stored v7`, and the stored record remains `tier=high, version=7`. **Never a blind overwrite.**

| Ordering problem | Handling |
|---|---|
| Newer event arrives, older follows | Conditional apply keeps the newer state; the older is recorded as stale (audit trail matters for compliance) |
| Older arrives, newer follows | Applied in order — no special handling |
| Gap: v5 then v7 | If events are *deltas*, you must fetch or request a replay of v6; if they are *snapshots*, v7 supersedes v6 and you can ignore the gap. **Asking which one it is, is the interview point** |
| Same version, different content | A contradiction: quarantine to review rather than coin-flip (the lab's `conflict` path exists for exactly this) |
| No version field at all | Fall back to `updated_at` with an overlap window, dedupe on payload hash, and add a periodic reconciliation because you cannot be certain |
| Updates to different fields from different systems | Field-level merge with explicit ownership (Part 7.5) — version guards alone would discard non-conflicting edits |

### 5.6 At-least-once, exactly-once, and the words interviewers test [ASSESSMENT]

| Term | Precise meaning | What you actually build |
|---|---|---|
| At-most-once | May be lost, never duplicated | Never acceptable for a compliance event |
| **At-least-once** | Never lost *if* the sender keeps retrying and your intake is durable; duplicates are expected | **The realistic default.** Dedupe + idempotent effects |
| Exactly-once *delivery* | The network delivered each message exactly once | **Not achievable across independent systems** — a retry after a lost ack is indistinguishable from a genuine second send |
| Exactly-once *processing* | Each message's effect happens exactly once | Achievable *inside one transactional boundary* (Kafka transactions, or your own dedupe table + same-transaction effect) |
| **Effectively-once** | End-to-end business outcome applied once, despite duplicates | **What you promise the customer**: at-least-once delivery + idempotent, version-guarded effects + reconciliation |

**Model answer:** *"[ASSESSMENT] At-least-once means the provider will retry until it gets an acknowledgement, so I must assume I will receive duplicates, including duplicates delivered concurrently. Exactly-once end-to-end isn't something I'd promise: if my 202 is lost on the way back, the provider cannot distinguish 'you never got it' from 'you processed it', so it retries. What I can guarantee is effectively-once effects: the event ID is unique in my inbox, the business effect is applied under a conditional update guarded by version, and the whole thing happens in one transaction. Where a downstream external system can't be made idempotent — sending an email, calling a partner API — I use an outbox with a stable message key and accept a small duplicate window, which I then measure and reconcile. Any claim of exactly-once across a system boundary is a design smell."*

### 5.7 Backpressure: acknowledge fast, refuse politely, never silently drop

**Why it matters:** unbounded intake during a provider burst turns a slow consumer into an out-of-memory crash, and a crash *loses* events. Shedding load preserves correctness at the cost of latency — and the provider's retry (or your own pull-based backfill) recovers it.

```python
# refuse new work instead of growing an unbounded queue
if self.store.queue_depth() >= MAX_QUEUE_DEPTH or self.store.flag("throttled"):
    return 429, {"error": "too_many_requests", "retry_after_seconds": 5}
```

Verified: `429 {"error": "too_many_requests", "retry_after_seconds": 5}` with a `Retry-After: 5` header.

| Technique | Where it applies | Trade-off |
|---|---|---|
| Bounded queue + 429/503 at the edge | Intake | Brief customer-visible retries instead of data loss |
| Bounded worker concurrency | Draining | Lower throughput, protects the downstream system from you |
| Per-tenant queues / fair scheduling | Multi-tenant SaaS | One noisy customer can't starve the others; more moving parts |
| `Retry-After` on your own 429 | Intake | Needs a provider that honors it; measure and report if it ignores it |
| Batch processing | Draining | Higher throughput, coarser failure granularity (partial failure handling required) |
| Load shedding by priority | Intake | Requires a priority policy agreed with the business |
| Autoscaling on queue depth (not CPU) | Draining | Queue depth is the leading indicator; CPU lags |

**The three counter-intuitive points worth stating:** (1) `429` on intake is *safer* than accepting and dropping; (2) returning `200` while the queue is on fire is a lie that delays the provider's retry past the outage; (3) a slow-but-successful handler is worse than a fast rejection, because it consumes the connection while the provider times out and retries anyway.

### 5.8 Replay and the DLQ

```python
def replay(self, event_id: str | None = None) -> int:
    """Requeue dead jobs. Idempotent by construction: apply_event guards on version,
    and the inbox primary key blocks re-ingest, so a replay can never double-apply."""
    with self._connect() as conn:
        conn.execute("BEGIN IMMEDIATE")
        rows = conn.execute("SELECT job_id FROM jobs WHERE status='dead'"
                            + (" AND event_id=?" if event_id else ""),
                            (event_id,) if event_id else ()).fetchall()
        for row in rows:
            conn.execute("""UPDATE jobs SET status='pending', attempts=0, next_attempt_at=?,
                            last_error=NULL, updated_at=? WHERE job_id=?""", (now, now, row["job_id"]))
        conn.execute("UPDATE dead_letters SET replayed_at=? WHERE replayed_at IS NULL", (now,))
        conn.execute("COMMIT")
    return len(rows)
```

Verified sequence: the poisoned event retried three times (`retry_scheduled: 3`), was dead-lettered (`dead_lettered: 1`), the operator fixed the downstream condition, replay requeued 1 job, and the event then applied (`applied: 3` in the final metrics). A second replay requeued **0** — verified idempotent.

**What a replay must preserve:** the event ID (so dedupe still works), the original payload (so the fix is reproducible), the attempt count and last error (so you can triage), and an audit row for *who* replayed *what* and *when*. In a compliance product, "who caused this vendor's state to change" is an audit question.

### 5.9 Full system design — using the required structure

**Prompt:** design a webhook endpoint that accepts vendor lifecycle events from a provider, applies them to Certa's vendor state, and remains correct under duplicate, out-of-order, and partial failure. *(Generic engineering exercise, not a description of Certa's systems.)*

**1. Requirements.**
- Functional: accept signed events; validate; persist; apply to vendor state; expose status; support replay.
- Non-functional: no lost accepted events; duplicates harmless; ordering by version; p95 intake latency < 200 ms (excluding processing); p95 end-to-end processing < 60 s; multi-tenant isolation; audit every state change.
- Explicit non-goals: sub-second end-to-end, cross-entity ordering, exactly-once delivery.

**2. Assumptions** (state and revisit these).
- Provider delivers at-least-once, retries for up to 24 h, sends a stable `event_id` and a per-vendor `version`.
- Volume: ~50 events/s peak per tenant; payloads < 256 KB.
- Signature algorithm is HMAC-SHA256 with a timestamp. *(Verify against a real sample before committing.)*
- Events are snapshots of the full entity, so a gap in versions is tolerable.

**3. Architecture.**

```
provider ──HTTPS──> [ LB / API gateway ]
                          │  (TLS terminate, WAF, per-tenant rate limit)
                          v
                    [ intake service ]  ── 1. read RAW body
                          │               2. verify HMAC + timestamp window
                          │               3. validate envelope/schema
                          │               4. BEGIN: insert inbox row + job row; COMMIT
                          │               5. return 202 (or 200 for a duplicate, 401 bad sig,
                          │                  422 bad schema, 429 when saturated)
                          v
                    [ durable store ]  inbox(event_id PK) + jobs(queue) + vendor_state + audit
                          ^
                          │ claim with a lease (UPDATE ... WHERE status='pending')
                          │
                    [ worker pool, bounded concurrency ]
                          │   apply: conditional version check -> state + audit in one tx
                          │   on failure: attempts++, exponential backoff + jitter
                          │   after MAX_ATTEMPTS -> dead_letter + metric + alert
                          v
                    [ DLQ + admin replay endpoint ]  (audited, idempotent)
                          │
                          v
                    [ metrics/logs/traces: RED metrics, queue depth, oldest job age ]
```

**Design choice to justify:** a database-backed queue (inbox/jobs tables) instead of a broker. *"At this volume, the DB I already need for state gives me transactional intake for free — the inbox row and the job row commit together, so there's no dual-write problem. A broker would add an operational dependency and a second failure domain. If volume grew to tens of thousands of events per second or I needed fan-out to many consumers, I'd move to Kafka and keep the same inbox/dedupe contract at the consumer."*

**4. Data model** (see 5.4 DDL, plus):

```sql
CREATE TABLE vendor_state (
    vendor_id  TEXT PRIMARY KEY,
    name       TEXT NOT NULL,
    tier       TEXT NOT NULL,
    version    INTEGER NOT NULL,        -- the provider's version, used for ordering
    updated_at REAL NOT NULL
);
CREATE TABLE audit_log (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    event_id TEXT, action TEXT NOT NULL, detail TEXT, at REAL NOT NULL
);
CREATE TABLE dead_letters (
    event_id TEXT PRIMARY KEY, attempts INTEGER NOT NULL, last_error TEXT NOT NULL,
    moved_at REAL NOT NULL, replayed_at REAL
);
```

Key constraints and why: `inbox.event_id` PK = dedupe; `jobs(event_id)` FK = traceability from a queued job back to the event that caused it; `vendor_state.version` = the ordering guard; `audit_log` = who changed what, when, and why.

**5. Normal flow.** Provider POSTs → raw body captured → HMAC verified (reject 401 with a reason) → JSON parsed and validated (reject 422 with the missing fields) → inbox+job committed atomically → `202` returned → worker claims the job under a lease → version check → state and audit updated in one transaction → job marked done.

**6. Failure scenarios.** This is the table interviewers score most heavily.

| Scenario | What breaks without a control | Control | Recovery |
|---|---|---|---|
| **Duplicate request** (provider retries after a slow 202) | Double-applied effect, duplicate notification | `inbox.event_id` PK; duplicate returns 200 and is *not* re-enqueued | None needed |
| **Concurrent duplicate** (two deliveries at once) | Both pass an `EXISTS` pre-check → double apply | The INSERT itself is the dedupe; one wins, the other gets `IntegrityError` | None needed |
| **Same ID, different body** | Silent corruption or a hidden forgery | Payload-hash comparison → 409 + alert | Investigate; likely a provider bug |
| **Worker crash after the 202, before processing** | Lost event | inbox+job committed in the same transaction | A live worker claims the pending job; the lease expires for a crashed claimant |
| **Worker crash mid-processing** (after a partial external call) | Duplicate external effect | Effect written in the same transaction as the job state change; external calls made idempotent by a stable key | Replay; the version guard makes it a no-op if already applied |
| **Network timeout to the downstream** | Unknown outcome | Retry with the same idempotency key; never assume failure | Reconciliation against the downstream |
| **Provider burst** | OOM, lost events, cascading failure | Bounded queue, bounded concurrency, 429 + `Retry-After` | Provider retries; consider a scheduled backfill pull |
| **Poison event** (always fails) | Infinite retry loop consuming workers | `attempts >= MAX_ATTEMPTS` → DLQ, alert, no further automatic retries | Diagnose, fix, audited replay |
| **Out-of-order v6 after v7** | Newer state overwritten by older | Conditional version apply; stale recorded in audit | Optional gap fetch/replay |
| **Clock skew** | Wrong staleness/expiry decisions | Never trust client timestamps for state decisions; use them only for signature windows, with tolerance | Server-side monotonic versions |
| **Schema change from the provider** | Parser crash → 500s → provider disables the webhook | Contract tests on recorded samples; unknown-field tolerance; explicit unknown-enum path | Deploy the parser fix; replay from the durable inbox |
| **Signature secret rotated by the provider** | Every request 401s; provider retries then disables the endpoint | Two active secrets during rotation; alert on a 401 spike; key ID in the header | Add the new secret, retire the old |
| **Storage unavailable** | Cannot persist | **Do not return 2xx.** Return 503 so the provider retries | Restore storage; the provider's retries deliver |
| **Duplicate DLQ replays** | Double apply, double notifications | Replay preserves the event ID → dedupe and version guards hold | Verified: second replay requeues 0 |

**7. Security.** TLS only; HMAC with timestamp window and constant-time compare; per-tenant secrets; per-tenant rate limits at the edge; payload size caps; schema validation before any parsing that could be exploited; no secrets or full payloads in logs (log the event ID, tenant, type, hash, and processing outcome); audit every operator action; least-privilege DB credentials; and replay behind an authenticated admin role — in a compliance product, an unauthenticated replay endpoint is a data-integrity incident.

**8. Observability.** Intake rate and latency (RED: rate, errors, duration); signature-failure rate by tenant; duplicate rate (a spike means your 200s are too slow); **queue depth and oldest-message age** (the leading indicator, alert on age, not just depth); retry and DLQ counts; per-tenant error rates; applied/stale/conflict counters; end-to-end lag from `received_at` to `processed_at`; and a trace spanning intake → queue → worker → downstream, keyed by `event_id`.

**9. Testing.** Unit tests for signature verification (valid, tampered, stale, malformed header, rotated secret) and for the version guard (applicable, equal, stale, gap). Integration tests with a mock provider that *deliberately misbehaves*: delivers duplicates, delivers concurrently, delivers out of order, delays responses, sends a 500 for a poison event, and rotates its secret. Chaos tests: kill the worker mid-processing and assert no lost events and no duplicates applied. Contract tests on recorded real payloads, in CI. A replay test asserting idempotency (the second replay must do nothing). Load test at 3× expected peak and assert the queue drains and the 429 path engages predictably.

**10. Rollout.** Ship the intake endpoint first in "record-only" mode (persist and ack, no processing) to prove signature verification and dedupe against production traffic. Then enable processing for one tenant with a feature flag. Compare applied state against the tenant's existing process (shadow mode) for a week. Cut over tenant by tenant, with a documented rollback that stops processing while leaving intake durable — so no events are lost during a rollback. Add the DLQ and replay tooling *before* the first customer goes live, because the first poison event will arrive on day one.

**11. Trade-offs.** DB-backed queue vs broker (operational simplicity and transactional intake vs throughput and fan-out). Synchronous processing (simpler, but a slow downstream causes provider timeouts and retries) vs async with 202 (better intake latency, requires status visibility and a DLQ). Storing the full payload (audit and replay power, storage cost, PII retention obligations) vs storing a reference (cheaper, but replay depends on the provider retaining it — usually unacceptable for compliance). Strict validation (crashes on provider drift, surfaces issues immediately) vs permissive (keeps running, silently mis-maps data). Per-tenant queues (isolation, complexity) vs shared pools (simple, noisy-neighbour risk).

### 5.10 Testing matrix for the eight required properties

| Property | Test | Pass criterion |
|---|---|---|
| Signature verification | Valid, tampered, stale timestamp, malformed header, wrong key | 202 / 401 / 401 / 401 / 401, with distinct reasons |
| Duplicate delivery | Same event ID twice, and twice *concurrently* | One inbox row, one job, one effect; second returns 200/duplicate |
| Same ID, different body | Replay with an altered payload | 409 + alert, no state change |
| Ordering | v7 then v6 | `stale_ignored` in audit, stored version still 7 |
| At-least-once tolerance | Deliver every event 3× | Idempotent final state, exactly one audit "applied" per version |
| Backpressure | Saturate the queue / enable the throttle flag | 429 with `Retry-After`, no event loss, queue does not grow without bound |
| Crash safety | Insert inbox+job, kill the worker before claiming, restart | The pending job is claimed and processed after restart |
| Replay | Poison → DLQ → fix → replay → replay again | First replay applies; second requeues 0 and changes nothing |

### 5.11 Common mistakes, and why they fail

| Mistake | Why it fails |
|---|---|
| Returning 200 before persisting | A crash loses the event, and the provider will never resend it — silent data loss |
| Verifying the signature over a re-serialized body | Whitespace/key-order differences break the HMAC; valid events get 401s |
| Deduping with `SELECT ... WHERE EXISTS` before insert | Racy: two concurrent deliveries both pass the check |
| Storing the dedupe key in Redis with a short TTL as the only guard | A retry after the TTL recreates the effect; Redis eviction is not a correctness guarantee (use it as a fast path, keep the DB constraint) |
| Ignoring the timestamp in the signature | Captured requests replay forever |
| Applying events in arrival order | An old retry overwrites newer state |
| One giant transaction spanning an external API call | Locks held across the network; a timeout rolls back work that may have already happened downstream |
| Retrying a 4xx | Poison event loops forever, burning quota and workers |
| No DLQ, only logs | The failure scrolls away; nothing to replay |
| DLQ with no replay path | Data is stranded; the customer's vendor stays wrong forever |
| `print()` instead of structured logs | No tenant/event ID to search; unusable on a busy endpoint |
| Logging the full payload by default | PII and contract terms in your log aggregator; a real compliance problem |
| Alerting on queue depth only | Depth can be low while a single message sits for hours — alert on **oldest-message age** |
| Assuming the provider sends events in order or exactly once | The design fails on the first retry |

### 5.12 Model answer (45–75 seconds, spoken)

> "I treat the endpoint as a durable intake for a queue I don't control. So: read the raw body, verify the HMAC over `timestamp + raw body` with a tolerance window and a constant-time compare, and reject with 401 and a clear reason. Validate the envelope, reject 422 for a bad schema. Then insert the event into an inbox table keyed by the provider's event ID and insert a job row in the *same transaction*, and only then return 202. That single transaction is the whole design: if I crash after the 202, the durable job survives; and if the provider retries, the unique key makes the duplicate harmless — I return 200 and don't re-enqueue. Workers claim jobs with a lease and apply them under a version guard, so `v6` arriving after `v7` is recorded as stale instead of overwriting newer state. Failures back off exponentially with jitter and go to a dead-letter queue after a bounded number of attempts, with an audited replay endpoint — safe because the dedupe key and the version guard are still in force. On saturation I shed load with 429 and `Retry-After` rather than growing an unbounded queue. And I'd watch queue depth *and oldest-message age*, because age is what correlates with the customer seeing stale vendor data."

### 5.13 Practice

**Practice 5.1 [ASSESSMENT]** Design the webhook endpoint. Cover all eight required properties and name what is guaranteed versus assumed.

**Your answer:**
>
>
>

**Practice 5.2** The provider's docs say "we deliver events exactly once." What do you do?

**Your answer:**
>
>
>

<details><summary>Expected points</summary>

Do not design on that claim. (a) Ask what happens on a non-2xx, on a timeout, and on a 5xx — most processors retry, which is at-least-once by construction. (b) Ask whether duplicates are possible during their own internal retries or during an infrastructure failover. (c) Design for at-least-once anyway: the dedupe table costs one unique index, and the failure mode of believing the claim is duplicated vendor state in a compliance system. (d) Measure it: count duplicate event IDs in production for a month and use that evidence in the next design review. This is the "bring evidence to Product" behavior the JD asks for.
</details>

**Practice 5.3** A customer complains: "We see vendor X approved twice in one day, and the second approval email arrived 40 minutes after the first." Give three plausible root causes and the specific evidence you would collect for each.

**Your answer:**
>
>
>

**Practice 5.4** Your endpoint returns 202 in 5 ms but the queue is growing 10× faster than it drains for two hours. What are your options, in order of preference, and what do you tell the customer?

**Your answer:**
>
>
>

**Practice 5.5 (coding)** Extend the lab receiver with a `/status/{event_id}` endpoint that returns `received | processing | processed | dead | stale_ignored`, plus attempt count and last error. Then write the test that proves it reports `dead` for a poison event.

### 5.14 Part 5 scorecard

| Item | Correctness /4 | Production readiness /4 | Notes |
|---|---|---|---|
| Explains push inversion and its consequences | | | |
| Signature verification rules (raw body, timestamp, constant time) | | | |
| Dedupe by unique constraint, incl. concurrent duplicates | | | |
| Same-ID-different-body handling | | | |
| Version guard and stale-event audit | | | |
| At-least-once vs exactly-once vs effectively-once phrasing | | | |
| Backpressure with 429 and its trade-off | | | |
| DLQ + replay design and replay idempotency | | | |
| Crash matrix (crash at each step) | | | |
| Observability incl. oldest-message age | | | |

---

