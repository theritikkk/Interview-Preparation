# Python

Resume connection: Hermes (9 pytest tests — smallest share of the test suite, so scope is likely narrow, `NEEDS VERIFICATION` on which component) and listed as a Distributed Task Scheduler language alongside Node.js (division of responsibility unstated).

---

## Cluster 1: Object Model, Mutability, References

### What is it?
Every value in Python is an object; variables are references (names bound to objects), not boxes holding values. Mutability is a property of the *object's type*, not the variable.

### How does it work?
```python
a = [1, 2, 3]
b = a          # b references the SAME list object
b.append(4)
print(a)       # [1, 2, 3, 4] — a changed too, because they're the same object

def f(lst):
    lst.append(99)   # mutates the caller's list — visible outside
def g(lst):
    lst = lst + [99]  # rebinds the local name to a NEW list — caller's list unaffected
```

### Interview Questions
- **L1:** "Python passes references to objects; whether a function's changes are visible to the caller depends on whether the object is mutable and whether the function mutates it in place or rebinds the local name."
- **L2:** explain the classic mutable-default-argument bug:
```python
def append_item(item, lst=[]):   # BUG: default list is created ONCE, shared across calls
    lst.append(item)
    return lst
```
- **L3:** explain CPython's reference counting + generational garbage collector (for cycles that refcounting alone can't free) and why `del` doesn't necessarily free memory immediately (other references may exist).

### DO NOT SAY THIS
"Python is pass-by-reference." — It's more precisely "pass object references by value" — the reference itself is copied, but it points to the same object. This distinction is exactly what the mutability example above tests.

---

## Cluster 2: Generators, Decorators, Context Managers

### What is it?
Generators (`yield`) produce values lazily, one at a time, without materializing a full sequence in memory. Decorators wrap a function to extend its behavior without modifying its source. Context managers (`with`) guarantee setup/teardown around a block, even on exception.

### How does it work?
```python
def read_large_file(path):
    with open(path) as f:        # context manager: file guaranteed closed, even on exception
        for line in f:
            yield line.strip()   # generator: one line in memory at a time

def timed(fn):
    def wrapper(*args, **kwargs):
        start = time.time()
        result = fn(*args, **kwargs)
        print(f"{fn.__name__} took {time.time() - start:.3f}s")
        return result
    return wrapper

@timed
def slow_task(): ...
```

### Interview Questions
- **L1:** "Generators yield values lazily for memory efficiency; decorators wrap functions to add cross-cutting behavior; context managers guarantee cleanup via `__enter__`/`__exit__`."
- **L2:** explain why a generator can only be iterated once, and what `yield from` does (delegates to a sub-generator).
- **L3:** implement a context manager manually with `__enter__`/`__exit__` and explain what `__exit__`'s return value controls (whether it suppresses a raised exception).

---

## Cluster 3: Concurrency — async, Threading, Multiprocessing, the GIL

### What is it?
The GIL (Global Interpreter Lock) means only one thread executes Python bytecode at a time in CPython, regardless of core count. This makes threads good for I/O-bound work (the GIL releases during I/O waits) but useless for CPU-bound parallelism — for that, use multiprocessing (separate processes, separate GILs) or `asyncio` (single-threaded cooperative concurrency for I/O).

### How does it work?
```python
async def fetch_all(urls):
    async with aiohttp.ClientSession() as session:
        tasks = [fetch(session, url) for url in urls]
        return await asyncio.gather(*tasks)   # concurrent I/O, single thread, no GIL contention
```

### Interview Questions
- **L1:** "The GIL means CPython threads don't give true CPU parallelism; use `asyncio` or threads for I/O-bound concurrency, and `multiprocessing` for CPU-bound parallelism."
- **L2:** explain *why* the GIL exists (simplifies CPython's memory management — refcounting isn't thread-safe without it) and its real cost (limits multi-core scaling for CPU-bound pure-Python code).
- **L3:** explain that C extensions (NumPy, etc.) can release the GIL during their internal computation, which is how CPU-bound *library* code can still parallelize across threads in Python despite the GIL.

### DO NOT SAY THIS
"Python can't do concurrency because of the GIL." — It can (I/O-bound concurrency via `asyncio`/threads is very effective); the GIL specifically limits *CPU-bound multi-threaded parallelism*, not concurrency in general.

---

## Cluster 4: FastAPI & Pydantic

⚠️ Not tied to a named project bullet in your resume — prepare at solid-fundamentals depth.

### What is it?
FastAPI is an async-first Python web framework using type hints for automatic request validation and OpenAPI docs generation; Pydantic is the validation library underneath it, turning type-annotated classes into runtime-validated data models.

### How does it work?
```python
from fastapi import FastAPI
from pydantic import BaseModel

class TaskCreate(BaseModel):
    name: str
    priority: int = 1

app = FastAPI()

@app.post("/tasks")
async def create_task(task: TaskCreate):   # Pydantic validates the request body automatically
    return {"id": 1, **task.model_dump()}
```

### Interview Questions
- **L1:** "FastAPI uses Python type hints to auto-validate requests (via Pydantic) and auto-generate OpenAPI docs, with native async support."
- **L2:** explain dependency injection in FastAPI (`Depends()`) for things like DB sessions or auth — reused across endpoints without repeating setup code.
- **L3:** explain why `async def` endpoints matter specifically for I/O-bound handlers (don't block the event loop) but using a blocking call inside one (e.g., a synchronous DB driver) defeats the purpose entirely — a real, common production bug.

### Quick Revision
- Python passes object references by value — mutability of the object determines visible side effects.
- Mutable default arguments are created once — classic bug.
- GIL: one thread executes bytecode at a time — fine for I/O concurrency, not CPU parallelism.
- FastAPI + Pydantic: type hints → automatic validation + docs.
