# ION Live-Coding Crash Course — simple code, every line explained

**Why this file exists:** [CAND intel] some candidates must **write, compile and RUN** code live (e.g., a banking system). This file (1) explains every construct in the drill from zero, (2) gives the **simple version** of the code you will actually type, (3) gives a second runnable program (bank system), (4) prepares the modifications they'll ask after it runs.

**Tonight's order:** Part 1 read (~25 min) → Part 2: type Vehicle/Car twice, compile → Part 3: type bank system once, **predict the output out loud, then run and match**.

Compile & run: `g++ -std=c++17 x.cpp -o x && ./x`

---

# PART 1 — The 10 micro-lessons

## M1. The header lines

```cpp
#include <iostream>     // gives you cout, endl (printing)
#include <string>       // gives you the string type
using namespace std;    // so you can write cout instead of std::cout
```
(You'll also see `#include <bits/stdc++.h>` in the guide — that's a "grab-everything" GCC header. Either is fine in an interview; if asked, say *"in production I'd include specific headers."*)

## M2. A class = blueprint; `public:` / `private:` = doors

```cpp
class Vehicle {
public:      // callers CAN use this
private:     // only the class's own code can touch this
};
```
The class is the blueprint; an **object** is a house built from it (`Car c(...)`). `private` is **encapsulation in code form**: the data is hidden so outsiders can't corrupt it; they use the public functions instead.

## M3. Member variables

```cpp
string regNo;    // data that belongs to each object (its state)
int seats;
```

## M4. The constructor — runs automatically when the object is created

```cpp
Vehicle(string regNo) {          // same name as the class, no return type
    this->regNo = regNo;         // this->regNo = the MEMBER; plain regNo = the PARAMETER
}
```
Its only job: get the object into a valid state. `this->` disambiguates "the member" from "the parameter with the same name".

## M5. Building the base-class part — `: Vehicle(regNo)` (the one syntax you can't skip)

```cpp
Car(string regNo, int seats) : Vehicle(regNo) {
    this->seats = seats;
}
```
A Car object = **a Vehicle part + the Car extras**. The Vehicle part must be built by Vehicle's constructor, and the `: Vehicle(regNo)` after the parameter list is exactly how you call it. **Interview line:** *"the base part is always constructed first; the colon syntax is how the derived class passes data up."* (You cannot put this inside the `{}` body — C++ gives no other way. Just memorise the shape.)

## M6. `virtual` — THE switch for runtime polymorphism

```cpp
virtual double farePerKm() { return 20; }              // in the BASE
double farePerKm() { return 30 + 5 * seats; }          // in Car — same signature = OVERRIDING
```
- `virtual` tells the compiler: *"resolve this call by the **real object's type at runtime**, not by the pointer's type."*
- Machinery (say in this order): each class with virtual functions gets one hidden **vtable** (array of function pointers). Each **object** carries one hidden **vptr** to its class's vtable. A call `v->farePerKm()` = load vptr → jump to that slot → Car's or Truck's version runs. **Cost: one indirection.**
- Without `virtual`: the pointer's type decides → a Car through a `Vehicle*` would charge 20, not 55. That bug is why the keyword exists.

## M7. Overriding vs overloading (+ the `override` keyword)

- **Overriding** = derived class redefines a virtual base function with the **same signature** → resolved at **runtime**.
- **Overloading** = same name, **different parameters**, same class → resolved at **compile time**. Here: `farePerKm()` vs `farePerKm(double surge)`.
- `override` written after the derived function's `()` is a free safety check: the compiler verifies the base really has a virtual with that exact signature. One typo without it = a *new unrelated function* and silently broken polymorphism. **Keep writing it — it's one word and interviewers notice it.**

| | Overloading | Overriding |
|---|---|---|
| Parameters | different | identical |
| Where | same class | base ↔ derived |
| Resolved | compile time | runtime (vtable) |

## M8. The virtual destructor + the trap

```cpp
virtual ~Vehicle() { }
```
- When you `delete` a `Car` through a `Vehicle*`, only a **virtual** destructor makes the *real* object's destructor run first. Non-virtual = undefined behaviour / the Car's extras leak.
- **The trap (memorise):** *"Derived destructors never call the base destructor — the compiler chains them automatically in reverse order: ~Car first, then ~Vehicle. Writing ~Vehicle() manually inside ~Car would destroy the base part twice — undefined behaviour."*

## M9. `new` / `delete` — and the one-line modern answer

```cpp
Vehicle* v1 = new Car("DL-02", 5);   // make a Car on the heap, hold it via a base pointer
...
delete v1;                            // free it (virtual dtor makes this safe)
```
- Forget `delete` = **memory leak**. Delete twice = crash.
- **If asked "how would a senior write it?":** *"replace raw `new`/`delete` with `unique_ptr` — a smart pointer that frees the object automatically when it goes out of scope (that's RAII); no leaks even if an exception is thrown."* One sentence — that's all you need to say; you don't have to write it.

## M10. Static vs dynamic binding — trace these two lines (the ONE big idea)

