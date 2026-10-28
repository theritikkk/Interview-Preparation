# Section B: C++ and OOP — Teach from First Principles

> **Priority: 🔴 CRITICAL** — ION interviews heavily test OOP with live C++ coding.
> Past candidates were asked to rate themselves on C++ (1–10) and then prove it on the spot.

---

## Table of Contents

1. [Classes and Objects](#1-classes-and-objects)
2. [The Four Pillars of OOP](#2-the-four-pillars-of-oop)
3. [Overloading vs Overriding](#3-overloading-vs-overriding)
4. [Compile-Time vs Runtime Polymorphism](#4-compile-time-vs-runtime-polymorphism)
5. [Constructors, Initialization Lists, and Destructors](#5-constructors-initialization-lists-and-destructors)
6. [Virtual Functions and Virtual Destructors](#6-virtual-functions-and-virtual-destructors)
7. [Why Derived Destructors Must NOT Call Base Destructors](#7-why-derived-destructors-must-not-call-base-destructors)
8. [Pointers, References, and Const Correctness](#8-pointers-references-and-const-correctness)
9. [Stack vs Heap](#9-stack-vs-heap)
10. [Inheritance Types — public, protected, private](#10-inheritance-types)
11. [Abstract Classes and Pure Virtual Functions](#11-abstract-classes-and-pure-virtual-functions)
12. [Composition vs Inheritance](#12-composition-vs-inheritance)
13. [RAII and Smart Pointers](#13-raii-and-smart-pointers)
14. [Common C++ Bugs](#14-common-cpp-bugs)
15. [⭐ Complete Vehicle/Car Example (ION Interview Scenario)](#15-complete-vehiclecar-example)
16. [Common Interview Mistakes (ION-Specific)](#16-common-interview-mistakes)

---

## 1. Classes and Objects

**Simple Explanation:**
A class is a user-defined blueprint that bundles data (member variables) and behavior (member functions) together. An object is a concrete instance of that class — it occupies memory and holds actual values. Think of a class as the architectural plan for a house; an object is the actual house you live in.

**Why It Matters:**
Classes are the foundation of every OOP answer. If you can't write a clean class from scratch on a whiteboard, the interview is over before it starts.

**Interview Answer (say aloud):**
> "A class is a blueprint that defines data members and member functions. An object is a runtime instance of that class. Classes enable encapsulation by bundling related data and operations, and they serve as the building block for inheritance and polymorphism."

```cpp
#include <iostream>
#include <string>
using namespace std;

class BankAccount {
private:
    string owner;
    double balance;

public:
    // Constructor with initialization list
    BankAccount(const string& owner, double balance)
        : owner(owner), balance(balance) {}

    void deposit(double amount) {
        if (amount > 0) balance += amount;
    }

    double getBalance() const { return balance; }

    void display() const {
        cout << owner << "'s balance: $" << balance << endl;
    }
};

int main() {
    BankAccount acc("Ritik", 1000.0);  // Object on stack
    acc.deposit(500.0);
    acc.display();  // Ritik's balance: $1500
    return 0;
}
```

**Common Mistakes:**
- Forgetting semicolon after class closing brace `};`
- Making all members `public` (defeats encapsulation)
- Confusing class declaration with object instantiation

**Likely Follow-ups:**
- *"What's the difference between a struct and a class in C++?"* → Default access: struct is public, class is private. Otherwise identical.
- *"What is the size of an empty class?"* → 1 byte (so each object gets a unique address).

---

## 2. The Four Pillars of OOP

### Quick Reference Table

| Pillar | One-Liner Definition | C++ Mechanism |
|---|---|---|
| **Encapsulation** | Bundling data + methods, restricting direct access | `private`/`protected` + getters/setters |
| **Abstraction** | Hiding implementation complexity, exposing only the interface | Abstract classes, pure virtual functions |
| **Inheritance** | Deriving a new class from an existing one to reuse/extend behavior | `class Car : public Vehicle {}` |
| **Polymorphism** | One interface, multiple behaviors | Virtual functions (runtime), overloading (compile-time) |

### 2.1 Encapsulation

**Simple Explanation:**
Encapsulation means wrapping data and the functions that operate on it into a single unit (a class), and controlling access through access specifiers (`private`, `protected`, `public`). It prevents outside code from directly modifying internal state.

**Why It Matters:**
Without encapsulation, any part of the codebase can corrupt an object's data. Encapsulation enforces invariants — e.g., a bank balance can never go negative if the only way to modify it is through a `withdraw()` method that checks.

**Interview Answer:**
> "Encapsulation bundles data and methods together and uses access specifiers to hide internal state. This protects data integrity — for example, you can't set a negative balance if `balance` is private and all modifications go through validated methods. The difference from plain C functions is that C has no access control; any function can access any struct field."

```cpp
#include <iostream>
using namespace std;

class Temperature {
private:
    double celsius;  // Internal representation — hidden

public:
    Temperature(double c) : celsius(c) {}

    // Controlled access — enforces valid range
    void setCelsius(double c) {
        if (c >= -273.15)  // Absolute zero check
            celsius = c;
    }

    double getCelsius() const { return celsius; }
    double getFahrenheit() const { return celsius * 9.0 / 5.0 + 32.0; }
};

int main() {
    Temperature t(100.0);
    // t.celsius = -500;  // ERROR: private — encapsulation prevents this
    t.setCelsius(37.0);   // OK: goes through validation
    cout << t.getFahrenheit() << "°F" << endl;  // 98.6°F
    return 0;
}
```

**Common Mistakes:**
- Making everything `public` "for convenience" — defeats the purpose
- Confusing encapsulation with abstraction

**Likely Follow-up:**
- *"How is encapsulation different from just using C functions with structs?"* → In C, any function can read/write any struct field. Encapsulation in C++ uses access specifiers to enforce that only the class's own methods can touch private data.

---

### 2.2 Abstraction

**Simple Explanation:**
Abstraction hides complex implementation details and exposes only the essential interface. A user of the class doesn't need to know *how* something works, only *what* it does. In C++, this is achieved through abstract classes and pure virtual functions.

**Interview Answer:**
> "Abstraction means showing only the relevant interface to the user while hiding the implementation. For example, when you call `sort()`, you don't care whether it uses quicksort or mergesort internally. In C++, we use abstract classes with pure virtual functions to define interfaces that derived classes must implement."

```cpp
#include <iostream>
using namespace std;

// Abstract class — defines WHAT, not HOW
class Shape {
public:
    virtual double area() const = 0;       // Pure virtual
    virtual void display() const = 0;      // Pure virtual
    virtual ~Shape() = default;
};

class Circle : public Shape {
    double radius;
public:
    Circle(double r) : radius(r) {}
    double area() const override { return 3.14159 * radius * radius; }
    void display() const override {
        cout << "Circle area: " << area() << endl;
    }
};

int main() {
    // Shape s;        // ERROR: cannot instantiate abstract class
    Circle c(5.0);
    c.display();       // Circle area: 78.5398
    return 0;
}
```

---

### 2.3 Inheritance

**Simple Explanation:**
Inheritance lets a derived class acquire properties and behaviors of a base class, promoting code reuse. The derived class can add new members or override existing behavior. It models the "is-a" relationship.

**Interview Answer:**
> "Inheritance allows a derived class to reuse the data and methods of a base class. A Car 'is-a' Vehicle, so Car inherits from Vehicle and gets its fields and methods for free, then adds or overrides what's specific to cars. C++ supports single, multiple, multilevel, hierarchical, and hybrid inheritance."

```cpp
#include <iostream>
#include <string>
using namespace std;

class Animal {
protected:
    string name;
public:
    Animal(const string& name) : name(name) {}
    void eat() const { cout << name << " is eating." << endl; }
    virtual ~Animal() = default;
};

class Dog : public Animal {
public:
    Dog(const string& name) : Animal(name) {}
    void bark() const { cout << name << " says Woof!" << endl; }
};

int main() {
    Dog d("Buddy");
    d.eat();   // Inherited from Animal
    d.bark();  // Dog's own method
    return 0;
}
```

---

### 2.4 Polymorphism

**Simple Explanation:**
Polymorphism means "many forms" — a single interface can behave differently depending on the context. Compile-time polymorphism is achieved through function/operator overloading; runtime polymorphism is achieved through virtual functions and late binding.

**Interview Answer:**
> "Polymorphism lets us write code that works with the base class interface but executes the derived class's implementation. Compile-time polymorphism uses function overloading or templates — the compiler decides which function to call. Runtime polymorphism uses virtual functions — the decision happens at runtime via the vtable."

*(Full code example in Section 3 and the Vehicle/Car mega-example in Section 15.)*

---

## 3. Overloading vs Overriding

> [!IMPORTANT]
> ION interviewers specifically asked candidates to show BOTH compile-time and runtime polymorphism using a Vehicle/Car hierarchy. Know the difference cold.

### Quick Comparison

| Feature | Overloading (Compile-Time) | Overriding (Runtime) |
|---|---|---|
| **Binding** | Compile-time (static) | Runtime (dynamic) |
| **Where** | Same class or different scopes | Base + derived class |
| **Signature** | Different parameter list | SAME signature |
| **Keyword** | None | `virtual` + `override` |
| **Mechanism** | Name mangling | vtable lookup |

```cpp
#include <iostream>
#include <string>
using namespace std;

class Vehicle {
protected:
    string brand;
    int year;

public:
    Vehicle(const string& brand, int year) : brand(brand), year(year) {}

    // --- COMPILE-TIME POLYMORPHISM: Function Overloading ---
    // Same function name, different parameters
    void start() const {
        cout << brand << " vehicle starting with key." << endl;
    }
    void start(const string& mode) const {
        cout << brand << " vehicle starting in " << mode << " mode." << endl;
    }

    // --- RUNTIME POLYMORPHISM: Virtual Function ---
    virtual void honk() const {
        cout << brand << " vehicle: *generic honk*" << endl;
    }

    virtual ~Vehicle() = default;
};

class Car : public Vehicle {
    int numDoors;

public:
    Car(const string& brand, int year, int doors)
        : Vehicle(brand, year), numDoors(doors) {}

    // OVERRIDING: Same signature as base, different behavior
    void honk() const override {
        cout << brand << " car: *beep beep!*" << endl;
    }
};

class Truck : public Vehicle {
public:
    Truck(const string& brand, int year)
        : Vehicle(brand, year) {}

    void honk() const override {
        cout << brand << " truck: *HOOOONK!*" << endl;
    }
};

int main() {
    // Compile-time polymorphism — compiler picks based on arguments
    Car myCar("Toyota", 2024, 4);
    myCar.start();             // start() — no args
    myCar.start("sport");     // start(string) — one arg

    cout << "---" << endl;

    // Runtime polymorphism — decided at runtime via vtable
    Vehicle* vehicles[] = {
        new Car("Honda", 2023, 4),
        new Truck("Volvo", 2022)
    };

    for (auto* v : vehicles) {
        v->honk();  // Calls Car::honk or Truck::honk based on actual type
        delete v;   // Safe: virtual destructor in base
    }

    return 0;
}
```

**Output:**
```
Toyota vehicle starting with key.
Toyota vehicle starting in sport mode.
---
Honda car: *beep beep!*
Volvo truck: *HOOOONK!*
```

**Common Mistakes:**
- Saying "overriding" when you mean "overloading" (different parameters = overloading)
- Forgetting `virtual` in base → no polymorphism, base version always called
- Forgetting `override` in derived → compiles but silently creates a new function if signature is wrong

**Likely Follow-ups:**
- *"Can you overload across base and derived?"* → Not directly; derived class hides base overloads unless you use `using Base::funcName;`
- *"What's the difference between overloading and default arguments?"* → Overloading creates separate functions; default arguments are a single function with optional parameters.

---

## 4. Compile-Time vs Runtime Polymorphism

### 4.1 Compile-Time Polymorphism

Resolved during compilation. The compiler picks the correct function based on the call signature.

**Mechanisms:**
1. **Function Overloading** — same name, different parameter types/count
2. **Operator Overloading** — redefine operators for user-defined types
3. **Templates** — generic programming (function/class templates)

```cpp
#include <iostream>
using namespace std;

class Vector2D {
public:
    double x, y;
    Vector2D(double x, double y) : x(x), y(y) {}

    // Operator overloading — compile-time polymorphism
    Vector2D operator+(const Vector2D& other) const {
        return Vector2D(x + other.x, y + other.y);
    }

    void print() const { cout << "(" << x << ", " << y << ")" << endl; }
};

// Function template — compile-time polymorphism
template <typename T>
T maxOf(T a, T b) {
    return (a > b) ? a : b;
}

int main() {
    Vector2D a(1, 2), b(3, 4);
    Vector2D c = a + b;     // operator+ resolved at compile time
    c.print();               // (4, 6)

    cout << maxOf(10, 20) << endl;        // int version
    cout << maxOf(3.14, 2.72) << endl;    // double version
    return 0;
}
```

### 4.2 Runtime Polymorphism — The vtable

**Interview Answer:**
> "When a class has at least one virtual function, the compiler creates a vtable — an array of function pointers. Each object of that class gets a hidden vptr that points to the vtable. When you call a virtual function through a base pointer, the program follows the vptr → vtable → actual function. This is why virtual calls are slightly slower than direct calls — there's an extra indirection."

```
                     ┌──────────────────────┐
 Base* ptr ──────►   │   vptr ──────────────┼──► vtable for Car
                     │   brand = "Toyota"   │     ┌──────────────────┐
                     │   year = 2024        │     │ &Car::honk       │
                     │   numDoors = 4       │     │ &Car::~Car       │
                     └──────────────────────┘     └──────────────────┘
```

**Key Points for Interview:**
- vtable is per-class (not per-object)
- vptr is per-object (adds sizeof(pointer) to each object)
- Only `virtual` functions go through the vtable
- Non-virtual calls are resolved at compile time (direct call, no vtable)

**Likely Follow-ups:**
- *"What overhead does virtual add?"* → One pointer (vptr) per object, one vtable per class, one extra indirection per virtual call.
- *"Can constructors be virtual?"* → No. The object's type must be known to construct it. But destructors absolutely should be virtual in polymorphic base classes.

---

## 5. Constructors, Initialization Lists, and Destructors

### 5.1 Constructor Types

| Type | Syntax | When Called |
|---|---|---|
| Default | `Foo()` | `Foo f;` |
| Parameterized | `Foo(int x)` | `Foo f(42);` |
| Copy | `Foo(const Foo& other)` | `Foo f2 = f1;` |
| Move | `Foo(Foo&& other)` | `Foo f2 = std::move(f1);` |

### 5.2 Initialization Lists (MUST KNOW)

**Simple Explanation:**
Initialization lists initialize member variables *before* the constructor body runs. They are required for `const` members, reference members, and base class constructors. They are also more efficient than assignment in the constructor body.

**Interview Answer:**
> "Initialization lists initialize members directly at construction time rather than default-constructing then assigning. They're required for const members, references, and calling base-class constructors. They're also more efficient because they avoid a redundant default construction."

```cpp
#include <iostream>
#include <string>
using namespace std;

class Engine {
    int horsepower;
    const string type;      // const — MUST use init list
    int& refToHP;           // reference — MUST use init list

public:
    // ✅ Correct: Initialization list
    Engine(int hp, const string& t)
        : horsepower(hp), type(t), refToHP(horsepower)
    {
        cout << "Engine created: " << type << " " << horsepower << "hp" << endl;
    }

    // ❌ WRONG: This would NOT compile for const/ref members
    // Engine(int hp, const string& t) {
    //     horsepower = hp;    // OK but less efficient
    //     type = t;           // ERROR: const can't be assigned after init
    //     refToHP = horsepower; // ERROR: reference must be bound at init
    // }
};

int main() {
    Engine e(200, "V6");
    return 0;
}
```

**Initialization Order Rule:**
Members are initialized in the order they are *declared* in the class, NOT the order in the initialization list. Mismatched order causes warnings and potential bugs.

### 5.3 Destructors

**Simple Explanation:**
A destructor is called automatically when an object goes out of scope (stack) or is `delete`d (heap). It cleans up resources. The destructor chain goes derived → base automatically.

```cpp
#include <iostream>
using namespace std;

class Resource {
    int* data;
public:
    Resource(int val) : data(new int(val)) {
        cout << "Resource acquired: " << *data << endl;
    }
    ~Resource() {
        cout << "Resource released: " << *data << endl;
        delete data;
    }
};

int main() {
    {
        Resource r(42);
        // r goes out of scope here → destructor called automatically
    }
    cout << "After scope" << endl;
    return 0;
}
// Output:
// Resource acquired: 42
// Resource released: 42
// After scope
```

**Common Mistakes:**
- Forgetting to `delete` dynamically allocated memory in the destructor
- Calling the base class destructor manually (see Section 7)
- Not making the base destructor `virtual` when using polymorphism (see Section 6)

---

## 6. Virtual Functions and Virtual Destructors

> [!CAUTION]
> This was tested directly in an ION interview. A candidate lost points for not making the base destructor virtual.

### 6.1 The Problem — Memory Leak Without Virtual Destructor

When you delete a derived object through a base pointer, and the base destructor is NOT virtual, only the base destructor runs. The derived destructor is skipped → **resource leak / undefined behavior**.

```cpp
#include <iostream>
using namespace std;

// ❌ BUG: Non-virtual destructor in polymorphic base class
class Base {
public:
    Base()  { cout << "Base constructed" << endl; }
    ~Base() { cout << "Base destroyed" << endl; }  // NOT virtual!
};

class Derived : public Base {
    int* data;
public:
    Derived() : data(new int[1000]) {
        cout << "Derived constructed (allocated 1000 ints)" << endl;
    }
    ~Derived() {
        delete[] data;
        cout << "Derived destroyed (freed 1000 ints)" << endl;
    }
};

int main() {
    Base* ptr = new Derived();
    delete ptr;  // ⚠️ ONLY calls ~Base()! Derived::data is LEAKED!
    return 0;
}
```

**Output (BUGGY):**
```
Base constructed
Derived constructed (allocated 1000 ints)
Base destroyed
// ← Derived destructor NEVER called! Memory leak!
```

### 6.2 The Fix — Virtual Destructor

```cpp
#include <iostream>
using namespace std;

// ✅ FIXED: Virtual destructor
class Base {
public:
    Base()          { cout << "Base constructed" << endl; }
    virtual ~Base() { cout << "Base destroyed" << endl; }  // VIRTUAL!
};

class Derived : public Base {
    int* data;
public:
    Derived() : data(new int[1000]) {
        cout << "Derived constructed (allocated 1000 ints)" << endl;
    }
    ~Derived() override {
        delete[] data;
        cout << "Derived destroyed (freed 1000 ints)" << endl;
    }
};

int main() {
    Base* ptr = new Derived();
    delete ptr;  // ✅ Calls ~Derived() THEN ~Base() — correct!
    return 0;
}
```

**Output (CORRECT):**
```
Base constructed
Derived constructed (allocated 1000 ints)
Derived destroyed (freed 1000 ints)
Base destroyed
```

**Interview Answer:**
> "A polymorphic base class must have a virtual destructor. Without it, deleting a derived object through a base pointer only calls the base destructor — the derived destructor is skipped, causing resource leaks. The virtual keyword makes the delete operation look up the vtable and call the correct destructor chain: derived first, then base."

**The Rule:**
> If a class has **any** virtual function, its destructor **must** be virtual.

---

## 7. Why Derived Destructors Must NOT Call Base Destructors

> [!WARNING]
> An ION candidate made this exact mistake. They manually called the parent destructor from the child destructor.

### The Rule

**The C++ runtime automatically calls the base destructor after the derived destructor finishes.** You should NEVER call `Base::~Base()` from inside `~Derived()`.

### What Happens If You Do

```cpp
#include <iostream>
using namespace std;

class Base {
public:
    int* baseData;
    Base() : baseData(new int(10)) {
        cout << "Base constructed" << endl;
    }
    virtual ~Base() {
        cout << "Base destroyed, deleting baseData" << endl;
        delete baseData;
    }
};

class Derived : public Base {
    int* derivedData;
public:
    Derived() : derivedData(new int(20)) {
        cout << "Derived constructed" << endl;
    }
    ~Derived() override {
        cout << "Derived destroyed" << endl;
        delete derivedData;

        // ❌❌❌ NEVER DO THIS — Base::~Base() is called AUTOMATICALLY
        // Base::~Base();   // DOUBLE DESTRUCTION! baseData deleted TWICE!
        //                  // → undefined behavior, likely a crash
    }
};

int main() {
    Derived d;  // When d goes out of scope:
                // 1. ~Derived() runs automatically
                // 2. ~Base() runs automatically AFTER ~Derived()
    return 0;
}
```

**Output (CORRECT — no manual base destructor call):**
```
Base constructed
Derived constructed
Derived destroyed
Base destroyed, deleting baseData
```

**Interview Answer:**
> "You should never manually call the base destructor from a derived destructor. C++ guarantees that destructors are called in reverse order of construction — derived first, then base — automatically. If you call the base destructor manually, the base's resources get cleaned up twice: once when you call it, and again when the compiler calls it automatically. This causes double-free, which is undefined behavior."

**Destruction Order:**
```
Construction: Base → Derived (base first)
Destruction:  Derived → Base (reverse order, automatic)
```

---

## 8. Pointers, References, and Const Correctness

### 8.1 Pass by Value vs Reference vs Pointer

```cpp
#include <iostream>
using namespace std;

void byValue(int x)      { x = 100; }     // Copy — original unchanged
void byReference(int& x) { x = 100; }     // Alias — modifies original
void byPointer(int* x)   { *x = 100; }    // Address — modifies original

int main() {
    int a = 1, b = 1, c = 1;
    byValue(a);       // a still 1
    byReference(b);   // b is now 100
    byPointer(&c);    // c is now 100
    cout << a << " " << b << " " << c << endl;  // 1 100 100
    return 0;
}
```

### 8.2 References vs Pointers

| Feature | Reference | Pointer |
|---|---|---|
| Can be null? | No | Yes (`nullptr`) |
| Can be reassigned? | No (always same referent) | Yes |
| Syntax | `int& r = x;` | `int* p = &x;` |
| Dereferencing | Automatic | Manual (`*p`) |
| Use when | You always have a valid object | Object may not exist, or ownership transfer |

### 8.3 Const Correctness

```cpp
#include <iostream>
#include <string>
using namespace std;

class Employee {
    string name;
    double salary;

public:
    Employee(const string& name, double salary)
        : name(name), salary(salary) {}

    // const method — promises not to modify the object
    string getName() const { return name; }
    double getSalary() const { return salary; }

    // non-const method — can modify the object
    void giveRaise(double pct) { salary *= (1 + pct / 100.0); }
};

// const reference parameter — efficient, no copy, no modification
void printEmployee(const Employee& emp) {
    cout << emp.getName() << ": $" << emp.getSalary() << endl;
    // emp.giveRaise(10);  // ERROR: can't call non-const method on const ref
}

int main() {
    Employee e("Ritik", 80000);
    printEmployee(e);
    e.giveRaise(10);
    printEmployee(e);
    return 0;
}
```

**Interview Answer:**
> "Const correctness means marking functions and parameters as `const` when they don't need to modify data. Const methods promise not to modify the object's state — they can be called on const references. Passing by const reference is the standard way to pass objects efficiently without allowing modification."

**Common Mistakes:**
- Forgetting `const` on getter methods → can't call them on const references
- Taking `string` by value instead of `const string&` in function parameters

---

## 9. Stack vs Heap

### Quick Comparison

| Feature | Stack | Heap |
|---|---|---|
| **Allocation** | Automatic | Manual (`new` / `malloc`) |
| **Deallocation** | Automatic (scope exit) | Manual (`delete` / `free`) or smart pointers |
| **Speed** | Very fast (pointer bump) | Slower (memory manager overhead) |
| **Size** | Limited (~1-8 MB) | Large (limited by RAM) |
| **Lifetime** | Until scope ends | Until explicitly freed |
| **Fragmentation** | None | Possible |

```cpp
#include <iostream>
using namespace std;

class Widget {
public:
    int id;
    Widget(int id) : id(id) { cout << "Widget " << id << " created" << endl; }
    ~Widget() { cout << "Widget " << id << " destroyed" << endl; }
};

int main() {
    // STACK allocation — automatic lifetime
    Widget w1(1);  // Created now, destroyed when main() exits

    {
        Widget w2(2);  // Created here
    }  // w2 destroyed here — out of scope

    // HEAP allocation — manual lifetime
    Widget* w3 = new Widget(3);  // Created here
    // ... w3 lives until we explicitly delete it
    delete w3;  // Destroyed here

    // HEAP with smart pointer — automatic cleanup, best of both
    // (See Section 13)

    return 0;
}
```

**Output:**
```
Widget 1 created
Widget 2 created
Widget 2 destroyed
Widget 3 created
Widget 3 destroyed
Widget 1 destroyed
```

**Interview Answer:**
> "Stack allocation is fast and automatic — objects are destroyed when they go out of scope. Heap allocation with `new` gives you manual control over lifetime, but you're responsible for `delete`-ing it. In modern C++, we prefer stack allocation and smart pointers over raw `new`/`delete` to avoid memory leaks."

**When to use heap:**
- Object needs to outlive the current scope
- Object is too large for the stack
- Polymorphism requires base-class pointers to derived objects
- Size determined at runtime

---

## 10. Inheritance Types

### Access Table

| Base Member | `public` Inheritance | `protected` Inheritance | `private` Inheritance |
|---|---|---|---|
| `public` | → `public` | → `protected` | → `private` |
| `protected` | → `protected` | → `protected` | → `private` |
| `private` | → inaccessible | → inaccessible | → inaccessible |

> [!NOTE]
> `public` inheritance = "is-a" relationship (Car **is a** Vehicle).
> `private` inheritance = "is-implemented-in-terms-of" (rarely used, prefer composition).
> `protected` inheritance = almost never used in practice.

```cpp
#include <iostream>
using namespace std;

class Base {
public:
    int pub = 1;
protected:
    int prot = 2;
private:
    int priv = 3;
};

// Public inheritance — most common, models "is-a"
class PublicDerived : public Base {
public:
    void test() {
        cout << pub << endl;   // OK — still public
        cout << prot << endl;  // OK — still protected
        // cout << priv;       // ERROR — private is never accessible
    }
};

// Private inheritance — models "implemented-in-terms-of"
class PrivateDerived : private Base {
public:
    void test() {
        cout << pub << endl;   // OK — but now private in PrivateDerived
        cout << prot << endl;  // OK — but now private in PrivateDerived
    }
};

int main() {
    PublicDerived pd;
    cout << pd.pub << endl;   // OK — public stays public
    // cout << pd.prot;       // ERROR — protected

    PrivateDerived pvd;
    // cout << pvd.pub;       // ERROR — pub became private in PrivateDerived
    pvd.test();               // OK — member function can access

    return 0;
}
```

**Interview Answer:**
> "Public inheritance means public members stay public and protected stay protected — it models 'is-a'. Private inheritance makes everything private in the derived class — it models 'implemented-in-terms-of' but composition is usually preferred. Private members of the base are never directly accessible in any derived class."

**Likely Follow-ups:**
- *"What is the diamond problem?"* → When a class inherits from two classes that share a common base, the common base is duplicated. Solved with virtual inheritance: `class B : virtual public A`.
- *"Which inheritance type is default?"* → `private` for classes, `public` for structs.

---

## 11. Abstract Classes and Pure Virtual Functions

**Simple Explanation:**
An abstract class has at least one pure virtual function (`= 0`). You cannot instantiate it. It defines an interface that derived classes must implement. This is C++'s version of Java's interface concept.

```cpp
#include <iostream>
#include <vector>
#include <memory>
using namespace std;

// Abstract class — defines an interface
class Drawable {
public:
    virtual void draw() const = 0;                    // Pure virtual
    virtual double getArea() const = 0;               // Pure virtual
    virtual ~Drawable() = default;                     // Virtual destructor
};

// Concrete class — implements ALL pure virtual functions
class Rectangle : public Drawable {
    double w, h;
public:
    Rectangle(double w, double h) : w(w), h(h) {}

    void draw() const override {
        cout << "Drawing rectangle " << w << "x" << h << endl;
    }
    double getArea() const override { return w * h; }
};

class Circle : public Drawable {
    double r;
public:
    Circle(double r) : r(r) {}

    void draw() const override {
        cout << "Drawing circle r=" << r << endl;
    }
    double getArea() const override { return 3.14159 * r * r; }
};

int main() {
    // Drawable d;  // ERROR: cannot instantiate abstract class

    vector<unique_ptr<Drawable>> shapes;
    shapes.push_back(make_unique<Rectangle>(5, 3));
    shapes.push_back(make_unique<Circle>(4));

    for (const auto& s : shapes) {
        s->draw();
        cout << "  Area: " << s->getArea() << endl;
    }
    return 0;
}
```

**Interview Answer:**
> "An abstract class has at least one pure virtual function declared with `= 0`. You can't create objects of it — it serves as a contract that derived classes must fulfill. If a derived class doesn't implement all pure virtual functions, it also becomes abstract. This is how we define interfaces in C++."

---

## 12. Composition vs Inheritance

### "Has-a" vs "Is-a"

| Aspect | Inheritance | Composition |
|---|---|---|
| Relationship | "is-a" | "has-a" |
| Coupling | Tight (derived depends on base internals) | Loose (uses interface only) |
| Flexibility | Fixed at compile time | Can swap components at runtime |
| Example | `Car : Vehicle` | `Car has an Engine` |
| Preference | When type substitution is needed | **Default choice** — prefer over inheritance |

```cpp
#include <iostream>
#include <string>
using namespace std;

// Component class
class Engine {
    int hp;
public:
    Engine(int hp) : hp(hp) {}
    void start() const { cout << hp << "hp engine started" << endl; }
    int getHP() const { return hp; }
};

// Composition: Car HAS-A Engine (not IS-A Engine)
class Car {
    string brand;
    Engine engine;  // Composition — Engine is a member

public:
    Car(const string& brand, int hp)
        : brand(brand), engine(hp) {}

    void start() const {
        cout << brand << ": ";
        engine.start();  // Delegates to Engine
    }
};

int main() {
    Car c("BMW", 300);
    c.start();  // BMW: 300hp engine started
    return 0;
}
```

**Interview Answer:**
> "Favor composition over inheritance. Inheritance creates tight coupling — changing the base class can break all derived classes. Composition is more flexible: a Car 'has-a' Engine. You can swap the Engine implementation without changing Car. Use inheritance only when you need polymorphism or there's a genuine 'is-a' relationship."

**Likely Follow-up:**
- *"When would you use inheritance over composition?"* → When you need runtime polymorphism via base pointers, or when the derived class truly is a specialization of the base (e.g., `SavingsAccount : BankAccount`).

---

## 13. RAII and Smart Pointers

### 13.1 RAII (Resource Acquisition Is Initialization)

**Simple Explanation:**
RAII ties resource management to object lifetime. A resource (memory, file handle, lock) is acquired in the constructor and released in the destructor. When the object goes out of scope, cleanup happens automatically. No leaks.

### 13.2 Smart Pointers

| Smart Pointer | Ownership | Use Case |
|---|---|---|
| `unique_ptr` | Sole owner | Default choice. Cannot be copied, only moved. |
| `shared_ptr` | Shared ownership | Multiple owners. Reference counted. |
| `weak_ptr` | Non-owning observer | Break circular references in `shared_ptr`. |

```cpp
#include <iostream>
#include <memory>
using namespace std;

class DatabaseConnection {
    string dbName;
public:
    DatabaseConnection(const string& name) : dbName(name) {
        cout << "Connected to " << dbName << endl;
    }
    ~DatabaseConnection() {
        cout << "Disconnected from " << dbName << endl;
    }
    void query(const string& sql) const {
        cout << "Executing on " << dbName << ": " << sql << endl;
    }
};

int main() {
    // unique_ptr — sole ownership, auto-deletes
    {
        auto db = make_unique<DatabaseConnection>("UserDB");
        db->query("SELECT * FROM users");
    }  // ← Automatically disconnected here

    cout << "---" << endl;

    // shared_ptr — shared ownership, ref-counted
    shared_ptr<DatabaseConnection> db1;
    {
        auto db2 = make_shared<DatabaseConnection>("OrderDB");
        db1 = db2;  // Both own it, refcount = 2
        cout << "Ref count: " << db1.use_count() << endl;  // 2
    }  // db2 destroyed, refcount = 1, NOT disconnected yet

    cout << "Ref count: " << db1.use_count() << endl;  // 1
    db1.reset();  // refcount = 0, NOW disconnected

    return 0;
}
```

**Output:**
```
Connected to UserDB
Executing on UserDB: SELECT * FROM users
Disconnected from UserDB
---
Connected to OrderDB
Ref count: 2
Ref count: 1
Disconnected from OrderDB
```

**Interview Answer:**
> "RAII means binding resource lifetime to object lifetime — acquire in the constructor, release in the destructor. Smart pointers are the RAII wrapper for heap memory. `unique_ptr` for sole ownership — it can't be copied, only moved. `shared_ptr` for shared ownership with reference counting. In modern C++, you should almost never write `new` or `delete` directly."

**Common Mistakes:**
- Using `shared_ptr` everywhere instead of `unique_ptr` (overuse of shared ownership)
- Creating circular references with `shared_ptr` (use `weak_ptr` to break cycles)
- Calling `new` without wrapping in a smart pointer

---

## 14. Common C++ Bugs

### 14.1 Dangling Pointer

```cpp
int* createValue() {
    int x = 42;
    return &x;  // ❌ x is destroyed when function returns
}               //    returned pointer is dangling!

// Fix: allocate on heap or return by value
int createValueFixed() {
    int x = 42;
    return x;  // ✅ Return by value
}
```

### 14.2 Memory Leak

```cpp
void leak() {
    int* arr = new int[1000];
    // ... code that might throw or return early ...
    // delete[] arr;  // Never reached → LEAK
}

// Fix: Use RAII
void noLeak() {
    auto arr = make_unique<int[]>(1000);
    // ... if exception or early return, unique_ptr cleans up
}
```

### 14.3 Object Slicing

```cpp
class Base {
public:
    virtual void whoAmI() { cout << "Base" << endl; }
};

class Derived : public Base {
    int extra = 42;
public:
    void whoAmI() override { cout << "Derived, extra=" << extra << endl; }
};

void slicingProblem(Base b) {  // ❌ Passed by VALUE → Derived part is sliced off!
    b.whoAmI();  // Always prints "Base" even if a Derived was passed
}

void noSlicing(Base& b) {     // ✅ Passed by REFERENCE → polymorphism works
    b.whoAmI();  // Prints "Derived, extra=42" if a Derived was passed
}
```

### 14.4 Forgetting Virtual Destructor

*(See Section 6 — causes memory leak when deleting derived through base pointer)*

### 14.5 Double Free

```cpp
void doubleFree() {
    int* p = new int(42);
    int* q = p;      // Both point to same memory
    delete p;         // Freed
    delete q;         // ❌ DOUBLE FREE — undefined behavior!
}

// Fix: Use unique_ptr (enforces single ownership)
void noDoubleFree() {
    auto p = make_unique<int>(42);
    // Cannot accidentally copy unique_ptr
}
```

### Bug Quick Reference

| Bug | Cause | Prevention |
|---|---|---|
| Dangling pointer | Returning address of local variable | Return by value, use smart pointers |
| Memory leak | Forgetting `delete` | RAII, smart pointers |
| Object slicing | Passing derived by value to base param | Pass by reference or pointer |
| Missing virtual dtor | Non-virtual base destructor | Always make base dtor virtual |
| Double free | Two raw pointers to same memory | `unique_ptr` (single ownership) |
| Use after free | Accessing memory after `delete` | Smart pointers, nullify after delete |

---

## 15. ⭐ Complete Vehicle/Car Example (ION Interview Scenario)

> [!IMPORTANT]
> **This exact scenario was asked in a real ION interview.** A candidate was asked to write Vehicle and Car classes demonstrating OOP principles. Study this until you can type it from memory.

This single program demonstrates ALL of the following:
- ✅ Class hierarchy with public inheritance
- ✅ Constructor with initialization list
- ✅ Virtual function for runtime polymorphism
- ✅ Function overloading for compile-time polymorphism
- ✅ Virtual destructor on base class
- ✅ Proper destructor chain (derived → base, automatic)
- ✅ Creating objects on stack and heap
- ✅ Deleting through base pointer safely
- ✅ `override` keyword
- ✅ `const` correctness
- ✅ Encapsulation with private members and public interface

```cpp
#include <iostream>
#include <string>
#include <vector>
using namespace std;

// ═══════════════════════════════════════════════════════════════
// BASE CLASS: Vehicle
// ═══════════════════════════════════════════════════════════════
class Vehicle {
private:
    string brand;
    int year;

protected:
    string getBrand() const { return brand; }

public:
    // Constructor with initialization list
    Vehicle(const string& brand, int year)
        : brand(brand), year(year)
    {
        cout << "[Vehicle] Constructed: " << brand << " (" << year << ")" << endl;
    }

    // ──── COMPILE-TIME POLYMORPHISM: Function Overloading ────
    void fuel() const {
        cout << brand << " is refueling with default fuel." << endl;
    }

    void fuel(const string& fuelType) const {
        cout << brand << " is refueling with " << fuelType << "." << endl;
    }

    void fuel(const string& fuelType, double liters) const {
        cout << brand << " is refueling with " << liters
             << "L of " << fuelType << "." << endl;
    }

    // ──── RUNTIME POLYMORPHISM: Virtual Functions ────
    virtual void start() const {
        cout << brand << " vehicle: starting (generic)." << endl;
    }

    virtual void displayInfo() const {
        cout << "Vehicle: " << brand << ", Year: " << year << endl;
    }

    // ──── VIRTUAL DESTRUCTOR (CRITICAL!) ────
    // Without this, deleting a Car through a Vehicle* would leak memory
    virtual ~Vehicle() {
        cout << "[Vehicle] Destroyed: " << brand << endl;
    }
};

// ═══════════════════════════════════════════════════════════════
// DERIVED CLASS: Car
// ═══════════════════════════════════════════════════════════════
class Car : public Vehicle {
private:
    int numDoors;
    string* features;  // Raw pointer to demonstrate destructor importance
    int featureCount;

public:
    // Constructor — calls base constructor via initialization list
    Car(const string& brand, int year, int doors, int featCount)
        : Vehicle(brand, year),       // Base class init
          numDoors(doors),
          features(new string[featCount]),  // Heap allocation
          featureCount(featCount)
    {
        cout << "[Car] Constructed: " << getBrand()
             << " with " << numDoors << " doors" << endl;
    }

    void setFeature(int index, const string& feat) {
        if (index >= 0 && index < featureCount)
            features[index] = feat;
    }

    // ──── RUNTIME POLYMORPHISM: Override base virtual functions ────
    void start() const override {
        cout << getBrand() << " car: push button start, engine roars!" << endl;
    }

    void displayInfo() const override {
        cout << "Car: " << getBrand() << ", Doors: " << numDoors
             << ", Features: " << featureCount << endl;
    }

    // ──── DESTRUCTOR ────
    // Cleans up Car-specific resources
    // Does NOT call ~Vehicle() — that happens AUTOMATICALLY
    ~Car() override {
        delete[] features;  // Free heap-allocated array
        cout << "[Car] Destroyed: " << getBrand()
             << " (features array freed)" << endl;
        // ~Vehicle() is called automatically after this — NEVER call it manually!
    }
};

// ═══════════════════════════════════════════════════════════════
// DERIVED CLASS: ElectricCar
// ═══════════════════════════════════════════════════════════════
class ElectricCar : public Car {
private:
    double batteryCapacity;  // in kWh

public:
    ElectricCar(const string& brand, int year, int doors,
                int featCount, double battery)
        : Car(brand, year, doors, featCount),
          batteryCapacity(battery)
    {
        cout << "[ElectricCar] Constructed: " << getBrand()
             << " with " << batteryCapacity << "kWh battery" << endl;
    }

    void start() const override {
        cout << getBrand() << " electric car: silent start, "
             << batteryCapacity << "kWh ready!" << endl;
    }

    void displayInfo() const override {
        Car::displayInfo();  // Reuse parent's display
        cout << "  Battery: " << batteryCapacity << " kWh" << endl;
    }

    ~ElectricCar() override {
        cout << "[ElectricCar] Destroyed: " << getBrand() << endl;
        // ~Car() called automatically, which then calls ~Vehicle() automatically
    }
};

// ═══════════════════════════════════════════════════════════════
// MAIN — Demonstrates everything
// ═══════════════════════════════════════════════════════════════
int main() {
    cout << "========== STACK OBJECTS ==========" << endl;
    {
        Car stackCar("Honda", 2024, 4, 2);
        stackCar.setFeature(0, "Sunroof");
        stackCar.setFeature(1, "GPS");
        stackCar.displayInfo();
        stackCar.start();

        // Compile-time polymorphism: overloaded fuel()
        stackCar.fuel();                          // No args
        stackCar.fuel("Petrol");                  // 1 arg
        stackCar.fuel("Petrol", 45.5);            // 2 args
    }
    // stackCar destroyed here: ~Car() then ~Vehicle() automatically
    cout << endl;

    cout << "========== HEAP + POLYMORPHISM ==========" << endl;
    // Runtime polymorphism — base pointer to derived objects
    vector<Vehicle*> fleet;
    fleet.push_back(new Car("Toyota", 2023, 4, 1));
    fleet.push_back(new ElectricCar("Tesla", 2024, 4, 3, 100.0));
    fleet.push_back(new Car("BMW", 2024, 2, 2));

    cout << "\n--- Polymorphic calls through Vehicle* ---" << endl;
    for (const auto* v : fleet) {
        v->start();        // Calls Car::start or ElectricCar::start
        v->displayInfo();  // Calls Car::displayInfo or ElectricCar::displayInfo
        cout << endl;
    }

    // Safe deletion through base pointer (virtual destructor!)
    cout << "--- Deleting through base pointers ---" << endl;
    for (auto* v : fleet) {
        delete v;  // Correct destructor chain called thanks to virtual ~Vehicle()
        cout << endl;
    }

    return 0;
}
```

**Expected Output:**
```
========== STACK OBJECTS ===========
[Vehicle] Constructed: Honda (2024)
[Car] Constructed: Honda with 4 doors
Car: Honda, Doors: 4, Features: 2
Honda car: push button start, engine roars!
Honda is refueling with default fuel.
Honda is refueling with Petrol.
Honda is refueling with 45.5L of Petrol.
[Car] Destroyed: Honda (features array freed)
[Vehicle] Destroyed: Honda

========== HEAP + POLYMORPHISM ===========
[Vehicle] Constructed: Toyota (2023)
[Car] Constructed: Toyota with 4 doors
[Vehicle] Constructed: Tesla (2024)
[Car] Constructed: Tesla with 4 doors
[ElectricCar] Constructed: Tesla with 100kWh battery
[Vehicle] Constructed: BMW (2024)
[Car] Constructed: BMW with 2 doors

--- Polymorphic calls through Vehicle* ---
Toyota car: push button start, engine roars!
Car: Toyota, Doors: 4, Features: 1

Tesla electric car: silent start, 100kWh ready!
Car: Tesla, Doors: 4, Features: 3
  Battery: 100 kWh

BMW car: push button start, engine roars!
Car: BMW, Doors: 2, Features: 2

--- Deleting through base pointers ---
[Car] Destroyed: Toyota (features array freed)
[Vehicle] Destroyed: Toyota

[ElectricCar] Destroyed: Tesla
[Car] Destroyed: Tesla (features array freed)
[Vehicle] Destroyed: Tesla

[Car] Destroyed: BMW (features array freed)
[Vehicle] Destroyed: BMW
```

---

## 16. Common Interview Mistakes (ION-Specific)

Based on actual reports from ION candidates:

### ❌ Mistake 1: Confusing Overloading and Overriding

| What You Say | What It Actually Is | Interviewer Thinks |
|---|---|---|
| "I'm overriding this function" | You're overloading (different params) | You don't know the difference |
| "This is compile-time overriding" | No such thing — overriding is always runtime | Confused fundamentals |

**Fix:** Overloading = same name, different parameters (compile-time). Overriding = same name AND same parameters in derived class with `virtual` (runtime).

### ❌ Mistake 2: Calling Parent Destructor in Child

```cpp
// ❌ WRONG — Candidate actually did this in ION interview
~Car() {
    delete[] features;
    Vehicle::~Vehicle();  // NEVER DO THIS! Causes double destruction
}

// ✅ CORRECT
~Car() override {
    delete[] features;
    // ~Vehicle() is called AUTOMATICALLY — trust the language
}
```

### ❌ Mistake 3: Forgetting Virtual Destructor

```cpp
// ❌ WRONG — Leads to memory leak when deleting through base pointer
class Vehicle {
public:
    ~Vehicle() { }  // Non-virtual!
};

// ✅ CORRECT
class Vehicle {
public:
    virtual ~Vehicle() { }  // Virtual — enables proper cleanup
};
```

**Rule of thumb:** If your class has ANY virtual function, make the destructor virtual too.

### ❌ Mistake 4: Panicking When Compiler Shows Errors

ION interviews involve writing C++ code live. Compilers are picky. Common quick fixes:

| Error Message | Likely Cause | Quick Fix |
|---|---|---|
| `expected ';' after class` | Missing `;` after `};` | Add `;` after closing brace |
| `no matching function` | Wrong argument types | Check parameter types |
| `cannot declare variable of abstract type` | Not all pure virtuals implemented | Implement missing `= 0` functions |
| `'override' on non-virtual` | Base function isn't `virtual` | Add `virtual` to base function |
| `use of deleted function` | Trying to copy a `unique_ptr` | Use `std::move()` |
| `'x' is a private member` | Accessing private from outside | Use getter or make it `protected` |

**Strategy:** Don't panic. Read the error message. Fix the FIRST error only (later errors are often cascading). Recompile.

### ❌ Mistake 5: Using Random Class Names

```cpp
// ❌ Bad — meaningless, hard to discuss
class A : public B { ... }

// ✅ Good — real-world, easy to reason about
class Car : public Vehicle { ... }
class SavingsAccount : public BankAccount { ... }
class Dog : public Animal { ... }
```

**Why it matters:** Using real-world class names shows you understand domain modeling, and makes it easier for the interviewer (and you) to reason about relationships.

---

## 🎯 Quick Revision Cheat Sheet

```
FOUR PILLARS:
  Encapsulation → private data + public methods
  Abstraction   → pure virtual functions (= 0), hide complexity
  Inheritance   → class Derived : public Base
  Polymorphism  → compile-time (overloading) + runtime (virtual)

OVERLOADING vs OVERRIDING:
  Overloading  = same name, DIFFERENT params, SAME class, compile-time
  Overriding   = same name, SAME params, DERIVED class, runtime, virtual

DESTRUCTOR RULES:
  1. Base class with virtual functions → MUST have virtual destructor
  2. Derived destructor → NEVER call base destructor manually
  3. Destruction order → Derived first, then Base (automatic)
  4. Construction order → Base first, then Derived

INIT LIST:
  REQUIRED for: const members, references, base class constructors
  Order: members initialized in DECLARATION order, not list order

SMART POINTERS:
  unique_ptr → sole ownership, cannot copy, use std::move
  shared_ptr → shared ownership, reference counted
  weak_ptr   → observer, breaks circular references

STACK vs HEAP:
  Stack → automatic, fast, limited size, prefer this
  Heap  → new/delete, manual, large, use smart pointers

VTABLE:
  One vtable per class (array of function pointers)
  One vptr per object (points to class's vtable)
  Only virtual functions use vtable lookup
```

---

*Next: [Section C — Data Structures and Algorithms](./C_dsa.md)*
