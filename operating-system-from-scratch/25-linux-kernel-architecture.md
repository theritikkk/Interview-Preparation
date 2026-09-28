# 25 — Linux Kernel Architecture

## Layer map for this file

```
┌─────────────────────────────────────────────────┐
│ 1. BOOK: none. But everything the book (and     │
│    files 14–24) built is a miniature of         │
│    something here. This file is the map         │
├─────────────────────────────────────────────────┤
│ 2. CORE CONCEPT: a monolithic kernel, its       │
│    subsystems, and the three contexts kernel    │
│    code runs in                                 │
├─────────────────────────────────────────────────┤
│ 3. MODERN: Linux is the modern system — this    │
│    layer is the whole file                      │
└─────────────────────────────────────────────────┘
```

Book coverage: **None**, by design. This is a bridge chapter: it turns
the toy OS you've been building into a lens for reading a real one.
Nothing here is Blundell's; everything here is built on top of things he
(and this course) taught.

## Prerequisites

Files `01`–`24`. This is the file that makes them pay off.

---

## The Whole Picture

![Linux architecture as layers: user space with applications and libc, the system call interface, then the kernel containing process and scheduler, memory management, VFS and filesystems, networking, IPC and security, the block layer, device drivers, and the architecture layer, above the hardware; notes on the three contexts kernel code runs in](images/25a-linux-architecture.svg)

That is file `01`'s abstraction stack with every rung filled in by a
real subsystem. Each box carries the source directory where it lives
and the course file where you built its miniature.

## [CORE CONCEPT] Monolithic, but Modular

Linux is a **monolithic kernel**: the scheduler, memory manager,
filesystems, network stack, and drivers all live in *one* program, in
one address space, all at ring 0. A bug in a driver can therefore
corrupt anything — the same trade-off your toy kernel has. (The
alternative, the **microkernel**, keeps most of this outside ring 0;
file `36` compares the designs.)

"Monolithic" doesn't mean "fixed at build time." **Loadable kernel
modules** (`.ko` files) let code be linked into the running kernel on
demand — `insmod`, `modprobe`, `lsmod`. A module is still kernel code
at ring 0; the flexibility is in *when* it's loaded, not *how
protected* it is.

## [CORE CONCEPT] Three Contexts for Kernel Code

Your toy kernel ran as one flow. Linux kernel code runs in three
distinct situations, and which one you're in decides what you may do:

| Context | Runs when | May it sleep? | Analog in your toy OS |
|---|---|---|---|
| **Process context** | inside a syscall, on behalf of a process | yes | file `21`'s handler |
| **Interrupt context** | inside an interrupt handler | **no** | file `14`'s ISR |
| **Kernel thread** | scheduled like a process, never in user space | yes | (no equivalent yet) |

Kernel threads show up in `ps` in square brackets: `[kthreadd]`
(always PID 2), `[ksoftirqd/0]`, `[kworker/...]`. They do deferred work
that an interrupt handler shouldn't do itself (file `14`'s "deferred
work"). Interrupt context can't sleep for the reason file `23` gave:
there's no process to move to Waiting.

## [CORE CONCEPT] Boot, Side by Side

![Six-row comparison of the toy OS's boot against Linux x86-64's boot: BIOS loads boot sector vs firmware loads GRUB or EFI stub; boot sector reads kernel vs GRUB loads kernel and initramfs; real to protected mode vs arch/x86/boot and compressed/head_64.S to long mode; kernel_entry.asm calls main vs startup_64 to start_kernel; toy subsystem setup vs start_kernel initializing everything; a shell vs kernel_init becoming PID 1](images/25b-boot-comparison.svg)

Two details worth naming. The first stage of Linux's own x86 boot code
lives in `arch/x86/boot/` — real-mode code, which is file `04`'s
"every x86 CPU starts in real mode" applied to a production kernel. And
`start_kernel()` in `init/main.c` is Linux's `main()`: it calls dozens
of `*_init()` functions, one per subsystem, ending in `rest_init()`,
which creates **PID 1** (`kernel_init`, which finally `exec`s
`/sbin/init` — usually systemd) and **PID 2** (`kthreadd`).

---

## The Rosetta Table: Your Toy OS → Linux

