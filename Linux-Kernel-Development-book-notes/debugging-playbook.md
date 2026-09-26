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


## Scenario from Ch.2: printk() Message Never Appears

| Step | Content |
|---|---|
| **Symptom** | Added printk() to kernel code, rebuilt, booted, but message never shows in dmesg |
| **Hypotheses** | (1) code guarded by CONFIG_* not enabled; (2) booted wrong kernel (uname -r mismatch); (3) printk priority filtered by console log level; (4) log buffer wrapped |
| **Tools** | grep for #ifdef around your edit, uname -r, cat /proc/sys/kernel/printk, dmesg -T |
| **Fix** | Enable the CONFIG option, or lower the console log level, or confirm correct kernel is booted |

## Scenario from Ch.3: Zombie Process Accumulation

| Step | Content |
|---|---|
| **Symptom** | ps shows many processes in Z (zombie) state, eventually PIDs may be exhausted |
| **Hypotheses** | (1) parent process not calling wait(); (2) parent ignoring SIGCHLD; (3) parent itself is stuck |
| **Tools** | ps aux \| grep Z, /proc/PID/status (check PPid), strace parent process |
| **Fix** | Fix parent to call wait()/waitpid(), or set SIGCHLD to SIG_IGN, or kill parent (init will reap orphaned zombies) |

## Scenario from Ch.3: Process Stuck in D (Uninterruptible Sleep) State

| Step | Content |
|---|---|
| **Symptom** | process shows D state in ps, cannot be killed even with SIGKILL |
| **Hypotheses** | (1) waiting on I/O that never completes (disk failure, NFS timeout); (2) kernel bug causing indefinite wait on a resource; (3) deadlock in kernel code holding the process |
| **Tools** | cat /proc/PID/wchan, /proc/PID/stack (if CONFIG_STACKTRACE), echo t > /proc/sysrq-trigger (shows all task states) |
| **Fix** | Address the underlying I/O/resource issue; if disk, check dmesg for I/O errors; if NFS, check network/mount options |

## Scenario from Ch.3: Fork Bomb (System Unresponsive)

| Step | Content |
|---|---|
| **Symptom** | System becomes unresponsive, many processes spawning rapidly, OOM killer may trigger |
| **Hypotheses** | A process is recursively forking without bound |
| **Tools** | ps aux (if system is responsive), /proc/sys/kernel/pid_max, ulimit -u (per-user process limit) |
| **Fix** | Kill the offending process tree (kill -9 -PGID), set process limits with ulimit or cgroups, use RLIMIT_NPROC |

---

## Scenario from Ch.4: High Context-Switch Rate / System Unresponsive

| Step | Content |
|---|---|
| **Symptom** | System feels sluggish; `vmstat` shows very high `cs` (context switches per second); `top` shows high `%sy` (system time) |
| **Hypotheses** | (1) too many runnable tasks competing; (2) tasks with inappropriate nice values or RT priorities; (3) a real-time task starving normal tasks; (4) lock contention causing excessive sleeping/waking |
| **Tools** | `vmstat 1`, `perf sched record` + `perf sched latency`, `/proc/schedstat`, `/proc/sched_debug`, `top` (look at PR/NI columns) |
| **Fix** | Adjust nice values, investigate RT tasks with `chrt -p PID`, check for lock contention (Ch.9-10), reduce number of runnable tasks, use CPU affinity to spread load |

## Scenario from Ch.4: Real-Time Task Starving the System

| Step | Content |
|---|---|
| **Symptom** | System completely unresponsive; only one task seems to be running; can't even type in a terminal |
| **Hypotheses** | A SCHED_FIFO or SCHED_RR task with high priority is monopolizing CPU(s) without yielding |
| **Tools** | If accessible: /proc/sched_debug, Magic SysRq key (Alt+SysRq+T for task dump), SSH from another machine |
| **Fix** | Use `chrt -o -p 0 PID` to move task to SCHED_OTHER, or kill it. Prevention: always test RT code in VMs, set RT throttling via /proc/sys/kernel/sched_rt_runtime_us |

## Scenario from Ch.4: Scheduling Latency — Interactive App Stuttering

| Step | Content |
|---|---|
| **Symptom** | A GUI or interactive application stutters or has noticeable input lag despite low overall CPU usage |
| **Hypotheses** | (1) competing CPU-bound tasks with same or better nice value; (2) kernel preemption not enabled; (3) long-running kernel code paths not yielding |
| **Tools** | `perf sched latency`, check CONFIG_PREEMPT in /boot/config, `/proc/PID/sched` (look at nr_involuntary_switches) |
| **Fix** | Lower the nice value of the interactive task, enable PREEMPT or PREEMPT_VOLUNTARY kernel config, use ionice for I/O-bound competitors |


## Scenario from Ch.5: System Call Returns -EFAULT (Bad Address)

| Step | Content |
|---|---|
| **Symptom** | A system call that should succeed returns -1 with errno=EFAULT (14), or strace shows the call returning -EFAULT |
| **Hypotheses** | (1) passing an invalid or NULL pointer to a syscall; (2) passing a kernel-space pointer where a user-space pointer is expected; (3) user-space buffer was munmap'd or freed before the syscall; (4) pointer to stack variable that went out of scope |
| **Tools** | strace (shows arguments and return), valgrind (catches invalid memory), AddressSanitizer, review code for pointer lifetime |
| **Fix** | Ensure all pointers passed to syscalls point to valid, accessible user-space memory with correct permissions |

