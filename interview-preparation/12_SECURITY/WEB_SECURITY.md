# Web Security

XSS/CSRF/CORS are covered from the frontend angle in `05_FRONTEND/JAVASCRIPT_BROWSER.md`. This file covers server-side injection classes.

---

## Cluster 1: SQL Injection

### What is it?
Untrusted input concatenated directly into a SQL query lets an attacker inject their own SQL logic — the single most classic web vulnerability, and still common in practice.

### How does it work?
```js
// VULNERABLE: string concatenation
const query = `SELECT * FROM users WHERE email = '${userInput}'`;
// userInput = "' OR '1'='1" turns this into a query returning ALL rows

// SAFE: parameterized query — the driver sends the input as DATA, never as SQL syntax
db.query('SELECT * FROM users WHERE email = $1', [userInput]);
```

### Interview Questions
- **L1:** "SQL injection happens when untrusted input is concatenated directly into a query string; parameterized queries (prepared statements) prevent it by sending input as data, never as executable SQL syntax."
- **L2:** explain why an ORM (or a query builder) doesn't *automatically* guarantee safety — raw-query escape hatches (most ORMs have one) reintroduce the exact same risk if untrusted input is concatenated into them directly, so "we use an ORM" isn't itself a complete answer to "how do you prevent SQL injection."

---

## Cluster 2: SSRF (Server-Side Request Forgery)

### What is it?
An attacker tricks your *server* into making a request to a URL of the attacker's choosing — dangerous specifically because the server-side request often has network access the attacker doesn't (internal services, cloud metadata endpoints like AWS's `169.254.169.254`, which can leak IAM credentials if reachable).

### How does it work?
A feature like "fetch a preview image from this URL the user provides" is a classic SSRF vector if the URL isn't validated — an attacker supplies an internal/metadata URL instead of an external image URL, and your server obligingly fetches it.

### Interview Questions
- **L1:** "SSRF tricks a server into making a request to an attacker-chosen URL, exploiting the server's network position/privileges rather than the attacker's own."
- **L2:** explain the standard mitigations: allowlist expected destination domains/protocols rather than blocklisting (blocklists are always incomplete), block requests to private/internal IP ranges and cloud metadata endpoints specifically, and validate/resolve the URL server-side rather than trusting client-supplied hostnames blindly.

---

## Cluster 3: Command Injection

### What is it?
Untrusted input passed into a system shell command lets an attacker inject additional shell syntax/commands — the OS-level sibling of SQL injection.

### How does it work?
```js
// VULNERABLE
exec(`convert ${userFilename} output.png`);   // userFilename = "a.jpg; rm -rf /" is catastrophic

// SAFE: avoid shell interpretation entirely — pass args as an array, not a concatenated string
execFile('convert', [userFilename, 'output.png']);
```

### Interview Questions
- **L1:** "Command injection happens when untrusted input reaches a shell command string; avoid it by never concatenating input into a shell string — use APIs that take arguments as an array, bypassing shell interpretation entirely."

### Quick Revision
- SQLi: parameterized queries, never string-concatenate input into SQL.
- SSRF: allowlist destinations, block internal IPs/metadata endpoints, don't trust client-supplied URLs blindly.
- Command injection: pass arguments as an array (`execFile`), never build a shell command string from input.
