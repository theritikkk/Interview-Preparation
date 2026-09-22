# REST

Underlies every API surface across your three projects — the API Gateway fronting Hermes's Lambdas, the Task Scheduler's API gateway, and (implicitly) the internship's Supabase-backed frontend calls.

---

## Cluster 1: Constraints & HTTP Method Semantics

### What is it?
REST (REpresentational State Transfer) is an architectural style with specific constraints: statelessness (no client session state stored server-side between requests — each request carries everything needed), a uniform interface (resources identified by URIs, manipulated via standard methods), and cacheability (responses explicitly marked cacheable or not).

**Idempotency** — the property that matters most in interviews: repeating the same request produces the same end state.

| Method | Idempotent? | Typical use |
|---|---|---|
| GET | Yes | Read a resource |
| PUT | Yes | Replace a resource entirely |
| DELETE | Yes | Remove a resource (repeating a delete of an already-deleted resource is still "deleted") |
| PATCH | Not guaranteed | Partial update — depends on the operation (`set field to X` is idempotent; `increment field by 1` is not) |
| POST | No | Create a new resource — repeating it typically creates a duplicate |

### Interview Questions
- **L1:** "REST is stateless, resource-oriented, and uses standard HTTP methods with well-defined semantics; idempotency means repeating a request doesn't change the outcome beyond the first call."
- **L2:** explain *why* idempotency matters operationally: a client that times out waiting for a response doesn't know if the request succeeded — for an idempotent method, it can safely retry; for POST, a blind retry risks duplicate creation, which is exactly the problem an idempotency key (see below) solves.
- **L3 — 🔴 directly relevant to your Task Scheduler:** explain how you'd make a non-idempotent operation (like "create a task") safe to retry: an **idempotency key** — the client generates a unique key per logical operation, the server stores which keys it's already processed, and a retry with the same key returns the original result instead of creating a duplicate. This is the general-purpose version of the exact problem your Task Scheduler's exactly-once-effect claim solves at the message-processing layer (see `01_PROGRAMMING/SQL.md`).

### DO NOT SAY THIS
"PATCH is always idempotent, like PUT." — Only *some* PATCH semantics are idempotent (setting a field to an exact value); others (incrementing a counter, appending to a list) are not — don't conflate them.

---

## Cluster 2: Status Codes, Pagination, Versioning

### What is it?
Status codes group by first digit: 2xx success, 3xx redirection, 4xx client error, 5xx server error — the specific code communicates *why*, which matters for both API consumers and observability dashboards.

**Pagination** — two common strategies:
- **Offset-based** (`?page=3&limit=20`): simple, but can skip/duplicate items if rows are inserted/deleted between page requests, and gets slower on large offsets (the DB still has to scan past all skipped rows).
- **Cursor-based** (`?after=<opaque_cursor>`): stable under concurrent writes, consistently fast regardless of position, but can't jump to an arbitrary page number.

**Versioning** — URI-based (`/v2/tasks`), header-based (`Accept: application/vnd.api.v2+json`), or query-param-based — URI versioning is the most common because it's the most visible/debuggable, at the cost of being the least "purist" from a REST-resource-identity perspective.

### Interview Questions
- **L1:** know the major status code families and 2-3 specific codes cold: 200/201/204, 400/401/403/404/409, 500/503.
- **L2:** explain 409 Conflict precisely (the request is valid but conflicts with current server state — e.g., trying to create a resource that already exists with a unique constraint) versus 400 Bad Request (the request itself is malformed).
- **L3:** explain why cursor-based pagination is generally preferred for a high-write-throughput, frequently-changing list (directly relevant to a task list in the Task Scheduler, which is being written to concurrently during the exact load tests your resume describes) — offset pagination against a table with 10,000 tasks arriving concurrently would visibly skip/duplicate rows as new tasks insert ahead of a page boundary mid-scroll.

### Common Mistakes
- Returning 200 for an error response body (breaks client error handling that relies on status codes, not response body inspection)
- Using 404 for "the resource exists but you're not authorized to see it" (should be 403, or sometimes 404 deliberately to avoid leaking existence — a legitimate but *deliberate* security tradeoff, not a default mistake)

### Quick Revision
- Idempotent: GET, PUT, DELETE. Not idempotent: POST (PATCH depends on the operation).
- Idempotency keys make non-idempotent operations retry-safe — the general pattern behind your Task Scheduler's delivery-correctness claims.
- Cursor pagination > offset pagination under concurrent writes.
- Status code first digit = category; the specific code = the "why."
