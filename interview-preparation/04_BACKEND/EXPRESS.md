# Express.js

⚠️ Listed as a skill, no project bullet names it explicitly — your Node services (Hermes, Task Scheduler) very plausibly use it for their API layers, but confirm before claiming specifics (`NEEDS VERIFICATION`).

---

## Cluster 1: Middleware Chain & Routing

### What is it?
Express handles a request by passing it through an ordered chain of middleware functions, each able to inspect/modify the request, respond directly, or call `next()` to pass control forward. Routing maps an HTTP method + path pattern to a specific handler (itself just the last middleware in a chain).

### How does it work?
```js
app.use(express.json());                         // parse JSON bodies — applies to all routes below it
app.use(authenticate);                            // custom middleware — must call next() to continue

app.get('/tasks/:id', validateId, (req, res) => {
    res.json({ id: req.params.id });
});

app.use((err, req, res, next) => {                // error-handling middleware: FOUR params is what marks it as such
    res.status(err.status || 500).json({ error: err.message });
});
```

### Interview Questions
- **L1:** "Express processes a request through an ordered chain of middleware functions; each one can act and call `next()` to continue the chain, or respond and end it."
- **L2:** explain the single most common Express bug: forgetting to call `next()` (or to send a response) inside a middleware — the request simply hangs with no response and no error, until the client times out.
- **L3:** explain why Express recognizes error-handling middleware specifically by its **arity** (exactly 4 parameters: `err, req, res, next`) rather than by name or position alone, and why that means a mistakenly-3-parameter "error handler" silently gets treated as regular middleware instead.

### DO NOT SAY THIS
"Middleware order in Express doesn't matter." — It matters enormously: `express.json()` must run before any handler that reads `req.body`; auth middleware must run before the routes it protects; error-handling middleware must be registered *last*, after all other routes/middleware, to actually catch their errors.

---

## Cluster 2: Validation, Rate Limiting, Error Handling as Middleware

### What is it?
Both request validation and rate limiting are naturally implemented as middleware — a function that runs before your route handler and either lets the request through or short-circuits it.

### Interview Questions
- **L1:** "Validation, auth, and rate limiting are all naturally expressed as middleware that runs before the actual route handler, keeping cross-cutting concerns out of business logic."
- **L2:** explain why centralizing error handling in one final error-middleware (rather than try/catch scattered in every handler) keeps error-response formatting consistent — and the corresponding gotcha: errors thrown inside an `async` route handler aren't automatically caught by Express's default error handling (pre-Express 5) unless you either wrap them (`try/catch` + `next(err)`) or use a helper that does so for you.

### Coding/Debugging Exercise
Given an async Express route handler that throws inside an `await`ed call with no `try/catch`, explain why the request hangs instead of returning a 500, and fix it.

### Quick Revision
- Middleware order matters: parsing → auth → validation → route handler → error handler (last).
- Error middleware is identified by exactly 4 parameters.
- Async route handlers need explicit error forwarding (`try/catch` + `next(err)`) in Express 4.

---

## Cluster 3: File Uploads

### What is it?
File uploads typically arrive as `multipart/form-data`, which needs a dedicated parser (e.g., `multer`) rather than `express.json()` — the request body isn't JSON, it's a multi-part binary encoding of one or more files plus form fields.

### How does it work?
```js
const upload = multer({ storage: multer.memoryStorage(), limits: { fileSize: 5 * 1024 * 1024 } });
app.post('/upload', upload.single('file'), (req, res) => {
    // req.file has the uploaded file's buffer/metadata; req.body has other form fields
    res.json({ filename: req.file.originalname, size: req.file.size });
});
```

### Interview Questions
- **L1:** "File uploads arrive as multipart/form-data and need a dedicated parser (like multer) rather than the standard JSON body parser; always set a file-size limit to avoid a single request exhausting server memory."
- **L2:** explain why routing the upload *directly to storage* (a pre-signed S3-style URL, per `11_SYSTEM_DESIGN/COMMON_DESIGNS.md`'s file-upload design) is usually preferred over uploading through your own API server for anything beyond small files: your server becomes a bandwidth bottleneck and memory-pressure point for every uploaded byte if it's the middleman, versus the client uploading directly to storage and your server only issuing a short-lived, scoped upload URL.
- **L3:** explain the security implications of accepting file uploads at all: validate file type by actually inspecting content (magic bytes), not just the client-supplied filename extension or `Content-Type` header (both are trivially spoofable) — and never execute or directly serve an uploaded file from a path an attacker could predict or control, to avoid it being used as a vector for stored XSS or, in the worst case, remote code execution if the upload path is web-servable and the server can be tricked into executing it.

### Quick Revision (Full File)
- Middleware order matters: parsing → auth → validation → route handler → error handler (last).
- Error middleware is identified by exactly 4 parameters.
- Async route handlers need explicit error forwarding (`try/catch` + `next(err)`) in Express 4.
- File uploads need a dedicated multipart parser, a file-size limit, and content-based (not extension-based) type validation. Prefer direct-to-storage uploads over routing large files through your own API server.