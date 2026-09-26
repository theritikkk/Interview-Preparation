# Source Code Map

> Your guide to the Linux kernel source tree itself. This file grows with every chapter — each one adds the specific files/functions/structures it covers. Read this file's top section once now; treat the per-chapter sections as reference material to return to.

---

## How to Get the Source

```bash
git clone --depth 1 https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git ~/linux
cd ~/linux
```
A shallow clone is enough for reading current code. If you later want history (`git log`, `git blame`) on a specific file:
```bash
git fetch --unshallow   # multi-GB, be patient — or just do this once you actually need history
```

---

## Top-Level Directory Tour

| Directory | Responsibility | First relevant chapter |
|---|---|---|
| `arch/` | Architecture-specific code (x86, arm64, riscv, ...) — boot, low-level entry/exit, page table formats, context-switch assembly | Ch.1 (mentioned), Ch.3-5 (used directly) |
| `block/` | Block I/O layer — request queues, I/O schedulers | Ch.14 |
| `certs/` | Kernel module/kexec signing infrastructure | (not covered in depth) |
| `crypto/` | Cryptographic algorithm implementations used internally | (not covered in depth) |
| `Documentation/` | Formal process docs, subsystem documentation, coding style | Ch.1, Ch.20 |
| `drivers/` | The overwhelming majority of the tree's code — device drivers of every kind | Ch.17 |
| `fs/` | Filesystems and the VFS (Virtual Filesystem) abstraction layer | Ch.13 |
| `include/` | Public and internal headers; `include/linux/` holds most core data structure declarations (`task_struct`, etc.) | Every chapter |
| `init/` | Kernel boot/initialization; `init/main.c` is the closest thing to "where the kernel proper begins" after architecture-specific early boot | Ch.2 |
| `ipc/` | System V IPC — semaphores, shared memory, message queues | (touched in Ch.9-10 context) |
| `kernel/` | The core kernel: process management, scheduling, most syscall infrastructure, `kernel/sched/`, `kernel/irq/`, etc. | Ch.3, 4, 5, 7, 8, 9, 10, 11 |
| `lib/` | Generic, architecture-independent library code (some Ch.6 data structures live here) | Ch.6 |
| `mm/` | Memory management | Ch.12, 15, 16 |
| `net/` | The networking stack | (connections drawn throughout in "Connection to Systems Engineering" sections) |
| `scripts/` | Build system and developer tooling | Ch.2, 20 |
| `security/` | LSM framework, SELinux, and related | (not covered in depth) |
| `sound/` | Audio subsystem | (not covered in depth) |
| `tools/` | User-space tools shipped alongside the kernel — `perf` lives here | Ch.18 |

## Navigation Tools — How and When to Use Each

### `ripgrep` (`rg`) — your default, fastest tool
```bash
rg -n "task_struct" kernel/ include/linux/sched.h   # -n shows line numbers
rg -t c "spin_lock" kernel/                          # -t c restricts to C files
```
Use for: quick, ad-hoc searches across the whole tree. This is what you'll reach for 90% of the time.

### `git grep` — like `rg`, but respects the git tree exactly and is always available if you have the repo
```bash
git grep -n "asmlinkage long sys_"
git grep -n "SYSCALL_DEFINE" -- '*.c'
```
Use for: the same job as `rg`, when you specifically want results scoped to tracked files only, or when `rg` isn't installed.

### `git log` / `git blame` — history, not just current content
```bash
git log --oneline -- kernel/sched/core.c | head       # recent commits touching this file
git blame -L 100,130 kernel/fork.c                      # who wrote these specific lines, and when
```
Use for: understanding *why* code looks the way it does — a huge advantage of open kernel development (Ch.1) is that this history is always available.