```cpp
Car c("DL-01", 4);
c.describe();          // compiler KNOWS c is a Car  → static binding
v1->describe();        // v1 is a Vehicle* pointing at a Car → dynamic binding
```
- `describe()` itself is **non-virtual** — resolved statically.
- **But inside describe, `farePerKm()` IS virtual** — so even inside a static call, the fare dispatches to the real object's version.
- That's why `v1->describe()` on a Car prints **Rs.55**, not 20. **Answer if asked "how did the right fare print through a Vehicle pointer?":** *"static dispatch of `describe`, dynamic dispatch of `farePerKm` — both mechanisms in one function."*

## The advanced-words decoder (know the NAME only — not needed to type)

| If you hear… | One-line meaning |
|---|---|
| `explicit` | stops sneaky implicit conversions like `Vehicle v = string("x")` |
| `std::move(x)` | "transfer the data instead of copying it" — an optimisation |
| `unique_ptr` / `make_unique` | the smart pointer from M9 — auto-delete, RAII |
| `const` member function | "this function promises not to modify the object" |
| member-init list assigning members (`: seats_(seats)`) | fancier way to initialise members; the **base-class call** part (M5) is the same colon syntax |
| `vector<unique_ptr<Vehicle>>` | a growable list of owned pointers — avoids "slicing" (copying only the base part, losing the derived extras) |
| object slicing | `Vehicle v = Car(...)` copies ONLY the Vehicle part — seats gone, polymorphism gone. Hence pointers. |

---

# PART 2 — The Vehicle/Car drill (SIMPLE version — type this)

```cpp
#include <iostream>
#include <string>
using namespace std;

class Vehicle {
private:
    string regNo;                        // hidden state (encapsulation)
public:
    Vehicle(string regNo) {              // constructor (M4)
        this->regNo = regNo;
    }
    virtual ~Vehicle() { }               // virtual destructor (M8)

    virtual double farePerKm() {         // runtime polymorphism (M6)
        return 20;
    }
    double farePerKm(double surge) {     // OVERLOADING — same name, extra param (M7)
        return farePerKm() * surge;
    }
    void describe() {                    // shared behaviour
        cout << regNo << " @ Rs." << farePerKm() << "/km" << endl;
    }                                    // note: farePerKm() inside is virtual (M10)
};

class Car : public Vehicle {             // inheritance: Car IS-A Vehicle
    int seats;
public:
    Car(string regNo, int seats) : Vehicle(regNo) {   // build the base part (M5)
        this->seats = seats;
    }
    double farePerKm() override {        // OVERRIDING + safety keyword (M7)
        return 30 + 5 * seats;
    }
};

class Truck : public Vehicle {
public:
    Truck(string regNo) : Vehicle(regNo) {}
    double farePerKm() override { return 45; }
};

int main() {
    Car c("DL-01", 4);
    c.describe();                        // static binding

    Vehicle* v1 = new Car("DL-02", 5);   // base pointer → derived object (M9)
    Vehicle* v2 = new Truck("DL-03");
    v1->describe();                      // dynamic binding → Car's fare
    v2->describe();                      // dynamic binding → Truck's fare

    cout << c.farePerKm() << " | " << c.farePerKm(1.5) << endl;   // overload pair

    delete v1;                           // virtual dtor → Car part destroyed properly
    delete v2;
    return 0;
}
```

**Predict the output BEFORE running (say it aloud, then verify):**
```
DL-01 @ Rs.50/km          ← 30 + 5×4 = 50
DL-02 @ Rs.55/km          ← Car 5 seats through Vehicle*: virtual dispatch → 30 + 25
DL-03 @ Rs.45/km          ← Truck's override through Vehicle*
50 | 75                   ← overload pair: 50, surged 50×1.5
```
**Then say, while pointing at the deletes:** "because the destructor is virtual, `delete` through the base pointer destroys the Car part first, then the Vehicle part — no leak."

**The 5-line narration while typing:** encapsulation (private state + public methods) → inheritance + base-constructor call → `virtual` + `override` (runtime) and the surge overload (compile-time) → vtable dispatch through the base pointer → virtual destructor + delete.

---

# PART 3 — The live write-compile-RUN drill: bank system (type once tonight)

Same concepts, different domain, ~65 lines, compiles clean, output predictable.

