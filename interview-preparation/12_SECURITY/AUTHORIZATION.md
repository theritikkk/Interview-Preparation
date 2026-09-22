# Authorization

RBAC implementation specifics for Hermes are in `08_AWS/COGNITO.md`. This file covers the general model.

---

## Cluster 1: RBAC vs. ABAC

### What is it?
**RBAC** (Role-Based Access Control): permissions attach to roles, users are assigned roles — simple, coarse-grained, easy to reason about ("admin can do X, viewer cannot"). **ABAC** (Attribute-Based Access Control): permissions are computed from attributes of the user, resource, and context (e.g., "a user can edit a document if `user.department == document.department` AND `time is business hours`") — more flexible, more complex, harder to audit at a glance.

### Interview Questions
- **L1:** "RBAC assigns permissions via roles — simple and auditable, but coarse. ABAC evaluates rules against attributes of the user/resource/context — more flexible for fine-grained or contextual rules, at the cost of complexity."
- **L2:** explain why Hermes's design (per its resume description — "role checks") sounds primarily RBAC-based, but its tenant-boundary enforcement is closer to an ABAC-style rule ("this request's tenant attribute must match this resource's tenant attribute") — real systems often blend both rather than being purely one or the other, and being able to name which parts of your own system are which is a stronger answer than picking one label for the whole system.

---

## Cluster 2: Defense-in-Depth Layering

### What is it?
Authorization checks at multiple independent layers (API gateway, application logic, database/IAM-level) mean a bug in any single layer doesn't fully compromise the system — each layer is a backstop for the others, not a redundant duplicate.

### Interview Questions
- **L1:** "Defense-in-depth means enforcing authorization at multiple independent layers — gateway, application, data-access — so a single bug in one layer isn't a complete compromise."
- **L2:** connect directly to Hermes/AWS: an API Gateway authorizer rejecting invalid JWTs (`08_AWS/API_GATEWAY.md`), application-level tenant/role checks (`08_AWS/COGNITO.md`), and IAM-level conditions scoping DynamoDB access by tenant tag (`08_AWS/AWS_FUNDAMENTALS.md`) are three genuinely independent layers — a bug in the application-level check (e.g., a missed tenant filter in one query) is still caught by the IAM-level condition, if that layer is actually implemented. `NEEDS VERIFICATION`: confirm how many of these layers Hermes actually has before claiming full defense-in-depth as a feature.

### Quick Revision
- RBAC: simple, role-based, coarse. ABAC: flexible, attribute/context-based, more complex.
- Real systems often blend both — know which parts of your own project are which.
- Defense-in-depth: independent layers, each a backstop, not a single point of failure for authorization.
