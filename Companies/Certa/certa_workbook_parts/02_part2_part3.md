<!-- Certa FDE mastery workbook — split part 2 of 7 (source lines 315-1095). Concatenating all 7 files in order reproduces certa_fde_interview_mastery.md. -->

## Part 2: REST APIs, HTTP semantics, auth, pagination, validation

### 2.1 First principles: an API is a contract with a machine on the other side

An integration is not "calling an endpoint". It is **two teams' data models agreeing on a protocol**. Every integration bug you will ever debug falls into one of four buckets:

| Bucket | Question | Typical failure |
|---|---|---|
| **Semantics** | What does this call mean? | Using POST where the API treats repeated calls as new resources |
| **Identity** | How do I know this is the same thing? | Matching vendors by name; two "Acme GmbH" rows |
| **Time** | Whose clock, whose ordering, whose deadline? | Retrying a request whose outcome is unknown; LWW silently discarding a newer edit |
| **Failure** | What happens when half of it worked? | Partial batch success treated as total failure (or total success) |

Hold this in mind: **most "API bugs" are identity or failure bugs wearing an HTTP costume.**

### 2.2 HTTP methods, safety, and idempotency [ASSESSMENT: idempotency]

| Method | Safe (no side effects) | Idempotent (repeat = same end state) | Notes for integration work |
|---|---|---|---|
| GET | Yes | Yes | Never mutate on GET. Caches, proxies and retry logic all assume it is safe |
| HEAD | Yes | Yes | Liveness/ETag checks without a body |
| POST | No | **No, not by default** | Either a creation or an action. Retry safety must come from an idempotency key |
| PUT | No | Yes | Full replacement at a known URI. Repeating leaves the same representation |
| PATCH | No | **Depends on the patch semantics** | `{"op":"add","path":"/tags","value":"x"}` is not idempotent; `{"tier":"high"}` is |
| DELETE | No | Yes | "Already gone" should be indistinguishable from "just deleted" (both 204) |

Two distinctions that get asked about and matter in practice:

- **Safe ≠ idempotent.** Safe means *no state change at all* (GET). Idempotent means *the end state is the same if repeated* (PUT, DELETE).
- **Idempotent ≠ retry-safe by itself.** PUT is idempotent, but if two workers PUT different bodies concurrently, last-write-wins still loses an update. Idempotency protects the *retry*, concurrency control protects the *race*. You need both. (Part 6.8.)

**What makes a POST effectively idempotent** [BEST PRACTICE, and see Part 6.2]:

```
1. Client generates a stable key:            Idempotency-Key: 8f14e45f-... (UUIDv4 or ULID)
2. Server stores  key -> (request_fingerprint, status, response_json, created_at)
   in a table with a UNIQUE constraint on the key.
3. First request: process, store the response, return it.
4. Retry with the same key and same body: return the stored response (typically with
   the original status code).
5. Same key, different body: 409 Conflict (or 422). Never silently return the first
   response for a different request - that is silent data corruption.
6. Keys expire (24h-7d is typical). Expiry + a unique natural key is the belt-and-braces
   combination: the key handles the retry window, the natural key handles forever.
```

### 2.3 Status codes: what they mean to a *client*, not a textbook [ASSESSMENT: 201, 429, 401/403]

| Code | Meaning | What an integration client must do | Retry? |
|---|---|---|---|
| 200 OK | Success with body | Parse | n/a |
| **201 Created** | A new resource exists **[ASSESSMENT]** | Read `Location` for the URI; treat a repeat with the same idempotency key as the same creation | n/a |
| 202 Accepted | Work queued, not complete | Poll the status resource, or rely on a webhook/callback. Do **not** assume it finished | n/a |
| 204 No Content | Success, no body | Do not try to parse a body (common bug: `.json()` on 204 raises) | n/a |
| 400 Bad Request | Malformed syntax | Fix the request; likely your serializer | No |
| **401 Unauthorized** | Unauthenticated (missing/expired/invalid credentials) | Refresh the token once, then fail loudly. Infinite refresh loops are a classic outage | Only after refresh |
| **403 Forbidden** | Authenticated but not permitted | Fix scopes/roles/config. Retrying will never help. Escalate to the customer's admin | No |
| 404 Not Found | Resource absent **or** invisible to you | Distinguish "never existed" from "no permission to see it" in your logs | No |
| 405 Method Not Allowed | Wrong verb | Fix the call | No |
| **409 Conflict** | State conflict (duplicate, version mismatch) | Re-read, re-apply your policy, retry once. This is the optimistic-concurrency signal | Once, after re-read |
| 412 Precondition Failed | ETag/If-Match mismatch | Same as 409 | Once, after re-read |
| **422 Unprocessable Entity** | Syntactically fine, semantically rejected **[ASSESSMENT]** | Fix the payload or the business rule. Never retry unchanged | No |
| **429 Too Many Requests** | Rate limited **[ASSESSMENT]** | Honor `Retry-After`, reduce concurrency, back off with jitter | Yes |
| 500 Internal Server Error | Provider bug or unhandled state | Retry with backoff if the operation is safe; capture the correlation ID | Yes, bounded |
| 502/503/504 | Gateway/timeout/unavailable | Retry with backoff; 504 means the outcome may be **unknown** — the dangerous case | Yes, bounded |
| 207 Multi-Status | Per-item results in a batch | Parse per-item statuses; do not treat the batch as atomic | Per item |

**The one-line rule for retries:** *the status code tells you whether to retry at all; the operation's semantics and your idempotency design tell you whether retrying is safe.*

**Model answer — "What does 201 mean and when do you return it instead of 200?"**

> "`201 Created` means the request resulted in a new resource existing, and the response should include a `Location` header pointing at it. I return 201 from a create endpoint and 200 when the call succeeded but didn't create anything — for example an upsert that updated an existing vendor, or a POST that triggered an action. The distinction matters to clients because 201 tells them a new identity exists, so they should store the returned ID rather than re-deriving it. It also matters for idempotent creates: if the same `Idempotency-Key` is replayed, I return the *original* 201 response — the resource was created once, and the client should see the same answer, not a 409."

### 2.4 Authentication and authorization [ASSESSMENT: OAuth 2.0]

