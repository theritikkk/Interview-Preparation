# AWS Lambda

🔴 Core compute layer of Hermes.

---

## Cluster 1: Execution Model & Cold Starts

### What is it?
Lambda runs your code in a managed, ephemeral execution environment, spun up on demand and torn down after inactivity. A **cold start** is the latency of spinning up a fresh environment (download code, initialize the runtime, run any top-level/init code) before your handler can process its first invocation; a **warm start** reuses an already-initialized environment for a subsequent invocation.

### How does it work?
```js
// Code OUTSIDE the handler runs once per cold start, reused across warm invocations —
// this is where you put expensive one-time setup (DB client, SDK client init).
const dynamoClient = new DynamoDBClient({});

exports.handler = async (event) => {
    // Code INSIDE the handler runs on every invocation.
    return dynamoClient.send(new GetItemCommand({ ... }));
};
```

### Interview Questions
- **L1:** "Lambda spins up an ephemeral execution environment per invocation, reusing it for subsequent invocations when possible; a cold start is the one-time setup latency for a fresh environment."
- **L2:** explain why initializing SDK clients *outside* the handler (module scope) matters — it's created once per cold start and reused across all warm invocations sharing that environment, instead of paying setup cost on every single call.
- **L3:** explain why polyglot Lambdas (Hermes's TS/Java/Python mix) have *different* cold-start profiles — JVM-based Lambdas (Java) typically have meaningfully higher cold-start latency than Node.js or Python, due to JVM startup and class loading — a legitimate reason to be selective about which components get which language, and a good question to be ready for given Hermes's stated polyglot design.

---

## Cluster 2: Concurrency & Limits

### What is it?
Each concurrent invocation gets its own execution environment; **reserved concurrency** caps how many an individual function can use (protects other functions from being starved, and protects downstream resources like a database from being overwhelmed); **provisioned concurrency** keeps a set number of environments pre-warmed to eliminate cold starts for latency-sensitive paths.

### Interview Questions
- **L1:** "Concurrent Lambda invocations each get a separate environment; reserved concurrency caps a function's usage, provisioned concurrency pre-warms environments to avoid cold-start latency."
- **L2:** explain why an unbounded Lambda fan-out (e.g., one event triggering many downstream Lambda invocations) can exhaust a downstream resource's connection limit (like PostgreSQL's max connections) faster than a traditional fixed-size server pool would — this is a legitimate serverless failure mode worth knowing for a "what are the tradeoffs of serverless" question (see `13_PROJECT_DEEP_DIVES/HERMES.md`).

### Quick Revision
- Module-scope code = cold-start-only setup, reused on warm invocations.
- JVM-based Lambdas have materially worse cold-start latency than Node/Python — relevant to a polyglot design's language choices per component.
- Unbounded concurrency can overwhelm a downstream resource with a fixed connection limit — a real serverless-specific failure mode.
