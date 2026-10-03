# Interview Questions — Debugging & Tooling

> Consolidated from `debugging/`.

---

## Easy

1. **What does `-g` do?** Embeds debug information (source-line and variable mapping) in
   the compiled output. (`01_toolchain_gdb.md` §1)
2. **What's the difference between `next` and `step` in `gdb`?** `next` steps over a
   function call on the current line; `step` steps into it. (§2)
3. **Does UBSan crash your program on the first detected issue, by default?** No — it
   reports and continues unless built with `-fno-sanitize-recover`.
   (`02_sanitizers_and_valgrind.md` §4)
4. **In `nm` output, what does a lowercase vs. uppercase symbol letter mean?** Internal
   (`static`) linkage vs. external linkage. (`03_static_and_binary_inspection.md` §1)
5. **What does exit status 139 tell you?** `139 - 128 = 11` (`SIGSEGV`) — killed by a
   segmentation fault. (`04_debugging_workflows_labs.md` §2)

## Medium

1. **Why is `-O0` conventionally paired with `-g` during debugging?** Optimization can
   reorder or eliminate code, breaking the correspondence between source and what's
   actually running. (`01_toolchain_gdb.md` §1)
2. **What's the fundamental mechanism difference between sanitizers and Valgrind?**
   Compile-time instrumentation (recompile required) vs. runtime emulation of an
   already-built binary. (`02_sanitizers_and_valgrind.md` §1)
3. **An ASan heap-buffer-overflow report includes stack traces for both the bad access and
   the original allocation. Why both?** The allocation trace tells you which `malloc` call
   owns the corrupted memory, so you can check its intended size against the overflowing
   access. (§2)
4. **Why does `main.o`'s `size` output show a nonzero `bss` even though the file doesn't
   grow to hold that memory?** `.bss` records only a size, not literal zero bytes.
   (`03_static_and_binary_inspection.md` §2)
5. **What's the first tool to reach for when a program crashes deep inside
   `malloc`/`free`?** AddressSanitizer, to pinpoint the actual corruption site rather than
   the crash site. (`04_debugging_workflows_labs.md` §4)

## Hard

1. **Explain why a core dump is sometimes the only practical way to debug a production
   crash.** The triggering conditions may not be reproducible locally; a core dump
   captures the actual crashed process's memory, letting `gdb` reconstruct a real backtrace
   with no live reproduction. (`01_toolchain_gdb.md` §3)
2. **Using the UBSan shift-overflow example, explain why undefined behavior is often more
   dangerous than a clean crash.** The program didn't stop — it printed a plausible-looking
   but meaningless value and continued, whereas a crash is at least immediately visible.
   (`02_sanitizers_and_valgrind.md` §4)
3. **Walk through what the four zero bytes after a `call` opcode in an unlinked object
   file represent, and how the linker resolves them.** A placeholder for the call target's
   address, not yet known because the target is defined elsewhere; the paired relocation
   entry names the target and patch method, and the linker overwrites those exact bytes
   once the address is known. (`03_static_and_binary_inspection.md` §3)
4. **Explain why "works at `-O0`, breaks at `-O2`" is close to a diagnostic signature for
   undefined behavior.** The optimizer only has to preserve standard-defined behavior; UB
   has none to preserve, so a more aggressive optimizer can legally transform code relying
   on it differently than a less aggressive one. (`04_debugging_workflows_labs.md` §7)

## Systems-level

1. **A production crash report shows memory corruption, but the team can't rebuild the
   exact release binary with sanitizer flags. What's the practical path forward?** Run the
   existing binary under Valgrind, which needs no recompilation.
   (`02_sanitizers_and_valgrind.md` §1, §5)
2. **A sanitizer-enabled test suite passes cleanly, but a leak still ships to production.
   Why?** The leaking code path wasn't exercised by the test suite — sanitizers only catch
   what actually executes during a run. (§6)
3. **A teammate's build fails with "undefined reference to `foo`" despite `foo` clearly
   being defined in the project. What two `nm` checks would you run first?** Confirm `foo`
   shows as a defined (uppercase) symbol in the file that's supposed to provide it, not
   accidentally `static`; and confirm the reference exists and is correctly spelled in the
   calling file. (`03_static_and_binary_inspection.md` §6, §8)
4. **A hang only appears after a server has handled many requests, never the first few.
   What two resource-exhaustion causes should you check first, and why does "only after
   many requests" implicate them?** Zombie process accumulation and file-descriptor leaks
   — both are cumulative, so their effects only surface once a resource limit is finally
   reached. (`04_debugging_workflows_labs.md` §6, cross-referencing
   `systems/05_processes_fork_exec_wait.md` §6 and `06_pipes_and_signals.md` §2)
