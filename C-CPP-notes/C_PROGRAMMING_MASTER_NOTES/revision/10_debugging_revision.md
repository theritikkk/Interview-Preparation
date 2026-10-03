# Revision — Phase 10: Debugging & Tooling

> Quick-reference companion to `debugging/`. Entirely `[External]` — none of the three
> books cover tooling. Every sanitizer/`nm`/`objdump`/linker-error transcript in the source
> files is real, captured output from this vault's own build environment, not reconstructed.

---

## `gdb` and the toolchain — 1-minute summary

`-g -O0` is the debug-build baseline: `-g` embeds source-line/variable debug info; `-O0`
keeps the compiler from reordering/eliminating things a debugger needs to show you. Core
dumps let you debug a crash *after the fact* with no live reproduction needed.

**Worth memorizing:** `next` (steps over a call) vs. `step` (steps into it).

---

## Sanitizers & Valgrind — 1-minute summary

ASan instruments memory accesses at compile time (`-fsanitize=address`) and reports the
exact byte, exact line, and (for heap bugs) both the allocation and the bad-access stack
trace — turning "corrupted somewhere" into an exact diagnosis. LeakSanitizer reports each
still-allocated block's origin at exit. UBSan (`-fsanitize=undefined`) reports UB like
signed overflow or an oversized shift **and then keeps running** by default — a real
captured run showed it printing a corrupted value (`-2147483648`) right after reporting the
overflow that produced it, a concrete demonstration of why UB is more dangerous than a
crash. Valgrind/Memcheck needs no recompilation (runs an existing binary under emulation)
at the cost of 10-50x slowdown versus a sanitizer's ~2x.

**Worth memorizing:** UBSan reports-and-continues by default; ASan aborts immediately —
genuinely different failure modes.
**Worth understanding, not memorizing:** why a stack-buffer-overflow ASan report names the
overflowing *variable itself* (sanitizers_and_valgrind.md §2) — it's frame-aware, not just
address-aware.

---

## Reading object files and binaries — 1-minute summary

`nm`'s letter case *is* Phase 2's `static` rule made visible: uppercase = external linkage,
lowercase = internal (`static`). `U` means referenced-but-undefined-here — exactly the
symbol the linker must resolve. A real captured `objdump -d -r` showed a `call` instruction
with `00 00 00 00` placeholder bytes and the paired relocation entry
(`R_X86_64_PLT32 square-0x4`) telling the linker how to patch them — Phase 5's relocation
concept, literally visible. `size`'s nonzero `bss` column with no corresponding file growth
confirms the "BSS trick" directly. `file` distinguishes a relocatable `.o` from a linked
executable by real ELF header bytes, not extension.

**Worth memorizing:** `nm`'s symbol-type letters (`T`/`t`, `D`/`d`, `B`/`b`, `U`), cold.
**Worth understanding, not memorizing:** what the four zero bytes after a `call` opcode in
an unlinked object file represent (static_and_binary_inspection.md §3) — you should be able
to explain this from first principles, not recall it as a fact.

---

## Debugging workflows — 1-minute summary

Reproduce → isolate → hypothesize → verify → fix. Exit status `139` decodes instantly to
`SIGSEGV` (`128 + 11`) before any tool runs. Crash site ≠ bug site for heap corruption —
trust ASan's allocation trace over the backtrace alone. A hang usually means "blocked in a
system call waiting for something that will never arrive" (classically, an unclosed pipe
write end) — attach a debugger to the *already-hung* process rather than trying to
reproduce from scratch. "Works at `-O0`, breaks at `-O2`" is close to a diagnostic
signature for undefined behavior specifically.

**Worth memorizing:** the `128 + signal` exit-status decode, and the symptom-to-tool table
(workflows_labs.md §8) — this is the fastest path from "something's wrong" to "which tool
do I reach for."

---

## Cross-topic traps worth drilling before an interview

1. Assuming a backtrace's crash frame is where the bug actually is, for heap-corruption
   symptoms specifically — check the allocation/free traces too.
2. Debugging at `-O2` and being confused by `<optimized out>` variables — rebuild at `-O0`.
3. Expecting UBSan to crash on the first violation — it reports and continues by default.
4. Reading `nm`'s `U` as "this symbol doesn't exist anywhere" rather than "not defined in
   *this* file."
5. Treating a hang as automatically an infinite loop in your own logic, rather than
   checking what system call the process is actually blocked in first.
