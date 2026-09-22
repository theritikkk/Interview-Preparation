# TypeScript

Your most heavily-tested language: 234 of Hermes's 285 automated tests are TypeScript.

---

## Cluster 1: Type System Basics — Structural Typing, Unions/Intersections, Narrowing

### What is it?
TypeScript's type system is **structural** (duck-typed): two types are compatible if their shapes match, regardless of declared name — unlike Java/C#'s **nominal** typing, where type identity matters even for identical shapes.

### How does it work?
```ts
interface Point { x: number; y: number; }
function log(p: Point) { console.log(p.x, p.y); }
log({ x: 1, y: 2, z: 3 });   // OK — has at least x and y; structural match, extra prop allowed here

type Result = { status: 'ok'; data: string } | { status: 'error'; message: string }; // union

function handle(r: Result) {
    if (r.status === 'ok') {          // narrowing: TS now knows r has 'data', not 'message'
        console.log(r.data);
    } else {
        console.log(r.message);
    }
}
```

### Interview Questions
- **L1:** "TypeScript uses structural typing — compatibility is based on shape, not declared type name — and types are erased at compile time, producing plain JavaScript."
- **L2:** explain discriminated unions (a shared literal field like `status` that lets TS narrow the type inside an `if`/`switch`) — this is the idiomatic way to model "one of several shapes" safely.
- **L3:** explain why structural typing means an "empty" interface or a class with only optional properties can accidentally accept almost anything — a real gotcha worth knowing precisely.

---

## Cluster 2: Generics

### What is it?
Generics let a function, interface, or class be parameterized over a type, preserving type information through the call instead of widening to `any`.

### How does it work?
```ts
function firstOf<T>(arr: T[]): T | undefined { return arr[0]; }
firstOf([1, 2, 3]);       // T inferred as number
firstOf(['a', 'b']);      // T inferred as string

function getProp<T, K extends keyof T>(obj: T, key: K): T[K] {  // constrained generic
    return obj[key];
}
```

### Interview Questions
- **L1:** "Generics preserve type relationships across a function's inputs and outputs instead of losing type information to `any`."
- **L2:** explain a constrained generic (`K extends keyof T`) and why it's safer than an unconstrained `string` key — the compiler now rejects a key that doesn't exist on `T`.
- **L3:** contrast with Java generics: TypeScript's are fully compile-time and erased (no runtime type info at all), whereas Java's generics are also erased but backed by an actual object-oriented type system underneath — the practical consequence being TS can't do `typeof T` checks at runtime any more than Java can, but for a different underlying reason (TS has no runtime type system at all, by design).

---

## Cluster 3: Runtime vs. Compile-Time

### What is it?
TypeScript types exist only during compilation/type-checking; they're erased entirely in the emitted JavaScript. This means you *cannot* check a TS type at runtime — only real JS values.

### DO NOT SAY THIS
"I can check `typeof SomeInterface` at runtime." — Interfaces don't exist at runtime at all; only `typeof someValue` (a JS runtime check) or a `class` (which does exist as a real JS constructor function) can be checked.

### Interview Questions
- **L1:** "Types are compile-time only and erased in the output JS — runtime validation needs an actual runtime check (`typeof`, `instanceof`) or a validation library like Zod/Pydantic-equivalent."
- **L2:** explain why this matters directly for API boundaries: a TS interface on an incoming HTTP request body is a *compile-time promise only* — nothing stops malformed JSON from arriving at runtime, which is why runtime validation libraries exist alongside TS types.

### Quick Revision
- Structural typing: shape matters, not the declared name.
- Discriminated unions + narrowing: the idiomatic way to model variant types safely.
- Generics: preserve type relationships across inputs/outputs; erased at compile time just like the rest of TS.
- TS types don't exist at runtime — always validate untrusted input (e.g., API request bodies) with real runtime checks, not just a type annotation.