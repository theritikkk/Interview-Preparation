# Java

Resume connection: part of Hermes's polyglot stack (42 JUnit5 tests) — but the resume doesn't say *which* Hermes components are Java. Be ready to say so honestly (`NEEDS VERIFICATION`) rather than guess in an interview.

---

## Cluster 1: JVM & Memory

### What is it?
The JVM is the runtime that executes compiled Java bytecode; JDK = compiler + JVM + libraries (for developers); JRE = JVM + libraries only (for running, not building). Memory splits into stack (per-thread, local variables/call frames) and heap (shared, objects, garbage-collected).

### Why does it exist?
"Write once, run anywhere" — bytecode is portable across platforms because the JVM (not the OS) executes it directly, JIT-compiling hot paths to native code at runtime.

### How does it work?
Garbage collection (typically generational): new objects go in the **young generation**; most die quickly (weak generational hypothesis) and are cheaply collected; survivors are promoted to the **old generation**, collected less often but more expensively.

### Interview Questions
- **L1:** "The JVM executes bytecode and manages memory automatically via garbage collection, so you don't manually free objects like in C."
- **L2:** explain generational GC and why it's an efficiency assumption (most objects die young) rather than a universal law.
- **L3:** explain what a "stop-the-world" pause is, why full GC pauses matter for latency-sensitive services (directly relevant if Hermes's Java components sit in a request path), and name at least one modern low-pause collector (G1, ZGC) conceptually.

### DO NOT SAY THIS
"Java has no memory leaks because of garbage collection." — GC prevents *forgetting to free*, not leaks caused by objects being unintentionally kept reachable (e.g., a growing static collection, or listeners never unregistered).

---

## Cluster 2: OOP, Collections, HashMap Internals

### What is it?
Java's `Collections` framework (`List`, `Set`, `Map` and implementations) plus the contract between `equals()` and `hashCode()` that makes hash-based collections work correctly.

### How does it work?
`HashMap` stores entries in buckets by `hashCode() % table_size`; on collision, entries chain (linked list, or a balanced tree in Java 8+ once a bucket gets large). **The contract:** if `a.equals(b)` is true, `a.hashCode() == b.hashCode()` must also be true — break this and objects silently vanish from `HashMap`/`HashSet`.

```java
class Point {
    int x, y;
    @Override public boolean equals(Object o) {
        if (!(o instanceof Point p)) return false;
        return x == p.x && y == p.y;
    }
    @Override public int hashCode() { return Objects.hash(x, y); } // MUST override alongside equals
}
```

### Interview Questions
- **L1:** state the equals/hashCode contract in one sentence.
- **L2:** explain why overriding `equals()` without `hashCode()` breaks `HashMap` lookups (the object hashes to a "wrong" bucket relative to where an equal object would look for it).
- **L3:** explain `HashMap`'s treeification (Java 8+): buckets with 8+ colliding entries convert from a linked list to a red-black tree, changing worst-case lookup from O(n) to O(log n) — a good sign of real depth if you can bring this up unprompted.

### Comparison With Alternatives
`HashMap` (unordered, O(1) average) vs. `TreeMap` (sorted by key, O(log n), red-black tree) vs. `LinkedHashMap` (insertion or access order preserved).

---

## Cluster 3: Concurrency — Threads, Synchronization, Executors, CompletableFuture

### What is it?
Java's concurrency toolkit: raw `Thread`/`synchronized`, higher-level `ExecutorService` (thread pool management), and `CompletableFuture` (composable async computation).

### How does it work?
```java
ExecutorService pool = Executors.newFixedThreadPool(4);
CompletableFuture<Integer> future = CompletableFuture
    .supplyAsync(() -> computeExpensive(), pool)
    .thenApply(result -> result * 2);
```
`synchronized` gives mutual exclusion via an intrinsic lock per object; `ExecutorService` decouples "submitting work" from "managing threads," avoiding the cost of spawning a thread per task.

### Interview Questions
- **L1:** "`synchronized` gives mutual exclusion on an object's monitor; `ExecutorService` manages a pool of reusable threads instead of creating one per task; `CompletableFuture` lets you chain async operations without blocking."
- **L2:** explain why unbounded thread creation is dangerous (memory per thread, context-switch overhead, OS thread limits) and how a bounded pool with a work queue avoids it.
- **L3:** explain the difference between `thenApply` (sync continuation on the same/calling thread context) and `thenApplyAsync` (dispatches to the pool) and why that choice affects thread starvation.

### Resume Connection
If your Hermes Java components handle request-level orchestration, this cluster is your most likely deep-dive — be ready to say plainly if the resume doesn't specify enough to answer implementation questions (`NEEDS VERIFICATION`).

---

## Cluster 4: Spring Boot Fundamentals

⚠️ **Exposure note:** listed as a skill, not tied to any project bullet. Prepare this at "solid fundamentals, ready to build with it" depth — don't imply a shipped Spring Boot project you don't have.

### What is it?
Spring Boot is an opinionated framework on top of Spring that auto-configures a production-ready application (embedded server, sensible defaults) so you write business logic instead of wiring.

### How does it work?
- `@RestController` → handles HTTP requests
- `@Service` → business logic layer
- `@Repository` → data access layer (often via Spring Data JPA)
- Dependency Injection: Spring's container instantiates and wires these beans together via `@Autowired`/constructor injection, rather than you calling `new` yourself.

```java
@RestController
class UserController {
    private final UserService service;
    UserController(UserService service) { this.service = service; } // constructor injection — preferred
    @GetMapping("/users/{id}")
    User get(@PathVariable Long id) { return service.findById(id); }
}
```

### Interview Questions
- **L1:** "Spring Boot auto-configures a Spring application with sensible defaults and an embedded server, so you can go from zero to a running REST API quickly."
- **L2:** explain dependency injection's purpose: decoupling classes from the responsibility of constructing their own dependencies, which makes testing (swap in a mock) and configuration (swap an implementation) trivial.
- **L3:** explain a transaction boundary with `@Transactional` — what happens on an exception inside it (rollback), and the classic gotcha that self-invocation (calling an `@Transactional` method from within the same class) bypasses the proxy and silently doesn't apply the transaction.

### DO NOT SAY THIS
"I built a production Spring Boot service for [X]" if you haven't — say instead: "I know Spring Boot's core model (controllers/services/repositories, DI, JPA) from study; my hands-on backend project work has been in Node.js/TypeScript and Python."

### Quick Revision
- JVM = executes bytecode, manages GC. JDK = build + run. JRE = run only.
- `equals()`/`hashCode()` contract: equal objects MUST have equal hash codes.
- `ExecutorService`: bounded thread pools over raw `Thread` creation.
- Spring Boot: DI + auto-configuration; `@Transactional` self-invocation is a classic trap.
