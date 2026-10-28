# ION Live-Coding Crash Course — understand every line, then type and run it

**Why this file exists:** [CAND intel] some candidates are asked to **write, compile and RUN** code live (e.g., a banking system). This file (1) explains every construct in the Vehicle/Car drill from zero, (2) gives you a second full runnable program (bank system) with its exact output, (3) prepares the modifications they'll ask after it runs.

**Tonight's order:** Part 1 read (~30 min) → Part 2: type Vehicle/Car twice, compile → Part 3: type bank system once, **predict the output out loud, then run and match**. Compile command: `g++ -std=c++17 x.cpp -o x && ./x`.

---

# PART 1 — The 13 micro-lessons (each explains one alien thing)

## L1. The two header lines

```cpp
#include <bits/stdc++.h>
using namespace std;
```
- `#include <bits/stdc++.h>` — a GCC "grab everything" header: gives you `vector`, `string`, `cout`, `unique_ptr`, algorithms — all at once. **Say proactively:** "It's a competitive-programming convenience; in production I'd include specific headers like `<vector>`, `<memory>`." That one sentence reads senior.
- `using namespace std;` — without it you'd write `std::cout`, `std::vector`. It imports the whole standard library namespace so you can drop the `std::` prefix.

## L2. A class = blueprint; `public:` / `private:` = doors

```cpp
class Vehicle {
public:                 // anything below: callers can touch
private:                // anything below: ONLY the class's own code can touch
};
```
- The class is the *blueprint*; an **object** is a house built from it. `Car c(...)` builds a house; `c.describe()` rings its doorbell.
- `private` is the **encapsulation** pillar in code form: `regNo_` is hidden so no outsider can set it to garbage. Outsiders use the accessor `regNo()` instead.

## L3. Member variables and the trailing underscore

```cpp
string regNo_;    // the object's own data (its state)
```
The trailing `_` (`regNo_`, `seats_`, `balance`) is a naming convention meaning "this is a member variable". Pure convention, zero syntax — but it instantly distinguishes members from parameters inside a function body. Mention it as "my convention to make member state obvious".

## L4. A constructor — the function that builds the object

```cpp
explicit Vehicle(string regNo) : regNo_(move(regNo)) {}
```
- Same name as the class, **no return type**, runs automatically at creation.
- Its one job: get every member into a valid state. After the constructor, there is no "half-built" vehicle.
- `{}` at the end = empty body — all the work happened in the initializer list (L6).

## L5. `explicit` — one word, one sentence of explanation

```cpp
explicit Vehicle(string regNo)
```
Without `explicit`, C++ would silently allow `Vehicle v = string("DL-01");` — a sneaky implicit conversion from string to Vehicle. `explicit` says: **you must construct me deliberately**, `Vehicle v("DL-01");`. One-line interview answer: *"explicit blocks accidental implicit conversions through constructors of one argument."*

## L6. The member-initializer list (the `:` after the parentheses)

```cpp
Car(string regNo, int seats) : Vehicle(move(regNo)), seats_(seats) {}
//                             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ this whole part
```
- Everything after `:` initialises members **before the body runs**.
- `Vehicle(move(regNo))` initialises the **base-class part first** — the base is always constructed before the derived body. This is how Car says "run Vehicle's constructor with this registration".
- `seats_(seats)` copies the parameter `seats` into the member `seats_`.
- **Why prefer it over assigning in the body?** Members are constructed *anyway*; the init list constructs them **with the right value directly**, instead of default-constructing then re-assigning. It's also the *only* way to initialise const members, references, and the base class.

## L7. `std::move` — "transfer, don't copy"

```cpp
Vehicle(move(regNo))
```
Plain-words answer: *"`std::move` hands over the string's internal buffer instead of copying it — after the move, the parameter is empty and my member owns the data. It's an optimisation; removing it would still be correct, just slower."* That's the whole depth you need. (Real definition: it casts to an rvalue reference, enabling the move constructor. Say "transfer instead of copy" and you're fine.)

## L8. `const` — the promise keyword (three places in this code)

```cpp
const string& regNo() const { return regNo_; }
//    ^^^^^^ read-only alias    ^^^^^ promise: this function doesn't modify the object
```
1. **`const` member function** (the trailing one): promises not to modify the object. Only const functions can be called on a const object — so the more you mark const, the more your class is usable.
2. **`const string&` return**: returns a *read-only alias*, not a copy — no copy cost, and caller can't modify your private state.
3. **`const auto& v` in a loop** (L12): iterate without copying each element, and promise not to modify it.
**Interview line:** *"const is a compile-time contract — it documents intent where comments fail."*

