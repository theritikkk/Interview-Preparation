# 13 — Quant / Low-Level C++ Interview Questions

Format: **Q** → short answer → deep answer → the common wrong answer → code where it helps.

---

## Containers

### Q: `vector<int>` vs `int[]` — which is better?

**Short:** Same performance for access; `vector` is safer and more flexible. Raw arrays win only for fixed-size global data.

**Deep:** `vector` stores its elements in a contiguous heap block, so `v[i]` compiles to the same load as `a[i]` after the pointer is in a register. The differences are: (1) `vector` costs one heap allocation, a raw global array costs nothing; (2) `vector` carries a 24-byte header; (3) a global array is zero-initialized by the loader for free, while `vector<int> v(n)` writes n zeros at runtime; (4) the compiler knows a fixed array's address at compile time and can fold it into addressing modes. In a hot loop the difference is typically 0–10%.

**Common wrong answer:** "vector is slower because it's dynamic." It isn't — the dynamism costs nothing on access, only on growth.

**When raw arrays are genuinely preferable in contests:** global fixed-size storage, avoiding per-test-case allocation, and 2D data where you want `int a[N][M]` contiguity without the vector-of-vectors indirection.

---

### Q: `array<int,N>` vs `vector<int>`?

**Short:** `array` is a fixed-size stack object with zero overhead; `vector` is heap-allocated and resizable.

**Deep:** `std::array` is a struct wrapping `T[N]` — `sizeof(array<int,10>) == 40`, no pointer indirection, lives wherever you declare it (stack, global, inside another struct). Size is part of the type, so it can't be changed or chosen at runtime. It gives you `.size()`, iterators, `==`, and copy semantics that raw arrays lack. Use it whenever N is a compile-time constant and small enough for the stack.

**Common wrong answer:** "array is just a vector with a fixed size." No — the storage location is fundamentally different (no heap allocation at all).

---

### Q: `vector` vs `deque`?

**Short:** `vector` is one contiguous block; `deque` is a sequence of fixed-size chunks with an index array.

**Deep:** `deque` gives O(1) `push_front`, which `vector` can't. The cost: elements are not globally contiguous, so `d[i]` needs two loads (index array, then chunk), traversal is slightly slower, and you can't get a pointer to a contiguous buffer. `deque` also never reallocates existing elements, so **references** stay valid across end insertions (iterators do not). libstdc++ uses 512-byte chunks, so a `deque<int>` with 3 elements still allocates 512 bytes.

**Use `deque` for:** 0-1 BFS, sliding-window min/max, anything needing both ends.

---

### Q: `vector` vs `list`?

See `12_CPP_PERFORMANCE.md` for the full argument. **Short:** `vector` almost always wins because list traversal is pointer chasing with a cache miss per node, while vector's O(n) shift is a single `memmove`. `list` wins when you need iterator/reference stability across insertions or O(1) splice.

---

### Q: `map` vs `unordered_map`?

**Short:** `map` is a red-black tree, O(log n), ordered. `unordered_map` is a hash table, O(1) average, O(n) worst case, unordered.

**Deep:** `map` gives you `lower_bound`, in-order iteration, and a hard worst-case guarantee, at ~40–56 bytes per node and a pointer chase per level (so ~20 cache misses for n = 1e6). `unordered_map` in libstdc++ is a bucket array of pointers to singly-linked node chains — one hash, one modulo (an expensive integer division against the prime bucket count), then a chain walk. It also invalidates **all iterators on rehash** (but not references/pointers to elements).

**In competitive programming:** `unordered_map` with default hashing is **hackable** — an adversary generates keys colliding into one bucket and turns it into O(n²). Use a randomized hash (see `02_STL_CHEATSHEET.md`) or just use `map`.

**Common wrong answer:** "unordered_map is always faster." Not for small n, not with a bad hash, not when ordering matters, and not against a plain array with bounded integer keys.

---

### Q: `set` vs `unordered_set`?

Same tree-vs-hash trade-off. The deciding question is whether you need **ordered operations** (`lower_bound`, `*begin()` as the minimum, in-order traversal). If yes, `set`, no debate.

---

### Q: Why can an array frequency table beat a hash map?

**Short:** One indexed load versus a hash, a division, and one or two cache misses.