```cpp
#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Account {
    string owner;
    int balance;                 // whole rupees keeps the demo clean (floats cause rounding bugs)
    vector<string> history;      // private audit trail
public:
    Account(string owner, int opening) : owner(owner), balance(opening) {
        history.push_back("opened with Rs." + to_string(opening));
    }
    virtual ~Account() { }

    virtual string type() { return "Account"; }              // virtual hooks — subclasses override
    virtual double interestPerYear() { return 0; }
    virtual int withdrawLimit() { return 100000; }

    void deposit(int amt) {
        if (amt <= 0) { cout << "  invalid deposit amount\n"; return; }
        balance += amt;
        history.push_back("deposited " + to_string(amt));
    }
    void withdraw(int amt) {
        if (amt <= 0 || amt > balance || amt > withdrawLimit()) {
            cout << "  withdraw of " << amt << " refused (balance/limit)\n";
            return;
        }
        balance -= amt;
        history.push_back("withdrew " + to_string(amt));
    }
    void showHistory() {
        cout << "  history of " << owner << ":\n";
        for (int i = 0; i < (int)history.size(); i++)
            cout << "    " << history[i] << "\n";
    }
    void statement() {
        cout << type() << " account of " << owner << " : Rs." << balance
             << " | interest " << interestPerYear() << "%/yr\n";
    }
    int getBalance() { return balance; }
};

class SavingsAccount : public Account {
public:
    SavingsAccount(string owner, int opening) : Account(owner, opening) {}
    string type() override { return "Savings"; }
    double interestPerYear() override { return 4; }
};

class CurrentAccount : public Account {
public:
    CurrentAccount(string owner, int opening) : Account(owner, opening) {}
    string type() override { return "Current"; }
    int withdrawLimit() override { return 500000; }     // businesses move more money
};

int main() {
    Account* a1 = new SavingsAccount("Ritik", 50000);
    Account* a2 = new CurrentAccount("ShopA", 120000);

    a1->deposit(1000);
    a1->withdraw(500);
    a1->statement();
    a1->showHistory();

    a2->deposit(1000);
    a2->withdraw(500);
    a2->withdraw(900000);         // refused — over Current's limit → shows virtual dispatch live
    a2->statement();
    a2->showHistory();

    delete a1;
    delete a2;
    return 0;
}
```

**Predict aloud, then run:**
```
Savings account of Ritik : Rs.50500 | interest 4%/yr
  history of Ritik:
    opened with Rs.50000
    deposited 1000
    withdrew 500
  withdraw of 900000 refused (balance/limit)
Current account of ShopA : Rs.120500 | interest 0%/yr
  history of ShopA:
    opened with Rs.120000
    deposited 1000
    withdrew 500
```
Check yourself: Ritik 50000 + 1000 − 500 = **50500** ✓ · ShopA 120000 + 1000 − 500 = **120500** ✓ · 900000 > Current limit 500000 → refused ✓ (and notice the SAME `withdraw` code used different limits per type — that's the whole point of `virtual withdrawLimit()`).

**Spoken script while typing (30 seconds):**
"Account is the abstraction — balance and history are **private**, changed only through `deposit` and `withdraw`, which validate first so bad states can't happen. `type()`, `interestPerYear()` and `withdrawLimit()` are **virtual hooks** — Savings and Current override only what differs; everything else is inherited. `statement()` is shared code that prints the right type and interest **at runtime through the vtable** — watch the refused withdrawal: same withdraw function, different limit per account type, decided at runtime. If asked how a senior would improve it: `unique_ptr` instead of new/delete."

## Modifications they'll ask after it runs (≤5 lines each)

1. **"Add FixedDeposit — no withdrawals."** New subclass; `withdraw` overridden to print "not allowed" and return; override `type()` (return "FD") and `interestPerYear()` (7). "Nothing else changes — Open/Closed: new subclass, zero edits to tested code."
2. **"Show the richest account."** Copy the pointers into a temp array, `sort` by `getBalance()` descending, print the first. "For a live top-K I'd keep a sorted `set` instead."
3. **"Two tellers deposit at the same moment — bug?"** "Both do `balance += amt` — read-modify-write race, one deposit is lost. Fix: a `mutex` per account around deposit/withdraw."
4. **"Monthly interest payout?"** "`balance += balance * interest / 1200` in a loop — and if rates get complex, make interest a separate interchangeable object (Strategy pattern)."
5. **"Save to disk."** "`ofstream` opened in a small wrapper class — constructor opens, destructor closes. Same RAII idea as the smart pointer."

## When it DOESN'T compile — stay calm, read it aloud

| Error | Meaning | Fix |
|---|---|---|
| `request for member ... in 'v' of non-class type` | used `.` on a pointer | use `->` |
| `no member named 'x' in 'Y'` | typo or signature mismatch | match the base signature exactly |
| `segmentation fault` after delete | double delete or use-after-delete | delete once; set pointer to `nullptr` |
| `expected ';' before ...` | missing semicolon on the previous line | look one line ABOVE the error line |
| link error `undefined reference` | declared a function, never wrote its body | give it a body |

Say while fixing: *"compile error — it's complaining about X, so the cause is Y."* Reading the error aloud **is** the demo.

---

# PART 4 — Tonight, in order

1. **Part 1 micro-lessons** — read once; re-read any fuzzy one only.
2. **Type Vehicle/Car (Part 2) from memory → compile → predict output aloud → run → match.** Twice if time allows.
3. **Type bank system (Part 3) once → predict → run → match → recite the 5 modifications from memory.**
4. Then continue the execution plan (Block 2 traps → Block 3 DSA …). Note: the guide's §B1 shows the *professional* version of the same drill (const, move, smart pointers) — you now know all those words from the decoder table; mention them only if asked "how would you improve it."
