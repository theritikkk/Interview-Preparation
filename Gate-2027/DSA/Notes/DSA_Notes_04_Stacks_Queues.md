# DSA Notes 04 — Stacks & Queues (High-Yield Deep Dive)

> **Why this file exists:** "Stack & Queue" is on the high-yield list. GATE consistently asks: expression conversion/evaluation, circular-queue arithmetic, stack permutations (Catalan counting), and achievability of a given push/pop interleaving. All four are drillable mechanics. All practice answers verified by simulation.
>
> **Companion question bank:** `DSA_01_Arrays_Stacks_Queues.md`. PW source: `../Reference/PW_Subject_Handbooks/PW_Programming_DS.txt` (§7).

---

## 0. The mental model: two ways to postpone work

```
  Stack:  insert & delete at ONE end (top)     → LIFO  (last in, first out)
  Queue:  insert at REAR, delete at FRONT      → FIFO  (first in, first out)
```

Stacks postpone decisions that are nested or reversible (matching brackets, recursion, "latest pending task"); queues preserve fair order (scheduling, buffering, BFS). This framing answers most "why stack/why queue" questions.

---

## 1. Stack

**ADT operations:** `push(x)`, `pop()`, `top()/peek()`, `isEmpty()`. All O(1).

**Array implementation (the version GATE draws):**

```c
  int a[MAX];  int top = -1;          // -1 = empty, MAX-1 = full
  push: if (top == MAX-1) overflow; else a[++top] = x;
  pop:  if (top == -1)   underflow; else return a[top--];
```

- `overflow` = full, `underflow` = empty — different errors (PW lists both).
- Element count = top + 1.
- Linked-list implementation: push/pop at head; no fixed capacity, extra pointer per node.

