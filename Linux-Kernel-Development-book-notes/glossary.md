# Glossary

> Every important term from this course, defined once, with why it matters and where it appears in the kernel. Organized alphabetically. Each entry notes which chapter introduced it. This file grows with every chapter — re-check it periodically rather than treating it as finished.

---

### `.config`
**Definition:** The file, at the root of the kernel source tree, recording every `CONFIG_*` symbol's value (`y` = built-in, `m` = module, unset = excluded) — the single artifact that determines what a given kernel build actually contains.
**Why it matters:** `CONFIG_*` symbols are consumed directly by `#ifdef`s in the source, so the *same* source tree compiles to genuinely different code depending on this one file.
**Related concepts:** `make menuconfig`/`oldconfig`, Kbuild.
**Introduced:** Chapter 2.


### `clone()`
**Definition:** system call that creates a new process with fine-grained control over what is shared
**Why it matters:** Unifies thread and process creation in Linux
**Related concepts:** fork(), kernel_clone()
**Introduced:** Chapter 3


### `container_of()`
**Definition:** A macro that, given a pointer to a member embedded inside a struct, returns a pointer to the enclosing struct. Uses offsetof() pointer arithmetic. The single most important pattern for understanding kernel code.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 6.


### copy_from_user() / copy_to_user()
**Definition:** safe kernel functions for transferring data between user-space and kernel-space memory. They validate pointers and handle page faults gracefully.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 5.


### `current`
**Definition:** A per-CPU pointer (implemented via an architecture-specific mechanism, not a global variable, since every CPU has its own "currently running task") to the `task_struct` of the process presently executing on that CPU.
**Why it matters:** It's how kernel code — running in process context — knows *whose behalf* it's running on. Introduced conceptually in Ch.1, given full structural treatment in Ch.3.
**Related concepts:** process context, `task_struct`.
**Introduced:** Chapter 1; detailed: Chapter 3.


### disable_irq() / enable_irq()
**Definition:** Functions to disable/enable a specific IRQ line across all processors.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 7.

### `do_exit()`
**Definition:** kernel function that performs process termination cleanup
**Why it matters:** The internal implementation of exit()
**Related concepts:** EXIT_ZOMBIE
**Introduced:** Chapter 3


### `do_IRQ()`
**Definition:** [BOOK — Linux 2.6] The main C function dispatching hardware interrupts on x86. Modern kernels use the generic IRQ subsystem (genirq).
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 7.

### errno
**Definition:** a per-thread variable set by the C library (not the kernel) to indicate which error occurred during a failed system call.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 5.


### `for_each_process()`
**Definition:** macro to iterate over all processes in the system
**Why it matters:** Provides a way to walk the entire process list
**Related concepts:** task_struct
**Introduced:** Chapter 3


### `fork()`
**Definition:** system call that creates a child process as a copy of the parent
**Why it matters:** The fundamental UNIX process creation mechanism
**Related concepts:** clone(), Copy-on-Write (COW)
**Introduced:** Chapter 3


### `free_irq()`
**Definition:** Unregisters an interrupt handler previously registered with request_irq().
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 7.

### `in_interrupt()`
**Definition:** Kernel function returning true if code is currently executing in any interrupt context (hardirq or softirq).
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 7.

### init process (PID 1)
**Definition:** the first user-space process, ancestor of all other processes
**Why it matters:** Forms the root of the process tree and reaps orphaned zombies
**Related concepts:** Reparenting
**Introduced:** Chapter 3


### `kernel_clone() / do_fork()`
**Definition:** internal kernel function implementing fork/clone
**Why it matters:** The actual workhorse behind all process creation syscalls
**Related concepts:** fork(), clone(), vfork()
**Introduced:** Chapter 3


### `kfifo`
**Definition:** The kernel's FIFO queue implementation. Lock-free for single-producer/single-consumer. Size must be power of 2 for efficient bitmask wrap-around.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 6.


