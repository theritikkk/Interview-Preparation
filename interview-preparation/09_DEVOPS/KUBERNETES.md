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

---

## Cluster 3: Config Management & Cluster Architecture

### What is it?
A **ConfigMap** holds non-sensitive configuration (environment variables, config files) decoupled from the container image — the same image can run in dev/staging/prod with different ConfigMap-supplied config, echoing Spring's profiles (`04_BACKEND/SPRING_BOOT.md`) or `.env`-style config generally. A **Secret** holds the same kind of data but for sensitive values (base64-encoded at rest by default — **not encrypted** unless additional encryption-at-rest is configured, a very common misconception). A **Namespace** partitions a cluster into isolated logical groups (e.g., separate namespaces per team or environment sharing one physical cluster). The **control plane** (API server, scheduler, controller manager, etcd) makes cluster-wide decisions; the **scheduler** specifically decides which node a new Pod runs on, based on resource requests and constraints.

### How does it work?
```yaml
apiVersion: v1
kind: ConfigMap
metadata: { name: app-config }
data: { LOG_LEVEL: "info" }
---
apiVersion: v1
kind: Secret
metadata: { name: db-secret }
type: Opaque
data: { password: cGFzc3dvcmQ= }   # base64-ENCODED, not encrypted — treat accordingly
```

### Interview Questions
- **L1:** "ConfigMaps hold non-sensitive config, decoupled from the image; Secrets hold sensitive values but are only base64-encoded by default, not encrypted, at rest — real encryption needs additional cluster configuration. Namespaces logically partition a cluster; the control plane (API server, scheduler, etcd) makes cluster-wide scheduling and state decisions."
- **L2 — a very common trap:** explain precisely why "Secrets are secure by default" is false: base64 is an *encoding*, not encryption — trivially reversible by anyone with read access to the Secret object; real protection requires enabling encryption-at-rest for etcd (where Secrets are ultimately stored) and tightly scoping RBAC permissions on who can read Secret objects at all — the Secret *object type* signals intent and gets some special handling (not exposed in `kubectl describe` output by default, for instance), but that's a usability/audit convenience, not cryptographic protection on its own.
- **L3:** explain what the scheduler actually optimizes for when placing a Pod: matching the Pod's resource *requests* (`04_BACKEND` and `09_DEVOPS/DOCKER.md`'s resource-limits discussion, at the cluster level instead of the single-host level) against each node's available capacity, plus any additional constraints (node affinity/taints-and-tolerations) — connecting this to your own Docker resource-limits knowledge is a strong way to show the concept transfers, even without hands-on K8s scheduling experience.

### DO NOT SAY THIS
"Kubernetes Secrets encrypt sensitive data automatically." — Base64 encoding is not encryption; say so plainly if asked, and name what real protection actually requires (etcd encryption-at-rest + RBAC scoping) rather than repeating the common misconception.

### Quick Revision (Full File)
- Pod < Deployment (manages replicas) < Service (stable address) < Ingress (external routing) — know the layering.
- Liveness = restart if failing; readiness = stop routing traffic if failing. Different failure responses for different problems.
- Be honest that hands-on K8s orchestration isn't demonstrated in your projects — your Task Scheduler achieves similar goals (scaling, fault tolerance) via direct Docker management instead.
- ConfigMap: non-sensitive config, decoupled from the image. Secret: base64-*encoded*, not encrypted, by default — a very common and checkable misconception to avoid repeating.
- Control plane (API server, scheduler, etcd) makes cluster-wide decisions; the scheduler places Pods based on resource requests vs. node capacity — the cluster-level analog of Docker's single-host resource limits.
