# Docker

🔴 Confirmed in the Distributed Task Scheduler.

---

## Cluster 1: Images, Layers, Multi-Stage Builds

### What is it?
An image is built as a stack of read-only, cacheable **layers**, each corresponding to a Dockerfile instruction — Docker reuses cached layers for instructions that haven't changed, which is why instruction *order* in a Dockerfile matters for build speed. A **multi-stage build** uses multiple `FROM` stages in one Dockerfile, copying only the needed artifacts from an earlier (e.g., build/compile) stage into a final, smaller runtime image — discarding build-time dependencies from the shipped image.

### How does it work?
```dockerfile
FROM node:20 AS build
COPY package*.json ./     # separate layer — cached unless package.json changes
RUN npm ci
COPY . .
RUN npm run build

FROM node:20-slim          # fresh, minimal final stage
COPY --from=build /app/dist ./dist   # only the built output, not devDependencies/build tools
CMD ["node", "dist/index.js"]
```

### Interview Questions
- **L1:** "Docker images are built as cacheable layers per instruction; multi-stage builds let you compile/build in one stage and copy only the final artifacts into a smaller, leaner runtime image, leaving build-time dependencies behind."
- **L2:** explain why copying `package*.json` and running `npm ci` *before* copying the rest of the source code (as above) is a deliberate ordering choice: source code changes far more often than dependencies, so this ordering means the expensive `npm ci` layer stays cached across most rebuilds, only re-running when dependencies actually change.
- **L3:** explain the security/size benefit of multi-stage builds concretely: a final image containing only compiled output (not source code, not build tools, not devDependencies) has a smaller attack surface and smaller image size — directly relevant to a Task Scheduler shipped as multiple worker containers, where image size affects both deployment speed and the resource footprint of each replica.

---

## Cluster 2: Networking & Volumes

### What is it?
Docker's default **bridge network** lets containers on the same network reach each other by container name (Docker's embedded DNS) rather than needing hardcoded IPs. **Volumes** persist data outside a container's writable layer, surviving container removal/recreation — essential for anything (like a database) that shouldn't lose data when its container restarts.

### Interview Questions
- **L1:** "Containers on the same Docker network can reach each other by name via Docker's built-in DNS; volumes persist data outside the container's own filesystem so it survives container restarts/recreation."
- **L2:** connect to your Task Scheduler: the coordinator, API gateway, and worker containers likely communicate over a shared Docker network by service name (e.g., `rabbitmq:5672`) rather than hardcoded IPs — and PostgreSQL's data almost certainly lives in a volume, since losing the database's data on every container restart would be an obvious, immediate bug during your own load testing.

### Quick Revision
- Layer caching rewards ordering rarely-changing instructions (dependency install) before frequently-changing ones (source copy).
- Multi-stage builds: smaller, leaner, more secure final images by discarding build-time-only content.
- Containers reach each other by service name over a shared Docker network; volumes persist data across container restarts.

---

## Cluster 3: Security & Resource Limits

### What is it?
By default, a container's main process runs as **root** inside the container — if an attacker exploits the application, they get root *inside the container*, which is a meaningfully worse starting position for further escalation than a non-root user would be. **Resource limits** (`--memory`, `--cpus`, or their Docker Compose/Kubernetes equivalents) cap what a single container can consume, preventing one runaway container from starving every other container on the same host.

### How does it work?
```dockerfile
FROM node:20-slim
RUN addgroup --system app && adduser --system --ingroup app app
USER app                              # drop root — subsequent instructions and CMD run as this user
COPY --chown=app:app . .
CMD ["node", "index.js"]
```
```yaml
# docker-compose.yml resource limits
services:
  worker:
    deploy:
      resources:
        limits: { memory: 512M, cpus: '0.5' }
```

### Interview Questions
- **L1:** "Containers run as root by default, which is a real security risk — set a non-root `USER` in the Dockerfile. Resource limits (memory/CPU) prevent one container from starving others on the shared host."
- **L2:** explain what happens when a container exceeds its memory limit: the kernel's OOM killer terminates the container's process — a hard stop, not graceful degradation — which is exactly why memory limits need to be set thoughtfully based on real observed usage (from your Prometheus metrics, per `10_OBSERVABILITY/PROMETHEUS.md`), not guessed arbitrarily.
- **L3 — 🔴 directly relevant to your Task Scheduler's 3 worker replicas:** explain why per-container resource limits matter specifically in a multi-container deployment on shared hardware: without limits, one worker replica under unusually heavy load (a burst of large tasks) could starve CPU/memory from its sibling replicas and the coordinator, turning a localized load spike into a system-wide degradation — exactly the kind of cascading failure a well-designed resource-limit policy prevents.

### DO NOT SAY THIS
"Running as root inside a container is fine because it's isolated from the host anyway." — Container isolation (namespaces, cgroups) is real but not absolute — container-escape vulnerabilities have existed, and even without one, root-inside-container is unnecessary exposure for essentially zero benefit in the overwhelming majority of application containers; non-root is close to a free security improvement.

### Quick Revision (Full File)
- Layer caching rewards ordering rarely-changing instructions (dependency install) before frequently-changing ones (source copy).
- Multi-stage builds: smaller, leaner, more secure final images by discarding build-time-only content.
- Containers reach each other by service name over a shared Docker network; volumes persist data across container restarts.
- Set a non-root `USER` — root-inside-container is unnecessary exposure with essentially no upside.
- Resource limits prevent one container from starving others on shared hardware; exceeding a memory limit triggers a hard OOM kill, not graceful degradation — size limits from real observed metrics.
