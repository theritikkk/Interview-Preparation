# Project Deep Dive: Hermes — Serverless Workflow Orchestration Platform

🔴🔴🔴 **Verified directly against your source code** (`hermes-release-v1.0.0.zip`). This repo is unusually well-documented for interview purposes already — it has its own `INTERVIEW_GUIDE.md`, `DECISIONS.md` (ADRs), `TRADE_OFFS.md`, and `EVIDENCE.md`. Where those exist, they're the authoritative source; this file synthesizes them with direct code verification and flags what changed from the earlier speculative draft.

---

## The Single Biggest Risk Item Is Now Resolved: Your Six Read Models

Confirmed directly from Terraform (`infra/environments/dev/*.tf`, `dynamodb-table` module instantiations):

1. **execution-read-model** — execution state/status queries
2. **workflow-read-model** — workflow definition/instance queries
3. **asset-read-model** — asset/document queries
4. **tenant-read-model** — tenant metadata queries
5. **metrics-read-model** — usage/metrics queries
6. **audit-read-model** — audit trail queries

Know these six names cold. "Why six read models" now has a real, specific answer instead of a generic "each is optimized for its own query" — you can name what each one actually serves.

---

## Corrections and Important Nuances From the Earlier (Pre-Verification) Draft

### 1. The "polyglot" framing needs precision — TypeScript is the deployed core; Java and Python are real but labeled differently by your own README
Your README's own runtime table is explicit: **TypeScript/Node 20 is "Active Core Platform (100% Implemented & Deployed)"** — 13 named Lambda services (`command-api`, `query-api`, `execution-projection`, `usage-projection`, `opensearch-projection`, `outbox-publisher`, `snapshot-trigger`, `dlq-handler`, `webhook-dispatcher`, `outbox-republisher`, `validate-worker`, `ocr-worker`, `classify-worker`). **Java (Spring Boot, `admin-service` + `replay-service`) and Python (`ai-workers`) are labeled "Architectural Expansion Option"** in the same table (ADR-014 and ADR-023 respectively) — but this doesn't mean stub code. Both have substantial, real, tested implementations (proper hexagonal/DDD-style Java with Spring Security and JWT converters; real Python handlers). The honest, precise framing for an interview: *"The TypeScript core is the actively deployed platform. I built out full, tested Java and Python services as an architectural expansion — proving the domain model and event contracts hold across languages — documented as ADRs rather than folded into the 'active' deployment claim."* This is a **stronger** answer than an unqualified "polyglot, all three deployed" claim, because it's precise about what's live vs. what's proven-but-optional, and it preempts the obvious follow-up ("show me the Java service running in prod") with an honest answer instead of an exposed overclaim.

### 2. Test counts: Java and Python match your resume exactly; TypeScript has grown
Verified via `TESTING.md`: **Java = 42 tests** (exact match), **Python = 9 tests** (exact match), **TypeScript = 260 tests / 57 suites** (your resume says 234 / 51+ suites). The codebase has grown since the resume snapshot — a normal, easily-explained gap, not a discrepancy to worry about. If asked directly: *"That was accurate as of when I wrote my resume; I've kept building since — it's 260 TypeScript tests across 57 suites now."*