This is the file's core deliverable. Every row is something you built
or studied; the last two columns say where Linux does the same job.

| You built / studied | File | Linux mechanism | Where to look | See it live |
|---|---|---|---|---|
| Boot sector + `0xAA55` | `06` | GRUB / EFI stub | `arch/x86/boot/`, `drivers/firmware/efi/libstub/` | `ls /boot`, `efibootmgr` |
| GDT, mode switch | `08` | Early boot GDT, long-mode entry | `arch/x86/boot/compressed/head_64.S`, `arch/x86/kernel/cpu/common.c` | — |
| `kernel_entry.asm` → `main()` | `12` | `startup_64` → `start_kernel()` | `arch/x86/kernel/head_64.S`, `init/main.c` | `dmesg \| head` |
| Makefile | `13` | Kbuild | top-level `Makefile`, per-directory `Makefile`/`Kconfig` | `/boot/config-$(uname -r)` |
| IDT + ISR stubs | `14` | IDT setup + generic IRQ layer | `arch/x86/kernel/idt.c`, `kernel/irq/` | `cat /proc/interrupts` |
| 8259 PIC | `14` | APIC / IOAPIC | `arch/x86/kernel/apic/` | `/proc/interrupts` per-CPU columns |
| `port_byte_in/out` | `15` | `inb`/`outb`, MMIO accessors | `arch/x86/include/asm/io.h` | `cat /proc/ioports` |
| Screen driver | `16` | VGA text console (and much more) | `drivers/video/console/vgacon.c` | `/dev/tty*` |
| Keyboard driver | `16` | i8042 + AT keyboard | `drivers/input/serio/i8042.c`, `drivers/input/keyboard/atkbd.c` | `cat /proc/bus/input/devices` |
| BIOS `int 0x13` / ATA | `17` | Block layer + NVMe/AHCI drivers | `block/`, `drivers/nvme/host/` | `lsblk`, `cat /proc/diskstats` |
| FAT12 reader | `18` | VFS + filesystem drivers | `fs/`, `fs/ext4/`, `fs/fat/` | `cat /proc/filesystems`, `mount` |
| PCB | `19` | `task_struct` | `include/linux/sched.h` | `cat /proc/<pid>/status` |
| `switch(from, to)` | `20` | `schedule()`, `context_switch()` | `kernel/sched/core.c` | `perf sched`, `/proc/<pid>/sched` |
| `int 0x80` handler | `21` | `entry_SYSCALL_64`, syscall table | `arch/x86/entry/entry_64.S`, `arch/x86/entry/syscalls/syscall_64.tbl` | `strace` |
| Page tables, vector 14 | `22` | Page-fault handler, `mm/` | `arch/x86/mm/fault.c`, `mm/memory.c` | `cat /proc/<pid>/maps` |
| `xchg` spinlock | `23` | `spinlock_t`, `mutex`, lockdep | `include/linux/spinlock.h`, `kernel/locking/` | — |
| `-smp 2`, BSP/AP | `24` | Per-CPU run queues, SMP boot | `arch/x86/kernel/smpboot.c`, `kernel/sched/` | `lscpu`, `mpstat -P ALL 1` |

Read down the "You built" column and notice: *every row has a
counterpart*. That's the point. Nothing in Linux is a fundamentally
different kind of thing from what you built — it's the same
mechanism, generalized, hardened, optimized, and made configurable.

---

## [MODERN SYSTEM] What Linux Adds That the Toy Never Faces

- **Scale.** Tens of millions of lines of source, most of it in
  `drivers/`. You cannot read it top to bottom; file `26` is about
  navigating it instead.
- **Configurability.** `make menuconfig` and Kconfig select which of
  thousands of features are compiled in at all. Two Linux kernels can
  differ enormously.
- **Many architectures.** `arch/` holds x86, ARM64, RISC-V and more;
  the rest of the tree is written to be architecture-independent, with
  `arch/` as the one place hardware-specific code is supposed to live.
- **Stable user interface.** Syscalls and `/proc`, `/sys` are promised
  not to break user programs. *Internal* kernel interfaces, by
  contrast, change freely — one reason out-of-tree drivers are painful.
