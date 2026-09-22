# Object-Oriented Programming

---

## Cluster 1: The Four Pillars

### What is it?
- **Encapsulation:** bundling data with the methods that operate on it, hiding internal state behind a controlled interface.
- **Abstraction:** exposing *what* something does while hiding *how* — an interface without implementation detail.
- **Inheritance:** a class acquiring behavior/state from a parent class.
- **Polymorphism:** the same interface behaving differently depending on the concrete runtime type (see `JAVA.md`'s virtual-dispatch discussion for the Java flavor of this, `CPP.md` for the vtable mechanics).

### How does it work?
```java
interface PaymentMethod { void charge(double amount); }         // abstraction
class CreditCard implements PaymentMethod {
    private String number;                                       // encapsulation — private field
    public void charge(double amount) { /* ... */ }               // polymorphic implementation
}
class PayPal implements PaymentMethod {
    public void charge(double amount) { /* different implementation */ }
}
void checkout(PaymentMethod method, double amount) { method.charge(amount); } // works with ANY PaymentMethod
```

### Interview Questions
- **L1:** state all four pillars with a one-line definition each.
- **L2:** explain why polymorphism specifically is what lets `checkout()` above be written once and work with any current or *future* `PaymentMethod` implementation without modification — this is the practical payoff of the other three pillars combined.
- **L3:** explain the difference between compile-time polymorphism (method overloading — same name, different parameter types, resolved at compile time) and runtime polymorphism (method overriding — resolved via dynamic dispatch, see vtables in `CPP.md`).

---

## Cluster 2: Composition Over Inheritance & SOLID

### What is it?
"Favor composition over inheritance": build behavior by *combining* small objects rather than growing a deep inheritance hierarchy — because inheritance creates tight coupling to a parent's implementation, and deep hierarchies become fragile (the "fragile base class" problem: changing a base class can silently break subclasses far away in the codebase).

**SOLID:**
- **S**ingle Responsibility — a class should have one reason to change.
- **O**pen/Closed — open for extension, closed for modification (add new behavior via new code, not by editing existing tested code).
- **L**iskov Substitution — a subclass must be usable anywhere its parent is expected, without breaking correctness.
- **I**nterface Segregation — many small, specific interfaces beat one large, general one.
- **D**ependency Inversion — depend on abstractions, not concrete implementations.

### How does it work?
```java
// Inheritance-heavy (fragile): Bird -> FlyingBird -> Penguin now must override flying to throw an error — violates LSP
// Composition-based (robust):
interface Flyable { void fly(); }
interface Swimmable { void swim(); }
class Duck implements Flyable, Swimmable { ... }     // has both behaviors, no forced hierarchy
class Penguin implements Swimmable { ... }            // simply doesn't implement Flyable — no broken contract
```

### Interview Questions
- **L1:** "Composition builds behavior from smaller, combinable pieces instead of a rigid inheritance tree, avoiding tight coupling to a parent's implementation."
- **L2:** give a concrete Liskov Substitution violation (the classic: `Square extends Rectangle`, but setting a Square's width also changes its height, breaking any code that assumes independent width/height setters work as they do on a plain Rectangle).
- **L3:** explain Dependency Inversion precisely — high-level modules (business logic) shouldn't depend on low-level modules (a specific database driver); both should depend on an abstraction (an interface) — this is the principle that makes dependency injection (Cluster 3) possible and testable.

---

## Cluster 3: Dependency Injection & Common Design Patterns

### What is it?
Dependency Injection (DI): a class receives its dependencies from the outside (constructor, setter, or a framework container) rather than constructing them itself — this is what makes Spring Boot's `@Autowired`/constructor injection work (see `JAVA.md`).

### How does it work?
```java
// Without DI — tightly coupled, hard to test (can't substitute a mock DB)
class OrderService { private Database db = new PostgresDatabase(); }

// With DI — the caller decides which Database implementation to provide
class OrderService {
    private final Database db;
    OrderService(Database db) { this.db = db; }   // easy to pass a mock in a unit test
}
```

Common patterns worth having ready:
- **Strategy:** swap an algorithm at runtime (e.g., different retry/backoff strategies).
- **Factory:** centralize object creation logic, decoupling callers from concrete classes.
- **Observer:** one-to-many notification (event listeners) — conceptually close to your event-driven Hermes architecture, though Hermes's EventBridge/SQS pub-sub is a distributed, infrastructure-level version of the same idea rather than the in-process GoF pattern.
- **Singleton:** exactly one instance — useful, but overused in practice; a common interview trap is treating Singleton as a default good idea rather than a specific-purpose tool (global mutable state makes testing harder).

### Interview Questions
- **L1:** "Dependency injection means a class receives its dependencies from outside rather than creating them itself, which decouples code and makes it testable via substitutable implementations/mocks."
- **L2:** explain why constructor injection is generally preferred over field injection (`@Autowired` directly on a field) — constructor injection makes dependencies explicit and required, and allows the object to be immutable and always fully constructed, versus field injection allowing a half-constructed object to exist before the framework populates it.
- **L3:** connect DI to testing directly: without it, unit-testing `OrderService` above would require a real Postgres connection; with it, a test can pass in an in-memory fake `Database` implementation, isolating the test to just `OrderService`'s own logic.

### DO NOT SAY THIS
"Singleton is always the right choice for something there's only one of." — Global mutable state introduced by a Singleton makes unit tests harder to isolate (state can leak between tests) — dependency injection of a single shared instance (managed by a framework/container) usually achieves the same practical goal more testably.

### Quick Revision
- Four pillars: encapsulation (hide state), abstraction (hide implementation), inheritance (reuse), polymorphism (uniform interface, varying behavior).
- Composition over inheritance avoids fragile-base-class and LSP-violation traps.
- SOLID's "D" (Dependency Inversion) is the principle; Dependency Injection is the technique that implements it.
- DI's real payoff is testability — substitute a mock without changing the class under test.
