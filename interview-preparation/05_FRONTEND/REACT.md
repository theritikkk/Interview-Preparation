# React

🔴 Your only React project is the S.N. Engineering Works internship — this is where these concepts should connect back to a real story (the B2B marketplace, the project configurator, the lead-pipeline status tracking).

---

## Cluster 1: Hooks — useState, useEffect, useMemo, useCallback, useRef

### What is it?
Hooks let function components hold state and side effects without class components. `useState` holds local state and triggers a re-render on change. `useEffect` runs side effects (data fetching, subscriptions) after render, optionally re-running when its dependency array changes. `useMemo`/`useCallback` memoize a computed value/function reference across renders to avoid unnecessary recomputation or child re-renders. `useRef` holds a mutable value that persists across renders *without* triggering one when changed.

### How does it work?
```jsx
function TaskList({ tenantId }) {
    const [tasks, setTasks] = useState([]);
    useEffect(() => {
        let cancelled = false;
        fetchTasks(tenantId).then(data => { if (!cancelled) setTasks(data); });
        return () => { cancelled = true; };   // cleanup: avoid setting state on an unmounted component
    }, [tenantId]);                            // re-run only when tenantId changes

    const sorted = useMemo(() => [...tasks].sort(byPriority), [tasks]); // avoid re-sorting every render
    return <ul>{sorted.map(t => <li key={t.id}>{t.name}</li>)}</ul>;
}
```

### Interview Questions
- **L1:** "`useState` holds local state; `useEffect` runs side effects after render, controlled by its dependency array; `useMemo`/`useCallback` avoid unnecessary recomputation; `useRef` persists a mutable value without causing re-renders."
- **L2:** explain the empty-dependency-array (`[]`) vs. missing-array vs. specific-deps distinction in `useEffect` precisely — this is one of the most common React interview questions, and getting the "runs once on mount" vs. "runs after every render" distinction backwards is a very visible mistake.
- **L3:** explain a stale-closure bug: an effect that reads a piece of state but omits it from the dependency array captures that state's value *at the time the effect was created*, not its current value — and how the ESLint `react-hooks/exhaustive-deps` rule exists specifically to catch this class of bug.

### DO NOT SAY THIS
"`useMemo`/`useCallback` should be used everywhere to optimize performance." — They have their own overhead (comparing dependencies every render) and only pay off when the memoized computation/reference is actually expensive or is passed to a child that itself needs referential stability (e.g., wrapped in `React.memo`) — using them by default everywhere can net-negative performance for cheap computations.

---

## Cluster 2: Reconciliation & the Virtual DOM

### What is it?
React builds a virtual (in-memory) representation of the UI tree; on state change, it diffs the new virtual tree against the previous one and applies only the minimal real-DOM changes needed — this is why direct DOM manipulation is avoided, and why the `key` prop on list items matters so much.

### Interview Questions
- **L1:** "React diffs a new virtual-DOM tree against the previous one and applies only the minimal necessary real-DOM updates, rather than re-rendering the whole page."
- **L2:** explain precisely why using array *index* as a list `key` is a common bug when the list can reorder/insert/delete — React uses the key to match elements between renders, and an index-based key can cause it to wrongly reuse/preserve state (like an input's cursor position or a checkbox's checked state) on the wrong logical item after a reorder.

---

## Cluster 3: Controlled vs. Uncontrolled Forms & State Management

### What is it?
A controlled input's value is driven entirely by React state (`value={state}` + `onChange`); an uncontrolled input keeps its own internal DOM state, read via a ref only when needed. For state shared across many components, "lifting state up" to a common ancestor, Context (for infrequently-changing, broadly-needed state like auth/theme), or a dedicated library (Redux) are the standard escalation path.

### Interview Questions
- **L1:** "Controlled inputs are driven by React state and are the default choice for validation/dynamic behavior; uncontrolled inputs are simpler for one-off cases like a file input, read via a ref when needed."
- **L2 — 🔴 Internship-relevant:** connect this directly to the project configurator: a multi-field configuration form with interdependent fields (e.g., selecting a machine type narrows valid options for a spec field) is a strong argument for controlled inputs + lifted/centralized state, since validation and cross-field logic need to see the current values of *other* fields, not just their own.
- **L3:** explain when Context causes a real performance problem (every consumer re-renders on any Context value change, even if it only cares about part of it) and how splitting into multiple, narrower Contexts or reaching for a library with selective subscriptions (like Redux with selectors) addresses it.

