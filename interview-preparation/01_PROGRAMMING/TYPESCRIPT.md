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

## Cluster 4: Interfaces vs. Type Aliases, Intersections

### What is it?
`interface` and `type` both describe object shapes and are largely interchangeable for that use case — but they have real, checkable differences. **Declaration merging**: multiple `interface` declarations with the same name automatically merge into one combined interface; `type` aliases cannot do this (redeclaring one is an error). **Intersections** (`A & B`): combine multiple types into one that must satisfy all of them simultaneously — the `type`-alias tool for composition, versus `interface`'s `extends`.

### How does it work?
```ts
interface Window { title: string; }
interface Window { size: number; }        // MERGES with the declaration above
const w: Window = { title: "x", size: 1 }; // must satisfy BOTH now

type Serializable = { serialize(): string };
type Comparable = { compareTo(other: unknown): number };
type Entity = Serializable & Comparable & { id: string };  // intersection — must satisfy ALL three

interface Base { id: string; }
interface Extended extends Base { name: string; }          // interface composition via extends
```

### Interview Questions
- **L1:** "Interfaces and type aliases both describe object shapes and are largely interchangeable; interfaces support declaration merging (redeclaring the same name adds to it) and `extends`-based composition, while type aliases use intersections (`&`) for composition and cannot merge."
- **L2:** explain a genuine reason to prefer one over the other in a specific case: declaration merging is exactly what lets library type definitions be *augmented* by consumers (e.g., adding a custom property to Express's `Request` type in a `.d.ts` file) — something only possible with `interface`, not `type`. Conversely, `type` aliases can represent things interfaces fundamentally cannot: union types (`type Result = Success | Failure`), primitives, tuples, and mapped/conditional types.
- **L3:** explain the practical consequence of intersections combining *conflicting* property types: `type A = { x: string }; type B = { x: number }; type C = A & B;` results in `x: never` (a type satisfying both `string` and `number` simultaneously is impossible) — a genuinely surprising result worth knowing if pushed, and a good illustration that intersections are a stricter, AND-style combination, not a merge-with-override like object spread (`{...a, ...b}`) would be.

### DO NOT SAY THIS
"Interfaces and type aliases are functionally identical, use whichever you like." — True for plain object shapes, but the union-type, declaration-merging, and mapped-type distinctions above are real and occasionally load-bearing — a precise answer names when the choice actually matters, not just "they're basically the same."

### Quick Revision
- Structural typing: shape matters, not the declared name.
- Discriminated unions + narrowing: the idiomatic way to model variant types safely.
- Generics: preserve type relationships across inputs/outputs; erased at compile time just like the rest of TS.
- TS types don't exist at runtime — always validate untrusted input (e.g., API request bodies) with real runtime checks, not just a type annotation.
- `interface`: supports declaration merging and `extends` — the right choice for anything meant to be augmented later (e.g., library type augmentation). `type`: required for unions, tuples, and mapped/conditional types; composes via intersection (`&`), which is AND-style and can produce `never` on conflicting properties.