### `kthread_create() / kthread_run()`
**Definition:** API for creating kernel threads
**Why it matters:** Standard interface for kernel subsystems to spawn background workers
**Related concepts:** Kernel thread
**Introduced:** Chapter 3


### `likely()` / `unlikely()`
**Definition:** GCC branch-prediction hint macros telling the compiler (and, transitively, the CPU) which direction a conditional branch is overwhelmingly expected to go.
**Why it matters:** Correctly used, they improve performance by reducing branch mispredictions; incorrectly used, they actively *hurt* performance rather than merely failing to help.
**Related concepts:** GNU C, branch prediction.
**Introduced:** Chapter 2.


### `list_for_each_entry()`
**Definition:** A macro to iterate over a linked list, automatically recovering the enclosing struct pointer via container_of(). The standard kernel list iteration pattern.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 6.


### `list_head` (struct `list_head`)
**Definition:** The kernel's generic doubly-linked circular list node, consisting of just two pointers (next, prev). Embedded inside data structures rather than wrapping them.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 6.


### local_irq_disable() / local_irq_enable()
**Definition:** Functions to disable/enable all interrupts on the current CPU. The most basic form of interrupt-context synchronization.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 7.

### local_irq_save() / local_irq_restore()
**Definition:** Functions that save the current interrupt state before disabling, then restore it. Preferred over local_irq_disable/enable for nested contexts.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 7.

### need_resched
**Definition:** A flag (in thread_info or task_struct) indicating the scheduler should be invoked at the next opportunity.
**Why it matters:** Core concept for process scheduling.
**Related concepts:** schedule()
**Introduced:** Chapter 4


### `offsetof()`
**Definition:** A standard C macro returning the byte offset of a member within a struct. The mathematical foundation of container_of().
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 6.


### `printk()`
**Definition:** The kernel's analog of `printf()` — formats a string identically, but writes it into the kernel log buffer (readable via `dmesg`/`syslog`) rather than to a file descriptor, optionally prefixed with a priority-level macro like `KERN_ERR`.
**Why it matters:** The kernel's primary logging/diagnostic mechanism, used constantly throughout kernel code and in this course's own examples.
**Related concepts:** `pr_err()`/`pr_info()`/etc. (modern convenience wrappers), `dmesg`.
**Introduced:** Chapter 2.


### pt_regs
**Definition:** the structure holding saved user-space register state upon kernel entry (syscall or interrupt).
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 5.


### `rb_node` / `rb_root`
**Definition:** The kernel's red-black tree node and root structures. Embedded inside data structures (same pattern as list_head). Provides O(log N) sorted operations.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 6.


### `request_irq()`
**Definition:** Registers an interrupt handler for a given IRQ number. Can sleep (allocates memory), so cannot be called from interrupt context.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 7.

### `request_threaded_irq()`
**Definition:** [MODERN LINUX] Registers an interrupt handler with a threaded bottom-half handler that runs in process context.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 7.

### sched_entity
**Definition:** Structure embedded in task_struct containing per-task scheduling information (vruntime, load weight, etc.).
**Why it matters:** Core concept for process scheduling.
**Related concepts:** schedule()
**Introduced:** Chapter 4


### schedule()
**Definition:** The main scheduler entry point; selects the next task to run and performs a context switch.
**Why it matters:** Core concept for process scheduling.
**Related concepts:** schedule()
**Introduced:** Chapter 4


### sys_call_table
**Definition:** the kernel's array of function pointers indexed by syscall number, mapping each number to its handler function.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 5.


### syscall (instruction)
**Definition:** the x86-64 CPU instruction that performs a fast privilege transition from user mode to kernel mode at the kernel's controlled entry point. Replaces the older int 0x80.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 5.


### SYSCALL_DEFINEn()
**Definition:** the macro family used to define system call implementations in the kernel, providing metadata, tracing hooks, and type safety.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 5.


### `task_struct`
**Definition:** the kernel's central data structure representing a process/task
**Why it matters:** The single most important structure in process management
**Related concepts:** Process descriptor, thread_info
**Introduced:** Chapter 3


