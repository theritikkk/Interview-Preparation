# Certa Forward Deployed Engineer (FDE) --- Interview Practice Workbook

**Purpose:** Use this workbook to prepare for a Certa FDE interview,
especially practical integration, backend reliability, APIs, SQL,
debugging, and customer-facing problem solving.

> **How to use this workbook:** Attempt each question aloud or in
> writing before reading the model-answer checklist. The answers are
> practice guidance, not claims about Certa's exact internal
> architecture. Adapt them to the actual job description and interview
> format.

------------------------------------------------------------------------

## 1. What an FDE is expected to demonstrate

A Forward Deployed Engineer works close to customers to understand a
business problem, integrate systems, ship a practical solution, and keep
it reliable in production.

Demonstrate that you can:

-   Translate a vague customer request into clear requirements and
    acceptance criteria.
-   Integrate external APIs and map mismatched data models.
-   Build reliable asynchronous workflows using queues, retries,
    idempotency, and durable storage.
-   Debug production issues methodically using logs, metrics, traces,
    and reproducible tests.
-   Write correct SQL and practical Python or backend code.
-   Handle security, privacy, rate limits, and partial failures.
-   Communicate trade-offs, risks, timelines, and scope to technical and
    nontechnical stakeholders.

### A useful structure for open-ended answers

1.  **Clarify:** State assumptions and ask the few questions that could
    change the design.
2.  **Design:** Describe the simplest architecture that meets the
    requirements.
3.  **Reliability:** Explain duplicates, retries, ordering, timeouts,
    and partial failures.
4.  **Security:** Cover authentication, authorization, validation,
    secrets, and sensitive data.
5.  **Operate:** Explain logs, metrics, alerts, reconciliation, and
    recovery.
6.  **Deliver:** Define the first slice, acceptance criteria, risks, and
    rollout plan.

------------------------------------------------------------------------

## 2. REST APIs and integration fundamentals

### Q1. A POST request creates a vendor, but the client times out before receiving the response. What happened, and how should you retry?

**Practice answer checklist** - A timeout does not prove the server
failed. The server may have committed the creation and lost or delayed
the response. - Retry with the same stable idempotency key if the API
supports it. - The server should persist the key, request fingerprint,
operation status, and original response/result. - Reusing the same key
with a materially different request should be rejected. - If idempotency
is unsupported, query by a stable external identifier or use a
reconciliation workflow before creating again. - Avoid relying only on a
pre-check such as "does it exist?" because concurrent requests can race.

### Q2. Explain GET, POST, PUT, and PATCH.

-   **GET:** Read a resource; should not change its state.
-   **POST:** Create a resource or trigger an operation; not inherently
    idempotent.
-   **PUT:** Replace a resource representation at a known URI;
    conventionally idempotent.
-   **PATCH:** Apply a partial update; idempotency depends on the patch
    operation and API design.
-   Updating one field commonly uses PATCH, but always follow the API
    contract.

### Q3. What is the difference between HTTP 401 and 403?

-   **401 Unauthorized:** Authentication is missing or invalid; despite
    the name, it means the caller is not successfully authenticated.
-   **403 Forbidden:** The caller is authenticated, but lacks permission
    for the requested action or resource.
-   Investigate token expiry, issuer/audience, scopes, roles, tenant
    boundaries, and resource-level authorization.

### Q4. An API returns 422 for a vendor payload. How would you debug it?

1.  Inspect the response body and validation details.
2.  Compare the actual JSON with the API schema: field names, types,
    required fields, enum values, nesting, and nullability.
3.  Check field mapping and serialization.
4.  Reproduce the request with safe test data.
5.  Add schema validation and contract tests before sending requests.
6.  Do not blindly retry an unchanged 422; fix the request first.

### Q5. Classify 401, 403, 422, 429, and 503.

  -----------------------------------------------------------------------
  Status                              Usual response
  ----------------------------------- -----------------------------------
  401                                 Refresh or correct credentials,
                                      then retry only when appropriate

  403                                 Check permissions/configuration; do
                                      not blindly retry

  422                                 Fix validation or business-rule
                                      errors before retrying

  429                                 Respect `Retry-After`; use bounded
                                      backoff and jitter

  503                                 Usually retry with bounded
                                      exponential backoff and jitter
  -----------------------------------------------------------------------

**Remember:** Retry policy should depend on both the response and
whether the operation is safe to repeat.

### Q6. What is pagination, and why might cursor-based pagination be preferable?

Offset pagination can skip or duplicate records when rows are inserted
or removed between requests, and large offsets may be expensive.
Cursor-based pagination continues from a stable position and is often
more reliable for large, changing datasets. It is not automatically
perfect: use stable ordering and understand the provider's cursor
semantics.

