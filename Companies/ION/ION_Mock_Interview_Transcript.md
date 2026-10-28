# ION SDE — Full Mock Interview Transcript (read, absorb, recite)

**What this is:** the complete technical round, case-study round, and HR/stakeholder block simulated on paper — **every question followed by its model answer**. No interaction needed. Everything aligns with `ION_Software_Developer_Interview_Master_Revision_Guide.md` (§ references given); this file is the *spoken* version.

**How to use (time-boxed):**
1. Read Part 1 straight through once (~50 min). Recite aloud every answer marked 🔊.
2. Re-read M2 (live code) and M5 (traffic light) — type both once in your editor.
3. Read Part 2 (case) + Part 3 (HR) (~30 min).
4. Before sleep: re-read only the ⚠️ trap boxes.

---

# PART 1 — TECHNICAL ROUND (60–90 min)

*Reported sequence [CAND]: intro → C++ self-rating → OOP verbal → live code → destructor trap → 1–2 DSA problems with modifications → LLD → puzzles → project deep-dive → CS rapid-fire.*

## M0. The opening (3 min)

**Interviewer:** "Tell me about yourself."

🔊 **You:** "I'm Ritik Raj, final-year B.Tech IT at MSIT, 2027 batch, CGPA 8.6. I work primarily in C++ and system design — I've solved 450+ LeetCode problems. My main work is three backend systems: a **distributed task scheduler** — cron-as-a-service with RabbitMQ and PostgreSQL, load-tested at 10,000 tasks with zero message loss; **WebChat** — a six-service real-time chat platform on Kubernetes with Socket.io and Redis; and **Hermes** — an event-sourced, multi-tenant workflow orchestrator on AWS Lambda, Step Functions and DynamoDB. I also interned at S.N. Engineering Works building EngineCore Pro, a React-and-Supabase product-ops dashboard. I enjoy systems where correctness under failure matters — that's the thread through all three projects."

**Interviewer:** "Rate yourself on C++."

🔊 **You:** "Seven out of ten. I'm confident in OOP, the object model, smart pointers and the STL — solid 8–9 there. I'd say 7 overall because I haven't used template metaprogramming or lock-free concurrency in production; I know the concepts but not the scars. And I'd rather tell you that honestly than claim a 10 and get exposed in five minutes."

> 💡 A 7 with an honest boundary reads *senior*. A 10 invites maximum-difficulty probing.

---

## M1. OOP verbal (5 min)

**Interviewer:** "What is OOP, and why is it a better paradigm?"

🔊 **You:** "OOP models a system as **objects** that bundle state with the behaviour that operates on it, instead of procedural code where data and functions live separately. It's 'better' when the domain has *entities with identity and lifecycle* — a vehicle, a session, a task — because each object guards its own invariants. That gives three wins: **encapsulation** lets me change internals without breaking callers; **polymorphism** lets me add new behaviour without editing old code; **composition of small objects** keeps each piece testable. It's not universally better — for pure data transforms, procedural or functional is simpler. My scheduler's gateway is Express handlers; there, classes would be ceremony."

**Interviewer:** "The four pillars?"

🔊 **You (one line each, with examples):**
- "**Encapsulation** — private state behind a public contract. My `Vehicle` owns `regNo_` privately; nobody can corrupt it."
- "**Abstraction** — expose *what*, hide *how*. Callers use `farePerKm()` without knowing each vehicle's pricing formula."
- "**Inheritance** — `Car : public Vehicle`, an is-a relationship that reuses and specialises."
- "**Polymorphism** — one interface, many behaviours. `v->farePerKm()` on a `Vehicle*` runs Car's or Truck's version depending on the real object — decided at runtime via the vtable."

**Interviewer:** "Compile-time vs runtime polymorphism?"

🔊 **You:** "Compile-time = **overloading and templates** — resolved by the compiler from static types, zero runtime cost. Runtime = **virtual functions** — resolved through the vtable at call time, cost is one indirection. Rule of thumb: compile-time when the type is known and speed matters; runtime when a base-class pointer must behave like its actual object."

**Interviewer:** "Overloading vs overriding?"

🔊 **You:** "Overloading: **same name, different signatures, same scope** — resolved at compile time, e.g. `farePerKm()` vs `farePerKm(double surge)`. Overriding: derived class **redefines a virtual base method with the same signature** — resolved at runtime. Write `override` on the derived function so the compiler verifies the signature matches — a typo'd overload otherwise silently becomes a new function."

---

## M2. THE LIVE-CODE DRILL (§B1 — 15 min, the #1 scored item)

**Interviewer:** "Open an editor. Implement classes showing **both** compile-time and runtime polymorphism. Model real entities, not Class A/B. Write destructors too."

> 🔴 **Type the SIMPLE version from Crash-Course Part 2** (`ION_Live_Coding_Crash_Course.md`) — no `explicit`, no `move`, no smart pointers. The block below is the same drill in professional dress (`const`, `std::move`, `unique_ptr`): **don't type it** — just know the words from the crash course's decoder table and mention them only if asked *"how would you improve it?"*

**Model answer (professional version — for reading, not typing):**