### 3. Optimistic concurrency is two sequential conditional writes, not one atomic transaction — a genuinely good, precise detail to know
Verified in `shared/event-store/src/index.ts`: the append path does **two separate conditional `PutCommand` calls**, not a single `TransactWriteItems`. First, a conditional `PutCommand` on a META item — `ConditionExpression: 'attribute_not_exists(version) OR version = :expectedVersion'` — which is what actually resolves the race (only one of N concurrent writers can succeed here for a given expected version). Second, a conditional `PutCommand` on the event item itself — `ConditionExpression: 'attribute_not_exists(SK)'` — preventing an overwrite at that sequence number. **The precise, honest answer if asked "is this atomic?":** the *concurrency resolution* is atomic (the META item's conditional write is the actual arbiter of who wins), but the two writes are not wrapped in a single DynamoDB transaction — a crash between them is a real, if narrow and recoverable, edge case (a claimed version with no corresponding event row). This is exactly the kind of precise, non-overclaiming answer that plays well against a rigorous interviewer: state the mechanism correctly, acknowledge the edge case, and note it's a deliberate tradeoff (avoiding the extra cost/complexity of `TransactWriteItems` for a two-item write) rather than an oversight you're unaware of.

### 4. Tenant claim extraction has a documented dev-mode fallback — know it exists, and confirm it's not reachable in production
Verified in `shared/domain/src/index.ts`, `extractAuthContext()`: the primary path reads `event.requestContext.authorizer.jwt.claims`, pulling `custom:tenantId` (with fallbacks to `custom:tenant_id`/`tenantId`), `sub` for userId, and roles from `cognito:groups`. **There is also an explicit fallback**, commented `"Fallback for unauthenticated / header-based dev access"`, that reads `x-tenant-id`/`x-role` HTTP headers directly with **no cryptographic verification**, defaulting to `['Admin', 'User', 'Service']` roles if no role header is present. This is a completely standard, reasonable pattern for local/offline testing without a real Cognito authorizer attached — but it's worth being able to say plainly, if asked, that this path only fires when `requestContext.authorizer` is absent, which should only happen outside a real API Gateway + Cognito authorizer deployment. **Action item for you, not just interview prep:** confirm your API Gateway config requires the Cognito authorizer on every route in the deployed environment, so this fallback is genuinely unreachable in production — if you haven't checked this, it's worth five minutes before an interview, since "does this ever run in prod" is a very natural, very sharp follow-up once an interviewer knows the fallback exists.

Related, and worth stating precisely: `enforceTenantIsolation()` includes an explicit Admin-role cross-tenant bypass (`if (auth.roles.includes('Admin') && requestedTenantId) return requestedTenantId`) — meaning RBAC and tenant isolation aren't fully independent checks; an Admin can operate across tenant boundaries by design. Good to know precisely rather than describe tenant isolation as an absolute wall with no exceptions.

### 5. Terraform: exactly 18 modules, confirmed
`infra/modules/` has exactly 18 directories — matches your resume exactly. Worth knowing a few by name if asked to elaborate: `cognito-user-pool`, `dynamodb-table`, `eventbridge` (+ `eventbridge-sfn-role`, `eventbridge-sfn-target`), `http-api` (confirms **HTTP API**, not REST API, type — a real, specific detail), `kms-key`, `lambda-function`, `opensearch`, `rds-postgres` (a relational DB module exists too — likely supporting the Java services' JPA/Hibernate persistence, separate from the DynamoDB-based event store/read models), `s3-bucket`, `sfn-exec-role`, `sqs-queue`, `multi-region-dr`, `workflow-registry`.

---

## Verified Interview-Specific Facts (From Your Own `INTERVIEW_GUIDE.md`)

Your repo already has purpose-built defense documentation — these are the specific numbers/decisions it cites:
- **Snapshotting is implemented**, triggered at **N=50 events** per aggregate — resolves the earlier "does Hermes snapshot?" unknown definitively: yes.
- Single-table DynamoDB key design: `TENANT#<id>`, `EXEC#<id>` — confirms tenant ID is embedded directly in the partition key, which is both a distribution strategy and a tenant-isolation mechanism at the storage layer (defense-in-depth alongside the application-level JWT check).
- OpenSearch indexes are **monthly-rotated** (`hermes-executions-YYYY.MM`) — a specific, concrete detail for index-lifecycle/retention questions.
- WAFv2 rate limiting: **1,000 requests per 5 minutes per IP.**
- KMS: a customer-managed key (CMK) with **annual rotation.**

Read `INTERVIEW_GUIDE.md`, `DECISIONS.md`, `TRADE_OFFS.md`, and `EVIDENCE.md` in full before an interview — they were written specifically for this purpose and are more authoritative than anything reconstructed secondhand here.

---

## Quick Revision
- Six read models: execution, workflow, asset, tenant, metrics, audit — know these names cold, this was the #1 risk item and it's now closed.
- TypeScript = active deployed core (13 named Lambdas); Java/Python = real, tested, but documented as "architectural expansion" (ADR-014/ADR-023) — a precise framing, not a downgrade.
- Optimistic concurrency = two sequential conditional PutCommands (META item resolves the race, event item prevents overwrite), not one atomic transaction — know the narrow crash-window edge case and why it's an acceptable tradeoff.
- Tenant ID comes from the verified JWT claim in production; a header-based dev fallback exists and should be confirmed unreachable outside local testing.
- Admin role has an explicit cross-tenant bypass in the isolation check — RBAC and tenant isolation interact, they're not fully separate.
- 18 Terraform modules confirmed exactly; `http-api` module confirms HTTP API (not REST API) type; `rds-postgres` module exists alongside DynamoDB, likely backing the Java services.
- Your own `INTERVIEW_GUIDE.md`/`DECISIONS.md`/`TRADE_OFFS.md`/`EVIDENCE.md` are authoritative — read them directly, don't rely solely on secondhand summaries.