### Q7. How would you safely manage API credentials?

-   Use a secret manager or protected environment configuration.
-   Never commit secrets or print tokens in logs.
-   Use least-privilege scopes and rotate credentials.
-   Use HTTPS and validate TLS certificates.
-   Separate development, staging, and production credentials.
-   Audit access and define a rotation/revocation process.

------------------------------------------------------------------------

## 3. Webhooks and event-driven reliability

### Q8. The same webhook arrives three times. How do you prevent duplicate processing?

-   Identify the provider's stable event ID.
-   Store it in a durable inbox/event table with a unique constraint,
    usually scoped to the provider or tenant.
-   Persist the event and processing state transactionally.
-   A duplicate should not schedule the business operation again;
    acknowledge it if it was already durably accepted.
-   Make downstream effects idempotent too. Deduplicating the event
    alone does not guarantee an email or external API call happens
    exactly once.

### Q9. Design a webhook endpoint.

A strong answer covers this flow:

1.  Receive the HTTPS request.
2.  Verify the signature over the raw body; validate timestamp/replay
    protections as supported by the provider.
3.  Validate the event envelope and required fields.
4.  Persist the event durably with a unique event key.
5.  Enqueue processing transactionally or use a transactional outbox.
6.  Return a 2xx acknowledgement after durable acceptance, not after all
    business processing finishes.
7.  Process asynchronously with bounded concurrency.
8.  Retry transient failures; route exhausted or permanent failures to a
    dead-letter queue.
9.  Track status, lag, retries, and outcomes; support audited replay.

**Important nuance:** If database persistence and queue publication are
separate operations, a crash between them can lose work. A transactional
outbox or an atomic database-backed job record closes this gap.

### Q10. A worker crashes after recording an event but before sending an approval email. How do you avoid losing the email and avoid sending it twice?

-   In one database transaction, update business state and insert an
    email-delivery intent into an outbox table.
-   A worker reads pending outbox rows and sends the email.
-   Record delivery status and retry transient failures.
-   Use a stable message key where the email provider supports
    idempotency.
-   Acknowledge work only after the durable state transition is
    complete.
-   Recognize that a crash after the email provider accepts the email
    but before the local success record is written can still cause
    duplicates unless the provider supports idempotency or the workflow
    can reconcile delivery.

### Q11. A provider returns 429 with `Retry-After: 10`. What should the client do? What if the next response is 422?

Wait at least the instructed ten seconds, then retry if the operation is
safe and within the retry budget. Add jitter when appropriate to avoid
synchronized retries. If the next response is 422, stop retrying the
unchanged request and fix the payload or business-rule issue.

### Q12. Events for one vendor arrive in versions 5, 7, 6. How do you prevent stale overwrites?

-   Store the latest applied version.
-   Apply an event only if its version is newer than the stored version.
-   Use a conditional update or transaction so concurrent workers cannot
    race.
-   For version 6 after version 7, ignore it or use it only for
    audit/history.
-   If events are deltas rather than full-state snapshots, detect gaps
    and fetch the current state or replay the missing sequence.

### Q13. What does at-least-once delivery mean? Is exactly-once delivery guaranteed?

At-least-once means an event may be delivered more than once, so
consumers must tolerate duplicates. End-to-end exactly-once effects
across independent databases, queues, and external APIs are generally
not guaranteed by default. In practice, use durable event records,
idempotent consumers, transactions where possible, and reconciliation to
achieve effectively-once business outcomes.

### Q14. What is a dead-letter queue, and how should replay work?

A dead-letter queue stores messages that have exhausted retries or
cannot be processed automatically. Preserve the event ID, payload or
secure payload reference, failure reason, attempt count, and timestamps.
Alert on growing DLQ size, expose a controlled operator workflow, fix
the underlying cause, and replay with idempotency intact. Do not
endlessly replay poison messages without diagnosis.

### Q15. How do you apply backpressure when a downstream API is slow?

-   Bound worker concurrency and queue growth.
-   Respect provider rate limits.
-   Use bounded retries with exponential backoff and jitter.
-   Separate queues or concurrency pools for different tenants or
    priorities where useful.
-   Monitor queue depth and oldest-message age.
-   Return webhook acknowledgements quickly after durable acceptance.
-   If storage is unavailable, do not acknowledge an event that has not
    been safely persisted.

------------------------------------------------------------------------

## 4. SQL and data correctness

### Q16. Count vendors per status, returning only statuses with at least ten vendors.

``` sql
SELECT status, COUNT(*) AS vendor_count
FROM vendors
GROUP BY status
HAVING COUNT(*) >= 10;
```

