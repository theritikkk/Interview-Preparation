# C++

⚠️ **Exposure note:** C++ is on your skills list but no project bullet uses it — NetSight is C, not C++. Prepare this at solid-fundamentals depth. If asked "tell me about a C++ project," the honest answer is that your systems-programming project (NetSight) is in C, and you can pivot to discussing why C was chosen there (see `C.md` and `13_PROJECT_DEEP_DIVES/NETSIGHT.md`) rather than implying a C++ project that doesn't exist.

---

## Cluster 1: RAII & Smart Pointers

### What is it?
RAII (Resource Acquisition Is Initialization): tie a resource's lifetime to an object's scope, so the destructor automatically releases it — no manual `free`/`close`/`unlock` calls to forget. Smart pointers (`unique_ptr`, `shared_ptr`, `weak_ptr`) are RAII applied to heap memory.

### Why does it exist?
C's manual `malloc`/`free` model is error-prone (leaks, double-frees, use-after-free). RAII makes correct cleanup automatic and exception-safe.

### How does it work?
```cpp
std::unique_ptr<int> p = std::make_unique<int>(42); // sole owner, freed automatically at scope exit
std::shared_ptr<int> s = std::make_shared<int>(42);  // refcounted, freed when last owner goes out of scope
std::weak_ptr<int> w = s;                             // non-owning observer, doesn't keep object alive
```

### Interview Questions
- **L1:** "RAII ties resource cleanup to object destruction, so it happens automatically and even during exception unwinding."
- **L2:** explain `unique_ptr` vs `shared_ptr` (exclusive vs. shared ownership, move-only vs. copyable) and why `shared_ptr` has overhead (atomic refcount).
- **L3:** explain why `shared_ptr` cycles leak (A holds a `shared_ptr` to B, B holds one back to A — refcount never hits zero) and how `weak_ptr` breaks the cycle.

### DO NOT SAY THIS
"`shared_ptr` prevents all memory leaks." — It prevents *forgetting to free*, not reference cycles. This is a very common overclaim.

### Comparison With Alternatives
Raw pointers + manual `delete` (C-style, what your NetSight work uses in spirit): full control, full responsibility. Smart pointers: automatic, small overhead, much safer default choice in application code — but often deliberately avoided in latency-sensitive systems code for predictability, which is a legitimate reason C was chosen for NetSight over C++.

---

## Cluster 2: OOP Mechanics — Virtual Functions, Vtables, Inheritance, Polymorphism

### What is it?
A virtual function lets a base-class pointer call the *derived* class's overridden implementation at runtime — resolved via a **vtable** (a per-class array of function pointers) rather than at compile time.

### How does it work?
```cpp
struct Shape { virtual double area() const = 0; virtual ~Shape() = default; };
struct Circle : Shape { double r; double area() const override { return 3.14159 * r * r; } };

Shape* s = new Circle{5};
s->area();  // vtable lookup picks Circle::area at runtime
delete s;   // virtual destructor is essential here — without it, ~Circle never runs
```

### Interview Questions
- **L1:** "Virtual functions enable runtime polymorphism — the actual function called depends on the object's real type, not the pointer's declared type."
- **L2:** explain the vtable mechanism: each polymorphic object carries a hidden pointer to its class's vtable; the call becomes an indirect jump through that table.
- **L3:** explain the classic bug: deleting a derived object through a base pointer *without* a virtual destructor causes undefined behavior (only the base destructor runs) — and why `override` and `= default` on destructors matter.