### `thread_info`
**Definition:** small structure linked to task_struct, historically at the base of the kernel stack
**Why it matters:** Contains low-level, architecture-specific process state
**Related concepts:** task_struct
**Introduced:** Chapter 3


### __user annotation
**Definition:** a sparse-checker annotation marking pointers as pointing to user-space memory, helping catch unsafe direct accesses.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 5.


### vDSO (Virtual Dynamic Shared Object)
**Definition:** kernel-mapped pages in user-space allowing certain "syscalls" (gettimeofday, clock_gettime) to execute without a privilege transition.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 5.


### `vfork()`
**Definition:** variant of fork() that shares address space and suspends parent until child execs/exits
**Why it matters:** Historic optimization before COW, still used in memory-constrained environments
**Related concepts:** fork()
**Introduced:** Chapter 3


### vruntime (virtual runtime)
**Definition:** A per-task counter tracking weighted CPU consumption. Tasks with the smallest vruntime are scheduled next under CFS.
**Why it matters:** Core concept for process scheduling.
**Related concepts:** schedule()
**Introduced:** Chapter 4


### `wait() / waitpid()`
**Definition:** system calls for a parent to collect a child's exit status and release its process descriptor
**Why it matters:** The required counterpart to fork() and exit() to prevent zombies
**Related concepts:** Zombie process, EXIT_ZOMBIE
**Introduced:** Chapter 3


### API (Application Programming Interface)
**Definition:** the programming interface applications use; distinct from the underlying system call implementation.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 5.


### APIC (Advanced Programmable Interrupt Controller)
**Definition:** The interrupt controller on modern x86 systems. Routes hardware interrupts from devices to CPUs.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 7.

### Bottom half
**Definition:** The deferred-work portion of interrupt handling that runs after the top half, with interrupts re-enabled. Detailed in Chapter 8.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 7.

### CFS (Completely Fair Scheduler)
**Definition:** [BOOK — Linux 2.6] The default scheduler class for normal tasks, using proportional fair-share scheduling based on virtual runtime. Replaced by EEVDF in Linux 6.6.
**Why it matters:** Core concept for process scheduling.
**Related concepts:** schedule()
**Introduced:** Chapter 4


### Context switch
**Definition:** The act of saving one process's state and loading another's, allowing the CPU to switch between tasks. Involves switching page tables (switch_mm) and processor registers (switch_to).
**Why it matters:** Core concept for process scheduling.
**Related concepts:** schedule()
**Introduced:** Chapter 4


### Cooperative multitasking
**Definition:** A multitasking model where processes voluntarily yield the CPU. Not used by Linux.
**Why it matters:** Core concept for process scheduling.
**Related concepts:** schedule()
**Introduced:** Chapter 4


### Copy-on-Write (COW)
**Definition:** memory optimization where forked processes share pages until one writes
**Why it matters:** Makes fork() extremely fast and efficient
**Related concepts:** fork(), virtual memory
**Introduced:** Chapter 3


### CPU affinity
**Definition:** Binding a process to run on specific CPUs, improving cache performance and NUMA locality.
**Why it matters:** Core concept for process scheduling.
**Related concepts:** schedule()
**Introduced:** Chapter 4


### CPU-bound process
**Definition:** A process that spends most of its time performing computation rather than waiting for I/O.
**Why it matters:** Core concept for process scheduling.
**Related concepts:** schedule()
**Introduced:** Chapter 4


### EEVDF (Earliest Eligible Virtual Deadline First)
**Definition:** [MODERN LINUX] The scheduler that replaced CFS in Linux 6.6, adding virtual deadline-based selection to the fairness model.
**Why it matters:** Core concept for process scheduling.
**Related concepts:** schedule()
**Introduced:** Chapter 4


### Exception
**Definition:** A synchronous interrupt generated by the CPU itself (e.g., divide by zero, page fault), as opposed to an asynchronous hardware interrupt.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 7.

