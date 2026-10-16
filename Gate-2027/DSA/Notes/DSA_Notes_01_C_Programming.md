# DSA Notes 01 — C Programming: Pointers, Functions, Recursion, Storage Classes (High-Yield Deep Dive)

> **Why this file exists:** "C (full: pointers, functions, recursion, storage classes)" is on the high-yield list. GATE's C questions are output-prediction or "what does this declaration mean" — both are pattern-matching skills built from a handful of mental models: the memory picture, pointer arithmetic, the always-call-by-value rule, static lifetime, and recursion traces. Every numeric answer below was **verified by compiling and running the code** with `cc`.
>
> **Companion question banks:** `DSA_08_Recursion.md`, `DSA_01_Arrays_Stacks_Queues.md` (array fundamentals). PW source: `../Reference/PW_Subject_Handbooks/PW_Programming_DS.txt` (§1–§5).

---

## 0. The mental model: one program, five regions

```
  ┌──────────────┐
  │    text      │  executable code (read-only)
  ├──────────────┤
  │  data        │  initialized globals/statics
  ├──────────────┤
  │  bss         │  uninitialized globals/statics  → zero
  ├──────────────┤
  │    heap      │  malloc/calloc/realloc … free     (grows up)
  ├──────────────┤
  │    stack     │  local variables, frames          (grows down)
  └──────────────┘
```

Consequences you'll be tested on:

- Locals live on the stack and **die with the function call** → returning the *address* of a local is a bug (dangling pointer).
- `static`/globals live in data/bss → survive calls, zero-initialized.
- If a local shadows a global, the local wins *inside its scope* (PW: "compiler looks in the innermost block first").

---

## 1. Operators: only the GATE-relevant edges

**Integer division and modulo truncate toward zero** (C99+); sign follows the dividend:

```
  -7 / 2 = -3        -7 % 2 = -1        7 / -2 = -3      7 % -2 = 1
```

**Relational/logical results are 0 or 1.** Operators:

