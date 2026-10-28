# ION Group — Software Developer Interview · Master Revision Guide

**For:** Ritik Raj · B.Tech IT, MSIT (2023–2027, CGPA 8.6/10) · **Round:** ~60-minute technical interview on **29 Oct 2026** + a separate **case-study** round [EMAIL].

Built from three sources: (1) your official email instructions, (2) six past ION candidate reports, (3) your real resume and projects. Past-candidate questions are **signals, not guarantees** — always tagged.

### Evidence legend

| Tag | Meaning |
|---|---|
| **[EMAIL]** | Stated in your official scheduling email — treat as confirmed |
| **[CAND]** | Reported by a past ION candidate — strong signal, **not** a guarantee |
| **[STD]** | Standard SDE interview material |
| **[ADVICE]** | My recommendation as your mentor |

### What the candidate reports actually tell us [CAND, aggregated]

- **OA has included:** string compression `aaabbccccd → a3b2c4d`; a job-portal design (`postJob`, `postUser`, `fetchJobs(userId, k)`); graph connected-components counting; MCQs on CS fundamentals + aptitude.
- **Technical round (~60–90 min):** intro → puzzles → **OOP live-coding** (must model *real* entities, e.g. Vehicle/Car, not Class A/B) → **LLD** (traffic light, snake & ladder, leaderboard, phone directory) → 1–2 easy/medium DSA problems **with repeated modifications** → resume project deep-dive.
- **Case-study round (separate [EMAIL]):** a PDF with company data; SWOT; growth strategy; city selection; cost estimation.
- **HR rounds:** Pune head → country head → (sometimes) global head. Short. Flexibility, clubs, internships, location.
- **Why candidates failed [CAND]:** buggy live OOP code (manually called the base destructor, confused overloading with overriding, panicked on compiler errors), didn't know the trie answer for a phone directory, unstructured case answers.

> **The single most important insight:** ION's technical round is *not* a hard-DSA round. It is a **fundamentals + applied-OOP + LLD + puzzles + projects** round with one or two easy/medium coding problems. This guide is weighted accordingly.

### Section index

| § | Section | Priority |
|---|---|---|
| A | Priority & study plans | read first |
| B | C++ & OOP (incl. the confirmed live-coding drill) | P1 |
| C | DSA — the reported problems, in priority order | P1 |
| D | Low-level design (traffic light, snake & ladder, directory, leaderboard, job portal, parking lot) | P1 |
| E | Project deep dives (Cron, WebChat, Hermes, NetSight, internship) | P1 |
| F | Core CS fundamentals | P2 |
| G | Puzzles & reasoning | P1 |
| H | Business case study + AI-tool discipline | P1 |
| I | HR & introductions | P2 |
| J | Rapid-revision cheat sheets | P1 (day-of) |
| K | **Self-drill exercise set (questions only — attempt before reading L)** | P1 |
| L | **Worked solutions & explanations for every K exercise** | read after attempting |

---

## A. Priority and study plan

### A.1 Evidence-weighted priority list

Your stated order was: C++/OOP → DSA → LLD → projects → puzzles → core CS → case study → HR. The candidate reports force two adjustments: **puzzles and the case study move up** (puzzles appeared in ≥3 of 6 reports; the case study is a separate eliminatory round [EMAIL]).

| Priority | Topic | Why |
|---|---|---|
| **P1** | Live OOP coding in C++ — the Vehicle/Car drill (§B1) | Reported verbatim in two experiences [CAND]; consumes ~20–25 min of the round |
| **P1** | LLD: traffic light, snake & ladder, leaderboard, phone directory, job portal (§D) | Reported repeatedly [CAND] |
| **P1** | The ~15 reported DSA problems, all easy/medium (§C) | String compression + job portal were the *actual* OA [CAND]; second-largest, Kadane, max-product-3, linked list, connected components all reported |
| **P1** | Puzzles: hourglass, bike, mislabeled jars, coins, bananas (§G) | Reported in ≥3 experiences [CAND]; graded on **approach**, not the answer |
| **P1** | Project defense + 30s/2min intros (§E) | "Detailed inquiries based on resume" [CAND] — this is where you're strongest; also HR asks projects |
| **P1** | Case-study framework + AI-tool discipline (§H) | Separate round [EMAIL]; SWOT + cab-city scenarios reported [CAND] |
| **P2** | Core CS rapid-fire: OOP theory, DBMS, OS, networks, SDLC (§F) | Asked in light rapid-fire [CAND] |
| **P2** | C++ trivia: `unordered_map` vs `map`, priority-queue complexity, arrays vs linked lists | Asked verbatim [CAND] |
| **P2** | HR: 60s intro, Why ION, flexibility (§I) | 5–60 min rounds [CAND] |
| **P3** | Hard DSA (DP beyond Kadane, Dijkstra, suffix structures), deep OS internals, system design at scale, WebRTC/gRPC internals, Terraform deep-dive | Low reported frequency — skip under time pressure |

### A.2 The 4-hour plan (today, 28 Oct) [ADVICE]

Work in 30-minute blocks. For every block: **type the code, then say the explanation out loud**. Speaking is the skill being graded.

| # | Time | Block | Deliverable |
|---|---|---|---|
| 1 | 30m | **§B1 OOP drill** — write the Vehicle/Car model answer from memory **twice**; once with narration. Memorize the destructor rule + `override` keyword. | Zero-error code, 5-line narration script |
| 2 | 30m | **§C**: second-largest (all 4 variants incl. updates), Kadane, max-product-of-3 — code + spoken approach | 3 snippets you can retype blind |
| 3 | 30m | **§C**: string compression, valid parentheses, two sum, binary search | 4 snippets |
| 4 | 30m | **§C**: linked list (implement + reverse + detect cycle) + connected components | 4 snippets |
| 5 | 30m | **§D**: traffic light — speak the 10-bullet design aloud twice; then snake & ladder code | Spoken design + code |
| 6 | 30m | **§D**: trie phone directory + leaderboard + job portal design | 3 designs |
| 7 | 30m | **§E**: rehearse 30s + 2min intros for Cron, WebChat, Hermes out loud; read follow-up lists | Spoken intros ×2 reps |
| 8 | 30m | **§G puzzles** (derive hourglass + bike once on paper) + **§H** case framework skim + SWOT skeleton | Framework memorized |

Total ≈ 4 hours. If you only have ~4.5h, add: §F rapid-fire read-through (20m) + §I 60s intro rehearsal (10m).

### A.3 The 2-hour emergency plan (if you're starting late) [ADVICE]

| # | Time | Do this |
|---|---|---|
| 1 | 25m | §B1 drill **once** + memorize the three one-liners: overload vs override, virtual destructor, "never call the base destructor" |
| 2 | 25m | Code once: second-largest (O(n) two-variable + set variant), Kadane, max-product-of-3, string compression |
| 3 | 25m | Code once: linked-list reverse + detect cycle; trie insert/search/prefix for phone directory |
| 4 | 15m | Traffic light: speak the 10-bullet design (§D1) twice |
| 5 | 10m | Derive hourglass + bike puzzle on paper (§G) |
| 6 | 10m | Say the 30s project intros + 60s HR intro out loud |
| 7 | 10m | Memorize the 7-step case framework + SWOT skeleton (§H) |

### A.4 Final 30-minute checklist (tomorrow, before the call) [ADVICE]

- [ ] Say the 60-second intro aloud once (§I).
- [ ] Recite: 4 pillars with one-line Vehicle/Car examples each.
- [ ] Recite: overload vs override (one line each); virtual destructor why; why NOT to call the base destructor manually.
- [ ] Recite complexities: `map`/`set` O(log n), `unordered_map` avg O(1)/worst O(n), `priority_queue` push/pop O(log n), sort O(n log n), binary search O(log n).
- [ ] Recite traffic-light 10 bullets (§D1).
- [ ] Recite hourglass solution steps (§G1).
- [ ] Recite the 7-step case framework (§H1).
- [ ] Prepare 2 questions to ask the interviewer (§I.13).
- [ ] Setup: laptop charged, editor open on an empty `.cpp`, compiler works (`g++ -std=c++17 x.cpp && ./a.out`), **AI tool installed for the case round** [EMAIL], water, headphones tested.

### A.5 Skip list if time runs out [ADVICE]

Skip (in this order): move semantics/rvalue deep-dive · templates & SFINAE · DP except Kadane · Dijkstra/Bellman-Ford · normalization proofs beyond 3NF definitions · HTTP/2–3 details · Kubernetes internals beyond your WebChat story · gRPC vs REST deep comparison · suffix trees · probability puzzles beyond §G · mock-system-design (URL shortener scale-out) — none of it is reported in ION rounds [CAND].

---

## B. C++ and OOP — teach-from-first-principles

### B1. THE confirmed live-coding drill [CAND] — do this first, twice

**What was actually asked [CAND, Nov 2025 report]:** "Rate yourself on C++." → "What is OOP and why is it a better paradigm?" → four pillars → polymorphism types → **"Open an editor and implement classes showing both compile-time and runtime polymorphism. Don't model random Class A/Class B — model real-world entities."** → candidate modeled Vehicle/Car → then: "Write destructors for both classes" → trap: candidate manually called the parent destructor. Total: 20–25 min.

**Your model answer — type this from memory until it compiles clean first try (≈10 min):**

```cpp
#include <bits/stdc++.h>
using namespace std;

class Vehicle {                                   // abstract-ish base: models a rental fleet vehicle
public:
    explicit Vehicle(string regNo) : regNo_(move(regNo)) {}
    virtual ~Vehicle() = default;                 // (1) virtual: safe polymorphic delete

    const string& regNo() const { return regNo_; }              // (2) encapsulation

    virtual double farePerKm() const { return 20.0; }           // (3) runtime polymorphism
    void describe() const {                                     // shared, non-virtual behaviour
        cout << regNo_ << " @ Rs." << farePerKm() << "/km\n";
    }
private:
    string regNo_;                                              // hidden state
};

class Car : public Vehicle {
public:
    Car(string regNo, int seats) : Vehicle(move(regNo)), seats_(seats) {}   // (4) init list
    double farePerKm() const override { return 30.0 + 5.0 * seats_; }       // (5) OVERRIDING
    double farePerKm(double surge) const { return farePerKm() * surge; }    // (6) OVERLOADING
private:
    int seats_;
};

class Truck : public Vehicle {
public:
    explicit Truck(string regNo) : Vehicle(move(regNo)) {}
    double farePerKm() const override { return 45.0; }
};

int main() {
    Car c("DL-01-AB-1234", 4);
    c.describe();                                 // compile-time resolution (static binding)

    vector<unique_ptr<Vehicle>> fleet;            // RAII: no manual delete, no leaks
    fleet.push_back(make_unique<Car>("DL-02-CD-5678", 5));
    fleet.push_back(make_unique<Truck>("DL-03-EF-9012"));
    for (const auto& v : fleet) v->describe();    // runtime polymorphism via vtable

    cout << c.farePerKm() << " | " << c.farePerKm(1.5) << "\n";   // overload pair: 50 | 75
    return 0;
}
```

Expected output: `DL-01-AB-1234 @ Rs.50/km`, `DL-02-CD-5678 @ Rs.55/km`, `DL-03-EF-9012 @ Rs.45/km`, `50 | 75`.

**Your 5-line narration script (say this *while* typing):**

1. "Vehicle is my abstraction — it owns the registration number as **private** state (encapsulation), exposes `describe()` as shared behaviour, and declares `farePerKm()` **virtual** so each vehicle type prices itself."
2. "Car **inherits** Vehicle, initializes the base class through the member-initialization list, and **overrides** `farePerKm` — same signature; `override` makes the compiler verify it."
3. "Car also **overloads** `farePerKm` with a surge parameter — same name, different signature, resolved at compile time."
4. "In `main`, calling `describe()` on a concrete object is **static binding**; iterating the fleet through base-class pointers is **dynamic dispatch** via the vtable — runtime polymorphism."
5. "I use `unique_ptr<Vehicle>` with a **virtual destructor**, so deleting through the base pointer destroys the real derived object first, then automatically chains the base destructor."

**The four reported failure points [CAND] — and your defenses:**

| Reported failure | Your defense |
|---|---|
| Wrote "overriding" while overloading | **Always** write `override` on the derived virtual fn; if it's an overload (different signature), say so explicitly: "this one is an overload — different parameter list, compile-time." |
| Manually called the parent destructor in the child | §B4 below — never do it; know the one-liner. |
| Panicked at compiler errors | Use the 6-step debug protocol below. |
| Modeled "random classes" (Class A/B) | Come armed with Vehicle/Car **and** TrafficLight/Road/Intersection (§D1) — two ready mental models. |

**Compile-error panic protocol (memorize)** [ADVICE]:
1. Read **only the first** error — later errors are usually echoes.
2. Classify it: (a) missing `;` or `}` · (b) missing `#include` · (c) undeclared identifier → typo / wrong scope / missing `this->` · (d) "no matching function" → check parameter **types and const-ness** · (e) "cannot assign to member in const member function" → you promised `const` but modified.
3. Fix it, recompile, **say out loud what you fixed** — debugging calmly is a scored skill.

**Likely follow-up the moment you finish:** "Now write the destructors." Answer:

```cpp
~Car()     { cout << "~Car\n";     }   // body only — compiler auto-chains ~Vehicle()
~Vehicle() { cout << "~Vehicle\n"; }   // already virtual
```

Destruction order for a `Car`: **~Car first, then ~Vehicle** — the reverse of construction (base constructed first, destroyed last).

### B2. The four pillars — one page, with Vehicle/Car mappings

| Pillar | One-line definition (say aloud) | In the Vehicle/Car code | Why it matters (say aloud) |
|---|---|---|---|
| **Encapsulation** | Bundle data + the methods that operate on it, and hide the data behind access control | `regNo_` is `private`; only `regNo()` exposes it read-only | Invariants can't be broken by outsiders; I can change the representation (e.g. store a struct) without touching callers — unlike a C struct where anyone writes any field |
| **Abstraction** | Expose **what** an object does, hide **how** | Callers use `farePerKm()`; they don't know how a car/truck computes it | Reduces coupling; new vehicle types plug in without changing calling code |
| **Inheritance** | A derived class reuses and extends a base's interface/implementation (`is-a`) | `Car : public Vehicle` | Code reuse + substitutability: any `Vehicle*` slot can hold a Car |
| **Polymorphism** | One interface, many behaviours | `farePerKm()` resolves to Car's or Truck's at runtime | Write fleet-level code once; add types without editing it (Open/Closed principle) |

**"Why is OOP better than procedural C?" [CAND asked verbatim]** — spoken answer:
> "C gives me structs and free functions — data and behaviour are separate, and there's no access control, so any code can corrupt any struct's state. OOP binds data with the operations that maintain its invariants (encapsulation), lets me abstract behaviour behind interfaces (virtual functions), and lets me write generic code against a base type that works for every derived type (polymorphism). For systems like a traffic controller or a payment type hierarchy, that means adding a new type doesn't require editing old code — which is where procedural code tends to break."

### B3. Overloading vs overriding — the trap topic [CAND]

| | **Overloading** (compile-time / static polymorphism) | **Overriding** (runtime / dynamic polymorphism) |
|---|---|---|
| Where | Same scope (usually same class) | Base declares `virtual`, derived re-declares |
| Signature | **Must differ** (parameter types/count) | **Must match exactly** (or covariant return) |
| Binding | At compile time by signature | At runtime via vtable when called through base pointer/reference |
| Keyword | none | `override` on the derived function |
| `Vehicle` example | `farePerKm()` vs `farePerKm(double surge)` | `Car::farePerKm() const override` vs `Vehicle::farePerKm()` |

**Must-know follow-ups:**
- *Can you overload on return type alone?* **No** — overload resolution ignores return type.
- *Can static functions be overridden?* **No** — statics have no `this`, no vtable entry; a same-signature static in the derived class **hides** the base one.
- *Does `const` matter?* Yes — `const` is part of a member function's signature; a const and non-const pair is a **redefining/overloading** pair (common in STL: `operator[]` const/non-const).
- *What if the signature differs by accident?* You get **hiding**, not overriding — the base versions are shadowed. `override` catches this at compile time. This is exactly the reported mistake [CAND].
- *Virtual keyword on the derived too?* Optional (it stays virtual), but write `override` — self-documenting and compiler-checked.

### B4. Virtual functions and destructors — know this cold [CAND: destructor trap asked verbatim]

**How runtime polymorphism works (vtable):** each class with virtual functions has one hidden **vtable** (array of function pointers); each object carries a hidden **vptr** to its class's vtable. A call `v->farePerKm()` through a `Vehicle*` becomes: fetch vptr → index `farePerKm` slot → call whatever is there (Car's or Truck's). Cost: one indirection. That's the whole machinery.

**When does dispatch happen?**
- Call through **base pointer/reference + virtual** → runtime (dynamic) dispatch.
- Call on a concrete object (`c.describe()`) → compile-time binding (compiler may devirtualize).
- Non-virtual call through base pointer → base version runs (**hiding**), regardless of the real object.

**Object slicing (classic follow-up):**
```cpp
Vehicle v = Car("DL-01", 4);   // copies ONLY the Vehicle part of Car — seats_ is sliced off
v.farePerKm();                 // Vehicle's version: 20. Car-ness is gone.
```
Polymorphism needs **pointers/references**. Storing derived objects by value in `vector<Vehicle>` slices them. Hence `vector<unique_ptr<Vehicle>>`.

**Why a polymorphic base class needs a virtual destructor:**

```cpp
Vehicle* v = new Car("DL-01", 4);
delete v;
```
`delete v` executes `~Vehicle` **statically** if `~Vehicle` is not virtual — the compiler sees a `Vehicle*` and calls the base destructor only. `Car`'s destructor never runs, so `Car`'s members (`seats_`, and any resource Car owns — e.g. a `std::string` buffer) are never destroyed: **undefined behaviour, resource leak**. With `virtual ~Vehicle()`, the delete dispatches through the vptr to `~Car`, which runs first and then chains `~Vehicle` automatically.

> **Spoken one-liner:** "If anyone can delete my object through a base-class pointer, I mark the base destructor virtual so the destructor chain starts at the *dynamic* type, not the pointer type."

**Why the derived destructor must NOT manually call the base destructor:**

The compiler **already generates the chain**: `~Car` body runs → then the compiler inserts an automatic call to `~Vehicle` → then memory is freed. This is the reverse-order mirror of construction (base constructor runs before the derived constructor body; base destructor runs after the derived destructor body).

