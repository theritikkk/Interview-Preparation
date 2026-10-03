# Debugging Workflows: From Symptom to Root Cause

> **Phase 10 · Debugging & Tooling — final file.** Depends on: `01_toolchain_gdb.md`,
> `02_sanitizers_and_valgrind.md`, `03_static_and_binary_inspection.md` (this phase — this
> file is where all three tool sets get dispatched by symptom rather than taught in
> isolation).
> Feeds into: `advanced/01_undefined_behavior.md` (Phase 11), `projects/` (Phase 12 —
> every project's inevitable bugs are triaged with this file's method).
> Sources: **entirely `[External]`**, synthesizing this phase's own material plus this
> vault's cross-references. The segfault exit-code example is real, captured output from
> this vault's own build environment.

---

## 1. A general method, before any specific symptom

`[Synthesis]` **Reproduce → isolate → hypothesize → verify → fix.** Reduce the failing
case to the smallest input/code that still fails (isolate) before reasoning about *why*
(hypothesize) — debugging a 3-line reproduction is categorically easier than debugging a
10,000-line program "somewhere in there." Every workflow below is a specialization of this
same loop, keyed to what the symptom itself already tells you.

---

## 2. Symptom: segmentation fault

`[External, verified — real captured output]`

```c
int main(void) { int *p = 0; return *p; }
```

```
$ ./segv
Segmentation fault
$ echo $?
139
```

**Read the exit status first, before anything else.** `[Synthesis]` `139 = 128 + 11`, and
`11` is `SIGSEGV`'s signal number (`libraries/07_setjmp_and_signal.md` §3) — the shell's
`128 + signal` convention (`systems/05_processes_fork_exec_wait.md` §5's `WIFSIGNALED`/
`WTERMSIG`, seen from the shell side) tells you *which signal* killed the process before
you've opened a single tool.

**Workflow:**
1. **Reproduce under a debugger or with a core dump** (`01_toolchain_gdb.md` §2–3) —
   `backtrace` immediately shows the crashing line and full call stack.
2. **If the crash is inside library code** (`malloc`, `strcpy`, …), the *bug* is almost
   always in your code, not theirs — walk `backtrace`'s frames outward to find where your
   code passed the bad pointer/size in the first place.
3. **If it doesn't reproduce reliably**, suspect uninitialized memory or a race — rebuild
   with ASan (`02_sanitizers_and_valgrind.md` §2), which frequently turns an intermittent
   crash into a deterministic, precisely-located one.

---

## 3. Symptom: memory leak (RSS grows over time, never shrinks)

`[Synthesis]`
1. **Confirm it's a real leak, not fragmentation** (`memory/05_dynamic_memory.md` §9) —
   a process's memory can plateau at a high-water mark without actually leaking further.
2. **Run under LeakSanitizer or Valgrind** (`02_sanitizers_and_valgrind.md` §3, §5) — get an
   exact allocation-site stack trace instead of guessing.
3. **If the leak is a linked structure** (a tree, a list), check specifically for
   `memory/05_dynamic_memory.md` §7's non-recursive-`free` trap — the single most common
   real-world leak shape in C.