**First principles:** authentication answers *who is calling*; authorization answers *what may they do*. They are different failures (401 vs 403) with different owners (the identity team vs the customer's admin).

| Mechanism | Shape | Use when | Failure modes you must handle |
|---|---|---|---|
| API key | Static secret in a header | Server-to-server, single tenant, low blast radius | No expiry, no scopes, leaks in logs, per-key rate limits |
| HMAC signature | `sign(secret, timestamp + body)` | Webhooks and partner callbacks (proves *body integrity* + *authenticity*) | Replay if you ignore the timestamp; broken if you verify against a re-serialized body |
| OAuth 2.0 client credentials | POST to `/token` with client_id+secret → short-lived bearer token | Service-to-service where a token server exists | Token expiry mid-batch; secret rotation; caching a token past its `expires_in` |
| OAuth 2.0 authorization code (+PKCE) | Redirect flow granting a token *on behalf of a user* | The customer authorizes Certa to read from their system as themselves | Refresh-token rotation, consent revocation, per-tenant token storage |
| OAuth 2.0 on-behalf-of / JWT bearer | Token exchange with an identity provider | Enterprise SSO / impersonation of a human, preserving audit identity | Audience/issuer validation, clock skew, scope mismatch |
| mTLS | Mutual certificate auth | High-assurance B2B (finance, health) | Certificate expiry — put a calendar alert 30 days out |
| JWT bearer | Self-contained token | Stateless auth at scale | Never trust the payload without verifying the signature, `iss`, `aud`, `exp`, `nbf` |

**OAuth 2.0 in one paragraph (interview-ready):** *"[ASSESSMENT] OAuth 2.0 is an authorization framework, not an authentication protocol — it lets a service obtain limited access to a resource on behalf of a user or itself, without sharing the user's password. For server-to-server work I use the client-credentials grant: the client authenticates at the token endpoint with its ID and secret, gets a short-lived access token with defined scopes, and sends it as `Authorization: Bearer <token>`. For anything acting on behalf of a human I use authorization code with PKCE, because the token must carry the user's identity for the audit trail. Tokens are refreshed before expiry, and a refresh failure is a distinct alert, because it usually means a rotated secret or a revoked consent — both of which need a human, not a retry loop. And scope errors show up as 403, not 401, which is why I check scopes before blaming the token.*"

**Wait — is OAuth also authentication?** *"OpenID Connect is the identity layer on top of OAuth 2.0 that gives you authentication; OAuth alone tells you what a client may access. That distinction matters when I'm asked to prove who an approver was in an audit trail."*

**Token handling rules that interviewers reward:**

```
- Store tokens in a secret manager, never in code, images, or logs.
- Never log Authorization headers. Redact them at the logger, not at call sites.
- Cache with a safety margin: expires_at = now + expires_in - 60s.
- One token per tenant; a shared token destroys your audit trail and multiplies blast radius.
- On 401: refresh once, retry once, then fail with an actionable error. No loops.
- On 403: do not refresh. Refresh cannot fix a missing scope.
- Rotate secrets on a schedule and on offboarding; verify rotation in staging first.
```

### 2.5 Pagination: offset vs cursor [ASSESSMENT: cursor pagination]

**First principles:** pagination is a *consistency* problem, not a UI problem. The question is: "what does page 2 mean if the underlying data changed after page 1?"

| Approach | Request shape | Strengths | Failure modes |
|---|---|---|---|
| Offset/limit | `?limit=100&offset=200` | Trivial, allows jumping to page N, supports total counts | **Skips and duplicates** when rows are inserted/deleted between pages; `OFFSET 1000000` scans and discards; expensive on large tables |
| Cursor / keyset | `?limit=100&cursor=<opaque>` | Stable under concurrent writes; index-friendly (`WHERE (created_at,id) > (?,?)`) | No random access; cursor usually opaque; if the provider's sort key is not unique you can still skip rows; must handle the provider changing cursor semantics |
| Page token (server-side snapshot) | `?page_token=...` | Can be snapshot-stable if the server pins it | Token expiry mid-walk; unclear semantics unless documented |

**The canonical keyset query (MySQL 8):**

```sql
-- Page 1
SELECT EntryID, VendorID, Region
FROM maintable_NOG6Y
WHERE Year = 2018
ORDER BY EntryID
LIMIT 5;

-- Page 2: resume strictly after the last key you saw. Never OFFSET when the table writes.
SELECT EntryID, VendorID, Region
FROM maintable_NOG6Y
WHERE Year = 2018 AND EntryID > 5
ORDER BY EntryID
LIMIT 5;
```

**Why a composite cursor:** ordering by a non-unique column alone (`ORDER BY updated_at`) can skip or repeat rows when many rows share a timestamp. The stable form is a **total order**: `ORDER BY updated_at, entry_id` with the cursor carrying both values, i.e. `WHERE (updated_at, entry_id) > (?, ?)` — which MySQL 8 can satisfy with a composite index `(updated_at, entry_id)`.

**Model answer — "Why might cursor pagination be preferable?"**

> "Because offset pagination is defined relative to the *current* result set, not to a stable position. If a row is inserted while I'm walking page 2, every subsequent page shifts and I either see a record twice or never see it — and for a vendor sync, a missed record means a vendor that never gets assessed, which is a compliance gap rather than a cosmetic bug. Cursors resume from a stable key, so concurrent inserts don't move my position, and the database can use the index directly instead of counting and discarding `OFFSET` rows. The trade-offs: I lose random access to page 47, and I have to trust the provider's ordering key to be unique and stable — if it isn't, I add a tie-breaker like the ID and I bound the walk so a bad cursor can't loop forever. Also, a cursor doesn't give me a free total count; I ask for it separately if the UI needs it, and I treat it as an estimate."

### 2.6 Schema validation and evolution [ASSESSMENT: schema evolution]

**First principles:** a schema is a promise about *shape*. Evolution is about keeping that promise while the shape changes. Compatibility is directional:

| Change | Backward compatible (old readers can read new data) | Forward compatible (new readers can read old data) | Safe default? |
|---|---|---|---|
| Add an **optional** field | Yes | Yes | ✅ Preferred |
| Add a **required** field | No (old readers break) | Yes | ❌ Needs a major version or a default |
| Remove an **optional** field | Yes | No | ⚠️ Only if nothing reads it (check access logs) |
| Remove a **required** field | No | No | ❌ Breaking |
| Rename a field | No | No | ❌ Add new + deprecate old, dual-write, then remove |
| Widen a type (`int`→`string`, `int32`→`int64`) | Usually | Depends | ⚠️ Verify consumer parsers |
| Narrow a type | No | No | ❌ Breaking |
| Change **semantics** (`tier: "high"` now means something else) | **No, and it is invisible** | No | ❌ The most dangerous change of all |
| Add a value to an enum | Only if consumers tolerate unknown values | Yes | ⚠️ Requires a documented "unknown" path |

**The rules that make evolution survivable** [BEST PRACTICE]:

```
1. Consumers must ignore unknown fields (Postel's law, but bounded: ignore unknown
   FIELDS, never ignore unknown REQUIRED VALUES silently).
2. Every consumer must have an explicit "unknown enum value" branch. The day the
   provider adds tier="critical_plus", the sync must not crash or drop the record.
3. Additive changes are free; breaking changes get a new version prefix
   (/v2) or a new field name with a deprecation window.
4. Deprecation requires evidence: check telemetry for usage before removing.
5. Contract tests: a test suite that validates a recorded sample of the provider's
   real responses against your parser. Run it in CI. When the provider ships a
   surprise, the pipeline fails loudly in staging rather than silently in production.
6. Never let a parser invent defaults for domain-critical fields. A missing
   `tier` must become "needs review", not "medium".
```

**JSON Schema is the machine-checkable version of that promise.** In Python (see Part 4 for the full pattern):

```python
SCHEMA = {
    "type": "object",
    "additionalProperties": False,
    "required": ["vendor_id", "legal_name"],
    "properties": {
        "vendor_id": {"type": "string", "pattern": "^V[0-9]{3,}$"},
        "legal_name": {"type": "string", "minLength": 1},
        "tier": {"enum": ["low", "medium", "high", "critical"]},
        "annual_spend": {"type": "number", "minimum": 0},
        "data_processing": {"type": "boolean"},
        "updated_at": {"type": "string", "format": "date-time"},
    },
}
```

**A subtle production rule:** validate **before** you transform, and validate **again** after. The first catches a provider breaking the contract; the second catches *your mapper* breaking it (a `None` slipping into a required field, a string where you meant a number).

### 2.7 Data mapping: the unglamorous work that decides whether integrations land

**First principles:** two systems never agree on names, units, granularity, or time. Mapping is the process of making those disagreements explicit and testable.

**A mapping spec is a spreadsheet with a row per field** [BEST PRACTICE — and this is exactly the sort of artifact an FDE produces in the room]:

| # | Certa field | Certa type | Source field (ServiceNow/GRC) | Transformation | Null policy | Owner | Test case |
|---|---|---|---|---|---|---|---|
| 1 | `legal_name` | string(255) | `u_vendor_name` | trim, collapse whitespace | reject if empty | Certa | `" Acme  GmbH "` → `"Acme GmbH"` |
| 2 | `tier` | enum | `u_risk_rating` | uppercase → map {1:LOW,2:MEDIUM,3:HIGH,4:CRITICAL} | unmapped → **review queue**, never a default | Certa | `"5"` → review |
| 3 | `region` | enum(ISO) | `u_geo` | map "EMEA"→"Europe", "AMER"→"Americas", "APJ"→"APAC" | unknown → review | Certa | `"LATAM"` → review |
| 4 | `annual_spend` | decimal(18,2) | `u_contract_value` | strip currency symbols, parse to decimal, **no implicit FX** | null → null (do not coerce to 0) | GRC | `"€1.2M"` → reject (ambiguous unit) |
| 5 | `data_processing` | boolean | `u_dpa_signed` | `Y/Yes/true/1 → true` | unknown → review | Certa | `"Y"` → true |
| 6 | `owner` | email | `u_assignment_group` | resolve group → email via a lookup table | unresolvable → keep raw, log | GRC | `"GRC-OPS"` → `grc-ops@corp` |
| 7 | `updated_at` | timestamptz | `sys_updated_on` | parse provider format, assume source tz, convert to UTC | unparseable → reject row | GRC | `"2026-10-10 11:00:00"` (tz unknown) → assumed UTC + logged |

**The five mapping questions that cause 90% of the incidents:**

1. **Units and currencies.** `1000` — thousand or one thousand *cents*? `EUR` or `USD`? Never silently FX-convert: a wrong number in a risk assessment is worse than a missing one.
2. **Timezones.** "11:00" in whose timezone? Store UTC with the source timezone recorded, and treat naive timestamps as a data-quality flag.
3. **Null vs empty vs absent.** `null`, `""`, and a missing key are three different statements. Collapse them only deliberately and document it.
4. **Truncation.** `VARCHAR(120)` in the destination vs 300 characters in the source silently loses data (or errors in strict mode). Detect length overflow and route it, don't let the database decide.
5. **Enum drift.** The destination's enum is a versioned contract. An unmapped source value must be *visible* (review queue + metric), never coerced.

### 2.8 Common mistakes in this part, and why they fail

| Mistake | Why it fails |
|---|---|
| Retrying a 422 or 403 | Burns quota, hides the real defect, and turns one bad record into a hot loop |
| Refreshing the token on a 403 | Forbidden ≠ unauthenticated. The loop never terminates and masks a permissions bug |
| Retrying a POST with a *new* idempotency key `[ASSESSMENT: idempotency]` | Creates duplicates — the exact failure idempotency keys exist to prevent |
| Treating 202 as "done" | Work is still queued; the client proceeds on state that isn't there yet |
| `.json()` on a 204 or an empty body | Raises in your client and looks like a provider outage |
| Assuming the provider sorts stably | Page boundaries duplicate/skip rows; sync silently misses vendors |
| Adding a `required` field to a webhook/API contract | Breaks every consumer that already deployed. Add optional + default, then tighten later |
| Coercing `null → 0` or `unknown → medium` | Fabricates data in a compliance system. A missing risk tier must be visibly missing |
| Verifying a webhook signature over a re-serialized body | Whitespace/key-order changes break the HMAC. Verify over the **raw bytes** |
| Logging full request/response bodies | Vendor contracts, DPA terms and PII in a log aggregator is an incident, not a debugging convenience |
| Building a bespoke retry loop per endpoint | Inconsistent behavior, no shared budget, no shared metrics. Centralize the HTTP client |

### 2.9 Model answers (rehearse these)

**"A POST creates a vendor and the client times out. What happened?"**

> "A timeout tells me the outcome is unknown, not that it failed. Three possibilities: the request never arrived, it arrived and failed, or it succeeded and the response was lost. Since a create is not safe to repeat blindly, I retry with the *same* idempotency key, and the server returns the original result rather than creating a second vendor. If the API has no idempotency support, I don't retry — I reconcile: search for a vendor with the same stable external identifier, and only create if it's genuinely absent, ideally with a unique constraint on that identifier so concurrent attempts cannot both succeed. The database constraint is the actual guarantee; a pre-check is only a latency optimization."

**"How would you safely manage API credentials for a customer integration?"**

> "Per tenant, in a secret manager, never in code or images, injected at runtime. Least-privilege scopes, separate credentials per environment so a staging bug cannot touch production data, and rotation on a schedule with a tested procedure. Tokens are cached with a safety margin against expiry; Authorization headers are redacted at the logger so they cannot leak into logs or traces. Access is audited, and offboarding removes the integration's credentials on a documented date. For webhooks, I hold a signing secret per tenant and support two active secrets during rotation so neither side has to stop traffic to rotate."

**"What is schema evolution and how do you avoid breaking consumers?"**

> "It's changing a contract over time without breaking the people who already depend on it. I think in terms of compatibility direction: adding an optional field is safe both ways, adding a required field breaks existing readers, and renaming or narrowing a type breaks both. So my default is additive changes with a default value, and consumers must ignore unknown fields while still failing on unknown *values* for closed enums — with an explicit 'needs review' path. Breaking changes get a version. And the safety net isn't documentation, it's a contract test suite running the provider's recorded real responses through my parser in CI, plus telemetry on how many records hit the 'unknown' branches so I can see drift before a customer does."

### 2.10 Practice

**Practice 2.1** A vendor sync calls `POST /vendors` for 500 vendors. Vendor 217 returns 422, vendor 218 returns 429 with `Retry-After: 30`, and the whole batch request times out at vendor 300. Describe exactly what you do, per case, and what state you must reconcile at the end.

**Your answer:**
>
>
>

<details><summary>Expected points and edge cases</summary>

- 422 (217): do **not** retry. Quarantine the record with the provider's error body, continue the batch, and surface it in a review queue with a count metric. Never let one bad record block 499 good ones.
- 429 (218): honor `Retry-After: 30` for *that* request, and cut concurrency for the whole client, not just the one call. Add jitter so multiple workers don't synchronize. 429 also means your pacing is wrong — fix the token bucket, don't just retry harder.
- Timeout at 300: **unknown outcome for vendor 300, known for 1–299.** If the API supported per-item idempotency keys, retry from 300 with the same keys. If it didn't, the only safe path is reconciliation: re-read the destination and compare by stable external ID before re-sending. Batch position is not a transaction boundary.
- End state: the run is not "complete" just because the loop finished. Report attempted / applied / quarantined / unresolved, and run reconciliation so the count of vendors in both systems matches with zero unexplained gaps.
- Edge cases interviewers push on: does the batch response distinguish per-item results (207-style) or is it all-or-nothing? If all-or-nothing, a batch of 500 with one bad record **must** be split — the design has a scaling defect. What if the same vendor appears twice in the batch (duplicate in the source feed)? What if the customer edits the record in the destination while you retry?
</details>

**Practice 2.2** You are told "the integration is fine, it returns 200". What five things do you still want to verify before believing that?

**Your answer:**
>
>
>

**Practice 2.3** Write the mapping-spec row for "risk tier" where the source system uses a free-text field (`u_risk_rating` containing values like `"Low"`, `"low risk"`, `"2"`, `"Critical - supplier"`, and sometimes empty).

**Your answer:**
>
>
>

**Practice 2.4 (coding)** Write a Python function that walks a cursor-paginated endpoint and returns all items, with a loop guard and a maximum page count. Then harden it: timeouts, retry on 429/5xx honoring `Retry-After`, and a clear exception on permanent errors.

### 2.11 Part 2 scorecard

| Item | Correctness /4 | Production readiness /4 | Notes |
|---|---|---|---|
| Status codes: retry vs fix vs escalate | | | |
| 201 vs 200 vs 202 vs 204 | | | |
| Idempotency key lifecycle incl. key reuse | | | |
| OAuth client-credentials vs auth-code, and 401 vs 403 handling | | | |
| Cursor vs offset, with the unique tie-break | | | |
| Schema evolution rules by compatibility direction | | | |
| Mapping spec with units/timezones/nulls/enums | | | |

---

## Part 3: SQL fundamentals and practical exercises

> Everything in this part was **executed and verified on a real MySQL server** (9.7.1, with 8.0-compatible syntax only). The companion file `certa_lab/sql_exercises.sql` contains the full schema, seed data and all 21 exercises; `certa_lab/sql_out.txt` contains the verified output. Reproduce it locally with:
> ```bash
> mysql -u root --table < certa_lab/sql_exercises.sql
> ```
> **Note on SQL "complexity":** databases don't have a big-O in the algorithm sense. The right question is *access path*: is it a covering index range scan, a full scan, a filesort, or a temporary table? Interviewers want the cost model, not a formula.

### 3.1 First principles: SQL is declarative, so learn the evaluation order

You describe the result; the optimizer chooses the plan. But the **logical** evaluation order explains nearly every confusing result:

```
FROM / JOIN   ->  WHERE  ->  GROUP BY  ->  HAVING  ->  SELECT  ->  DISTINCT
              ->  ORDER BY  ->  LIMIT
```

Consequences you can quote:

1. `WHERE` cannot see an aggregate; `HAVING` can. That's why `WHERE COUNT(*) > 3` is an error.
2. `SELECT` aliases are visible to `ORDER BY` (evaluated later) but not to `WHERE` (evaluated earlier) — MySQL is lenient with `HAVING` aliases, but don't rely on it.
3. `LIMIT` is applied last, so `ORDER BY` without a unique tie-break gives you a *nondeterministic* page boundary.
4. `DISTINCT` after `SELECT` means `COUNT(DISTINCT x)` is doing dedupe *per group*, not `COUNT(DISTINCT)` over the whole table.

### 3.2 NULLs and three-valued logic

`NULL` means "unknown", not "empty". Every comparison with NULL yields `NULL` (unknown), and `WHERE` keeps only rows where the predicate is `TRUE`.

| Expression | Result | Trap |
|---|---|---|
| `NULL = NULL` | `NULL` | Use `IS NULL` / `IS NOT NULL` |
| `NULL <> 'x'` | `NULL` | The row is filtered out, surprising people filtering for "not x" |
| `COALESCE(x, '')` | fallback value | Correct way to make a NULL comparable |
| `COUNT(*)` | counts all rows | Counts rows, not non-NULL values |
| `COUNT(col)` | counts non-NULL values | Probably what you meant |
| `COUNT(DISTINCT col)` | distinct non-NULL values | **NULL is never counted** |
| `AVG(col)` | ignores NULLs | Denominator is the non-NULL count, not the row count |
| `SUM(col)` | ignores NULLs | Returns `NULL` (not 0) if all values are NULL |
| `'a' IN (1, NULL)` | `NULL` for a non-matching value | The `NOT IN` with NULL trap below |
| `GROUP BY col` | NULL becomes one group | Silently produces a `NULL` group in reports |

**The `NOT IN` trap (asked surprisingly often, and a real production bug):**

```sql
-- If ANY vendor_id in the subquery is NULL, this returns ZERO rows.
-- Because "x NOT IN (1, 2, NULL)" is "x<>1 AND x<>2 AND x<>NULL" -> unknown -> filtered.
SELECT * FROM certa_vendors WHERE vendor_id NOT IN (SELECT vendor_id FROM grc_vendors);

-- Correct and NULL-safe: NOT EXISTS (or a LEFT JOIN ... IS NULL anti-join)
SELECT c.* FROM certa_vendors AS c
WHERE NOT EXISTS (SELECT 1 FROM grc_vendors AS g WHERE g.vendor_id = c.vendor_id);
```

### 3.3 Aggregation: `COUNT(*)` vs `COUNT(col)` vs `COUNT(DISTINCT col)`

Verified against the seeded table (15 rows, one with a NULL region):

```sql
SELECT
    COUNT(*)                   AS rows_total,          -- 15
    COUNT(`Region`)            AS rows_with_region,    -- 14  (NULL excluded)
    COUNT(DISTINCT `Region`)   AS distinct_regions,    --  3  (NULL excluded)
    COUNT(`VendorID`)          AS non_null_vendor_ids, -- 15
    COUNT(DISTINCT `VendorID`) AS distinct_vendors     -- 10
FROM `maintable_NOG6Y`;
```

`COUNT(DISTINCT ...)` is the correct tool for "how many unique vendors", and it costs a sort/hash per group. On a very large table, the cheaper alternatives are a covering index whose second column is the distinct column, or a pre-aggregated summary table refreshed on a schedule — at the cost of staleness.

### 3.4 Joins, the dimension-join trap, and anti-joins

```sql
-- Regions with zero activity: use the dimension as the driving table
SELECT r.`RegionCode` AS Region,
       COUNT(DISTINCT v.`VendorID`) AS UniqueVendors,
       COUNT(v.`EntryID`)           AS TotalEntries   -- COUNT(v.col), never COUNT(*)
FROM `regions` AS r
LEFT JOIN `maintable_NOG6Y` AS v
       ON v.`Region` = r.`RegionCode` AND v.`Year` = 2018
GROUP BY r.`RegionCode`
ORDER BY UniqueVendors DESC, TotalEntries DESC, Region ASC;
```

**Verified output — the single most instructive bug in this part:**

| Region | `COUNT(*)` (wrong) | `COUNT(v.EntryID)` (right) |
|---|---|---|
| Americas | 4 | 4 |
| APAC | 2 | 2 |
| Europe | 5 | 5 |
| **MEA** | **1** | **0** |

MEA has no 2018 rows. The `LEFT JOIN` still emits one row — with NULLs on the right side — so `COUNT(*)` counts that phantom row as 1. `COUNT(v.EntryID)` counts non-NULL values and correctly returns 0.

**Anti-join (find what's missing on one side, NULL-safe and usually faster than `NOT IN`):**

```sql
SELECT g.`VendorID`
FROM `grc_vendors` AS g
LEFT JOIN `certa_vendors` AS c ON c.`VendorID` = g.`VendorID`
WHERE c.`VendorID` IS NULL;       -- only in the GRC
```

### 3.5 Window functions (MySQL 8+): the tool that replaces self-joins

```sql
-- Busiest region per year: ROW_NUMBER in a CTE beats a correlated subquery both
-- in readability and in plan quality.
WITH per_year_region AS (
    SELECT `Year`, `Region`, COUNT(*) AS entries
    FROM `maintable_NOG6Y`
    WHERE `Region` IS NOT NULL
    GROUP BY `Year`, `Region`
),
ranked AS (
    SELECT `Year`, `Region`, entries,
           ROW_NUMBER() OVER (PARTITION BY `Year` ORDER BY entries DESC, `Region`) AS rn
    FROM per_year_region
)
SELECT `Year`, `Region`, entries FROM ranked WHERE rn = 1 ORDER BY `Year` DESC;
-- Verified: 2018 | Europe | 5 ; 2017 | Europe | 3
```

| Function | Use | Integration example |
|---|---|---|
| `ROW_NUMBER()` | Exactly one row per group (deterministic with a full ORDER BY) | Latest event per vendor |
| `RANK()` / `DENSE_RANK()` | Keep ties | Tied top-3 risk exposures |
| `SUM() OVER (ORDER BY ...)` | Running totals | Cumulative vendors onboarded this quarter |
| `LAG()` / `LEAD()` | Compare with previous/next row | Detect version regressions in an event feed |
| `SUM(COUNT(*)) OVER ()` | Share of total without a second query | Region's % of entries |

**Percentage share, one query, no self-join (verified output: Europe 5 → 41.7%, Americas 4 → 33.3%, APAC 2 → 16.7%, NULL 1 → 8.3%):**

```sql
SELECT `Region`, COUNT(*) AS TotalEntries,
       ROUND(100 * COUNT(*) / SUM(COUNT(*)) OVER (), 1) AS pct_of_2018
FROM `maintable_NOG6Y`
WHERE `Year` = 2018
GROUP BY `Region`
ORDER BY TotalEntries DESC, `Region`;
```

`GROUP BY ... WITH ROLLUP` plus `GROUPING()` gives you the total row in the same result (verified: `ALL REGIONS | 9 | 12`).

### 3.6 Indexes, `EXPLAIN`, and the composite-order rule

**Cost model in words:** a B-tree index turns an equality/range lookup into `O(log n)` page reads plus the rows you actually need; a full table scan is `O(n)`. A covering index (all selected + filtered columns in the index) avoids touching the table at all. Every index makes writes slower and consumes storage — indexes are a read/write trade, not free.

**The rule that decides most index questions:** *leftmost prefix*. For `KEY (Year, Region)`, the index can serve `WHERE Year = 2018`, `WHERE Year = 2018 AND Region = 'Europe'`, and `GROUP BY` on that order — but **not** `WHERE Region = 'Europe'` alone.

**Verified plan with the index in place (`EXPLAIN FORMAT=TREE`):**

```
-> Group aggregate: count(distinct maintable_nog6y.VendorID)  (cost=3.88 rows=2)
    -> Filter: (maintable_nog6y.`Year` = 2018)  (cost=1.35 rows=11)
        -> Index scan on maintable_NOG6Y using idx_year_region  (cost=1.35 rows=11)
```

**Verified plan for a non-sargable predicate (`YEAR(CreatedAt) = 2018`):** MySQL can only *scan* the index and filter, because wrapping the indexed column in a function destroys the ordering. The sargable rewrite is:

```sql
-- Non-sargable: no index range on CreatedAt
WHERE YEAR(`CreatedAt`) = 2018
-- Sargable: a range the B-tree can seek
WHERE `CreatedAt` >= '2018-01-01 00:00:00' AND `CreatedAt` < '2019-01-01 00:00:00'
```

Honest caveat: on a 15-row demo table the optimizer may pick a covering index scan either way, so you cannot *see* the difference. The difference is real at scale — and being explicit about that ("the demo table is too small to show the plan change") is itself a senior signal.

**Composite order matters (verified):**

| Query | Index chosen | Plan |
|---|---|---|
| `WHERE Year = 2018` | `idx_year_region` | `ref`, `Using index` (covering) |
| `WHERE Region = 'Europe'` | `idx_region_year` | `ref`, `Using index` |
| `WHERE VendorName = 'Acme GmbH'` | none | `ALL` — full scan |

**Choosing the composite order** is a frequency question, not a style question: put the equality column that appears in *most* queries first, then the range/group column, then the covering columns. `(Year, Region)` is right for this assessment because the filter is always the year and the grouping is the region.

### 3.7 Transactions, isolation, and race conditions [ASSESSMENT: race conditions, SQL indexes]

**First principles:** a transaction groups statements so they commit or fail together. Isolation levels trade correctness for concurrency:

| Level | Dirty read | Non-repeatable read | Phantom read | Notes |
|---|---|---|---|---|
| READ UNCOMMITTED | Yes | Yes | Yes | Never use for money or compliance data |
| READ COMMITTED | No | Yes | Yes | PostgreSQL and Oracle default |
| **REPEATABLE READ** | No | No | **No in InnoDB** (gap locks) | **MySQL/InnoDB default** |
| SERIALIZABLE | No | No | No | Highest cost; can deadlock frequently |

**The three concurrency patterns you must be able to name:**

```sql
-- 1. LOST UPDATE -- two workers read-modify-write and one write disappears. BROKEN.
--    read: SELECT Value FROM counters WHERE Id='x'   -> 10   (both workers)
--    write: UPDATE counters SET Value = 11 ...       -> one update is lost

-- 2. ATOMIC UPDATE -- let the database do the arithmetic (verified: 10 -> 11)
UPDATE `counters` SET `Value` = `Value` + 1 WHERE `Id` = 'sync_attempts';

-- 3. OPTIMISTIC CONCURRENCY -- compare-and-set on a version (verified: A updates 1 row, B 0)
UPDATE `vendor_record`
SET `Name` = ?, `Version` = `Version` + 1
WHERE `VendorID` = ? AND `Version` = ?;   -- 0 rows updated => someone else won; re-read and retry

-- 4. PESSIMISTIC LOCK -- hold the row for the duration (blocks competitors)
START TRANSACTION;
SELECT `Version` FROM `vendor_record` WHERE `VendorID` = 'V001' FOR UPDATE;
UPDATE `vendor_record` SET `Version` = `Version` + 1 WHERE `VendorID` = 'V001';
COMMIT;
```

**Deadlocks:** two transactions each holding a lock the other needs. InnoDB detects and kills one with error 1213. Prevention: take locks in a deterministic order (always `ORDER BY vendor_id`), keep transactions short, and never hold a transaction open across an HTTP call.

**The guarantee hierarchy — this is the interview answer:**

> "A unique constraint is the only *real* duplicate guarantee. A `SELECT ... EXISTS` pre-check is a latency optimization that is racy by construction, because two workers can both see 'not exists'. So for idempotent intake I insert into a table whose primary key is the provider's event ID and let the duplicate-key error be the dedupe decision — verified: the second `INSERT IGNORE` of the same event ID reports `ROW_COUNT() = 0` and the table still holds one row. Optimistic concurrency handles the *update* race with a version column; the constraint handles the *create* race."

### 3.8 THE assessment exercise: Regional Vendors [ASSESSMENT topic 1]

**Prompt (as given):** count distinct vendor IDs and total entries per region for a specified year, with multi-column sorting.

**Step 1 — state the grain and the assumptions (10 seconds).** *"That's one row per region for 2018, with a distinct-vendor count and a row count, ordered by unique vendors descending and then total entries descending. I'm assuming the sort direction applies to both columns and that I should not include regions with no 2018 entries unless the query joins a region dimension — I'll show both."*

**Step 2 — the canonical answer:**

```sql
SELECT
    `Region`,
    COUNT(DISTINCT `VendorID`) AS UniqueVendors,
    COUNT(*)                   AS TotalEntries
FROM `maintable_NOG6Y`
WHERE `Year` = 2018
GROUP BY `Region`
ORDER BY UniqueVendors DESC, TotalEntries DESC;
```

**Verified output (exactly as executed against MySQL):**

| Region | UniqueVendors | TotalEntries |
|---|---|---|
| Europe | 3 | 5 |
| Americas | 3 | 4 |
| APAC | 2 | 2 |
| NULL | 1 | 1 |

**Read the output like an engineer, not a student.** Europe and Americas tie on 3 unique vendors; the second sort key breaks the tie (5 vs 4 entries). That tie is the whole point of the multi-column sort and the most likely follow-up question. The `NULL` group is a **data-quality signal** — a region-less row — and it disappears if you group on `COALESCE(Region, '(unassigned)')`, which is usually what a customer wants to *see* rather than be protected from.

**Step 3 — the follow-ups they will ask, with answers:**

**(a) "Why `COUNT(DISTINCT VendorID)` and not `COUNT(VendorID)`?"**
> "Because the question asks for unique vendors. `COUNT(VendorID)` counts entries where VendorID is not null, so a vendor with three entries counts three times. `COUNT(DISTINCT ...)` deduplicates inside each group. And it never counts NULLs, which matters if a data-quality issue leaves the ID empty — I would rather see the count drop than treat NULL as a vendor."

**(b) "Where would the index go?"**
> "A composite index on `(Year, Region)`: the year is the equality filter and comes first so the B-tree can seek, the region comes second so the grouping can be satisfied in index order, and both columns in the index make the aggregation cheap. If the table also had to return `VendorName`, the index stops being covering and MySQL has to visit the table. The cost is on writes — another B-tree to maintain per insert."

**(c) "What if there are ties on both sort keys?"**
> "Then the order is not fully determined, and the result can differ between runs — which is exactly the kind of thing that shows up as a flaky test. I'd add a final tie-break on the region itself so the ordering is total and reproducible."

**(d) "What if the customer wants regions with zero entries?"**
> "I need a region dimension to drive the join, otherwise those regions simply don't exist in the fact table and cannot be produced. Then I use a `LEFT JOIN` and — critically — `COUNT(v.EntryID)` instead of `COUNT(*)`, because `COUNT(*)` counts the phantom join row and reports 1 instead of 0. That bug is in section 3.4 with the verified output."

### 3.9 The rest of the SQL exercise set (verified)

**HAVING filters groups, WHERE filters rows** — verified: only Europe (5) and Americas (4) survive `HAVING COUNT(*) >= 3`.

```sql
SELECT `Region`, COUNT(*) AS TotalEntries
FROM `maintable_NOG6Y`
WHERE `Year` = 2018
GROUP BY `Region`
HAVING COUNT(*) >= 3
ORDER BY TotalEntries DESC;
```

**Running total across regions** (note MySQL sorts NULL first in ascending order):

```sql
SELECT `Region`,
       COUNT(DISTINCT `VendorID`) AS UniqueVendors,
       SUM(COUNT(DISTINCT `VendorID`)) OVER (ORDER BY `Region`) AS running_unique_vendors
FROM `maintable_NOG6Y`
WHERE `Year` = 2018
GROUP BY `Region`
ORDER BY `Region`;
```

| Region | UniqueVendors | running_unique_vendors |
|---|---|---|
| NULL | 1 | 1 |
| Americas | 3 | 4 |
| APAC | 2 | 6 |
| Europe | 3 | 9 |

**Idempotent upsert for a replay-safe sync** (verified: replaying the same version changes nothing; a stale version-4 replay after version 5 leaves `SourceVer = 5`):

```sql
INSERT INTO `vendor_sync` (`VendorID`, `Region`, `VendorName`, `SourceVer`, `PayloadHash`)
VALUES (?, ?, ?, ?, ?)
ON DUPLICATE KEY UPDATE
  `VendorName`  = IF(VALUES(`SourceVer`) > `SourceVer`, VALUES(`VendorName`),  `VendorName`),
  `PayloadHash` = IF(VALUES(`SourceVer`) > `SourceVer`, VALUES(`PayloadHash`), `PayloadHash`),
  `SourceVer`   = GREATEST(`SourceVer`, VALUES(`SourceVer`));
```

> MySQL 8.0.20+ note: `VALUES(col)` in the update clause is **deprecated** in favour of the row alias form `INSERT ... AS new ON DUPLICATE KEY UPDATE col = new.col`. Both work on MySQL 8; the alias form is the future-proof one. Mentioning that deprecation is a cheap senior signal.

**Dedupe — keep the newest row per natural key** (verified: version 6 wins over the out-of-order 5 and the older 4):

```sql
WITH ranked AS (
    SELECT `Id`, `VendorID`, `Region`, `VendorName`, `SourceVer`,
           ROW_NUMBER() OVER (PARTITION BY `VendorID`, `Region`
                              ORDER BY `SourceVer` DESC, `IngestedAt` DESC, `Id` DESC) AS rn
    FROM `vendor_raw_landing`
)
SELECT `VendorID`, `Region`, `VendorName`, `SourceVer` FROM ranked WHERE rn = 1;
```

Note the **full tie-break chain**: version, then ingest time, then the surrogate ID. Without `Id DESC` as the final term, two rows with identical version and timestamp would be ordered arbitrarily and the "dedupe" would be nondeterministic.

**Delete duplicates while keeping one winner** (MySQL-safe: you cannot `DELETE FROM t WHERE id NOT IN (SELECT ... FROM t)`, but a self-join delete works) — verified: 4 landing rows → 2:

```sql
DELETE l
FROM `vendor_raw_landing` AS l
JOIN `vendor_raw_landing` AS keep
  ON keep.`VendorID` = l.`VendorID`
 AND keep.`Region`   = l.`Region`
 AND (keep.`SourceVer` > l.`SourceVer`
      OR (keep.`SourceVer` = l.`SourceVer` AND keep.`Id` > l.`Id`));
```

**Reconciliation in two queries** — presence diff and field-level drift (verified output: V001 owner drift, V002 tier drift, V004 only in Certa, V009 only in GRC):

```sql
-- Presence diff, both directions in one pass
SELECT g.`VendorID`, 'only_in_grc' AS presence
FROM `grc_vendors` AS g
LEFT JOIN `certa_vendors` AS c USING (`VendorID`)
WHERE c.`VendorID` IS NULL
UNION ALL
SELECT c.`VendorID`, 'only_in_certa'
FROM `certa_vendors` AS c
LEFT JOIN `grc_vendors` AS g USING (`VendorID`)
WHERE g.`VendorID` IS NULL;

-- Field-level drift, with NULL-safe string comparison
SELECT c.`VendorID`,
       CASE WHEN c.`Name` <> g.`Name` THEN 'Name' END AS name_drift,
       CASE WHEN c.`Tier` <> g.`Tier` THEN 'Tier' END AS tier_drift,
       CASE WHEN COALESCE(c.`Owner`, '') <> COALESCE(g.`Owner`, '') THEN 'Owner' END AS owner_drift
FROM `certa_vendors` AS c
JOIN `grc_vendors`  AS g USING (`VendorID`)
WHERE c.`Name` <> g.`Name`
   OR c.`Tier` <> g.`Tier`
   OR COALESCE(c.`Owner`, '') <> COALESCE(g.`Owner`, '');
```

The `COALESCE(..., '')` on both sides is the NULL-safe comparison: `NULL <> 'x'` is `NULL`, which is **not true**, so a naive comparison silently misses drift. (MySQL also has the `<=>` NULL-safe equality operator; `NOT (a <=> b)` is the idiomatic form for "differs".)

**Keyset pagination** (verified: page 1 = rows 1–5, page 2 continues strictly after 5):

```sql
SELECT `EntryID`, `VendorID`, `Region` FROM `maintable_NOG6Y`
WHERE `Year` = 2018 AND `EntryID` > :last_seen_id
ORDER BY `EntryID` LIMIT 5;
```

**JSON extraction into generated, indexable columns** (verified: the two `vendor.updated` events for V001 return versions 5 and 3, and `->>` unquotes while `JSON_EXTRACT` stays JSON):

```sql
CREATE TABLE `webhook_events` (
  `EventID`   VARCHAR(64) PRIMARY KEY,
  `Payload`   JSON NOT NULL,
  `EventType` VARCHAR(40) GENERATED ALWAYS AS (JSON_UNQUOTE(JSON_EXTRACT(`Payload`, '$.type'))) STORED,
  `VendorID`  VARCHAR(40) GENERATED ALWAYS AS (JSON_UNQUOTE(JSON_EXTRACT(`Payload`, '$.data.vendor_id'))) STORED,
  KEY `idx_event_type` (`EventType`),
  KEY `idx_vendor` (`VendorID`)
);
```

**Defensive date handling** (verified: `STR_TO_DATE('2027-02-30', '%Y-%m-%d')` → `NULL`, because an impossible date must not silently become March 2nd):

```sql
SELECT STR_TO_DATE('28 February 2027', '%d %M %Y') AS parsed,   -- 2027-02-28
       STR_TO_DATE('2027-02-30', '%Y-%m-%d')        AS impossible_null;  -- NULL
```

**`ONLY_FULL_GROUP_BY`** (MySQL 8 default, verified in `@@sql_mode`): `SELECT Region, VendorID, COUNT(*) ... GROUP BY Region` raises error 1055, because `VendorID` is neither grouped nor aggregated. MySQL 5.7 allowed it and picked an arbitrary row; that behavior is gone. The portable fix is an explicit aggregate (`MIN(VendorID)`, `MAX(VendorID)`) or adding the column to `GROUP BY`.

### 3.10 Common SQL mistakes, and why they fail

| Mistake | Why it fails |
|---|---|
| `COUNT(*)` after a LEFT JOIN | Counts phantom NULL rows → reports 1 instead of 0 |
| `NOT IN (subquery)` with a nullable column | Returns zero rows whenever the subquery yields a NULL |
| `ORDER BY` without a unique tie-break + `LIMIT` | Page boundaries shift between runs; tests flake; syncs skip rows |
| `OFFSET` pagination on a writing table | Skips or duplicates rows; large offsets scan and discard |
| Indexing every column "just in case" | Write amplification and storage; the optimizer also gets worse at choosing |
| `index_col = 1 OR other_col = 2` | The OR often defeats index usage entirely — rewrite as `UNION ALL` and verify with `EXPLAIN` |
| Wrapping the indexed column in a function | Non-sargable predicate; no index range |
| `SELECT *` in a sync job | Breaks when a column is added/reordered; pulls bytes you never use |
| Relying on `VARCHAR` case-insensitivity for identity | `utf8mb4_0900_ai_ci` makes `'Acme' = 'acme'`; that is a *collation* decision, not a business rule. Vendor identity must be an explicit, normalized key |
| Storing money in `FLOAT` | Binary floating point cannot represent 0.10; use `DECIMAL(18,2)` |
| Long-running transactions holding locks during HTTP calls | Deadlocks and lock-wait timeouts; blocks the pipeline |
| Assuming `ROW_COUNT()` semantics through a pool | It is per-connection and reset by the next statement (verified: a `SELECT` reports -1) |
| Trusting `ROW_COUNT()` after `INSERT ... ON DUPLICATE KEY UPDATE` | Returns 1 for insert, 2 for update, 0 for no-change — do not branch on it without documenting |

### 3.11 Model answer — "How would you debug a slow query?" (interview-ready)

> "First I get the actual query and the actual plan: `EXPLAIN` and, on MySQL 8.0.18+, `EXPLAIN ANALYZE` for real timings rather than estimates. I'm looking for a full table scan where I expected a range, a filesort on a large result, or a temporary table from grouping. Then I check the obvious things: does the predicate match an index's leftmost prefix, is the predicate sargable, is the index covering, and are the statistics stale. I also want the shape of the data — a `WHERE status = 'Open'` index is useless if 90% of rows are `Open`, because the optimizer will correctly choose a scan and the fix is a different index or a different query, not a new index. If it is genuinely a big aggregation, I look at whether the result can be pre-aggregated or materialized. And I confirm with the customer's actual latency budget, because 'slow' needs a number."

### 3.12 Practice

**Practice 3.1 [ASSESSMENT]** For `maintable_NOG6Y`, return, for 2018: region, distinct vendors, total entries, and the region's share of all 2018 entries, ordered by distinct vendors descending then total entries descending, with a deterministic tie-break. Write it from memory.

**Your answer:**
> ```sql
>
> ```

<details><summary>Expected answer</summary>

```sql
SELECT `Region`,
       COUNT(DISTINCT `VendorID`) AS UniqueVendors,
       COUNT(*)                   AS TotalEntries,
       ROUND(100 * COUNT(*) / SUM(COUNT(*)) OVER (), 1) AS pct_of_year
FROM `maintable_NOG6Y`
WHERE `Year` = 2018
GROUP BY `Region`
ORDER BY UniqueVendors DESC, TotalEntries DESC, `Region` ASC;
```
Edge cases: the window `SUM(COUNT(*)) OVER ()` is evaluated **after** grouping, over all groups — that's what makes it a correct share of the total. Adding `, Region ASC` makes the ordering total. If the customer wants NULL regions labelled, `COALESCE(Region,'(unassigned)')` in both `SELECT` and `GROUP BY`.
</details>

**Practice 3.2** Two workers both run `UPDATE counters SET value = (SELECT value FROM counters WHERE id=1) + 1 WHERE id=1`. Is that safe? Explain precisely.

**Your answer:**
>
>
>

**Practice 3.3** A vendor sync runs every 15 minutes and pulls "changed since last run" using `WHERE updated_at > :last_run`. Name three ways this silently misses records.

**Your answer:**
>
>
>

<details>
<summary>Expected points</summary>

1. **Equal timestamps / clock granularity** — a row updated in the same second as the watermark (or with a coarser timestamp precision) is skipped forever. Fix: overlap the window and dedupe, e.g. `> last_run - INTERVAL 5 MINUTE`, or use a monotonic change sequence.
2. **Late-arriving writes / clock skew** — the destination's `updated_at` uses its own clock; a row written with an earlier timestamp after your watermark never appears in the delta. Fix: high-water mark on the *source's* change sequence, plus a periodic full reconciliation.
3. **A crash between "read page" and "commit watermark"** — if you advance the watermark before the data is durably applied, a crash loses rows. Fix: persist the data, then advance the watermark, in one transaction (or store the watermark with the batch).
Bonus: deletes and re-creates with the same ID look like an update; hard deletes are invisible in a delta and need tombstones or a full reconcile.
</details>

**Practice 3.4** Write the query that finds vendors present in the GRC but not in Certa, and the query that finds rows where the two systems disagree on `tier`, using NULL-safe comparison.

**Practice 3.5** A customer says "the report is wrong: 42 vendors in Europe, but the dashboard says 40." List, in order, what you check.

**Your answer:**
>
>
>

<details><summary>Expected points</summary>

Grain first: is the report counting distinct vendors or entries? Then filters: does the dashboard exclude a status (e.g. `offboarded`) or a date range the report doesn't? Then NULLs (a NULL region excluded from one path and counted in another). Then collation — `ai_ci` makes `Acme` and `ACME` equal in one query and different in another if one path normalized and the other didn't. Then refresh staleness: the dashboard may be a materialized view lagging by an hour. Then duplicates in the source. Only then do you look for a code bug — and you reproduce the number with the customer's exact filters before proposing a fix.
</details>

### 3.13 Part 3 scorecard

| Item | Correctness /4 | Speed (target) | Notes |
|---|---|---|---|
| Regional Vendors query from memory | | < 90s | |
| `COUNT(*)` vs `COUNT(col)` vs `COUNT(DISTINCT)` | | < 30s | |
| Explain the LEFT JOIN count trap | | < 30s | |
| Keyset pagination with a composite tie-break | | < 60s | |
| Optimistic vs pessimistic concurrency, and the unique-constraint guarantee | | < 60s | |
| Index choice + leftmost prefix explanation | | < 60s | |
| Upsert semantics incl. stale-version protection | | < 90s | |
| `NOT IN` NULL trap | | < 20s | |

---