### `EXIT_ZOMBIE`
**Definition:** process state after exit but before parent acknowledges via wait()
**Why it matters:** Prevents process descriptors from disappearing before parents can read exit statuses
**Related concepts:** do_exit(), wait()
**Introduced:** Chapter 3


### GNU C
**Definition:** The C dialect the kernel is actually written in — ISO C99 plus GCC-specific language extensions (inline functions, inline assembly, branch annotation, and more), rather than strict ANSI/ISO C alone.
**Why it matters:** Explains why kernel code sometimes looks unfamiliar even to an experienced C programmer, and why the kernel has historically been tied to GCC (with Clang/LLVM as a modern, fully-supported alternative).
**Related concepts:** inline function, `likely()`/`unlikely()`, inline assembly.
**Introduced:** Chapter 2.


### Hash table
**Definition:** A data structure providing O(1) average-case key-value lookup, used in the kernel for PID hashing, inode caching, and dentry caching (not covered in depth by Ch.6 but mentioned).
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 6.


### I/O-bound process
**Definition:** A process that spends most of its time waiting for I/O operations, running briefly when data arrives.
**Why it matters:** Core concept for process scheduling.
**Related concepts:** schedule()
**Introduced:** Chapter 4


### IDR (ID Radix)
**Definition:** A kernel data structure for mapping unique integer IDs to kernel object pointers. Backed by XArray in modern kernels. Used for file descriptor allocation, POSIX timer IDs, etc.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 6.


### IDT (Interrupt Descriptor Table)
**Definition:** An x86 CPU table mapping interrupt vectors to handler entry points.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 7.

### Inline Function
**Definition:** A function (declared `static inline`) whose body is copied directly into each call site at compile time, rather than reached via a normal call/return sequence.
**Why it matters:** Eliminates call overhead for small, hot functions, and is explicitly preferred over complex macros because inline functions get real C type-checking.
**Related concepts:** GNU C, macro.
**Introduced:** Chapter 2.


### Interrupt
**Definition:** An asynchronous signal from hardware diverting the CPU to a kernel-controlled handler, regardless of what was executing.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 1; detailed: Chapter 7.

### Interrupt Context
**Definition:** The execution context the kernel is in while handling a hardware interrupt — kernel-mode, kernel-space, but *not* associated with any particular process, and subject to strict constraints (cannot sleep, must be fast).
**Why it matters:** Defines what kernel code is and isn't allowed to do — the foundation for why bottom halves (Ch.8) and most synchronization primitives (Ch.9-10) are designed the way they are.
**Related concepts:** process context, `current`, bottom half, atomic context.
**Introduced:** Chapter 1.


### Interrupt handler (ISR)
**Definition:** A C function registered with the kernel to handle a specific interrupt. Runs in interrupt context with strict constraints (cannot sleep).
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 7.

### IRQ (Interrupt Request)
**Definition:** A unique number identifying a specific interrupt source.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 7.

### IRQ_HANDLED / IRQ_NONE
**Definition:** Return values from interrupt handlers indicating whether the interrupt was handled or not (important for shared interrupt lines).
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 7.

### `IRQF_SHARED`
**Definition:** Flag indicating multiple devices share the same interrupt line. Each handler must check if its device triggered the interrupt.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 7.

### Kernel
**Definition:** The innermost, most-privileged part of an operating system — the software layer that directly manages hardware and provides the basic services (scheduling, memory management, I/O) every other part of the system relies on.
**Why it matters:** Distinguishing "kernel" from "operating system" (the broader collection including the boot loader, shell, and basic utilities) is the very first conceptual move this entire course makes.
**Related concepts:** operating system, monolithic kernel, microkernel.
**Introduced:** Chapter 1.


### Kernel preemption
**Definition:** Linux's ability to preempt kernel code (not just user code), improving system responsiveness. Controlled by CONFIG_PREEMPT and preempt_count.
**Why it matters:** Core concept for process scheduling.
**Related concepts:** schedule()
**Introduced:** Chapter 4