## L9. `virtual` — THE switch that turns on runtime polymorphism

```cpp
virtual double farePerKm() const { return 20.0; }   // in base
double farePerKm() const override { return 30.0 + 5.0 * seats_; }  // in Car
```
- Marking a function `virtual` tells the compiler: *"resolve this call by the **real object's type at runtime**, not by the pointer's type."*
- Machinery (say it in this order): every class with virtual functions gets one hidden **vtable** — a static array of function pointers. Every **object** gets one hidden **vptr** pointing to its class's vtable, set during construction. A call `v->farePerKm()` = load vptr → index the `farePerKm` slot → call whatever address is there — Car's or Truck's. **Cost: one indirection.**
- Without `virtual`: the compiler uses the **pointer's** type → always Vehicle's version → the 5-seat car charges 20, not 55. That bug is the whole point of the keyword.

## L10. `override` + overloading vs overriding

- `override` on the derived function makes the **compiler verify** a virtual function with that exact signature exists in the base. Without it, a typo (say you forget `const`) silently creates a *new unrelated function* — and polymorphism breaks with zero errors. `override` turns that silent bug into a compile error.
- **The table to recite:**

| | Overloading | Overriding |
|---|---|---|
| Signature | same name, **different** parameters | **identical** signature |
| Where | same scope (usually same class) | base ↔ derived |
| Resolved | compile time (static) | runtime (vtable) |
| Here | `farePerKm()` vs `farePerKm(double surge)` | `Vehicle::farePerKm()` vs `Car::farePerKm()` |

## L11. `= default` and the virtual destructor

```cpp
virtual ~Vehicle() = default;
```
- `= default` = "compiler, generate the standard destructor for me." Used when the class needs a *virtual* destructor but has no custom cleanup.
- **Why virtual?** When you `delete` (or a `unique_ptr` dies) through a `Vehicle*`, only a virtual destructor dispatches to the real object's destructor first. Non-virtual = undefined behaviour / leaked derived parts.
- **The trap answer (memorise):** *"Derived destructors never call the base destructor — the compiler chains them automatically, in reverse construction order: ~Car body, then ~Vehicle. Writing ~Vehicle() manually inside ~Car would destroy the base part twice — undefined behaviour."*

## L12. Smart pointers, vector, range-for, `->` vs `.`

```cpp
vector<unique_ptr<Vehicle>> fleet;
fleet.push_back(make_unique<Car>("DL-02", 5));
for (const auto& v : fleet) v->describe();
```
- **`unique_ptr<Vehicle>`** = a pointer that **owns** its object and destroys it automatically when it goes out of scope. That's **RAII** — resource lifetime tied to object lifetime; no `delete` anywhere, no leaks even if an exception flies. Cannot be copied (one owner!), only moved — that's the safety.
- **`make_unique<Car>(...)`** = constructs a Car on the heap and wraps it — the modern way, exception-safe.
- **Why `vector<unique_ptr<Vehicle>>` and not `vector<Vehicle>`?** Storing by value **slices**: only the Vehicle part of Car is copied, `seats_` is lost, and dispatch breaks. Polymorphism needs **pointers/references**. One sentence: *"store by pointer, slice by value."*
- **`for (const auto& v : fleet)`** = for each element, `auto` lets the compiler deduce the type, `&` avoids copying it, `const` promises not to modify it.
- **`v->describe()` vs `c.describe()`**: `.` works on the object itself; `->` works through a pointer (`v` is a `unique_ptr`, and `v->f()` means `(*v).f()`).

## L13. Static vs dynamic binding — trace these two lines (the ONE big idea)

```cpp
c.describe();              // c is a Car  → compiler knows the type → static binding
v->describe();             // v is a Vehicle* pointing at a Car → dynamic binding
```
- `describe()` itself is **non-virtual** — the call to it is resolved statically.
- **But watch inside describe:** it calls `farePerKm()`, which **is virtual** — so even inside a static call, the fare dispatches through the vtable to the real object's version.
- That's why `v->describe()` on a Car prints **Rs.55**, not Rs.20: *static dispatch of `describe`, dynamic dispatch of `farePerKm` — both mechanisms in one function.* If the interviewer asks "how did the right fare print through a Vehicle pointer?" — **this is the answer.**

---

# PART 2 — The Vehicle/Car code, annotated + traced output

This is the guide's §B1 model (the one you pasted). **Type it twice tonight.** The margin notes are the narration.

