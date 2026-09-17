# Glossary

> Every important term from this course, defined once, with why it matters and where it appears in the kernel. Organized alphabetically. Each entry notes which chapter introduced it. This file grows with every chapter — re-check it periodically rather than treating it as finished.

---

### `current`
**Definition:** A per-CPU pointer (implemented via an architecture-specific mechanism, not a global variable, since every CPU has its own "currently running task") to the `task_struct` of the process presently executing on that CPU.
**Why it matters:** It's how kernel code — running in process context — knows *whose behalf* it's running on. Introduced conceptually in Ch.1, given full structural treatment in Ch.3.
**Related concepts:** process context, `task_struct`.
**Introduced:** Chapter 1; detailed: Chapter 3.

### Interrupt
**Definition:** A signal from hardware (or, for a "software interrupt," from software) that asynchronously diverts the CPU to a kernel-controlled handler, regardless of what was executing at the time.
**Why it matters:** It's the *hardware-initiated* counterpart to a system call's *software-initiated* kernel entry — together they are the only two sanctioned doors into kernel-space.
**Related concepts:** interrupt context, interrupt handler, bottom half.
**Introduced:** Chapter 1; detailed: Chapter 7.

### Interrupt Context
**Definition:** The execution context the kernel is in while handling a hardware interrupt — kernel-mode, kernel-space, but *not* associated with any particular process, and subject to strict constraints (cannot sleep, must be fast).
**Why it matters:** Defines what kernel code is and isn't allowed to do — the foundation for why bottom halves (Ch.8) and most synchronization primitives (Ch.9-10) are designed the way they are.
**Related concepts:** process context, `current`, bottom half, atomic context.
**Introduced:** Chapter 1.

### Kernel
**Definition:** The innermost, most-privileged part of an operating system — the software layer that directly manages hardware and provides the basic services (scheduling, memory management, I/O) every other part of the system relies on.
**Why it matters:** Distinguishing "kernel" from "operating system" (the broader collection including the boot loader, shell, and basic utilities) is the very first conceptual move this entire course makes.
**Related concepts:** operating system, monolithic kernel, microkernel.
**Introduced:** Chapter 1.

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

### Microkernel
**Definition:** A kernel architecture where most kernel functionality is implemented as separate, isolated server processes communicating via message passing (IPC), rather than as one program in one address space.
**Why it matters:** The contrast case that clarifies, by comparison, exactly what "monolithic" means and what tradeoffs Linux's monolithic design makes deliberately.
**Related concepts:** monolithic kernel, IPC, fault isolation.
**Introduced:** Chapter 1.

### Monolithic Kernel
**Definition:** A kernel architecture where the entire kernel — process management, memory management, filesystems, networking, device drivers — runs as one program in one address space, using direct function calls for internal communication.
**Why it matters:** This is what Linux is. Understanding it (and its deliberate borrowing of microkernel-style modularity via loadable modules) explains why device drivers "are the kernel" rather than separate programs.
**Related concepts:** microkernel, loadable kernel module.
**Introduced:** Chapter 1.

### Operating System
**Definition:** The broader software collection responsible for basic system use and administration: the kernel, device drivers, the boot loader, a shell or basic interface, and basic utilities.
**Why it matters:** Distinct from — and broader than — "the kernel." This course is about the kernel specifically.
**Related concepts:** kernel.
**Introduced:** Chapter 1.

### Privilege Level ("Ring," on x86)
**Definition:** A hardware-enforced CPU execution mode determining which instructions are permitted. x86 defines rings 0 (most privileged) through 3 (least privileged); Linux uses only ring 0 (kernel mode) and ring 3 (user mode).
**Why it matters:** This is the actual hardware mechanism underneath the kernel-space/user-space software abstraction — the "master key" in the Ch.1 building analogy is real silicon, not just a convention.
**Related concepts:** kernel-space, user-space, trap instruction.
**Introduced:** Chapter 1.

### Process Context
**Definition:** The execution context the kernel is in while running kernel code on behalf of a specific, identifiable process — typically while servicing that process's system call.
**Why it matters:** The counterpart to interrupt context; code running in process context has far fewer restrictions (it may sleep, may be preempted normally) than interrupt-context code.
**Related concepts:** interrupt context, `current`, system call.
**Introduced:** Chapter 1.

### System Call
**Definition:** A well-defined, numbered request from user-space to the kernel, invoked via a trap instruction, requesting a privileged operation be performed on the calling process's behalf.
**Why it matters:** The primary, software-initiated door between user-space and kernel-space; the mechanism nearly every kernel service is ultimately reached through.
**Related concepts:** trap instruction, library call, `errno`, system call table.
**Introduced:** Chapter 1; detailed: Chapter 5.

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

### User-Space
**Definition:** The memory and privilege state associated with user mode — where ordinary application code runs, with restricted privileges and no direct hardware access.
**Why it matters:** The counterpart to kernel-space; the "employee-accessible rooms" in the Ch.1 building analogy.
**Related concepts:** kernel-space, privilege level.
**Introduced:** Chapter 1.

---

*(This glossary will grow substantially starting with Chapter 3 — `task_struct`, `jiffies`, `runqueue`, `spinlock`, `mutex`, `bio`, `inode`, `dentry`, `softirq`, `tasklet`, `workqueue`, `kobject`, `sysfs`, `VMA`, `page cache`, `slab`, and more, exactly as anticipated in the course's original plan — each will be added as its introducing chapter is written.)*