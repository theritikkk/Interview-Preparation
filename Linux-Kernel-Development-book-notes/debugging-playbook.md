# Debugging Playbook

> Organized by **symptom**, not by chapter — the point of this file is to let you debug *backward* from a real problem you're facing, once you have enough chapters under your belt to recognize the symptom. Each entry follows: Symptom -> Hypotheses -> Observations -> Tools -> Evidence -> Root Cause -> Fix -> Verification.

---

## How to Use This File

When something goes wrong on a real system (or in one of this course's experiments), **do not immediately jump to "fix."** Work through the columns left to right. The habit of generating multiple hypotheses *before* reaching for a tool, and only trusting a root cause once you have direct evidence for it, is the actual skill this file is teaching — the specific scenarios are secondary.

---

## Scenario: Unexplained `SIGSEGV` on a Suspicious-Looking Address

*(Introduced in Chapter 1 — "Debugging mindset")*

| Step | Content |
|---|---|
| **Symptom** | A user-space program crashes with `SIGSEGV` immediately when dereferencing a pointer, and the faulting address looks suspiciously like a high, kernel-range address (e.g., `0xffffffff81000000`). |
| **Hypotheses** | (1) Uninitialized pointer holding garbage that happens to alias a kernel address; (2) use-after-free where freed memory was reused and now holds a corrupted value; (3) a genuine off-by-huge-amount pointer arithmetic bug; (4) stack corruption (buffer overflow) overwriting a saved pointer. |
| **Observations** | Note the exact faulting address; note whether it's reproducible or intermittent (intermittent suggests uninitialized memory/UB, consistently reproducible suggests a logic bug). |
| **Tools** | `cat /proc/PID/maps` (confirm the address is outside every legitimately mapped region); `dmesg` (some fault types are logged); `gdb` with a core dump (`ulimit -c unlimited` beforehand); `valgrind --tool=memcheck` or AddressSanitizer (`-fsanitize=address`) for a much earlier, more precise catch. |
| **Evidence** | `/proc/PID/maps` shows no VMA covering the address (confirms it's not a "real" mapped region gone wrong — likely garbage or a corrupted pointer, not a legitimate-but-wrongly-permissioned access); ASan/valgrind pinpoint the exact line where the pointer was last validly written vs. where it was read. |
| **Root Cause** | (Case-dependent — this scenario is a template.) Commonly: reading a `malloc`'d struct field before initializing it, or dereferencing a pointer after its backing memory was `free()`'d. |
| **Fix** | Initialize all pointer fields explicitly (or use `calloc`); set pointers to `NULL` after `free()` and check before dereferencing; fix the specific logic bug ASan/valgrind identified. |
| **Verification** | Re-run under ASan/valgrind with zero errors reported; re-run the original reproduction steps and confirm no crash. |

---

*(This file will grow substantially starting with Chapter 7 (interrupt-context misuse), Chapter 9-10 (races, deadlocks, "sleeping function called from invalid context"), Chapter 12/15 (memory-management failures), Chapter 17 (module load/unload failures), and Chapter 18 (systematic kernel debugging with `kgdb`/`crash`/`ftrace`) — each chapter contributes its own symptom-indexed entries here, in this same table format.)*