**Why:** `GROUP BY` creates one group per status; `COUNT(*)` counts
rows; `HAVING` filters groups after aggregation. `WHERE` filters
individual rows before grouping.

### Q17. Count unique vendor IDs and total entries per region for 2018, ordered by unique vendors and then total entries, both descending.

Assuming the table contains `Year`, `Region`, and `VendorID`:

``` sql
SELECT
    Region,
    COUNT(DISTINCT VendorID) AS UniqueVendors,
    COUNT(*) AS TotalEntries
FROM maintable_NOG6Y
WHERE Year = 2018
GROUP BY Region
ORDER BY
    UniqueVendors DESC,
    TotalEntries DESC;
```

If the actual column is named `Year`, quote it if required by the
database dialect. In MySQL, backticks can be used: `` `Year` ``.

**Distinction:** `COUNT(DISTINCT VendorID)` counts unique vendors;
`COUNT(*)` counts every qualifying row, including repeated vendor IDs.

### Q18. A query filtering `WHERE status = 'Open'` is slow on a very large table. What do you investigate first?

-   Inspect the query plan using `EXPLAIN` or the database's equivalent.
-   Check whether an appropriate index exists and whether the optimizer
    uses it.
-   Consider selectivity, table statistics, composite-index ordering,
    and whether the query is actually I/O-bound.
-   Avoid adding indexes blindly: indexes consume storage and slow
    writes.
-   Selecting fewer columns can reduce data transfer, but does not
    replace investigating the filter plan.

### Q19. Two workers read, modify, and write the same counter. How do you avoid lost updates?

Use an atomic database operation, transaction with suitable
locking/isolation, optimistic concurrency control, or a row lock. For a
simple increment, prefer an atomic statement such as:

``` sql
UPDATE counters
SET value = value + 1
WHERE id = ?;
```

Do not rely on logging, slower execution, or a single blind retry to
prevent race conditions.

### Q20. What is an upsert?

An upsert inserts a row if it does not exist and updates it if it does,
using a defined conflict key. It helps make synchronization repeatable,
but correctness still depends on a stable unique key and an explicit
policy for which source is allowed to update each field.

------------------------------------------------------------------------

## 5. Python and coding practice

### Q21. Find whether two numbers sum to a target.

Expected average time: **O(n)**; extra space: **O(n)**.

``` python
def has_pair_with_sum(nums, target):
    seen = set()

    for value in nums:
        needed = target - value
        if needed in seen:
            return True
        seen.add(value)

    return False
```

The set stores values already seen. Checking the complement before
inserting the current value prevents one array element from being used
twice to match itself.

### Q22. Return the top three most expensive products for each city from an API response.

``` python
def top_3_products_per_city(data):
    result = {}

    for city in data:
        city_name = city["city"]
        products = city.get("products", [])

        ranked = sorted(
            products,
            key=lambda product: float(product["price"]),
            reverse=True,
        )[:3]

        result[city_name] = [
            (product["name"], f'{float(product["price"]):.2f}')
            for product in ranked
        ]

    return result
```

**Important details** - Sort by numeric price, not a string
representation. - Sort descending for most expensive first. - Slice with
`[:3]`. - Handle missing fields, invalid prices, and HTTP errors
according to the API contract. - Confirm the required output shape:
tuples, dictionaries, JSON, or formatted strings may differ between
tests.

### Q23. How do you call an external API robustly in Python?

``` python
import requests

def fetch_json(url):
    response = requests.get(url, timeout=(3, 15))
    response.raise_for_status()
    return response.json()
```

This adds connection/read timeouts and raises on non-success HTTP
statuses. Production code should also validate the response schema,
handle known retryable errors with a bounded retry policy, avoid
retrying permanent 4xx errors blindly, and protect sensitive data in
logs.

### Q24. What should you test for an integration?

-   Happy path and expected output.
-   Missing and malformed fields.
-   Unexpected status codes and malformed JSON.
-   Timeouts and connection failures.
-   Rate limiting and `Retry-After`.
-   Duplicate events and concurrent deliveries.
-   Out-of-order versions.
-   Partial batch failures and pagination.
-   Authentication expiry and permission failures.
-   Retry exhaustion, DLQ behavior, replay, and reconciliation.

Use unit tests for transformation logic, contract tests for schemas, and
integration tests against a sandbox or controlled mock server.

------------------------------------------------------------------------

## 6. LLM and document-extraction reliability

### Q25. An LLM extracts a vendor risk level from a PDF but sometimes returns invalid JSON or invents information. How do you reduce risk?

