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

---

## Cluster 3: Middleware & Custom Validation

### What is it?
FastAPI (via Starlette) middleware wraps every request/response, useful for cross-cutting concerns (CORS, request logging, timing) that apply globally rather than per-route. Pydantic **validators** let you express validation rules beyond basic type-checking — custom logic that runs as part of the same automatic validation Pydantic already does for types.

### How does it work?
```python
@app.middleware("http")
async def add_process_time(request: Request, call_next):
    start = time.time()
    response = await call_next(request)               # runs the rest of the pipeline
    response.headers["X-Process-Time"] = str(time.time() - start)
    return response

from pydantic import BaseModel, field_validator
class TaskCreate(BaseModel):
    priority: int
    @field_validator('priority')
    @classmethod
    def priority_in_range(cls, v):
        if not (1 <= v <= 5):
            raise ValueError('priority must be between 1 and 5')   # becomes an automatic 422 response
        return v
```

### Interview Questions
- **L1:** "Middleware wraps every request/response for cross-cutting concerns like logging or timing; Pydantic validators extend the automatic type-based validation with custom business rules, and a validation failure automatically becomes a structured 422 response — no manual error handling needed."
- **L2:** explain why middleware is the right place for concerns like request timing/logging but the *wrong* place for business-logic validation (like the priority-range check) — middleware runs for every route indiscriminately, while Pydantic validators are scoped precisely to the model/field they validate, keeping business rules colocated with the data they constrain rather than scattered across a global middleware function.
- **L3:** connect to `04_BACKEND/EXPRESS.md`'s middleware discussion: this is the same architectural pattern (cross-cutting concerns as a wrapping layer around route handlers) implemented via Starlette's ASGI middleware protocol instead of Express's callback-chain — recognizing this as "the same pattern, different framework" is a stronger answer than describing FastAPI middleware as if it were a wholly novel concept.

### Quick Revision (Full File)
- `Depends()` + `dependency_overrides` = FastAPI's DI-for-testability pattern.
- `yield`-based dependencies guarantee cleanup runs after the response.
- `BackgroundTasks` ≠ a task queue — no persistence, no retries; contrast directly with your Task Scheduler's RabbitMQ-based design if asked.
- Middleware: cross-cutting concerns, applies to every route. Pydantic validators: field/model-scoped business rules, colocated with the data they constrain — don't conflate the two roles.