```cpp
#include <iostream>
#include <memory>
#include <vector>
#include <string>
using namespace std;

class Vehicle {                                    // abstraction: a rental-fleet vehicle
public:
    explicit Vehicle(string regNo) : regNo_(move(regNo)) {}
    virtual ~Vehicle() = default;                  // virtual: safe polymorphic delete

    const string& regNo() const { return regNo_; } // encapsulated state
    virtual string describe() const { return "Vehicle " + regNo_; }
    virtual double farePerKm() const { return 20.0; }
    double farePerKm(double surge) const {         // OVERLOAD — compile-time polymorphism
        return farePerKm() * surge;
    }
private:
    string regNo_;
};

class Car : public Vehicle {
public:
    Car(string regNo, int seats) : Vehicle(move(regNo)), seats_(seats) {}  // init list
    string describe() const override {             // OVERRIDE — runtime polymorphism
        return "Car " + regNo() + " (" + to_string(seats_) + " seats)";
    }
    double farePerKm() const override { return 25.0; }
private:
    int seats_;
};

class Truck : public Vehicle {
public:
    explicit Truck(string regNo) : Vehicle(move(regNo)) {}
    string describe() const override { return "Truck " + regNo(); }
    double farePerKm() const override { return 35.0; }
};

int main() {
    vector<unique_ptr<Vehicle>> fleet;             // RAII — no manual delete, no leaks
    fleet.push_back(make_unique<Car>("DL-01", 4));
    fleet.push_back(make_unique<Truck>("HR-77"));

    for (const auto& v : fleet) {
        cout << v->describe() << " @ " << v->farePerKm()   // runtime dispatch
             << " | surged: " << v->farePerKm(1.5) << "\n";// compile-time dispatch
    }
}   // destructors run automatically: ~Car, ~Truck, then ~Vehicle each
```

🔊 **The 5-line narration (say while pointing at code):**
1. "`Vehicle` is my abstraction — it owns the registration number as **private** state and declares `farePerKm()` **virtual** so each vehicle type prices itself."
2. "`Car` **inherits** Vehicle, initialises the base through the member-init list, and **overrides** `farePerKm` — `override` makes the compiler verify the signature."
3. "`farePerKm(double surge)` **overloads** in the same class — that's my compile-time polymorphism."
4. "In `main`, a `vector<unique_ptr<Vehicle>>` holds mixed types; `v->farePerKm()` **dispatches at runtime through the vtable** — Car's or Truck's version runs."
5. "Because the destructor is **virtual**, destroying through the base pointer destroys the real derived object first, then automatically chains the base destructor — no leak, no manual `delete`."

> ⚠️ **TRAP that follows immediately [CAND — a real candidate failed here]:** *"Write the destructors for both classes."* If you write `~Car() { ~Vehicle(); }` — **that is a compile error** and a conceptual red flag. Never call a base destructor manually. See M3.1.

---

## M3. Destructor & object-model follow-ups

**3.1 "You wrote destructors for both. Does Car's destructor need to call Vehicle's?"**

🔊 **You:** "**No — and it must not.** Destructors are chained automatically by the compiler, in exactly reverse construction order: `~Car` body first, then `~Vehicle`. Calling `~Vehicle()` manually would run it **twice** — once from my call, once from the automatic chain — which is undefined behaviour on the same memory. The only destructor I ever write a body for is cleanup the class itself owns: file handles, sockets, heap it allocated. `~Car` has nothing of its own, so I'd write `~Car() = default;` and let the chain do the rest."

**3.2 "What happens if the base destructor is NOT virtual and you `delete` a `Car` through a `Vehicle*`?"**

🔊 **You:** "Undefined behaviour — in practice, only `~Vehicle` runs, so anything Car owns leaks. That's exactly why any **polymorphic base class gets a virtual destructor**: `delete` must dispatch to the real object's destructor, just like any other virtual call."

**3.3 "What is object slicing?"**

🔊 **You:** "`Vehicle v = Car("DL-01", 4);` — assigning a derived object **by value** to a base copies only the base part; `seats_` is sliced off, and `v.farePerKm()` statically runs Vehicle's version. Polymorphism needs **pointers or references**. Hence my fleet is `vector<unique_ptr<Vehicle>>`, never `vector<Vehicle>`."

**3.4 "Why `unique_ptr` and not raw `new`/`delete`?"**

🔊 **You:** "RAII — ownership is tied to object lifetime. The vector's elements free themselves on scope exit, even through exceptions; raw `new` leaks on early return or throw, and I'd have to remember `delete` in every path. `unique_ptr` is zero-overhead — same size as a raw pointer. Shared ownership is the only case for `shared_ptr`, and it costs an atomic refcount — I don't need it here."

**3.5 "Where exactly is the vtable? Show me the machinery."**

🔊 **You:** "One vtable **per class** — a hidden static array of function pointers, holding the addresses of that class's overrides. Each **object** carries a hidden vptr to its class's vtable, set during construction. `v->farePerKm()` compiles to: load vptr → index the `farePerKm` slot → call whatever pointer is there. Cost: one indirection. The slot exists because `farePerKm` is virtual; a non-virtual call is resolved statically, no vtable involved."

**3.6 "Add a Bike class. What changes?"**

🔊 **You:** "Nothing above Vehicle changes — that's the point. `class Bike : public Vehicle` with its own `describe` and `farePerKm`, and `main` accepts it unchanged. Adding a type without touching existing tested code is the **Open/Closed principle** — extension by new subclasses, not by editing the `switch` statements a procedural version would need."

**3.7 "Change the fleet from a vector to holding objects directly in an array of Vehicle."**

🔊 **You:** "That's the slicing trap in storage form — `Vehicle arr[] = {Car(...), Truck(...)}` copies base parts only. To keep polymorphism I must store indirections: pointers or smart pointers. If C-style is forced, `vector<Vehicle*>` with careful ownership, but `unique_ptr` is the modern answer."

**3.8 "Why is `regNo_` private if both classes could use it protected?"**

🔊 **You:** "Because derived classes don't need to *mutate* the registration — and `protected` data couples every subclass to the base's internals; change the representation and all subclasses break. The public `regNo()` accessor keeps one contract. I use `protected` for behaviour hooks, not for data."

---

## M4. DSA — two problems + the modifications (25 min)

*Pattern [CAND]: you solve one, then the interviewer changes the requirement 2–3 times. State brute force first — it buys thinking time and shows honesty.*

### M4.1 Second-largest element

**Statement:** largest and second-largest in an array.

**Brute:** sort → take `a[n-2]`. O(n log n). "Works but does more than needed."

**Optimal — one pass, O(1) space:**

