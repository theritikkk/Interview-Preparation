# FastAPI (Backend Specifics)

Core type-hint/Pydantic-validation model is covered in `01_PROGRAMMING/PYTHON.md` (Cluster 4). This file adds backend-operational patterns.

---

## Cluster 1: Dependency Injection & Testability

### What is it?
FastAPI's `Depends()` injects shared logic (DB sessions, current-user extraction from a JWT, pagination params) into route handlers — and, critically, can be **overridden** in tests to substitute a fake implementation without touching route code.

### How does it work?
```python
def get_db():
    db = SessionLocal()
    try: yield db
    finally: db.close()

@app.get("/tasks")
def list_tasks(db: Session = Depends(get_db)):
    return db.query(Task).all()

# In tests:
app.dependency_overrides[get_db] = lambda: fake_session
```

### Interview Questions
- **L1:** "`Depends()` injects shared logic like DB sessions or auth into route handlers, and can be overridden in tests to substitute fakes — the same dependency-injection-for-testability idea as Spring's DI, in a function-based style."
- **L2:** explain why `yield`-based dependencies (like `get_db` above) are useful specifically for resources needing cleanup — code after the `yield` runs after the response is sent, guaranteeing the session closes regardless of whether the handler raised an exception.

---

## Cluster 2: Background Tasks & OpenAPI

### What is it?
`BackgroundTasks` runs a function after the response is already sent (for fire-and-forget work like sending a notification) — not a substitute for a real task queue/worker system under real load, but fine for lightweight, best-effort post-response work. FastAPI auto-generates OpenAPI/Swagger docs from your type hints and Pydantic models with zero extra annotation work.

### Interview Questions
- **L1:** "`BackgroundTasks` runs lightweight work after the response is sent; for anything that needs retries, persistence, or real scale, you'd reach for a proper queue/worker system instead — like the RabbitMQ-based worker fleet in the Task Scheduler."
- **L2:** if asked to compare: `BackgroundTasks` has no retry, no persistence (a crash loses the task) — a good contrast point to explicitly why the Task Scheduler needed a real message broker instead of this kind of in-process shortcut.

### Quick Revision
- `Depends()` + `dependency_overrides` = FastAPI's DI-for-testability pattern.
- `yield`-based dependencies guarantee cleanup runs after the response.
- `BackgroundTasks` ≠ a task queue — no persistence, no retries; contrast directly with your Task Scheduler's RabbitMQ-based design if asked.
