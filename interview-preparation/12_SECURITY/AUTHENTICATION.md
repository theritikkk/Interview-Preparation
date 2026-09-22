# Authentication

Cognito/JWT specifics are in `08_AWS/COGNITO.md`. This file covers the underlying general concepts.

---

## Cluster 1: Password Hashing

### What is it?
Passwords must never be stored in plaintext or reversibly encrypted — they're hashed with a slow, purpose-built algorithm (bcrypt, argon2, scrypt) that includes a per-password **salt**, so identical passwords produce different hashes and precomputed rainbow-table attacks don't work.

### Interview Questions
- **L1:** "Passwords are hashed with a slow, salted algorithm like bcrypt or argon2 — never stored plaintext, and never hashed with a fast general-purpose hash like SHA-256 alone, which is too fast to resist brute-forcing."
- **L2:** explain *why* speed is a deliberate anti-feature here: bcrypt/argon2 are intentionally slow (tunable work factor) specifically to make brute-forcing every possible password computationally expensive, unlike SHA-256 which is designed to be fast — fast is exactly wrong for password hashing.

---

## Cluster 2: Session-Based vs. Token-Based Auth

### What is it?
**Session-based**: server generates a session ID on login, stores session state server-side (in memory or a shared store like Redis), and gives the client an opaque session-ID cookie — every request needs a server-side lookup. **Token-based (JWT)**: the server issues a signed, self-contained token; validation is local (verify the signature), no server-side lookup or shared session store needed.

### Interview Questions
- **L1:** "Session-based auth requires server-side state and a lookup per request; token-based (JWT) auth is stateless and self-contained, verified locally via signature — the tradeoff is revocation: a session can be invalidated instantly server-side, a JWT is valid until it expires unless you build extra revocation infrastructure."
- **L2:** explain the revocation problem specifically, since it's the most common JWT gotcha in interviews: if a JWT is compromised or a user needs to be logged out immediately (e.g., account compromise), you can't simply "delete" a stateless token the way you'd delete a session — mitigations include short expiry + refresh tokens, or maintaining a server-side revocation/blocklist (which reintroduces some statefulness specifically for the revocation case).

---

## Cluster 3: OAuth 2.0 (Conceptual)

### What is it?
OAuth is an **authorization** delegation protocol (letting an app act on a user's behalf with a third-party service) — often confused with authentication, but they're distinct: OpenID Connect (OIDC) is built on top of OAuth specifically to add an authentication/identity layer. The **Authorization Code flow** (the standard for server-side apps) exchanges a short-lived code (obtained via a redirect through the identity provider) for tokens, server-to-server, never exposing the tokens directly in the browser URL.

### Interview Questions
- **L1:** "OAuth handles delegated authorization (letting an app act on your behalf); OIDC layers identity/authentication on top of it. The Authorization Code flow exchanges a redirect-delivered code for tokens via a server-to-server call, avoiding exposing tokens in the browser."
- **L2:** explain why the Authorization Code flow is preferred over the (largely deprecated) Implicit flow, which returned tokens directly in the URL fragment — directly exposed to the browser history, referrer leaks, and any script running on the page, a real security downgrade compared to a server-side token exchange.

### Quick Revision
- Hash passwords with a slow, salted algorithm (bcrypt/argon2) — never plaintext, never a fast general hash alone.
- Session auth: stateful, instant revocation. Token/JWT auth: stateless, harder revocation, needs a deliberate strategy.
- OAuth = authorization; OIDC = authentication built on OAuth. Authorization Code flow is the standard, secure pattern.