### Quick Revision
- `useEffect` dependency array: `[]` = once on mount; omitted = every render; `[x]` = when `x` changes.
- Stale closures: an effect/callback capturing an old value because it wasn't listed as a dependency.
- `key` prop must be stable per logical item, not array index, when the list can reorder.
- Controlled inputs by default; uncontrolled for simple, ref-read-only cases.

---

## Cluster 4: Redux (State Management Library)

### What is it?
Redux centralizes application state in a single store, updated only through a strict, predictable cycle: components **dispatch** an **action** (a plain object describing what happened, e.g., `{ type: 'task/added', payload: task }`); a **reducer** (a pure function — same input always produces the same output, no side effects) computes the new state from the old state + the action; components **subscribe** (via `useSelector`) to the pieces of state they need and re-render only when *that* piece changes.

### How does it work?
```js
// Reducer — MUST be pure: no mutation, no side effects, same input → same output every time
function tasksReducer(state = [], action) {
    switch (action.type) {
        case 'task/added':
            return [...state, action.payload];        // new array, never mutate `state` directly
        case 'task/removed':
            return state.filter(t => t.id !== action.payload.id);
        default:
            return state;
    }
}

// Component
const tasks = useSelector(state => state.tasks);       // re-renders ONLY when state.tasks changes
const dispatch = useDispatch();
dispatch({ type: 'task/added', payload: newTask });
```

### Interview Questions
- **L1:** "Redux centralizes state in one store; components dispatch plain-object actions describing what happened, pure reducer functions compute new state from the old state plus the action, and components subscribe via selectors to only the state slices they need, minimizing re-renders."
- **L2:** explain *why* reducers must be pure functions specifically: predictability (the same action sequence always produces the same final state, making bugs reproducible) and enabling features built on that guarantee — like Redux DevTools' time-travel debugging, which literally replays actions against the reducer to reconstruct past states, something impossible if reducers had side effects or depended on anything outside their inputs.
- **L3:** explain how Redux's selector-based subscription model (`useSelector`) solves the exact Context re-render problem named in Cluster 3 — a component subscribed to `state.tasks` via a selector does *not* re-render when an unrelated slice of state (e.g., `state.user`) changes, unlike a plain Context consumer, which re-renders on *any* change to the Context value regardless of which part it actually reads. This precise mechanism — selector-based, slice-level subscription — is the concrete technical reason Redux (or similar libraries) outperforms Context for larger, frequently-updated global state.
- **L4 (if asked to compare with your own architecture):** note the structural echo: Redux's action → reducer → new-state cycle is a small-scale, client-side version of Event Sourcing's command → event → derived-state pattern (`07_DISTRIBUTED_SYSTEMS/EVENT_DRIVEN.md`) — both enforce that state changes happen through an explicit, auditable, replayable sequence rather than arbitrary mutation. Drawing this connection explicitly is a strong, memorable answer if a system-design-minded interviewer asks you to compare frontend and backend state-management philosophies.

### DO NOT SAY THIS
"Redux and Context solve the same problem, just use whichever." — They overlap for *simple* shared state, but Context lacks Redux's selector-based partial-subscription model — for state that changes frequently and is read by many components each caring about different slices, that difference has real, measurable performance consequences, not just stylistic ones.

### Quick Revision (Full File)
- `useEffect` dependency array: `[]` = once on mount; omitted = every render; `[x]` = when `x` changes.
- Stale closures: an effect/callback capturing an old value because it wasn't listed as a dependency.
- `key` prop must be stable per logical item, not array index, when the list can reorder.
- Controlled inputs by default; uncontrolled for simple, ref-read-only cases.
- Redux: action (what happened) → pure reducer (computes new state) → selector-based subscription (re-render only on the relevant slice). Reducer purity enables time-travel debugging and predictable, reproducible state changes.
- Redux's action/reducer cycle is structurally the same idea as Event Sourcing's command/event pattern — a strong cross-domain connection to draw if asked.