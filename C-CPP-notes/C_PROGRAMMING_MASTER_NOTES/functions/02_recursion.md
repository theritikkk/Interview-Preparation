# Recursion

> **Phase 2 · Functions, Scope & the Preprocessor.** Depends on:
> `01_functions_and_stack_frames.md` §6 (the stack-frame model is what makes recursion
> possible and what makes it fail).
> Feeds into: `memory/06_struct_union_layout.md` (Phase 4 — recursively-defined data
> structures like trees are recursion's most natural use case).
> Sources: **Book B** §4.10 (primary).

---

## 1. What recursion actually is, mechanically

`[Book-derived, B]` A C function may call itself, directly or indirectly (through another
function that calls back into it — *mutual recursion*, `[External]`, less common but legal).
The crucial fact making this safe rather than catastrophic: **each invocation gets a fresh,
independent set of automatic variables** — this isn't a special rule for recursion, it's
just the ordinary stack-frame behavior from `01_functions_and_stack_frames.md` §6, applied
to a function calling itself. Every recursive call pushes a new stack frame; that frame's
locals and parameters are physically separate storage from every other frame's, including
frames from earlier calls to the *same* function.

### 🧠 Mental model: recursion is stack frames, not magic

```
printd(123) frame:  n=123  ─┐
                             │ calls printd(12)
printd(12)  frame:  n=12   ─┤
                             │ calls printd(1)
printd(1)   frame:  n=1    ─┘  ← base case: prints '1', returns
                             │ back in printd(12)'s frame: prints '2', returns
                             │ back in printd(123)'s frame: prints '3', returns
```

Every box in that stack is a genuinely separate frame with its own `n` — there is no
shared, overwritten `n` variable, which is exactly why the unwinding order comes out
correct.

---

## 2. Every recursive function needs two things

`[Synthesis, extending B's examples into an explicit checklist]`

1. **A base case** — a condition under which the function returns *without* recursing
   further. Without one, recursion never terminates, and per
   `01_functions_and_stack_frames.md` §6, that means stack frames keep getting pushed until
   the stack's reserved memory is exhausted — a **stack overflow** crash.
2. **Progress toward the base case** — each recursive call must move the problem strictly
   closer to the base case (a smaller number, a shorter array, fewer remaining elements).

### Worked example 1 — the simplest possible recursion

```c
/* factorial: n! for n >= 0 */
long factorial(int n)
{
    if (n <= 1)          /* base case */
        return 1;
    return n * factorial(n - 1);   /* progress: n-1 is strictly closer to the base case */
}
```

**Execution flow for `factorial(4)`:** `factorial(4)` calls `factorial(3)` calls
`factorial(2)` calls `factorial(1)`, which hits the base case and returns `1` without
recursing further; each frame then multiplies its own `n` by the value returned from below
as the calls unwind: `1 → 1*2=2 → 2*3=6 → 6*4=24`.

### Worked example 2 — recursion that generates output in the *right* order

`[Book-derived, B]` Printing an integer's digits is a good illustration of recursion
solving an ordering problem elegantly: digits become available lowest-order first (via
repeated `% 10`), but need to be printed highest-order first.

```c
/* printd: print n in decimal, handling the sign */
void printd(int n)
{
    if (n < 0) {
        putchar('-');
        n = -n;
    }
    if (n / 10)
        printd(n / 10);          /* print everything before the last digit, first */
    putchar(n % 10 + '0');       /* then print the last digit */
}
```

**Why this works:** `printd` recurses on `n / 10` (all digits *except* the last) *before*
printing `n % 10` (the last digit) — so the highest-order digit is the first one actually
printed, purely as a side effect of "recurse first, then act" ordering. This is the
recursive mirror image of the iterative `itoa` from `fundamentals/04_control_flow.md` §5,
which instead generates digits low-order-first into an array and reverses it afterward —
the two are a genuinely instructive contrast between an iterative and a recursive solution
to the same underlying ordering problem.

### ⚠️ Common bug — the same edge case in both solutions
`[Book-derived, B]` Both `itoa` (§5 of the control-flow file) and `printd` above **fail on
the most negative representable `int`** (e.g. `INT_MIN` on a two's-complement machine):
negating it (`n = -n`) overflows, because the positive counterpart of `INT_MIN` isn't
representable as a positive `int` — an instance of the signed-overflow undefined behavior
introduced in `fundamentals/02_types.md` §9, showing up in innocent-looking utility code.

---

## 3. Recursion on recursively-shaped problems

`[Book-derived, B]` Recursion is at its most natural on data or problems that are
*themselves* recursively structured — Book B's example is quicksort: partition an array
around a chosen element, then apply the same partitioning process to each resulting
sub-array, stopping once a sub-array has fewer than two elements (the base case).

```c
/* quicksort-style partitioning, adapted for teaching (not a verbatim reproduction of any
   book's exact listing — see the source book for its own concrete implementation) */
void quicksort(int v[], int left, int right)
{
    if (left >= right)                 /* base case: 0 or 1 elements, already sorted */
        return;

    int pivot = v[left];
    int last = left;
    for (int i = left + 1; i <= right; i++)
        if (v[i] < pivot) {
            ++last;
            swap_ints(v, last, i);      /* swap_ints: trivial int-swap helper */
        }
    swap_ints(v, left, last);           /* move the pivot into its final position */

    quicksort(v, left, last - 1);       /* progress: strictly smaller sub-problems */
    quicksort(v, last + 1, right);
}
```

**Execution flow:** each call picks a pivot, partitions its sub-range around it, then
recurses on the two resulting sub-ranges — both strictly smaller than the original range,
guaranteeing progress toward the base case. This is the same "trust the recursion to
correctly handle a smaller version of the exact same problem" mental model as `factorial`,
just applied to a structural (divide-and-conquer) rather than purely numeric problem.

---

## 4. What recursion costs — and what it doesn't buy you

`[Book-derived, B]` Recursion is **not** a free-lunch optimization over iteration, and
Book B is direct about this: it "may provide no saving in storage, since somewhere a stack
of the values being processed must be maintained. Nor will it be faster." What recursion
buys you is **clarity** — for genuinely recursively-structured problems (tree traversal,
divide-and-conquer algorithms, recursively-defined data structures), the recursive
formulation is often dramatically easier to write and verify correct than the equivalent
iterative version built around an explicit, hand-managed stack.

### 🔬 Under the hood — tail calls
`[External]` A special case worth knowing, not discussed in the source books: a *tail
call* is a recursive call that is the very last action a function performs (nothing
happens after it returns — contrast `printd` above, which does work *after* its recursive
call returns, so it is **not** tail-recursive). Some compilers can optimize a genuine tail
call into a plain jump, reusing the current stack frame instead of pushing a new one —
"tail-call optimization" (TCO). **C compilers are not required to perform this
optimization**, unlike some functional languages that guarantee it — so writing
deeply-recursive C code and assuming TCO will save you from a stack overflow is a portability
gamble, not a guarantee. `[Needs verification: exact conditions under which GCC/Clang apply
TCO — this is compiler- and optimization-level-dependent; check with `-O2`/`-O3` and by
inspecting the generated assembly (`fundamentals/01_program_structure.md` §2's `gcc -S`
technique) if this matters for a specific piece of code.]`

---

## 5. ⚠️ Misconceptions to unlearn now

- **"Recursion is generally faster than a loop."** Generally the opposite — each call has
  real overhead (frame setup/teardown, per `01_functions_and_stack_frames.md` §6) that a
  loop doesn't pay.
- **"A missing base case is caught by the compiler."** It is not — an infinitely recursive
  function compiles fine and only fails at *run time*, via stack overflow, often after an
  unpredictable (input-dependent) amount of progress.
- **"Recursion and iteration are just two syntaxes for the same thing."** They share
  expressive power (anything recursive can be rewritten iteratively with an explicit stack,
  and vice versa), but they are not mechanically interchangeable without that explicit
  bookkeeping — the compiler's automatic stack-frame management is precisely what a
  hand-written iterative equivalent has to replicate manually.

---

## 6. 🎯 Interview questions

**Easy**
- What two things does every correct recursive function need? → A base case, and guaranteed
  progress toward it (§2).
- Why doesn't a recursive call overwrite the outer call's local variables? → Each call gets
  its own stack frame with independent storage (§1).

**Medium**
- Why does `printd`'s recursive-call-before-print ordering matter? → It ensures the
  highest-order digit is emitted first, since the recursive call (which handles all
  *earlier* digits) fully completes — including all of *its* printing — before the current
  call's own `putchar` runs (§2).
- Is recursion generally faster or slower than an equivalent loop, and why? → Generally
  slower, due to per-call frame setup/teardown overhead that a loop doesn't incur (§4).

**Hard**
- What specifically causes a stack overflow from unbounded recursion, and how does it
  differ from a heap-exhaustion crash? → Each call pushes a new stack frame; with no base
  case (or a broken one), frames accumulate until the stack's reserved region is exhausted
  — a distinct memory region and failure mode from heap allocation running out of space
  (`memory/05_dynamic_memory.md`, Phase 3).
- What is tail-call optimization, and why can't you rely on it in portable C? → Reusing the
  current stack frame for a call that is the last action in a function, avoiding a new
  frame; C compilers aren't required to perform it, so deep tail-recursive C code can still
  overflow the stack depending on compiler and optimization level (§4).

**Systems-level**
- You're debugging a segfault that only happens on large inputs, in a recursive function
  processing a list. What's your first hypothesis, and how would you confirm it? → Stack
  overflow from recursion depth proportional to input size; confirm via the crash address
  pattern (a fault very close to the stack's boundary) or by checking whether increasing
  the stack limit (`ulimit -s`, `[External]`) changes the failure point — full debugging
  workflow in `debugging/04_debugging_workflows_labs.md`, Phase 10.

---

## 7. External references

- [cppreference — Recursion](https://en.cppreference.com/w/c) `[External]` — language-level
  notes on recursive function calls.
- [GCC documentation — optimization options](https://gcc.gnu.org/onlinedocs/) `[External]`
  — for confirming tail-call optimization behavior at specific `-O` levels on a specific
  target, referenced in §4.

## 8. 🛠 Practice

1. `[Book-derived, B, Exercise 4-12]` Write a recursive version of `itoa` (convert an
   integer to a string), modeled on `printd`'s "recurse first, act after" structure.
2. Write both a recursive and an iterative version of `factorial`, then use `gcc -S`
   (`fundamentals/01_program_structure.md` §2) to compare the generated assembly at `-O0`
   versus `-O2` — does the recursive version's call structure change between optimization
   levels?
3. Deliberately remove the base case from a recursive function, run it, and observe exactly
   how the crash presents (signal, address pattern) — then compare this against a
   deliberately-triggered heap exhaustion later in `memory/05_dynamic_memory.md` (Phase 3)
   to build intuition for telling the two failure modes apart.