### `ctags` / `cscope` — jump-to-definition, jump-to-references
```bash
ctags -R .          # build a tag index once (regenerate after pulling new code)
cscope -Rbq          # build a cscope database
```
Use for: editor integration (Vim, Emacs, and many others support both natively) — once built, "go to definition" and "find all callers" become instant, essential for tracing execution across a codebase this large. This course will lean on this heavily from Chapter 3 onward.

### `pahole` — inspect a compiled structure's real, compiler-decided memory layout
```bash
pahole task_struct    # requires debug info available, e.g. on a locally-built kernel with CONFIG_DEBUG_INFO
```
Use for: seeing exactly how a struct like `task_struct` is laid out in memory once the compiler applies padding/alignment — introduced properly once we reach data-structure-heavy chapters (Ch.6, 12).

### `objdump` / `readelf` / `nm` — inspect compiled binaries and modules
```bash
objdump -d my_module.ko | less    # disassemble a compiled kernel module
readelf -S my_module.ko            # inspect ELF sections
nm my_module.ko                     # list symbols
```
Use for: Chapter 17 (modules) and Chapter 18 (debugging), when you need to inspect what the compiler/linker actually produced, not just what you wrote.

### `gdb` / `crash` — live and post-mortem kernel debugging
Introduced properly in Chapter 18, where you'll set up `kgdb`/`crash` against a real (VM) kernel.

---

## Chapter 1's Contribution to This Map

Chapter 1 didn't touch specific functions yet — it oriented you to the tree's *shape*. The one concrete pointer from Chapter 1:

```bash
# Where the x86-64 syscall entry mechanism (conceptually introduced in Ch.1)
# actually lives — full detail comes in Chapter 5.
arch/x86/entry/entry_64.S            # low-level asm entry point
arch/x86/entry/common.c              # do_syscall_64() -- the C-level dispatcher
arch/x86/include/generated/uapi/asm/unistd_64.h   # syscall numbers (__NR_*)
```


### Chapter 2's Contribution to This Map
- `MAINTAINERS` — who maintains each subsystem
- `scripts/get_maintainer.pl` — query MAINTAINERS for a specific file
- `Documentation/` — in-tree documentation (Sphinx-based on modern kernels)
- `lib/string.c` — kernel's own string functions (replacing libc)
- `include/linux/printk.h` — printk, pr_err, pr_info, etc.
- `Makefile` (top-level) — drives the entire build
- Build artifacts: `vmlinux`, `bzImage`, `System.map`

### Chapter 3's Contribution to This Map
Key files:
- `include/linux/sched.h` — task_struct definition (the single most important header in the kernel)
- `include/linux/sched/task.h` — task lifecycle helpers
- `kernel/fork.c` — fork, clone, copy_process, dup_task_struct implementation
- `kernel/exit.c` — do_exit, exit_notify, release_task
- `kernel/pid.c` — PID allocation and management
- `arch/x86/include/asm/current.h` — current macro (x86-64)
- `arch/x86/include/asm/thread_info.h` — thread_info structure (x86)
- `include/linux/kthread.h` — kernel thread creation API
- `kernel/kthread.c` — kernel thread implementation

Key structures: task_struct, thread_info, pid
Key functions: copy_process, dup_task_struct, do_fork/kernel_clone, do_exit, release_task, alloc_pid, kthread_create, kthread_run

Navigation commands:
```bash
rg -n "struct task_struct {" include/linux/sched.h
rg -n "copy_process" kernel/fork.c
rg -n "do_exit" kernel/exit.c
rg -n "DEFINE_PER_CPU.*current_task" arch/x86/
git grep -n "for_each_process" -- '*.h'
```

Call chain:
```
User: fork()
  → libc wrapper
    → syscall (clone/clone3)
      → kernel_clone() [kernel/fork.c]
        → copy_process()
          → dup_task_struct()
          → copy_mm() / copy_files() / copy_fs() / ...
          → alloc_pid()
        → wake_up_new_task()
```

