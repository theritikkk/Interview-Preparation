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