```cpp
~Car() { ~Vehicle(); /* your cleanup */ }   // ❌ WRONG
```
Calling `~Vehicle()` manually destroys the Vehicle subobject **now** (frees anything it owns, e.g. `regNo_`'s string buffer), and then the compiler's automatic chain call destroys it **again** at the end of `~Car` → **double destruction / double free → undefined behaviour**. You also can't "undo" a destructor — an object whose destructor ran twice is permanently broken.

> **Spoken one-liner:** "The destructor chain is compiler-managed, like nested `close()` calls — the derived body runs first and the compiler automatically chains every base destructor in reverse construction order. Calling it myself would destroy the base twice — double free, undefined behaviour. I only write my *own* class's cleanup in my own destructor."

**Rule of thumb:** every class you intend to inherit from and delete polymorphically → `virtual ~Base() = default;`. (Fine print for follow-ups: a base used *only* as a mixin never deleted via base ptr can use a `protected` non-virtual destructor instead — mentioning this is bonus points, not required.)

### B5. Constructors, initialization lists, destructors

**Construction order (say aloud):** base class constructor → members **in their declaration order** (not the order in the init list!) → derived constructor body. Destruction is exact reverse.

**Initialization list vs assignment in body:**
```cpp
Car(string reg, int seats) : Vehicle(move(reg)), seats_(seats) {}   // ✅ members constructed directly
Car(string reg, int seats) : Vehicle(move(reg)) { seats_ = seats; } // ⚠️ members first DEFAULT-constructed, then assigned — wasted work; and const/reference members or members without default constructors make this illegal
```
- The init list is the **only** way to initialize: base classes, `const` members, reference members, members without a default constructor.
- `explicit` on one-argument constructors stops implicit conversions (`Vehicle v = "abc";` won't compile) — prevents surprising overloads.
- Member-init order follows **declaration order in the class**, not the init-list order — compilers warn; if your list order differs, that's a latent bug (e.g. initializing `seats_` with a member declared later).

**Destructor rules:** one per class, no parameters, no return; can't be overloaded; called automatically at scope end / `delete` / smart-pointer release; base destructor chains automatically (§B4). Rule of Three/Five: if you define a custom destructor, copy constructor, or copy assignment, you probably need all of them (raw owning pointer → deep copies). Best answer: use `vector`/`string`/smart pointers so you need **none** of them (Rule of Zero).

**Likely follow-ups:** *Can constructors be virtual?* No — vtable doesn't exist until the object exists; polymorphic creation is done with a **virtual clone idiom**: `virtual unique_ptr<Vehicle> clone() const`. *Can a destructor be pure virtual?* Yes (then you must still provide a body — derived chain calls it).

### B6. Pointers, references, const correctness

| | Pointer `Vehicle* p` | Reference `Vehicle& r` |
|---|---|---|
| Can be null | yes (check it) | no — must bind to an object |
| Can be reseated | yes (`p = &other`) | no — bound for life |
| Needs `*`/`->` | yes | no |
| Arithmetic | yes | no |
| Use for | optional values, arrays/iteration, ownership transfers | parameters & return of existing objects, polymorphic dispatch |

**Const correctness — the checklist the interviewer probes:**
```cpp
const string& regNo() const { return regNo_; }
//     ↑ returns const ref: caller can't modify, no copy   ↑ method doesn't modify *this
void setFare(const Vehicle& v);   // ① cheap pass-by-ref + ② promise not to modify
Vehicle* const p;      // const pointer     (can't reseat)
const Vehicle* p;      // pointer to const  (can't modify object)
```
- Read `const` right-to-left. A `const` member function may not modify members (except `mutable` ones, e.g. caches).
- Pass big objects by `const&`; return by `const&` only for members that outlive the call.
- *Follow-up:* "What does `const` buy me?" → documents and **enforces** contracts at compile time; enables optimizations; const methods can be called on const objects/refs.

### B7. Stack vs heap

| | Stack | Heap |
|---|---|---|
| Allocation | move stack pointer — ~nanoseconds | allocator bookkeeping — slower, can lock |
| Lifetime | automatic — dies at scope exit | manual — until `delete`/smart pointer |
| Size | small (MBs, default ~1–8 MB) | large, limited by RAM |
| Fragmentation | none | yes, over time |
| Failure | stack overflow (deep recursion) | bad_alloc / OOM |

```cpp
Car c("DL-01", 4);            // stack: auto-destructed at '}'
auto p = make_unique<Car>("DL-02", 5);  // heap object owned by a stack smart pointer → RAII
```
**Spoken line:** "Prefer stack/automatic storage; go to the heap for lifetime that outlives the scope, runtime-variable sizes, or polymorphic ownership — and then own it via smart pointers, never raw `new`/`delete`."

### B8. Inheritance types, access, and the diamond

**Five shapes:** single (`A→B`), multilevel (`A→B→C`), multiple (`B, C → D`), hierarchical (one base, many derived), hybrid (combination).

**Access table (`class D : X B`):**

| `X` | base `public` members become | base `protected` members become |
|---|---|---|
| `public` | public | protected |
| `protected` | protected | protected |
| `private` | private | private |

(Base `private` members are inherited but only accessible through base methods.)

**Diamond problem:** `D : B, C` where both inherit `A` → D has **two** A subobjects → ambiguous calls, duplicated state. Fix: `virtual` inheritance (`class B : virtual public A`) → single shared A. Cost: extra indirection (virtual-base offset). Say it as: "virtual inheritance makes the shared grandbase exist once."

### B9. Abstract classes and pure virtual functions

```cpp
class Shape {                              // abstract: cannot instantiate
public:
    virtual double area() const = 0;       // pure virtual — no implementation required here
    virtual ~Shape() = default;
    void describe() const {                // concrete: shared behaviour is fine
        cout << "area=" << area() << '\n'; // can call pure virtuals from base methods
    }
};
```
- One pure virtual ⇒ abstract; you can't create `Shape`, only derived concrete classes.
- An abstract class **may** have data members, constructors, and implemented methods — an **interface-style** class is the stricter idiom: only pure virtuals + a virtual destructor.
- Pure virtual **can** have a body and be called explicitly via `Derived::area()` — niche.
- `final`: lock a class/override from further derivation; `sealed` equivalent. Know the keyword, that's enough.

### B10. Composition vs inheritance — design-judgment question

- **`is-a` → inheritance**; **`has-a` → composition**. Car **is-a** Vehicle; Car **has-an** Engine (Engine as a member, not a base class!).
- **Prefer composition**: it's more flexible (swap behaviour at runtime — strategy objects), avoids fragile base classes, avoids deep hierarchies.
- Inheritance is right when there's true substitutability (LSP): derived must be usable anywhere the base is, without surprises (don't strengthen preconditions, don't weaken postconditions).
- **Spoken line:** "I inherit for substitutability, compose for reuse. In my traffic system the controller *has-a* timer and *has* phases; a Vehicle *is-a* FleetMember."

### B11. RAII and smart pointers

**RAII (Resource Acquisition Is Initialization):** acquire a resource in a constructor, release it in the destructor — cleanup is then **automatic on every exit path** (return, exception, scope end). C++'s core idiom: file handles, locks (`lock_guard`), sockets, memory.

```cpp
{
    lock_guard<mutex> g(m);            // locks here
    /* ... even if this throws ... */
}                                      // unlocks here, always
```

| Smart pointer | Meaning | Cost / gotcha |
|---|---|---|
| `unique_ptr<T>` | sole owner; move-only; zero overhead | default choice |
| `shared_ptr<T>` | shared ownership via refcount | atomic count overhead; **cycles leak** → break with `weak_ptr` |
| `weak_ptr<T>` | non-owning observer of a shared object | `.lock()` to use; breaks shared_ptr cycles (parent→child→parent) |
| raw `T*` / `T&` | **non-owning** observer only | never `new`/`delete` manually in modern code |

> **Spoken line:** "RAII means lifetime of resource == lifetime of object. I never write bare `new`/`delete`; ownership is `unique_ptr` by default, `shared_ptr` only for genuinely shared lifetimes, raw pointers only as non-owning observers."

### B12. Common C++ bugs — ranked by interview likelihood

1. **Calling base destructor manually** in derived → double destruction, UB (§B4) [CAND asked].
2. **Missing virtual destructor** on a polymorphic base → derived cleanup skipped, leak/UB.
3. **Overload/override mix-up** → accidental hiding; use `override`.
4. **Object slicing** — storing deriveds by value in base-typed containers.
5. **Dangling pointer/reference** — returning a reference to a local; using a pointer after delete.
6. **Iterator invalidation** — `erase` while ranging (fix: `it = v.erase(it)`, or iterate by index).
7. **Uninitialized members** — always init in the ctor (or use `{}` at declaration).
8. **Signed/unsigned comparison** — `for (int i=0; i<v.size(); ++i)` warns/loops forever on empty vector → use `size_t` or `ssize(v)` (C++20).
9. **Shallow copy of an owning raw pointer** → double free (Rule of Three).
10. **`=` vs `==`** in conditions; **off-by-one** loop bounds; **integer overflow** in `mid = (lo+hi)/2` (use `lo + (hi-lo)/2`).

### B13. C++/OOP rapid-fire (one-line spoken answers)

| Q | A (say this) |
|---|---|
| struct vs class? | Only default access: `struct` public, `class` private. Everything else identical. |
| static member? | Belongs to the class, not an object; one copy; must be defined out-of-class (unless inline/const integral). |
| static member function? | No `this` → can't be virtual, can only touch statics. |
| friend? | Grants a function/class access to private members — use sparingly (operators, test hooks). |
| inline? | Modern meaning: one-definition-rule linker permission, not "make it fast" — the optimizer decides inlining. |
| deep vs shallow copy? | Shallow copies the pointer (two owners → double free); deep duplicates the pointee. Rule of Three/Zero. |
| this pointer? | Implicit pointer to the invoking object inside non-static member functions; `delete this` is legal-but-lethal trivia. |
| Can a constructor be virtual? | No — no vtable before the object exists; use a virtual `clone()` factory instead. |
| Why C++ over Java for a systems task? | Deterministic destruction (RAII), no GC pauses, zero-overhead abstractions, manual memory layout — at the cost of manual safety. |
| const method modifying a member? | Only if the member is `mutable` (e.g. a memoization cache). |
| What is `override`/`final`? | `override` = compiler-checked redefinition; `final` = forbid further overriding/derivation. |

---

## C. DSA — interview-focused practice [P1]

> **Method note:** do NOT binge-read all solutions. The guide orders them by likelihood [CAND]. Attempt → speak approach → then read. In our chat sessions we do this interactively.

### C0. Complexity essentials + "which data structure when"

**Operation complexities (memorize):**

| Structure | Access | Search | Insert | Delete | Notes |
|---|---|---|---|---|---|
| array / `vector` | O(1) | O(n) | O(1) amortized (push_back), O(n) middle | O(n) | contiguous, cache-friendly |
| linked list | O(n) | O(n) | O(1) with node in hand | O(1) with node | poor cache locality |
| stack / queue | — | — | O(1) | O(1) | LIFO / FIFO |
| hash map `unordered_map` | — | **avg O(1), worst O(n)** (collisions) | avg O(1) | avg O(1) | no ordering, no range queries |
| ordered map `map`/`set` (RB-tree) | — | **O(log n)** | O(log n) | O(log n) | sorted order, `lower_bound`, range scans |
| priority queue (binary heap) | top O(1) | — | push **O(log n)** | pop O(log n) | build from n items: **O(n)** (`make_heap`) |
| sort | — | — | — | — | O(n log n) comparison; O(n) counting/radix when range is small |
| binary search | — | O(log n) | — | — | needs sorted, random-access |
| BFS/DFS | — | — | — | — | O(V+E), O(V) space |
| trie | — | O(L) by key **or prefix** | O(L) | O(L) | L = key length |

**"Which DS when" — the decision table (say aloud):**
- Need fast lookup by key, no order → `unordered_map`. Need sorted order / range / prefix-adjacent queries → `map`/`set` (ordered). Need repeated max/min → heap. Need sliding-window min/max → `deque`. Need prefix search → **trie**. Need connectivity/merging → union-find. Need counts of items → `unordered_map<T,int>` frequency. Need top-k streaming → min-heap of size k.

### C1. Second-largest element [CAND — asked with repeated modifications; highest-priority problem]

**Clarify first:** distinct or duplicates allowed? size < 2 → what to return? Are elements **updated/inserted/removed between queries** (this was the reported twist [CAND])?

**Approach ladder (the exact ladder the reporter was pushed through [CAND]):**
1. **Brute force:** two nested passes — find max, then max of all `< max` → O(n²) (or sort → O(n log n)).
2. **Single pass, two variables** — best static answer:
```cpp
// second-largest DISTINCT element; returns -1 if none
int secondLargest(const vector<int>& a) {
    long long m1 = LLONG_MIN, m2 = LLONG_MIN;
    for (int x : a) {
        if (x > m1) { m2 = m1; m1 = x; }          // new champion: old champion becomes runner-up
        else if (x < m1 && x > m2) m2 = x;        // skip duplicates of the max (distinct!)
    }
    return m2 == LLONG_MIN ? -1 : (int)m2;
}
```
O(n) time, O(1) space. Edge cases: all equal (`{5,5,5}` → -1), single element, all negatives, duplicates of max.
3. **Insert-only stream:** same two variables work — just keep them updated per element.
4. **Elements get inserted AND erased between queries [CAND's final twist]:** a sorted structure —
```cpp
set<int> s;                                  // or multiset if duplicates must count
// insert/erase: O(log n)
int secondLargest() {
    if (s.size() < 2) return -1;
    auto it = s.rbegin(); ++it;              // second element from the top
    return *it;
}
```
"Set keeps order under arbitrary inserts/erases; the two-variable trick can't survive deletions." (A lazy-deletion heap also works but is messier — mention it as an alternative.)

**Spoken approach:** "One pass keeping the max and the max of the rest — O(n)/O(1). If the interviewer then allows updates between queries, I switch to an ordered set: O(log n) insert/erase and the answer is the second element from the top."

**Almost-certain follow-up:** *k-th largest?* → min-heap of size k (O(n log k)), or quickselect (avg O(n)), or `nth_element` in C++.

### C2. Maximum-sum subarray (Kadane) [CAND 2023]

**Clarify:** at least one element required? all-negative allowed? want indices too?

```cpp
// returns {bestSum, startIdx, endIdx}; handles all-negative arrays
long long maxSubarraySum(const vector<int>& a, int& st, int& en) {
    long long best = a[0], cur = a[0]; st = en = 0;
    for (int i = 1; i < (int)a.size(); ++i) {
        if (cur < 0) { cur = a[i]; st = i; }      // a negative prefix only drags me down — restart
        else cur += a[i];
        if (cur > best) { best = cur; en = i; }
    }
    return best;
}
```
O(n) time, O(1) space. **Intuition to say aloud:** "At each index I decide: extend the current subarray, or start fresh here — start fresh whenever the running prefix has gone negative, because a negative prefix can only hurt the future."
**Follow-ups:** all-negative (`{-3,-1,-5}` → -1: the code already handles it since `cur<0` restarts); circular variant (total − min-subarray, edge case all-negative); divide-and-conquer O(n log n) (mention only if asked).

### C3. Maximum product of three elements (negatives included) [CAND]

**Key insight:** with negatives, three largest isn't always the answer — two big negatives × big positive wins.
**Answer:** `max(max1*max2*max3, min1*min2*max1)` where min1,min2 are the two smallest (most negative) values.

```cpp
int maxProductOfThree(vector<int> a) {         // simple version: sort
    sort(a.begin(), a.end());
    int n = a.size();
    return max({a[n-1]*a[n-2]*a[n-3], a[0]*a[1]*a[n-1]});
}
```
O(n log n). **O(n) version:** one pass tracking 3 largest + 2 smallest, then combine. Edge cases: zeros, exactly 3 elements, all negatives (`{-5,-4,-3}` → -60, i.e. product of all three — covered by `a[n-1]*a[n-2]*a[n-3]`).

**Spoken:** "Brute force three loops O(n³). Sort → check the two candidates: three largest, or two smallest × largest — O(n log n). If asked to avoid sorting, track 3 maxes and 2 mins in one pass — O(n)."

### C4. String compression `aaabbccccd → a3b2c4d` [CAND — actual OA question 2025]

**Clarify (the make-or-break):** is the count printed when it's **1**? (Target shows `b2`… and plain `d` → **omit 1**.) What about counts ≥ 10 (multi-digit)? Must it be in-place / O(1) space? What if compression grows the string (e.g. `ab` → `ab` fine, `aabb` → `a2b2` equal)?

```cpp
string compress(const string& s) {
    string out;  out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ) {
        size_t j = i;
        while (j < s.size() && s[j] == s[i]) ++j;   // find run end
        out += s[i];
        if (j - i > 1) out += to_string(j - i);     // omit count when 1  → matches "a3b2c4d"
        i = j;
    }
    return out;
}
```
O(n) time, O(n) space. **In-place variant if asked:** since counts can only shrink length, use two pointers writing back into the same buffer — O(1) extra space; count how many digits a number needs (`to_string(count).size()`) before writing.
**Edge cases:** empty string; single char; count 10+ (`"a" * 12` → `a12` — `to_string` handles); whole string distinct (returns unchanged).

### C5. Two Sum [STD warm-up]

**Clarify:** exactly one solution? can't reuse an element? return values or indices? sorted input (→ two pointers / binary search)?

```cpp
vector<int> twoSum(const vector<int>& a, int target) {
    unordered_map<int,int> seen;                     // value -> index
    for (int i = 0; i < (int)a.size(); ++i) {
        auto it = seen.find(target - a[i]);
        if (it != seen.end()) return {it->second, i};
        seen[a[i]] = i;                              // insert AFTER checking → handles x+x=target
    }
    return {};
}
```
O(n) avg / O(n) space. Brute force O(n²). If sorted: two pointers O(n)/O(1) — say that variant unprompted.

### C6. Valid parentheses [STD warm-up]

```cpp
bool isValid(const string& s) {
    unordered_map<char,char> open{{')','('},{']','['},{'}','{'}};
    stack<char> st;
    for (char c : s) {
        if (open.count(c)) {                         // closer
            if (st.empty() || st.top() != open[c]) return false;
            st.pop();
        } else st.push(c);                           // opener
    }
    return st.empty();
}
```
O(n)/O(n). Edge cases: `"]"` (closer first), `"("` (unclosed → stack not empty), `")("`. **Say aloud:** "stack because the most recently opened bracket must close first — LIFO matches nesting."

### C7. Binary search [CAND 2023 HR asked to explain]

```cpp
int binarySearch(const vector<int>& a, int t) {
    int lo = 0, hi = (int)a.size() - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;                // no (lo+hi)/2 overflow
        if (a[mid] == t) return mid;
        if (a[mid] < t)  lo = mid + 1;
        else             hi = mid - 1;
    }
    return -1;
}
```
O(log n)/O(1). **Follow-ups they love:** first/last occurrence → `lower_bound` / `upper_bound`:
```cpp
auto lo = lower_bound(a.begin(), a.end(), t);   // first ≥ t
auto up = upper_bound(a.begin(), a.end(), t);   // first > t
int firstIdx = (lo != a.end() && *lo == t) ? lo - a.begin() : -1;
int countOf  = up - lo;                          // number of occurrences
```
Explain **why binary search is correct**: invariant "if t exists, it's in `[lo, hi]`" — every step halves the interval while preserving the invariant.

### C8. Linked list: implement + reverse + detect cycle [CAND 2023: "write code to implement a linked list", "how does it work", "arrays vs linked list?"]

**Implement (singly):**
```cpp
struct Node { int val; Node* next; explicit Node(int v) : val(v), next(nullptr) {} };

class LinkedList {
    Node* head_ = nullptr;
public:
    ~LinkedList() { while (head_) { Node* nx = head_->next; delete head_; head_ = nx; } }  // RAII
    void pushFront(int v) { Node* n = new Node(v); n->next = head_; head_ = n; }
    void pushBack(int v)  {
        Node* n = new Node(v);
        if (!head_) { head_ = n; return; }
        Node* c = head_; while (c->next) c = c->next; c->next = n;
    }
    bool removeFirst(int v) {
        Node **pp = &head_;                          // pointer-to-pointer: handles head uniformly
        while (*pp && (*pp)->val != v) pp = &(*pp)->next;
        if (!*pp) return false;
        Node* dead = *pp; *pp = dead->next; delete dead;
        return true;
    }
    void print() const { for (Node* c = head_; c; c = c->next) cout << c->val << " -> "; cout << "NULL\n"; }
};
```
**How it works (say aloud):** "Each node holds data and a pointer to the next node; the list is just a head pointer. Insertion at head is O(1) — rewire two pointers; search is O(n) because nodes are scattered in memory — no random access, no cache locality."

**Reverse (iterative) — must be automatic:**
```cpp
Node* reverse(Node* head) {
    Node* prev = nullptr;
    while (head) { Node* nx = head->next; head->next = prev; prev = head; head = nx; }
    return prev;
}
```
Walk through one iteration aloud: "cut `next`, point it back, slide both pointers."

**Detect cycle (Floyd)** + find its start:
```cpp
bool hasCycle(Node* head) {
    Node *slow = head, *fast = head;
    while (fast && fast->next) {
        slow = slow->next; fast = fast->next->next;
        if (slow == fast) return true;               // fast laps slow inside the loop
    }
    return false;
}
Node* cycleStart(Node* head) {                       // follow-up: where does the cycle begin?
    Node *slow = head, *fast = head;
    while (fast && fast->next) { slow = slow->next; fast = fast->next->next;
        if (slow == fast) break; }
    if (!fast || !fast->next) return nullptr;
    slow = head;
    while (slow != fast) { slow = slow->next; fast = fast->next; }
    return slow;                                     // equal distances prove this meets at the entry
}
```
**Why Floyd works (say aloud):** "when they meet, fast has covered exactly twice slow's distance; subtracting cancels the loop repeats, so the distance from head to cycle-entry equals the distance from meeting point to entry — hence resetting one pointer to head and stepping both by one meets at the entry." Alternative: hash set of visited nodes, O(n) space.

**Arrays vs linked lists [CAND verbatim]:** "Array: O(1) random access, cache-friendly; insert/delete in middle O(n). Linked list: O(1) insert/delete given the node, but O(n) access/search and poor cache locality. Prefer arrays by default; linked lists when you splice constantly with node handles or need stable iterators/addresses. Fastest insertion **at a known position** → linked list; fastest searching → sorted array + binary search O(log n) (a plain linked list is O(n))." Mention `std::list` vs `std::vector` real-world caveat: vector usually wins even for insertion due to cache effects — bonus points.

### C9. Count connected components [CAND — actual OA 2023 graph question]

**Clarify:** undirected? nodes 0..n-1? self-loops/parallel edges?

```cpp
int countComponents(int n, const vector<vector<int>>& edges) {
    vector<vector<int>> g(n);
    for (auto& e : edges) { g[e[0]].push_back(e[1]); g[e[1]].push_back(e[0]); }
    vector<bool> vis(n, false);
    int cnt = 0;
    for (int i = 0; i < n; ++i) {
        if (vis[i]) continue;
        ++cnt;                                        // each unvisited seed = new component
        queue<int> q; q.push(i); vis[i] = true;       // BFS flood-fill
        while (!q.empty()) {
            int u = q.front(); q.pop();
            for (int v : g[u]) if (!vis[v]) { vis[v] = true; q.push(v); }
        }
    }
    return cnt;
}
```
O(V+E)/O(V). **Say aloud:** "scan every node; each time I find an unvisited node, that's a new component — flood-fill it with BFS/DFS." **Alternatives:** DFS recursion (stack overflow risk on huge graphs) and **union-find**: start with n components, every successful union decrements the count — mention it for bonus points.

### C10. Top-K elements [CAND via leaderboard/heap questions]

```cpp
vector<int> topK(const vector<int>& a, int k) {      // k largest, ties arbitrary
    priority_queue<int, vector<int>, greater<int>> minHeap;   // keep the K biggest, smallest on top
    for (int x : a) {
        if ((int)minHeap.size() < k) minHeap.push(x);
        else if (x > minHeap.top()) { minHeap.pop(); minHeap.push(x); }
    }
    vector<int> out;
    while (!minHeap.empty()) { out.push_back(minHeap.top()); minHeap.pop(); }
    reverse(out.begin(), out.end());
    return out;
}
```
O(n log k) time, O(k) space — say why a **min**-heap for k-largest (the smallest of the kept set is the gatekeeper). Alternatives: sort O(n log n); `nth_element` quickselect avg O(n) but unordered result. Stream version = same heap logic.

### C11. Phone directory with prefix search — TRIE [CAND — the question that sank a candidate]

**The reported trap [CAND]:** candidate answered `map<int,string>`; interviewer pointed out a phone directory is searched **by name prefix**, so that structure is useless. The expected answer: **trie**.

**Clarify:** search by name prefix → return matching contacts? case sensitivity? also exact phone-number lookup? deletion needed?

```cpp
struct TrieNode {
    TrieNode* ch[26] = {};
    bool isEnd = false;
    int contactId = -1;                 // leaf → index into contacts vector
    ~TrieNode() { for (TrieNode* c : ch) delete c; }   // RAII: recursive cleanup
};

class PhoneDirectory {
    TrieNode root_;
    vector<pair<string,string>> contacts_;             // {name, number}
public:
    ~PhoneDirectory() = default;                       // root_'s destructor frees the whole tree

    void add(const string& name, const string& number) {
        contacts_.push_back({name, number});
        TrieNode* cur = &root_;
        for (char c : name) {
            int i = tolower(c) - 'a';
            if (!cur->ch[i]) cur->ch[i] = new TrieNode();
            cur = cur->ch[i];
        }
        cur->isEnd = true;
        cur->contactId = (int)contacts_.size() - 1;
    }

    const TrieNode* findNode(const string& prefix) const {
        const TrieNode* cur = &root_;
        for (char c : prefix) {
            int i = tolower(c) - 'a';
            if (!cur || !cur->ch[i]) return nullptr;
            cur = cur->ch[i];
        }
        return cur;
    }

    vector<int> searchByPrefix(const string& prefix) const {   // collect subtree (DFS)
        vector<int> out;
        const TrieNode* n = findNode(prefix);
        if (!n) return out;
        // iterative DFS collecting all ends below n
        vector<const TrieNode*> st{n};
        while (!st.empty()) {
            const TrieNode* u = st.back(); st.pop_back();
            if (u->isEnd) out.push_back(u->contactId);
            for (TrieNode* c : u->ch) if (c) st.push_back(c);
        }
        return out;
    }
    bool exactExists(const string& name) const {
        const TrieNode* n = findNode(name);
        return n && n->isEnd;
    }
};
```

**Complexities:** insert O(L); exact search O(L); prefix search O(P + R) where P = prefix length, R = size of the matched subtree. Space O(alphabet × total chars) worst case.
**Why not alternatives (say aloud):** "`map<string,contact>` gives exact lookup O(L) but prefix search needs a full scan O(n). A sorted array + binary search on `lower_bound(prefix)` finds the block of matches in O(log n + R) — a legitimate alternative worth mentioning! Trie wins when you also want **interactive prefix suggestions** (after each typed char, O(1) lookup of the node) and incremental inserts." Saying the sorted-array alternative yourself = the maturity the failed candidate lacked.
**Design upgrade:** store up to 3 suggestions at each node for O(P) autocomplete — standard follow-up.

### C12. Job portal: `postJob`, `postUser`, `fetchJobs(userId, k)` [CAND — actual OA 2025]

**Clarify (say these):** "fetchJobs returns up to k jobs where `job.role == user.interestedRole` **and** `job.salary >= user.salaryExpectation` — correct? Among matches, which k? I'll assume **top-k by salary, descending**; ties by jobId. Can jobs be removed/updated after posting? (Assume no for now.)"

**Design:** bucket jobs by role; inside a bucket keep a **min-heap of size k**... but k varies per user and per call → better: keep per-role **max-heap / sorted structure** and pop k.

```cpp
struct Job  { int id; string role; long long salary; };
struct User { int id; string role; long long salaryExpectation; };

class JobPortal {
    unordered_map<int, Job> jobs_;                          // jobId -> job
    unordered_map<int, User> users_;                        // userId -> user
    unordered_map<string, priority_queue<pair<long long,int>>> byRole_;
        // role -> max-heap of (salary, jobId); ties broken by jobId (pair order)
public:
    void postJob(int jobId, const string& role, long long salary) {
        jobs_[jobId] = {jobId, role, salary};
        byRole_[role].push({salary, jobId});
    }
    void postUser(int userId, const string& interestedRole, long long salaryExpectation) {
        users_[userId] = {userId, interestedRole, salaryExpectation};
    }
    vector<int> fetchJobs(int userId, int k) {
        const User& u = users_.at(userId);
        vector<int> out;
        auto& pq = byRole_[u.role];                         // only this role's jobs are candidates
        priority_queue<pair<long long,int>> tmp = pq;       // O(m); or store per-role sorted vector
        while (!tmp.empty() && (int)out.size() < k) {
            auto [sal, id] = tmp.top(); tmp.pop();
            if (sal >= u.salaryExpectation) out.push_back(id);
        }
        return out;
    }
};
```
O(1) posts (amortized), fetch O(m log m) worst (copying heap) — or O(m + k) with a per-role sorted `vector` + scan, or O(m log k) streaming into a size-k min-heap. **Better spoken design:** "bucket by role; a fetch scans only that role's bucket, filters by salary floor, takes top-k." **SQL equivalent (they may ask):**
```sql
SELECT j.job_id FROM jobs j JOIN users u ON u.user_id = :uid
WHERE j.role = u.interested_role AND j.salary >= u.salary_expectation
ORDER BY j.salary DESC LIMIT :k;
```
with `INDEX(role, salary DESC)` → the DB streams top-k without a full sort. Saying the index makes you sound senior.
**Follow-ups:** job deletion → lazy invalidation (check `jobs_.count(id)` when popping); salary update → re-push + lazy skip; ties → comparator; millions of jobs → this is exactly "filter + order + limit + index" = database territory.

### C13. The "modification playbook" [ADVICE]

Interviewers iterate on a solved problem to test flexibility. Standard next moves and your reactions:

| Modification | Your move |
|---|---|
| "Do it in one pass / O(1) space" | two variables (second-largest), two pointers (in-place compression) |
| "Now it's a stream" | same O(1) state; or heap of size k |
| "Now inserts AND deletes" | ordered `set`/`multiset`, O(log n) |
| "Return indices / handle ties" | store (value,index) pairs; define comparator explicitly |
| "Millions of items" | talk DB: filter → index → order → limit |
| "What breaks at scale?" | memory (hash of all), then answer: buckets/index/DB |
| "Another constraint: only 10 MB RAM" | external processing / streaming aggregates |

---

## D. Object-oriented design and low-level design [P1]

### D0. The LLD method (use for every prompt)

1. **Clarify requirements** (2–3 questions — scope, what changes at runtime, edge cases). Interviewers grade the questions.
2. **Nouns → classes; verbs → methods.** Assign ONE responsibility per class (say "single responsibility principle" once, not five times).
3. **Relationships:** is-a (inheritance, sparingly) vs has-a (composition, default). Controller *owns* its parts.
4. **Data structures** per class — justify with complexities (use the C0 table).
5. **Core method walkthrough** — trace one full request end-to-end out loud.
6. **Edge cases + "how would the design change if…?"** — this is what separates pass from strong pass.

---

### D1. Traffic-light management system [CAND — the flagship LLD question]

**Required coverage [CAND + your prep spec]:** stable road IDs when roads merge · multiple lanes · **conflicting green signals** · pedestrians · emergency vehicles.

**Clarify:** one intersection or many? fixed-time cycles or sensor-based? what counts as conflict (cross-traffic, opposing turns)? pedestrian buttons or automatic? emergency preemption expected? failure behaviour?

**Design decisions to state explicitly:**

1. **Stable identities:** every approach gets an immutable `roadId` ("N1","S1","E1","W1") assigned once at construction and stored in a `map<string, Approach>`. **Never use array indices as identity** — when a road is added/merged, indices shift and every stored reference breaks; string/UUID keys are stable. This is the exact sub-question reported [CAND]: *roads merge → how do IDs stay valid?* Answer: "IDs are primary keys, not positions. Merging adds a new approach with its own ID and new conflict rows; nothing existing is renamed or re-indexed."
2. **Conflicting greens:** conflicts are a **property of the intersection, precomputed as a conflict matrix** (pairs of approaches that may never be green simultaneously: crossing flows, opposing left-turns). A **Phase** is a set of approaches that may be green together — validated against the matrix at construction. The controller never decides green-ness per light ad hoc; it only ever *applies whole phases*. This guarantees no conflicting green by construction.
3. **State machine per signal:** `RED → GREEN → AMBER → (all-red clearance) → RED`. Transitions: GREEN(min green) → AMBER(3 s) → ALL-RED(2 s) → next phase. All-red clearance time is what physically prevents the "both yellow" race.
4. **Lanes:** an Approach has lanes; a lane is vehicle or pedestrian type; special lanes (left-turn arrows) can carry their own signal state — lane-level state optional, approach-level default.
5. **Pedestrians:** pedestrian phases run **only in parallel with the vehicular green they cross against** (parallel, never perpendicular); button request = queued demand consumed at the next compatible phase; countdown display.
6. **Emergency vehicles:** preemption request for approach X → if X already green: extend green; else pick the phase containing X, transition **safely** (current green → amber → all-red clearance → X green), then optionally resume the interrupted phase. Never jump straight to green.
7. **Failure:** any fault → fail-safe state = flashing red / all-red (treat as stop signs).

**Classes (compact skeleton):**

```cpp
#include <bits/stdc++.h>
using namespace std;

enum class LightState { RED, AMBER, GREEN };
enum class LaneType   { VEHICLE, PEDESTRIAN };

class Lane {
public:
    Lane(string id, LaneType t) : id_(move(id)), type_(t) {}
    const string& id() const { return id_; }
    LaneType type() const { return type_; }
    LightState state() const { return state_; }
    void setState(LightState s) { state_ = s; }        // controller is the only writer
private:
    string id_; LaneType type_; LightState state_ = LightState::RED;
};

class Approach {                                       // ONE road entering the intersection
public:
    Approach(string roadId, string name) : roadId_(move(roadId)), name_(move(name)) {}
    const string& roadId() const { return roadId_; }   // stable primary key — never an index
    void addLane(Lane l) { lanes_.push_back(move(l)); }
    const vector<Lane>& lanes() const { return lanes_; }
private:
    string roadId_; string name_; vector<Lane> lanes_;
};

class ConflictMatrix {                                 // precomputed safety knowledge
public:
    void markConflicting(const string& a, const string& b) { pairs_.insert(minmax(a, b)); }
    bool areConflicting(const string& a, const string& b) const {
        return pairs_.count(minmax(a, b)) > 0;
    }
private:
    set<pair<string,string>> pairs_;                   // normalized unordered pairs
};

class Phase {                                          // a CONFLICT-FREE green set
public:
    Phase(string name, int greenSecs, vector<string> approachIds, const ConflictMatrix& m)
        : name_(move(name)), greenSecs_(greenSecs), ids_(move(approachIds)) {
        for (size_t i = 0; i < ids_.size(); ++i)       // validate: no internal conflicts, ever
            for (size_t j = i + 1; j < ids_.size(); ++j)
                if (m.areConflicting(ids_[i], ids_[j]))
                    throw invalid_argument("phase contains conflicting approaches");
    }
    bool contains(const string& roadId) const {
        return find(ids_.begin(), ids_.end(), roadId) != ids_.end();
    }
    const string& name() const { return name_; }
    int greenSecs() const { return greenSecs_; }
    const vector<string>& approachIds() const { return ids_; }
private:
    string name_; int greenSecs_; vector<string> ids_;
};

class IntersectionController {
public:
    IntersectionController(ConflictMatrix m, vector<Phase> phases, int amberSecs, int allRedSecs)
        : matrix_(move(m)), phases_(move(phases)), amberSecs_(amberSecs), allRedSecs_(allRedSecs) {}

    void runCycle() {                                  // one full loop over phases
        for (size_t i = 0; i < phases_.size(); ++i) runPhase(i);
    }
    void requestEmergencyPreemption(const string& roadId) {
        size_t target = phaseContaining(roadId);
        if (target == current_) { extendCurrent(); return; }   // already green: just extend
        transitionTo(target);                              // amber + all-red + green (never instant)
    }
    void requestPedestrian(const string& roadId) { pedestrianPending_.insert(roadId); }
        // consumed when the phase that is safe for this crossing runs next

private:
    void runPhase(size_t idx) {
        current_ = idx;
        setGreen(phases_[idx]);                            /* GREEN for greenSecs_ (timer/sensors) */
        setAll(LightState::AMBER);                         /* AMBER amberSecs_                  */
        setAll(LightState::RED);                           /* ALL-RED clearance allRedSecs_     */
    }
    void transitionTo(size_t target) { /* finish current safely: amber -> all-red -> green(target) */ }
    void setAll(LightState s);  void setGreen(const Phase& p);  void extendCurrent();
    size_t phaseContaining(const string& roadId) const;

    ConflictMatrix matrix_;
    vector<Phase> phases_;
    unordered_map<string, Approach> approaches_;       // roadId -> approach (STABLE ids)
    unordered_set<string> pedestrianPending_;
    size_t current_ = 0; int amberSecs_; int allRedSecs_;
};
```

**Complexities & edges:** conflict lookup O(log C) (set) or O(1) with hashing; phase validation O(k²) at construction; N phases → cycle O(N·timings). Edge cases: unknown roadId in a request → reject/log; pending pedestrian when their compatible phase never comes → watchdog; all lights must go fail-safe on controller crash (hardware timer / watchdog).

**"How does the design change if…?"** — sensor-actuated timing → replace fixed `greenSecs_` with a `TimingPolicy` interface (strategy pattern); adaptive network of intersections → controller talks to a coordinator (green waves); tram priority → new request type → new handler class; want unit tests → inject a `Clock` instead of `sleep`.

**Spoken walkthrough (60 seconds):** "I model the intersection as approaches with immutable IDs — IDs are keys, not positions, so merging roads never breaks anything. Safety is structural: a conflict matrix marks pairs that can never be green together, and the controller only ever activates whole **phases** validated at construction against that matrix — so a conflicting green is impossible by construction, not by runtime checks. Each phase runs green → amber → all-red clearance → red. Pedestrian requests are queued and served by the phase that's safe for them. Emergency preemption either extends the current green or performs the safe transition — amber, clearance, then green — and we fail safe to all-red flashing on any fault."

---

### D2. Snake and Ladder — with a snake that vanishes after first use [CAND]

**Clarify (say these):** board size 100? dice 1–6? need **exact** roll to finish or overshoot bounces back? multiple players? ladder and snake never share a start cell? — and the twist [CAND]: **"a snake disappears after biting the first player who lands on it"** — only that snake, ladders stay?

**Core rule to nail:** land on a snake-head → slide to its tail, then **erase that snake from the board** (it bit once). Later landings on that cell = normal cell.

```cpp
class Board {
public:
    explicit Board(int cells = 100) : cells_(cells) {}
    void addSnake(int from, int to)   { snakes_[from] = to; }    // from > to
    void addLadder(int from, int to)  { ladders_[from] = to; }   // from < to

    // returns final cell after applying jumps; consumes the snake if triggered
    int resolveLanding(int cell) {
        if (ladders_.count(cell)) return ladders_[cell];         // ladders persist
        auto it = snakes_.find(cell);
        if (it != snakes_.end()) { int tail = it->second; snakes_.erase(it); return tail; }
        return cell;                                             // snakes vanish after first bite
    }
    int cells() const { return cells_; }
private:
    int cells_;
    unordered_map<int,int> snakes_, ladders_;
};

class Player { public: string name; int pos = 1; explicit Player(string n) : name(move(n)) {} };

class Game {
public:
    Game(Board b, vector<string> names) : board_(move(b)) {
        for (auto& n : names) players_.emplace_back(move(n));
    }
    void play() {                       // loop turns until someone wins
        while (true) {
            Player& p = players_[turn_];
            if (takeTurn(p)) { cout << p.name << " wins\n"; return; }
            turn_ = (turn_ + 1) % players_.size();
        }
    }
private:
    bool takeTurn(Player& p) {
        int roll = dice();                                  // 1..6 (inject RNG for tests)
        int target = p.pos + roll;
        if (target > board_.cells()) return false;          // house rule: overshoot = no move (clarify!)
        p.pos = board_.resolveLanding(target);
        return p.pos == board_.cells();
    }
    static int dice() { static mt19937 rng(42); return uniform_int_distribution<int>(1,6)(rng); }

    Board board_; vector<Player> players_; size_t turn_ = 0;
};
```
**Edge cases to mention:** snake tail that is another snake head (chain) → `resolveLanding` applies one jump only; decide/clarify chaining; landing exactly on 100 wins; snake eaten changes future play — note it's the `unordered_map::erase` that implements "disappears".

**"How does the design change if…?"** — 6 gives extra turn → `takeTurn` returns bool "extra turn"; two dice → dice becomes a `Dice` interface class; persistence/undo → store moves as event list (ties to your Hermes event-sourcing story — free credibility!); many players online → turn queue + concurrency.

### D3. Phone directory (LLD shape) — reuse the trie from §C11

- `Contact { name, number }`; `PhoneDirectory` **owns** a trie + `vector<Contact>` (§C11 code).
- Methods: `add`, `searchExact`, `searchByPrefix`, optional `remove` (unmark `isEnd`, clear `contactId`; physical pruning of empty branches is optional — say "lazy deletion, prune later").
- If interviewer wants OOP structure: `Directory` delegates to an internal `Trie` class (composition!) — "I keep the trie as an implementation detail so I can swap it for a sorted-vector + binary-search implementation without touching the API."
- Follow-up: autocomplete with top-3 suggestions per node; case-insensitivity (normalize on insert); unicode → switch `ch[26]` to `map<char, node*>`.

### D4. Leaderboard [CAND]

**Clarify:** `addScore(playerId, score)` adds to a running total? `top(k)` returns k highest (order? ties?)? `reset(playerId)`? negative scores possible?

**Choose:** `unordered_map<id, score>` for O(1) updates **+** an ordered structure for top-k. Two clean options:

```cpp
class Leaderboard {
    unordered_map<int, long long> score_;                 // id -> total
    map<long long, unordered_set<int>, greater<>> byScore_;  // score -> ids (ordered desc)
public:
    void addScore(int id, int pts) {
        long long old = 0;
        if (auto it = score_.find(id); it != score_.end()) old = it->second;
        long long nw = old + pts;
        if (old) { byScore_[old].erase(id); if (byScore_[old].empty()) byScore_.erase(old); }
        score_[id] = nw;
        byScore_[nw].insert(id);
    }
    void reset(int id) {
        if (auto it = score_.find(id); it != score_.end()) {
            byScore_[it->second].erase(id);
            if (byScore_[it->second].empty()) byScore_.erase(it->second);
            score_.erase(it);
        }
    }
    vector<int> top(int k) const {
        vector<int> out;
        for (auto& [s, ids] : byScore_) {                 // scores descend; sets give tie order
            for (int id : ids) { out.push_back(id); if ((int)out.size() == k) return out; }
        }
        return out;
    }
};
```
`addScore`/`reset` O(log n); `top` O(k + log n). **Alternative to say aloud:** "single min-heap of size k doesn't survive arbitrary score updates — I'd need lazy deletion: push updated entries, skip stale ones when popping. That's O(log n) amortized but messier; the ordered map keeps invariants clean." (Ties: ordered `set`/`multimap` gives deterministic order; if interviewer says ties broken by earliest to reach the score → add a timestamp to the key.)

### D5. Job portal (LLD shape) — reuse §C12

- Classes: `Job {id, role, salary}`, `User {id, interestedRole, salaryExpectation}`, `JobPortal` (owns the maps).
- `postJob` O(1), `postUser` O(1), `fetchJobs(userId,k)` = role-bucket scan + salary filter + top-k (§C12 code + SQL index answer).
- **Requirement-change drill:** "add location" → filter key becomes `(role, location)` composite bucket or multi-index; "rank by relevance not salary" → scoring function injected (strategy); "jobs expire" → add TTL + lazy cleanup (ties into your Cron TTL story).

### D6. Parking lot / vehicle management [CAND-adjacent classic]

- Reuse §B1's Vehicle hierarchy: `Vehicle` (plate) ← `Car`/`Bike`/`Truck` — **this is the same Vehicle/Car code from B1; saying so shows reuse**.
- Classes: `Spot {id, SpotType}` , `Floor`, `ParkingLot` (owns floors), `Ticket {vehicle, spot, entryTime}`, `FeeStrategy` (interface → `HourlyFee`, `FlatFee`).
- Core: `park(vehicle)` → find free spot of matching type: per-type `queue<Spot*>` of free spots → O(1) pop; on exit, push spot back O(1). Nearest-first preference → min-heap by (floor, distance) or accept first-fit.
- `exit(ticket)` → fee = strategy(ticket.duration) → free the spot.
- Changes: EV spots (new SpotType), reservations (booking + TTL), multiple entrances (concurrency → lock per-type queue or DB-backed).

---

## E. Project deep dives [P1]

### E0. How to defend ANY project (the 5-part spoken structure) [ADVICE]

1. **Problem** (1 sentence) → 2. **Architecture** (components + why) → 3. **Data flow** (one request end-to-end) → 4. **Hardest problem** (symptom → investigation → root cause → fix → proof) → 5. **Trade-offs + what I'd do differently.**

**Honesty rules (non-negotiable):**
- Never quote a number you can't derive or restate the source of. Your resume's load-test numbers are quotable **because the resume cites the Load Test doc** — know how each was measured.
- If asked about something not built: "**Not implemented — here's how I'd do it**" is a *strong* answer. Guessing is the weak one.
- "Led a migration" on a solo project → say "I was the sole engineer, so I owned and led the migration myself." True and confident.

---

### E1. Distributed Task Scheduler (Cron-as-a-Service) — deep dive [VERIFIED against your repo]

> Everything below is verified from your source (`docker-compose.yml`, `database/init.sql`, `api-gateway/src/queue.ts`, `coordinator/internal/queue/*.py`, `worker/main.py`, `monitoring/*`, `screenshots/deployment/test-5/README.md`). Cite these names confidently — they are real.

**Say the stack exactly right:** the API gateway is **Node.js/TypeScript (Express)**; the **coordinator and workers are Python**; PostgreSQL 15 is the source of truth; RabbitMQ 3.13 provides due-time delivery; 21 containers via Docker Compose on one EC2 (t3.large, ap-south-1) behind Nginx. If asked "all Node?" → *"Gateway is Node/TS because the API surface is its strength; coordinator and workers are Python — I chose runtime per job and the contract lives in RabbitMQ + PostgreSQL, not in-process calls."*

**30-second intro (final version):**
> "I built a distributed cron service with a JWT-authenticated, rate-limited REST gateway, PostgreSQL as the authoritative store for schedules, and RabbitMQ delivering tasks at exactly their due time using per-message TTL + dead-lettering instead of database polling. Stateless workers scale horizontally — the benchmark ran three. I load-tested 10,000 tasks scheduled for the same instant: RabbitMQ peaked at 8,562 messages, drained at 50–61 tasks/s in ~7 min 46 s, and p95 recovered from a 160 ms spike to a ~76 ms floor with zero DLQ messages, zero task loss, zero worker crashes. Full observability — Prometheus, Grafana with an 18-panel dashboard, Loki, Alertmanager — deployed on EC2 via Docker Compose."

**Architecture with real names (draw this):**

```
Client ──JWT──> api-gateway (Express :3000) ──> PostgreSQL
                (helmet, validator, fixed-window rate limit)   users · tasks · task_executions · workers
                    │ publishScheduleWakeup(taskId, next_execution_time)
                    ▼
        scheduler.delay  (per-message `expiration` = due time, persistent)
                    │ TTL expires → dead-letter (DLX '' + routing key scheduler.due)
                    ▼
        scheduler.due ──> coordinator due_consumer
                    │  SELECT ... FOR UPDATE · mint executionId · INSERT task_executions
                    │  UPDATE tasks SET status='running' WHERE status='scheduled'
                    ▼
        scheduler.tasks ──> workers (prefetch 5; subprocess.run, no shell, 300 s timeout)
                    │ results                        │ failure → scheduler.retry_delay (TTL 5s·2^n, cap 5 min)
                    ▼                                └─ maxRetries=3 → scheduler.dlx → scheduler.tasks.dlq
        scheduler.results ──> result_consumer: UPDATE ... WHERE completed_at IS NULL
                    │ recurring → croniter next run → re-arm wakeup
   Reconciliation sweep: every 120 s · status='scheduled' AND due → nudge (LIMIT 100/pass)
```

**End-to-end data flow — 8 steps (know the order cold):**
1. `POST /api/v1/tasks` → express-validator (name ≤255 + escape, cron check for recurring, ISO8601 next time) → `INSERT INTO tasks ... status='scheduled'` → `publishScheduleWakeup(id, next_execution_time)`.
2. Gateway computes `delayMsUntil` = target − now, **capped at 2,147,483,647 ms** (the AMQP `expiration` field is an INT32 — know this number), publishes to `scheduler.delay` with per-message `expiration`, `persistent:true`.
3. TTL expires → RabbitMQ dead-letters that single message to `scheduler.due` (queue args `x-dead-letter-exchange: ''`, `x-dead-letter-routing-key: 'scheduler.due'`).
4. Coordinator `due_consumer`: `SELECT ... FOR UPDATE` → if `next_execution_time` still in the future, **re-arm a new wakeup** (guards premature dispatch) → `INSERT INTO task_executions` (minted **executionId**, status running) → `UPDATE tasks SET status='running' WHERE status='scheduled'` — rowcount 0 ⇒ someone else dispatched → delete execution + rollback → publish `{taskId, executionId, payload, retryCount:0, maxRetries}` to `scheduler.tasks`.
5. Worker: `execution_already_finished(executionId)` idempotency check → `subprocess.run(parts, timeout=300)` (whitespace-split, no shell) → success → publish result to `scheduler.results`.
6. Coordinator `result_consumer`: `UPDATE task_executions SET ... WHERE id=%s AND completed_at IS NULL` (rowcount 0 ⇒ duplicate → idempotent-skip counter) → update task status → **recurring**: `croniter` computes next time → status back to `scheduled` → new wakeup published.
7. Failure path: worker republishes to `scheduler.retry_delay` with TTL `min(300_000, 5_000 · 2^retryCount)` (5 s, 10 s, 20 s … capped 5 min); after `maxRetries` (default 3) → poison envelope to `scheduler.dlx` → `scheduler.tasks.dlq` **plus** a final `failed` result.
8. Reconciliation: every 120 s, `WHERE status='scheduled' AND next_execution_time <= NOW() ORDER BY next_execution_time LIMIT 100` → nudged to due. This catches the INSERT-then-publish window and any missed wakeup — worst case is a *delayed* execution, never a lost one.

**Verified load-test numbers (screenshots/deployment/test-5/README.md — you can defend how each was measured):**

| Metric | Value | How measured |
|---|---|---|
| Tasks submitted | 10,000 one-time `echo` tasks, all due +2 min (same instant) | bash+curl loop; SQL row count |
| Ingest rate | 73–74 msg/s | publish timestamps |
| Queue peak | **8,562 messages** @ 14:54:56 | RabbitMQ management UI (screenshot) |
| TTL-expiry burst | ~1,600 msg/s | management UI delta |
| Sustained ack rate | 50–61 tasks/s | management UI |
| Drain time | 14:55 → 15:02:46 ≈ **7 min 46 s** to zero | management UI timeline |
| p95 | 160 ms spike → ~77 ms steady → **~76 ms floor** | `scheduler_worker_task_duration_seconds` histogram in Grafana |
| End state | DLQ 0 · redelivered 0 · task loss 0 · worker crashes 0 · `status='scheduled'` rows 0 | SQL + UI |

**Mechanism one-liners (say these verbatim when probed):**
- **Why per-message TTL, not queue TTL:** queue-level TTL only expires the head; per-message `expiration` dead-letters each message individually. Residual head-of-line caveat exists — the 120 s reconciliation sweep bounds the damage (that's the 14:55 plateau you can point to in the graphs).
- **Exactly-once framing:** *"RabbitMQ is at-least-once — no publisher confirms, no quorum queues in my build, so duplicates are possible. The exactly-once **effect** comes from the executionId: minted inside a row-locked transaction, checked by the worker (`execution_already_finished`), enforced again at the result consumer (`WHERE completed_at IS NULL`), with the `WHERE status='scheduled'` transition as the dispatch guard. A duplicate becomes a no-op, never a double run."*
- **Worker crash:** unacked messages (prefetch 5) are redelivered to surviving workers; the execution ledger makes the retry a no-op if it already completed.
- **Heartbeats:** worker heartbeat thread every 20 s → `workers.last_heartbeat`; coordinator checker every 30 s marks `offline` after 2 min of silence; consumer liveness is *separately* gauged by `scheduler_worker_consumer_up` + the `WorkerConsumerDown` alert. (DB is the shared truth; the alert catches "process alive but consumer dead".)
- **Auth & limits:** JWT HS256, `expiresIn 7d`, bcrypt(10); rate limit = express-rate-limit **fixed window, 10,000 req / 15 min on `/api/`**, in-memory store (say "per-process; Redis store is the next step" if pushed); helmet, CORS, 10 kb body cap, per-user row scoping, parameterized SQL everywhere.
- **Alert rules (6):** TaskQueueBacklogHigh (queue >1000 for 5m) · DLQMessagesPresent (>0 for 1m) · WorkerConsumerDown (sum==0 for 2m) · WorkerAckRateZero · HighTaskRetryRate (rate >0.5 for 10m) · DLQGrowthRate.
- **p95 definition (define it before they do):** worker-side consume→finish duration including queue wait — the spike→floor shape in test-5 is backlog draining. It is **not** API→execution end-to-end latency — say so if asked.

**⚠️ Honest gaps (volunteer ONE proactively as "what I'd fix next"; own the rest if probed):**
1. **No automated unit/integration tests** — correctness came from live load + chaos tests (kill worker-1 mid-run, restart RabbitMQ, poison commands) with screenshots; CI runs lint/build only. "First thing I'd add: a pytest suite against docker-compose."
2. **gRPC proto is unwired** (`proto/scheduler.proto`: RegisterWorker/Heartbeat/RequestTask/ReportResult) — *"an early contract I drafted, then REST + the broker shipped first; it's dead code today — I'd wire RequestTask or delete it."*
3. **INSERT-then-publish window** on task creation — reconciliation compensates (delay, not loss), but a transactional outbox would close it properly.
4. Compose defines workers 1–10 while Prometheus scrapes 1–3 and the benchmark used 3 — say: *"three during the benchmark; ten in compose; scale is a config knob, the worker is stateless."*
5. Alertmanager's receiver has no notification channel wired (Slack commented out) — pipeline ends at the UI.
6. Worker executes arbitrary commands (no allowlist; no shell, 300 s cap) — documented trade-off in ARCHITECTURE.md; production fix = allowlist + container-per-task.
7. Hardcoded fallback creds in code (`change-me-in-production` JWT fallback) — known wart; the fail-fast secret guard from WebChat is the pattern I'd apply.

**Follow-up Q&A (the 10 hardest, with your real answers):**
1. *"Two workers get the same task?"* → at-least-once delivery + executionId guards; second run hits `execution_already_finished` → skip counter.
2. *"Worker crashes between finishing and acking?"* → redelivery; if the result was already applied (`completed_at` set) → idempotent skip; else duplicate result dropped by the conditional update.
3. *"Why not the RabbitMQ delayed-message plugin?"* → core AMQP only (per-message TTL + DLX to the default exchange); no plugin dependency; trade-off = head-of-line caveat, mitigated by reconciliation.
4. *"Reconciliation LIMIT 100 — doesn't 10k due tasks starve?"* → it re-scans every 120 s; tasks stay `scheduled` until dispatched, next pass picks more; the due-consumer's future-time guard prevents double-dispatch. That's exactly the test-5 drain plateau.
5. *"Why PostgreSQL and not Redis for schedules?"* → durability, transactional ledger, `FOR UPDATE`, partial index `tasks(next_execution_time) WHERE status IN ('pending','scheduled')` — the reconciliation query is index-covered.
6. *"Single coordinator — HA?"* → row-locked transition makes a second coordinator safe; compose runs one; next step would be leader election or accepting the small blast radius with the reconciler as backstop.
7. *"Scale past 10 workers?"* → workers are stateless; next bottleneck is DB write contention on the ledger + broker throughput; shard by queue or tenant.
8. *"Zero DLQ — is the DLQ useless?"* → no: it's the alarm; zero in that run means no poison messages; the DLQMessagesPresent alert fires within a minute if that changes.
9. *"Why is the retry TTL exponential 5s·2^n?"* → cheap failures fail fast; capped at 5 min to bound re-drive latency; after 3 attempts it's a poison and belongs in the DLQ, not the retry loop.
10. *"What does DELETE /tasks do to an in-flight wakeup?"* → the task row leaves `scheduled`; when the stale wakeup dead-letters, due_consumer sees non-`scheduled` status and acks it as a no-op — cancellation is eventually consistent by design.

---

### E2. WebChat — distributed real-time chat platform — deep dive [VERIFIED against your repo]

> Verified from source: `services/*` (6 services), `services/chat/src/server.js`, `services/message-worker/src/index.js`, `docs/ENGINEERING_AUDIT.md`, `deploy/k8s/*`, `.github/workflows/ci.yml`.

**The six services (exact names, ports, stores — this is the answer to "walk me through the architecture"):**

| Service | Port | Job | Stores |
|---|---|---|---|
| `gateway` | 4000 | reverse proxy, rate limit (200/min), CORS, `/metrics` (loopback-only) | — |
| `auth` | 3001 | JWT, users, rooms/membership, refresh tokens | **PostgreSQL** (Sequelize + migrations) |
| `chat` | 5000 | Socket.io, presence, RabbitMQ publish, WebRTC target | **Redis** (+ RMQ publish) |
| `messages` | 3003 | history, edit/delete, ack/read API, search | **MongoDB** + **Elasticsearch** (+ Redis cache) |
| `uploads` | 3004 | presigned S3 PUT/GET, delete | **S3** (MinIO in local dev) |
| `message-worker` | — | consumes persistence queue → Mongo + ES + Redis confirm | Mongo, ES, Redis |

K8s cluster also runs: client (nginx), Postgres 16, Mongo 7, Redis 7, RabbitMQ 3.13, ES 8.13.4, Prometheus, Grafana (MinIO StatefulSet in base; removed in the eks-demo overlay).

**One message end-to-end (the 8-step story — say it in this order):**
1. Client `handleSend` → `socket.emit("send_message", {roomId, content})`.
2. Chat handler validates **membership via `socket.rooms.has(roomId)`** (room membership was established at join time) → mints `tempId = temp_<ts>_<rand>`.
3. **Optimistic broadcast:** room receives the message immediately with `status:"sent"` and the temp ID.
4. `publishMessage()` → **confirm-channel** publish to exchange `webchat.messages`, routing key `message.persist`, `persistent:true`. **Fallback:** if RabbitMQ is down → synchronous HTTP POST to the messages service.
5. `message-worker` consumes `messages.persist` (prefetch 10) → `Message.create()` in Mongo → `indexInES()` (index `webchat-messages`) → publishes `webchat:message:persisted` on **Redis pub/sub**.
6. Chat subscribes to that channel → emits `message_confirmed` to the room.
7. Client swaps temp ID → real `_id` (`confirmMessage`); **dedupe guard** in `addMessage` skips any `_id` already rendered.
8. History reloads are safe: only persisted docs exist; read receipts / edit / delete have HTTP PATCH endpoints in the messages service.

**RabbitMQ topology (real):** exchange `webchat.messages` (direct, durable) → queue `messages.persist` (durable, `x-message-ttl: 600000` = 10 min, DLX `webchat.messages.dlx` → `messages.persist.dead`). **Retry = republish a NEW message with `x-retry-count` header + ACK the original** (never `requeue`); `MAX_RETRIES=3` → `nack(false,false)` → DLQ.

**The two race conditions — your REAL story (from `ENGINEERING_AUDIT.md`, memorize this framing):**
> "Two independent defects, both in the async persistence path. **Dropped (critical):** the worker persisted the message but never published `message_confirmed` when Redis wasn't available — clients kept a temp-ID bubble that silently vanished on reload. I fixed it with the Redis pub/sub confirm channel plus an HTTP fallback confirm path, and made sure `file_url` survives both paths. **Duplicated (high):** the worker's retry used requeue semantics — redelivery loops duplicated messages. Fixed by republishing a fresh message with an `x-retry-count` header and acking the original — no requeue, no loops."
**The killer follow-up and your answer:** *"what's still missing?"* → **"An idempotency key. The persist payload carries no stable messageId and Mongo has no unique index on (room_id, sender, tempId), so a crash between the Mongo write and the ack can still duplicate. The fix is client-supplied messageId + unique index + upsert — it's the top item in my engineering audit."** Volunteering this is a senior signal.

**Auth & security (real, quote the numbers):** access JWT **15 min**; refresh = 48-byte random, stored **SHA-256-hashed** (unique index), 7-day expiry; **rotation** on `/refresh` (old row destroyed); logout destroys the refresh row (revokes the refresh path; the access token simply expires ≤15 min — say that honestly). **Startup secret guard:** every service does `process.exit(1)` in production if the secret is still the dev default. Refresh-tokens are **rejected as access tokens** (`typ` check) in HTTP and socket paths. bcrypt(12). **XSS:** `sanitize-html` with a zero-allowlist at BOTH the messages service and the worker. Gateway rate limit: 200 req/min keyed on a slice of the Bearer token (else IP); auth: 30/15 min. Helmet on every HTTP service (CSP currently disabled — if asked: SPA inline assets; "nonce-based CSP is the next hardening step").

**Migrations (real):** custom runner with a `_migrations` ledger table — files sorted, applied once, recorded after success; invoked at service startup. **Replaced `sequelize.sync({alter:true})`**, which can drop/recreate columns in production. Say: *"the ledger gives me idempotent, ordered, reviewable schema change — the same reason Hermes uses Flyway."*

**Kubernetes (real):** kustomize base + `eks-demo` overlay; replicas base chat=1 → overlay chat=3; **HPAs:** gateway 2–6 @60% CPU, chat 2–10 @60%, message-worker 2–8 @70%; `default-deny-all` NetworkPolicy with per-app allowances (gateway→each service, chat→redis/rabbitmq/messages/auth, auth→postgres); PDBs (minAvailable 1) for gateway/chat/messages; ingress: `/socket.io` → chat:5000 with **3600 s proxy read/send timeouts**, `/api` → gateway, `/` → client nginx.
**Sticky sessions — you don't use them (say why):** *"the Socket.io Redis adapter makes room fan-out pod-independent — an event published on pod A reaches sockets on pod B — and presence lives in shared Redis keys with TTLs. Connections are pod-local, but correctness isn't."* Cross-pod delivery is **proven by test** (`chat-multi-instance.test.js` — one of your 22).
**Presence (real):** `presence:{userId}` JSON key (TTL 300 s) + `presence:online` sorted set; heartbeat refresh every 60 s; offline-delete is guarded by socketId match (reconnect race fix).

**The 9 deployment defects (real list — memorize 3, summarize the rest):** (1) NetworkPolicy blocked gateway→chat/uploads; (2) chat missing `RABBITMQ_URL`; (3) worker missing `REDIS_URL`; (4) uploads had MinIO env vars but S3 code; (5) CI k6 smoke ran without the gateway; (6) compose pointed at the wrong worker Dockerfile; (7) Elasticsearch index-name mismatch; (8) `message_confirmed` broken without Redis (the dropped race); (9) worker duplicate retries (the duplicated race). Punchline: *"most were config/contract drift between services — which is exactly why the 22-test integration gate now blocks every release."*

**The 22 tests (real breakdown):** auth 11 (register/validation/409-dupes, login, JWT /me + 401, rooms, members) + messages 5 (create + **HTML sanitize**, 401, pagination, author edit, soft delete) + chat 5 (socket auth accept/reject, join ack+broadcast, room broadcast, disconnect) + **1 cross-pod delivery test over the Redis adapter**. CI: `test-and-build` (Redis service container) → `npm test` **gates** `build-images` (3 images → ghcr.io) → compose integration smoke → deploy job (`kubectl set image` ×6 + rollout status 120 s).

**WebRTC — the honest scope:** client emits `call_offer/call_answer/call_ice_candidate/call_end`; `VideoCall.jsx` uses **Google STUN only** (`stun:stun.l.google.com:19302`), **no TURN**; the server-side relay handlers for `call_*` are not wired today. Say: *"P2P with STUN works on permissive networks; symmetric NAT needs a TURN relay — the socket relay handlers are the next piece of server work."* Do NOT claim calls work behind restrictive NAT.

**⚠️ Honest gaps (own them — they're in your own audit, which is your credibility):** `message_ack`/`mark_read`/`typing_*` socket events have no server handlers yet (HTTP PATCH endpoints exist — the socket path is unwired) · no graceful-shutdown handlers in services · placeholder secrets in the kustomize base (must be overridden per environment) · JWTs live in localStorage (XSS-surface trade-off; httpOnly cookie is the alternative) · membership cache TTL 60 s means a kicked user keeps access ≤60 s (invalidation hook not wired) · no unique dedupe index in Mongo (the idempotency gap above).

**Attack Q&A (10 hardest, real answers):**
1. *"Exactly-once persistence from chat → queue → Mongo?"* → at-least-once by design; confirm channel + persistent publishes; retries capped at 3 → DLQ; dedupe today = client `_id` guard; the real fix is the messageId unique index (say it first).
2. *"Client saw the message but Mongo lost it — walk the failure modes."* → optimistic emit precedes publish; publish failure falls back to HTTP; if both fail, the bubble never confirms — the Redis-confirm fix closed the silent-drop path; remaining mitigation: client timeout → mark "failed" + retry with the same tempId.
3. *"Why no sticky sessions?"* → adapter + shared Redis presence; nothing correctness-critical is pod-local. (This test exists: `chat-multi-instance.test.js`.)
4. *"Two workers on one queue — can message order invert?"* → yes: concurrent consumers, no ordering guarantee; Mongo sorts history by `{room_id, timestamp:-1}` (ms resolution) so near-simultaneous messages can render out of order; fix = Lamport-style per-room sequence.
5. *"Rate limit keyed on a JWT slice — attack it."* → one bucket per user (fine); unauthenticated falls back to IP behind the ingress — without `X-Forwarded-For` trust handling, a proxy can collapse everyone into one bucket; fix: key on real client IP + user id.
6. *"What happens when a worker pod dies mid-write?"* → unacked message redelivered → possible duplicate (see idempotency gap); grace period 60 s; no SIGTERM drain handler yet — the honest "next fix".
7. *"Why Elasticsearch when you have Mongo?"* → fuzzy full-text (`multi_match` + `fuzziness AUTO`) over content/username filtered by room; Mongo fallback `$regex` keeps search working if ES is down; trade-off = indexing lag (eventual consistency), which the UI shows as a source badge.
8. *"Why PostgreSQL AND MongoDB — justify the split."* → auth/rooms/refresh tokens are relational with FKs and unique constraints (Postgres); messages are write-heavy, schema-flexible documents with room-scoped compound indexes (Mongo); I can defend each schema line — and I'd consider consolidating messages onto Postgres if write volume stayed modest.
9. *"Where can a kicked user still send messages?"* → membership cache TTL 60 s (`room:{id}:member:{uid}`); invalidation hook not wired — bounded staleness, known gap.
10. *"Refresh-token rotation — replay window?"* → rotation destroys the old row; no reuse-detection alarm yet; logout revokes only the presented refresh token; the 15-min access token is the standing window — denylist on compromise is the design answer.

---

### E3. Hermes — event-sourced, multi-tenant workflow orchestration — deep dive [VERIFIED against your repo]

> Verified from source: `shared/event-store`, `shared/domain` (16 event types), `services/*` (13 TS Lambdas + Java replay/admin), `infra/` (18 Terraform modules), `docs/adr/*` (23 ADRs), `docs/INCIDENTS.md`, `workflows/templates/*.json`, `TESTING.md`.

**Say what it actually is (30-second intro, final version):**
> "Hermes is a multi-tenant workflow-orchestration platform built on event sourcing and CQRS. The core is TypeScript on AWS Lambda — command API, query API, activity workers, projections, and outbox/snapshot workers — with Step Functions executing the workflows, EventBridge as the event backbone, a single-table DynamoDB event store, six read-model tables, and OpenSearch — all provisioned by 18 Terraform modules. Two Java 25 + Spring Boot services survive: the time-travel replay service and the admin service (tenants + workflow registry on PostgreSQL with Flyway). State is an append-only event log, so execution history is replayable and auditable by design — and I migrated the command/query services from a JVM framework to serverless mid-project, documenting every removal as an ADR."

**Event store (real — this is your strongest 60 seconds):**
- Single table `hermes-dev-event-store`. **PK = `AGG#<AggregateType>#<id>`, SK = `EVT#<10-digit seq>#<eventId>`**; a `META` row carries the aggregate `version`.
- **Append-only, enforced:** event writes carry `ConditionExpression: attribute_not_exists(SK)`; nothing in the code deletes or updates event payloads.
- **Optimistic concurrency:** the META version write is conditional → conflict raises `ConcurrencyError` (there's a dedicated race test for this).
- **Multi-tenant reads:** `GSI1PK = TENANT#<tenantId>#<aggregateType>` partitions per tenant; every read model is tenant-prefixed.
- Envelope (quote it): `eventId, eventType, eventVersion, aggregateType, aggregateId, tenantId, correlationId, occurredAt, sequence, payload` — with **JSON-schema versioned contracts (v1→v3 upcasting tests)**.
- Streams `NEW_AND_OLD_IMAGES`; PITR on; KMS CMK with rotation.

**CQRS flow (real):** `POST /assets` → command-api → rehydrate aggregate from the stream → append → publish to bus `hermes-dev-events` (90-day archive) → EventBridge rules (content-filtered on sources) → **projections:** execution-projection (fans out to 6 read models), usage-projection (atomic `ADD` counters, PK `USAGE#<tenant>` SK `MONTH#YYYY-MM`), opensearch-projection (bulk index `hermes-executions-YYYY.MM`). `query-api` serves **only** read models (6 GET routes: execution, asset, workflow, tenant, tenant metrics, tenant audit). EventBridge → Step Functions via a **native `states:StartExecution` target** with an input transformer + retry policy + DLQ — no glue Lambda.

**Workflows (real):** 4 ASL state machines: `document-pipeline-v1` (Validate→OCR→Classify; retry 3× at 2 s ×2 backoff; Catch→WorkflowFailed — the functional end-to-end path) · `document-pipeline-v2` (adds replay-caching via Choice states reading `replayContext.cachedOutputs`, `Parallel(ner, embed)`, `Catch→CompensateSaga` — ⚠️ placeholder ARNs) · `approval-flow-v1` (`waitForTaskToken` human gate) · `batch-etl-v1` (`Map`, MaxConcurrency 10).

**⚠️ The transactional outbox — the honest version (they WILL probe this; this answer is a strength, not a weakness):**
> "The relay infrastructure is real and tested: `outbox-publisher` consumes an SQS queue (batch 10, `maxReceiveCount 3` → DLQ) and stamps entries `PUBLISHED`; `outbox-republisher` runs every 5 minutes sweeping stale `PENDING` items. But the producer side has a gap I know about: `append()` doesn't stamp `outboxStatus='PENDING'` yet, and the DynamoDB-Streams→SQS hop isn't wired, so today the live delivery path is the **inline EventBridgePublisher right after append — which is exactly the dual-write window my own ADR-012 warns about.** The fix is mechanical: stamp PENDING in the same write and let the relay own delivery. I can walk you through both paths and the failure modes of each."
*(Volunteering the gap + knowing the fix cold reads as senior engineering honesty — interviewers rate this far above fake completeness.)*

**⚠️ Saga + compensation — the honest version:** designed and typed: `CompensationTriggered` event type, ADR-019 defines per-step `compensationLambdaArn` (async, best-effort), ASL `Catch→CompensateSaga` in the v2 template. **The compensator Lambda itself is not implemented** — the old `SagaManager` lived in the Java command-api that I migrated away from. Say exactly that: *"designed, typed, not wired — the compensator is the first thing I'd build on the v2 template, and it's LIFO reverse-execution of completed steps."*

**Replay / time-travel (real):** Java `replay-service` exposes `POST /api/v1/replay` → loads the stream → `projectToEnd / projectToSequence / projectToTimestamp` → computes a **ReplayDelta** (steps to re-run vs `cachedOutputs` harvested from past `StepCompleted` payloads, honoring `skipCompletedSteps`) → returns `{replayJobId, eventsReplayed, stepsSkipped, cachedOutputsInjected, projectedToSequence}`. **Snapshots:** `snapshot-trigger` Lambda folds the stream on every 50th event (`sequence % 50 == 0`) and writes `SK = SNAP#<seq>`; the Java `SnapshotStore` reads the latest snapshot and resumes after it (10–50× fold reduction on long streams). **⚠️ Known bug to own proactively:** the Java reader's PK format (`EXECUTION#<id>`) **mismatches** the TS writer's (`AGG#WorkflowExecution#<id>`), and Java parses `payload` as a string while TS stores a map — runtime schema drift with no cross-runtime contract test. Your line: *"that mismatch is exactly why I'd add contract tests between runtimes — the store is the contract, and today only TS writes it."*

**The migration story (real, with ADR receipts — this is a genuinely great story):**
- **ADR-014 (superseded):** originally Java 21 + Spring Boot for command-api/query-api on ECS Fargate → **reverted**: the Java implementations were removed, TypeScript Lambdas took over (ADR-013: cold-start/runtime rationale). `docs/build-investigation.md` even records "Deleted JVM Services: services/command-api".
- **ADR-023 (superseded):** deleted duplicate Python OCR/classify workers — "no second implementation" principle.
- **ADR-022 (superseded):** ECS Fargate dropped once the services were Lambda handlers.
- **ADR-011 (removed components):** audit-service (the event store *is* the audit log), ingestion-service (merged into command-api), SNS, per-step queues, distributed locks, plugin system.
- Spoken frame: *"I led the migration as the sole engineer: strangler-style, component by component, each removal recorded as a superseded ADR with the reason. The resume's '100+ files' is the deleted JVM command/query services plus the duplicate Python workers — tracked as directories removed; the ADRs are the receipts."*

**The pipeline-defect story (real mechanism in your repo):** deployment uses two hand-synced shell lists — `bundle-lambdas.sh` and `deploy-lambdas.sh` (the script itself warns *"this list must stay in sync"*), and a missing zip is **silently skipped** (`skipping $NAME`). Concrete instance: the `saga-compensator` Lambda is referenced by the v2 state machine but absent from both lists — it would never deploy, on every release, with no error. Detection: inventory diff (Terraform-declared functions vs deployed artifacts vs runtime logs). Fix: generate both lists from a single source of truth and **fail the pipeline on a missing artifact**. Bonus real post-mortems from `INCIDENTS.md` if asked "walk me through a production incident": (1) **outbox stagnation** — batch size 1→10, concurrency 5→50; (2) **out-of-order projection updates** → Optimistic Sequence Guard design; (3) **KMS throttling** under burst.

**Multi-tenancy & auth (real, own the caveats):** `tenantId` from Cognito custom claims (`custom:tenantId`, dev fallback `x-tenant-id` header); `enforceTenantIsolation` throws on cross-tenant access; RBAC roles Admin/User/Service enforced in handlers; tenant-prefixed PKs in every read model; GSI1 tenant partition. **⚠️** The API Gateway JWT authorizer is **disabled by default** (smoke-test config); WAF (1000 req/5 min per IP + managed rules) and in-Lambda RBAC are the live controls; ADR-017's IAM `LeadingKeys` row-level condition is not implemented. If asked about auth depth, say this exactly — the nuance reads as maturity.

**Tests (real):** 28 TS test files, ~252 cases (Node `node:test` via tsx): domain invariants, **fast-check property tests**, **optimistic-lock race tests**, chaos duplicate-delivery tests, snapshot/replay tests, schema v1→v3 contract tests, per-Lambda handler tests. Java: 42 tests across admin + replay. Python: 9. CI: build+test+`terraform validate` (dev+staging), plus `verify-all.sh`.

**Attack Q&A (10 hardest, real answers):**
1. *"Prove your event append is safe."* → append-only via conditional `attribute_not_exists(SK)`; optimistic version on META; **but META and EVENT are two separate PutCommands — my ADR-015 mandates `TransactWriteItems`, and the shipped code doesn't use it yet: a crash between the two writes advances the version without an event. I know the exact fix (2-item transact write) and the test gap it implies.**
2. *"Does the outbox actually fire?"* → relay tested; producer stamp + Streams→SQS mapping missing; inline publisher is the live path (see the honest version above).
3. *"Where's the compensator?"* → typed + designed (ADR-019), not implemented; the placeholder ARN in the v2 template is honest scaffolding.
4. *"Java replay reads which items?"* → today: a PK-format mismatch means Java can't read TS-written streams — schema drift; fix = shared key/payload contract + cross-runtime contract tests.
5. *"Out-of-order projections?"* → EventBridge gives no cross-rule ordering; INCIDENTS.md's sequence guard is the designed fix (`SET sequence WHERE sequence < :seq` style) — the shipped projection has the conditional-start guard, not the sequence guard yet.
6. *"Idempotency under at-least-once?"* → layered: command short-circuits on already-registered assets / already-completed steps; conditional META version; projection conditional start; DLQ handler classifies poison at receiveCount ≥ 5.
7. *"Snapshot consistency?"* → snapshots fold every 50th event; the TS reducer is deliberately naive ("demonstration") while the domain reducers are the real fold; Java store guards snapshot races with conditional writes; only the Java reader consumes snapshots today.
8. *"Why six read models instead of one table?"* → per-model IAM write isolation and separate access patterns; the cost is eventual consistency between them — a trade I can justify per model.
9. *"How would you replay 10k events?"* → snapshot at seq 9,950 + delta fold; Java reader resumes from `resumeFromSequence` with pagination; the v2 Choice states inject cached outputs so completed steps never re-execute.
10. *"Hot tenant problem?"* → GSI1 partitions per tenant; a hot tenant concentrates on one partition — mitigation: per-tenant rate limits (the admin schema already has `api_rate_limit_per_second`), shard keys, or SQS buffering between command and projection.

---

### E4. NetSight — Linux network telemetry & traffic analysis ⚠️ INSUFFICIENT INFO [ADVICE: must fill before interview]

Your resume supplied does **not** include NetSight; you listed only a one-line description. **I will not invent its features, design, or numbers.** Before the interview, send me (or fill in here): capture method (libpcap / AF_PACKET / PF_RING / eBPF?), language, what "flow tracking" concretely does, what's actually implemented vs planned, any performance numbers, and how Prometheus metrics are exposed. Until then, only this skeleton is safe to rehearse:

- Purpose: passive traffic analysis on Linux; flows = aggregated packet streams (5-tuple).
- Likely probes you should be ready to answer **from your real work**: packet-capture path and its cost; ring buffer + memory-pool reasons (avoid alloc in hot path, avoid drops); epoll event loop vs blocking reads; kernel vs user-space boundary costs (copies, syscalls); eBPF's role (in-kernel filtering/aggregation — concept-level); drop accounting; exporter → Prometheus metrics.

### E5. Internship — S.N. Engineering Works (EngineCore Pro) [resume facts]

- 30s: "I took a B2B machinery marketplace from an empty repo to production in a React + Supabase stack: catalog browsing/filtering/configuring of precision-engineering products, and a custom project configurator that replaced manual email quoting with structured, instantly-generated quote requests feeding a lead pipeline with status tracking and team assignment."
- Defense points: why Supabase (managed Postgres + auth + realtime = solo-dev velocity); why the configurator beats email (structured data, no back-and-forth — measurable); post-internship ticketing module shipped at company request = ownership.
- Follow-ups: schema design (products, configs, leads, statuses); row-level security; how you'd add PDF quote generation; how you tested; what you'd rebuild differently.

### E6. Every skill on your resume — explained, tiered, and tied to YOUR projects

> **How to use:** ★★★ = deep, defend with repo specifics. ★★ = working knowledge. ★ = listed but shallow — answer concept-level and immediately bridge to your nearest ★★★ project context. Never oversell a ★ skill; redirect: "I know it conceptually; where I've actually used it is X — let me talk about that."

**Languages**

| Skill | Tier | What to know + your tie-in |
|---|---|---|
| C++ | ★★★ (interview) | Your interview language; 450+ LeetCode. §B is your defense. Tie-in: "systems mindset — that's why NetSight-style packet work appeals to me." |
| JavaScript / TypeScript | ★★★ | Gateway (TS, strict typing, express-validator), WebChat services, Hermes SDK/CLI. |
| Python | ★★ | Scheduler coordinator + workers (async consumers, croniter, psycopg). Q: "GIL?" → "Workers are subprocess/queue-bound; the coordinator loop is I/O-bound — the GIL isn't the bottleneck; I'd use asyncio for heavy fan-in." |
| Java | ★★ | Hermes replay + admin (Spring Boot, Flyway, JPA). Q: "Why Spring there?" → "Ecosystem: Flyway, Spring Data, mature testing — and ADR-013 explains why the hot path went TS instead." |
| SQL | ★★★ | Scheduler `init.sql` (partial index, `FOR UPDATE`), WebChat Sequelize migrations, Hermes Flyway. Q: "window functions?" — only answer to your real depth. |
| C | ★ | Pointers/memory fundamentals; concept-level unless you have coursework to show. |

**Frontend**

| Skill | Tier | Tie-in |
|---|---|---|
| React.js | ★★ | WebChat client (optimistic send + temp-ID confirm swap, IntersectionObserver read-receipt trigger, XHR upload progress, token single-flight refresh); EngineCore Pro. |
| Responsive UI | ★★ | Both client projects; mobile-friendly layouts, IntersectionObserver patterns. |
| Next.js | ★ | Concept-level: SSR/SSG/ISR, app router, why SSR helps SEO/first paint. Bridge: "my production React work is WebChat + internship." |
| Tailwind CSS | ★ | Utility-first model; concept-level. |

**Backend**

| Skill | Tier | Tie-in |
|---|---|---|
| Node.js / Express.js | ★★★ | Gateway + 6 WebChat services; middleware chains (helmet, CORS, rate limit), graceful structure, `/health` everywhere. |
| REST APIs | ★★★ | Versioned REST in all projects; correct status codes (401/403/409/429); per-user row scoping; pagination (`?page&limit` max 100). |
| WebSockets | ★★★ | Socket.io across pods via **Redis adapter**; socket-auth middleware; rooms; reconnect semantics. Raw-WS basics: upgrade handshake, frames vs polling fallback. |
| Spring Boot | ★★ | Hermes admin/replay: controllers, service layer, Flyway migrations, JPA repositories. |
| gRPC | ★ | Scheduler `proto/scheduler.proto` (RegisterWorker/Heartbeat/RequestTask/ReportResult) — drafted, unwired. Concept answer: HTTP/2 multiplexing + protobuf vs JSON; 4 streaming modes; when gRPC beats REST (internal, low-latency, contract-first). Honest line: "defined the contract early, shipped REST first — the proto is unwired today." |
| WebRTC | ★★ | WebChat: offer/answer + ICE via socket events, **STUN only** (no TURN). Know: SDP offer/answer, ICE candidates, STUN discovers public reflexive address, TURN relays when NAT is symmetric — say which piece your code has and which it lacks. |

**Data & Messaging**

| Skill | Tier | Tie-in |
|---|---|---|
| PostgreSQL | ★★★ | Scheduler schema (4 tables, partial index on due tasks, `SELECT ... FOR UPDATE`); WebChat auth (users/rooms/refresh_tokens, unique hash index); Hermes admin (Flyway). |
| MongoDB | ★★★ | WebChat messages: compound index `{room_id:1, timestamp:-1}`, soft delete, `$regex` search fallback. Q: "documents vs relational?" → write-heavy flexible docs vs FK-constrained identity data — and why I split them that way. |
| Redis | ★★★ | WebChat: Socket.io adapter pub/sub, presence keys (TTL 300 s) + sorted set, 60 s membership cache, confirm channel. Q: "Redis vs RabbitMQ?" → cache/fan-out/presence vs durable broker with acks/DLX — I use both, for different jobs. |
| RabbitMQ | ★★★ | Scheduler (7 queues: delay/due/tasks/retry_delay/results/DLX/DLQ; per-message TTL; prefetch 10/5; manual acks) + WebChat pipeline (confirm channel, DLX, x-retry-count). |
| Elasticsearch | ★★ | WebChat `webchat-messages` index: multi_match + fuzziness AUTO, room filter, monthly indices; inverted-index basics; eventual-consistency lag named aloud. |
| DynamoDB | ★★★ | Hermes: single-table event store (PK/SK/META/GSI1, conditional writes, streams) + 6 read-model tables; usage counters via atomic ADD. Know: partition distribution, RCU/WCU, item size 400 KB. |
| SQS | ★★ | Hermes outbox queue + DLQ (batch 10, maxReceiveCount 3, KMS). Q: "SQS vs RabbitMQ?" → managed, serverless-native, DLQ built-in vs self-hosted routing richness. |
| Supabase | ★★ | Internship: managed Postgres + auth + RLS; quote configurator data model. |

**Cloud & Infra**

| Skill | Tier | Tie-in |
|---|---|---|
| AWS Lambda | ★★★ | Hermes: 13 functions (command/query/workers/projections/outbox/snapshot/DLQ/webhook); nodejs20.x, X-Ray tracing, per-function least-privilege IAM. Know cold starts, memory→CPU scaling, 15-min cap. |
| Step Functions | ★★★ | 4 ASL state machines; Retry/Catch, `waitForTaskToken`, `Map` MaxConcurrency, Choice for replay caching. Q: "SFN vs your own loop?" → durable execution, visual audit, built-in retries — that's WHY I chose it over hand-rolled orchestration. |
| EventBridge | ★★★ | Bus + 90-day archive + content-filtered rules + native SFN target with input transformer. |
| API Gateway | ★★★ | Hermes HTTP API: 8 routes, WAFv2 (1000 req/5 min/IP + managed rules), Cognito authorizer (disabled by default — own that nuance), throttling. |
| AWS S3 | ★★ | WebChat presigned uploads (PUT TTL 300 s, GET 1 h, key pattern `rooms/{roomId}/{date}/{uuid}`); Hermes raw + replay-checkpoint buckets. |
| Cognito | ★★ | Hermes user pool; tenant claims (`custom:tenantId`); authorizer-present-but-disabled nuance. |
| AWS EC2 | ★★ | Scheduler deployment: t3.large, Nginx reverse proxy, security groups (public 80/443; DB/AMQP internal only). |
| AWS ECS | ★ | ADR-022: designed Fargate for Hermes, removed when services went Lambda — say "designed for it, deleted it with a documented reason." |
| Docker | ★★★ | Multi-stage builds (node:20-alpine 2-stage), compose with 21 services, healthchecks (`pg_isready`, RMQ), restart policies, per-service Dockerfiles. |
| Kubernetes | ★★★ | WebChat: kustomize base+overlay, HPAs (2–10 @60% CPU), NetworkPolicies (default-deny-all + allowances), PDBs, probes, ingress timeouts (3600 s WS). Know the 9-defect debugging list (§E2). |
| Terraform | ★★ | Hermes: 18 reusable modules + dev/staging envs; remote state concept, plan/apply review flow, `terraform validate` in CI. |
| GitHub Actions | ★★★ | Scheduler (lint/build per service), WebChat (test-gated image build → ghcr, compose integration, kubectl deploy), Hermes (build/test/tf-validate, OIDC role, deploy-dev). |

**Observability**

| Skill | Tier | Tie-in |
|---|---|---|
| Prometheus | ★★★ | Scheduler: worker duration histogram (→ p95 panel), queue-depth gauges, 6 alert rules; WebChat: per-service counters/gauges. Know counter/gauge/histogram/summary + rate()/histogram_quantile(). |
| Grafana | ★★ | Scheduler dashboard v2: 18 panels (queue depth, DLQ, per-worker utilization, p95); WebChat provisioned dashboard JSON. |
| Loki | ★★ | Scheduler logs via Promtail docker_sd (labels: container/stream/service); know label-based indexing vs ES full-text. |
| CloudWatch | ★★ | Hermes: alarms (DLQ depth, DynamoDB), observability dashboard, EMF structured logs (know: EMF = metrics embedded in log lines). |
| Micrometer | ★ | Spring Boot's metrics facade (actuator → Prometheus). Concept-level; bridge: "Hermes Java services would expose via actuator; my TS/Python services expose via prom-client/client_python." |
| k6 | ★★ | Scheduler load test (VU stages 50→200), WebChat websocket smoke scripts; know VUs, stages, thresholds. |
| Playwright | ★ | Listed; concept-level E2E (browser automation, assertions); bridge: "my E2E today is the 22-test integration suite + k6 smoke." |

### E7. "Why X, not Y?" — decision defense for every technology on your resume

**The universal 3-part formula — use it for ANY why-question:**
1. **The requirement that drove it** — specific to *your* project, not a generic fact about the tool.
2. **What the named alternative fails to give you FOR THAT requirement.**
3. **The honest cost you accepted + the condition under which you'd switch.** Conceding one real cost = instant credibility; fake absolutes = instant suspicion.

#### E7.1 Databases

| "Why X, not Y?" | Your answer (requirement → alternative gap → cost) |
|---|---|
| PostgreSQL (scheduler, WebChat auth) — not MySQL / Mongo? | *"Schedules and identity need transactional truth: `SELECT ... FOR UPDATE` guards double-dispatch of the same task, and a **partial index** (`WHERE status IN ('pending','scheduled')`) makes my reconciliation query index-covered — that's the query the whole design leans on. MySQL has no idiomatic partial index; Mongo has no FK integrity for users→rooms→memberships. Cost: I manage pooling myself (pg Pool, max 20)."* |
| MongoDB for messages — not Postgres? | *"Messages are write-heavy, append-mostly documents with a flexible shape (deliveries array, file_url) queried one way: newest-first per room — the compound index `{room_id:1, timestamp:-1}` is the whole access pattern. Postgres JSONB could carry it; the split exists because access patterns differ, not fashion. If ops burden dominated, I'd consolidate."* |
| BOTH Postgres AND MongoDB in WebChat? | *"Identity, rooms, refresh tokens = FKs, unique constraints, transactions (Postgres). Message store = throughput + flexible schema (Mongo). Polyglot by access pattern — and I can defend every column of each schema."* |
| DynamoDB — not RDS/Postgres — for the Hermes event store? | *"An event store's shape is: key lookup by aggregate + ordered scan by sequence. That's literally PK + sorted SK in DynamoDB — free ordering, `attribute_not_exists` for append-only, streams propagate changes to projections, and pay-per-request fits spiky workflow traffic with zero servers to patch. Postgres is honestly better for ad-hoc analytics — costs I accepted: no ad-hoc SQL, 400 KB item limit, hot-partition risk per tenant (mitigated: GSI1 tenant partition + per-tenant rate limits in the admin schema)."* |
| Single-table event store but SIX read-model tables? | *"The event store has ONE dominant access pattern — single-table discipline fits. The read models have distinct access patterns AND per-writer IAM isolation; separating them buys security and simplicity. Single-table is a discipline, not a religion."* |
| Redis — not in-process memory / Memcached? | *"The state must be shared and pod-independent: Socket.io adapter pub/sub, presence keys with TTLs, membership cache, message-confirmed channel. In-process memory dies with the pod; Memcached has no pub/sub or sorted sets. Redis's structures map 1:1 to my needs."* |
| Elasticsearch — not Mongo `$regex`? | *"Search needs fuzzy full-text (`multi_match` + `fuzziness AUTO`) ranked and room-filtered; regex scans neither scale nor rank. Cost: indexing lag (eventual consistency — my UI shows a source badge) and a dual-write path. (Hermes uses OpenSearch — same engine family, managed.)"* |
| Supabase — not Firebase / raw Postgres? | *"Internship constraint: one developer, weeks not months. Supabase = real Postgres (FKs, RLS) + managed auth + realtime in one — velocity without giving up relational integrity. Firebase is NoSQL-first, wrong fit for a product catalog with relational config data."* |

#### E7.2 Messaging & eventing

| "Why X, not Y?" | Your answer |
|---|---|
| RabbitMQ — not Kafka? | *"I need work-queue semantics: **per-message TTL** (deliver at due time), dead-letter exchanges, ack/nack, prefetch fairness, routing keys, a management UI to watch the 8,562-message peak. Kafka is an append-only log with consumer offsets — no native per-message delay, DIY dead-lettering, and overkill at my throughput (74 msg/s ingest). Switch condition: sustained 100k+ msg/s fanned to many replayable consumers."* |
| Per-message TTL + DLX — not the delayed-message plugin / Redis ZSET / DB polling? | *"Core AMQP only — no plugin to install or version-lock. Polling was the thing I removed (steady DB load + latency floor). A Redis ZSET would work but makes my own process the scheduler again. Facts to quote: `expiration` is INT32 ms (≈24.8-day max delay); head-of-line caveat is real and bounded by the 120 s reconciliation sweep."* |
| SQS in Hermes — not RabbitMQ there? | *"Serverless-native: no broker to host, DLQ + maxReceiveCount built in, Lambda event-source mapping with batch/concurrency tuning (I run batch 10, concurrency 50 — from a real incident). And note the contrast: SQS max delay is 15 minutes — fine for the outbox relay, but that's exactly why the scheduler used RabbitMQ TTL for arbitrary future times."* |
| EventBridge — not SNS / direct Lambda invokes? | *"I need content-filtered routing, a 90-day archive for replay/audit, and a native Step Functions target with an input transformer — multiple consumers per event without coupling producers to them. SNS is fan-out without rules or archive."* |
| Step Functions — not a custom orchestrator? | *"Durable execution survives Lambda restarts; Retry/Catch is declarative (my ASL: 3×, 2 s, ×2 backoff); `waitForTaskToken` gives human approval gates; `Map` gives bounded parallelism. A hand-rolled loop would reimplement all of that badly. Cost: per-transition pricing, AWS lock-in — accepted deliberately."* |

#### E7.3 Languages & runtimes

| "Why X, not Y?" | Your answer |
|---|---|
| **Java 25** — and why Java at all for replay/admin? | *"25 is the current LTS (Sept 2025) — virtual threads (the replay service does blocking stream folding), records + pattern matching for event envelopes, current Spring Boot support, zero upgrade debt on greenfield services. Why Java there at all: strong typing for state-machine math and the Spring ecosystem (Flyway, JPA) for admin. And the contrast IS the story: the hot, spiky command/query path moved to TypeScript Lambdas for cold starts — that's ADR-013/ADR-014."* If pushed on 21 vs 25: *"21 would've worked; 25 cost nothing and bought the newest features."* |
| Python 3.12 for coordinator/workers — not Node everywhere? | *"Right ecosystem for the job — croniter for cron math, psycopg for row-locked transactions — and clean long-running consumer loops. The runtime boundary is RabbitMQ + PostgreSQL, so mixed runtimes cost nothing architecturally. Single-runtime ops simplicity is real; my answer is that workers are stateless — I scale by replicas, not shared code."* |
| TypeScript — not JS? | *"Six-plus services sharing contracts: compile-time types are the API documentation and kill a whole class of null/shape bugs before runtime. Cost: a build step — accepted."* |
| Node 20? | *"The LTS line during development; Alpine multi-stage keeps images small. Upgrade path is routine."* |

#### E7.4 AWS building blocks

| "Why X, not Y?" | Your answer |
|---|---|
| Lambda — not ECS/containers — for the Hermes core? | *"Workflow traffic is spiky and event-driven: pay-per-invocation, zero patching, per-function least-privilege IAM. Cold starts measured acceptable on nodejs20.x (ADR-013) — and ADR-022 then deleted ECS Fargate entirely once command/query went Lambda. Switch condition: sustained high-throughput or <10 ms latency SLAs → containers."* |
| Terraform — not CDK / CloudFormation / Pulumi / console? | *"18 reusable modules shared across dev/staging; every change is a reviewed PR with `plan` preview — that's the safety mechanism; providers cover everything I use (SFN, EventBridge, WAFv2, Cognito). Cost: state-file discipline; `terraform validate` runs in CI."* |
| Cognito — not custom auth / Auth0? | *"Managed pools with custom tenant claims (`custom:tenantId`) that plug straight into the API Gateway authorizer — I don't want to own token infrastructure in a serverless design. Honest nuance: the authorizer is disabled in the dev smoke config today; RBAC runs in handlers — enabling edge validation is config, not code."* |
| HTTP API Gateway — not REST API / ALB / Lambda URLs? | *"JWT authorizer hook, WAF attachment, throttling — at roughly 70% cheaper than the REST API type. ALB wins only when you need heavier header routing or WebSocket fronting."* |
| Presigned S3 URLs — not upload-through-my-service? | *"Large files never traverse my containers — no bandwidth/memory tax, time-boxed credentials (300 s PUT, 1 h GET), CDN-friendly keys (`rooms/{roomId}/{date}/{uuid}`), and MinIO speaks the same API for local dev."* |
| WAF at the edge? | *"Managed rules + per-IP rate limits stop abuse **before** it invokes Lambda — that's cost protection, not just security."* |

#### E7.5 Infra & tooling

| "Why X, not Y?" | Your answer |
|---|---|
| Docker Compose for the scheduler — not Kubernetes? | *"The scheduler is single-node, 21 containers, no autoscaling or self-healing requirement — an orchestrator would be ceremony. WebChat runs Kubernetes precisely because IT needed HPAs, rolling updates, and NetworkPolicies. Same engineer, two honest answers — tool matches problem."* |
| Kubernetes for WebChat — not Compose? | *"Autoscaling was a requirement: chat HPA 2–10 @60% CPU, worker 2–8 @70%; plus rolling deploys, PDBs, default-deny NetworkPolicy, kustomize overlays. Compose can't express any of that."* |
| Socket.io — not raw `ws`? | *"Rooms, ack callbacks, auto-reconnect with polling fallback, and the Redis adapter — raw WS means rebuilding all of it, and the fallback transport also survives restrictive proxies."* |
| Separate coordinator — not workers doing DB transitions? | *"One writer owns state transitions so the invariants (`FOR UPDATE`, status guards) live in one audited place; workers stay dumb and stateless — I scale workers, not correctness."* |
| `subprocess` without shell=True? | *"Injection safety: args are tokenized, no shell interpretation of pipes/globs, 300 s timeout caps runaways. Allowlist + container-per-task is the documented next hardening."* |
| UUID primary keys? | *"No enumeration, safe across services, generated client-side; cost = index locality/bloat — fine at this scale."* |
| express-rate-limit **fixed window** — not token bucket? | *"Built-in and sufficient for the threat model — flood control, not billing precision. Honest limits: fixed window allows a 2× burst at boundaries and the in-memory store resets per process — Redis-backed token bucket is the production upgrade."* |
| kustomize — not Helm? | *"Base + overlay patches (replica counts, env) with no templating language. Helm earns its complexity for multi-parameter environments; I had one base and one demo overlay."* |
| Prometheus + Grafana + Loki — not CloudWatch-only / Datadog? | *"One stack works identically on EC2 compose and k8s; I control the panels (the p95 histogram panel is mine); Loki's label-based logs sit next to the metrics. CloudWatch is used where serverless demands it (Hermes EMF + alarms). Datadog: great product; cost + lock-in at student scale."* |
| k6 — not JMeter? | *"Tests as JS code, VU stages map directly to my 50→200 ramp, CI-friendly, WebSocket scenarios supported."* |
| node:test — not Jest? | *"Zero-config with tsx, native ESM, fast in CI. Jest's mocking ecosystem is the trade-off accepted."* |
| croniter — not pg_cron / hand-rolled? | *"Standard cron semantics for computing next run times without coupling scheduling into the database; hand-rolled cron parsing is a classic bug farm."* |
| GitHub Actions — not Jenkins / GitLab CI? | *"Repo-native; OIDC federation to AWS in the Hermes deploy (no long-lived keys); service containers (Redis in WebChat CI) make integration tests trivial."* |

#### E7.6 Frontend & security

| "Why X, not Y?" | Your answer |
|---|---|
| React — not Vue/Svelte? | *"Depth of ecosystem + the component model I know best; no strong technical rejection — I ship fastest in React, and shipping was the constraint."* |
| Vite for the WebChat client — not Next.js? | *"A chat SPA needs no SSR/SEO; Vite's HMR and tiny config win. (This is also my honest Next.js answer: I know it conceptually; where SSR wasn't needed, I chose Vite deliberately.)"* |
| JWT in localStorage — not httpOnly cookie? | *"Simplest cross-service bearer auth for the topology; the XSS trade-off is named in my audit — nonce CSP + cookie/CSRF is the hardening path."* |
| JWT access 15 min + hashed refresh rotation — not server sessions? | *"Stateless auth across horizontally scaled pods — this is literally WHY no sticky sessions are needed. Revocation story: rotation + logout deletion; the ≤15-min access-token window is the accepted cost."* |
| bcrypt(10/12) — not argon2? | *"Battle-tested, universal support. Argon2id is the modern pick for a greenfield production system — I'd say so myself."* |
| sanitize-html zero-allowlist — not client-side escaping only? | *"Sanitize at BOTH write points (messages service + worker) so every consumer — API readers, the ES index, web clients — gets clean HTML. Client-only escaping trusts the attacker's environment."* |

#### E7.7 Versions you must be able to quote

| Version | Why this one |
|---|---|
| Java 25 | current LTS (Sept 2025): virtual threads, records, pattern matching, current Spring Boot support |
| Node 20 / Python 3.12 | LTS / stable lines during development; Alpine/slim images |
| PostgreSQL 15 (scheduler) / 16 (WebChat k8s) | the features I rely on — partial index, `FOR UPDATE`, JSONB — exist in both; version = image at build time |
| RabbitMQ 3.13 | management UI (the evidence behind the 8,562 peak); quorum queues available if pushed on HA |
| MongoDB 7 · Redis 7 · Elasticsearch 8.13 · MinIO | current stable images in the k8s base manifests |

**If they push "would you choose differently today?" — have ONE prepared yes per project** (this shows growth, not flip-flopping): scheduler → *transactional outbox for INSERT-then-publish*; WebChat → *idempotency key + unique messageId index*; Hermes → *`TransactWriteItems` for the event append + wire the outbox producer stamp*. Every one of these is already documented in §E1–E3 — quote them as "my own audit," not as admissions.





---

## F. Core CS fundamentals [P2 — rapid-fire format]

### F1. DBMS

- **Keys:** superkey ⊇ candidate key (minimal superkey) ⊇ primary key (chosen candidate; NOT NULL, unique) · foreign key → referential integrity · surrogate (meaningless ID) vs natural (SSN) keys.
- **Normalization (say the one-liners):** 1NF atomic values, no repeating groups · 2NF no partial dependency on part of a composite key · 3NF no transitive dependency (non-key → non-key). BCNF: every determinant is a candidate key. Trade-off: more normalization = fewer anomalies but more joins.
- **Joins:** INNER (matches only) · LEFT/RIGHT OUTER (preserve one side, NULL-pad) · FULL OUTER · CROSS (cartesian). "Which rows survive" is the only question.
- **Indexes:** B+tree — sorted, shallow, leaf-level linked → range scans; lookup O(log n). Composite index order = equality columns first, range last (a range column kills use of later columns). Index slows writes & costs storage. Covering index = query answered from index alone.
- **Transactions/ACID:** Atomicity (all-or-nothing, undo log) · Consistency (constraints hold) · Isolation (concurrent txns don't see partial work) · Durability (committed survives crash — WAL, fsync).
- **Isolation levels vs anomalies:**

| Level | Dirty read | Non-repeatable read | Phantom |
|---|---|---|---|
| READ UNCOMMITTED | possible | possible | possible |
| READ COMMITTED | no | possible | possible |
| REPEATABLE READ | no | no | possible* |
| SERIALIZABLE | no | no | no |

(*InnoDB's MVCC RR largely prevents snapshots phantoms via gap locks — bonus point.) MVCC: readers don't block writers.
- **Deadlocks:** two txns hold locks the other needs → DB detects (wait-for graph) and aborts one. Prevention: consistent lock ordering, short transactions, lock escalation awareness.
- **SQL vs NoSQL (the honest answer):** relational for structured data, joins, strong consistency, mature tooling; NoSQL when access patterns need horizontal scale/flexible schema — document (MongoDB: flexible docs, app-level relations), KV (Redis: cache/session/leaderboards — O(1)), wide-column (DynamoDB: predictable-key scale), search (Elasticsearch: full-text). "It's polyglot persistence — I used both in WebChat, chosen by access pattern." Then defend your actual split.
- **EXPLAIN plans:** look for seq-scan-on-big-table, wrong index, row-estimate blowups.

### F2. Operating systems

- **Process vs thread:** process = isolated address space + resources; thread = execution unit *inside* a process, shares heap/fd table, has own stack/registers/PC. Inter-thread communication = shared memory (needs sync); inter-process = IPC (pipes, sockets, shared memory). Crash isolation: thread crash can kill the process; a process crash doesn't kill siblings.
- **Context switch:** save/restore registers + switch page tables (process) or not (thread); cost = direct (µs) + indirect (cache/TLB pollution — often the bigger cost).
- **Virtual memory:** each process gets private virtual address space; MMU translates via page tables; TLB caches translations; pages lazily loaded on fault (demand paging); page replacement (LRU-ish); benefits: isolation, overcommit, mmap files, copy-on-write fork.
- **Stack vs heap (OS view):** stack = per-thread, fixed-ish, fast, function frames; heap = allocator-managed (brk/mmap), shared across threads → needs allocator locks; stack overflow = deep recursion; heap fragmentation.
- **Mutex vs semaphore:** mutex = ownership, unlock by the locker (binary lock, supports priority inheritance); semaphore = counter + signal/unlock-by-anyone semantics (producer-consumer); spinlock = busy-wait (only for very short critical sections on multicore); condition variable = wait-for-state with a mutex.
- **Race condition:** result depends on timing; check-then-act and read-modify-write are the classic shapes. Fix: atomic ops, locks, or design (immutable data, channels). **Deadlock** = circular wait (4 conditions: mutual exclusion, hold-and-wait, no preemption, circular wait — break any one); **livelock** = actively retrying forever; **starvation** = never scheduled.
- **System call:** user → trap into kernel mode → kernel validates + executes → returns; costly (~µs) — that's why buffering/batching/vDSO exist. Examples: read/write, mmap, epoll_wait.
- **Scheduling (one line):** Linux CFS weights vruntime; priorities via nice; I/O-bound vs CPU-bound trade-off; you can mention "my workers are I/O-bound so more threads than cores is correct there."

### F3. Computer networks

- **OSI (7) vs TCP/IP (4/5):** map them: L7 app (HTTP/DNS) · L4 transport (TCP/UDP, ports) · L3 network (IP, routing) · L2 link (Ethernet/Wi-Fi) · L1 physical. OSI = teaching model; TCP/IP = what runs.
- **TCP vs UDP:** TCP = connection, ordered, reliable (seq/ACK/retransmit), flow control (rwnd), congestion control (cwnd/AIMD), overhead. UDP = datagram, no guarantees, tiny header — used when latency > completeness or app handles reliability: DNS, video, QUIC (UDP-based, has its own reliability + TLS built in).
- **DNS:** recursive resolver ↔ root → TLD → authoritative; record types A/AAAA/CNAME/MX/NS/TXT; caching with TTL at every level; UDP 53 (TCP for large/zone transfer).
- **HTTP vs HTTPS:** HTTPS = HTTP over TLS: handshake (cert chain → verify CA → key exchange e.g. ECDHE → symmetric session key); gives confidentiality + integrity + server authentication; port 443; cert expiry/mismatch errors (ties to your KeyVault/expiry knowledge).
- **TCP 3-way handshake:** SYN (client, seq=x) → SYN-ACK (server, seq=y, ack=x+1) → ACK (ack=y+1). Why not two-way: both sides must confirm each other's ISN and receive capability; prevents old duplicate SYNs creating ghost connections. Teardown: FIN/ACK ×2, TIME_WAIT.
- **IP, port, socket:** IP address = host identity; port = process endpoint (well-known 0–1023, ephemeral); socket = {protocol, src IP, src port, dst IP, dst port} — a 4-tuple connection identity. NAT maps private→public.
- **REST:** resources + verbs (GET safe/idempotent, PUT/DELETE idempotent, POST not) + status codes: 200 OK · 201 Created+Location · 204 No Content · 301/302 redirect · 400 client syntax · 401 unauthenticated · 403 unauthorized · 404 not found · 409 conflict · 429 rate-limited (your gateway!) · 5xx server fault.
- **Authentication vs authorization:** authN = who you are (JWT/session/OAuth); authZ = what you may do (roles/scopes/ACL). JWT = header.payload.signature (HMAC/RSA); stateless but revocation needs a denylist or short TTL — exactly your WebChat logout story.
- **Load balancing:** L4 (routes TCP flows, fast, blind to content) vs L7 (routes by URL/header/cookie, can health-check & stick sessions); reverse proxy = front door doing TLS termination, compression, caching, rate limiting (NGINX); algorithms: round-robin, least-connections, consistent hashing (ties to autoscaling WebChat).

### F4. Software engineering

- **SDLC:** requirements → design → implement → test → deploy → maintain. Models: waterfall (fixed requirements), agile (iterate), and in practice: CI/CD-driven trunk development.
- **Agile/Scrum:** sprints (1–4 weeks), backlog, ceremonies (planning, daily standup, review, retro), user stories, velocity. Point: fast feedback and course-correction, not documentation-free.
- **Git:** feature branch → PR → review → squash-merge to main (trunk-based) vs gitflow (release branches); rebase = linear history, merge = preserve topology; force-push discipline; conventional commits.
- **Testing pyramid:** many unit tests (fast, isolated, mock edges) < fewer integration tests (real DB/queues via containers) < few E2E tests (critical journeys); your 22-test CI suite + Playwright fits here — say where each sits.
- **Debugging method:** reproduce → isolate (bisect inputs/layers) → hypothesize → instrument (logs/breakpoints) → fix → add regression test. Say you use this loop; give the WebChat race as the proof.
- **Code review:** small diffs, intent stated in description, review for correctness/design/tests first, nitpicks last; CI gates before humans.
- **CI/CD:** on every push: build → static checks → tests → artifact → deploy with health checks + rollback (your Hermes pipeline-defect story shows you've lived this).
- **Logging/monitoring:** structured logs w/ levels (error/warn/info/debug); metrics vs logs vs traces; alert on symptoms (SLO burn) not causes; dashboards: latency/errors/saturation/traffic — your Prometheus/Grafana/Loki setup maps 1:1.

---

## G. Puzzles and reasoning [P1 — they grade the approach]

**Universal method (say aloud before any puzzle):** "Let me state my assumptions first…" → list them → derive → sanity-check the extreme cases. If the puzzle is ambiguous, **say what needs clarifying** — that's scored positively.

### G1. Hourglasses (4 min and 7 min → measure 9 min) [CAND]

- **t=0:** start **both** hourglasses.
- **t=4:** 4-glass empties → **flip the 4-glass**.
- **t=7:** 7-glass empties → **flip the 7-glass**.
- **t=8:** 4-glass empties (4 min since its flip) → **flip the 7-glass**, which has been running exactly 1 minute since t=7, so it holds 1 minute of sand in its bottom — flipping puts that 1 minute on top.
- **t=9:** the 7-glass empties → **9 minutes measured.** ✅

**Say aloud:** "9 = 8 + 1; I manufacture the 1-minute leftover from the 7-glass's first flip." Sanity check: every flip is at a moment a glass ran out, so all measurements are legal.

### G2. Three people, one bike, 300 km [CAND]

**Given [CAND]:** bike carries at most 2, moves 60 km/h; walking 15 km/h; everyone must reach 300 km; minimize time for **all**. **Assumptions to state:** the bike never moves without a rider; instantaneous mounting/dismounting; the bike is ridden *back* by a rider to fetch the walker.

**Strategy:** A+B ride from start; at km *p* B dismounts and walks the rest; A rides *back* to meet C at km *q*, picks up C, and rides to the finish. Optimal when **everyone arrives simultaneously** (any idle capacity is waste) and the bike never idles.

**Derive (this is the scored part):**
- C's walking time to q equals A's total time to reach q: `q/15 = p/60 + (p−q)/60` ⟹ `2p = 5q` ⟹ `q = (2/5)p`.
- Simultaneous arrival of B and A/C: `p/60 + (300−p)/15 = p/60 + (p−q)/60 + (300−q)/60`. Substituting `q = 2p/5` gives `21p/5 = 900` ⟹ **p = 1500/7 ≈ 214.3 km**, **q = 600/7 ≈ 85.7 km**.
- **T = p/60 + (300−p)/15 = 25/7 + 40/7 = 65/7 ≈ 9.286 h ≈ 9 h 17 min.**
- Check each person: A: 25/7 + 15/7 (back) + 25/7 = 65/7 ✓ · B: 25/7 + 40/7 = 65/7 ✓ · C: 40/7 + 25/7 = 65/7 ✓.
- **Sanity bounds:** all walk = 20 h (worse); bike ferries pairs without backhaul = way worse. 9.29 h sits sensibly between.

**Say aloud:** "The optimal solution has three properties: the bike is never idle, nobody waits, everyone arrives together. Two unknowns — drop point and pickup point — and two equations: the walker-meets-bike equation and the simultaneous-arrival equation."

### G3. Mislabeled jars [CAND]

Three jars labeled *Apples*, *Oranges*, *Mixed* — **every label is wrong**. Identify all with **one fruit drawn** (eyes closed).
- **Draw from the jar labeled "Mixed".** It can't be mixed (labels lie). If you draw an **apple** → that jar is *Apples*. The jar labeled *Apples* can't be Apples and can't be Mixed (taken) → it's *Oranges*. Remaining jar labeled *Oranges* → *Mixed*. (Symmetric if orange.)
**Why it works:** the "Mixed" label is guaranteed wrong, so that jar's true type is knowable from one draw; the rest falls out by elimination. Generalize: every jar labeled X is not X → pick the maximally-constrained one first.

### G4. Bag of coins [CAND]

10 bags × 10 coins; real coins 10 g; **one bag** has all-fake 9 g coins; **one weighing** on a digital scale finds the fake bag.
- Take **1 coin from bag 1, 2 from bag 2, …, 10 from bag 10** (55 coins) and weigh once. If all real: 550 g. Actual = 550 − (k grams) → **bag k** is fake. (Each fake coin is exactly 1 g light, and bag k contributes exactly k fake coins — the deficit *is* the bag's identity.)
- Generalize if asked: n bags → take i coins from bag i; or fake coins lighter by d grams → deficit/d = k.

### G5. Camel / "Banana Desert" [CAND — reported as a *modified* version; numbers may differ]

**Classic version:** 3000 bananas, camel carries ≤ 1000, eats 1 banana/km, market 1000 km away. Maximize bananas delivered.
- **Method (the scored part — marginal cost per km):** while stock > 2000, you must shuttle **3 loads** → 5 bananas burned per km forward (3 forward trips + 2 returns… net 3 bananas moved costs 5): stock 3000→2000 over **200 km**. While 2000 > stock > 1000: 2 loads → 3 bananas/km: 2000→1000 over **333⅓ km** (at km 533⅓). Last leg: 1 load, 1 banana/km: 1000 bananas at km 533⅓ → travel remaining 466⅔ km → arrive with **533⅓ → 533 bananas**.
- **If the interviewer's numbers differ** (they said "modified" [CAND]): don't recite — **re-derive** with the same segmentation: for each regime, cost/km = 2·(number of shuttles) − 1; find breakpoints where the stock hits 2000, then 1000.
- **Assumptions to state:** bananas can be cached at any km; camel eats only while moving; returns are empty.

### G6. Two more quick ones (30 seconds each)

- **Two ropes, each burns 60 min non-uniformly → measure 45 min:** light rope A at both ends + rope B at one end; A finishes at 30 min → light B's other end; B finishes 15 min later = 45 min. ("Burn rate varies, so only symmetric lighting gives known times.")
- **3 switches downstairs, 1 bulb upstairs, one trip:** switch 1 on for 5 min → off; switch 2 on; go up: bulb on = switch 2; bulb off + warm = switch 1; off + cold = switch 3. (Heat = free extra bit of information — say that phrase.)

---

## H. Business case-study interview [P1 — separate round per EMAIL]

**Confirmed [EMAIL]:** a separate case-study interview; you must install an AI tool on your laptop. The AI tool is *provided infrastructure* — it does **not** mean unrestricted use; **ask the interviewer what's permitted and follow their rules exactly.**

**Reported format [CAND]:** ~30–60 min; a paragraph + PDF of statistics; questions like: pick a city / SWOT / growth strategy / biggest challenges / initial costs / which segment (rural-urban-suburban).

### H1. The 7-step universal framework (memorize — works on every case)

1. **Clarify** the objective (revenue? profitability? visibility? feasibility?) and constraints (budget, timeline, geography). Ask 2–3 sharp questions.
2. **Structure** the answer before diving in: "I'll look at market, unit economics, options, then recommend."
3. **Facts vs assumptions table:** every number from the PDF = *fact*; anything you add = *assumption, stated aloud*.
4. **Quantify:** compute unit economics (below). Check every calculation twice — AI-verified *and* self-verified.
5. **Options** (2–3 real alternatives) with trade-offs — never jump to one answer.
6. **Recommendation** in one sentence, then the why in 3 bullets.
7. **Risks + mitigation + a 30/60/90-day plan.**

### H2. The business toolkit (learn these formulas cold)

- **Revenue = price × volume.** Gross profit = revenue − COGS. **Gross margin = gross profit / revenue.** Net margin after opex.
- **Contribution margin per unit** = price − variable cost per unit → covers fixed costs → break-even units = fixed costs / contribution margin.
- **CAC** (marketing+sales spend ÷ new customers) vs **LTV** = avg revenue/user × gross margin % × avg lifetime (or ARPU ÷ churn). **LTV:CAC ≥ ~3** healthy; **payback months** = CAC / monthly contribution.
- **Churn/retention:** retain 95% → avg lifetime = 1/0.05 = 20 months. Small churn changes dominate LTV.
- **Market sizing:** top-down (TAM→SAM→SOM) **and** bottom-up (# targets × reach % × conversion × price) — doing both is the senior move; they should roughly agree.
- **Pricing levers:** cost-plus (floor), value-based (ceiling), competitive (anchor); freemium/channel discounts = CAC tools.
- **KPI menu:** CAC, LTV, churn, ARPU, DAU/MAU stickiness, utilization (asset businesses), NPS, gross margin, payback.
- **SWOT template:** Strengths/Weaknesses = *internal, today*; Opportunities/Threats = *external, future*. Every quadrant: 3 bullets max, each with evidence from the data.

### H3. Worked case 1 — incumbent vs fast-growing challenger (kids' one-stop solutions) [CAND]

*Scenario [CAND]: established, sales-driven company vs newer, fast-growing competitor excelling in social media, same kids-sector; PDF data provided. Tasks: SWOT both, growth strategy for the incumbent.*

**SWOT — incumbent (compose from the PDF's actual numbers; placeholders show the logic):**
- **S:** brand trust + existing customer base + sales/distribution network + (probably) profitability → funds to invest.
- **W:** weak digital marketing muscle; likely aging acquisition channels; slower product iteration.
- **O:** challenger proves the *demand* exists via social → incumbent can adopt the channel with 10× budget; bundle "one-stop" breadth (cross-sell into existing base = near-zero CAC).
- **T:** rising CAC for the challenger could slow them — but also they may convert younger parents permanently; risk of talent drain to the trendier competitor.

**Growth strategy (pick 4, quantify each):**
1. **Retention & share-of-wallet first:** cheaper than acquisition — cross-sell/upsell bundles to existing customers; target: +X% ARPU.
2. **Build the social engine:** short-form video + parent-community content; KPI = follower→lead conversion; fund properly instead of dabbling.
3. **Referral loop:** parent-network referrals with incentives — kids categories are inherently community-driven; cuts CAC.
4. **Acquire capability, not just ads:** hire/partner for content ops; consider acqui-hiring a creator team.
5. **Defend with breadth:** exclusive "one-stop" bundles the niche challenger can't match (service breadth vs their focus).
**Say the trade-off aloud:** "organic social takes 6–12 months; paid + referral produces revenue this quarter — so run both with different success metrics."

### H4. Worked case 2 — cab-service startup: which of 5 cities? [CAND]

**Build a weighted scorecard (say the weights aloud):**

| Criterion | Weight (say aloud) | What to read from the PDF |
|---|---|---|
| Demand density | 25% | population, urban share, commute volume |
| Ability to pay | 20% | income per capita, existing cab/auto fares |
| Supply feasibility | 15% | driver availability, vehicle costs |
| Competition | 15% | incumbent strength (less = better) |
| Regulation | 10% | permits, price caps |
| Infrastructure | 10% | traffic, roads, airport/metro |
| Cost to launch | 5% | marketing + ops setup |

Score each city 1–5 per row, multiply by weights, pick the top city — **then sanity-check it against the narrative** ("is the story coherent?").

**Rural vs urban vs suburban [CAND asked]:** **start urban** — density shortens pickup times, raises utilization (rides per driver-hour), which is the core of cab unit economics; suburban second wave (commuter flows); rural last (low density kills utilization). Show the chain: *density → utilization → revenue per driver-hour → profitability*.

**Initial cost table (skeleton — plug PDF numbers):** driver onboarding & incentives · fleet/leasing or marketplace model · marketing/CAC (city launch budget) · ops team & support hub · licenses/compliance · tech scaling. State assumptions ("I assume a marketplace model, drivers own cars — else fleet capex multiplies").

**Biggest challenges:** chicken-and-egg supply↔demand (solve with concentrated zone launches, not city-wide spray) · driver incentives burn (watch contribution margin/ride) · regulation & price caps · local incumbents' retaliation · cash collection/trust.

**30/60/90:** 30d — launch 2–3 dense zones, recruit X drivers, baseline KPIs; 60d — utilization & repeat-rate targets, fix pricing from real data; 90d — expand zones or city #2 **only if** unit economics positive (state the threshold: contribution margin per ride > 0 after incentives).

### H5. Worked case 3 — tech business: reliability issues, complaints, rising infra costs

1. **Quantify the pain:** complaints trend + churn/attrition + revenue at risk + SLA credits + engineering time on firefighting. ("If 2% monthly churn traces to reliability, that's X% of ARR — now the fix has a budget.")
2. **Triage root causes:** quick wins (caching, DB indexes, queue backpressure, autoscale limits) vs structural (multi-AZ, capacity planning, SLOs).
3. **Define SLOs + error budget:** e.g. 99.9% → 43 min/month; when budget burns, freeze features and pay down reliability debt.
4. **Cost levers:** rightsize instances, autoscaling windows, storage tiering, spot for batch, kill zombie services. Show cost *per transaction* trend, not absolute spend.
5. **Communicate:** customer-comms plan + status page; trust is a product feature.
6. **30/60/90:** 30d — measure + top-3 quick wins; 60d — error budget governance + structural fix #1; 90d — SLOs published, cost/transaction trending down, complaint driver closed.

### H6. Using the AI tool correctly [EMAIL + ADVICE — this is graded]

1. **Ask what's allowed** ("May I use the tool for calculations? For structuring?"). Then stay inside that line, visibly.
2. **Use AI to:** extract numbers from the PDF into a table; generate alternative framings you might miss; sanity-check arithmetic.
3. **Never let AI:** pick your recommendation. You own the judgment; interviewers watch whether you can think *with* the tool, not through it.
4. **Audit routine (say parts aloud):** "Let me verify that number…" — recompute one AI claim independently; challenge one AI conclusion ("that assumes constant conversion — the data shows seasonality").
5. **Show your structure first,** then use AI — sequencing proves you're the analyst, it's the intern.
6. Useful prompts to prepare: "Extract all quantitative facts from this document as a table with units and source line." / "List 3 alternatives with pros/cons; do not recommend." / "Recompute this; show your work."

---

## I. HR and introductions [P2]

### I.1 The 60-second introduction (say aloud until smooth — facts only, from your resume)

> "I'm Ritik Raj, final-year B.Tech Information Technology at MSIT, CGPA 8.6. I'm a backend-leaning full-stack engineer — my strongest stack is Node.js/TypeScript with PostgreSQL and Redis, plus C++ and 450+ LeetCode problems. I've built three systems I can talk through: a **distributed cron-as-a-service** — RabbitMQ, PostgreSQL, three workers, full observability, load-tested with 10,000 simultaneous tasks; **WebChat**, a 6-service real-time chat platform on Kubernetes with Redis-backed state sync and WebRTC calling; and **Hermes**, an event-sourced multi-tenant workflow engine on AWS serverless. I also interned at S.N. Engineering Works, where I took their B2B machinery marketplace from an empty repo to production in React and Supabase — and shipped an internal ticketing module for them after the internship ended. I enjoy the hard distributed-systems problems, and I'm looking to build production software at scale — which is exactly why ION interests me."

### I.2 The 90-second version: 60s content + one closing sentence
> "…What ties my projects together is wanting to know *why* systems break — I've debugged race conditions that silently dropped chat messages, and found a CI pipeline that silently skipped a critical service on every release. That debugging instinct is what I want to bring to ION's trading-grade software."

### I.3 Tailored answers (all facts from your resume — no invented metrics)

- **Walk me through your resume:** chronological in 4 beats: academics (IT, 8.6) → skills (C++/Java; JS/TS full-stack; AWS/K8s) → projects (one line each, deepest on Cron) → internship + Gen AI Study Jams award (15 GCP badges, GDG MSIT) and Geek Room (organized DSA contests, Letter of Appreciation).
- **Why ION?** *(verify facts on iongroup.com the night before — [ADVICE])* "ION builds mission-critical software for global financial markets — trading, analytics, treasury. That combination — commercial impact plus engineering depth — is exactly what I want: I've built distributed systems at project scale, and I want to learn what production-grade reliability means where money moves in microseconds. I also value that ION's process includes a case round — I like business thinking, not just code."
- **Why software development?** "I like systems that have to *be correct* — scheduling, messaging, state. Development is the only discipline where I can design, build, break, and prove things daily; my internship confirmed it — shipping to real users beat every classroom moment."
- **Strongest project:** Cron (30s intro from §E1) — say why: measurable (load test), end-to-end owned, honest about exactly-once semantics.
- **Difficult technical challenge:** the WebChat race conditions (§E2 story) OR Cron's polling→TTL redesign (§E1). Structure: symptom → investigation → root cause → fix → proof.
- **Weakness (honest + improving):** "Under pressure I historically rushed code — my ION-relevant fix has been deliberate process: I now state assumptions, write the skeleton first, compile early — which is how I stopped panicking on compiler errors. I also speak fast when excited; I consciously slow down when explaining design." *(Deliver calmly — meta-irony kills it.)*
- **Handle pressure?** "The 10k-task load test: things broke — p95 spiked to 160 ms — instead of guessing, I instrumented (Prometheus/Grafana), isolated the burst to TTL expiry, tuned, and got to 76 ms with zero loss. Pressure + measurement = my default."
- **Teamwork:** Geek Room MSIT — organized DSA/coding competitions with a team; internship — built from requirement-gathering to production with the founder's feedback loop; Letter of Appreciation for consistent contribution.
- **Why should we hire you?** "Three reasons: I ship end-to-end (empty repo → production twice: EngineCore Pro and my scheduler); I debug systematically (two silent races found and fixed); and I'm honest about trade-offs — I'll tell you when something is at-least-once, not exactly-once."
- **Flexible about responsibilities/location?** "Yes — role matters less than the work quality; I'm flexible on location across ION's India offices, including relocation."
- **What did you learn from the internship?** "Scope small, ship, iterate: the quote configurator replaced email chaos only because I kept it narrow and shipped fast; also that users (the sales team) define 'done', not the ticket; and ownership — I shipped a ticketing module after my internship ended because the system mattered to them."

### I.4 Questions to ask them (pick 2)

1. "What does a first-year software developer at ION actually work on — product features, production support, or tooling?"
2. "How does ION's engineering culture differ between its India offices?"
3. "What separates the developers who grow fastest at ION in the first two years?"

---

## J. Rapid revision [day-of reference]

### J1. C++/OOP cheat sheet (P1)

| Concept | One-liner |
|---|---|
| Encapsulation | private data + controlled access; protects invariants |
| Abstraction | expose what, hide how |
| Overloading | same name, **different signature**, compile-time |
| Overriding | same **virtual** signature, base→derived, runtime (vtable) |
| Virtual destructor | base needs it if deleted via base* — else derived cleanup skipped (UB/leak) |
| Never call base dtor manually | compiler auto-chains after derived dtor body; manual call = destroyed twice |
| Object slicing | derived-by-value stored in base container loses derived state; use ptrs/smart ptrs |
| Init list | only way to init base/const/ref members; order = **declaration** order |
| Rule of Three/Five/Zero | custom dtor/copy/assign together — or use value types & smart pointers (Zero) |
| unique_ptr / shared_ptr / weak_ptr | sole owner / refcounted (cycles→weak) / observer |
| Abstract class | ≥1 pure virtual (`=0`); can't instantiate; may still hold data + implemented methods |
| Composition > inheritance | has-a default; is-a only with true substitutability (LSP) |
| `override` / `final` | compiler-checked override / forbid further override |

### J2. DSA complexity + pattern cheat sheet (P1)

| Pattern | Use when | Complexity |
|---|---|---|
| Two variables (max/second-max) | static or streaming extremes | O(n)/O(1) |
| Hash map | lookup/complement (Two Sum), counting | avg O(1) |
| Ordered set/map | order + dynamic insert/erase; k-th | O(log n) |
| Min-heap size k | top-k streaming | O(n log k) |
| Kadane | max-sum subarray | O(n)/O(1) |
| Prefix product of extremes | max product of 3 | O(n) or sort O(n log n) |
| Run-length two pointers | string compression | O(n) |
| Stack | matching/nesting (parentheses), monotonic | O(n) |
| Floyd tortoise-hare | cycle detect/start | O(n)/O(1) |
| BFS/DFS | components, flood fill | O(V+E) |
| Union-find | dynamic connectivity | ~O(α) amortized |
| Binary search / lower_bound | sorted space; first/last occurrence | O(log n) |
| Trie | prefix search (phone directory!) | O(L) per op |

### J3. LLD checklist (P1 — run it in this order on any design prompt)

1. Clarify: scope? what changes at runtime? edge cases?
2. Nouns→classes, verbs→methods; one responsibility per class.
3. Relationships: is-a (rare) vs has-a (default). Controller owns parts.
4. Data structures with justified complexities.
5. Trace ONE request end-to-end aloud.
6. Identity: stable IDs/keys, never positional indices.
7. Concurrency/state: who is the single writer?
8. Failure mode + fail-safe behavior.
9. "How does the design change if…" — have one extension ready per class.
10. Complexity summary sentence at the end.

### J4. Project revision checklists (P1)

**Cron — numbers I must quote correctly:** 10,000 tasks same instant · RabbitMQ peak 8,562 msgs · 74 msg/s ingest · ~1,600 msg/s TTL-expiry burst · drained ~7 min 46 s at 50–61 tasks/s · p95 160 ms→76 ms · zero DLQ / zero loss / zero crashes · 3 worker replicas.
**Cron — one-liners:** TTL+DLX replaced DB polling · reconciliation + execution IDs = exactly-once *effect* · JWT + rate limiting at gateway · Prometheus/Grafana/Loki/Alertmanager · EC2 + Docker Compose + GitHub Actions.

**WebChat — one-liners:** 6 services · Socket.io + Redis state sync = horizontal autoscale without dropped connections · async persistence pipeline (RabbitMQ) off the request path · 2 races fixed (dropping + duplicating) · version-controlled migrations replaced auto-migrate · 9+ deployment defects fixed · secret validation, token revocation, rate limiting, XSS protection · 22-test CI gate · S3 + WebRTC calling.

**Hermes — one-liners:** event sourcing + CQRS → replayable, auditable history · transactional outbox kills dual-write · saga compensation + automatic rollback · time-travel replay from checkpoints · Java 25 + Spring Boot → Lambda/DynamoDB/EventBridge/Step Functions/API Gateway, Terraform · migrated mid-flight, deleted 100+ files · fixed pipeline skipping a critical service.

**NetSight — ⚠️ fill from your repo before the interview** (capture path, flow definition, ring buffers/memory pools, epoll, eBPF role, metrics, numbers).

### J5. Core CS rapid-fire (P2 — 20 Q→A)

1. Primary vs foreign key? → PK uniquely identifies a row, NOT NULL; FK references another table's key, enforces integrity.
2. Why B+tree over binary tree for DB indexes? → low, block-friendly, leaf-linked range scans.
3. DELETE vs TRUNCATE vs DROP? → rows (where-able, logged) / all rows, fast / table gone.
4. ACID? → atomicity, consistency, isolation, durability.
5. Process vs thread? → isolated address space vs shared-heap execution unit with own stack.
6. Mutex vs semaphore? → ownership lock vs counting signal.
7. Deadlock's 4 conditions? → mutual exclusion, hold-and-wait, no preemption, circular wait.
8. Context switch cost? → register save/restore + cache/TLB pollution (indirect dominates).
9. TCP vs UDP? → ordered/reliable/congestion-controlled vs fast/fire-and-forget (DNS, video, QUIC).
10. 3-way handshake? → SYN → SYN-ACK → ACK; both sides sync ISNs; 2-way can't confirm both directions.
11. 401 vs 403? → not authenticated vs not allowed.
12. 429? → rate limited (your gateway returns this).
13. DNS record types? → A/AAAA, CNAME, MX, NS, TXT.
14. What does HTTPS add? → TLS: confidentiality, integrity, server authentication.
15. REST idempotency? → GET/PUT/DELETE idempotent; POST not.
16. Load balancer L4 vs L7? → flow routing vs content-aware routing.
17. Unit vs integration vs E2E? → isolated logic / real collaborators / user journeys; pyramid ratios.
18. CI/CD? → every push builds+tests+ships with health checks and rollback.
19. Git rebase vs merge? → linear history vs preserved topology.
20. Agile point? → iterate in sprints; feedback beats prediction.

### J6. Case-study framework card (P1)

**7 steps:** Clarify → Structure → Facts/Assumptions table → Quantify → Options → Recommend (1 sentence + 3 bullets) → Risks + 30/60/90.
**Formulas:** revenue = price × volume · contribution = price − variable cost · break-even = fixed / contribution · LTV = ARPU × margin × lifetime · LTV:CAC ≥ 3 · payback = CAC / monthly contribution.
**Scorecard:** criteria × weights × 1–5 scores → weighted total → narrative sanity-check.
**AI discipline:** ask the rules → AI extracts/structures → you verify one number → you challenge one conclusion → your recommendation, your words.

### J7. Final interview checklist (P1)

- [ ] 60s intro smooth; 2 questions ready to ask.
- [ ] Vehicle/Car code typeable in ≤10 min with narration.
- [ ] One-liners: overload vs override · virtual dtor · "never call base dtor manually".
- [ ] 6 core snippets: second-largest, Kadane, max-product-3, compress, reverse LL, detect cycle (+ trie if time).
- [ ] Traffic-light 60s spoken design.
- [ ] Puzzles: hourglass steps, bike-puzzle method (2p=5q → 65/7 h), jars (draw from "Mixed"), coins (i coins from bag i), bananas (segment method).
- [ ] Case: 7-step framework + LTV/CAC formulas + one scorecard.
- [ ] NetSight facts filled in.
- [ ] Laptop: editor + compiler verified; AI tool installed (case round); charged; water.
- [ ] Sleep. A rested brain beats one more LeetCode problem.

---

*End of guide. Sources: your scheduling email [EMAIL]; six ION candidate reports [CAND] (GeeksforGeeks 2024, LeetCode discuss Nov 2025, Medium Aug 2023, LinkedIn SDE report, Reddit Technical Analyst post, AmbitionBox interview); your resume and stated background. Anything marked [ADVICE] is mentor judgment, not ION fact.*

---

# K. Self-drill exercise set [P1]

> **Protocol for every exercise:** (1) attempt in a real editor / on paper, (2) **speak the explanation aloud** while working, (3) compile/run, (4) only then scroll to Section L and self-grade. Mark each ✅ when done. No peeking — the graded skill is *producing* under pressure, not recognizing.

### K1. C++ / OOP drills (10–12 min each)

**K1.1 — The confirmed ION drill [CAND].** Model `Vehicle` + `Car` (+ one more derived type). Must show: private state · compile-time polymorphism (overloading) · runtime polymorphism (overriding through a base pointer) · virtual destructor · print destructor order when deleting through a base pointer. Then write the 5-bullet narration you'd say while typing (§B1 script is your model answer — write yours before comparing).

**K1.2 — Predict the output.** Two variants. Write the exact output of each, then check L1.2.

```cpp
#include <bits/stdc++.h>
using namespace std;
struct A { A(){ cout << "A+"; }  ~A(){ cout << "A-"; } };
struct B : A { B(){ cout << "B+"; } ~B(){ cout << "B-"; } };

int main() {
    { B b; }                 cout << "|";
    A* p = new B; delete p;  cout << "\n";
}
```
(a) As written (non-virtual destructor). (b) Same code with `virtual ~A()`. Explain the difference in one sentence.

**K1.3 — Bug hunt.** This code has 5 deliberate bugs. Find, fix, and justify each (L1.3):

```cpp
#include <bits/stdc++.h>
using namespace std;

class Account {
public:
    Account(string owner) : owner_(owner) {}
    ~Account() { ~Account(); }                       // ?
    string& owner() { return owner_; }               // ?
    double balance() { return balance_; }            // ?
private:
    string owner_; double balance_ = 0;
};

class Savings : public Account {
public:
    Savings(string owner, double rate) { owner_ = owner; rate_ = rate; }   // ?
    double withBonus(double amt) { return amt * rate_; }
    double withBonus(double amt) const { return amt * (rate_ + 0.01); }
private:
    double rate_;
};

int main() {
    vector<Account> accounts;                        // ?
    accounts.push_back(Savings("rita", 0.05));
    Savings s("raj", 0.04);
    cout << s.withBonus(100.0) << "\n";
}
```

**K1.4 — Abstract classes.** Build `Shape` (pure virtual `area()`, `perimeter()`), derive `Circle`, `Square`, `Rectangle`. In `main`: store 3 shapes in a `vector<unique_ptr<Shape>>`, print each area, print total area, and sort the vector by area. Add a non-virtual `describe()` in the base that *calls* `area()` — explain why that still dispatches correctly.

**K1.5 — const correctness.** Fix this class until it compiles under a const instance `const Stats s;`, explaining each change (L1.5):

```cpp
class Stats {
public:
    Stats(vector<int> v) : data_(v) {}
    double mean() {                                  // should work on const objects
        double s = 0; for (int x : data_) s += x; return s / data_.size();
    }
    int& at(size_t i) { return data_[i]; }           // read-only users shouldn't be able to modify
    void print(std::ostream& os) { os << mean(); }   // should not modify *this
private:
    vector<int> data_;
};
```

**K1.6 — RAII / smart pointers.** (a) Rewrite this raw-pointer class so it needs **no** destructor, copy ctor, or assignment (Rule of Zero). (b) Then answer: parent holds `shared_ptr<Child>`, child holds `shared_ptr<Parent>` — what breaks and which pointer fixes it? (L1.6)

```cpp
class Buffer {
public:
    Buffer(size_t n) : data_(new int[n]), n_(n) {}
    ~Buffer() { delete[] data_; }
private:
    int* data_; size_t n_;
};
```

**K1.7 — TrafficLight/Road mini-hierarchy.** Write (don't over-engineer): `enum class LightState { RED, AMBER, GREEN }`; `Road` with a stable `roadId`, `name`, and its `LightState`; `Intersection` owning a `map<string, Road>` plus a `vector<string>` phase order, with `advance()` moving the current phase's road GREEN→AMBER→RED and the next road to GREEN, with an all-red safety tick between. Trace 2 cycles by hand and write the printed sequence.

**K1.8 — Overload vs override vs hiding.** Classify each call as overload / override / name-hiding, and predict the printed output (L1.8):

```cpp
struct B {
    void f(int)   { cout << "B::f(int)\n"; }
    virtual void g(int) { cout << "B::g(int)\n"; }
};
struct D : B {
    void f(double) { cout << "D::f(double)\n"; }      // (1) what is this?
    void g(int) override { cout << "D::g(int)\n"; }   // (2) and this?
};
int main() {
    D d; d.f(3); d.f(3.0); d.g(3);
    B* p = &d; p->f(3); p->g(3);
}
```

### K2. DSA drills (15 min each; say your approach aloud before coding)

Attempt these **blind**, then compare with L2. Constraints to state before coding: input size, value range, duplicates, required complexity.

- **K2.1 Second-largest distinct element** — (a) static array O(n)/O(1); (b) *modification:* values arrive as a stream, insert-only; (c) *modification:* inserts **and** erases between queries; (d) *modification:* k-th largest. Which structure for each?
- **K2.2 Kadane** — max-sum subarray; return the **start and end indices** too; prove all-negative arrays work.
- **K2.3 Max product of three elements** — negatives allowed; (a) sort version; (b) O(n) no-sort version tracking 3 maxima + 2 minima.
- **K2.4 String compression** — `aaabbccccd → a3b2c4d` (count omitted when 1); *modifications:* (a) always print the count; (b) in-place, O(1) extra space; (c) counts ≥ 10.
- **K2.5 Implement a singly linked list** — `pushFront`, `pushBack`, `removeFirst(value)`, `print`, plus a correct destructor. No memory leaks.
- **K2.6 Reverse a linked list** — iterative **and** recursive versions; walk through one iteration aloud.
- **K2.7 Cycle detection** — `hasCycle` (Floyd) + `cycleStart` (return the entry node) + the math for why the second phase works.
- **K2.8 Merge two sorted linked lists** — O(n+m), no new nodes (rewire only).
- **K2.9 Valid parentheses** — then *modification:* return the **minimum number of insertions** to make it valid.
- **K2.10 Two Sum** — hash version; *modification:* input is sorted (two pointers, O(1) space); *modification:* return **all** unique pairs.
- **K2.11 Binary search** — standard; then first/last occurrence with `lower_bound`/`upper_bound`; *(optional P3)* search in a rotated sorted array.
- **K2.12 Connected components** — count components (BFS); *modification:* same with union-find; *modification:* edges arrive one at a time, report count after each.
- **K2.13 Top-K frequent elements** — heap version O(n log k); *(bonus)* O(n) bucket-sort version.
- **K2.14 Trie phone directory** — `add(name, number)`, `exactSearch`, `searchByPrefix` returning all matches; *modification:* autocomplete — 3 best suggestions at each node in O(prefix) time.
- **K2.15 Job portal** — `postJob(jobId, role, salary)`, `postUser(userId, interestedRole, salaryExpectation)`, `fetchJobs(userId, k)` (role match + salary floor + top-k by salary); *modifications:* job deletion after posting (lazy invalidation); write the SQL + the index you'd create.

### K3. LLD drills (30–40 min each; design aloud, then code the core)

- **K3.1 Traffic light system** — from §D1 spec: stable road IDs under merging roads · lanes · conflict-free phases · pedestrians · emergency preemption · fail-safe. Write the class skeleton (L3.1 gives the reference). Then the change-drills: (a) two roads merge — what changes? (b) add a dedicated left-turn lane with its own arrow; (c) emergency vehicle request while pedestrian phase is active; (d) make timings sensor-driven.
- **K3.2 Snake & Ladder with a vanishing snake** — full game, 2 players, snake disappears after its first bite. Change-drills: (a) rolling a 6 grants an extra turn; (b) exact finish required to win; (c) support undo — what structure makes this trivial? (Think of your Hermes event log.)
- **K3.3 Leaderboard** — `addScore(id, pts)`, `top(k)`, `reset(id)`; change-drills: (a) ties broken by who reached the score first; (b) `top(k)` called every second by thousands of clients — what changes?
- **K3.4 Phone directory (LLD shape)** — `Directory` class wrapping a trie; change-drills: case-insensitive matching; `removeContact`; top-3 autocomplete per keystroke.
- **K3.5 Parking lot** — vehicles (reuse your K1.1 Vehicle hierarchy!), spot types, `park`, `exit`, fee strategy; change-drills: nearest-first spot allocation; EV charging spots; cars arriving from **3 entrances simultaneously** (what breaks?).

### K4. Puzzle drills (paper, 5–8 min each; state assumptions aloud first)

- **K4.1** Hourglasses 4 min + 7 min → measure exactly **9 minutes**. List every flip with its timestamp.
- **K4.2** Three people, 300 km, one bike (max 2 riders, 60 km/h), walking 15 km/h. Minimum time for **all** to arrive? Derive, don't guess. *Modification:* what if the bike's return can be ridden by **no one** (it can't — state why that's impossible)?
- **K4.3** Three jars labeled Apples / Oranges / Mixed — all labels wrong. One fruit drawn (eyes closed) identifies all three. Which jar do you draw from, and why?
- **K4.4** 10 bags × 10 coins, one bag all-fake (9 g vs 10 g). **One** weighing finds the fake bag. How? *Modification:* fake coins are lighter by an unknown *d* grams — still one weighing?
- **K4.5 Banana-desert variant (numbers deliberately changed — re-derive, don't recite):** 2400 bananas, camel carries ≤ 800, eats 1 banana/km, market 600 km away. Maximum bananas deliverable?
- **K4.6 (bonus)** (a) Two ropes, each burns 60 min non-uniformly — measure 45 min. (b) 3 switches downstairs, one bulb upstairs, one trip upstairs — identify each switch.

### K5. Case-study drills (timed — use a real timer; then self-grade with the rubric)

- **K5.1 (20 min)** Incumbent kids-solutions company vs fast-growing social-media-native challenger [CAND scenario]. Write: SWOT for the incumbent (3 bullets/quadrant, each tied to a data point you'd request) + a 4-point growth strategy with **one KPI per point**.
- **K5.2 (25 min)** Cab startup, 5 hypothetical cities. Build the weighted scorecard (choose your weights, say them), pick the city, list initial costs (table), name the top-3 launch challenges, draft the 30/60/90.
- **K5.3 (20 min)** Tech business: rising customer complaints + rising infra costs. Quantify the pain, separate quick wins from structural fixes, define an SLO + error budget, list 5 cost levers, produce the 30/60/90.

**Self-grade rubric (all three):** Did I clarify the objective? Did I separate facts from assumptions? Did I compute at least one number? Did I give a one-sentence recommendation with 3 reasons? Did I end with risks + a plan?

### K6. Spoken / HR drills

- **K6.1** Write your 60-second intro as 8 bullet points. Record it on your phone. Listen once: cut filler ("basically", "actually"), verify every claim is resume-real.
- **K6.2** Strongest project (pick Cron) in 2 minutes using the 5-part structure (problem → architecture → data flow → hardest problem → trade-off). Record once.
- **K6.3** Write your own one-liners for: weakness · pressure · teamwork · why should we hire you. No clichés; each must contain one **real, specific** story beat.

### K7. Rapid-fire quiz (30 questions — answer in ≤ 20 seconds each, out loud)

1. The 4 OOP pillars — one line each.
2. Overloading vs overriding — one line each.
3. Why does a polymorphic base class need a virtual destructor?
4. Why must a derived destructor never call its base destructor manually?
5. What is object slicing, and how do you avoid it?
6. Name three things that can **only** be initialized in a member-init list.
7. `unique_ptr` vs `shared_ptr` vs `weak_ptr` — when each?
8. `struct` vs `class` — the only difference?
9. Can a constructor be virtual? What's the alternative idiom?
10. `map` vs `unordered_map` — complexities and when each?
11. `priority_queue` push/pop complexity? Building a heap from n elements?
12. Arrays vs linked lists — insertion vs search trade-off?
13. Which DS for a phone directory searched by **name prefix**? Why not `map`?
14. Kadane — what is the per-index decision?
15. Linked-list cycle detection — algorithm + complexities?
16. Count connected components — two approaches + complexities?
17. ACID — the four letters?
18. Which isolation level blocks dirty reads but still allows non-repeatable reads?
19. Process vs thread — the single key difference?
20. Mutex vs semaphore?
21. TCP vs UDP — name two UDP use cases and why.
22. Why does TCP need a 3-way handshake, not 2?
23. 401 vs 403 vs 429 — what does each mean?
24. Authentication vs authorization? How do you revoke a JWT?
25. LTV formula + the LTV:CAC rule of thumb?
26. Break-even units formula?
27. In SWOT, which two quadrants are **internal**?
28. Hourglass 4/7 → 9: what do you flip at t = 8, and why does that work?
29. Bike puzzle: write the two setup equations (drop point p, pickup point q).
30. Your Cron project claims "exactly once" — why must that claim be worded carefully?

---

# L. Worked solutions & explanations

> Read **only after attempting**. Each solution: the answer, why it works, what to say aloud, and the mistake to avoid.

## L1. C++ / OOP drill solutions

### L1.1 — The confirmed ION drill: checkpoints + the destructor extension

Full model code is §B1 — verify yours against these checkpoints (each is worth marks in a real interview):

- [ ] Base class has **private** data + a public accessor (encapsulation named aloud).
- [ ] One **overload** present (e.g. `farePerKm(double surge)`) — and you said "compile-time".
- [ ] Base declares a **virtual** function; derived **overrides** it with the `override` keyword.
- [ ] Polymorphism exercised through a **base pointer or reference** (container of `unique_ptr<Vehicle>` or `Vehicle*`), and you said "runtime dispatch via vtable".
- [ ] `virtual ~Vehicle()` — and you explained *why* (deleting through base* must destroy the derived part first).
- [ ] Members initialized via the **init list** (not assignment in the body).

**The destructor follow-up — the exact trap from the [CAND] report:**

```cpp
~Car()     { cout << "~Car\n"; }      // ✅ your own cleanup only
~Vehicle() { cout << "~Vehicle\n"; }   // ✅ already virtual

// ~Car() { ~Vehicle(); ... }         // ❌ NEVER — compiler already chains ~Vehicle()
                                       //    after the body → base destroyed TWICE → UB/double free
```

Expected order for a `Car` destroyed through a `Vehicle*`: **`~Car` then `~Vehicle`** (reverse of construction). Spoken one-liner: *"The destructor chain is compiler-managed — derived body first, then the base is destroyed automatically. I only mark the base destructor virtual so the chain starts from the dynamic type."*

### L1.2 — Predict the output (construction/destruction order)

**(a) As written (non-virtual `~A`):**

```
A+B+B-A-|A+B+A-
```
- `{ B b; }`: constructing B = base first → `A+`, `B+`; scope ends → derived first → `B-`, `A-`. So `A+B+B-A-`.
- `A* p = new B; delete p;`: heap object constructs `A+`,`B+`. **`delete p` sees only `A*`** and `~A` is non-virtual → calls `~A` **statically** → prints only `A-`. `B`'s destructor never runs → formally UB, leaked derived resources.
- Sentence: *"Destruction order is always reverse of construction, but `delete` through a base pointer only destroys the base part unless the base destructor is virtual."*

**(b) With `virtual ~A()`:**

```
A+B+B-A-|A+B+B-A-
```
`delete p` now dispatches through the vptr to `~B`, which runs first and then auto-chains `~A`. Same visible order as the stack case — that symmetry is the point.

### L1.3 — Bug hunt: the 5 bugs

1. **`~Account() { ~Account(); }`** — a destructor **calling itself** → infinite recursion/double-destruction → UB. A destructor contains *only your cleanup*; it is never called explicitly. Fix: `~Account() {}` or `= default`.
2. **`string& owner()`** — returns a non-const reference to a private member → destroys encapsulation (callers can mutate hidden state). Fix: `const string& owner() const`.
3. **`double balance()`** — not a **const** member function, so it can't be called on `const Account&`. Fix: `double balance() const`. (Rule: getters are always `const`.)
4. **`Savings` constructor assigns `owner_` in its body** — `owner_` is a **private base member**: not accessible in the derived class at all (compile error), and even if it were, assignment-after-construction wastes work. Fix: `Savings(string o, double r) : Account(move(o)), rate_(r) {}` — base via init list.
5. **`vector<Account> accounts; accounts.push_back(Savings(...))`** — **object slicing**: only the `Account` subobject of the `Savings` temporary is copied; `rate_` is silently dropped. Fix: `vector<unique_ptr<Account>>`.
   - Bonus (6th, subtle): `s.withBonus(100.0)` with **two** overloads — `withBonus(double)` (non-const) and `withBonus(double) const` — on a non-const `s`, **overload resolution prefers the non-const one**; the const overload only serves const objects. This is legal overload design (mirrors `std::vector::operator[]`), but know that both exist and when each is chosen.

### L1.4 — Abstract class solution

```cpp
#include <bits/stdc++.h>
using namespace std;

class Shape {
public:
    virtual double area() const = 0;                 // pure virtual → abstract
    virtual double perimeter() const = 0;
    virtual ~Shape() = default;                      // polymorphic base → virtual dtor
    void describe() const {                          // NON-virtual, calls a virtual fn
        cout << "area=" << area() << "\n";           // dispatches to the DERIVED area()
    }
private:                                             // abstract classes may still hold state
    int id_ = ++counter_;
    static inline int counter_ = 0;
};

class Circle : public Shape {
public:
    explicit Circle(double r) : r_(r) {}
    double area() const override { return 3.14159 * r_ * r_; }
    double perimeter() const override { return 2 * 3.14159 * r_; }
private:
    double r_;
};
class Square : public Shape {
public:
    explicit Square(double s) : s_(s) {}
    double area() const override { return s_ * s_; }
    double perimeter() const override { return 4 * s_; }
private:
    double s_;
};
class Rectangle : public Shape {
public:
    Rectangle(double w, double h) : w_(w), h_(h) {}
    double area() const override { return w_ * h_; }
    double perimeter() const override { return 2 * (w_ + h_); }
private:
    double w_, h_;
};

int main() {
    // Shape s;                    // ❌ would not compile: abstract
    vector<unique_ptr<Shape>> shapes;
    shapes.push_back(make_unique<Circle>(2.0));      // ≈ 12.57
    shapes.push_back(make_unique<Square>(3.0));      // 9.0
    shapes.push_back(make_unique<Rectangle>(2, 5));  // 10.0

    double total = 0;
    for (const auto& s : shapes) { s->describe(); total += s->area(); }
    cout << "total = " << total << "\n";             // ≈ 31.57

    sort(shapes.begin(), shapes.end(),              // sort unique_ptrs by pointee area
         [](const unique_ptr<Shape>& a, const unique_ptr<Shape>& b) {
             return a->area() < b->area();
         });
    for (const auto& s : shapes) s->describe();
}
```

**Why `describe()` works although it is non-virtual:** the *function chosen* is `Shape::describe` (static binding), but *inside* it, the call `area()` is made **through `this`, which is a `Shape*`** — and `area` is virtual → dynamic dispatch to the real type. Key line to say: *"virtuality is decided per-call-site, not per-function; `describe` binds statically, the `area()` call inside it binds dynamically."* Also note `sort` on `unique_ptr`s needs the lambda — you can't copy shapes.

### L1.5 — const-correctness fix

```cpp
class Stats {
public:
    explicit Stats(vector<int> v) : data_(move(v)) {}
    double mean() const {                            // ① const: callable on const objects
        double s = 0; for (int x : data_) s += x; return s / data_.size();
    }
    int& at(size_t i) { return data_[i]; }           // ② mutating accessor stays non-const …
    int at(size_t i) const { return data_[i]; }      //    … add the const overload for read-only use
    void print(ostream& os) const { os << mean(); }  // ③ const: doesn't modify *this
private:
    vector<int> data_;
};
```
Explanation bullets: ① `mean` doesn't modify → mark `const`, now works on `const Stats`. ② Returning `int&` from a const method would break the const contract; the pair (non-const returns ref, const returns value) mirrors `vector::operator[]`. ③ `print` only reads → `const`. Also fixed: pass `v` by value + `move` (avoids surprising copies) and `explicit` on the one-arg ctor. Spoken: *"const is a contract — every method that doesn't mutate gets marked const, which is what lets const objects exist at all."*

### L1.6 — RAII rewrite + the shared_ptr cycle

**(a) Rule of Zero rewrite** — use a container that owns:

```cpp
class Buffer {
public:
    explicit Buffer(size_t n) : data_(n) {}          // vector<int> sized n
    int& at(size_t i) { return data_[i]; }
    size_t size() const { return data_.size(); }
private:
    vector<int> data_;                               // owns the memory; cleans up itself
};
```
No destructor/copy/assignment needed — `vector` already implements all three correctly. Alternative: `unique_ptr<int[]> data_ = make_unique<int[]>(n);`.

**(b) The cycle:** `Parent → shared_ptr<Child>` and `Child → shared_ptr<Parent>` → each holds the other alive → refcounts never reach 0 → **leak** (destructors never run). Fix: make the child's back-pointer a **`weak_ptr<Parent>`** — non-owning; use `parent.lock()` to access, which returns null if the parent died. Rule: *ownership down the hierarchy, observation up the hierarchy.*

### L1.7 — TrafficLight/Road mini-hierarchy (reference solution)

```cpp
#include <bits/stdc++.h>
using namespace std;

enum class LightState { RED, AMBER, GREEN };
const char* name(LightState s) {
    switch (s) { case LightState::GREEN: return "GREEN"; case LightState::AMBER: return "AMBER"; default: return "RED"; }
}

class Road {                                          // stable identity: roadId is a key, not an index
public:
    Road(string roadId, string nm) : roadId_(move(roadId)), name_(move(nm)) {}
    const string& roadId() const { return roadId_; }
    LightState state() const { return state_; }
    void setState(LightState s) { state_ = s; }
private:
    string roadId_, name_;
    LightState state_ = LightState::RED;
};

class Intersection {
public:
    Intersection(vector<string> phaseOrder, int amberTicks = 1, int allRedTicks = 1)
        : phaseOrder_(move(phaseOrder)), amber_(amberTicks), allRed_(allRedTicks) {}

    void addRoad(Road r) {
        if (roads_.count(r.roadId())) throw invalid_argument("duplicate roadId");
        roads_.emplace(r.roadId(), move(r));
    }
    void advance() {                                  // one scheduler tick
        switch (phase_) {
            case Phase::GREEN:
                if (--greenLeft_ > 0) return;
                setPhaseRoads(LightState::AMBER); left_ = amber_; phase_ = Phase::AMBER;
                break;
            case Phase::AMBER:
                if (--left_ > 0) return;
                setAll(LightState::RED); left_ = allRed_; phase_ = Phase::ALL_RED;
                break;
            case Phase::ALL_RED:
                if (--left_ > 0) return;
                current_ = (current_ + 1) % phaseOrder_.size();   // next phase
                setPhaseRoads(LightState::GREEN);
                greenLeft_ = 5; left_ = greenLeft_; phase_ = Phase::GREEN;
                break;
        }
    }
    void dump() const {
        for (const auto& [id, r] : roads_) cout << id << ":" << name(r.state()) << " ";
        cout << "\n";
    }
private:
    enum class Phase { GREEN, AMBER, ALL_RED };
    void setAll(LightState s) { for (auto& [id, r] : roads_) r.setState(s); }
    void setPhaseRoads(LightState s) {
        for (const string& id : phaseOrder_) at(current_)->setState(s);
    }
    Road* at(size_t i) { return &roads_.at(phaseOrder_.at(i)); }

    unordered_map<string, Road> roads_;   // ID → road (stable under add/remove)
    vector<string> phaseOrder_;           // which roads share a green — a conflict-free set
    size_t current_ = 0;
    Phase phase_ = Phase::ALL_RED; int left_ = 0, greenLeft_ = 5;
    int amber_, allRed_;
};

int main() {
    Intersection ix({"N", "E", "S", "W"});
    ix.addRoad(Road("N", "North Ave")); ix.addRoad(Road("E", "East Rd"));
    ix.addRoad(Road("S", "South Ave")); ix.addRoad(Road("W", "West Rd"));
    ix.advance();                          // start first green
    for (int i = 0; i < 14; ++i) { ix.dump(); ix.advance(); }
}
```
Two-cycle trace (green=5, amber=1, all-red=1): N GREEN ×5 → ALL AMBER ×1 → ALL RED ×1 → E GREEN ×5 → … — each transition passes through the all-red clearance, so **no two conflicting roads are ever green**: safety is in the sequencing, not in runtime checks. Note the `map<string, Road>` keyed by stable ID (K3.1's merging-roads question).

### L1.8 — Overload vs override vs hiding — classification + output

| Call | Classification | Why |
|---|---|---|
| `D::f(double)` | **name hiding** (an *overload of `f` in D's scope*) | different parameter type; hides `B::f(int)` when called through `D` |
| `D::g(int) override` | **overriding** | same signature, base is `virtual` |
| `d.f(3)` / `d.f(3.0)` | compile-time overloads within `D` | `3`→int→ promoted to `double` → `D::f(double)`; `3.0` → `D::f(double)` |
| `p->f(3)` | static binding in **B's scope** | `p` is `B*`; `f` non-virtual; D's `f` invisible through B* |
| `p->g(3)` | runtime dispatch | virtual → `D::g(int)` runs |

**Output:**
```
D::f(double)      // d.f(3):  int → double, name hiding means only D::f visible via d
D::f(double)      // d.f(3.0)
D::g(int)         // d.g(3):  override
B::f(int)         // p->f(3): static scope B, non-virtual
D::g(int)         // p->g(3): virtual dispatch
```
The two `p->` lines are the interview's favorite trap: **non-virtual calls follow the pointer's static type; virtual calls follow the object's dynamic type.**

## L2. DSA drill solutions

> Compare structure, not just code. For every problem the graded sequence is: **clarify → brute force in one sentence → optimized → complexity → edge cases**. Full treatments for several of these are in §C; this section adds the answers *plus* the modification drills.

### L2.1 Second-largest distinct

**(a) Static, O(n)/O(1)** — §C1 code. Key edge cases: `<2` elements, all-equal, duplicates of max. Return sentinel (-1) when no second exists.
**(b) Insert-only stream:** the *same two variables* work — state survives across elements; no structure needed. Say: "streaming insert-only changes nothing; I keep running max and runner-up."
**(c) Inserts AND erases between queries [CAND's twist]:** ordered `set<int>` (or `multiset` if duplicates count) — insert/erase O(log n), answer = second-from-top. The two-variable state can't survive deletions. Lazy-deletion heap is an acceptable alternative; say why set is cleaner.
**(d) k-th largest:** min-heap of size k (O(n log k)) or `nth_element` (avg O(n), unordered result) or ordered set (O(log n) per op, ordered output).

### L2.2 Kadane with indices

§C2 code already returns `st`/`en`. Walkthrough to say: *"cur<0 means the prefix can only hurt everything after it — restart at i. best tracks the max of all cur."* Edge cases: single element; all-negative (returns the max element — test `{-3,-1,-5}` → -1, indices 1..1); overflow (use `long long` for the sum).

### L2.3 Max product of three

Sort version in §C3. **O(n) no-sort version:**

```cpp
long long maxProduct3(const vector<int>& a) {
    long long max1=LLONG_MIN, max2=LLONG_MIN, max3=LLONG_MIN;
    long long min1=LLONG_MAX, min2=LLONG_MAX;
    for (int x : a) {
        if (x > max1)      { max3=max2; max2=max1; max1=x; }
        else if (x > max2) { max3=max2; max2=x; }
        else if (x > max3) { max3=x; }
        if (x < min1)      { min2=min1; min1=x; }
        else if (x < min2) { min2=x; }
    }
    return max(max1*max2*max3, min1*min2*max1);
}
```
Say: "the only competitor to 'three largest' is 'two most-negative × largest'."

### L2.4 String compression

Base version in §C4 (run-detect + `to_string`, count omitted at 1 → `a3b2c4d` ✓).
- (a) Always print count: drop the `if (j-i>1)` guard → `a3b2c4d1`.
- (b) In-place O(1): two pointers `read`/`write`; first count the run, then write the char, then the digits (backwards if writing digits in place). Legal because output never exceeds input length for this alphabet; if it could (e.g. always-print-count on `ab`), return the original if not shorter.
- (c) Counts ≥ 10: `to_string` handles multi-digit automatically (`"a"*12 → a12`) — that's the *answer*, plus: quote-generation needs the run length in digits (affects in-place sizing).

### L2.5 Linked list implementation

§C8 code. Checklist: destructor frees every node (RAII) · `removeFirst` handles the head case (the pointer-to-pointer trick, or the explicit two-branch version — either is fine, explain it) · no leaks on any path. Say: "insert at head O(1), search O(n), poor cache locality — vector usually beats list in practice even for mid-insertions because of cache effects."

### L2.6 Reverse a linked list

**Iterative (must be automatic):** §C8 — `prev/curr/next` slide. One-iteration narration: *"cut curr's next, point it back, slide prev and curr forward."*

**Recursive:**

```cpp
Node* reverseRec(Node* head) {
    if (!head || !head->next) return head;            // last node becomes new head
    Node* newHead = reverseRec(head->next);
    head->next->next = head;                          // my next points back at me
    head->next = nullptr;                             // I become the tail (temporarily)
    return newHead;
}
```
O(n) time, O(n) stack — say that cost; iterative is preferred in production.

### L2.7 Cycle detection + entry

`hasCycle` and `cycleStart` in §C8. The math, one line each: *"Inside the loop, fast gains on slow by 1 per step, so it must lap it. At the meeting point, fast traveled exactly 2× slow's distance; subtracting the common path, the head→entry distance equals meeting→entry — so a pointer from head and a pointer from the meeting point, both stepping by one, meet exactly at the entry."* Hash-set alternative O(n) space — mention it, then say Floyd wins on space.

### L2.8 Merge two sorted linked lists

```cpp
Node* mergeSorted(Node* a, Node* b) {
    Node dummy(0); Node* tail = &dummy;               // dummy head kills edge cases
    while (a && b) {
        if (a->val <= b->val) { tail->next = a; a = a->next; }
        else                  { tail->next = b; b = b->next; }
        tail = tail->next;
    }
    tail->next = a ? a : b;                           // append the leftover — no node copied
    return dummy.next;
}
```
O(n+m)/O(1), rewire-only. The `<=` keeps it stable. Say: "dummy head is the standard trick to avoid special-casing the first node."

### L2.9 Valid parentheses + min insertions

Base in §C6. **Min insertions to make valid:**

```cpp
int minAddToMakeValid(const string& s) {
    int open = 0, add = 0;
    for (char c : s) {
        if (c == '(') ++open;                         // unmatched opener so far
        else if (open > 0) --open;                    // matches an opener
        else ++add;                                   // unmatched closer needs an opener
    }
    return add + open;                                // + unmatched openers each need a closer
}
```
O(n)/O(1). Say: "two counters instead of a stack — we only need *counts*, not the actual brackets."

### L2.10 Two Sum variants

Hash version in §C5 (insert after checking → handles `x+x=target`).
**Sorted input, two pointers O(n)/O(1):**

```cpp
pair<int,int> twoSumSorted(const vector<int>& a, int t) {
    int lo = 0, hi = (int)a.size() - 1;
    while (lo < hi) {
        long long s = (long long)a[lo] + a[hi];
        if (s == t) return {lo, hi};
        if (s < t)  ++lo; else --hi;                  // move the side that can only increase/decrease the sum
    }
    return {-1, -1};
}
```
**All unique pairs (unsorted):** hash set of *values already paired*; for each `x` check `t-x` seen; store `(min,max)` pairs in a `set` to dedupe. Say: "deduping is the actual difficulty here."

### L2.11 Binary search variants

Standard + `lower_bound`/`upper_bound` in §C7. **Rotated sorted array (P3, optional):**

```cpp
int searchRotated(const vector<int>& a, int t) {
    int lo = 0, hi = (int)a.size() - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if (a[mid] == t) return mid;
        if (a[lo] <= a[mid]) {                        // left half is sorted
            if (t >= a[lo] && t < a[mid]) hi = mid - 1;
            else lo = mid + 1;
        } else {                                      // right half is sorted
            if (t > a[mid] && t <= a[hi]) lo = mid + 1;
            else hi = mid - 1;
        }
    }
    return -1;
}
```
Say: "at every step exactly one half is sorted; check whether the target lies in the sorted half."

### L2.12 Connected components

BFS version in §C9. **Union-find version:**

```cpp
struct DSU {
    vector<int> p, r;
    DSU(int n) : p(n), r(n, 0) { iota(p.begin(), p.end(), 0); }
    int find(int x) { return p[x] == x ? x : p[x] = find(p[x]); }   // path compression
    bool unite(int a, int b) {
        a = find(a); b = find(b);
        if (a == b) return false;                     // already connected
        if (r[a] < r[b]) swap(a, b);
        p[b] = a; if (r[a] == r[b]) ++r[a];           // union by rank
        return true;
    }
};
int countComponentsDSU(int n, const vector<vector<int>>& edges) {
    DSU d(n); int cnt = n;
    for (auto& e : edges) if (d.unite(e[0], e[1])) --cnt;
    return cnt;
}
```
**Incremental edges:** this is where union-find is strictly better — start `cnt = n`, decrement on every *successful* unite; each query is near O(1). BFS would re-flood per query. Say that sentence; it's the payoff of knowing both.

### L2.13 Top-K frequent

```cpp
vector<int> topKFrequent(const vector<int>& a, int k) {
    unordered_map<int,int> freq;
    for (int x : a) ++freq[x];
    priority_queue<pair<int,int>, vector<pair<int,int>>, greater<>> mh;   // (count, value)
    for (auto& [val, c] : freq) {
        mh.push({c, val});
        if ((int)mh.size() > k) mh.pop();             // evict the least frequent of the kept set
    }
    vector<int> out;
    while (!mh.empty()) { out.push_back(mh.top().second); mh.pop(); }
    return out;                                       // ascending frequency; reverse if needed
}
```
O(n log k)/O(n). **Bonus O(n):** bucket sort — `buckets[freq].push_back(val)`, then walk buckets from highest frequency. Say: "counts are bounded by n, so frequency is a perfect bucket key."

### L2.14 Trie phone directory

Base in §C11 (`add`, `exactExists`, `searchByPrefix` with subtree DFS). **Autocomplete — 3 suggestions at each node in O(prefix):** maintain a small sorted list `top3` **at every node**, updated along the insert path:

```cpp
// inside TrieNode: vector<int> top3;   // up to 3 contactIds, smallest lexicographic first
// on insert, at every node along the path:
//   keep only ids whose name < all current top3 entries; insert in sorted position; truncate to 3
// autocomplete(prefix): find node in O(P); answer = node->top3   — no subtree DFS at query time
```
Trade-off to say: "O(3) queries at the cost of O(L·3) extra work per insert — pay at write time to make reads instant; classic read-vs-write optimization." (Alternative: precompute per node the lexicographically-smallest-3 subtree words.)

### L2.15 Job portal

Base in §C12 (role buckets + salary filter + top-k) and the SQL + composite index `INDEX(role, salary DESC)`.
**Lazy deletion for removed jobs:**

```cpp
// jobs_ map holds live jobs. When popping from a per-role heap:
//   if (!jobs_.count(id)) continue;   // stale entry — skip it
// Deletion = jobs_.erase(id); the heap entry becomes a ghost that is skipped on sight.
```
Costs: deletion O(1) (no heap surgery), ghost entries cleaned only when popped. Trade-off to say: "lazy deletion trades a little memory + scan time for O(1) deletes — the same trick I'd use for a leaderboard with mutable scores." (Also acceptable: `std::set`/indexed DB rows with real deletion.)

## L3. LLD drill solutions

### L3.1 Traffic light — reference skeleton + the four change-drills

The full class skeleton (Approach/Lane/ConflictMatrix/Phase/Controller with the GREEN→AMBER→ALL-RED sequencing) is **§D1** — grade yours against its 7 design decisions. The change-drill answers:

- **(a) Two roads merge:** nothing is renamed. IDs are primary keys assigned at construction; a merge *adds* a new approach (new ID) plus new rows in the conflict matrix. Signals reference IDs, never positions — so no existing reference breaks. Say the anti-pattern: *"if I'd used array indices as identity, inserting a road would shift every index and corrupt every stored reference."*
- **(b) Dedicated left-turn lane with its own arrow:** lanes gain independent signal state; a left-turn arrow becomes its own *phase element* — conflict matrix now includes "left-turn vs opposing through traffic" pairs; phases are re-validated at construction. Composition win: `Lane` already exists, we extend `Phase`'s membership to lane IDs.
- **(c) Emergency request while pedestrian phase is active:** preemption must pass through the safe sequence anyway (current green → amber → all-red clearance → emergency green). The pedestrian countdown is *cut short* only by entering amber — pedestrians already crossing get the all-red clearance to finish. Never jump states; the state machine's edges are the only legal moves.
- **(d) Sensor-driven timings:** extract a `TimingPolicy` interface (fixed-time vs actuated vs adaptive) injected into the controller — the controller stops owning durations. Classic strategy-pattern answer; also makes the controller unit-testable with a fake clock.

### L3.2 Snake & Ladder — vanishing snake + the three change-drills

Reference code is **§D2** — the key line is `snakes_.erase(it)` inside `resolveLanding`: the bite applies once, then the map entry is gone, so later landings behave like a normal cell. Ladders persist. Change-drills:

- **(a) Extra turn on a 6:** `takeTurn` returns `bool tookExtraTurn`; the game loop repeats the same player's turn when true (guard against infinite 6-chains with a house rule if asked).
- **(b) Exact finish required:** `if (target > cells_) return false;` — already in the reference; state the rule question *before* coding ("overshoot: bounce or stay?") because it changes the endgame.
- **(c) Undo:** keep the game as an **event log** — every move appended as an event `{player, roll, from, to, jumpApplied}`; undo = pop the last event and recompute (or replay) position from the log. Say explicitly: *"this is event sourcing in miniature — the log is the truth, positions are projections. My Hermes project is built on exactly this idea."* Free credibility.

### L3.3 Leaderboard — reference + tie-breaks + scale

Reference code is **§D4** (`unordered_map` + ordered `map<score, set<ids>>`). Change-drills:

- **(a) Ties by who reached the score first:** the ordered key becomes `(score, timestamp)` descending — store `map<pair<long long,long long>, int>` where the second field is a monotonic arrival counter (smaller counter = earlier → put *negated* counter or order the pair so earlier wins). O(log n) still.
- **(b) `top(k)` hammered by thousands of clients:** precompute/serve — maintain a Redis **ZSET** (`ZREVRANGE 0 k-1`) in front of the authoritative store, or maintain a running top-k heap updated on write. Say the pattern: *"precompute on write to make reads O(1) — pay at update time."* Also mention caching + read replicas.

### L3.4 Phone directory (LLD shape) — changes

Composition structure: `PhoneDirectory` wraps a private `Trie` — swap-able internals. Change-drills: case-insensitive → normalize (`tolower`) once at the trie boundary, never store mixed case; `removeContact` → unmark `isEnd`, clear `contactId`, optionally prune empty branches (say "lazy deletion, prune later" — same trick as L2.15); top-3 autocomplete → per-node `top3` lists (L2.14).

### L3.5 Parking lot — reference sketch + the three change-drills

```cpp
#include <bits/stdc++.h>
using namespace std;

enum class SpotType { MOTORCYCLE, CAR, EV, TRUCK };

struct Spot { int id; SpotType type; bool occupied = false; };
struct Vehicle { string plate; SpotType needed; };        // reuse §B1's Vehicle hierarchy in a real answer

struct Ticket { string plate; int spotId; time_t entry; };

class FeeStrategy { public: virtual ~FeeStrategy() = default;
    virtual double fee(time_t from, time_t to) const = 0; };   // strategy: Hourly / Flat / Progressive

class ParkingLot {
public:
    ParkingLot(unique_ptr<FeeStrategy> fee) : fee_(move(fee)) {}

    int park(const Vehicle& v) {
        auto& q = free_[v.needed];
        if (q.empty()) throw runtime_error("no spot for type");
        Spot* s = q.front(); q.pop(); s->occupied = true;
        active_[v.plate] = Ticket{v.plate, s->id, time(nullptr)};
        return s->id;
    }
    double exit(const string& plate) {
        Ticket t = active_.at(plate); active_.erase(plate);
        Spot* s = byId_.at(t.spotId); s->occupied = false;
        free_[s->type].push(s);                           // spot returns to the free pool
        return fee_->fee(t.entry, time(nullptr));
    }
private:
    unordered_map<SpotType, queue<Spot*>> free_;          // O(1) pop/push per type
    unordered_map<string, Ticket> active_;
    unordered_map<int, Spot*> byId_;
    unique_ptr<FeeStrategy> fee_;
};
```
Change-drills: **nearest-first** → replace per-type `queue` with a min-heap keyed by (floor, distance) — O(log n) instead of O(1), that's the trade; **EV spots** → new `SpotType::EV` + a rule "EV cars may take CAR spots, not vice versa" (fall-back matching order); **3 entrances at once** → the per-type queue is now a shared mutable resource: protect with a mutex per type (or one global lock = simpler, more contention), and say *"the DB-backed version is the real answer at scale — the same row-level locking problem any inventory system has."*

## L4. Puzzle drill solutions

### L4.1 Hourglasses → 9 minutes

t=0 start both → t=4 flip the 4 → t=7 flip the 7 → t=8 the 4 empties; flip the **7** (it has run exactly 1 minute since t=7, so 1 minute of sand sits in its bottom chamber; flipping puts that 1 minute on top) → t=9 the 7 empties = **9 minutes**. Key sentence: *"9 = 8 + 1; the 1-minute leftover is manufactured from the 7-glass's first flip."*

### L4.2 Bike puzzle → 65/7 h ≈ 9 h 17 min

Strategy: A+B ride, B drops at km *p* and walks; A rides back to meet C at km *q*; A+C ride to the finish. Everyone arrives simultaneously; bike never idles.

- Meet equation: `q/15 = p/60 + (p−q)/60` ⟹ `2p = 5q` ⟹ `q = 2p/5`
- Simultaneous arrival: `p/60 + (300−p)/15 = p/60 + (p−q)/60 + (300−q)/60` ⟹ with `q=2p/5`: `21p/5 = 900` ⟹ `p = 1500/7 ≈ 214.3`, `q = 600/7 ≈ 85.7`
- **T = 25/7 + 40/7 = 65/7 ≈ 9.286 h ≈ 9 h 17 min.** Verify all three (A: 25/7+15/7+25/7 ✓, B: 25/7+40/7 ✓, C: 40/7+25/7 ✓).

The "no-rider return" modification: impossible — a bike can't move itself; the return leg always consumes a rider's progress, which is *why* the backhaul leg exists in the optimum. State that; it shows you model the constraint, not just the arithmetic.

### L4.3 Mislabeled jars

Draw from the jar labeled **"Mixed"** — its label is guaranteed wrong, so it's pure Apples *or* pure Oranges; one fruit names it. Then: the jar labeled with that fruit can't be itself and can't be Mixed (taken) → it's the *other* pure type; the last jar → Mixed. Why it works: you draw from the **maximally constrained** jar — the only one whose label eliminates two of three possibilities at once.

### L4.4 Ten bags of coins

Take *i* coins from bag *i* (1+2+…+10 = 55 coins), weigh once. All-real weight = 550 g; the deficit in grams **is** the fake bag's number (bag k contributes exactly k light coins). Modification (unknown *d*): deficit = k·d → divide the deficit by the *greatest common divisor pattern* — with unknown d you can't separate k from d in one weighing *unless* you weight counts asymmetrically (e.g. powers of 2: 1,2,4,…,512 → the deficit in binary identifies the bag regardless of d, since each bag's contribution is a unique power of d). Say: "with one unknown scale factor I switch to a unique-code construction — powers of two." (If d is known to be exactly 1, the 1..10 scheme suffices.)

### L4.5 Banana variant (2400 / 800 capacity / 600 km)

Re-derive with the segment method (shuttles = ⌈stock/capacity⌉, cost per km = 2·shuttles − 1):

- Stock 2400 → 3 loads → burn 5/km until stock = 1600: distance = (2400−1600)/5 = **160 km** (at km 160).
- Stock 1600 → 2 loads → burn 3/km until stock = 800: (1600−800)/3 ≈ **266.7 km** (at km 426.7).
- Stock 800 → 1 load → burn 1/km for the remaining 600 − 426.7 = 173.3 km → arrive with 800 − 173.3 ≈ **626.7 → 626 bananas**.

The point of the drill: you **re-derived** the regime breakpoints instead of reciting 533 — that's what the interviewer is watching for.

### L4.6 Bonus

(a) **Ropes → 45 min:** light rope A at *both* ends and rope B at *one* end simultaneously. A finishes at 30 min → immediately light B's other end → B's remaining 30 min burns in 15 min → total 45. ("Non-uniform burn rates mean only symmetric lighting produces known times.")
(b) **3 switches:** switch 1 ON for 5 min → OFF; switch 2 ON; go up. Bulb ON = switch 2; bulb OFF + **warm** = switch 1; OFF + cold = switch 3. Key phrase: *"the bulb's heat is a free extra bit of state."*

## L5. Case-study drill solutions (model answers to self-grade against)

### L5.1 Kids-sector incumbent vs challenger — model answer shape

**Clarify first:** "Am I optimizing revenue, visibility, or margin? Over what horizon?" (The reported case asked for visibility + revenue growth [CAND].)

**SWOT (incumbent) — 3 bullets per quadrant, each tied to a data request:**

| Quadrant | Bullets |
|---|---|
| **S** | Established brand + existing paying base (request: # active customers, repeat rate) · sales/distribution network incumbents already fund · profitability headroom to spend (request: margin) |
| **W** | Weak social/content muscle (the challenger's whole engine) · likely older acquisition channels, rising CAC · slower product iteration |
| **O** | The challenger has *proven* the social channel converts in this exact category → adopt it with 10× the budget · bundle breadth: "one-stop" cross-sell into the existing base ≈ near-zero CAC · partnerships (schools, pediatricians, retailers) |
| **T** | Younger parents may be permanently captured by the challenger · talent (creators) prefers the challenger · if challenger raises a round, ad auctions get more expensive |

**Growth strategy — 4 points, one KPI each:**
1. **Retention & share-of-wallet first** (cheapest revenue): bundles/cross-sell to existing customers — KPI: **+15% ARPU** in 2 quarters.
2. **Build the social engine** properly: short-form video + parent community — KPI: **follower→lead conversion ≥ 3%**.
3. **Referral loop** (kids categories are community-driven) — KPI: **30% of new customers via referral within 12 months**.
4. **Defend with breadth:** exclusive one-stop bundles a focused challenger can't match — KPI: **attach rate ≥ 2 products per customer**.

**Trade-off to state:** "social compounds but takes 6–12 months; referral + bundles produce revenue this quarter. Run both with different horizons." End: risks (brand dilution on cheap content, channel conflict with the sales team) + 30/60/90.

### L5.2 Cab startup, 5 cities — model answer shape

**Scorecard (weights said aloud):** demand density 25 · ability to pay 20 · supply feasibility 15 · competition 15 · regulation 10 · infrastructure 10 · launch cost 5. Score each city 1–5 from the PDF, multiply, rank, then **narrative-check** the winner ("does the story hold — e.g., highest income but tiny population?").

**Then:** start **urban core** (density → short pickups → high driver utilization → contribution per driver-hour), suburban second (commuter flows), rural last. **Costs table:** driver onboarding/incentives · marketing CAC · ops hub & support · licensing · tech scaling — state the marketplace-vs-fleet assumption because it multiplies capex. **Challenges:** chicken-and-egg supply↔demand (solve by concentrating on 2–3 dense zones, not city-wide), incentive burn (watch contribution/ride after incentives), regulation, incumbent retaliation, cash/trust. **30/60/90:** 30d launch zones + recruit; 60d hit utilization/repeat targets, fix pricing on real data; 90d expand **only if** contribution margin per ride > 0 post-incentive — say the threshold number.

### L5.3 Reliability + rising infra cost — model answer shape

Quantify first (churn traced to incidents × ARR; SLA credits; support tickets trend; eng-hours on firefighting). Split fixes: **quick wins** (cache hot reads, add the missing index, queue backpressure, raise autoscale limits) vs **structural** (multi-AZ, capacity model, SLO governance). Define **SLO 99.9% → error budget ≈ 43 min/month**; when budget burns, feature work freezes. **Cost levers:** rightsize, autoscale windows, storage tiering, spot for batch, delete zombie services — track **cost per transaction**, not absolute spend. 30/60/90: measure + 3 quick wins → budget governance + structural fix #1 → publish SLOs, cost/transaction down, top complaint driver closed.

## L6. Spoken / HR drill solutions (self-grade checklists)

- **K6.1 (60s intro):** passes if — under 60s when read aloud · 8 beats: name/college/CGPA → stack → Cron → WebChat → Hermes → internship → one proof-of-debugging sentence → why ION · zero invented facts · no filler words. (§I.1 is the reference script — yours must not copy it verbatim; make it yours.)
- **K6.2 (2-min Cron):** passes if all 5 parts appear: problem (1 sentence) · architecture (gateway/Postgres/RabbitMQ/3 workers/observability) · data flow (one POST traced end-to-end) · hardest problem (polling → TTL+DLX, with the 10k-test numbers) · trade-off (what you'd change: delayed-exchange benchmark, multi-tenant quotas).
- **K6.3 (one-liners):** each passes if it contains **one specific, real** story beat (a number, a named system, a concrete fix) and ends with what changed. Weakness: name a real one + the mechanism you use to manage it. Pressure: the load-test story. Teamwork: Geek Room organizing + internship feedback loop. Hire-you: ship end-to-end + debug systematically + honest about trade-offs.

## L7. Rapid-fire quiz answers (K7)

1. Encapsulation: private data + controlled access, protecting invariants · Abstraction: expose what, hide how · Inheritance: is-a reuse/substitutability · Polymorphism: one interface, many runtime behaviors.
2. Overloading: same name, different signature, compile-time. Overriding: same virtual signature in derived, runtime via vtable.
3. Deleting via `Base*` runs the destructor **statically**; without `virtual`, the derived destructor never runs → derived resources leak, UB.
4. The compiler already auto-chains base destructors after the derived body; calling it manually destroys the base **twice** → double free/UB.
5. Copying a derived object **by value** into a base-typed variable/container keeps only the base subobject; derived state is sliced off. Avoid: pointers/smart pointers, virtual functions through references.
6. Base classes, `const` members, reference members (also members without default constructors).
7. `unique_ptr`: sole owner, move-only, zero overhead — default. `shared_ptr`: shared ownership via refcount (overhead, cycle risk). `weak_ptr`: non-owning observer; breaks cycles.
8. Default member access: `struct` public, `class` private. Nothing else.
9. No — no vtable exists before construction; use a virtual `clone()` factory idiom.
10. `map`/`set`: O(log n), sorted order, range/prefix queries. `unordered_map`: avg O(1), worst O(n), no ordering. Ordered when order/range matters; hashed for pure lookup speed.
11. push/pop O(log n), top O(1); building from n elements O(n) (`make_heap`/heapify).
12. Array/vector: O(1) access, O(n) middle-insert; linked list: O(1) insert at a known node, O(n) access/search + poor cache locality.
13. **Trie** — prefix search is the core operation, O(P + results); a `map` needs a full O(n) scan per prefix query.
14. At each index: extend the running subarray, or restart here — restart when the running prefix went negative.
15. Floyd tortoise-and-hare: O(n) time, O(1) space (hash-set alternative O(n) space).
16. BFS/DFS flood-fill from each unvisited node: O(V+E). Union-find: near O(α) per edge, better for incremental edges.
17. Atomicity, Consistency, Isolation, Durability.
18. READ COMMITTED.
19. A thread lives inside a process and **shares its heap/fds** while keeping its own stack/registers; a process has an isolated address space.
20. Mutex: ownership lock, unlocked by the locker. Semaphore: counting signal, can be posted by anyone (producer-consumer).
21. DNS, live video/QUIC — latency or per-packet overhead matters more than guaranteed ordered delivery, or the app handles reliability itself.
22. Both sides must independently confirm the other's sequence number and receive capability; 2-way can't prevent old duplicate SYNs creating ghost connections.
23. 401: unauthenticated (who are you?) · 403: authenticated but forbidden (not allowed) · 429: too many requests (rate limited).
24. AuthN = identity, AuthZ = permission. JWTs are stateless → revoke via a server-side denylist/short-TTL + refresh rotation (your WebChat logout story).
25. LTV = ARPU × gross margin × avg lifetime (≈ ARPU ÷ churn). Healthy ≈ **LTV:CAC ≥ 3**.
26. Break-even units = fixed costs ÷ contribution margin per unit (price − variable cost).
27. Strengths and Weaknesses (Opportunities/Threats are external).
28. Flip the **7-glass**: it has run exactly 1 minute since its t=7 flip, so its bottom chamber holds 1 minute of sand; flipping puts that 1 minute on top → it empties at t=9.
29. Meet equation `q/15 = p/60 + (p−q)/60` ⟹ `2p = 5q`; simultaneous-arrival equation `p/60 + (300−p)/15 = p/60 + (p−q)/60 + (300−q)/60` ⟹ with q = 2p/5 → T = 65/7 h.
30. RabbitMQ is **at-least-once** — duplicates are possible; "exactly-once" is only achieved as an *effect* via execution IDs + idempotent reconciliation. Saying "exactly-once delivery" invites the interviewer to prove you wrong.

---

*End of drill sections. Attempt K → verify in L → mark ✅ → move to the next block. Anything marked [CAND] mirrors the real reported interview questions; [ADVICE] items are my judgment.*