### Chapter 4's Contribution to This Map
Key files:
- `kernel/sched/` — the entire scheduler subsystem lives here
- `kernel/sched/core.c` — schedule(), context_switch(), the main scheduler logic
- `kernel/sched/fair.c` — CFS/EEVDF fair scheduling implementation
- `kernel/sched/rt.c` — real-time scheduling (SCHED_FIFO, SCHED_RR)
- `kernel/sched/deadline.c` — deadline scheduling (SCHED_DEADLINE)
- `kernel/sched/sched.h` — internal scheduler headers, struct rq (runqueue)
- `include/linux/sched.h` — task_struct with embedded sched_entity
- `arch/x86/include/asm/switch_to.h` — low-level context switch
- `kernel/sched/idle.c` — idle task scheduling

Key structures: sched_entity, sched_class, rq (runqueue), cfs_rq
Key functions: schedule(), pick_next_task(), context_switch(), switch_mm(), switch_to(), update_curr(), enqueue_task(), dequeue_task(), check_preempt_curr(), wake_up_process()

Navigation commands:
```bash
rg -n "struct sched_entity" include/linux/sched.h
rg -n "static void update_curr" kernel/sched/fair.c
rg -n "context_switch" kernel/sched/core.c
rg -n "pick_next_task" kernel/sched/core.c
git grep -n "need_resched" -- '*.h'
```

Call chain:
```
Timer interrupt
  → scheduler_tick() [kernel/sched/core.c]
    → task_tick_fair() [kernel/sched/fair.c]
      → update_curr() — update vruntime
      → check if need_resched should be set
  → on return from interrupt:
    → schedule() [kernel/sched/core.c]
      → pick_next_task()
      → context_switch()
        → switch_mm() — switch page tables
        → switch_to() — switch registers/stack
```


### Chapter 5's Contribution to This Map
Key files:
- `arch/x86/entry/entry_64.S` — x86-64 syscall entry point (`entry_SYSCALL_64`), the assembly code that runs when the `syscall` instruction fires
- `arch/x86/entry/common.c` — `do_syscall_64()`, the C-level syscall dispatcher
- `arch/x86/entry/syscalls/syscall_64.tbl` — the syscall number table for x86-64
- `include/linux/syscalls.h` — SYSCALL_DEFINEn() prototypes for all system calls
- `include/uapi/asm-generic/unistd.h` — generic (architecture-independent) syscall numbers
- `include/linux/uaccess.h` — copy_from_user(), copy_to_user(), and related safe-access functions
- `kernel/sys.c` — implementations of various system calls
- `arch/x86/include/asm/vdso/` — vDSO implementation

Key structures: pt_regs, sys_call_table
Key functions: do_syscall_64(), entry_SYSCALL_64, copy_from_user(), copy_to_user(), capable(), syscall_get_nr()
Key macros: SYSCALL_DEFINE0..6(), __user

Navigation commands:
```bash
rg -n "entry_SYSCALL_64" arch/x86/entry/entry_64.S
rg -n "do_syscall_64" arch/x86/entry/common.c
rg -n "SYSCALL_DEFINE" include/linux/syscalls.h | head -20
rg -n "copy_from_user" include/linux/uaccess.h
cat arch/x86/entry/syscalls/syscall_64.tbl | tail -20  # newest syscalls
```

Call chain:
```
User: read(fd, buf, count)
  → glibc wrapper: sets rax=__NR_read, rdi=fd, rsi=buf, rdx=count
    → syscall instruction (privilege transition to ring 0)
      → entry_SYSCALL_64 [arch/x86/entry/entry_64.S]
        → saves registers into pt_regs
        → do_syscall_64() [arch/x86/entry/common.c]
          → sys_call_table[__NR_read] → __x64_sys_read()
            → ksys_read() → vfs_read() → file->f_op->read()
              → copy_to_user(buf, kernel_buf, count)
          → return value in rax
        → sysret instruction (return to ring 3)
      → glibc checks rax; if negative, sets errno = -rax, returns -1
```