4. **If no sanitizer is available and the process is already running in production**,
   `/proc/<pid>/status` (`VmRSS`) or periodic heap-profiling tools (`[External]`, beyond
   this vault's core scope) can at least confirm growth before you can attach a debugger.

---

## 4. Symptom: heap corruption / double-free / "crashes somewhere unrelated"

`[Cross-reference]` `memory/05_dynamic_memory.md` §6 already named this pattern: the crash
site and the bug site are often different lines, sometimes different functions entirely.

1. **Rebuild with ASan first, always** (`02_sanitizers_and_valgrind.md` §2) — this symptom
   is exactly what ASan is built to pinpoint precisely, converting "crashes somewhere in
   `malloc`" into "wrote 1 byte past this exact allocation, at this exact line."
2. **Without a sanitizer available**, `gdb`'s `watch` command on a suspected corrupted
   variable (`01_toolchain_gdb.md` §2) can at least tell you *when* it changes, narrowing
   the search.
3. **If the corruption is intermittent**, suspect a buffer sized from user/network input
   without validation (`memory/04_arrays_and_strings.md` §8) — the overflow only triggers
   for inputs beyond whatever you tested.

---

## 5. Symptom: linker error ("undefined reference" / "multiple definition")

`[Cross-reference]` Fully covered in `systems/02_linking_and_loading.md` §2 and
`debugging/03_static_and_binary_inspection.md` §6 — the workflow, condensed:

1. **Read the exact symbol name and object file** the error names — don't guess.
2. **`nm` the object file that should define it** (`03_static_and_binary_inspection.md`
   §1) — is it there at all? Is it accidentally `static` (lowercase) when it needs external
   linkage?
3. **For "multiple definition"**, check whether a full function body or non-`extern`
   variable definition ended up in a header included from more than one `.c` file
   (`preprocessing/02_includes_and_header_design.md` §2).

---

## 6. Symptom: the program hangs (no crash, no output, never returns)

`[Cross-reference, synthesizing `06_pipes_and_signals.md` §2 and `05_processes_fork_exec_wait.md` §6]`
This symptom class has no single tool the way a crash has `gdb`/ASan — hangs require first
narrowing *where* execution is stuck.

1. **If it's I/O-related** (reading a pipe, socket, or file): suspect an unclosed write-end
   descriptor (`systems/06_pipes_and_signals.md` §2) — the single most common real cause of
   "blocked in `read` forever."
2. **Attach a debugger to the already-hung process** (`gdb -p PID`, `[External]`) and
   `backtrace` — this immediately shows which system call or loop it's stuck in, without
   needing to reproduce anything from scratch.
3. **If it's a `wait`-related hang**, check for a child that never actually calls `exit`
   (blocked itself, or in an infinite loop) — `ps` showing the child's own state is the
   next place to look.
4. **Rule out zombie/descriptor exhaustion** (`systems/05_processes_fork_exec_wait.md` §6)
   if the hang only appears after many prior iterations, not immediately.

---

## 7. Symptom: "it works at `-O0` but breaks at `-O2`" (or vice versa)

`[Cross-reference]` This is close to a direct signature of undefined behavior
(`systems/01_compilation_toolchain.md` §2, full catalogue in
`advanced/01_undefined_behavior.md`, Phase 11).

1. **Rebuild with `-fsanitize=undefined`** (`02_sanitizers_and_valgrind.md` §4) at the
   *working* optimization level first — UBSan often catches the exact UB the higher
   optimization level is legally exploiting, even in a build that "seems fine."
2. **Bisect optimization flags** (`-O1`, then individual `-f...` flags) if `-fsanitize=undefined`
   finds nothing, to narrow which specific optimization pass changes behavior.
3. **Treat "works at `-O0`" as no evidence of correctness at all** — it's evidence the bug
   is latent, not absent (Phase 11 makes this precise).

---

## 8. Symptom-to-tool quick reference

`[Synthesis, consolidating §2–7]`

| Symptom | First tool | This vault |
|---|---|---|
| Segfault | `gdb` + core dump, or ASan | §2, `01_toolchain_gdb.md` |
| Memory grows unboundedly | LeakSanitizer / Valgrind | §3, `02_sanitizers_and_valgrind.md` §3, §5 |
| Crash in unrelated-looking code | ASan | §4, `02_sanitizers_and_valgrind.md` §2 |
| Build fails to link | `nm` on the named object files | §5, `03_static_and_binary_inspection.md` §1, §6 |
| Program never returns | Attach `gdb`, check for unclosed pipe ends | §6, `06_pipes_and_signals.md` §2 |
| Behavior changes with optimization level | `-fsanitize=undefined` | §7, `02_sanitizers_and_valgrind.md` §4 |

---

## 9. ⚠️ Misconceptions to unlearn now

- **"The crash location in a backtrace is always where the bug is."** For heap corruption
  and use-after-free specifically, it frequently isn't — the *allocation*/*free* site
  ASan reports alongside the crash site is often more informative (§4).
- **"A hang means an infinite loop in my own code."** It's just as often a blocking
  system call waiting on a resource that will never arrive (an unclosed pipe end, a
  never-signaled condition) — check what the process is blocked *in*, not just what it's
  *doing* (§6).
- **"If it passes at `-O0`, optimization-level differences are a compiler bug."** Overwhelmingly,
  it's undefined behavior the higher optimization level is legally exploiting, not a
  compiler defect (§7).

---

## 10. 🎯 Interview questions

**Easy**
- What does exit status 139 tell you before you've run any tool? → `139 - 128 = 11`
  (`SIGSEGV`) — the process was killed by a segmentation fault (§2).
- What's the first tool to reach for when a program crashes deep inside `malloc`/`free`? →
  AddressSanitizer, to pinpoint the actual overflow/corruption site rather than the crash
  site (§4).

**Medium**
- A build fails with "undefined reference to `foo`." What two things does `nm` let you
  check before touching the linker command itself? → Whether `foo` is defined at all in the
  object file that should provide it, and whether it's accidentally `static` (internal
  linkage) instead of external (§5).