### Kernel thread
**Definition:** a process that runs exclusively in kernel-space with no user address space (mm = NULL)
**Why it matters:** Allows the kernel to perform background operations
**Related concepts:** kthread_create(), kthread_run()
**Introduced:** Chapter 3


### Kernel-Space
**Definition:** The memory and privilege state associated with kernel mode — the region of a process's address space, and the CPU privilege level, reserved for the kernel and inaccessible to ordinary user-mode code.
**Why it matters:** The single most important boundary in this entire course; nearly everything else is a consequence of enforcing it safely and usably.
**Related concepts:** user-space, privilege level ("ring"), system call, trap instruction.
**Introduced:** Chapter 1.


### LKML (Linux Kernel Mailing List)
**Definition:** The historical primary mailing list where kernel patches are proposed, reviewed, and discussed.
**Why it matters:** Understanding that kernel development is a real, observable, email-and-`git`-based open process (not a closed corporate process) changes how you approach reading and eventually contributing to kernel code.
**Related concepts:** `git`, `lore.kernel.org`, subsystem maintainer.
**Introduced:** Chapter 1.


### `MAINTAINERS`
**Definition:** A top-level file in the kernel source tree listing who maintains each subsystem/file pattern, and which mailing list/git tree it belongs to.
**Why it matters:** The real, practical mechanism kernel contributors use to find out who to email/CC for a given patch — queryable via `scripts/get_maintainer.pl`.
**Related concepts:** LKML, subsystem maintainer.
**Introduced:** Chapter 2.


### Microkernel
**Definition:** A kernel architecture where most kernel functionality is implemented as separate, isolated server processes communicating via message passing (IPC), rather than as one program in one address space.
**Why it matters:** The contrast case that clarifies, by comparison, exactly what "monolithic" means and what tradeoffs Linux's monolithic design makes deliberately.
**Related concepts:** monolithic kernel, IPC, fault isolation.
**Introduced:** Chapter 1.


### Minimum granularity
**Definition:** The minimum time a task runs before being preempted, preventing excessive context switching under CFS/EEVDF.
**Why it matters:** Core concept for process scheduling.
**Related concepts:** schedule()
**Introduced:** Chapter 4


### Monolithic Kernel
**Definition:** A kernel architecture where the entire kernel — process management, memory management, filesystems, networking, device drivers — runs as one program in one address space, using direct function calls for internal communication.
**Why it matters:** This is what Linux is. Understanding it (and its deliberate borrowing of microkernel-style modularity via loadable modules) explains why device drivers "are the kernel" rather than separate programs.
**Related concepts:** microkernel, loadable kernel module.
**Introduced:** Chapter 1.


### MSI/MSI-X (Message Signaled Interrupts)
**Definition:** [MODERN LINUX] Modern interrupt delivery where devices write to special memory addresses instead of using physical interrupt lines.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 7.

### Nice value
**Definition:** An integer (-20 to +19) controlling a normal process's scheduling priority. Higher nice = lower priority = more generous to others.
**Why it matters:** Core concept for process scheduling.
**Related concepts:** schedule()
**Introduced:** Chapter 4


### Oops
**Definition:** A major kernel error, logged with a stack trace and register dump, resulting from an illegal kernel-level memory access — the kernel-space analog of a user-space `SIGSEGV`, but with no more-privileged layer to catch and cleanly contain it.
**Why it matters:** Illustrates precisely why "no memory protection" (Ch.2) is dangerous, and is a concept developed further into full kernel-debugging practice in Chapter 18.
**Related concepts:** panic, memory protection.
**Introduced:** Chapter 2; detailed: Chapter 18.


### Operating System
**Definition:** The broader software collection responsible for basic system use and administration: the kernel, device drivers, the boot loader, a shell or basic interface, and basic utilities.
**Why it matters:** Distinct from — and broader than — "the kernel." This course is about the kernel specifically.
**Related concepts:** kernel.
**Introduced:** Chapter 1.


