# Cognito

🔴 Hermes's tenant-isolation and RBAC foundation — likely your single most heavily-probed security topic.

---

## Cluster 1: User Pools & JWT Issuance

### What is it?
A Cognito User Pool is a managed user directory that handles authentication and issues JWTs (ID token, access token, refresh token) on successful login — you don't store or verify passwords yourself.

### How does it work?
```
Client → Cognito (login) → { idToken, accessToken, refreshToken }
Client → API Gateway/your API, Authorization: Bearer <accessToken>
Your service → verifies the JWT signature against Cognito's public keys (JWKS), checks expiry → trusts the claims inside
```

### Interview Questions
- **L1:** "Cognito authenticates users and issues signed JWTs; your API verifies the token's signature and expiry against Cognito's public keys, then trusts the claims inside without needing to call Cognito again per request."
- **L2:** explain *why* JWT verification doesn't require a network call to Cognito on every request: the token is signed with Cognito's private key, and your service verifies it using the corresponding public key (fetched once, cached) — this is what makes JWT-based auth horizontally scalable without a shared session store.

---

## Cluster 2: Claims Extraction, Tenant-Boundary Enforcement, Role Checks

### What is it?
Custom claims (e.g., `tenantId`, `role`) can be embedded in the JWT (via Cognito pre-token-generation triggers, or app client scopes) — your API extracts them after verifying the token, then enforces two separate things: **tenant boundary** (does this request only ever touch data belonging to `tenantId`?) and **RBAC** (does this `role` permit this specific action?).

### Interview Questions
- **L1:** "After verifying the JWT, the service extracts custom claims like tenantId and role, then enforces both a tenant-boundary check (data scoping) and a role check (permission) — two independent authorization dimensions, not one."
- **L2:** explain why tenant-boundary enforcement needs to happen **on every single data access**, not just at the API's entry point — a role check at the top of a request doesn't prevent a downstream query (e.g., a poorly parameterized DynamoDB query) from accidentally fetching another tenant's data; the tenant ID from the verified claim needs to flow all the way down into the actual data-access layer's query parameters.
- **L3 — 🔴 the single most likely deep-dive question here:** be ready to walk through, precisely, *where in your code* the tenant ID from the JWT claim gets attached to a DynamoDB query (e.g., as part of the partition key, per `08_AWS/DYNAMODB.md`) — this is exactly the kind of "read your code for 30 minutes" question your resume invites. `NEEDS VERIFICATION`: confirm this mechanism against your actual implementation; don't describe a plausible-sounding mechanism you haven't confirmed.

### Follow-up Chain
> **Q: How do you prevent tenant A from accessing tenant B's data?**
> A: The JWT's tenantId claim is extracted after verification, and every data-access call is scoped by that tenantId.
> **Follow-up: What stops a malicious client from just sending a different tenantId in the request body to impersonate another tenant?**
> A: The tenantId used for authorization must come from the **verified JWT claim**, never from client-supplied request data (body/params/headers) — if your code ever reads tenantId from the request body instead of the token, that's exactly the vulnerability being probed for here.
> **Trap: "We validate the tenantId matches on the frontend before sending the request."**
> Correct response: frontend validation is not a security boundary at all — it's trivially bypassed by anyone calling your API directly. All authorization must be enforced server-side, off data extracted from the verified token, full stop.

### Quick Revision
- JWT verification is local (signature + expiry check against cached public keys) — no per-request call back to Cognito needed.
- Tenant ID and role are two independent authorization checks: data scoping vs. permission.
- Authorization must always derive from the verified JWT claim, never from client-supplied request data.
- Tenant scoping must reach the actual data-access layer, not just gate the API entry point.
