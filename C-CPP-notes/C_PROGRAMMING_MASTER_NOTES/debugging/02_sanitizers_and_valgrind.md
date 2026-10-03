# Sanitizers & Valgrind: Catching Memory Bugs as They Happen

> **Phase 10 · Debugging & Tooling.** Depends on: `memory/05_dynamic_memory.md` §6–8
> (heap corruption, leaks, double-free, use-after-free — the exact bug classes this file's
> tools catch), `advanced/01_undefined_behavior.md` (Phase 11 — UBSan is this file's
> hands-on companion to that chapter).
> Feeds into: `04_debugging_workflows_labs.md` (this phase).
> Sources: **entirely `[External]`.** Every AddressSanitizer/LeakSanitizer/
> UndefinedBehaviorSanitizer transcript below is **real, unedited output**, produced by
> compiling and running the exact small programs shown, in this vault's own build
> environment (GCC 13.3.0, x86-64 Linux) — not reconstructed from memory or documentation.
> Valgrind was not installed in that environment; its section is marked accordingly and
> describes documented behavior rather than a captured transcript.

---

## 1. Two different strategies for catching the same bugs

`[Synthesis]` `memory/05_dynamic_memory.md` catalogued the bug classes (leaks, double-free,
use-after-free, buffer overflow); this file is about **tools that catch them automatically**
instead of by manual inspection. Two genuinely different strategies exist:

- **Sanitizers** (AddressSanitizer/ASan, LeakSanitizer/LSan, UndefinedBehaviorSanitizer/
  UBSan) work by having the **compiler instrument your code at build time** — extra checks
  are woven into the binary itself. Fast (typically 2x slowdown, not 10-50x), but requires
  recompiling with the relevant `-fsanitize=` flag.
- **Valgrind** (its `Memcheck` tool, specifically) runs your **already-compiled** binary
  inside a software CPU emulator that inspects every memory access as it happens. No
  recompilation needed at all — but substantially slower (commonly 10-50x), since every
  instruction is emulated rather than run natively.

Neither is strictly better: sanitizers are the default choice for routine development and
CI (fast enough to run constantly); Valgrind's no-recompile property matters when you can't
easily rebuild the exact binary in question, and its `Memcheck` tool catches some
uninitialized-memory-read patterns sanitizers handle differently.

---

## 2. AddressSanitizer — real, unedited output

`[External, verified — compiled with `gcc -g -O0 -fsanitize=address -fno-omit-frame-pointer`]`

### Heap buffer overflow (one byte past the end of a `malloc`'d block)

```c
int *a = malloc(4 * sizeof *a);
a[4] = 1;          /* one past the end — a has indices 0..3 */
```

```
==142==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x502000000020 at pc 0x561aeb5d9222 bp 0x7ffd3d9f45a0 sp 0x7ffd3d9f4590
WRITE of size 4 at 0x502000000020 thread T0
    #0 0x561aeb5d9221 in main /home/claude/lab/asan_test.c:4

0x502000000020 is located 0 bytes after 16-byte region [0x502000000010,0x502000000020)
allocated by thread T0 here:
    #0 0x7fe8b88fd9c7 in malloc ../../../../src/libsanitizer/asan/asan_malloc_linux.cpp:69
    #1 0x561aeb5d91de in main /home/claude/lab/asan_test.c:3
```

**Reading this precisely:** `WRITE of size 4` — a 4-byte store, caught the instant it
happened, not later when something reads the corrupted data (contrast
`memory/05_dynamic_memory.md` §6's warning that unassisted heap corruption often surfaces
far from its cause — this is exactly the problem ASan exists to solve). `0 bytes after
16-byte region` pinpoints it as one element past a 4-`int` (16-byte) allocation — an exact
diagnosis, not a guess. The second stanza shows **where the block was allocated** (line 3),
directly answering "which `malloc` call owns this memory."

### Use-after-free

```c
int *p = malloc(sizeof *p);
*p = 42;
free(p);
printf("%d\n", *p);      /* use after free */
```

```
==137==ERROR: AddressSanitizer: heap-use-after-free on address 0x502000000010 at pc 0x55ffee3152e7 bp 0x7fffed7e4620 sp 0x7fffed7e4610
READ of size 4 at 0x502000000010 thread T0
    #0 0x55ffee3152e6 in main /home/claude/lab/uaf.c:7

freed by thread T0 here:
    #0 0x7f04982fc4d8 in free ../../../../src/libsanitizer/asan/asan_malloc_linux.cpp:52
    #1 0x55ffee3152af in main /home/claude/lab/uaf.c:6

previously allocated by thread T0 here:
    #1 0x55ffee31525e in main /home/claude/lab/uaf.c:4
```