```cpp
#include <bits/stdc++.h>                    // L1  (say: "convenience header; prod → specific headers")
using namespace std;                        // L1

class Vehicle {                             // L2  blueprint for a rental-fleet vehicle
public:
    explicit Vehicle(string regNo) : regNo_(move(regNo)) {}
    // L4 constructor · L5 explicit · L6 init list · L7 move
    virtual ~Vehicle() = default;           // L11 virtual destructor → safe polymorphic delete
    const string& regNo() const { return regNo_; }
    // L8 read-only alias + const promise  (encapsulation: state hidden, accessor exposed)
    virtual double farePerKm() const { return 20.0; }   // L9 the runtime-polymorphism switch
    void describe() const {                 // non-virtual SHARED behaviour…
        cout << regNo_ << " @ Rs." << farePerKm() << "/km\n";
    }                                       // …but farePerKm() inside is virtual → L13
private:
    string regNo_;                          // L3 hidden state
};

class Car : public Vehicle {                // inheritance: Car IS-A Vehicle
public:
    Car(string regNo, int seats) : Vehicle(move(regNo)), seats_(seats) {}
    // L6 base-first init, then own member
    double farePerKm() const override { return 30.0 + 5.0 * seats_; }
    // L10 OVERRIDING (runtime) — same signature, override = compiler-checked
    double farePerKm(double surge) const { return farePerKm() * surge; }
    // L10 OVERLOADING (compile-time) — extra parameter; note it REUSES the override above
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
    c.describe();                           // static binding: compiler knows c is a Car

    vector<unique_ptr<Vehicle>> fleet;      // L12 RAII container of mixed types (no slicing)
    fleet.push_back(make_unique<Car>("DL-02-CD-5678", 5));
    fleet.push_back(make_unique<Truck>("DL-03-EF-9012"));
    for (const auto& v : fleet) v->describe();   // L13 dynamic binding via vtable

    cout << c.farePerKm() << " | " << c.farePerKm(1.5) << "\n";  // L10 overload pair
    return 0;
}                                           // fleet destructs itself: ~Truck/~Car then ~Vehicle each
```

**Predict the output BEFORE running (say it aloud, then verify):**
```
DL-01-AB-1234 @ Rs.50/km        ← Car 4 seats: 30 + 5×4 = 50
DL-02-CD-5678 @ Rs.55/km        ← Car 5 seats via Vehicle*: virtual dispatch → 30 + 25
DL-03-EF-9012 @ Rs.45/km        ← Truck via Vehicle*: Truck's override
50 | 75                         ← overload pair: base rate 50, surged 50×1.5
```
Being able to **predict then confirm** is the single most senior-looking move in a run-it-live round.

**The 5-line narration while typing (same as transcript M2):** abstraction/encapsulation → inheritance + init list → override + override keyword → vtable dispatch through `Vehicle*` → virtual destructor, RAII, automatic chaining.

---

# PART 3 — The live write-compile-RUN drill: bank system [NEW — type once tonight]

**If they say "write a banking system and run it" — this is your version.** Same concepts, different domain, ~60 lines, compiles clean, output predictable. Every construct is already explained in Part 1.

```cpp
#include <bits/stdc++.h>
using namespace std;

class Account {                                             // base: common banking contract
    string owner;
    long long balance;          // rupees as INTEGER: float money = rounding bugs (say this!)
    vector<string> history;     // audit trail — private state (encapsulation)
public:
    Account(string o, long long opening) : owner(move(o)), balance(opening) {
        history.push_back("opened with " + to_string(opening));
    }
    virtual ~Account() = default;

    virtual string type() const { return "Account"; }        // virtual hooks for subclasses
    virtual double interestPerYear() const { return 0.0; }
    virtual long long withdrawLimit() const { return 100000; }

    bool deposit(long long amt) {
        if (amt <= 0) return false;                          // validate FIRST: invariant guarding
        balance += amt;
        history.push_back("deposit " + to_string(amt));
        return true;
    }
    bool withdraw(long long amt) {
        if (amt <= 0 || amt > balance || amt > withdrawLimit()) return false;
        balance -= amt;
        history.push_back("withdraw " + to_string(amt));
        return true;
    }
    void printHistory(int lastN = 3) const {                 // default argument
        int start = max(0, (int)history.size() - lastN);
        for (int i = start; i < (int)history.size(); ++i)
            cout << "    " << history[i] << "\n";
    }
    void statement() const {                                 // shared behaviour, like describe()
        cout << type() << " [" << owner << "] Rs." << balance
             << " | interest " << interestPerYear() << "%/yr\n";
    }
    long long getBalance() const { return balance; }         // read-only accessor (L8)
};

class SavingsAccount : public Account {
public:
    SavingsAccount(string o, long long b) : Account(move(o), b) {}
    string type() const override { return "Savings"; }
    double interestPerYear() const override { return 4.0; }
};

class CurrentAccount : public Account {
public:
    CurrentAccount(string o, long long b) : Account(move(o), b) {}
    string type() const override { return "Current"; }
    long long withdrawLimit() const override { return 500000; }   // businesses move more money
};

int main() {
    vector<unique_ptr<Account>> bank;                        // RAII fleet of accounts
    bank.push_back(make_unique<SavingsAccount>("Ritik", 50000));
    bank.push_back(make_unique<CurrentAccount>("ShopA", 120000));

    for (auto& acc : bank) {
        acc->deposit(1000);
        acc->withdraw(500);
        acc->statement();                                    // virtual type() + interestPerYear()
        acc->printHistory();
    }
}
```

