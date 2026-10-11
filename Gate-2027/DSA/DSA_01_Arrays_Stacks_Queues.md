# Data Structures & Algorithms → Arrays, Stacks & Queues

**GATE 2027 syllabus:** Programming and Data Structures — arrays, stacks, queues.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Stacks and queues hold distinct elements unless stated otherwise. "Push order 1, 2, …, n" means the values are pushed in that order (not necessarily consecutively — other operations may interleave). A circular queue of capacity n is considered full when it holds n elements.

---

## Questions

### Q1 · MCQ · Fundamental
A stack is used to check whether a string of parentheses is balanced. When a closing bracket is read and the stack is empty, this means

- (A) the string is balanced so far
- (B) there is an unmatched closing bracket — the string is not balanced
- (C) push the closing bracket instead
- (D) skip the character and continue

---

### Q2 · NAT · Fundamental
A circular queue implemented in an array of size 8 uses the standard "front"/"rear" pointers and always keeps one slot empty to distinguish full from empty. The maximum number of elements this queue can hold at once is ______.

---

### Q3 · MSQ · Standard
Elements 1, 2, 3, 4, 5 are pushed onto a stack in this order, with pops allowed to happen at any point in between pushes (i.e., any stack permutation is possible). Which of the following output sequences is/are **achievable**?

- (A) 3, 2, 1, 4, 5
- (B) 3, 5, 4, 2, 1
- (C) 4, 5, 3, 2, 1
- (D) 5, 3, 4, 2, 1

---

### Q4 · NAT · Standard
The number of distinct output permutations achievable from a stack (in the sense of Q3) when n = 4 distinct elements are pushed in order 1, 2, 3, 4, with pops interleaved arbitrarily, is ______.

---

### Q5 · MCQ · Standard
A queue is implemented using two stacks, S1 and S2. Enqueue always pushes onto S1. Dequeue: if S2 is empty, pop every element off S1 and push it onto S2, then pop and return the top of S2; otherwise just pop and return the top of S2. For a sequence of n enqueues followed by n dequeues, the total number of individual stack push/pop operations (across both stacks, counting every single push and every single pop) is

- (A) n
- (B) 2n
- (C) 3n
- (D) 4n

---

### Q6 · NAT · Moderate
A stack-based expression evaluator processes the postfix expression `6 2 3 + - 3 8 2 / + *` using a single operand stack (binary operators pop two operands and push one result). The maximum number of elements ever present on the stack **at the same time** during this evaluation is ______.

---

### Q7 · MSQ · Moderate
A deque (double-ended queue) supports insertion and deletion at both ends. Which of the following can be implemented using only a deque as the sole underlying structure, using only its native operations (no extra data structures)?

- (A) A stack (LIFO)
- (B) A queue (FIFO)
- (C) A priority queue with O(1) worst-case insert and O(1) worst-case extract-min for arbitrary priorities
- (D) A structure that supports "push to front" and "pop from back" only

---

### Q8 · NAT · Moderate
The elements 1, 2, 3, 4, 5, 6, 7 are pushed onto a stack in this order, with pops allowed at any point. The output sequence observed is 3, 4, 2, 6, 5, 7, 1. The maximum number of elements present on the stack at any single instant during this process is ______.

---

### Q9 · MSQ · Hard
An array-based implementation of a stack of fixed capacity n uses a top index initialized to −1 (empty). Which of the following statements is/are TRUE about this implementation?

- (A) Push and pop are both O(1) in the worst case.
- (B) Detecting stack overflow requires comparing the top index against n − 1 before pushing.
- (C) A sequence of n pushes followed by n pops always leaves the top index at −1.
- (D) The array elements below the current top index are guaranteed to be in sorted order.

---

### Q10 · NAT · Killer
Elements 1, 2, …, 6 are pushed onto a stack in this order, with pops allowed to happen at any point in between pushes. The number of distinct **output sequences** in which element 4 appears strictly before element 2 in the output is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | An unmatched closer means the string cannot be balanced |
| 2 | 7 | n − 1 slots usable when one is kept empty |
| 3 | A, B, C | (D) is not achievable: 5 can only come out first if 1,2,3,4 are still pushed, but then 3 cannot appear before 4 while 1,2 remain below — check by simulation: pushing 1,2,3,4,5, popping 5 then needing 3 next requires 4 still on stack, but the target has 4 after 3, contradiction |
| 4 | 14 | Catalan number C₄ |
| 5 | C | Each of the n elements is pushed onto S1 once, popped from S1 once and pushed onto S2 once, then popped from S2 once: 3 ops per element |
| 6 | 4 | Track the operand stack through the postfix evaluation |
| 7 | A, B, D | (C) is false — no comparison-based structure gives O(1) worst-case extract-min for arbitrary keys |
| 8 | 3 | Verified by simulating the push/pop trace that produces 3, 4, 2, 6, 5, 7, 1; the stack never holds more than 3 elements at once |
| 9 | A, B, C | (D) is false — the stack is not required to be sorted |
| 10 | 57 | Of the 132 valid stack-permutations of 6 elements (Catalan C₆), exactly 57 have 4 appearing before 2 — verified by exhaustive check |
