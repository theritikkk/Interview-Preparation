# Spring Boot (Backend Specifics)

Core DI/controller/service/repository model is covered in `01_PROGRAMMING/JAVA.md` (Cluster 4) — including the exposure-risk note (this is a skills-list item, not tied to a project bullet). This file adds backend-operational concepts that would come up if this phase gets probed further.

---

## Cluster 1: Profiles & Configuration

### What is it?
Spring profiles (`application-dev.yml`, `application-prod.yml`) let the same codebase run with different configuration (DB connection strings, log levels, feature flags) depending on environment, activated via `spring.profiles.active`.

### Interview Questions
- **L1:** "Profiles let you swap configuration by environment without changing code — dev, staging, prod each get their own properties file."
- **L2:** explain why secrets (DB passwords, API keys) shouldn't live directly in a profile's properties file checked into source control — externalize via environment variables or a secrets manager instead, and reference them in the properties file.

---

## Cluster 2: Actuator

### What is it?
Spring Boot Actuator exposes production-readiness endpoints out of the box — `/actuator/health`, `/actuator/metrics`, `/actuator/info` — for load balancers (health checks) and monitoring systems to consume without you hand-building them.

### Interview Questions
- **L1:** "Actuator gives you built-in health/metrics endpoints for monitoring and load-balancer health checks, without writing that plumbing yourself."
- **L2:** connect this to your observability stack (`10_OBSERVABILITY/`): Actuator's `/metrics` endpoint is commonly scraped by Prometheus via the Micrometer integration — the same conceptual pipeline your Task Scheduler already uses for its Node/Python services, just Java's idiomatic version of it.

---

## Cluster 3: Exception Handling

### What is it?
`@ControllerAdvice` + `@ExceptionHandler` centralizes exception-to-HTTP-response mapping across all controllers, the Spring equivalent of Express's error-handling middleware (`EXPRESS.md`).

### How does it work?
```java
@ControllerAdvice
class GlobalExceptionHandler {
    @ExceptionHandler(ResourceNotFoundException.class)
    ResponseEntity<?> handleNotFound(ResourceNotFoundException ex) {
        return ResponseEntity.status(404).body(Map.of("error", ex.getMessage()));
    }
}
```

### Interview Questions
- **L1:** "`@ControllerAdvice` centralizes exception handling across all controllers, mapping specific exception types to specific HTTP responses — avoiding repeated try/catch in every handler."
- **L2:** explain the parallel to Express's error middleware directly if asked to compare frameworks — same underlying idea (centralize cross-cutting error-to-response mapping), different mechanism (annotation-based interception vs. an explicit middleware function in the request chain).

### Quick Revision
- Profiles: environment-specific config, never commit secrets directly.
- Actuator: free health/metrics endpoints, natural Prometheus integration point.
- `@ControllerAdvice`: Spring's centralized exception-to-response mapping, conceptually the same job as Express's error middleware.

---

## Cluster 4: JPA / Hibernate — Entity Mapping, Lazy Loading, the N+1 Problem

### What is it?
JPA (the specification) + Hibernate (its most common implementation) map Java objects to relational tables (an ORM) — `@Entity` classes become tables, `@OneToMany`/`@ManyToOne` relationships become foreign keys, and repository methods generate SQL for you. **Lazy loading** (the default for collections) means a related entity/collection isn't fetched from the database until it's actually accessed in code; **eager loading** fetches it immediately as part of the original query.

### How does it work? — The N+1 Query Problem (🔴 a near-certain interview question if Spring/JPA comes up at all)
```java
@Entity class Order { @ManyToOne Customer customer; ... }

List<Order> orders = orderRepository.findAll();      // 1 query: fetch all orders
for (Order o : orders) {
    System.out.println(o.getCustomer().getName());    // LAZY — triggers a SEPARATE query PER order!
}
// Total: 1 + N queries for N orders — the "N+1" problem, a severe, easy-to-introduce performance bug
```
**The fix:** fetch the related data in the *same* query using a JOIN FETCH (JPQL) or an entity graph, collapsing N+1 queries into 1:
```java
@Query("SELECT o FROM Order o JOIN FETCH o.customer")
List<Order> findAllWithCustomer();
```

### Interview Questions
- **L1:** "JPA/Hibernate maps entities to tables; lazy loading defers fetching related data until accessed, which can silently cause the N+1 query problem — one query for the main list, plus one more per row for each lazily-accessed relationship — fixed with JOIN FETCH or an entity graph to collapse it into a single query."
- **L2:** explain why N+1 is specifically dangerous rather than just a minor inefficiency: it's invisible in code review (the loop looks innocuous) and invisible in small local testing (N is small) — it only becomes a severe, often production-incident-triggering problem at real data scale, which is exactly the profile of bug that's easy to ship and expensive to discover late.
- **L3:** explain the lazy-vs-eager tradeoff precisely, since "just make everything eager" is a tempting but wrong fix: eager loading fetches the relationship *every time* the parent entity is loaded, even in code paths that never touch it — trading N+1's occasional-but-severe cost for a smaller, constant cost paid on every single load, which can itself be wasteful; the correct fix is almost always JOIN FETCH scoped to the *specific query* that actually needs the relationship, not a blanket eager-loading default.

### DO NOT SAY THIS
"Switching everything to eager loading fixes the N+1 problem." — It avoids the N+1 *pattern* by fetching upfront, but at the cost of over-fetching in every code path that loads the entity, including ones that never need the relationship — the precise, correct fix is a query-scoped JOIN FETCH, not a global loading-strategy change.

---

## Cluster 5: Transaction Propagation

### What is it?
`@Transactional` wraps a method in a database transaction — but when a `@Transactional` method calls *another* `@Transactional` method, **propagation** rules decide whether the inner call joins the outer transaction, starts a new independent one, or something else. The default, `REQUIRED`, joins the existing transaction if one is active (or starts one if not) — the right default for most cases, since it means the whole call chain commits or rolls back together.

### How does it work?
```java
@Transactional(propagation = Propagation.REQUIRED)      // default — joins caller's transaction if any
void updateOrder() { ... }

@Transactional(propagation = Propagation.REQUIRES_NEW)  // always starts a fresh, independent transaction
void logAuditEvent() { ... }  // e.g., audit logging that should commit even if the outer transaction rolls back
```

### Interview Questions
- **L1:** "Propagation controls how a `@Transactional` method behaves when called from within another transaction — `REQUIRED` (the default) joins the existing one so everything commits/rolls back together; `REQUIRES_NEW` always starts an independent transaction, useful when a sub-operation (like audit logging) needs to survive the outer transaction's rollback."
- **L2:** connect directly to the self-invocation gotcha already noted in `01_PROGRAMMING/JAVA.md` Cluster 4: propagation rules only apply through Spring's proxy mechanism — calling a `@Transactional` method on `this` from within the same class bypasses the proxy entirely, meaning *no* propagation behavior (not `REQUIRED`, not `REQUIRES_NEW`) actually applies, regardless of what's annotated — a real, specific, checkable trap.

### Quick Revision (Full File)
- Profiles: environment-specific config, never commit secrets directly.
- Actuator: free health/metrics endpoints, natural Prometheus integration point.
- `@ControllerAdvice`: Spring's centralized exception-to-response mapping, conceptually the same job as Express's error middleware.
- N+1 query problem: lazy loading + a loop over a relationship = 1+N queries, invisible until real scale — fix with a query-scoped JOIN FETCH, not blanket eager loading.
- `REQUIRED` (default, joins caller's transaction) vs `REQUIRES_NEW` (always independent) — and self-invocation bypasses the proxy, silently disabling propagation entirely.