**Deep:** `cnt[x]++` on an `int[1e6]` is a single load-add-store at a predictable address that the prefetcher can hide. `mp[x]++` hashes, computes `hash % bucket_count` (an integer division, 20–40 cycles), loads a bucket pointer (likely cache miss), follows a node pointer (another miss), compares the key, then increments. Typical gap: **10–100×**. Whenever integer keys are bounded by ~1e7, the array wins even if it's 99% empty.

---

## Vector internals

### Q: What is `capacity`? What is `size`?

`size` = number of constructed elements. `capacity` = number of elements that fit in the currently allocated block. `capacity >= size` always. The slots between them are **raw uninitialized memory** — reading `v[size]` when `size < capacity` is UB.

---

### Q: Why is `push_back` amortized O(1)?

**Short:** Because `vector` grows geometrically.

**Deep:** On overflow, capacity doubles and all elements are moved. Inserting n elements from empty triggers reallocations at capacities 1, 2, 4, …, and copies `1 + 2 + 4 + … + n/2 < n` elements in total. So n insertions cost O(n) → O(1) amortized. With **constant** growth (+k each time), the total would be O(n²/k) → O(n) amortized per insert. The geometric factor is what makes the amortization work.

**Common wrong answer:** "because reallocation is rare." True but insufficient — you need the *sum* of the copy costs to be linear, which requires geometric growth specifically.

---

### Q: What exactly happens during reallocation?

1. Allocate a new block of `2 × capacity`.
2. Move (or copy) each element into the new block — **move** if the type has a `noexcept` move constructor, otherwise **copy** (for the strong exception guarantee).
3. Destroy the old elements.
4. Free the old block.

**Two consequences:** peak memory is briefly ~3× the data (old block + new block), and **every iterator, pointer and reference into the vector dangles.**

**The `noexcept` detail is a favourite interview question:** if your move constructor isn't marked `noexcept`, `vector` falls back to copying during growth, because it can't roll back a partially-moved range if a move throws.

---

### Q: Are `vector` elements contiguous?

**Yes**, guaranteed by the standard since C++03/11. `&v[0] + i == &v[i]`, and `v.data()` is a valid pointer you can pass to a C API. **Exception: `vector<bool>`**, which is a bit-packed specialization and is not a real container.

---

### Q: What is `sizeof(vector<int>)`? Does it depend on the number of elements?

**24 bytes** on x86-64 libstdc++ (three pointers: begin, end, end-of-capacity). **It does not depend on the element count** — the elements live on the heap. `sizeof` is a compile-time property of the type.

---

### Q: What does `reserve` do? What does `resize` do?

`reserve(k)` ensures `capacity >= k`. It does **not** change `size` and does **not** construct anything. `resize(k)` changes `size` to `k`, value-initializing new elements (zeros for `int`) or destroying the excess.

**Memory hook:** *reserve buys land; resize builds houses.*

`clear()` sets size to 0 but **keeps the capacity**. To actually release memory: `shrink_to_fit()` (non-binding) or `vector<T>().swap(v)` (guaranteed).

---

### Q: What happens to pointers and iterators after reallocation?

All invalidated. Using them is undefined behaviour — often it "works" until it silently doesn't.

```cpp
vector<int> v = {1, 2, 3};
int* p = &v[0];
v.push_back(4);          // may reallocate
*p = 10;                 // UB
```

---

## Iterator invalidation (the table)

| Container | Insert | Erase |
|---|---|---|
| `vector` | all iterators/pointers/refs if reallocating; else those at/after the point | those at/after the erased point |
| `deque` | **all iterators**, but refs/pointers survive end-insertion | at the ends: only the erased; in the middle: all |
| `list` | none | only the erased element |
| `set`/`map` | none | only the erased element |
| `unordered_*` | iterators invalidated **on rehash**; refs/pointers never | only the erased element |

**Why `set`/`map` are iteration-safe:** nodes are stable; inserting or erasing a *different* node doesn't move anything.

```cpp
for (auto it = s.begin(); it != s.end(); ) {
    if (bad(*it)) it = s.erase(it);   // erase returns the next iterator (C++11)
    else ++it;
}
```

---

## Memory model

### Q: Stack vs heap allocation?

**Stack:** bump a pointer, ~1 ns, freed automatically at scope exit, limited size, always cache-hot, LIFO.
**Heap:** `malloc` walks free lists / size classes, ~50–100 ns, needs explicit or RAII-managed release, can fragment, arbitrary lifetime.