```cpp
long long secondLargest(const vector<int>& a) {
    long long best = LLONG_MIN, second = LLONG_MIN;
    for (int x : a) {
        if (x > best)      { second = best; best = x; }   // old champion demotes
        else if (x > second && x < best) second = x;      // strict '<' skips duplicates of best
    }
    return second;   // LLONG_MIN ⇒ no second-largest exists (all equal / size<2)
}
```

🔊 "Correctness: every element is compared exactly once against the champion; anything demoted can never be beaten by a later smaller element. Edge cases: empty, single element, all-equal → I return a sentinel and **say so out loud** — inventing a return value silently is the real bug."

**Modification 1 — "duplicates count as separate ranks"** (`[5,5,4]` → second is 5): "Then the `x < best` guard goes; instead of a sentinel I need to *distinguish* 'not seen' from 'value = MIN' — I use a bool `hasSecond`, or sort. The requirement decides the guard, which is why I asked."

**Modification 2 — "stream of numbers, dynamic insert/delete"**: "Single-pass two-variables no longer fits — I need a structure: `multiset` gives insert/erase O(log n) and second-largest as `*prev(prev(s.end()))` in O(1). If it were 'kth largest under a stream', two heaps — a max-heap of the k smallest half — with lazy deletion."

### M4.2 Kadane — maximum subarray sum

**Optimal + indices:**

```cpp
struct Result { long long sum; int l, r; };
Result kadane(const vector<int>& a) {
    long long best = a[0], cur = a[0];
    int bl = 0, cl = 0, br = 0;
    for (int i = 1; i < (int)a.size(); ++i) {
        if (cur < 0) { cur = a[i]; cl = i; }      // a negative prefix only hurts
        else           cur += a[i];
        if (cur > best) { best = cur; bl = cl; br = i; }
    }
    return {best, bl, br};
}
```

🔊 "Invariant: `cur` is the best subarray **ending at i**. If the running prefix went negative, dropping it can only help — restart at i. All-negative array: `cur` resets to `a[i]` every time, so `best` ends as the maximum single element — that's why I initialise with `a[0]`, not 0. O(n) time, O(1) space."

**Modification — "and the maximum product subarray":** "Product breaks Kadane's one-invariant because a negative flips sign: I carry **two** running values — max ending here and min ending here — swap them on a negative, and reset on a zero. Same O(n)."

### M4.3 Maximum product of three numbers

```cpp
long long maxProduct3(vector<int> a) {            // by value: I sort my copy
    sort(a.begin(), a.end());
    int n = a.size();
    return max((long long)a[n-1]*a[n-2]*a[n-3],   // three largest
               (long long)a[0]*a[1]*a[n-1]);      // two most-negative × largest
}
```

🔊 "The negative case is the whole question: two big negatives make a big positive. `[-10,-10,1,2,3]` → `(-10)(-10)(3)=300`, not `1·2·3=6`. O(n log n) by sort; O(n) if I track 3 maxima + 2 minima in one pass. If they push 'no sorting, streaming', that's exactly the one-pass version."

### M4.4 String compression ("aaabb" → "a3b2")

```cpp
string compress(const string& s) {
    string out;
    for (size_t i = 0; i < s.size();) {
        size_t j = i;
        while (j < s.size() && s[j] == s[i]) ++j;
        out += s[i];
        if (j - i > 1) out += to_string(j - i);
        i = j;
    }
    return out.size() < s.size() ? out : s;   // classic contract: only if smaller
}
```

🔊 "Two pointers, one write pass — O(n). Modifications to expect: **return original when compression grows it** (done above, and I state the contract aloud), **in-place** (two-pointer overwrite since output ≤ input), **counts ≥ 10** (multi-digit — my `to_string` handles it)."

### M4.5 Linked list — reverse + cycle (both reported asked [CAND])

```cpp
ListNode* reverse(ListNode* head) {              // in-place, O(1) space
    ListNode* prev = nullptr;
    while (head) { ListNode* nx = head->next; head->next = prev; prev = head; head = nx; }
    return prev;
}

ListNode* cycleEntry(ListNode* head) {           // Floyd: detect, then find entry
    auto *slow = head, *fast = head;
    while (fast && fast->next) {
        slow = slow->next; fast = fast->next->next;
        if (slow == fast) {
            auto *p = head;
            while (p != slow) { p = p->next; slow = slow->next; }
            return p;                            // cycle entry
        }
    }
    return nullptr;
}
```

🔊 "Brute for cycle: hash the visited nodes, O(n) space. Floyd: fast gains one node per step on slow inside the loop, so they must meet. Why does restarting one pointer from head find the *entry*? If head→entry is *a*, and at meeting slow has gone *a + b* while fast went *a + b + k·cycle*, then 2(a+b) = a+b+k·c → a = k·c − b — stepping both one-at-a-time meets exactly at the entry. Variants to quote: **reverse in k-groups** (reverse each chunk, splice), **middle** (slow/fast half-speed), **merge two sorted lists** (dummy head)."

---

## M5. LLD — traffic light (the flagship design, §D1)

**Interviewer:** "Design a traffic-light management system for an intersection where roads can merge. Cover conflicting greens, multiple lanes, pedestrians, emergency vehicles."