## Scenario from Ch.5: Mysterious Performance Regression After "Simple" Code Change

| Step | Content |
|---|---|
| **Symptom** | Application performance drops significantly; profiling shows increased time in kernel mode |
| **Hypotheses** | (1) a code change replaced a vDSO-accelerated call (gettimeofday) with a real syscall; (2) added unnecessary syscalls in a hot loop; (3) changed from buffered I/O to unbuffered, causing many more write() syscalls |
| **Tools** | strace -c (count syscalls), perf stat (count syscall events), compare strace output before/after code change |
| **Fix** | Use vDSO-friendly functions, batch I/O operations, use buffered I/O, minimize syscall count in hot paths |


## Scenario from Ch.6: List Corruption / General Protection Fault in list operations

| Step | Content |
|---|---|
| **Symptom** | Kernel oops/panic with a stack trace pointing to list_add(), list_del(), or list_for_each_entry(). Error message may include "list_add corruption" or "list_del corruption" |
| **Hypotheses** | (1) double list_del() on the same entry (poisoned pointers caught); (2) concurrent list manipulation without proper locking; (3) use-after-free of a struct containing a list_head; (4) memory corruption overwriting list_head pointers |
| **Tools** | CONFIG_DEBUG_LIST (enables runtime list integrity checking), KASAN (catches use-after-free), stack trace analysis, git blame the function in the stack trace |
| **Fix** | Add proper locking (spinlock or mutex) around list operations; ensure entries are removed from lists before being freed; use list_del_init() instead of list_del() if re-insertion is possible |

## Scenario from Ch.6: Kernel Module Crashes on Iteration After Deletion

| Step | Content |
|---|---|
| **Symptom** | A kernel module iterates a linked list and deletes entries during iteration. The module crashes with a null pointer dereference or accesses freed memory |
| **Hypotheses** | Using list_for_each_entry() instead of list_for_each_entry_safe() during deletion, causing the iterator to follow a poisoned next pointer |
| **Tools** | Code review, CONFIG_DEBUG_LIST, KASAN |
| **Fix** | Always use list_for_each_entry_safe() when removing elements during iteration. The _safe variant saves the next pointer before the loop body executes |

### Scenario from Ch.7: "BUG: sleeping function called from invalid context"
- Symptom: Kernel BUG or WARNING with stack trace showing a sleeping function (kmalloc with GFP_KERNEL, mutex_lock, copy_to_user) called from an interrupt handler
- Hypotheses: (1) interrupt handler is calling a function that can sleep; (2) code running in softirq/tasklet context (also atomic) is calling a sleeping function; (3) preemption or interrupts were disabled when the sleeping function was called
- Tools: Read the stack trace — identify which function triggered the BUG, trace back to the interrupt handler. Check if in_interrupt() or in_irq() would return true at that point
- Fix: Move the sleeping operation to a bottom half that runs in process context (workqueue), or use GFP_ATOMIC instead of GFP_KERNEL for memory allocation, or use spin_lock instead of mutex

### Scenario from Ch.7: Interrupt Storm / High CPU in Interrupt Handling
- Symptom: System sluggish or frozen; /proc/interrupts shows one IRQ firing thousands of times per second; top shows very high %hi (hardware interrupt time)
- Hypotheses: (1) hardware device misbehaving (stuck asserting interrupt); (2) interrupt handler not properly acknowledging the interrupt to hardware; (3) shared interrupt line where one device's handler incorrectly returns IRQ_HANDLED for another device's interrupt
- Tools: cat /proc/interrupts (identify the runaway IRQ), dmesg for hardware errors, watch -n1 cat /proc/interrupts to see rate
- Fix: Fix the device driver's acknowledge logic; check hardware connections; disable the faulty IRQ line temporarily with disable_irq(); replace faulty hardware

### Scenario from Ch.7: Missed Interrupts / Device Not Responding
- Symptom: A device (e.g., NIC, disk controller) stops responding; /proc/interrupts shows the IRQ count is not incrementing
- Hypotheses: (1) interrupt handler was never registered (request_irq failed); (2) IRQ was disabled and never re-enabled; (3) IRQ affinity set to a CPU that is offline; (4) interrupt controller misconfiguration; (5) MSI not properly configured
- Tools: /proc/interrupts (verify IRQ exists), dmesg (check for request_irq errors), /proc/irq/N/smp_affinity, lspci -v (check MSI status)
- Fix: Verify request_irq return value; check enable_irq/disable_irq balance; verify IRQ affinity; check MSI configuration

*(This file will grow substantially starting with Chapter 7 (interrupt-context misuse), Chapter 9-10 (races, deadlocks, "sleeping function called from invalid context"), Chapter 12/15 (memory-management failures), Chapter 17 (module load/unload failures), and Chapter 18 (systematic kernel debugging with `kgdb`/`crash`/`ftrace`) — each chapter contributes its own symptom-indexed entries here, in this same table format.)*