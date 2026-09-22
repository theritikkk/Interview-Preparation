# JavaScript in the Browser

Execution model (event loop, closures) is language-level and covered in `01_PROGRAMMING/JAVASCRIPT.md`. This file covers browser-specific APIs and security — directly relevant to the internship's frontend.

---

## Cluster 1: Browser Storage

### What is it?
`localStorage` (persists indefinitely, ~5-10MB, synchronous API), `sessionStorage` (cleared when the tab closes, same size/API otherwise), and cookies (small, ~4KB, sent automatically with every matching request — the only one of the three the *server* can set directly via `Set-Cookie` and read on every request).

### Interview Questions
- **L1:** "localStorage persists across sessions, sessionStorage clears per tab, cookies are small and automatically sent with requests — the right choice depends on whether the server needs to see the data on every request."
- **L2:** explain why storing a JWT in `localStorage` is a common but debated practice: it's vulnerable to theft via XSS (any injected script can read it), whereas an `httpOnly` cookie can't be read by JavaScript at all — a real security tradeoff worth naming precisely rather than picking one by default.

---

## Cluster 2: CORS

### What is it?
The browser's Same-Origin Policy blocks a page from JavaScript-reading responses from a different origin (scheme+host+port) unless the server explicitly opts in via CORS headers (`Access-Control-Allow-Origin`, etc.) — this is a *browser*-enforced protection, not a server-side one, and doesn't stop a server from receiving the request; it stops the browser from letting the calling page's JS *read the response*.

### Interview Questions
- **L1:** "CORS is the browser's mechanism for allowing (or blocking) cross-origin requests initiated by JavaScript, controlled by response headers the server sends back."
- **L2:** explain the preflight `OPTIONS` request: for "non-simple" requests (custom headers, methods beyond GET/POST, certain content-types), the browser sends an `OPTIONS` request first to ask permission before sending the real request — directly relevant if your internship's frontend called a Supabase API from a different origin.
- **L3:** explain precisely why CORS is *not* a server-security mechanism against non-browser clients — `curl` or a server-to-server call ignores CORS entirely, since CORS is enforced by the browser, not the server; real authorization must still happen server-side regardless of CORS configuration.

---

## Cluster 3: XSS & CSRF (Frontend Perspective)

### What is it?
XSS (Cross-Site Scripting): an attacker gets their JavaScript to run in your page's context (e.g., via unsanitized user input rendered as HTML) — able to steal tokens from storage, make authenticated requests as the victim, etc. CSRF (Cross-Site Request Forgery): a malicious site tricks a victim's browser into making a request to your site using the victim's *existing* cookies/session, without the attacker ever seeing the response.

### Interview Questions
- **L1:** "XSS injects and runs attacker JS in your page; CSRF tricks a victim's browser into making an unwanted authenticated request using their existing session — different attack shape, different defenses."
- **L2:** explain the standard defenses for each: XSS — escape/sanitize any user-controlled content rendered as HTML (React does this by default for JSX text content, which is a real, concrete security benefit of the framework — but `dangerouslySetInnerHTML` opts back out of it). CSRF — CSRF tokens (a per-session secret the form must echo back) and/or `SameSite` cookie attributes that stop cookies from being sent on cross-site requests in the first place.
- **L3:** explain why React's JSX auto-escaping doesn't protect you if you build HTML strings yourself and use `dangerouslySetInnerHTML` with unsanitized user input — the framework's default protection is bypassed exactly where the name says it is.

### Quick Revision
- CORS is a browser-enforced, server-header-controlled cross-origin *read* permission — not a substitute for server-side auth.
- localStorage: readable by any JS on the page (XSS risk for tokens). httpOnly cookie: not readable by JS at all, but vulnerable to CSRF without additional protection.
- XSS defense: escape/sanitize rendered user content. CSRF defense: tokens + `SameSite` cookies.