**Predict aloud, then run:**
```
Savings [Ritik] Rs.50500 | interest 4%/yr
    opened with 50000
    deposit 1000
    withdraw 500
Current [ShopA] Rs.120500 | interest 0%/yr
    opened with 120000
    deposit 1000
    withdraw 500
```
Check yourself: Ritik 50000+1000−500 = **50500** ✓. Withdraw of 500 < limit 100000 ✓ so it succeeds for both.

**Spoken script while typing (30 seconds, matches the pillars):**
"Account is the abstraction — balance and history are **private**, mutated only through `deposit`/`withdraw`, which validate first so the invariant 'no negative, no over-limit' can never break. `type()`, `interestPerYear()`, `withdrawLimit()` are **virtual hooks** — Savings and Current **override** what differs; everything else is inherited unchanged. `statement()` is shared behaviour that **dispatches to the real type at runtime** through the vtable. The bank is a `vector<unique_ptr<Account>>` — RAII, no leaks, and a **virtual destructor** makes polymorphic cleanup safe. Money is `long long` because floating-point money causes rounding bugs."

## The modifications they'll ask after it runs (answer in ≤5 lines each)

1. **"Add FixedDeposit — no withdrawals allowed."** New subclass; override `withdraw` to `return false`, override `type()` and `interest` (say 7%). "Nothing else changes — that's Open/Closed: extension by a new subclass, zero edits to tested code."
2. **"Show the richest account."** Add accessor `getBalance()` (already there), then copy the pointers, `sort` by balance descending, print the first. O(n log n); "for a live top-K under updates I'd keep a `set<pair<balance,id>>` instead."
3. **"Two tellers deposit simultaneously — bug?"** "Both do `balance += amt` — read-modify-write race, a lost update. Fix: a `mutex` per account around deposit/withdraw; say 'in the real design, account-level locking, and the ledger append becomes the source of truth'."
4. **"Interest payout?"** "A monthly loop: `balance += balance × interest/1200` — and I'd make interest a **virtual method on a Strategy object** if rates get complex; that's the Strategy pattern in one line."
5. **"Save accounts to disk."** "An RAII `ofstream` wrapper class — constructor opens, destructor flushes/closes; exactly the RAII pattern from `unique_ptr`, applied to files."

## When it DOESN'T compile — the calm-error table (this is the scored moment)

| Error you see | What it means | Fix |
|---|---|---|
| `request for member 'x' in 'v' of non-class type` | you used `.` on a pointer | use `->` |
| `'class X' has no member named 'y'` | typo, or override signature mismatch | match the signature **exactly** (including `const`) |
| `passing 'const X' as 'this' argument discards qualifiers` | calling a non-const method from a const one | mark the called method `const` |
| `invalid application of 'sizeof' to incomplete type` | forward-declared but never included | add the real `#include` |
| copy of `unique_ptr` refused | smart pointers can't be copied | `make_unique` a new one, or `std::move` it |

Say while fixing: *"compile error — let me read it: it's complaining about X, so the cause is Y."* Reading the error aloud is the demo.

---

# PART 4 — Tonight, in order

1. **Part 1 micro-lessons** — read once; for any lesson that still feels fuzzy, re-read only that one (~30 min).
2. **Type Vehicle/Car (Part 2) from memory → compile → predict output aloud → run → match.** Twice if time allows.
3. **Type bank system (Part 3) once → predict output → run → match.** Then recite the 5 modifications from memory.
4. Then continue the execution plan (Block 2 traps → Block 3 DSA …). The K1.2/K1.3 trap exercises will now make sense — L1–L13 are exactly what they test.
