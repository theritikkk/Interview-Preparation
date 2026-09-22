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