- Why is "attach a debugger to the hung process" often more useful than trying to
  reproduce a hang from scratch? → It shows exactly what the process is currently blocked
  in, immediately — no need to reconstruct the triggering conditions first (§6).

**Hard**
- Explain why "works at `-O0`, breaks at `-O2`" is close to a diagnostic signature for
  undefined behavior specifically, rather than just "the optimizer has a bug." → The
  optimizer is only obligated to preserve standard-defined behavior; UB has no defined
  behavior to preserve, so a more aggressive optimizer is free to transform code relying on
  it differently than a less aggressive one — the *symptom itself* is a direct consequence
  of what "undefined" means (§7, `systems/01_compilation_toolchain.md` §2).
- A hang only appears after a server has handled many requests, never on the first few.
  What two resource-exhaustion causes from this vault should you check first, and why does
  "only after many requests" point at them specifically? → Zombie process accumulation and
  file-descriptor leaks (`systems/05_processes_fork_exec_wait.md` §6,
  `06_pipes_and_signals.md` §2) — both are *cumulative*, so their effects only become
  visible once a resource limit (the process table, the descriptor limit) is finally
  reached, exactly matching a "fine at first, breaks later" symptom (§6).

**Systems-level**
- Design a triage checklist for "the program crashed in production, no sanitizer build
  available, can't reproduce locally." What's the order of operations? → (1) Check the
  exit/signal status if available (§2); (2) look for a core dump and open it in `gdb`
  (`01_toolchain_gdb.md` §3) for an immediate backtrace with no reproduction needed; (3) if
  no core dump exists, attempt reproduction locally with ASan/UBSan enabled, since they
  frequently surface the same class of bug faster than an unassisted build; (4) if
  reproduction fails entirely, fall back to Valgrind against the existing binary, since it
  needs no recompilation (`02_sanitizers_and_valgrind.md` §1, §5).

---

## 11. External references

- [man7.org — `signal(7)`](https://man7.org/linux/man-pages/) `[External]` — the full
  signal-number table behind §2's `128 + signal` exit-code convention.
- [man7.org — `proc(5)`](https://man7.org/linux/man-pages/) `[External]` — `/proc/<pid>/status`
  and related files, referenced in §3 for production-safe memory inspection.
- [man7.org — `strace(1)`](https://man7.org/linux/man-pages/) `[External]` — for tracing
  exactly which system call a hung process (§6) is blocked in, when attaching `gdb`
  directly isn't practical.

## 12. 🛠 Practice — a debugging lab, symptom by symptom

Deliberately construct, then diagnose, one example of each symptom class in this file,
using only the workflow (not foreknowledge of the bug) to find it:

1. A segfault via `NULL` dereference — find it with a core dump and `gdb backtrace` alone.
2. A memory leak via the non-recursive-tree-`free` trap
   (`memory/06_struct_union_layout.md` §5's tree) — find it with LeakSanitizer.
3. A one-byte heap overflow that corrupts an adjacent block's header, causing a *later*,
   unrelated `free` to crash — find the real culprit with ASan, not the crash site alone.
4. An "undefined reference" from a genuinely `static` function called from another file —
   find it with `nm` before attempting to fix the link command.
5. A hang from a `fork`-plus-`pipe` program missing one `close` call
   (`systems/06_pipes_and_signals.md` §2) — find it by attaching `gdb` to the hung process
   and reading its backtrace.
6. A signed-overflow bug that only misbehaves at `-O2` — find it with
   `-fsanitize=undefined` before it ever reaches a "why does this only break in release
   builds" production incident.
