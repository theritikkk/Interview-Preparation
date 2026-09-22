# Node.js

Runtime under Hermes and the Distributed Task Scheduler. Event-loop/microtask mechanics are covered in `01_PROGRAMMING/JAVASCRIPT.md` (Cluster 1) and `epoll`'s role underneath it in `03_CS_FUNDAMENTALS/OS.md` (Cluster 4) — this file covers what's specific to the Node *runtime* itself, not the JS language.

---

## Cluster 1: Non-Blocking I/O, libuv, Buffers & Streams

### What is it?
Node's single JS thread never blocks on I/O — libuv (Node's C++ I/O layer) hands off filesystem/network/DNS work to a background thread pool (default size 4) or the OS's async facilities, and delivers results back to the JS event loop as callbacks. A **Buffer** is a fixed-size raw binary chunk (outside the V8 heap); a **Stream** processes data incrementally (chunk by chunk) instead of loading everything into memory at once.

### How does it work?
```js
const fs = require('fs');
fs.createReadStream('huge-file.log')
  .pipe(transformStream)          // process each chunk as it arrives — constant memory regardless of file size
  .pipe(fs.createWriteStream('out.log'));
```

### Interview Questions
- **L1:** "Node's single JS thread stays non-blocking by delegating I/O to libuv, which uses a background thread pool or OS async facilities and delivers results as callbacks via the event loop."
- **L2:** explain why `fs.readFileSync` inside a request handler is a real production bug: it blocks the single JS thread, stalling *every other concurrent request* until it completes — a direct, practical consequence of Node's single-threaded model that's very easy to introduce accidentally.
- **L3:** explain when you'd reach for a Stream instead of loading a full file/response into memory — directly relevant if either of your Node services processes large payloads (e.g., bulk task imports in the Task Scheduler, or large event payloads in Hermes).

### DO NOT SAY THIS
"Node.js is single-threaded, so it can't do CPU-heavy work at all." — It can, via `worker_threads` (true parallel JS execution) or by offloading to a separate process/service — the limitation is specifically that the *default* single event-loop thread shouldn't be blocked by CPU-heavy synchronous code, not that Node has no path to parallelism.

---

## Cluster 2: Scaling — Clustering & Worker Threads

### What is it?
`cluster` forks multiple Node *processes* (each with its own event loop and V8 instance) sharing a listening port, load-balanced by the OS/Node — this scales across CPU cores for I/O-bound workloads by running N independent event loops. `worker_threads` gives true multi-threaded parallelism *within* one process for CPU-bound work, at the cost of no shared memory by default (message passing, or explicit `SharedArrayBuffer`).

### Interview Questions
- **L1:** "`cluster` runs multiple Node processes to use multiple CPU cores for I/O-bound concurrency; `worker_threads` gives real parallel execution for CPU-bound work within one process."
- **L2:** connect this to your Task Scheduler's worker fleet: if your Node.js workers are separate OS processes (not `cluster`-managed), that's a deliberate choice for **fault isolation** (one worker crashing doesn't affect siblings) over the shared-listening-port convenience `cluster` provides — `NEEDS VERIFICATION` on which model your worker fleet actually uses.

### Quick Revision
- Never call a synchronous, blocking function inside a request-handling code path.
- Streams = constant memory for large data, chunk by chunk.
- `cluster` = multi-process, I/O-bound scaling. `worker_threads` = true parallelism, CPU-bound work.

---

## Cluster 3: Structured Logging

### What is it?
Structured logging emits logs as machine-parseable data (JSON) with consistent fields, rather than free-form text strings — essential for logs to be searchable/filterable at scale (directly what `10_OBSERVABILITY/LOKI.md`'s label-based model expects) rather than only grep-able.

### How does it work?
```js
logger.info('task_dispatched', { taskId, workerId, tenantId, durationMs, correlationId });
// vs. the unstructured version, much harder to query at scale:
console.log(`Task ${taskId} dispatched to worker ${workerId}`);
```
A **correlation ID** (generated at the request's entry point, passed through every downstream call and log line) is what lets you reconstruct one logical request's full path across services from logs alone — the same concept as X-Ray's trace ID (`10_OBSERVABILITY/XRAY.md`), implemented at the log-line level instead of a dedicated tracing system.

### Interview Questions
- **L1:** "Structured (JSON) logging with consistent fields makes logs queryable at scale, unlike free-form text; a correlation ID threaded through every log line for one request lets you reconstruct its full path across services."
- **L2:** explain why log *level* discipline matters operationally (`debug`/`info`/`warn`/`error`) — logging everything at `info` in production either floods your log storage/costs or buries real signal; the practical discipline is `error` for things requiring attention, `warn` for recoverable anomalies, `info` for major lifecycle events (not every function call), `debug` disabled by default and enabled only when actively investigating.
- **L3:** connect directly to your Task Scheduler: with 3 worker replicas, a correlation ID tying a task's full lifecycle (dispatched → claimed → processed → acked) together across coordinator and worker logs is exactly what would let you answer "why did task X take so long" by reading logs alone, without needing to manually cross-reference timestamps across three separate log streams — a concrete, project-specific application of this concept.

### Final Quick Revision (All Clusters)
- Structured (JSON) logs + a correlation ID per request = the log-level equivalent of distributed tracing — essential once more than one process/service is involved.
- Log level discipline (error/warn/info/debug) prevents both signal-loss and cost/volume blowup in production.