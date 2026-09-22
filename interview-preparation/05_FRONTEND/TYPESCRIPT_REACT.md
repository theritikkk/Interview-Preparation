# TypeScript + React

Type-system fundamentals are in `01_PROGRAMMING/TYPESCRIPT.md`; React fundamentals are in `05_FRONTEND/REACT.md`. This file is specifically the intersection.

---

## Cluster 1: Typing Props, State, and Generic Components

### What is it?
Typing a component's props with an `interface`/`type` catches a whole class of bugs (missing/mistyped props) at compile time instead of at runtime; typing `useState` lets TypeScript infer or explicitly pin the state's shape.

### How does it work?
```tsx
interface TaskCardProps {
    task: { id: string; name: string; priority: number };
    onSelect?: (id: string) => void;   // optional prop — caller may omit it
}
function TaskCard({ task, onSelect }: TaskCardProps) {
    return <div onClick={() => onSelect?.(task.id)}>{task.name}</div>;
}

const [tasks, setTasks] = useState<Task[]>([]);   // explicit generic — otherwise TS infers `never[]` from an empty array literal

function List<T>({ items, render }: { items: T[]; render: (item: T) => React.ReactNode }) {
    return <>{items.map(render)}</>;               // generic component — reusable across any item type
}
```

### Interview Questions
- **L1:** "Typed props catch missing/mismatched props at compile time; typing `useState`'s generic avoids TS inferring an overly narrow type like `never[]` from an empty initial array."
- **L2:** explain why `onSelect?: (id: string) => void` (optional prop) requires `onSelect?.(task.id)` (optional chaining) at the call site — TypeScript won't let you call a possibly-`undefined` value without acknowledging that possibility.
- **L3:** explain a generic component's real payoff: one `List<T>` implementation is reusable and fully type-checked for *any* item type, versus either duplicating the component per type or using `any` and losing type safety entirely.

---

## Cluster 2: Typing Event Handlers

### What is it?
React's synthetic events have specific TypeScript types (`React.ChangeEvent<HTMLInputElement>`, `React.MouseEvent<HTMLButtonElement>`) that give you correctly-typed access to `event.target.value` and similar properties.

### How does it work?
```tsx
function handleChange(e: React.ChangeEvent<HTMLInputElement>) {
    console.log(e.target.value);   // correctly typed as string, no `any` needed
}
```

### Interview Questions
- **L1:** "React's event types (like `React.ChangeEvent<HTMLInputElement>`) give correctly-typed access to event properties like `target.value`, instead of falling back to `any`."
- **L2:** explain why getting the generic element type wrong (e.g., typing a `<select>` change handler as `HTMLInputElement`) still often "works" at runtime but loses type safety on element-specific properties — a subtle but real correctness gap worth being aware of.

### Quick Revision
- Type props with an interface; use optional (`?`) for genuinely optional props and remember optional chaining at call sites.
- Explicitly type `useState`'s generic when the initial value doesn't fully convey the intended type (especially empty arrays/objects).
- React's synthetic event types give typed access to event properties — match the generic to the actual DOM element.