🔊 **Opening move (say this first — it's the scored part):** "Let me clarify scope and state assumptions: one intersection, fixed-time phases with preemption, a conflict = crossing or opposing-turn flows, pedestrian buttons queue demand, and on any fault the system must fail safe to all-red. Also — one design decision I'll defend up front: **approach IDs are primary keys, never array indices**, so a road merge can't invalidate stored references."

**The 7 decisions (each is a likely follow-up):**

1. **Stable IDs:** every approach = immutable `roadId` ("N1","S1","E1","W1") in a `map<string, Approach>`. A merge adds a *new* ID + new conflict rows; nothing is renamed or re-indexed. "IDs are primary keys, not positions."
2. **Conflicting greens:** the conflict matrix is **precomputed at construction**; a **Phase** is a set of approaches green together, *validated against the matrix when constructed* — a conflicting phase literally cannot exist. The controller only ever applies whole phases.
3. **Per-signal state machine:** `RED → GREEN → AMBER(3s) → ALL-RED clearance(2s) → RED`. The all-red clearance is what physically kills the both-yellow race.
4. **Lanes:** approach has lanes; lane type vehicle/pedestrian; special lanes (left-turn arrows) can carry their own state — approach-level default otherwise.
5. **Pedestrians:** walk **only parallel** to the vehicular green they cross against; button = queued demand consumed at the next compatible phase; countdown display.
6. **Emergency preemption:** if target approach already green → extend; else finish the current green safely (amber → all-red) → target green → optionally resume. **Never jump straight to green.**
7. **Fail-safe:** any fault → flashing red / all-red (treat as stop signs).

**Compact class skeleton (write this level of code):**

```cpp
enum class LightState { RED, AMBER, GREEN };
enum class LaneType   { VEHICLE, PEDESTRIAN };

class Lane {
public:
    Lane(string id, LaneType t) : id_(move(id)), type_(t) {}
    void setState(LightState s) { state_ = s; }      // controller is the only writer
private:
    string id_; LaneType type_; LightState state_ = LightState::RED;
};

class Approach {                                     // one road entering
public:
    explicit Approach(string roadId) : roadId_(move(roadId)) {}
    const string& roadId() const { return roadId_; } // stable primary key
    void addLane(Lane l) { lanes_.push_back(move(l)); }
private:
    string roadId_; vector<Lane> lanes_;
};

class ConflictMatrix {
public:
    void mark(const string& a, const string& b) { pairs_.insert(minmax(a, b)); }
    bool conflicts(const string& a, const string& b) const { return pairs_.count(minmax(a, b)); }
private:
    set<pair<string,string>> pairs_;                 // normalised unordered pairs
};

class Phase {                                        // a CONFLICT-FREE green set
public:
    Phase(string name, int greenSecs, vector<string> ids, const ConflictMatrix& m)
        : name_(move(name)), greenSecs_(greenSecs), ids_(move(ids)) {
        for (size_t i = 0; i < ids_.size(); ++i)     // validate at construction
            for (size_t j = i + 1; j < ids_.size(); ++j)
                if (m.conflicts(ids_[i], ids_[j]))
                    throw invalid_argument("conflicting phase");
    }
    bool contains(const string& id) const {
        return find(ids_.begin(), ids_.end(), id) != ids_.end();
    }
private:
    string name_; int greenSecs_; vector<string> ids_;
};

class IntersectionController {
public:
    void requestEmergencyPreemption(const string& roadId) {
        size_t target = phaseContaining(roadId);
        if (target == current_) { extendGreen(); return; }  // already green: extend
        transitionTo(target);                               // amber → all-red → green
    }
    void requestPedestrian(const string& crossing) { pending_.insert(crossing); }
private:
    void runPhase(size_t i) {
        current_ = i;
        setGreen(phases_[i]);                    // GREEN greenSecs_ (timer or sensors)
        setAll(LightState::AMBER);               // 3 s
        setAll(LightState::RED);                 // all-red clearance 2 s
    }
    void transitionTo(size_t target);            // finish safely: amber → all-red → green
    // members: map<string,Approach> approaches_; ConflictMatrix matrix_;
    //          vector<Phase> phases_; size_t current_ = 0; set<string> pending_;
};
```

**Likely follow-up probes:** *"What if a road merges mid-operation?"* → new ID + new conflict rows + rebuild phase list; nothing else changes. *"Who updates Lane state?"* → only the controller — one writer, no races. *"Two emergency vehicles?"* → priority queue of preemptions; serve highest first; others queue. *"Why not one timer per light?"* → then safety lives in every light's logic; here it lives in one validated structure (the matrix), which is auditable.

**Phone directory follow-up [CAND-reported]:** *"Which data structure for a phone directory?"* → "**Trie** — prefix lookup in O(|prefix|) independent of N; a hash map can't do prefix search, and sorting + binary search costs O(log n) per query *plus* range scanning." Then explain: 26-way children, `end` flag, insert/collect DFS for suggestions (code in §C if asked).

---

## M6. Puzzles (grade = approach, not the number) — §G

🔊 Universal opener: **"Let me state my assumptions first…"** — that sentence is scored.

**Hourglass (4 & 7 → 9 min):**
t=0 start both → t=4 flip the 4-glass → t=7 flip the 7-glass → t=8 the 4-glass empties, and the 7-glass has been running exactly 1 minute — flip it, putting that 1 minute of fallen sand on top → t=9 it empties. **"9 = 8 + 1; I manufacture the 1-minute leftover from the 7-glass's first flip."** Sanity check: every flip happens at the moment a glass ran out — all legal.

**Bike (3 people, 300 km, bike ≤2 riders 60 km/h, walk 15 km/h):**
Strategy: A+B ride to km *p*; B walks on; A rides back to km *q*, picks up C, rides to finish. Optimal ⟺ **bike never idles, nobody waits, everyone arrives together.** Two unknowns, two equations:
- walker-meets-bike: `q/15 = p/60 + (p−q)/60` → `2p = 5q`
- simultaneous arrival: `p/60 + (300−p)/15 = p/60 + (p−q)/60 + (300−q)/60`
→ **p ≈ 214.3 km, q ≈ 85.7 km, T = 65/7 h ≈ 9 h 17 min.** Every person arrives at exactly 65/7 — verify one aloud. Bounds: all-walk = 20 h; so 9.29 h sits sensibly between extremes.

**Mislabeled jars (every label wrong, one draw):**
Draw from the jar **labeled Mixed** — it can't be mixed, so its draw *reveals* it (apple → it's Apples). Then labeled-Apples can't be Apples and can't be Mixed → Oranges; last jar → Mixed. **"Pick the maximally-constrained jar first."**