- **Everything is a file.** Devices appear as files in `/dev` (file
  `16`'s character/block devices), kernel state as files in `/proc` and
  `/sys` — so the same `open`/`read`/`write` syscalls reach a disk file,
  a device, and a kernel counter alike.

---

## Practical Experiment

1. `uname -r`, then `cat /proc/version` — which kernel, built how.
2. `ps -ef | head -20` — find **PID 1** and **`[kthreadd]` as PID 2**;
   count the bracketed kernel threads.
3. `dmesg | head -100` — file `01`'s experiment again, now readable:
   match lines to the boot-comparison diagram (firmware handoff, CPU
   detection, memory map, subsystem init messages).
4. `lsmod | head` and `modinfo <one of them>` — see real loadable
   modules, their sizes, and their dependencies.
5. Pick five rows of the Rosetta table and run each "See it live"
   command. For each, write one sentence connecting the output to the
   toy-OS file it corresponds to.
6. `cat /proc/interrupts` while typing and moving the mouse: file `14`
   and file `24` on one screen.

## Debugging

The user-facing debugging chain from earlier files, now with real
subsystem names — a symptom lives in one of these, and each has a
tool:

```
Application slow or stuck
   → strace -T            which syscall, how long          (file 21)
   → VFS / filesystem     open, read, write paths           (fs/)
   → page cache           cached or not?                    (mm/)
   → block layer          iostat -x, blktrace               (block/)
   → driver               dmesg errors, /sys/block/...      (drivers/)
   → device               smartctl, hardware itself
```

The discipline is the one from file `01`: identify the layer *first*,
then choose the tool. Read `dmesg` early — the kernel reports driver
errors, oopses, and hardware faults there.

## Source Code

You aren't writing new code here. Browse the paths in the Rosetta
table, without trying to understand every line. File `26` teaches how.

---

## Common Misconceptions

- **"Linux is an operating system."** Strictly, it's the kernel. The
  operating system you use is a **distribution**: that kernel plus
  glibc, systemd, shells, and packages (file `01`'s distinction).
- **"The kernel is a process running in the background."** It isn't a
  process. It's code that runs *in the context of* whichever process
  made a syscall, or in an interrupt, or in kernel threads.
- **"Monolithic means no modules."** Modules exist; they just run at
  the same privilege and in the same address space as everything else.
- **"Linux is mostly scheduler and memory manager."** By size, the
  driver tree dominates.
- **"A real kernel is fundamentally different from a toy one."** It's
  the same set of mechanisms, at scale — the whole point of this table.

## Interview Questions

**Basic:** What does "monolithic kernel" mean, and how do loadable
modules fit that?
**Intermediate:** What are the three contexts Linux kernel code runs
in, and which one can't sleep?
**Advanced:** Walk through what `start_kernel()` is responsible for and
how it ends.
**Systems Design:** What is the practical cost of putting drivers at
ring 0 in the same address space as everything else, and what would a
microkernel change?
**Debugging:** A program is slow reading files. Name the layers between
`read()` and the disk, and one tool per layer.
**Explain what happens when...:** a Linux machine boots, from the
firmware handoff to a login prompt, naming the source directory
involved at each stage?

## Active Recall

1. Where does Linux's counterpart to your `kernel_entry.asm` → `main()`
   live?
2. What are PID 1 and PID 2?
3. Why can't interrupt-context code sleep?
4. Which layer holds the largest share of the source tree?
5. What's the difference between Linux and a Linux distribution?

## Answers

1. `startup_64` (`arch/x86/kernel/head_64.S`), leading to
   `start_kernel()` in `init/main.c`.
2. PID 1 is `kernel_init`, which becomes `/sbin/init` (usually systemd);
   PID 2 is `kthreadd`, parent of the kernel threads.
3. There is no process to move to the Waiting state.
4. Device drivers, in `drivers/`.
5. Linux is the kernel; a distribution bundles it with libc, an init
   system, tools, and package management.

---

## What You Should Now Understand

- Linux as file `01`'s stack with every layer implemented — the same
  mechanisms, at scale.
- The Rosetta table: what every piece of your toy OS corresponds to,
  and where to find it.
- Monolithic-but-modular, and the three contexts kernel code runs in.
- That a kernel is not a background process, and that Linux is not the
  same thing as a distribution.

---

**Next:** file `26-linux-source-code-navigation.md`.
