# Data Structures & Algorithms → Linked Lists

**GATE 2027 syllabus:** Programming and Data Structures — linked lists.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** A singly linked list node has one `next` pointer; a doubly linked list node has `next` and `prev`. Floyd's cycle-detection ("tortoise and hare") moves the slow pointer one node and the fast pointer two nodes per step, both starting at the head. μ (mu) is the number of nodes before the cycle starts (0 if the list is entirely a cycle) and λ (lambda) is the cycle's length.

---

## Questions

### Q1 · MCQ · Fundamental
Reversing a singly linked list of n nodes **iteratively**, using only a constant number of extra pointers, takes worst-case time

- (A) O(1)
- (B) O(log n)
- (C) O(n)
- (D) O(n²)

---

### Q2 · NAT · Fundamental
A singly linked list has 20 nodes. Starting from the head, the number of `next`-pointer traversals needed to reach the **last** node is ______.

---

### Q3 · MSQ · Standard
Which of the following operations is O(1) on a **doubly** linked list, given only a pointer to the node itself (not the head)?

- (A) Deleting that node
- (B) Inserting a new node immediately before that node
- (C) Finding the node's position (index) from the head
- (D) Moving to the previous node

---

### Q4 · NAT · Standard
A singly linked list has μ = 5 nodes before the start of a cycle, and the cycle itself has λ = 7 nodes. Floyd's algorithm starts both pointers at the head. The number of steps (moves of the slow pointer) until the two pointers first meet is ______.

---

### Q5 · MCQ · Standard
After Floyd's slow and fast pointers meet inside the cycle, the standard technique to find the **first node of the cycle** is to

- (A) reset the fast pointer to the head and advance both pointers one step at a time; they meet at the cycle's start
- (B) reset the slow pointer to the head and advance only the slow pointer until it equals the fast pointer's current position
- (C) count λ using the meeting point, then the cycle start is exactly λ nodes from the head
- (D) it cannot be found without extra memory to record visited nodes

---

### Q6 · NAT · Moderate
A singly linked list has μ = 10 nodes before the cycle starts, and the cycle has λ = 4 nodes. The number of steps until Floyd's slow and fast pointers first meet is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements about singly vs. doubly linked lists is/are TRUE?

- (A) A doubly linked list uses more memory per node than a singly linked list (for the same payload).
- (B) Deleting a given node (with only a pointer to it, no pointer to its predecessor) in O(1) time, without traversing from the head, is possible for a doubly linked list but not directly for a singly linked list.
- (C) Traversing a doubly linked list backward from the tail is O(1) per step.
- (D) A circular doubly linked list has no NULL pointers among its `next`/`prev` fields (assuming at least one node).

---

### Q8 · NAT · Moderate
A singly linked list has μ = 9 nodes before the cycle starts, and the cycle has λ = 2 nodes. The number of steps until Floyd's slow and fast pointers first meet is ______.

---

### Q9 · MSQ · Hard
A skip list is used as a probabilistic alternative to a balanced tree. Which of the following statements about skip lists is/are TRUE?

- (A) Expected search time is O(log n).
- (B) Every element is guaranteed to appear at every level.
- (C) Insertion and deletion have expected time O(log n).
- (D) Worst-case search time is still O(log n), never worse.

---

### Q10 · NAT · Killer
A singly linked list has μ = 6 nodes before the cycle starts, and the cycle has λ = 5 nodes. Floyd's algorithm meets at some node inside the cycle after the standard slow/fast traversal. Using the standard two-pointer technique (Q5, option A) to then locate the exact **first node of the cycle**, the *additional* number of single-node steps needed (after the meeting point, moving one pointer from the head and one from the meeting point, one step at a time, until they coincide) is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | C | One pass, one pointer flip per node |
| 2 | 19 | 20 nodes need 19 hops from the 1st to the 20th |
| 3 | A, B, D | (C) needs traversal from the head — that is O(n) |
| 4 | 7 | Meeting step is the smallest multiple of λ that is ≥ μ: smallest multiple of 7 that is ≥ 5 is 7 |
| 5 | A | The classic cycle-start-finding technique |
| 6 | 12 | Smallest multiple of 4 that is ≥ 10 is 12 |
| 7 | A, B, C, D | All are standard facts about doubly and circular doubly linked lists |
| 8 | 10 | Smallest multiple of 2 that is ≥ 9 is 10 |
| 9 | A, C | (B) is false — higher levels hold progressively fewer elements; (D) is false — worst case can be O(n) |
| 10 | 6 | This second phase always takes exactly μ additional steps: μ = 6 |