**10 bags, one fake (9 g vs 10 g), one weighing:**
Take i coins from bag i (55 total) → weigh once → 550 − deficit = k → **bag k is fake**. "The deficit encodes the bag's identity — one weigh, 10 bits of position."

**Camel & bananas (3000 bananas, cap 1000, eats 1/km, 1000 km):**
Segment by load count: stock >2000 → 3 shuttles → cost **5 bananas/km** → 3000→2000 over 200 km; stock 1000–2000 → 2 shuttles → **3/km** → 2000→1000 over 333⅓ km (at km 533⅓); last leg 1 load → **1/km** → remaining 466⅔ km → deliver **533 bananas**. "If your numbers differ, don't recite — re-derive: cost/km = 2·(loads) − 1; breakpoints where stock hits 2000, then 1000." State assumptions: caching allowed anywhere, camel eats only while moving, returns empty.

**Two quick ones:** *ropes 60 min non-uniform → 45 min:* light A at both ends + B at one end; A done at 30 → light B's other end → B done 15 later. *3 switches, one trip:* switch 1 ON 5 min → off; switch 2 on; go up: ON=2, off+warm=1, off+cold=3 — **"heat is a free extra bit of information."**

---

## M7. Project defense (the reported 30-min resume block) — §E1–E3

> **Honesty rule:** every number below is verified from your own repos/tests. Never round up, never invent. "Not implemented — here's how I'd do it" is a *passing* answer; bluffing is a failing one.

### Scheduler — 30-second version 🔊
"Distributed cron-as-a-service. A Node/TypeScript **gateway** accepts tasks and publishes to **RabbitMQ** with a **per-message TTL** — the message waits in a parking queue until its due time, then the dead-letter exchange delivers it to workers. **Python workers** execute jobs as sandboxed subprocesses; a **Python coordinator** owns PostgreSQL state transitions under row locks and runs a **120-second reconciliation sweep** for anything TTL missed. Load test at 10,000 tasks: peak queue 8,562, ingestion 73–74 msg/s, the TTL burst fired ~1,600 msg/s, full drain in **7 min 46 s**, p95 gateway latency recovered from 160 ms to 76 ms, and **zero** messages lost or dead-lettered."

### Scheduler — the probes you will get:
- **"Why per-message TTL and not a database poller?"** → "Polling was the thing I removed: it reintroduced steady DB load and a latency floor. TTL gives push-style delivery at due time; the 120 s reconciliation covers the head-of-line caveat — if one long task blocks the queue head, late deliveries get caught by the sweep and re-published."
- **"Rate limiting — how?"** → "express-rate-limit, **fixed window, 10,000 requests / 15 min** on the ingest endpoint. I'll volunteer the limits: fixed window allows a 2× burst at window boundaries, and the in-memory store resets per process — a Redis-backed token bucket is the production upgrade."
- **"Your workers — what if a job hangs?"** → "subprocess with **no shell** (injection-safe argument passing) and a **300-second timeout**; the worker kills and records a failure. The documented next hardening is an image allowlist and container-per-task."
- **"Two coordinators — corruption?"** → "State transitions happen inside `SELECT … FOR UPDATE` row locks, and status guards make transitions idempotent — a second coordinator re-attempting a 'claimed' task is a no-op. Plus a partial index on `status IN ('pending','scheduled')` makes the reconciliation query index-covered."
- **"What would you fix first?"** → "Three honest gaps I can name: **no automated tests** today — I'd add contract tests per stage first; the **gRPC proto is defined but unwired**; and there's an **INSERT-then-publish window** — crash between them loses a task. The fix is a **transactional outbox**: commit the row and the event in one Postgres transaction, relay publishes separately."
- **"What exactly did your numbers prove?"** → "Only what the run measured: the queue absorbed the burst and drained with no loss *at 3 workers* — compose defines 10 but the benchmark and Prometheus scraping ran 3. I won't extrapolate to 10,000 msg/s; that needs its own run."

### WebChat — 30-second version 🔊
"Real-time chat on **six services** — gateway(4000), auth(3001), messages(3003), Socket.io chat(5000), uploads(3004), and a message-worker — on Kubernetes with HPAs, backed by Postgres for identity, MongoDB for the message store, Redis for the Socket.io adapter, and Elasticsearch for search. What I'm proudest of is the **race-condition analysis**: a worker crash mid-delivery can drop or duplicate a message, and I can explain both."

### WebChat — the race-condition probes:
- **"Explain the duplicate path."** → "Worker writes to Mongo, then publishes over RabbitMQ with **publisher confirms**. Crash *after* DB write, *before* confirm → unacked message is redelivered → republished → a downstream consumer sees it twice. The fix is an **idempotency key** — the messageId as a unique index; today that index does not exist, and I say that plainly."
- **"Explain the drop path."** → "Confirm means the broker persisted it; if the worker crashes before the confirm, treating the message as lost is *safe for the broker* but the event may still not have been reprocessed — the recovery relies on redelivery semantics. That's why the confirm channel is the correctness boundary, and why unacked work must be requeued."
- **"Why no sticky sessions?"** → "The Socket.io **Redis adapter** fans events to every pod holding a subscriber — connections are pod-local, correctness isn't. Sticky sessions would couple scaling to routing; the adapter removes the need. Load test: the multi-pod test is one of the 22 integration tests."
- **"22 tests — what do they cover?"** → "11 auth, 5 messages, 5 chat, 1 multi-pod fan-out. They gate image builds in CI — fail fast before an expensive docker build."

