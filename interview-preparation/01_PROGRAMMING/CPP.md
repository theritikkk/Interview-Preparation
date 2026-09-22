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

### Quick Revision
- RAII: resource lifetime tied to object scope — automatic, exception-safe cleanup.
- `unique_ptr`: exclusive ownership. `shared_ptr`: refcounted, can leak via cycles. `weak_ptr`: breaks cycles.
- Virtual functions: runtime dispatch via vtable; always give polymorphic base classes a virtual destructor.
- Templates: compile-time generics, zero runtime overhead, but code bloat from multiple instantiations.
