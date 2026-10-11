# Compiler Design → Runtime Environments

**GATE 2027 syllabus:** Compiler Design — runtime environments.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** An activation record (stack frame) typically holds: return address, saved frame pointer (control link), parameters, local variables, and temporaries, in some layout. Call-by-value copies the argument's value in; call-by-reference passes an address, so writes affect the caller's variable; call-by-value-result copies the value in, runs with a local copy, then copies the final value back out (the copy-back order matters when two parameters alias the same variable). Static scoping resolves a name using the program's lexical nesting; dynamic scoping resolves it using the current call stack at runtime.

---

## Questions

### Q1 · MCQ · Fundamental
An activation record is typically allocated

- (A) once per program, shared by all function calls
- (B) once per function *definition*, reused across all its calls
- (C) once per function *call* (activation), freed when that call returns
- (D) once per variable declared in the program

---

### Q2 · NAT · Fundamental
A function's activation record has a 4-byte return address field, a 4-byte saved frame-pointer field, 3 parameters (4 bytes each), 5 local variables (4 bytes each), and 2 temporaries (4 bytes each). The total activation record size, in bytes, is ______.

---

### Q3 · MSQ · Standard
Which of the following statements about parameter-passing mechanisms is/are TRUE?

- (A) Call-by-value never allows a called procedure to modify the caller's actual argument variable through that parameter.
- (B) Call-by-reference passes the address of the actual argument, so modifications inside the procedure are visible to the caller.
- (C) Call-by-value-result behaves like call-by-reference for simple (non-aliased) arguments, but can differ from call-by-reference when the same variable is passed as two different aliased parameters.
- (D) Call-by-name re-evaluates the argument expression each time the parameter is used inside the procedure body (textual substitution semantics).

---

### Q4 · NAT · Standard
A procedure `p(a, b) { a = a + 1; b = b + 1; }` is called as `p(x, x)` (the same variable x passed for both parameters), with x initially 5, using **call-by-reference**. After the call, the value of x is ______.

---

### Q5 · MCQ · Standard
Consider: `int x = 1; void p() { print(x); } void q() { int x = 2; p(); } main() { q(); }`. Under **static (lexical) scoping**, calling q() (which calls p()) prints

- (A) 1
- (B) 2
- (C) an undefined/garbage value
- (D) a compile-time error

---

### Q6 · NAT · Moderate
Using the same program as Q5, under **dynamic scoping**, calling q() (which calls p()) prints the value ______.

---

### Q7 · MSQ · Moderate
Which of the following statements about storage allocation strategies is/are TRUE?

- (A) Static allocation assigns a fixed memory address to a variable at compile time, used for variables whose lifetime is the entire program execution (e.g., in languages without recursion, or for global variables).
- (B) Stack allocation is well-suited for local variables of a function, since their lifetime matches the function call's duration, naturally following a last-in-first-out discipline with nested calls.
- (C) Heap allocation is needed for data whose lifetime extends beyond the call that created it (e.g., dynamically allocated objects that outlive their creating function).
- (D) A language that permits recursion can still use purely static allocation for all local variables without any issue, since each call reuses the same fixed addresses.

---

### Q8 · NAT · Moderate
The same aliased call `p(x, x)` (procedure `p(a,b){a=a+1;b=b+1;}`, x initially 5) is now made using **call-by-value-result**, where the final copy-back from the local parameter copies to the actual argument happens in left-to-right parameter order (a's copy-back happens first, then b's copy-back, which overwrites it since both alias x). The value of x after the call is ______.

---

### Q9 · MSQ · Hard
Which of the following statements about activation records and the run-time stack is/are TRUE?

- (A) The "control link" (dynamic link) in an activation record points to the activation record of the caller, supporting a return to the correct point after the call completes.
- (B) The "access link" (static link), used in languages with nested procedure definitions, points to the activation record of the lexically enclosing procedure's most recent activation, supporting access to non-local variables under static scoping.
- (C) Recursive procedures require a new activation record for each active call, since each call may have different parameter and local-variable values simultaneously live on the stack.
- (D) Under dynamic scoping, a simple global "central table" or stack of active bindings (rather than static/access links) is a natural way to implement non-local name resolution, since lookup follows the call sequence, not lexical structure.

---

### Q10 · NAT · Killer
A recursive function has an activation record of 40 bytes (fixed size per call) and is called with a recursion depth of 100 (100 simultaneously active calls on the stack at the deepest point). Separately, consider the non-recursive version of a similarly-structured computation using the activation record size from Q2 (48 bytes), called exactly once (depth 1, no recursion). Let T = (total stack memory for the 100-deep recursive calls) and S = (stack memory for the single non-recursive call). The value of T − S is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | C | One activation record per call, not per definition |
| 2 | 48 | 4+4 + 3×4 + 5×4 + 2×4 = 8 + 12 + 20 + 8 |
| 3 | A, B, C, D | All four are standard, correct facts about these parameter-passing mechanisms |
| 4 | 7 | Both a and b alias x; two increments apply: 5+1+1 (verified by simulation) |
| 5 | A | Static scoping: p's free variable x resolves to the global x=1, regardless of who calls p |
| 6 | 2 | Dynamic scoping: p's x resolves to the most recently active x on the call stack, which is q's local x=2 (verified by simulation) |
| 7 | A, B, C | (D) is false — purely static allocation cannot support recursion correctly, since multiple simultaneous activations would need multiple, distinct storage locations |
| 8 | 6 | a's local copy becomes 6, b's local copy becomes 6; copy-back order a-then-b means b's value (6) is the final one written to x (verified by simulation) |
| 9 | A, B, C, D | All four are standard, correct facts about activation records and scoping implementation |
| 10 | 3952 | T = 40 × 100 = 4000; S = 48 × 1 = 48; T − S = 4000 − 48 |