### Hermes — 30-second version 🔊
"Event-sourced multi-tenant workflow orchestrator on AWS: **13 TypeScript Lambdas** carry command/query and execution paths; events append to a **single-table DynamoDB** event store (PK `AGG#<Type>#<id>`, sorted SK `EVT#<seq>#<eventId>` — free ordering per aggregate); **EventBridge** fans out with a 90-day archive; **Step Functions** runs the durable workflow with declarative retries (3×, 2 s, ×2 backoff); an **SQS outbox** relays side effects. Java 25 replay/admin services, 18 Terraform modules, ~252 TS + 42 Java tests."

### Hermes — the probes:
- **"Why did the JVM move off the hot path?"** → ADR-013/014 story: "Cold-start cost on spiky Lambda traffic pushed command/query to TypeScript on nodejs20; replay/admin stayed on **Java 25** because they're low-traffic, long-running services where the JVM's strength — records, virtual threads, Spring/Flyway — pays."
- **"Event sourcing — what's the dual-write risk?"** → "Write event and publish notification must be atomic-ish. Honest state: the **inline EventBridgePublisher** is the live path and the SQS outbox producer **never stamps PENDING** — the outbox is scaffolded but not the primary path. The full fix is: stamp the outbox row in the same transaction as the append, relay publishes, mark SENT."
- **"Multi-tenancy enforcement?"** → "Cognito carries `custom:tenantId`; the admin schema adds per-tenant rate limits. Honest nuance: the API Gateway JWT authorizer is **disabled in the dev smoke config** — RBAC currently runs in handlers; enabling edge validation is config, not code."

### The "would you do it differently?" trap
Have **one prepared yes per project**, framed as growth: scheduler → transactional outbox; WebChat → idempotency key + unique messageId; Hermes → `TransactWriteItems` for the append + wire the outbox stamp. Quote them as "my own audit", not confessions.

---

## M8. CS rapid-fire (one breath each)

| Question | 🔊 Model answer |
|---|---|
| MySQL vs MongoDB [CAND-asked] | "MySQL: relational, schema-enforced, ACID joins — I pick it for identity/transactions. MongoDB: document, flexible shape, horizontal-friendly — I picked it for the write-heavy append-mostly message store queried one way. Polyglot by access pattern." |
| `map` vs `unordered_map` | "map = red-black tree, O(log n), ordered iteration, stable pointers; unordered = hash, O(1) average / O(n) worst, no order. Ordered need or worst-case guarantees → map; pure lookup → unordered." |
| Best sorting algorithm | "No universal best — introsort (std::sort) ≈ O(n log n) worst-case; merge for stability/linked lists; counting/radix for bounded integer keys. Quicksort's O(n²) worst case is why std::sort isn't plain quicksort." |
| Dijkstra | "Single-source shortest path, non-negative weights; greedy: extract min-distance node, relax edges; O((V+E)log V) with a heap. Fails negative weights → Bellman-Ford. I can whiteboard the heap loop." |
| Second-highest SQL without LIMIT/aggregate tricks | "`SELECT MAX(salary) FROM emp WHERE salary < (SELECT MAX(salary) FROM emp);` — correlate, don't sort." |
| Process vs thread | "Process: own address space, heavy isolation, IPC costs; threads: shared heap, cheap context switch, need synchronisation. My scheduler workers are *processes* by choice — a crashing job can't corrupt its siblings." |
| TCP vs UDP | "TCP: ordered, reliable, congestion-controlled — chat transport under WebSocket. UDP: fast, unreliable — DNS, gaming. Reliability is a *layer choice*, not free." |
| What is a DB index, cost? | "A sorted side-structure (B-tree) turning scans into O(log n) lookups; cost = every write maintains it + storage. My partial index exists precisely because indexing *all* rows would tax writes." |
| Virtual function cost | "One vtable indirection per call + one vptr per object + no inlining across the call. Negligible here; matters in tight numeric loops." |
| const correctness | "`const` is a compile-time contract: `const T&` params avoid copies and promise no mutation; const member functions can run on const objects. It documents intent where comments fail." |

---

# PART 2 — CASE STUDY ROUND (separate, eliminatory) — §H

**Framework to recite internally (H1):** 1 Clarify objective → 2 Structure → 3 Facts vs assumptions → 4 Quantify → 5 Compare ≥2 alternatives → 6 Recommend with reasons → 7 Risks + 30/60/90 plan.

**Formulas to have cold (H2):** Revenue = price × volume · Gross margin = (rev − COGS)/rev · **Contribution margin = price − variable cost** · **Break-even units = fixed costs / contribution per unit** · **LTV:CAC ≥ 3** healthy · Payback = CAC / monthly contribution · Scorecard = weights × scores.

## M9. Worked case ( practise once on paper)

**Brief:** "FreshBite, a 2-year-old meal-subscription startup in Pune: 8,000 active subscribers, ₹499/month average, gross margin 55%, monthly churn 6%, CAC ₹1,800 via paid ads. Board wants +30% revenue in 12 months. Options: (A) discount push in Pune, (B) enter one new city, (C) B2B corporate plans."

🔊 **Model walkthrough:**
1. **Clarify:** "Is churn 6%/month → lifetime ≈ 1/0.06 ≈ 16.7 months? Marketing budget constraint? Ops capacity? Is the 30% revenue *or* profit?" (If unstated, assume and label.)
2. **Base economics:** ARPU ₹499, contribution ≈ ₹274 (55%). LTV ≈ 274 × 16.7 ≈ **₹4,576**. LTV:CAC = 4,576/1,800 ≈ **2.5 — below 3**: acquisition efficiency is the real problem.
3. **Option A (discount):** 10% price cut → CM ₹225; payback = 1800/225 = **8 months** (vs 6.6 now); revenue per subscriber falls. Fights the wrong problem.
4. **Option B (new city):** fixed entry ≈ ₹15L (kitchen + ops); needs **15L/225 ≈ 6,700 subscriber-months ≈ 2,600 subscribers** steady to break even; CAC likely *higher* in a new market (brand unknown). High variance, slow.
5. **Option C (B2B):** corporate deals cut CAC (one buyer, many seats) and churn (annual contracts): even at ₹399/seat with 60% margin → CM ₹239; 50 companies × 100 seats = 5,000 seats ≈ **₹20L/month contribution** — beats A, de-risks churn, uses existing kitchens.
6. **Recommendation:** "**C as the engine, B as a bounded pilot, A rejected.** C: hire 2 enterprise sellers, target 40 corporates in 2 quarters. B: cap pilot at ₹5L with explicit exit criteria (CAC < ₹2,500, 1,000 subscribers in 4 months). Also fix the LTV:CAC 2.5 problem with referral credits — cheaper than paid ads."
7. **Risks:** key-account concentration (cap any client at 20%), kitchen capacity (pre-commit batches), margin dilution (floor price sheet). **30/60/90:** 30d — sign 5 lighthouse corporates; 60d — 20 clients or kill; 90d — city-pilot verdict.

