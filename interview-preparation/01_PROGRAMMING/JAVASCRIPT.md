# JavaScript

Foundation under your TypeScript work in Hermes and the Task Scheduler, and under your React internship project.

---

## Cluster 1: Execution Model — Call Stack, Event Loop, Microtasks/Macrotasks

### What is it?
JavaScript runs on a single thread with a call stack (synchronous execution) plus an event loop that pulls queued callbacks in once the stack is empty. Two queues matter: **microtasks** (Promise callbacks, `queueMicrotask`) and **macrotasks** (`setTimeout`, I/O callbacks) — all microtasks drain completely before the next macrotask runs.

### How does it work?
```js
console.log('1');
setTimeout(() => console.log('2'), 0);      // macrotask
Promise.resolve().then(() => console.log('3')); // microtask
console.log('4');
// Output: 1, 4, 3, 2  — sync code first, then ALL microtasks, then next macrotask
```

### Interview Questions
- **L1:** "JS is single-threaded with an event loop; synchronous code runs first, then all pending microtasks (Promises), then the next macrotask (setTimeout, I/O)."
- **L2:** predict output for a mix of `setTimeout`, `Promise.then`, and `async/await` (async/await is sugar over Promises — an `await` schedules the rest of the function as a microtask continuation).
- **L3:** explain why a long synchronous loop blocks *everything* — timers, UI rendering (in a browser), and I/O callbacks — because they all wait for the single call stack to empty; this is the direct justification for async patterns in Node.js backend work (Hermes, Task Scheduler).

### DO NOT SAY THIS
"`setTimeout(fn, 0)` runs immediately." — It queues a macrotask that runs only after the current synchronous code *and all microtasks* finish, not "immediately."

---

## Cluster 2: Closures, Prototypes, `this`

### What is it?
A closure is a function bundled with references to its enclosing scope's variables, which stay alive as long as the closure does. Prototypes are JS's inheritance mechanism (objects delegate lookups to a prototype chain rather than a class-based hierarchy). `this` is dynamically bound based on *how* a function is called, not where it's defined.

### How does it work?
```js
function makeCounter() {
    let count = 0;                 // captured by the closure
    return () => ++count;          // arrow function: 'this' is lexically inherited, not rebindable
}
const counter = makeCounter();
counter(); counter();              // 1, 2 — count persists between calls via the closure

const obj = { name: 'a', regular() { return this.name; }, arrow: () => this?.name };
obj.regular();  // 'a' — this = obj, because called as obj.regular()
obj.arrow();    // undefined — arrow captured 'this' from the surrounding (module) scope, not obj
```

### Interview Questions
- **L1:** "A closure lets an inner function retain access to its outer function's variables after the outer function returns."
- **L2:** explain why arrow functions don't have their own `this` and why that makes them the right choice for callbacks inside class methods (e.g., React event handlers, or Node.js callbacks) where you want `this` to mean the enclosing context.
- **L3:** explain prototype-chain lookup: `obj.prop` checks `obj` itself, then `Object.getPrototypeOf(obj)`, and so on until `null` — and how `class` syntax is sugar over this same mechanism.

---

## Cluster 3: Modules

### What is it?
ES Modules (`import`/`export`) are statically analyzed (imports resolved at parse time, enabling tree-shaking) versus CommonJS (`require`/`module.exports`), which is dynamic and resolved at runtime — relevant to your Node.js work directly.

### Interview Questions
- **L1:** "ESM is static and tree-shakeable; CommonJS is dynamic and resolved at runtime — Node.js supports both, with `.mjs`/`type: module` or `.cjs` controlling which."
- **L2:** explain a circular-`require` gotcha in CommonJS (you can get a partially-populated `module.exports` object) versus how ESM handles the same case (live bindings, generally more resilient but still order-sensitive).

### Quick Revision
- Sync code → all microtasks → next macrotask, always in that order.
- Closures: inner function + captured outer variables, alive as long as referenced.
- Arrow functions: no own `this` — inherits from enclosing scope. Regular functions: `this` depends on call site.
- ESM: static/tree-shakeable. CommonJS: dynamic/runtime-resolved.
