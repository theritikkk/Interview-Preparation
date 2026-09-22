# Kubernetes

⚠️ Skills-list item — your actual projects use Docker directly (Task Scheduler), not K8s orchestration. Prepare conceptually and be ready to say so honestly if asked for a K8s project story.

---

## Cluster 1: Core Objects

### What is it?
A **Pod** is the smallest deployable unit (one or more tightly-coupled containers sharing network/storage). A **Deployment** manages a desired number of Pod replicas, handling rolling updates and self-healing (replacing crashed Pods automatically). A **Service** gives a stable network identity/DNS name in front of a set of Pods whose individual IPs change as they're recreated. An **Ingress** routes external HTTP(S) traffic into Services based on host/path rules.

### Interview Questions
- **L1:** "A Pod runs containers; a Deployment manages replicas of a Pod and handles rolling updates/self-healing; a Service gives a stable address in front of Pods with changing IPs; Ingress routes external traffic in based on host/path."
- **L2:** explain why a Service is necessary at all given Pods already have IPs — Pod IPs are ephemeral (a new IP every time a Pod is recreated after a crash or a rolling update), so anything depending on a Pod's IP directly would break constantly; a Service provides a stable abstraction layer over that churn.
- **L3:** if asked to contrast with your actual Task Scheduler design: your worker fleet's "horizontal scaling" and fault tolerance goals (per your resume) are conceptually what a Kubernetes Deployment automates (desired replica count, self-healing) — but you achieved it via direct Docker container management instead; a reasonable, honest answer to "why not K8s here" is that the project's scope didn't need K8s's full orchestration surface (service discovery, rolling updates, auto-scaling) for a fixed-size worker fleet you controlled directly.

---

## Cluster 2: Probes & Autoscaling

### What is it?
A **liveness probe** checks if a container is still alive (K8s restarts it if not); a **readiness probe** checks if it's ready to receive traffic (K8s stops routing to it if not, without necessarily restarting it) — a subtle but important distinction. **Horizontal Pod Autoscaling (HPA)** adjusts replica count automatically based on a metric (commonly CPU/memory, or a custom metric).

### Interview Questions
- **L1:** "Liveness probes trigger a restart if a container is unhealthy; readiness probes control whether traffic is routed to it at all, without necessarily restarting it; HPA scales replica count based on observed load."
- **L2:** explain a concrete scenario where the liveness/readiness distinction matters: a Pod that's temporarily overloaded and slow to respond (but not actually dead) should fail its readiness probe (stop receiving new traffic, let it recover) rather than its liveness probe (which would restart it, likely making the underlying overload problem worse by briefly reducing capacity further during the restart).

### Quick Revision
- Pod < Deployment (manages replicas) < Service (stable address) < Ingress (external routing) — know the layering.
- Liveness = restart if failing; readiness = stop routing traffic if failing. Different failure responses for different problems.
- Be honest that hands-on K8s orchestration isn't demonstrated in your projects — your Task Scheduler achieves similar goals (scaling, fault tolerance) via direct Docker management instead.