**If it's the trading-company variant [CAND]:** same skeleton; emphasise *risks in three buckets* — strategic (client concentration, regulation), financial (fixed-cost bloat, FX), market (competition, demand seasonality) — and "immediate avoidable expenses" = idle infrastructure, duplicate tooling licences, manual-process labour that automation replaces.

**AI-tool discipline (the rules question):** 🔊 "Before using the tool, may I confirm what's permitted? If allowed: I'd use it to extract and organise the data — but I'll **verify one number by hand** and I own the recommendation." Then actually do exactly that.

---

# PART 3 — STAKEHOLDER & HR BLOCK

## M10. The $1M-more-expensive pitch [CAND — real stakeholder scenario]

🔊 **You:** "I won't sell the feature; I'll sell the *risk-adjusted cost*. First, the stakeholders: the money-owner (CEO/CFO), the operator who lives with the pain daily, the end client, and my own team's capacity. The manual process isn't free — I quantify its true annual cost: labour + error rate × cost-per-error + opportunity cost of delays. If manual truly costs less, **I say so and drop my proposal** — credibility first. If the $1M system wins, I present: payback period in months, error-cost eliminated, phased adoption — pilot with one team, explicit success metrics, and an exit clause so the CEO risks $100k, not $1M. The close: 'I'm not asking you to believe the vendor; I'm asking for a 90-day pilot with numbers we both agree on now.'"

## M11. HR one-liners (I-section)

- **Why ION?** 🔊 "Three honest reasons: the product depth — mission-critical trading software where a bug is money, which is exactly the correctness-under-failure space my projects live in; the engineer culture your interviewers show — every answer I gave today got challenged, which I want in a workplace; and scale — real financial systems, not toy dashboards."
- **Why should we choose you?** 🔊 "Evidence, not adjectives: 450+ problems solved; three production systems I can defend to any depth — including their flaws; a load-test I can quote to the message; and I improve when challenged — today's round proved you can push me and I'll reason, not freeze."
- **Views on AI and jobs** [CAND-asked] 🔊 "Optimistic with open eyes. Every general-purpose tool — computers, internet, now AI — removed tasks and created higher-leverage work. My own habit is the answer: I use AI daily to scaffold and review, but I verify what it produces and I own the design — the skill that compounds is *judgment over generation*. People who leverage it will take the meaningful jobs; the risk is not AI, it's refusing to learn it."
- **Weakness** 🔊 "I over-engineer for edge cases before shipping the core — my scheduler's first draft optimised reconciliation before it had basic tests. Working fix: I now write the happy path plus the failure I'm most afraid of, then ship, then harden."
- **Explain X to a non-technical person** [CAND-asked pattern] 🔊 Formula: analogy from their world → one sentence of mechanism → why it matters to them. ("Microservices: instead of one big shop where one fire closes everything, it's a street of small shops — one can burn without closing the street. You care because we can fix one shop at 2 AM without closing the market.")
- **Two questions to ask:** "What does the on-call look like for a new joiner on your trading platforms?" · "What separates your best first-year engineers from the good ones?"

---

# PART 4 — THE DIALOGUE-LLD ROUND (printing-system pattern) 🔴 NEW INTEL