| Operator | Rule |
|---|---|
| `&&` `\|\|` | **short-circuit**: `a && b` skips `b` if `a` is 0; `a \|\| b` skips `b` if `a` is non-zero |
| `++x` | increment first, *then* yield the value |
| `x++` | yield the value, *then* increment |
| `~x` | bitwise NOT = **−(x + 1)** (two's complement) |
| `x << k` | multiply by 2^k (for non-negative x) |
| `? :` | both `?` and `:` counts must match; right-associative; allow nesting |
| `=` | the *value* of an assignment is the assigned value: `x = 3 + (x = 10)` → x = 13 |
| `,` | lowest precedence of all |

**Precedence essentials (memorize the ranks, not the table):**

```
  () [] -> .        ← highest
  ! ~ ++ -- + - * (cast) sizeof     (unary, right-to-left)
  * / %
  + -
  << >>
  < <= > >=
  == !=
  &   ^   |
  &&  ||
  ?:
  = += -= ...                        (right-to-left)
  ,                                  ← lowest
```

**Danger zone — don't trust implementation order:** `i++ + ++i` and `printf("%d %d", i++, ++i)` are **undefined/unspecified** in C (no sequencing). GATE asks output predictions only for *well-defined* code; if a question shows this, the correct reasoning is "unspecified behavior". Sequence the increments onto separate statements to make reasoning deterministic.

---

## 2. Storage classes (the four-row table to burn in)

| Class | Scope | Lifetime | Default value | Where |
|---|---|---|---|---|
| `auto` | block (local) | within function | garbage | stack (RAM) |
| `static` | block (local) — if local; file — if global | whole program | 0 | data/bss |
| `extern` | global (file/other file) | whole program | 0 | **no allocation here** |
| `register` | block | within function | garbage | register (a request, not a command) |

Key facts (PW highlights + classics):

- **Static local:** scope stays local, but it keeps its value **between calls**. `static int x=0; x++;` inside a function = a per-function counter.
- **`extern` declares; a definition allocates.** `extern int g;` in a second file doesn't create storage.
- **`register` is a request**; taking its address with `&` is **not allowed** (compiler error).
- Global and local name clash → local wins in its scope.
- String literals (e.g., `"hello"`) live in **read-only** memory — writing through a pointer to one (`p[0]='H'`) is undefined behavior.

---

## 3. Pointers

**One sentence:** a pointer is a variable whose *value* is an address; `&` gives an lvalue's address, `*` dereferences.

```c
int a = 5;
int *p = &a;     // p holds the address of a
*p = 7;          // writes 7 into a
```

**Pointer arithmetic scales by the pointee type:** for `int *p`, `p + 1` advances by `sizeof(int)` bytes. So for `int a[5]`:

```
  p = &a[0]   →   p + i ≡ &a[i]       *(p + i) ≡ a[i]
  a[i] ≡ *(a + i) ≡ *(i + a) ≡ i[a]
```

- **Array name is a constant address** — `a++`, `a = something` are illegal. A pointer *variable* can move.
- **`sizeof(a)` vs `sizeof(p)` differ** — array vs pointer; passing an array to a function decays it to a pointer.
- **Multi-level pointers:** `int **q = &p;` → `*q` is `p` (an address), `**q` is `a`'s value. Read right-to-left when decoding declarations.

**Declaration decoder (PW's table — a GATE favorite):**

| Declaration | Read as |
|---|---|
| `int *P[10]` | P is an **array of 10 pointers** to int (P is the array — `[]` binds tighter) |
| `int (*P)[10]` | P is a **pointer to an array** of 10 ints |
| `int (*P)(int,int)` | P is a **pointer to a function** taking 2 ints, returning int |
| `int (*P[3])(int,int)` | P is an **array of 3** such function pointers |

**Pointer pathologies (name them):**

- **Wild pointer:** uninitialized (`int *p;`) → garbage address. Always `int *p = NULL;`.
- **NULL pointer:** *defined* value that is not a valid address; dereferencing → error. NULL ≠ uninitialized.
- **Dangling pointer:** points to deallocated/out-of-scope memory (returning `&local`, or after `free`).
- **`void *`:** generic address holder; must be **cast before dereferencing** (`*(int*)p`); **no pointer arithmetic** on `void *`.

**Memory leak:** `malloc` without matching `free` — program still correct, resource lost. (Dangling ≠ leak: leak loses the address; dangling keeps a dead address.)

---

## 4. Functions

**C is always call-by-value.** "Call by reference" in C = passing the *address* and dereferencing inside:

```c
void f(int x){ x = 100; }        // caller's variable unchanged
void g(int *x){ *x = 100; }      // caller's variable changed
```

**The classic GATE trap — swapping the pointers instead of the pointees:**

```c
void swap(int *a, int *b){ int *t = a; a = b; b = t; }   // swaps LOCAL copies of addresses
```

After `swap(&x, &y)`, `x` and `y` are unchanged — the function only re-pointed its own parameters. Swapping `*a` and `*b` (via a temp int) would work.

Other sharp edges:

- **Arrays decay:** `void h(int a[])` receives a pointer; element writes inside `h` do modify the caller's array. But `sizeof(a)` inside `h` is the pointer size — a classic trap.
- **Returning a pointer to a local** arrays: compiles (maybe with a warning), then segfault/garbage at runtime — the frame is gone (PW's `int *fun(){ int marks[5]; ... return marks; }` example).
- **Function pointers:** store `&f` (or just `f`), call as `(*fp)(args)` or `fp(args)` — identical.

---

## 5. Strings

A C string is a `char` array ending with `'\0'` (the terminator is not counted by `strlen`, but needs storage).

```c
char s1[] = "gate";       // writable array: g a t e \0
char *s2  = "gate";       // pointer to READ-ONLY literal
s1[0] = 'G';              // ok
s2[0] = 'G';              // undefined behavior (literal in read-only memory)
s2 = "new";               // ok — re-points the pointer
```

Library essentials: `strlen` (length, no `\0`), `strcpy(dst, src)` (copies into dst), `strcmp` (0 if equal), `strcat(dst, src)`. `scanf("%s")` stops at whitespace; `gets` reads a line (unsafe, removed from C11).

**Trap:** `char name[20] = "Pankaj"; name = "Neeraj";` — illegal: array name is not assignable. But `char *p = "Pankaj"; p = "Neeraj";` is fine.

---

## 6. Recursion

**Mental model — stack frames:** each call gets a frame (parameters, locals, return address); the chain of open frames *is* the recursion. Recursion is elegant but costs O(depth) space (stack overflow on deep recursion — PW).

**Two questions every recursion problem reduces to:**

1. **What is the call tree's shape?** (linear, binary, branching)
2. **What happens before vs after the recursive call?** (print order!)

**Call counting:** if `T(n)` = number of calls started by `f(n)`:

```
  f(n) = one call + calls of children:
  T(n) = T(n−1) + T(n−2) + 1  for naive Fibonacci   (T(0)=T(1)=1)
```

**Static + recursion:** a `static` local is shared across all frames (not per-frame!) — incrementing it counts *total calls made so far*.

**Print order:** `printf` before the recursive call → values print on the way **down**; after the call → on the way **up**:

```c
void fun(int n){ if(n==0) return; printf("%d ", n); fun(n−1); printf("%d ", n); }
fun(3) →  3 2 1 1 2 3
```

---

## 7. Traps & exam patterns

1. **`-7 % 2 = -1`** — modulo follows the dividend's sign (C99). Don't copy Python's floor-mod.
2. **`*(p+2) + *p` reads TWO elements** — expand pointer arithmetic to `a[i]` first, then compute.
3. **Swap-by-pointers swaps addresses, not values**, unless you dereference inside.
4. **static wins over re-entry:** one shared instance across all recursive frames.
5. **`extern` never allocates; `register` never addressable.**
6. **String literal pointers are read-only; `char[]` is writable.**
7. **Sequence points:** combine `++`/`--` on the same variable in one expression only when the standard sequences them (statements, `&&`, `||`, `,`).
8. **Declaration parsing:** `[]`/`()` bind tighter than `*` — use the "read outward" trick.

---

## 8. Practice set (every answer verified by compiling with `cc`, 64-bit; print format shown)

### Q1 · NAT · Fundamental

```c
int main(){ int a[] = {2,4,6,8,10}; int *p = &a[1]; printf("%d", *(p+2) + *p); }
```

Output is ______.

### Q2 · MCQ · Fundamental

```c
void swap(int *a, int *b){ int *t = a; a = b; b = t; }
int main(){ int x = 1, y = 2; swap(&x, &y); printf("%d %d", x, y); }
```

Output:

- (A) 2 1
- (B) 1 2
- (C) 1 1
- (D) 2 2

### Q3 · NAT · Standard

```c
int fun(int n){ static int x = 0; x++; if(n > 0) fun(n - 1); return x; }
int main(){ printf("%d", fun(5)); }
```

Output is ______.

### Q4 · NAT · Standard

```c
int x = 10;
int main(){ int x = 20; { int x = 30; printf("%d ", x); } printf("%d", x); }
```

The **second** value printed is ______.

### Q5 · NAT · Standard

```c
char s[] = "hello"; char *p = s; while(*p) p++; printf("%d", (int)(p - s));
```

Output is ______.

### Q6 · NAT · Standard

```c
printf("%d", -7 % 2);
```

Output is ______.

### Q7 · NAT · Moderate

```c
int x = 6; printf("%d", x << 2);
```

Output is ______.

### Q8 · NAT · Moderate

```c
int a[2][3] = {{1,2,3},{4,5,6}};
printf("%d", *(*(a + 1) + 2));
```

Output is ______.

### Q9 · MSQ · Moderate

Which of the following statements is/are TRUE?

- (A) A static local variable keeps its value between calls, while its scope remains the block in which it is declared.
- (B) An `extern` declaration does not allocate storage; a definition later does.
- (C) The address of a `register` variable can be taken with `&`.
- (D) Writing through a pointer that points to a string literal can cause undefined behavior.

### Q10 · NAT · Killer

```c
int sum(int *p, int n){ if(n == 0) return 0; return *p + sum(p + 1, n - 1); }
int main(){ int a[] = {1,2,3,4,5}; printf("%d", sum(a, 5)); }
```

Output is ______.

---

## Answer Key

| Q | Answer | Why (condensed) |
|---|---|---|
| 1 | 12 | *(p+2)=a[3]=8, *p=a[1]=4 |
| 2 | B (1 2) | only the local pointer copies were swapped |
| 3 | 6 | one static x shared by all 6 calls |
| 4 | 20 | outer x is not touched by the inner block |
| 5 | 5 | p walks to the '\0'; p − s = length |
| 6 | −1 | truncation toward zero |
| 7 | 24 | 6 × 4 |
| 8 | 6 | *(a+1) row 2; +2 third element |
| 9 | A, B, D | (C) is illegal in C |
| 10 | 15 | 1+2+3+4+5 via recursion |

### Full solutions

**Q1.** `p = &a[1]`. `*(p+2) = a[3] = 8`; `*p = a[1] = 4`; 8 + 4 = **12**. *(Compiler-verified.)*

**Q2.** Inside `swap`, `a` and `b` are local copies of the addresses of x and y. Swapping them re-points the copies — the caller's x, y never change: **1 2 (B)**. To fix: swap `*a` and `*b` through a temp `int`.

**Q3.** `x` is a *static* local: initialized once, shared by every call. Calls `fun(5)→fun(4)→…→fun(0)` make x go 1…6; each frame's `return x` reads the same x = **6**. *(Compiler-verified.)*

**Q4.** Three different `x`s: global 10 (hidden), outer-local 20, inner-block 30. First printf: 30. Second (after the inner block ends): **20**. *(Verified: prints `30 20`.)*

**Q5.** `p` advances while characters are non-zero — stops at the terminator. `p − s` = number of characters walked = **5** (pointer subtraction counts elements).

**Q6.** C99 integer division/modulo truncate toward zero: `-7/2 = -3`, and `-7 % 2 = **-1**` (sign of dividend). *(Verified.)*

**Q7.** `6 << 2 = 6 × 2² = 24`. *(Verified.)*

**Q8.** `a + 1` = pointer to the second row; `*(a+1)` = `a[1]`; `*(a+1)+2` = `&a[1][2]`; dereference → `a[1][2] = 6`. Memory picture: rows contiguous → element = row×3+col = 1×3+2 = 5th element = **6**.

**Q9.** A ✔ — the definition of static-storage-duration locals. B ✔ — extern is a declaration. C ✘ — `&` on a register variable is a compile error (register placement is a request). D ✔ — literals live in read-only memory; writing is UB.

**Q10.** Recursive walk: `1 + sum(&a[1],4)` … = `1+2+3+4+5 = 15`. Trace the frames down to `n=0` (returns 0), then add on the way back up. *(Verified.)*

---

## 9. Next steps

1. Hand-trace Q3 and Q10 on paper frame-by-frame — this is exactly how GATE expects the work.
2. Recheck the declaration decoder (§3) by covering the table and re-deriving.
3. Practice reps: `DSA_08_Recursion.md`, then `DSA_01_Arrays_Stacks_Queues.md`.
4. Continue to `DSA_Notes_02_Trees.md`.