**Interview follow-up — "where does `vector<int> v(100)` live?"** The 24-byte `vector` object lives wherever you declared it (stack, if it's a local). The 400 bytes of elements live on the heap. This split is the answer they're looking for.

---

### Q: What is memory alignment? What is padding?

**Alignment:** a type's required address multiple (`int` → 4, `double`/pointer → 8). Hardware loads are fastest (and on some ISAs, only legal) at aligned addresses.
**Padding:** the compiler inserts unused bytes between members to satisfy alignment, and pads the struct's total size up to a multiple of its strictest member's alignment (so arrays of the struct stay aligned).

```cpp
struct A { char c; int i; char d; };   // 1 + 3pad + 4 + 1 + 3pad = 12
struct B { int i; char c; char d; };   // 4 + 1 + 1 + 2pad = 8
```
**Rule: declare largest-first.** At 1e6 objects that's 4 MB saved.

---

### Q: What is false sharing?

Two threads writing to distinct variables that happen to share a 64-byte cache line. The cache-coherence protocol bounces the line between cores on every write, so both threads stall despite no logical dependency. **Fix:** pad to a cache line (`alignas(64)`).

---

### Q: What causes cache misses?

- Random / strided access (hash tables, linked lists, column-major traversal).
- Working set larger than the cache.
- Pointer chasing — the next address isn't known until the current load completes, so it can't be prefetched.
- Too many concurrent streams (the prefetcher tracks a limited number).

---

## Language features

### Q: What is RAII?

**Resource Acquisition Is Initialization**: a resource's lifetime is tied to an object's lifetime. The constructor acquires it, the destructor releases it. Because destructors run deterministically on scope exit — **including during stack unwinding from an exception** — resources can't leak on any path.

`vector`, `string`, `unique_ptr`, `lock_guard`, `fstream` are all RAII. It's why C++ needs no `finally` block.

---

### Q: What is move semantics?

**Short:** transferring ownership of a resource instead of copying it.

**Deep:** a move constructor takes an rvalue reference `T&&`, steals the source's internal pointers, and leaves the source in a valid-but-unspecified (typically empty) state. For `vector`, a move is three pointer assignments — O(1) — versus an O(n) copy. `std::move` is not a move; it's a cast to `T&&` that makes the object *eligible* for move overload resolution.

```cpp
vector<int> a(1'000'000);
vector<int> b = a;              // O(n) copy
vector<int> c = std::move(a);   // O(1); a is now empty
```

**Follow-up — "when does a copy still happen despite `std::move`?"** When the type has no move constructor, when the target is `const`, when the move constructor isn't `noexcept` and the container needs the strong exception guarantee (vector growth), or when you move a small string within SSO (the inline buffer still gets copied — just 16 bytes).

---

### Q: What happens when you pass `vector<int>` by value?

The copy constructor runs: a fresh heap allocation plus an O(n) element copy. For 1e6 ints that's ~4 MB and a few milliseconds, per call.

```cpp
void f(vector<int> v);          // copies — O(n)
void f(const vector<int>& v);   // no copy, read-only  ← default
void f(vector<int>& v);         // no copy, can mutate the caller's data
void f(vector<int>&& v);        // takes ownership of a temporary
```

**Follow-up — "when is by-value correct?"** When the function needs its own copy anyway (it will modify and keep it). Then taking by value lets the caller `std::move` in and pay nothing. This is the "sink parameter" idiom.

---

### Q: `vector<int>&` vs `const vector<int>&`?

`const&` promises not to modify and binds to temporaries and to `const` objects. Non-const `&` is your API saying "I will write to this". Use `const&` by default; use non-const `&` only as a deliberate out-parameter.

---

### Q: What does `inline` actually mean?

**Short:** it's about the **One Definition Rule**, not about inlining.

**Deep:** `inline` tells the linker that multiple identical definitions of this symbol across translation units are allowed and should be merged — which is why you can define a function in a header. It is only a *hint* about actual inlining, and modern compilers ignore the hint entirely: they decide based on size, call count, and optimization level. At `-O2` they inline plenty of functions never marked `inline`, and refuse to inline large ones that are. `-flto` extends this across translation units. C++17 added `inline` variables for the same ODR reason.

**Common wrong answer:** "it forces the compiler to inline the function."

---

### Q: What does `constexpr` mean?

**Short:** "can be evaluated at compile time."

**Deep:** `constexpr` on a variable *requires* compile-time initialization. On a function it means "usable in a constant expression **if** the arguments are constant" — the same function can also run at runtime with non-constant arguments. `consteval` (C++20) forces compile-time only. The practical win is moving work off the hot path: lookup tables, `array` sizes, template arguments.

```cpp
constexpr int fib(int n) { return n < 2 ? n : fib(n-1) + fib(n-2); }
constexpr int x = fib(20);     // computed at compile time
int y = fib(k);                // runtime call
```

---

### Q: What is small string optimization?

`std::string` stores short strings (≤ 15 chars in libstdc++) **inside the string object itself**, skipping heap allocation entirely. `sizeof(string)` is 32: a pointer, a size, and a 16-byte union holding either the inline buffer or the capacity.

**Consequences:** short-string copies are allocation-free; a `map<string,int>` with short keys is far cheaper than you'd expect; *moving* a short string is not free (the buffer is copied) unlike a long one.

---

### Q: Why might recursion be slower than iteration?

Stack frame setup/teardown per call, register spills across the call boundary, no guaranteed tail-call elimination, cache pressure from the frames, and the compiler's reduced ability to keep loop variables in registers. Typical penalty: 2–3×. Recursion can also overflow the stack.

**Counterpoint worth making in the interview:** for tree/graph traversal and DP with subtle dependency order, recursion is dramatically easier to get right, and the compiler often turns simple recursion into a loop anyway.

---

### Q: What is undefined behaviour? Name some.

Behaviour the standard imposes no requirements on — the compiler may assume it never happens and optimize accordingly, which is why UB can cause changes far from the offending line.

- signed integer overflow (unsigned wraps and is **well-defined**)
- out-of-bounds array access
- dereferencing null or a dangling pointer
- reading an uninitialized variable
- shifting by ≥ the bit width, or shifting a negative number left
- modifying a `const` object
- using an invalidated iterator
- a comparator that isn't a strict weak ordering
- data race
- `__builtin_clz(0)` / `__builtin_ctz(0)` (GCC-specific, but same category)

**The contest-relevant point:** `-fsanitize=undefined` catches most of these at runtime, which is why it's worth running once on every mystery WA.

---

### Q: What is virtual dispatch and what does it cost?

A `virtual` call loads the object's vptr, indexes the vtable, and makes an indirect call. Cost: one extra load (usually cached) and an indirect branch the predictor must learn; more importantly, **the call can't be inlined**, which blocks downstream optimization. Each polymorphic object also carries an 8-byte vptr.

In low-latency code people avoid virtuals via templates (CRTP), `std::variant`, or function pointers hoisted out of the hot path.

---

### Q: What's the overhead of `shared_ptr` vs `unique_ptr`?

`unique_ptr` is zero-overhead — a single pointer, `sizeof` = 8, all operations compile away. `shared_ptr` is 16 bytes (pointer + control block pointer), the control block is a separate allocation unless you use `make_shared`, and the refcount is **atomic** — so every copy is an atomic increment, which is expensive and contended across threads. Prefer `unique_ptr`, and pass `shared_ptr` by `const&` when you don't need to extend ownership.

---

### Q: What sorting algorithm is `std::sort`?

**Introsort:** quicksort, switching to heapsort when the recursion depth exceeds ~2·log n (guaranteeing O(n log n) worst case), and to insertion sort for small partitions (~16 elements). It is **not stable** — `stable_sort` is a merge sort that allocates a buffer and is O(n log² n) if allocation fails.

**Why it beats C's `qsort`:** `std::sort` takes the comparator as a template parameter, so it's inlined; `qsort` takes a function pointer and makes an indirect call per comparison. Typical gap: 2–3×.

---

### Q: Why can theoretically O(1) operations have very different real performance?

Because "O(1)" bounds the *number* of operations, not their *cost*. A hash-map lookup is O(1) but may cost 200 cycles (hash + division + two cache misses); an array index is O(1) and costs 4 cycles. The constant hidden by the notation varies by two orders of magnitude depending on memory locality, branch predictability, and allocation. **Asymptotic analysis picks the algorithm; the memory hierarchy decides the wall clock.**

This is the thesis statement of this whole file, and it's the answer they're usually fishing for.