> **Source [CAND, friend's ION SDE round]:** *No DSA coding questions at all — pure approach. The interviewer stated basic requirements himself, narrated them back, and kept cross-questioning: "how are you supposed to do this? how would you add this? how would you handle this?" Candidate never coded anything except a basic skeleton showing what goes where. Escalation: basic **queue** → real-world scenario forced **LRU** → more data-structure choices.*

If your round looks like this, Part 1's *coding* matters less — but **M1–M5 verbal answers and M5's DS reasoning matter more**. This part trains exactly that format.

## M12. The five rules of the dialogue round

1. **Mirror every requirement before solving it.** He narrates requirements loosely — you restate + label the assumption and get a nod: *"So: multi-user submission, FIFO order by default, one printer to start, and we generalize when you say so. Job state = id, user, pages, priority, submittedAt, status."* (Assuming things yourself and narrating them is **exactly** what he did — it's a scored behaviour.)
2. **One step per answer, then stop.** Give the change + the DS + the cost, then *let him pick the next probe*. Monologuing kills the format he enjoys.
3. **Every "how would you add/handle X?" uses the same formula:** *"New requirement → the change is (component / data structure) → cost = complexity → trade-off = what now degrades."* Say it in exactly that shape every time. Repetition reads as discipline, not scriptedness.
4. **Name the data structure out loud** — that's the keyword he's listening for (queue, heap, LRU, hash map). Then justify with *one* reason.
5. **End where he ended with your friend: a skeleton, no bodies.** Classes + fields + one-line responsibility each. "Here's what goes in it" is a complete answer.

## M13. The printing system — the full escalation (speak this aloud once)

**Stage 0 — requirements narration.** "Shared office printer service. Multiple users submit; jobs wait their turn; the printer consumes them; users see status. I'll assume: FIFO fairness by default, one printer first, priorities only when you say they exist." *(Now he owns the next constraint — that's the game.)*

**Stage 1 — the base: a QUEUE.** "Submission is **enqueue O(1)**, the print worker pops front — FIFO, arrival-order fairness. Why a queue and not a list I scan: it **decouples submission speed from printing speed** — the queue is the buffer when the printer is slower than the users."
> *If he stops here, you're done: queue + worker loop + job status machine.*

**Stage 2 — "the director's job must jump the queue."** "That's no longer FIFO — **priority queue (min-heap)** ordered by (priority, submittedAt) — the timestamp tie-breaker preserves FIFO *within* the same priority. Enqueue O(log n). New problem this creates, which I'd raise myself: **starvation** — a low-priority job can wait forever under constant VIP traffic. Fix: **aging** — effective priority = base + k × minutes waited, so old jobs eventually float up."

**Stage 3 — "user double-clicks submit; the server crashed and resubmitted the batch."** "Duplicate detection in the recent window. I keep an **LRU cache of job fingerprints** — hash of (document content + user) — at submission: if the fingerprint was seen in the last N minutes, reject or ask 'did you mean to reprint?'. **LRU = hash map + doubly-linked list, get/put O(1)**; the list keeps recency order, the map gives O(1) lookup, eviction drops the *least recently seen* fingerprint so memory stays bounded. Why LRU and not just a set: duplicates that matter are the *recent* ones, and an unbounded set grows forever — the eviction policy is the memory-bound fix."

**Stage 4 — "printer jams / goes offline mid-job."** "Job gets a status machine: `QUEUED → PRINTING → DONE | FAILED`. The worker marks PRINTING before sending; a **health ping** per printer; on failure, retry N times, then move to a **dead-letter list** and notify the user — never silently drop. If the printer is offline, dispatch **pauses** and jobs hold in the queue — which is exactly the buffering benefit from Stage 1; nothing is lost."

**Stage 5 — "the admin wants usage per user."** "A **ledger** — `map<userId, pagesThisMonth>` updated on completion, quotas checked at submission. Append-only records if they want auditability." *(This is your WebChat refresh-token-ledger thinking — same muscle.)*

**Stage 6 — "now 100 printers, 10,000 users."** "Dispatcher routes by printerId into **per-printer queues** — chosen by least-loaded or nearest; each queue is independent so one jammed printer can't block others. Scale = more dispatcher workers; shard by printerId. The single-queue design was the right *first* version; this is its growth path, not a rewrite."

**The skeleton you'd draw (what your friend did — classes + fields only):**

```cpp
struct PrintJob { long id; string user; string docHash; int pages;
                  int priority; long submittedAt; Status st; };

class JobQueue     { deque<PrintJob> fifo; };          // or priority_queue<(priority, submittedAt)>
class LruDedup     { size_t cap; list<Fingerprint> recency;
                     unordered_map<Fingerprint, list<Fingerprint>::iterator> lookup;
                     bool seenRecently(const Fingerprint&); void touch(const Fingerprint&); };
class Printer      { int id; bool healthy(); bool print(const PrintJob&); };
class Dispatcher   { void route(const PrintJob&); void onPrinterDown(int id); };
class UsageLedger  { unordered_map<string, int> pagesPerUser; void charge(user, pages); };
class PrintService { void submit(PrintJob); void workerLoop(); };   // owns the others
```

**DS-cheat table for this round (any system, same mapping):**

| When he says… | You name… | Cost | One-line why |
|---|---|---|---|
| "in arrival order" | Queue / deque | O(1) ends | arrival fairness + buffering |
| "VIP / priority / urgent" | Heap (priority queue) | O(log n) | + timestamp tie-break, + aging for starvation |
| "duplicate / double-submit / retry" | LRU (hash map + DLL) | O(1) | recent-window dedup, bounded memory |
| "fast lookup by id/user" | Hash map | O(1) avg | |
| "fairness across groups" | Per-group round-robin | O(1) | weighted RR beats raw FIFO |
| "it failed / hung" | Status machine + retry + DLQ | — | never silent loss |
| "track / bill / quota" | Ledger (map) | O(1) | auditability |

**Transfer drill (5 min):** run the same escalation on **parking lot** (queue at entry → levels/priority for VIP → LRU for recent-plate fast re-entry → payment ledger → multiple gates) and **leaderboard** (list → heap/top-K → hash for O(1) user lookup → update aggregation). If the real question is one of these instead, you've already rehearsed the *shape*.

## M14. Tonight's drill for this format (25 min — inside Block 5)

- [ ] Read M12's five rules aloud once (2 min).
- [ ] Speak the M13 six-stage escalation **without looking**, as if answering his cross-questions (10 min). Reset and repeat once if you stalled anywhere.
- [ ] Say the DS-cheat table's left column as prompts, answer with the middle column (5 min).
- [ ] Transfer drill on parking lot once (5 min).
- [ ] Recite the skeleton out loud: six classes, one line each.

---

## Final night checklist (from §A.4, compressed)
- [ ] **Part 4 dialogue-LLD drill (M12–M14): printing-system escalation aloud once + 5 rules + skeleton.** [NEW intel — highest priority with M2]
- [ ] Type the **SIMPLE** Vehicle/Car (Crash-Course Part 2) once more, clean compile; recite M3.1 (never call base dtor) + M3.3 (slicing) + the advanced-words decoder table.
- [ ] Kadane + second-largest + reverse + Floyd — once each on paper.
- [ ] Traffic light: recite the 7 decisions aloud once.
- [ ] Hourglass + bike derivations aloud once.
- [ ] Scheduler/WebChat/Hermes 30-second scripts aloud once each + the three "do differently" answers.
- [ ] Case formulas: contribution margin, break-even, LTV:CAC, payback.
- [ ] 60-second intro aloud once. AI tool installed and logged in.
- [ ] Sleep on time. The rested candidate who *talks through* problems beats the tired one who knows more.