### Panic
**Definition:** A more severe kernel failure than an oops — typically unrecoverable, halting or restarting the system.
**Why it matters:** The more severe endpoint of the same "no memory protection" risk an oops represents.
**Related concepts:** oops.
**Introduced:** Chapter 2; detailed: Chapter 18.


### PID (Process ID)
**Definition:** unique integer identifying a process
**Why it matters:** The user-space identifier for tasks
**Related concepts:** task_struct
**Introduced:** Chapter 3


### POSIX
**Definition:** a family of standards specifying the API that Unix-like operating systems should provide. Linux implements most of POSIX.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 5.


### Preemptive multitasking
**Definition:** A multitasking model where the scheduler can forcibly preempt a running process. Used by Linux.
**Why it matters:** Core concept for process scheduling.
**Related concepts:** schedule()
**Introduced:** Chapter 4


### Privilege Level ("Ring," on x86)
**Definition:** A hardware-enforced CPU execution mode determining which instructions are permitted. x86 defines rings 0 (most privileged) through 3 (least privileged); Linux uses only ring 0 (kernel mode) and ring 3 (user mode).
**Why it matters:** This is the actual hardware mechanism underneath the kernel-space/user-space software abstraction — the "master key" in the Ch.1 building analogy is real silicon, not just a convention.
**Related concepts:** kernel-space, user-space, trap instruction.
**Introduced:** Chapter 1.


### Process
**Definition:** an active, running program plus its associated resources
**Why it matters:** The fundamental abstraction of execution in the OS
**Related concepts:** task_struct, Thread
**Introduced:** Chapter 3


### Process Context
**Definition:** The execution context the kernel is in while running kernel code on behalf of a specific, identifiable process — typically while servicing that process's system call.
**Why it matters:** The counterpart to interrupt context; code running in process context has far fewer restrictions (it may sleep, may be preempted normally) than interrupt-context code.
**Related concepts:** interrupt context, `current`, system call.
**Introduced:** Chapter 1.


### Process descriptor
**Definition:** see `task_struct`
**Why it matters:** The kernel's representation of a process
**Related concepts:** task_struct
**Introduced:** Chapter 3


### Real-time priority
**Definition:** Priority level (1–99) for SCHED_FIFO and SCHED_RR tasks. RT tasks always preempt normal tasks.
**Why it matters:** Core concept for process scheduling.
**Related concepts:** schedule()
**Introduced:** Chapter 4


### Red-Black Tree (rbtree)
**Definition:** A self-balancing binary search tree guaranteeing O(log N) insertion, deletion, and lookup. Used by CFS scheduler (vruntime ordering), VMA management, and I/O schedulers.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 6.


### Reparenting
**Definition:** reassigning orphaned children to init or a subreaper
**Why it matters:** Prevents orphaned processes from becoming permanent zombies
**Related concepts:** init process, Zombie process
**Introduced:** Chapter 3


### Runqueue
**Definition:** A per-CPU data structure holding the set of runnable tasks for that CPU.
**Why it matters:** Core concept for process scheduling.
**Related concepts:** schedule()
**Introduced:** Chapter 4


### SCHED_FIFO
**Definition:** Real-time scheduling policy: run until yield or preempted by higher-priority RT task. No timeslice.
**Why it matters:** Core concept for process scheduling.
**Related concepts:** schedule()
**Introduced:** Chapter 4


### SCHED_RR
**Definition:** Real-time scheduling policy: round-robin among same-priority tasks with a timeslice.
**Why it matters:** Core concept for process scheduling.
**Related concepts:** schedule()
**Introduced:** Chapter 4


### Scheduler class (sched_class)
**Definition:** A pluggable scheduling algorithm module. Linux has stop, deadline, RT, fair, and idle classes.
**Why it matters:** Core concept for process scheduling.
**Related concepts:** schedule()
**Introduced:** Chapter 4


### System Call
**Definition:** A well-defined, numbered request from user-space to the kernel, invoked via a trap instruction, requesting a privileged operation be performed on the calling process's behalf.
**Why it matters:** The primary, software-initiated door between user-space and kernel-space; the mechanism nearly every kernel service is ultimately reached through.
**Related concepts:** trap instruction, library call, `errno`, system call table.
**Introduced:** Chapter 1; detailed: Chapter 5.


