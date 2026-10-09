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

## Cluster 5: Interfaces vs. Abstract Classes

### What is it?
An interface declares a contract (method signatures, optionally with default implementations since Java 8) that any implementing class must fulfill; a class can implement *multiple* interfaces. An abstract class can hold state (fields), constructors, and a mix of implemented and unimplemented methods, but a class can extend only *one*.

### How does it work?
```java
interface Movable { void move(); default void stop() { System.out.println("stopped"); } } // default method
abstract class Vehicle {
    protected int speed;                       // abstract classes CAN hold state
    Vehicle(int speed) { this.speed = speed; }  // and constructors
    abstract void accelerate();                 // still can declare abstract methods
}
class Car extends Vehicle implements Movable {
    Car(int speed) { super(speed); }
    void accelerate() { speed += 10; }
    public void move() { System.out.println("driving at " + speed); }
}
```

### Interview Questions
- **L1:** "Interfaces define a pure contract and support multiple implementation; abstract classes can hold shared state/constructors but only support single inheritance."
- **L2:** explain why Java added default methods to interfaces in Java 8: to let library authors add new methods to an existing interface (e.g., `List.stream()`) without breaking every class that already implements it — without default methods, adding any new interface method would be a breaking API change for all implementers.
- **L3:** explain the classic decision rule: use an abstract class when subclasses share common *state or implementation* (an "is-a" relationship with real shared code); use an interface when you're defining a *capability* multiple unrelated classes might have (a "can-do" relationship) — and note that a class can combine both, as `Car` does above.

### DO NOT SAY THIS
"Interfaces can never have any implementation." — Default and static methods (Java 8+) can carry implementation; the real, still-true distinction is that interfaces cannot hold *instance state* (fields with per-instance values), not that they can hold *no code at all*.

---

## Cluster 6: Generics & Type Erasure

### What is it?
Java generics are checked at compile time but **erased** at runtime — `List<String>` and `List<Integer>` are the same class (`List`) at the bytecode level; the compiler inserts casts for you and prevents type-unsafe usage, but no generic type information survives into the running program.

### How does it work?
```java
List<String> list = new ArrayList<>();
list.add("hello");
// Bytecode-level: effectively List list = new ArrayList(); list.add("hello");
// with a compiler-inserted cast wherever you READ from the list: String s = (String) list.get(0);

// Bounded wildcards — PECS: Producer Extends, Consumer Super
void copy(List<? extends Number> src, List<? super Number> dest) {  // read from src, write to dest
    for (Number n : src) dest.add(n);
}
```

### Interview Questions
- **L1:** "Java generics are compile-time-only — erased at runtime, meaning `List<String>` and `List<Integer>` are identical classes once compiled; the compiler enforces type safety and inserts casts for you."
- **L2:** explain why you can't do `new T[10]` inside a generic method or `instanceof List<String>` — both require runtime type information that erasure has already discarded; this is a very common "gotcha" question testing whether you understand erasure isn't just trivia but has real, checkable consequences.
- **L3:** explain the PECS mnemonic (**P**roducer **E**xtends, **C**onsumer **S**uper) precisely: use `? extends T` for a generic parameter you only *read* from (it can be any subtype, safely widened to T on read), and `? super T` for one you only *write* to (it can be any supertype, safely accepting a T being written) — getting this backwards is a classic, specific mistake worth being able to name and correct on the spot.

---

## Cluster 7: Exceptions

### What is it?
**Checked exceptions** (extend `Exception`, not `RuntimeException`) must be either caught or declared in a method's `throws` clause — the compiler enforces handling. **Unchecked exceptions** (extend `RuntimeException`) require no such declaration — typically used for programming errors (`NullPointerException`, `IllegalArgumentException`) rather than recoverable conditions.

### How does it work?
```java
// try-with-resources — AutoCloseable is guaranteed closed, even on exception, no manual finally needed
try (FileReader reader = new FileReader("file.txt")) {
    // use reader
} catch (IOException e) {
    throw new ServiceException("failed to read config", e);   // exception chaining — preserves the original cause
}
```

### Interview Questions
- **L1:** "Checked exceptions must be caught or declared, enforced by the compiler, typically for recoverable conditions; unchecked (RuntimeException) exceptions need no declaration, typically for programming errors."
- **L2:** explain try-with-resources precisely: any object implementing `AutoCloseable` declared in the `try(...)` parentheses has its `close()` method called automatically when the block exits, *even if an exception was thrown* — eliminating the classic bug of a `finally` block being forgotten or itself throwing and masking the original exception.
- **L3:** explain exception chaining (`throw new X(msg, originalException)`): preserves the original exception as the new one's *cause*, so a stack trace shows the full causal chain rather than losing the original error's context — a real, checkable production-code-quality signal worth mentioning proactively when discussing error handling in any of your projects.

### DO NOT SAY THIS
"Checked exceptions are just an outdated Java quirk with no real benefit." — This is a genuinely contested design opinion in the Java community (Kotlin notably dropped checked exceptions), but as an interview answer it reads as dismissive rather than informed; better: state the intended benefit (compiler-enforced handling of recoverable conditions) and, if asked, the common critique (they can lead to overly broad `catch (Exception e)` blocks or exception-swallowing to satisfy the compiler) — showing you understand the tradeoff rather than just having an opinion.

### Quick Revision
- JVM = executes bytecode, manages GC. JDK = build + run. JRE = run only.
- `equals()`/`hashCode()` contract: equal objects MUST have equal hash codes.
- `ExecutorService`: bounded thread pools over raw `Thread` creation.
- Spring Boot: DI + auto-configuration; `@Transactional` self-invocation is a classic trap.
- Interface = pure contract, multiple allowed; abstract class = shared state/code, single inheritance only.
- Generics are erased at runtime — no `new T[]`, no `instanceof List<String>`. PECS: extends for producers (read), super for consumers (write).
- try-with-resources guarantees `close()` even on exception; exception chaining preserves the original cause in the stack trace.
