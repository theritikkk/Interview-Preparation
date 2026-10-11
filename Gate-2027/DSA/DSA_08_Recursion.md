# Data Structures & Algorithms → Recursion

**GATE 2027 syllabus:** Programming and Data Structures — programming in C, recursion.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** "Number of calls" counts every invocation of the function, including the initial (outermost) call. Recursion depth is the maximum number of **pending** (not-yet-returned) calls on the call stack at any instant, counting the initial call as depth 1. All functions below terminate (no infinite recursion).

---

## Questions

### Q1 · MCQ · Fundamental
A recursive function that calls itself exactly once per invocation, reducing the problem size by 1 each time until reaching a base case at size 0, starting from size n, has a maximum recursion depth of

- (A) 1
- (B) log₂ n
- (C) n + 1
- (D) 2n

---

### Q2 · NAT · Fundamental
The Tower of Hanoi problem with n = 10 disks, solved by the standard recursive algorithm, requires ______ disk moves.

---

### Q3 · MSQ · Standard
```c
int f(int n) {
    if (n <= 1) return n;
    return f(n - 1) + f(n - 2);
}
```
This is the naive (non-memoized) recursive Fibonacci function. Which of the following statements about `f(n)` is/are TRUE?

- (A) The total number of calls made (including the initial call) grows exponentially in n.
- (B) The maximum recursion depth is Θ(n), not Θ(2ⁿ).
- (C) Adding memoization (caching previously computed results) would make the total number of distinct values computed only Θ(n).
- (D) `f(n)` runs in O(n) time as written, without any modification.

---

### Q4 · NAT · Standard
Using the naive recursive Fibonacci function of Q3, the total number of calls made by `f(10)` (including the initial call) is ______.

---

### Q5 · MCQ · Standard
```c
int g(int n) {
    if (n <= 0) return 0;
    g(n - 1);
    g(n - 1);
    return 1;
}
```
The total number of calls made by `g(6)` (including the initial call) is

- (A) 63
- (B) 64
- (C) 127
- (D) 128

---

### Q6 · NAT · Moderate
```c
long long fastpow(int x, int y) {
    if (y == 0) return 1;
    if (y % 2 == 0) {
        long long h = fastpow(x, y / 2);
        return h * h;
    } else {
        return x * fastpow(x, y - 1);
    }
}
```
The number of recursive calls made (not counting the initial call) when evaluating `fastpow(3, 13)` is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements about recursion vs. iteration is/are TRUE?

- (A) Every recursive function can be rewritten as an equivalent iterative one using an explicit stack.
- (B) Tail-recursive functions can always be automatically transformed by a compiler into a loop with O(1) auxiliary space, though not every compiler performs this optimization.
- (C) A recursive function's worst-case auxiliary space is always O(1), same as its iterative equivalent.
- (D) Deep, non-tail recursion on a large input can cause a stack overflow even when an equivalent iterative version would run fine.

---

### Q8 · NAT · Moderate
A recursive function generates all permutations of n elements using the standard "swap and recurse" technique (at depth k, it tries placing each of the remaining n − k elements at position k, then recurses to depth k + 1; the base case is reached at depth n). For n = 5, the total number of calls made (including the initial call) is ______.

---

### Q9 · MSQ · Hard
Which of the following statements about **mutual recursion** (two functions A and B that call each other) is/are TRUE?

- (A) Mutual recursion can always be mechanically rewritten as direct recursion (a single function calling only itself) using standard techniques.
- (B) Mutual recursion necessarily requires more stack space than an equivalent direct recursion for the same problem.
- (C) A base case must eventually be reached in at least one of the mutually recursive functions for the recursion to terminate.
- (D) Even and odd number checking (`isEven(n)` calls `isOdd(n-1)`, and vice versa, with base cases `isEven(0) = true`, `isOdd(0) = false`) is a classic example of mutual recursion.

---

### Q10 · NAT · Killer
The Tower of Hanoi problem is solved for n = 8 disks, but using a **modified** recursive strategy that requires moving disks via **two** intermediate pegs in sequence rather than one (a 4-peg variant is NOT what is meant here — instead, consider the standard 3-peg recursive strategy applied twice in a row, i.e., the disks are moved from peg A to peg B, and then, as a separate second phase using the same recursive algorithm, immediately moved again from peg B to peg C). The total number of individual disk moves across both phases combined is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | C | n recursive calls plus the base case itself |
| 2 | 1023 | 2¹⁰ − 1 |
| 3 | A, B | (C) is false — memoization gives Θ(n) distinct *values*, but this doesn't make the unmodified function itself O(n); (D) is false — the naive version is exponential |
| 4 | 177 | Verified by direct simulation of the call tree for f(10) |
| 5 | C | Each call (except the deepest) spawns 2 more: total calls = 2⁷ − 1 = 127 |
| 6 | 6 | Verified by simulation: fastpow(3,13) makes 6 further recursive calls beyond the initial one (7 total) |
| 7 | A, B, D | (C) is false — non-tail recursion generally needs O(depth) stack space, not O(1) |
| 8 | 326 | Verified by simulating the standard recursive permutation-generation call tree for n = 5 |
| 9 | A, C, D | (B) is false — mutual recursion does not inherently need more stack space than direct recursion for a comparable problem |
| 10 | 2046 | Each phase is an independent 8-disk Hanoi solve: 2 × (2⁸ − 1) = 2 × 255 |
