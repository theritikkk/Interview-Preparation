# OOP + SOLID + Patterns — the 20-minute revision (read BEFORE typing 01)

**Purpose:** you're about to type `code/01_vehicle_car.cpp`. This file puts every concept that code touches (and everything an interviewer hangs off it) into your head FIRST. Read once (~20 min), recite the bolded lines, then go type.

---

## 1. The four pillars — with YOUR code as the example

| Pillar | One-liner (say aloud) | Where it lives in your code |
|---|---|---|
| **Encapsulation** | "State is private; it changes only through public methods that validate first." | `regNo` in `Vehicle`; `balance`+`history` in `Account` — `withdraw()` refuses bad amounts so bad states can't exist |
| **Abstraction** | "Expose WHAT, hide HOW — callers use `farePerKm()` without knowing each type's formula." | `describe()` in 01; `statement()` in 02 |
| **Inheritance** | "`Car : public Vehicle` — an is-a reuse relationship; the base part is built first." | 01 and 02 (`SavingsAccount : Account`) |
| **Polymorphism** | "One call site, many behaviours — decided at runtime through the vtable." | `v1->describe()` prints Car's fare; `statement()` prints the right type |

**Interview opener:** "OOP bundles state with the behaviour that guards it. It's better when the domain has entities with identity and lifecycle — vehicles, accounts, jobs. It's NOT automatically better: pure data transforms are simpler procedural."

---

## 2. Compile-time vs runtime polymorphism (the question they WILL ask)

| | Overloading | Overriding |
|---|---|---|
| Signature | same name, **different** parameters | **identical** signature |
| Where | same class | base ↔ derived |
| Resolved | **compile time** (static binding) | **runtime** (vtable, dynamic binding) |
| In your code | `farePerKm()` vs `farePerKm(double surge)` — both in **Car** | `Vehicle::farePerKm()` → `Car::farePerKm()` |

- `override` keyword = compiler-checked overriding. Free safety; always write it.
- ⚠️ **Name hiding (the bug we caught in 01):** if a derived class declares a function name, it **hides ALL base-class overloads of that name**. That's why the surge overload lives in `Car`, not `Vehicle`. If they hand you code with an overload in the base and call it on a derived object — *this* is the reason it fails to compile.

**The one big idea (M10):** `c.describe()` = static dispatch; but `farePerKm()` INSIDE `describe()` is virtual = dynamic dispatch. **"Static dispatch of describe, dynamic dispatch of farePerKm — both mechanisms in one function."** That's why a `Vehicle*` still prints the Car's ₹55.

---

## 3. Virtual functions — the vtable in 4 sentences

1. Every class with virtual functions gets one hidden **vtable** — a static array of function pointers.
2. Every **object** carries one hidden **vptr** to its class's vtable, set during construction.
3. `v->farePerKm()` = load vptr → index the slot → call whatever is there (Car's or Truck's).
4. Cost: one indirection. Without `virtual`, the **pointer's type** decides → the Car charges ₹20 through a `Vehicle*` — the bug the keyword prevents.