### Chapter 6's Contribution to This Map
Key files:
- `include/linux/list.h` — THE most important header for reading kernel code. Defines list_head and all list manipulation/iteration macros
- `include/linux/container_of.h` — container_of() macro (also historically in `include/linux/kernel.h`)
- `include/linux/rbtree.h` — red-black tree API (rb_node, rb_root, rb_link_node, rb_insert_color, rb_erase)
- `lib/rbtree.c` — red-black tree implementation
- `include/linux/kfifo.h` — kfifo API
- `kernel/kfifo.c` — kfifo implementation
- `include/linux/idr.h` — IDR/IDA integer-to-pointer mapping
- `include/linux/xarray.h` — [MODERN] XArray (replaced many radix tree/IDR uses)
- `lib/idr.c` — IDR implementation
- `include/linux/types.h` — struct list_head definition

Key structures: list_head, rb_node, rb_root, kfifo
Key macros: container_of(), list_for_each_entry(), list_for_each_entry_safe(), list_add(), list_del(), LIST_HEAD(), rb_entry()
Key functions: rb_link_node(), rb_insert_color(), rb_erase(), rb_first(), rb_next(), kfifo_alloc(), kfifo_in(), kfifo_out(), idr_alloc(), idr_find(), idr_remove()

Navigation commands:
```bash
rg -n "struct list_head" include/linux/types.h
rg -n "container_of" include/linux/container_of.h
rg -n "list_for_each_entry" include/linux/list.h
rg -c "struct list_head" include/linux/ | sort -t: -k2 -rn | head  # most list-heavy headers
rg -n "struct rb_node" include/linux/rbtree.h
```

### Chapter 7's Contribution to This Map
Key files:
- `kernel/irq/` — the generic IRQ subsystem (genirq)
- `kernel/irq/handle.c` — generic interrupt dispatching
- `kernel/irq/manage.c` — request_irq(), free_irq(), request_threaded_irq()
- `kernel/irq/chip.c` — irq_chip abstraction for interrupt controllers
- `arch/x86/kernel/irq.c` — x86-specific interrupt handling
- `arch/x86/entry/entry_64.S` — interrupt/exception entry points (also used by syscalls)
- `include/linux/interrupt.h` — IRQ handler API, IRQF_* flags, tasklet declarations
- `include/linux/irqreturn.h` — IRQ_NONE, IRQ_HANDLED, IRQ_WAKE_THREAD
- `include/linux/irq.h` — irq_desc, irq_chip structures
- `/proc/interrupts` — runtime interrupt statistics

Key structures: irq_desc, irq_chip, irq_data, irqaction
Key functions: request_irq(), free_irq(), request_threaded_irq(), do_IRQ(), local_irq_disable(), local_irq_enable(), local_irq_save(), local_irq_restore(), disable_irq(), enable_irq(), in_interrupt(), in_irq()

Navigation commands:
```bash
rg -n "request_irq" include/linux/interrupt.h
rg -n "do_IRQ" arch/x86/kernel/irq.c
rg -n "struct irq_desc" include/linux/irq.h
rg -n "IRQF_SHARED" include/linux/interrupt.h
cat /proc/interrupts  # runtime observation
```

Interrupt flow:
```
Hardware device asserts interrupt
  → Interrupt controller (APIC) routes to CPU
    → CPU looks up IDT[vector]
      → Saves state, jumps to kernel entry
        → Kernel entry code (entry_64.S)
          → Calls handler dispatch
            → irq_desc[irq].action→handler()
              → Top half runs (fast, interrupts disabled on this line)
              → Returns IRQ_HANDLED or IRQ_NONE
          → ret_from_intr: check softirqs, need_resched
        → Restore state, iret back to interrupted code
```

*(Every subsequent chapter appends its own section here, in the same format: which files, which structures, which functions, and how they call into each other.)*