**Applications (PW's list + the standard set):**

- Recursion (call stack — frames), backtracking, DFS
- Infix → postfix/prefix conversion; evaluation of prefix/postfix
- Balanced parentheses/bracket checking
- Reversing, undo, expression trees, Tower of Hanoi (conceptually recursive)
- "Postpone certain decisions" — PW's one-line summary

---

## 2. Expressions: infix, prefix (Polish), postfix (reverse Polish)

| Form | Example | Note |
|---|---|---|
| Infix | `A + B * C` | needs precedence/associativity |
| Prefix | `+ A * B C` | operator **before** operands |
| Postfix | `A B C * +` | operator **after** operands |

**Evaluation with one operand stack** (postfix — read left to right):

```
  operand → push
  operator → pop b, pop a → push (a op b)
```

**Worked evaluation:** `5 3 + 6 2 / *`:

```
  8 (5+3)   → stack [8]
  6 2 / = 3 → stack [8, 3]
  *         → 24
```

**Conversion infix → postfix (operator stack rules):**

1. Operand → output immediately.
2. Operator: pop while (stack top has higher precedence) or (equal precedence and the operator is left-associative), then push.
3. `(` → push; `)` → pop until `(` (discard both).
4. End: pop everything.

**Worked conversion:** `A + B * C − D / E`:

```
  A            → output            out: A
  +            → stack             st: +
  B            → out               out: A B
  *  (higher than +) → push        st: + *
  C            → out               out: A B C
  -  (pop * and +; same prec, L-assoc) → st: - ; out: A B C * +
  D → out;  / → push (higher than -)   out: A B C * + D
  E → out                              out: A B C * + D E
  end: pop / then -                    out: A B C * + D E / -
```

**Notes:** right-associative operators (and unary minus) change the "equal precedence → pop" rule — check associativity before popping. Division inside evaluation truncates toward zero in C-style semantics (state the convention if the question mixes integers).

---

## 3. Stack permutations (Catalan territory)

Push `1, 2, …, n` in order; pop at any time. Which output sequences are possible?

**How to check achievability (pattern rule):** an output is impossible iff it contains three positions whose **values** appear in the relative order **3, 1, 2** (big, small, middle) — you cannot output the big first, then the small, with the middle still stuck on top.

Example (n = 4): `3,1,2,4` is impossible — to emit 3 first, you pushed 1,2,3; the stack now reads (bottom) 1,2 (top), so after 3 the next pop must be 2, never 1.

**Total count:** the number of achievable outputs = the number of achievable pop sequences = **Catalan(n) = (1/(n+1))·C(2n,n)**:

```
  n:      1   2   3   4    5
  Cat(n): 1   2   5   14   42
```

Handy check: `Cat(4) = 14` means 14 of the 24 permutations of `1..4` are reachable.

---

## 4. Queue

**ADT:** `enqueue(x)` at rear, `dequeue()` at front, `front()`, `isEmpty()`. All O(1).

**Linear array queue problem:** front drifts right forever → "false overflow" even with free space at the left. Fix: **circular queue**.

**Circular queue conventions (the questions always hinge on these):**

```
  Indices 0 … m−1, arithmetic mod m.
  (a) Sacrifice one slot (most common):
        empty: front == rear
        full:  (rear + 1) mod m == front
        capacity = m − 1
        count  = (rear − front + m) mod m
  (b) With a count variable or full-flag: capacity = m.
```

`dequeue`: read a[front], then `front = (front + 1) mod m`. `enqueue`: `a[rear] = x`, then `rear = (rear + 1) mod m` (rear = next empty slot).

**Variants:** priority queue (process by priority, arrival order breaks ties — PW; implemented via heap), **deque** (insert/delete at both ends — can implement both stack and queue), circular/deque-based buffers.

**Applications (PW's list + standard):** BFS, FCFS/round-robin scheduling, printer/IO buffers, asynchronous data transfer, shared-resource access.

---

## 5. Traps & exam patterns

1. **Postfix/prefix evaluation order:** for postfix `a b op` → the FIRST popped operand is `b` (right operand). Mixing the order flips subtraction/division.
2. **Precedence pops on conversion:** pop on strictly-greater precedence, or on equal **if left-associative**.
3. **Catalan questions:** "number of permutations" (n elements) = Catalan(n) — don't confuse with n! or 2^n.
4. **Circular queue full condition needs the mod:** `rear == m-1 ∧ front == 0` is the *non-circular* test; with wraparound it's `(rear+1) mod m == front`.
5. **Count formula `(rear − front + m) mod m`** assumes rear = next-insert position.
6. **One-slot sacrifice:** a size-m array holds at most m−1 elements with convention (a).
7. **Stack permutation ≠ any permutation:** filter with the 3-1-2 pattern rule.

---

## 6. Practice set (verified by simulation)

### Q1 · NAT · Fundamental
Evaluate the prefix expression `- * 2 3 + 4 5`. Result: ______.

### Q2 · NAT · Fundamental
Evaluate the postfix expression `5 3 + 6 2 / *` (integer division). Result: ______.

### Q3 · MCQ · Standard
The postfix form of the infix expression `A + B * C - D / E` is:

- (A) A B C * + D E / -
- (B) A B + C * D E / -
- (C) A B C * D E / - +
- (D) A B C * + D E - /

### Q4 · NAT · Standard
A stack (initially empty, `top = -1`) executes: push 3, push 2, pop, push 7, push 1, push 9. The number of elements on the stack at the end is ______.

### Q5 · NAT · Standard
A circular queue of size m = 8 uses the one-slot-sacrifice convention, with `rear` pointing to the next free slot. `front = 3` and `rear = 6`. The number of elements currently in the queue is ______.

### Q6 · MCQ · Moderate
Elements `1, 2, 3, 4` are pushed onto a stack in this order, with pops allowed at any time. Which output sequence is **NOT** achievable?

- (A) 1 2 3 4
- (B) 4 3 2 1
- (C) 3 2 4 1
- (D) 3 1 2 4

### Q7 · MSQ · Moderate
Which of the following is/are TRUE?

- (A) Recursive function calls are managed using a stack.
- (B) Breadth-first search (BFS) uses a queue.
- (C) Evaluation of a postfix expression uses a stack.
- (D) Level-order traversal of a tree uses a stack.

### Q8 · NAT · Moderate
Evaluate the postfix expression `8 2 3 ^ / 2 3 * +`, where `^` denotes exponentiation. Result: ______.

### Q9 · NAT · Moderate
Among the following bracket strings, the number of **balanced** ones is ______.

```
  (i)  ((a+b)*(c-d))
  (ii) (a+b))
  (iii) ((a+b)
  (iv) a*((b+c)-d)
```

### Q10 · NAT · Killer
A circular queue is implemented in an array of size 10 using the one-slot-sacrifice convention; `rear` points to the next free slot; initially `front = rear = 5` (empty). Execute: enqueue 6 elements → dequeue 2 elements → enqueue 3 elements. The number of elements now in the queue is ______.

---

## Answer Key

| Q | Answer | Why (condensed) |
|---|---|---|
| 1 | −3 | (2·3) − (4+5) |
| 2 | 24 | (5+3) × (6/2) |
| 3 | A | standard operator-stack conversion |
| 4 | 4 | pushes/one pop tracked via top |
| 5 | 3 | (6−3+8) mod 8 |
| 6 | D | contains the 3-1-2 pattern |
| 7 | A, B, C | (D) false — level-order uses a queue |
| 8 | 7 | 8/(2^3) + 2·3 |
| 9 | 2 | (i) and (iv) |
| 10 | 7 | 6−2+3 with wraparound |

### Full solutions

**Q1.** Prefix: `* 2 3 = 6`; `+ 4 5 = 9`; `- 6 9 = ` **−3**. (Read right-to-left with a stack: push 5,4; `+`→9; push 3,2; `*`→6; `-`→6−9.)

**Q2.** `5 3 +` → 8; `6 2 /` → 3; `*` → **24**.

**Q3.** See the worked conversion in §2: **A B C * + D E / - (A)**. (B) ignores precedence; (C) moves the final `-` too far right; (D) misplaces the `/`.

**Q4.** Track `top` (0-based, starts −1): push→0, push→1, pop→0, push→1, push→2, push→3 → count = top+1 = **4**. *(Verified.)*

**Q5.** `(rear − front + m) mod m = (6 − 3 + 8) mod 8 = ` **3** elements (slots 3, 4, 5). *(Verified.)*

**Q6.** D: to output 3 first, push 1,2,3 then pop 3 — the stack now has (bottom→top) 1,2, so the next pop must be 2. Outputting 1 before 2 is impossible any time 3 precedes both: the pattern "3 … 1 … 2" (values 3,1,2 in order) is the forbidden 3-1-2 pattern. (A) pop-as-you-push; (B) push all then pop all; (C) push 1,2,3 → pop 3,2 → push 4 → pop 4,1. *(All three verified by exhaustive enumeration.)*

**Q7.** A ✔ (call frames), B ✔, C ✔. D ✘ — level-order needs FIFO (queue); DFS is the stack one.

**Q8.** `2 3 ^` → 8; `8 8 /` → 1; `2 3 *` → 6; `1 + 6 = ` **7**. *(Verified.)*

**Q9.** (i) balanced ✔; (ii) extra `)` ✘; (iii) missing `)` ✘; (iv) balanced ✔ → count = **2**.

**Q10.** Simulate `count`: enqueue 6 → count 6 (rear: 5→1 wraps); dequeue 2 → count 4 (front: 5→7); enqueue 3 → count **7**. Final pointers: front = 7, rear = 4; check `(4 − 7 + 10) mod 10 = 7` ✔. Capacity check: size 10 with one slot sacrificed → max 9, so 7 fits. *(Verified.)*

---

## 7. Next steps

1. Convert `A * (B + C) - D / (E + F)` to both prefix and postfix by hand.
2. Draw a 10-slot circular array and replay Q10 with pointer labels.
3. **DSA notes complete** — practice reps: `DSA_01_Arrays_Stacks_Queues.md`.
4. Subject order next: **TOC** — planned notes: Finite Automata → PDA → Language Detection.