1.  Require a strict schema and validate output with a JSON parser plus
    schema validation.
2.  Use constrained structured output or tool/function calling when
    supported.
3.  Ask the model to provide evidence snippets or page references for
    extracted fields.
4.  Distinguish "not found" from a guessed value; do not force a value
    when evidence is absent.
5.  Normalize risk levels against an allowed enum.
6.  Apply deterministic business rules and cross-field checks.
7.  Route low-confidence, conflicting, or high-impact results to human
    review.
8.  Evaluate against a representative labeled dataset and track
    extraction accuracy and unsupported-claim rates.

**Principle:** Valid JSON proves structural correctness, not factual
correctness.

------------------------------------------------------------------------

## 7. Observability and production debugging

### Q26. A customer says vendor synchronization is stuck. How do you investigate?

1.  Identify the affected tenant, vendor IDs, time window, and expected
    outcome.
2.  Trace a request/event using a correlation ID across API, database,
    queue, and worker logs.
3.  Inspect queue depth, oldest-message age, worker health, retry
    counts, DLQ, and external API status codes.
4.  Determine whether the event was never received, not persisted, not
    queued, repeatedly failing, or successfully processed but not
    reflected downstream.
5.  Reproduce the failure with sanitized data.
6.  Fix the root cause, replay safely, and verify the final state in
    both systems.
7.  Communicate impact, mitigation, next update, and prevention actions.

### Q27. Which metrics would you monitor?

-   Webhook receipt and validation failures.
-   Successful and failed sync operations.
-   End-to-end synchronization latency and lag.
-   Queue depth and oldest-message age.
-   Retry rate and DLQ size.
-   External API latency, timeouts, 429s, and 5xx responses.
-   Duplicate-event rate and version conflicts.
-   Reconciliation mismatch count.
-   Per-tenant error rates and rate-limit consumption.

Use structured logs, correlation IDs, dashboards, and alerts tied to
actionable thresholds. Avoid logging secrets or unnecessary sensitive
vendor data.

------------------------------------------------------------------------

## 8. Customer discovery and delivery

### Q28. A stakeholder says, "We need vendor onboarding to sync with our procurement tool by the end of the quarter." What do you ask?

-   Which procurement system and API are involved?
-   Which system owns each field?
-   What exactly counts as onboarding completion?
-   Is the integration one-way or bidirectional?
-   What volume, latency, and availability are expected?
-   Are sandbox credentials and representative test data available?
-   What security, compliance, audit, and data-retention requirements
    apply?
-   Who owns business-rule decisions and acceptance testing?
-   What must be included in the first release, and what can wait?

### Q29. How do you de-risk the deadline?

-   Validate API access, authentication, schemas, and rate limits early.
-   Agree on the smallest useful scope and measurable acceptance
    criteria.
-   Build a thin end-to-end prototype with realistic data.
-   Identify external dependencies and assign owners.
-   Test failure cases before rollout.
-   Reserve time for security review, user acceptance testing, and
    production monitoring.
-   Use feature flags, a staged rollout, and a rollback plan.

### Q30. How would you handle a conflict between customer urgency and reliability?

Explain the risk in terms of customer impact, propose a smaller safe
release, distinguish must-have controls from later enhancements, and
agree on measurable acceptance criteria. Never silently remove
safeguards such as authorization, durable persistence, or duplicate
protection to meet a date.

------------------------------------------------------------------------

## 9. Mock interview drills

Attempt each prompt in 5--10 minutes without reading the earlier
sections.

### System design drills

-   [ ] Design a reliable Certa-to-ServiceNow vendor synchronization
    service.
-   [ ] Design a webhook receiver supporting duplicates, out-of-order
    events, retries, and replay.
-   [ ] Design a vendor onboarding workflow that calls three external
    services with different rate limits.
-   [ ] Design reconciliation for two systems that occasionally drift
    out of sync.
-   [ ] Design an LLM-assisted PDF extraction workflow with human review
    and auditability.

### Debugging drills

-   [ ] A vendor was created twice after a timeout. Find the likely race
    and prevent recurrence.
-   [ ] Webhooks are arriving, but queue lag is growing.
-   [ ] A sync job succeeds locally but the destination record remains
    unchanged.
-   [ ] A deployment causes 422 responses for a subset of vendors.
-   [ ] A retry storm begins after a provider returns 429.
-   [ ] A replay causes an older vendor version to overwrite a newer
    one.

### Coding and SQL drills

-   [ ] Two-sum with expected O(n) time.
-   [ ] Top three products by price per city.
-   [ ] Count unique vendors and total entries per region for a given
    year.