### Common Mistakes
- Forgetting `virtual` on the base destructor when the class is meant to be polymorphically deleted
- Assuming virtual dispatch is free (it's an indirect call — real, if small, overhead, relevant in a systems-programming interview context)

---

## Cluster 3: STL & Templates

### What is it?
The STL (Standard Template Library) gives you generic containers (`vector`, `map`, `unordered_map`) and algorithms; templates are C++'s compile-time generics mechanism — a template is instantiated (compiled) separately for each concrete type it's used with.

### Interview Questions
- **L1:** "Templates generate type-specific code at compile time — zero runtime overhead vs. a hand-written version for that type, unlike Java generics which use type erasure."
- **L2:** `vector` (contiguous, amortized O(1) push_back, cache-friendly) vs. `map` (ordered, red-black tree, O(log n)) vs. `unordered_map` (hash table, average O(1)) — pick correctly for a stated access pattern.
- **L3:** explain iterator invalidation: inserting into a `vector` can reallocate and invalidate all existing iterators/pointers/references — a classic source of subtle bugs.

### Coding/Debugging Exercise
Given code that erases elements from a `vector` while iterating over it with a range-based for loop, explain why it's undefined behavior and fix it using the erase-remove idiom or iterator-returned-by-erase.

## Cluster 4: References vs. Pointers, Iterators

### What is it?
A **reference** is an alias for an existing variable — it must be bound at declaration, can never be null, and can never be reseated to refer to something else afterward. A **pointer** is a variable holding an address — it can be null, reassigned, and supports pointer arithmetic. An **iterator** is a generalized "position" abstraction over a container (like a smart pointer into a sequence), letting algorithms work uniformly across `vector`, `list`, `map`, etc. without knowing the container's internal layout.

### How does it work?
```cpp
int x = 5;
int& ref = x;      // must bind now; ref IS x, forever — no null, no rebinding
ref = 10;           // x is now 10

int* ptr = &x;      // can be null, can be reassigned to point elsewhere
ptr = nullptr;       // valid — ref could never do this

std::vector<int> v = {1, 2, 3};
for (auto it = v.begin(); it != v.end(); ++it) {   // iterator — works identically for list, set, map
    std::cout << *it;
}
```

### Interview Questions
- **L1:** "A reference is a non-null, non-reseatable alias bound at declaration; a pointer is a reassignable, nullable address holding variable — references are generally preferred when null/reassignment isn't needed, since they're safer by construction."
- **L2:** explain *why* you'd still choose a pointer over a reference in specific cases: representing "optionally points to nothing" (a null pointer, pre-`std::optional`), or needing to rebind to a different object later (a reference can't do either) — the choice should be deliberate, not just "references are always better."
- **L3:** explain iterator invalidation precisely (echoing the `vector` erase example elsewhere in this file): different containers have different invalidation rules — `vector` insertion can invalidate *all* iterators/pointers/references if it reallocates; `list`/`map` insertion invalidates none except iterators to erased elements — knowing this container-specific behavior, not just "iterators can be invalidated" vaguely, is what separates a strong answer.

---

## Cluster 5: RAII-Style Locking — `std::mutex`, `lock_guard`, `unique_lock`

### What is it?
C++ applies the same RAII principle from Cluster 1 to locking: `std::lock_guard` acquires a mutex on construction and releases it on destruction (scope exit) — so a lock is never accidentally left held, even if an exception is thrown mid-scope. `std::unique_lock` is the more flexible sibling — supports deferred locking, manual unlock/relock, and is required by `std::condition_variable::wait()`.

### How does it work?
```cpp
std::mutex mtx;
void increment(int& counter) {
    std::lock_guard<std::mutex> lock(mtx);   // acquired here
    counter++;
}                                              // released here automatically, even on exception — no manual unlock()

void withCondVar(std::condition_variable& cv, std::mutex& mtx, bool& ready) {
    std::unique_lock<std::mutex> lock(mtx);   // unique_lock — required here, lock_guard can't do this
    cv.wait(lock, [&]{ return ready; });       // releases the lock while waiting, reacquires on wake
}
```

### Interview Questions
- **L1:** "`lock_guard` acquires a mutex on construction and releases it on destruction — RAII applied to locking, guaranteeing release even on an exception. `unique_lock` adds flexibility (deferred lock, manual unlock, condition-variable compatibility) at a small extra overhead."
- **L2:** explain why manual `mtx.lock()` / `mtx.unlock()` (without RAII) is a real, common bug source: any early return, exception, or `break` between lock and unlock leaves the mutex held forever, deadlocking every other thread waiting on it — `lock_guard`/`unique_lock` eliminate this entire bug class structurally, the same argument as RAII for memory (Cluster 1), applied to locks instead.
- **L3:** connect directly to your own systems background: this is the traditional, lock-based alternative to NetSight's lock-free approach (`01_PROGRAMMING/C.md` Cluster 2) — `lock_guard`/`unique_lock` are the *correct*, idiomatic way to use mutexes in C++ when you do need them, versus C's more manual `pthread_mutex_lock`/`unlock` pairing, which carries the exact "forgot to unlock on an early return" risk that RAII locking exists to eliminate.

### Quick Revision
- RAII: resource lifetime tied to object scope — automatic, exception-safe cleanup.
- `unique_ptr`: exclusive ownership. `shared_ptr`: refcounted, can leak via cycles. `weak_ptr`: breaks cycles.
- Virtual functions: runtime dispatch via vtable; always give polymorphic base classes a virtual destructor.
- Templates: compile-time generics, zero runtime overhead, but code bloat from multiple instantiations.
- References: non-null, non-reseatable, bound once. Pointers: nullable, reassignable. Iterator invalidation rules are container-specific — know `vector`'s (can invalidate everything) vs `list`/`map`'s (narrower) behavior.
- `lock_guard`/`unique_lock`: RAII for mutexes — eliminates the "forgot to unlock" bug class the same way smart pointers eliminate "forgot to free."