**Related rapid-fire:**
- *Can a constructor be virtual?* **No** — the vptr doesn't exist yet during construction; the exact type being built must be known.
- *Can a destructor be virtual?* **Yes — and the base class of any polymorphic hierarchy MUST have one** (next section).
- *What is a pure virtual function?* `virtual double farePerKm() = 0;` — no body; makes the class **abstract** (can't instantiate; derived MUST override). "Abstract class = at least one pure virtual; an interface = all functions pure virtual."
- *Early vs late binding?* Early = compile time (overloads, non-virtual calls); late = runtime (virtual).

---

## 4. Constructors & the base call — the syntax you cannot skip

```cpp
Car(string regNo, int seats) : Vehicle(regNo) {   // ": Vehicle(regNo)" builds the BASE part
    this->seats = seats;
}
```
- A Car object = **Vehicle part + Car extras**. The base constructor must run first — the colon syntax is the ONLY way to call it.
- **Construction order: base first, then derived** (Vehicle → Car).
- **Destruction is the exact reverse: ~Car body first, then ~Vehicle — chained automatically by the compiler.**
- `this->x = x;` disambiguates member from parameter.

## 5. The destructor rules — THE trap of the round

1. **Derived destructors never call the base destructor.** The compiler chains them. Writing `~Vehicle()` inside `~Car` would destroy the base part **twice = undefined behaviour**. [A real ION candidate failed exactly here.]
2. **`virtual ~Vehicle()`** makes `delete` through a `Vehicle*` destroy the REAL object first. Non-virtual + delete via base pointer = undefined behaviour / the derived parts leak.
3. Memory rules: forget `delete` = leak; `delete` twice = crash. Decoder answer: *"a senior uses `unique_ptr` — RAII, the object frees itself."*

---

## 6. Object slicing — why we store pointers

```cpp
Vehicle v = Car("DL-01", 4);   // copies ONLY the Vehicle part — seats is sliced off
```
**"Store by pointer, slice by value."** Polymorphism needs pointers/references. That's why the bank uses `Account*` pointing at derived objects.

## 7. Copy behaviour (name-drop level)

- Default copy = member-by-member copy. If a class owns a raw pointer, two objects end up pointing at the same memory → **shallow copy bug**, double-delete on destruction.
- **Rule of Three** (name it): if you define a destructor / copy constructor / copy assignment, you probably need all three. One-line fix for interviews: "manage ownership with smart pointers and the problem disappears."

---

## 8. OOP rapid-fire (self-test — cover the answer column)

| Question | Answer in one breath |
|---|---|
| Why is OOP better? | state+behaviour bundled, encapsulation protects invariants, polymorphism extends without editing |
| When is OOP worse? | pure data transforms / simple scripts — procedural is simpler |
| `private` vs `protected`? | private: only this class; protected: this + subclasses (prefer private + public accessor — protected data couples subclasses to internals) |
| is-a vs has-a? | inheritance vs composition — **prefer composition**; inherit only for true is-a + polymorphism |
| What is a friend function? | an outside function granted access to privates — use sparingly |
| Multiple inheritance + diamond problem? | two bases share a common base → ambiguity; fixed by **virtual inheritance** (name it; C++ avoids the issue in practice — Java forbids it) |
| Abstract class vs interface? | abstract = ≥1 pure virtual, may hold state; interface = all pure virtual, stateless |
| Can we overload the destructor? | No — destructors take no parameters |
| Static member? | belongs to the CLASS, one copy shared by all objects (instance counter example) |
| What does `this` hold? | the address of the current object — that's why `this->x = x` works |

---

## 9. SOLID — one breath each + which of YOUR drills proves it

| Principle | One breath | Your proof |
|---|---|---|
| **S** — Single Responsibility | one class, one reason to change | `Account` does banking, not file IO |
| **O** — Open/Closed | extend by NEW subclasses, not by editing tested code | **the calculator**: `Operation` interface + `map<char, Operation*>`; adding Multiply = new class + register — zero edits. Same as Car joining the fleet |
| **L** — Liskov Substitution | any subclass must work anywhere the base is expected | Car/Truck in a `Vehicle*`; Savings/Current in an `Account*` — `withdraw`'s contract holds, limits differ |
| **I** — Interface Segregation | small focused hooks, not one fat interface | Account exposes `type/interestPerYear/withdrawLimit` — three tiny virtual hooks |
| **D** — Dependency Inversion | depend on interfaces, not concrete classes | `Calculator` calls `ops[symbol]->apply()` — it never knows Add or Multiply |

**If they say "explain SOLID with an example" → use the calculator (Open/Closed) + the fleet (Liskov). Those two you can whiteboard in 3 minutes.**

---

## 10. Design patterns — name-drop level (intent + where in your drills)

| Pattern | Intent (one line) | Where you ALREADY use it |
|---|---|---|
| **State** | object behaviour changes with internal state; transitions are explicit | Traffic light: RED→GREEN→AMBER→RED; ATM/vending machines; message SENT→DELIVERED→READ |
| **Strategy** | swap the algorithm behind one interface | `Operation` in the calculator; `interestPerYear()` variants; fee slabs in parking |
| **Factory** | central function creates the right subclass from input | `Vehicle* createVehicle(string type)` → new Car/Truck; account opening by type |
| **Observer** | one event notifies many listeners | printing system: jam event → notify user + admin; chat fan-out |
| **Singleton** | exactly one instance (config, one parking-lot controller) | the Controller in 04 |
| **Adapter** | wrap an incompatible interface behind the expected one | "our payment gateway wraps the bank's API" — one-liner only |

**Skeletons worth typing (3 min each):**

```cpp
// FACTORY
Vehicle* createVehicle(string type) {
    if (type == "car")   return new Car("GEN-0", 4);
    if (type == "truck") return new Truck("GEN-0");
    return NULL;
}
// main: Vehicle* v = createVehicle("car"); ... delete v;   (virtual dtor makes this safe)

// SINGLETON
class ParkingLot {
    static ParkingLot* instance;
    ParkingLot() { }                          // private constructor
public:
    static ParkingLot* get() {
        if (instance == NULL) instance = new ParkingLot();
        return instance;
    }
};
ParkingLot* ParkingLot::instance = NULL;
// If pushed "problems?": global state, hidden dependencies, harder tests —
// "I'd use dependency injection in a large codebase; Singleton is fine for one true controller."
```

---

## 11. The 60-second spoken summary (recite before you type)

*"OOP bundles state with behaviour. My Vehicle hides `regNo` as private state — encapsulation. Car inherits and builds the base part through the colon syntax — inheritance. `farePerKm` is virtual, so a call through a Vehicle* resolves at runtime through the vtable to the real object's version — runtime polymorphism; the surge version is an overload — compile-time polymorphism. Derived destructors are chained automatically in reverse order — writing the base destructor call manually would destroy the base twice, so we never do; and the base destructor is virtual so delete-through-base-pointer is safe. Storing by value would slice the derived parts off, so we store pointers — a senior would use unique_ptr for RAII. Extending with new vehicle types without touching old code is Open/Closed; that's the design pattern story: State for the traffic light, Strategy for the calculator's operations, Factory for creating vehicles."*

---

## Before you type 01 — 8 checkpoints
- [ ] 4 pillars one-liners + code locations
- [ ] overload vs override table (resolved WHEN and WHERE each lives)
- [ ] vtable in 4 sentences
- [ ] construction order (base first) & destruction order (reverse, auto-chained)
- [ ] **never call base destructor manually** — say why (double destruction = UB)
- [ ] virtual destructor why
- [ ] slicing: store by pointer, slice by value
- [ ] SOLID: O (calculator) + L (fleet) examples ready

**Now open `code/01_vehicle_car.cpp` and type it from memory.**
