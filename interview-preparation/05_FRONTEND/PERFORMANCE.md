# Frontend Performance

---

## Cluster 1: Code Splitting & Lazy Loading

### What is it?
Instead of shipping one giant JS bundle, code splitting breaks the app into chunks loaded on demand — so a user visiting one page doesn't download JS for pages they haven't visited yet.

### How does it work?
```jsx
const AdminPanel = React.lazy(() => import('./AdminPanel'));   // separate chunk, loaded only when rendered
function App() {
    return (
        <Suspense fallback={<Spinner />}>
            <AdminPanel />
        </Suspense>
    );
}
```

### Interview Questions
- **L1:** "Code splitting breaks the bundle into on-demand chunks via dynamic `import()`, reducing initial load time; `React.lazy` + `Suspense` is React's built-in mechanism for this."
- **L2:** explain route-based code splitting specifically (splitting per page/route is the most common and highest-value granularity) versus splitting every component individually (usually not worth the overhead of many tiny network requests).

---

## Cluster 2: Memoization & Avoiding Unnecessary Re-Renders

### What is it?
`React.memo` skips re-rendering a component if its props haven't changed (shallow comparison); `useMemo`/`useCallback` (see `REACT.md`) stabilize values/function references passed as props so `React.memo` on the child actually has a chance to skip work.

### How does it work?
```jsx
const TaskRow = React.memo(function TaskRow({ task, onSelect }) {
    return <div onClick={() => onSelect(task.id)}>{task.name}</div>;
});
// Without useCallback, a new onSelect function reference is created every parent render,
// which defeats React.memo on TaskRow — it will re-render every time regardless.
const onSelect = useCallback((id) => setSelected(id), []);
```

### Interview Questions
- **L1:** "`React.memo` skips re-rendering when props are shallow-equal; it only works well if props (including function props) are actually stable across renders, which is what `useCallback`/`useMemo` provide."
- **L2:** explain why wrapping *everything* in `React.memo` isn't automatically a win — the shallow-comparison check itself has a cost, and for cheap components that would re-render fast anyway, it can be a net loss.
- **L3:** explain a real production symptom of this class of bug: a list where every row re-renders on every keystroke in an unrelated search box, traced back to either an unstable object/function created inline in the parent's render, or state that's unnecessarily lifted too high up the tree.

---

## Cluster 3: Virtualization & Bundle Size

### What is it?
List virtualization renders only the visible rows (plus a small buffer) of a very long list, recycling DOM nodes as the user scrolls — necessary once a list is large enough that rendering every row at once becomes the bottleneck itself, independent of any memoization.

### Interview Questions
- **L1:** "Virtualization renders only visible list items instead of the whole dataset, which memoization alone can't fix once a list is large enough that even the *initial* render of every row is the bottleneck."
- **L2:** explain when you'd reach for virtualization vs. pagination vs. infinite scroll — virtualization keeps DOM size bounded, pagination limits data fetched, infinite scroll combines fetching-in-chunks with (often) virtualization for the render side too.

### Quick Revision
- Code splitting: `React.lazy` + `Suspense`, usually split per route.
- `React.memo` only helps if props are actually stable — pair with `useCallback`/`useMemo`.
- Virtualization solves a different problem than memoization: bounding DOM size for very long lists.
