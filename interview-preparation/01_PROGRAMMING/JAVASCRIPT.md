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

## Cluster 4: Promises & Async/Await

### What is it?
A Promise represents a value that will exist eventually — it's always in one of three states: **pending**, **fulfilled** (resolved with a value), or **rejected** (failed with a reason), and once settled (fulfilled or rejected), it can never change state again. `async`/`await` is syntax sugar over Promises — an `async` function always returns a Promise, and `await` pauses that function (without blocking the thread — other code can still run) until the awaited Promise settles.

### How does it work?
```js
function fetchUser(id) {
    return fetch(`/users/${id}`)
        .then(res => res.json())
        .catch(err => { console.error(err); throw err; });  // re-throw to propagate, or swallow to recover
}

// Same logic, async/await — reads top-to-bottom instead of chained
async function fetchUser(id) {
    try {
        const res = await fetch(`/users/${id}`);
        return await res.json();
    } catch (err) {
        console.error(err);
        throw err;
    }
}

// Combinators
await Promise.all([p1, p2, p3]);        // waits for ALL; rejects immediately if ANY rejects
await Promise.allSettled([p1, p2, p3]); // waits for ALL, never short-circuits — get every result/error
await Promise.race([p1, p2, p3]);       // settles as soon as the FIRST one settles (fulfilled or rejected)
```

### Interview Questions
- **L1:** "A Promise is pending/fulfilled/rejected, settling exactly once; async/await is sugar over Promises — await pauses the async function without blocking the thread, letting other code run while waiting."
- **L2:** explain why an unhandled Promise rejection is a real, checkable bug class — a rejected Promise with no `.catch()` (or no surrounding `try/catch` if awaited) doesn't necessarily crash synchronously the way a thrown exception would, but modern Node/browsers will log an "unhandled promise rejection" warning, and in newer Node versions can actually crash the process — never leave a Promise chain uncaught in production code.
- **L3:** explain `Promise.all` vs `Promise.allSettled` precisely, and when each is correct: `Promise.all` is right when *any* failure should abort the whole operation (e.g., you need all N results to proceed meaningfully); `Promise.allSettled` is right when you want every result regardless of individual failures (e.g., sending notifications to N users — one failing shouldn't stop the others, and you want to know which ones failed) — directly relevant to a Hermes-style fan-out scenario where multiple independent downstream calls happen from one event.

### DO NOT SAY THIS
"`await` blocks the thread until the Promise resolves." — It does not block the JS thread at all; it pauses only the current `async` function's execution, yielding control back to the event loop so other code (other requests, other callbacks) can run in the meantime — this is the entire point of async/await over a genuinely blocking call.

### Common Mistakes
- Forgetting `await` on an async call inside another async function — silently gets a Promise object instead of the resolved value, often not erroring immediately but producing confusing downstream bugs.
- Using `Promise.all` when one failure shouldn't abort the others — should have been `allSettled`.
- Sequential `await`s for independent operations that could run concurrently: `const a = await fetchA(); const b = await fetchB();` needlessly serializes two independent calls — `const [a, b] = await Promise.all([fetchA(), fetchB()]);` runs them concurrently instead, a common, easy performance win worth mentioning proactively.

### Quick Revision
- Sync code → all microtasks → next macrotask, always in that order.
- Closures: inner function + captured outer variables, alive as long as referenced.
- Arrow functions: no own `this` — inherits from enclosing scope. Regular functions: `this` depends on call site.
- ESM: static/tree-shakeable. CommonJS: dynamic/runtime-resolved.
- Promise: pending → fulfilled/rejected, settles once. `await` pauses the async function only, never blocks the thread.
- `Promise.all` (abort on first failure) vs `Promise.allSettled` (get every result regardless) — pick based on whether one failure should cancel the rest.
- Independent `await`s in sequence needlessly serialize — use `Promise.all` to run them concurrently instead.