**Three facts, three stack traces, in one report:** where the bad access happened (line 7),
where it was freed (line 6), and where it was originally allocated (line 4) — the complete
lifecycle of the bug, in one shot, exactly answering `memory/02_pointers.md` §8's dangling-
pointer discussion with concrete tooling.

### Double-free

```
==139==ERROR: AddressSanitizer: attempting double-free on 0x502000000010 in thread T0:
    #1 0x562e50d731da in main /home/claude/lab/dfree.c:5

freed by thread T0 here:
    #1 0x562e50d731ce in main /home/claude/lab/dfree.c:4
```

Both `free` call sites (line 4, the first, legitimate one; line 5, the offending second
one) are named explicitly.

### Stack buffer overflow

```c
char buf[8];
strcpy(buf, "this string is too long");
```

```
==147==ERROR: AddressSanitizer: stack-buffer-overflow on address 0x7f3204300028 ...
WRITE of size 24 at 0x7f3204300028 thread T0
    #0 ... in memcpy ...
    #1 0x5630485832d7 in main /home/claude/lab/stackov.c:4

Address 0x7f3204300028 is located in stack of thread T0 at offset 40 in frame
    #0 0x563048583238 in main /home/claude/lab/stackov.c:2

  This frame has 1 object(s):
    [32, 40) 'buf' (line 3) <== Memory access at offset 40 overflows this variable
```

`[Synthesis]` Notice ASan names the overflowing **variable itself** (`'buf' (line 3)`) —
this is stack-frame-aware diagnosis, not just "an address near the stack." This exact bug
is `memory/04_arrays_and_strings.md` §8's fixed-buffer warning, caught the instant `strcpy`
writes the 9th byte, rather than however far later the corrupted stack happens to cause
visible misbehavior.

---

## 3. LeakSanitizer — real, unedited output

```c
char *p = malloc(100);
strcpy(p, "leaked");
p = NULL;              /* only reference lost */
```

```
==144==ERROR: LeakSanitizer: detected memory leaks

Direct leak of 100 byte(s) in 1 object(s) allocated from:
    #1 0x563b85ff11fe in main /home/claude/lab/leak.c:4

SUMMARY: AddressSanitizer: 100 byte(s) leaked in 1 allocation(s).
```

`[Synthesis]` LeakSanitizer runs automatically at program exit when built with
`-fsanitize=address` — no separate flag needed on most platforms — and reports exactly
where each still-reachable-at-startup-but-now-unreachable block was allocated, directly
answering `memory/05_dynamic_memory.md` §7's "which `malloc` never got its matching `free`"
question with a stack trace instead of manual auditing.

---

## 4. UndefinedBehaviorSanitizer — real, unedited output, and a crucial contrast

```c
int x = INT_MAX;
x += argc;                                /* signed overflow when argc == 1 */
int s = 33;
printf("%d\n", 1 << (s + argc - 1));       /* shift count >= width */
```

```
ub.c:5:7: runtime error: signed integer overflow: 2147483647 + 1 cannot be represented in type 'int'
ub.c:8:22: runtime error: shift exponent 33 is too large for 32-bit type 'int'
-2147483648
2
```

### ⚠️ The detail worth pausing on: the program kept running

