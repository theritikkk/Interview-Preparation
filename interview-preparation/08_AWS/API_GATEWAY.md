# API Gateway

⚠️ Listed as a skill, not explicitly named in the Hermes bullet — but Lambda functions need *some* HTTP entry point, so this is almost certainly present. `NEEDS VERIFICATION`: confirm API Gateway (vs. e.g. a Lambda Function URL) actually fronts Hermes before stating it as fact.

---

## Cluster 1: REST API vs. HTTP API, Throttling, Authorizers

### What is it?
API Gateway fronts Lambda (or other backends) with HTTP routing, and can handle cross-cutting concerns (auth, throttling, request validation, caching) before a request ever reaches your code. AWS offers two flavors: **REST APIs** (more features — request/response transformation, usage plans, API keys) and **HTTP APIs** (newer, simpler, cheaper, lower latency — a deliberate subset for the common case).

### How does it work?
A **Lambda authorizer** (or, more simply, a **Cognito authorizer** wired directly to a User Pool) validates the JWT *before* your Lambda handler even runs — rejecting unauthenticated requests at the gateway layer, so your handler code can assume it's only ever invoked with a valid, already-verified identity.

### Interview Questions
- **L1:** "API Gateway fronts your Lambdas with routing, throttling, and auth handled at the gateway layer — a Cognito or Lambda authorizer can reject invalid requests before your function code even runs."
- **L2:** explain throttling's purpose precisely: a per-client (or per-API-key) rate limit protects both your downstream resources (a Lambda fan-out that could otherwise exhaust a database's connections, per `LAMBDA.md`) and other tenants sharing the same API from one noisy client degrading everyone's experience.
- **L3:** connect to Cognito directly: wiring a Cognito authorizer at the gateway means your Lambda's JWT-verification code (`COGNITO.md`) either doesn't need to re-verify the signature at all (the gateway already did) or does so as defense-in-depth — know which your actual implementation does, since claiming double-verification you don't have is an easy thing to be caught overstating.

### Quick Revision
- HTTP API: simpler/cheaper/lower-latency subset. REST API: more features (transformations, usage plans).
- Authorizers reject bad requests at the gateway, before your Lambda runs.
- Throttling protects both downstream resources and other tenants from one client's traffic spike.
