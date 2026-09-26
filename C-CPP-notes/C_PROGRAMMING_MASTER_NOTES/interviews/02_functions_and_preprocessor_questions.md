# Interview Questions — Functions, Scope & the Preprocessor

> Consolidated from `functions/01_functions_and_stack_frames.md`, `02_recursion.md`, and
> `preprocessing/01_macros.md` → `03_conditional_compilation.md`. Full reasoning in each
> topic file's own "🎯 Interview questions" section.

---

## Easy

1. **Is C pass-by-value or pass-by-reference?** Always pass-by-value; there's no
   pass-by-reference. (`functions/01_functions_and_stack_frames.md` §2)
2. **Default initial value of an automatic (non-`static`) local variable?** Undefined/
   garbage. (§8)
3. **What two things does every correct recursive function need?** A base case and
   guaranteed progress toward it. (`02_recursion.md` §2)
4. **What does `#define PI 3.14159` do to the source text?** Textually replaces every later
   `PI` token with `3.14159`, before compilation. (`preprocessing/01_macros.md` §1)
5. **Difference between `#` and `##` in a macro?** `#` stringifies a parameter; `##`
   concatenates two tokens into one. (§3)
6. **Purpose of an include guard?** Stops a header's content from being pasted in more than
   once per translation unit. (`02_includes_and_header_design.md` §3)

## Medium

1. **`static` means two different things depending on context — what are they?** File-
   scope linkage restriction (on externals/functions) vs. persistent storage across calls
   (on a function-local variable). (`01_functions_and_stack_frames.md` §5)
2. **Why can't you take the address of a `register` variable?** It may live entirely in a
   CPU register, with no memory address at all. (§5)
3. **Why does `printd`'s recurse-before-print ordering matter?** It guarantees the highest-
   order digit prints first, since the recursive call (handling all earlier digits)
   finishes completely before the current call's own `putchar` runs. (`02_recursion.md` §2)
4. **Is recursion generally faster than an equivalent loop?** No — per-call frame overhead
   generally makes it slower; recursion buys clarity, not speed. (§4)
5. **Why does `#define square(x) x*x` break on `square(z+1)`?** No parentheses in the
   replacement — the caller's `+` and the macro's `*` collide via ordinary precedence on
   the expanded text. (`preprocessing/01_macros.md` §2)
6. **Why is `max(i++, j++)` dangerous as a macro call?** The argument is textually repeated
   in the expansion, so the "winning" operand gets incremented twice. (§2)
7. **Why can't a `#if` expression use `sizeof`?** The preprocessor runs before the compiler
   has any type information. (`03_conditional_compilation.md` §1)

## Hard

1. **Explain precisely why naive `swap(int x, int y)` can't work in C, and why
   `swap(int *px, int *py)` does, without contradicting call-by-value.** Every argument —
   including a pointer — is passed as a copy; but a pointer's copy still refers to the same
   underlying object, so dereferencing it reaches the caller's original storage.
   (`01_functions_and_stack_frames.md` §2)
2. **What's in a stack frame, and what specifically runs out during a stack-overflow
   crash?** Return address, saved registers, parameters, locals; overflow happens when
   nested calls (often unbounded recursion) exhaust the stack's reserved memory faster than
   frames are popped. (§6)
3. **What is tail-call optimization, and why can't you rely on it in portable C?** Reusing
   the current stack frame for a call that's the last action in a function; C compilers
   aren't required to perform it, so deep tail-recursive C can still overflow the stack
   depending on compiler/optimization level. (`02_recursion.md` §4)
4. **Why is `do { ... } while (0)` the standard multi-statement macro wrapper, instead of
   plain braces?** Braces alone leave a stray trailing-semicolon/`if`-`else`-desync problem
   at the call site; `do`-`while(0)` is a single genuine statement that correctly absorbs a
   trailing `;`. (`preprocessing/01_macros.md` §6)

## Systems-level

1. **A colleague replaces a small, hot function with a function-like macro for
   performance. What's actually gained, and what new bug classes appear?** Gained: avoided
   call overhead (real for tiny, extremely hot functions, though compilers often already
   inline these). New risk: every macro pitfall — double-evaluation of side-effecting
   arguments, precedence leaks, no type checking at all. A `static inline` function
   (`[External]`, C99) usually gets the same benefit without the hazards.
   (`preprocessing/01_macros.md` §9)
2. **Why does changing one header potentially force recompiling many unrelated-looking
   `.c` files?** `#include` is textual pasting, so every file that (directly or
   transitively) includes the changed header has different preprocessed content, even
   though its own source text didn't change — each must be recompiled before re-linking.
   (`02_includes_and_header_design.md` §5)
3. **You're debugging a segfault that only appears on large inputs, in a recursive list-
   processing function. First hypothesis, and how would you confirm it?** Stack overflow
   from recursion depth proportional to input size; confirm via the crash's address
   pattern near the stack boundary, or by checking whether raising the stack limit
   (`ulimit -s`) changes the failure point. (`02_recursion.md` §6)
4. **How does `#include HDR` (with `HDR` a conditionally-defined macro) let one source file
   target multiple platforms cleanly?** Macro expansion resolves `HDR` to a specific header
   name before `#include` processes it, so platform branching lives in one small block
   rather than scattered `#ifdef`s throughout the file. (`03_conditional_compilation.md` §1)