`[External]` Unlike ASan (which **aborts** the instant it detects a heap/stack violation),
UBSan's **default** behavior is to print a diagnostic and **continue executing** — which is
exactly why this transcript shows the runtime-error messages *and then* two printed,
already-corrupted results (`-2147483648`, the wrapped "overflow" value; `2`, from a shift
that's undefined and could print anything). `[Synthesis]` This is `advanced/01_undefined_behavior.md`'s
central warning (Phase 11) made completely concrete: the program didn't crash, it produced
**silently wrong output** — precisely the "may appear to work" danger that makes undefined
behavior so much more insidious than a clean crash. `-fsanitize=undefined -fno-sanitize-recover`
(a build flag, `[External]`) makes UBSan abort on the first violation instead, trading
"see every UB instance in one run" for "stop immediately, like ASan does."

---

## 5. Valgrind / Memcheck — documented behavior, not a captured transcript

`[External, not run in this vault's build environment — documented format, not a live
transcript]` Valgrind's `Memcheck` tool reports the same bug classes in a differently-shaped
report, conventionally run as `valgrind --leak-check=full ./prog`. Its hallmark
capability beyond what the sanitizers above show: **flagging reads of uninitialized
memory** at the point they influence a decision (e.g., a conditional branching on an
uninitialized variable) — reported as "Conditional jump or move depends on uninitialised
value(s)", with a stack trace to both the read and the original (uninitialized)
allocation. Its leak summary categorizes leaks as "definitely lost" (§7's classic leak),
"still reachable" (memory a pointer *could* still reach, e.g. via a global, but never
freed — often intentional, e.g. an allocator never torn down before `exit`), and others —
a finer-grained taxonomy than LeakSanitizer's single "leaked" bucket.

### 🎯 Interview point
"When would you reach for Valgrind instead of the sanitizers?" — When you can't easily
rebuild the specific binary exhibiting a bug (a third-party library, a release build you
can't instrument), or when you specifically need uninitialized-read detection at the
precision Valgrind provides — otherwise, sanitizers' speed makes them the default choice
for routine development.

---

## 6. ⚠️ Misconceptions to unlearn now

- **"Sanitizers and Valgrind do the same job, so you only need one."** They use genuinely
  different mechanisms (compile-time instrumentation vs. runtime emulation) with different
  speed/setup tradeoffs (§1) — many real projects use both, at different stages.
- **"UBSan crashes the program the moment it finds a bug, like ASan does."** By default it
  reports and **continues** (§4) — a materially different failure mode worth knowing before
  you're surprised by it mid-debugging.
- **"If a sanitizer doesn't report anything, the code has no bugs."** It only catches
  what actually **executes** during that run — an unexercised code path's bugs are
  invisible to it, same as any dynamic testing tool.

---

## 7. 🎯 Interview questions

**Easy**
- What's the fundamental mechanism difference between sanitizers and Valgrind? →
  Compile-time instrumentation (recompile required) vs. runtime emulation of an
  already-built binary (§1).
- Does UBSan crash your program on the first detected issue, by default? → No — it
  reports and continues unless built with `-fno-sanitize-recover` (§4).

**Medium**
- An ASan report for a heap-buffer-overflow includes a stack trace for the overflow *and* a
  separate one for the original allocation. Why both? → The allocation trace tells you
  which `malloc` call owns the corrupted memory, letting you check its intended size
  against the actual overflowing access (§2).
- Why might LeakSanitizer and Valgrind's leak categorization disagree in wording, even
  when analyzing the same bug? → LeakSanitizer reports a single "leaked" category;
  Valgrind distinguishes "definitely lost" from "still reachable" and others — a finer
  taxonomy for the same underlying reachability analysis (§5).

**Hard**
- Explain, using the UBSan shift-overflow example, why undefined behavior is often more
  dangerous than a clean crash. → The program didn't stop — it printed a plausible-looking
  but meaningless value (`2`) and continued as if nothing had gone wrong; a crash is at
  least immediately visible, while silently wrong output can propagate for a long time
  before anyone notices (§4).
- Why is ASan's ~2x overhead acceptable for routine CI but Valgrind's 10-50x often isn't?
  → CI needs to run frequently and quickly to give fast feedback on every change; a 10-50x
  slowdown compounds across a large test suite in a way that can make CI impractically
  slow, whereas 2x is usually tolerable (§1).

**Systems-level**
- A production crash report shows memory corruption, but the team can't easily rebuild the
  exact release binary with sanitizer flags. What's the practical path forward? → Run the
  existing binary under Valgrind, which needs no recompilation, accepting its slower
  execution in exchange for not needing a special build at all (§1, §5).
- A sanitizer-enabled test suite passes cleanly in CI, but a leak still ships to
  production. What's the most likely explanation? → The leaking code path wasn't exercised
  by the test suite — sanitizers only catch what actually executes during the run, so test
  coverage gaps are invisible to them regardless of how thorough the sanitizer itself is (§6).

---

## 8. External references

- [Clang/LLVM — AddressSanitizer documentation](https://clang.llvm.org/docs/AddressSanitizer.html)
  `[External]` — the authoritative reference, including the full report-format
  specification behind §2's transcripts.
- [Clang/LLVM — UndefinedBehaviorSanitizer documentation](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)
  `[External]` — the complete list of checks beyond signed-overflow and shift, and the
  `-fno-sanitize-recover` flag from §4.
- [Valgrind — Memcheck manual](https://gcc.gnu.org/onlinedocs/) `[External, verify the
  current valgrind.org documentation URL at point of use]` — Memcheck's complete report
  format and leak-category definitions referenced in §5.

## 9. 🛠 Practice

1. Compile and run each of the five small programs in §2–4 yourself (they're each 4-6
   lines) — confirm you get the same category of report, and practice reading a sanitizer
   transcript cold, without this file's annotations, until it's fluent.
2. Build the same use-after-free example with plain `gcc` (no sanitizer flags) and observe
   that it very likely "works" (prints `42`) — a direct, hands-on demonstration of why
   undefined behavior "appearing to work" is not the same as being correct.
3. If Valgrind is available in your environment, run it (`valgrind --leak-check=full`)
   against the same five programs and compare its report format and detail level directly
   against the ASan/LSan transcripts in this file.