### System call number
**Definition:** a unique, architecture-specific integer identifying a system call. Once assigned, it can never change or be recycled (ABI stability).
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 5.


### `System.map`
**Definition:** A plain-text file, produced by the kernel build, mapping every kernel symbol (function/global variable name) to its address in the compiled image.
**Why it matters:** Lets debugging tools translate a raw crash-report memory address back into a human-readable function name.
**Related concepts:** oops, kernel debugging (Ch.18).
**Introduced:** Chapter 2.


### Targeted latency
**Definition:** The period over which CFS/EEVDF ensures all runnable tasks get at least one turn on the CPU.
**Why it matters:** Core concept for process scheduling.
**Related concepts:** schedule()
**Introduced:** Chapter 4


### Timeslice
**Definition:** The amount of time a process runs before the scheduler considers preempting it. CFS uses proportional shares rather than fixed timeslices.
**Why it matters:** Core concept for process scheduling.
**Related concepts:** schedule()
**Introduced:** Chapter 4


### Top half
**Definition:** The interrupt handler itself — runs immediately with the interrupt line disabled, must be fast, schedules deferred work.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 7.

### Trap Instruction
**Definition:** A special CPU instruction (`int 0x80` historically, `syscall` on modern x86-64) that user-mode code executes to deliberately and safely transition into kernel mode at a kernel-controlled entry point.
**Why it matters:** It's the *only* sanctioned mechanism for a privilege-level transition initiated by software — user code cannot simply jump into the kernel arbitrarily.
**Related concepts:** system call, privilege level, kernel entry point.
**Introduced:** Chapter 1.


### Unix
**Definition:** The multi-decade family of operating systems (and shared design philosophy) originating at Bell Labs in the 1970s, whose API (not source code) Linux implements.
**Why it matters:** "Everything is a file," small composable tools, and mechanism-not-policy are philosophical roots that show up throughout Linux's own design, even decades and a full source-code rewrite later.
**Related concepts:** POSIX, Single Unix Specification.
**Introduced:** Chapter 1.


### User preemption
**Definition:** Preemption that occurs when returning to user-space from a syscall or interrupt handler, if need_resched is set.
**Why it matters:** Core concept for process scheduling.
**Related concepts:** schedule()
**Introduced:** Chapter 4


### User-Space
**Definition:** The memory and privilege state associated with user mode — where ordinary application code runs, with restricted privileges and no direct hardware access.
**Why it matters:** The counterpart to kernel-space; the "employee-accessible rooms" in the Ch.1 building analogy.
**Related concepts:** kernel-space, privilege level.
**Introduced:** Chapter 1.


### Wait queue (wait_queue_head_t)
**Definition:** A linked list of sleeping tasks waiting for a specific condition to become true.
**Why it matters:** Core concept for process scheduling.
**Related concepts:** schedule()
**Introduced:** Chapter 4


### XArray
**Definition:** [MODERN LINUX] A data structure that replaced many uses of radix trees and IDR in modern kernels. Provides efficient integer-indexed storage with RCU-safe operations.
**Why it matters:** 
**Related concepts:** 
**Introduced:** Chapter 6.


### Zombie process
**Definition:** a terminated process whose task_struct still exists because the parent hasn't called wait()
**Why it matters:** A common resource leak if parents don't clean up correctly
**Related concepts:** wait(), EXIT_ZOMBIE
**Introduced:** Chapter 3


---


*(This glossary will grow substantially starting with Chapter 3 — `task_struct`, `jiffies`, `runqueue`, `spinlock`, `mutex`, `bio`, `inode`, `dentry`, `softirq`, `tasklet`, `workqueue`, `kobject`, `sysfs`, `VMA`, `page cache`, `slab`, and more, exactly as anticipated in the course's original plan — each will be added as its introducing chapter is written.)*