-   [ ] Find statuses with at least N records.
-   [ ] Implement a bounded retry policy that respects `Retry-After`.
-   [ ] Validate and normalize a vendor JSON payload.

### Customer communication drills

-   [ ] Explain why a timed-out request may still have succeeded.
-   [ ] Explain why "exactly once" is not a safe default assumption.
-   [ ] Explain a deadline risk without overwhelming a nontechnical
    stakeholder.
-   [ ] Write a short incident update that states impact, mitigation,
    and next steps.

------------------------------------------------------------------------

## 10. Self-scoring rubric

Score each answer from 0 to 4.

  -----------------------------------------------------------------------
                                     Score Meaning
  ---------------------------------------- ------------------------------
                                         0 No answer or fundamentally
                                           incorrect

                                         1 Knows a term but cannot
                                           explain the behavior

                                         2 Correct basic approach,
                                           important edge cases missing

                                         3 Correct, practical, includes
                                           relevant failure handling

                                         4 Production-ready reasoning,
                                           trade-offs, security,
                                           observability, and recovery
  -----------------------------------------------------------------------

Track your scores:

  Topic                                Score / 4 Next improvement
  ---------------------------------- ----------- ------------------
  REST and HTTP                           \_\_\_ \_\_\_
  Authentication and authorization        \_\_\_ \_\_\_
  SQL                                     \_\_\_ \_\_\_
  Python/API coding                       \_\_\_ \_\_\_
  Webhooks and idempotency                \_\_\_ \_\_\_
  Queues, retries, and DLQ                \_\_\_ \_\_\_
  Debugging and observability             \_\_\_ \_\_\_
  Customer discovery and delivery         \_\_\_ \_\_\_
  LLM output validation                   \_\_\_ \_\_\_

**Readiness target:** Aim for 3 or 4 consistently, and be able to
explain why alternatives fail.

------------------------------------------------------------------------

## 11. Final revision sheet

-   **Timeout:** Outcome is unknown; reconcile or retry with the same
    idempotency key.
-   **Duplicate webhook:** Persist and deduplicate using a unique event
    ID.
-   **Out-of-order event:** Compare versions; never blindly overwrite
    newer state.
-   **429:** Honor `Retry-After`, back off, add jitter, and limit
    concurrency.
-   **503 / transient network error:** Bounded retry if the operation is
    safe.
-   **422:** Fix the payload or business rule; do not retry unchanged.
-   **401 vs 403:** Authentication failure vs insufficient permission.
-   **Database + queue dual write:** Use a transactional outbox or
    equivalent durable design.
-   **DLQ:** Preserve context, alert, diagnose, then replay safely.
-   **Sync correctness:** Use stable IDs, explicit field ownership,
    version checks, and reconciliation.
-   **SQL aggregation:** `WHERE` filters rows; `GROUP BY` groups;
    `HAVING` filters groups.
-   **Unique count:** `COUNT(DISTINCT id)`; total rows: `COUNT(*)`.
-   **Large-table query:** Inspect the execution plan and indexes before
    guessing.
-   **LLM extraction:** Schema validation plus evidence, deterministic
    checks, and human review.
-   **Production debugging:** Reproduce, trace, isolate, fix, replay,
    verify, prevent recurrence.
-   **Customer delivery:** Clarify outcomes, validate dependencies, ship
    a thin safe slice, measure success.

------------------------------------------------------------------------

## 12. Reusable practice prompt

Copy the prompt below into an AI practice session when you want a fresh
mock interview.

> Act as a rigorous interviewer for a Forward Deployed Engineer role
> focused on enterprise integrations, vendor onboarding, REST APIs, SQL,
> Python, webhooks, backend reliability, and customer-facing delivery.
>
> Ask one question at a time. Mix practical coding, SQL, debugging,
> system design, API semantics, authentication, idempotency, retries,
> queues, rate limits, schema mapping, reconciliation, observability,
> LLM extraction reliability, and customer discovery.
>
> Do not reveal the answer until I respond. After each answer, score
> correctness and production readiness from 0--4, identify
> misconceptions, explain the ideal approach, and provide a concise
> model answer. Ask a realistic follow-up that tests edge cases or
> trade-offs.
>
> For coding questions, require working code, complexity analysis, and
> edge cases. For SQL, require exact query semantics. For system design,
> evaluate correctness, failure handling, security, observability, and
> rollout strategy. For customer questions, evaluate clarification,
> assumptions, prioritization, and communication.
>
> Track weak topics across the session and prioritize them. At the end,
> provide a scorecard, the five most important gaps, and a targeted
> revision plan. Do not assume undocumented facts about the employer;
> distinguish stated requirements from assumptions.
