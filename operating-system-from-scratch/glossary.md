# Glossary

Every term is defined precisely and points to the course file where it's
taught in depth. Terms are alphabetical; cross-references use → *file*.

---

**ABI (Application Binary Interface)** — the binary-level contract
between pieces of compiled code: register usage, stack layout, calling
convention. The runtime counterpart to a language's API. → `10`

**APIC / IOAPIC** — the modern replacement for the 8259 PIC; more
vectors, per-CPU targeting, priority handling. → `14`, `24`

**ASCII** — a fixed encoding mapping bytes 0–127 to characters; UTF-8's
subset. → `02`

**ASLR (Address Space Layout Randomization)** — randomizing where code
and data load each run, so an attacker can't predict addresses in
advance. → `12`, `31`

**Atomicity** — a step that completes indivisibly from every other
observer's point of view; the fix for a race condition. → `23`

**BIOS (Basic Input/Output Software)** — legacy PC firmware; loads the
first disk sector matching a fixed magic number, with no filesystem
concept. → `06`

**bootloader** — code, often multi-stage, that loads and transfers
control to a kernel; a boot sector is only its first, tiniest stage.
→ `06`, `07`

**branch prediction** — a CPU's guess, made before a conditional jump's
outcome is known, about which way it will go, enabling speculative
execution. → `33`

**cache coherence** — hardware protocol (MESI and relatives) keeping
multiple CPU cores' cached copies of the same memory consistent.
→ `24`

**cache line** — the fixed-size unit (64 bytes on x86) a cache actually
moves and tracks; the unit behind false sharing. → `24`, `33`

**calling convention** — the agreed contract between caller and callee:
how arguments pass, which registers survive a call, where the return
value goes, who cleans the stack. → `03`, `05`, `10`

**cgroup** — a Linux mechanism limiting and accounting for a group of
processes' resource usage (CPU, memory, I/O); distinct from a
namespace. → `29`

**character device** — a device exposing a byte stream with no fixed
block size (keyboard, serial port), contrasted with a block device.
→ `16`, `17`

**CHS (Cylinder, Head, Sector)** — the original three-coordinate disk
addressing scheme; a compatibility fiction on modern drives, which
actually use LBA internally. → `17`

**CISC / RISC** — Complex vs. Reduced Instruction Set Computer: x86 is
classically CISC (variable-length, do-a-lot-per-instruction); ARM32 is
classically RISC (fixed-width, simpler decode). → `03`

**context switch** — saving one process's/thread's full CPU state and
loading another's; on this course's design, reduces to swapping one
stack pointer. → `19`, `20`, `22`

**control register (CR0–CR4)** — architectural x86 registers
configuring CPU mode itself (protection, paging, and more). → `08`,
`22`

**copy-on-write** — sharing pages read-only between processes (notably
after `fork()`) and copying only on the first write to each one.
→ `19`, `22`

**critical section** — code that must not be interleaved with other
code touching the same shared state. → `23`

**deadlock** — a cycle of threads each waiting on a resource another
holds, with none able to proceed. → `23`

**DMA (Direct Memory Access)** — a controller moving data to/from
memory independent of the CPU, interrupting only on completion.
→ `15`

**eBPF** — verified, sandboxed bytecode run inside the kernel at
chosen hook points, without a kernel module's unconstrained trust.
→ `35`

**ELF (Executable and Linkable Format)** — the standard Linux object/
executable format; carries sections (link-time view) and segments
(runtime-load view). → `11`, `12`

**EPT / NPT (Extended / Nested Page Tables)** — a second page-table
walk translating guest-physical to host-physical addresses under
hardware virtualization. → `28`

**exception** — the CPU signaling a problem with the currently
executing instruction (divide-by-zero, page fault); see also fault,
trap. → `14`

**fault** — an exception where the faulting instruction can be retried
after handling (the page fault is the canonical example). → `14`, `22`

**flags register** — holds condition results (zero, carry, and others)
set implicitly by instructions like `cmp`, read by the following
conditional jump. → `05`

**frame (page frame)** — a fixed-size unit of physical memory, the
counterpart to a virtual page. → `22`

**GDT (Global Descriptor Table)** — the protected-mode structure of
segment descriptors; mostly vestigial for protection on a modern
64-bit system, where paging does that work. → `08`

**heap** — a memory pool for allocations whose size/lifetime doesn't
fit the stack's automatic, nested pattern; needs an explicit allocator.
→ `10`

**hypervisor** — software managing virtual machines; Type 1 runs on
bare hardware, Type 2 runs as a host application, KVM is a hybrid.
→ `28`

**IDT (Interrupt Descriptor Table)** — protected mode's table of
interrupt/exception handlers, the GDT's cousin. → `14`

**inode** — a filesystem structure holding a file's metadata and block
locations, addressed by number, separate from any name. → `18`

**interrupt (hardware)** — a device signaling the CPU asynchronously.
→ `14`

**interrupt (software)** — a deliberate `int n` instruction requesting
a service (BIOS, or a syscall). → `03`, `21`

**IOMMU** — hardware restricting and translating DMA-capable devices'
memory access, the DMA analog of the CPU's own MMU. → `15`, `28`

**ISA (Instruction Set Architecture)** — the contract of instructions
and encodings a CPU family supports; "x86" names an ISA, a specific
chip is one implementation. → `03`

**ISR (Interrupt Service Routine)** — the handler an interrupt/
exception transfers control to; must save all state, since it can
interrupt anything, and exit via `iret`. → `14`

**KASLR** — ASLR applied to the kernel's own load address. → `31`

**kernel** — the privileged core managing hardware and enforcing
isolation; not the same as "the operating system." → `01`, `25`

**KPTI (Kernel Page-Table Isolation)** — Meltdown's mitigation; keeps
kernel mappings almost entirely out of user-mode page tables, at a
real performance cost. → `22`, `31`

**LBA (Logical Block Addressing)** — a flat sector-number addressing
scheme, what drives actually use internally today. → `17`

**linear address** — the result of segment translation alone, before
any paging; equals the physical address only when paging is off.
→ `09`, `22`

**linker** — the tool resolving an object file's relative references
into final absolute addresses and combining multiple files. → `11`,
`12`

**logical address** — what an instruction actually specifies:
segment:offset (real mode) or selector:offset (protected mode). → `09`

**magic number** — a fixed byte signature at a known offset letting
software self-identify data without external metadata (BIOS's `0x55,
0xAA`; ELF's `\x7fELF`). → `02`, `06`

**measured boot** — hashing each boot stage into a TPM's PCRs as
tamper-evident evidence, without blocking anything — contrasted with
Secure Boot's prevention. → `34`

**microkernel** — a design keeping the kernel itself minimal, running
drivers/filesystems as isolated user processes communicating by
message passing (Minix). → `36`

**MMU (Memory Management Unit)** — the hardware performing every
virtual-to-physical address translation automatically. → `22`

**monolithic kernel** — a design running drivers, filesystems, and
networking all in one kernel address space at ring 0 (Linux, and this
course's own toy OS). → `25`, `36`

**MSR (Model-Specific Register)** — a larger, less architecturally
fixed register category than CR0–CR4, accessed via `rdmsr`/`wrmsr`
(`EFER` is the key example). → `08`, `21`

**namespace** — a Linux mechanism giving a process a private view of a
global kernel resource (PID, mount, network, and others); distinct
from a cgroup. → `29`

**NX bit** — a page table entry bit marking a page non-executable,
defeating naive inject-and-run shellcode attacks. → `22`, `31`

**object file** — compiler/assembler output: relocatable machine code
with unresolved, relative references, not yet runnable. → `11`

**opcode** — the portion of an instruction's encoding naming which
operation to perform. → `03`

**page** — a fixed-size unit of virtual memory (4 KB, typically), the
counterpart to a physical frame. → `22`

**page fault** — the MMU's signal (exception 14) that a translation
couldn't complete; usually routine, not an error. → `22`

**page table** — the multi-level tree structure recording virtual-to-
physical mappings, walked by the MMU. → `22`

**PCB (Process Control Block)** — everything needed to pause and later
correctly resume a process; in this course's design, essentially just
a saved stack pointer. → `19`, `20`

**PIC (8259 Programmable Interrupt Controller)** — legacy hardware
routing IRQs to interrupt vectors; must be remapped off its colliding
defaults. → `14`

**PIE (Position-Independent Executable)** — a binary compiled to run
correctly at any load address, the prerequisite for ASLR. → `12`, `31`

**pipelining** — overlapping multiple instructions across CPU stages
(fetch/decode/execute/memory/write-back) simultaneously. → `33`

**privilege ring** — x86's 0–3 privilege levels; ring 0 is most
privileged (kernel), ring 3 least (user applications). → `08`, `21`

**process** — a running instance of a program: its own address space,
file handles, and resumable state; distinct from both "program" and
"thread." → `19`

**protected mode** — 32-bit x86 mode (80386-era) using the GDT for
flat, privilege-aware addressing. → `08`

**race condition** — a bug where correctness depends on timing between
concurrent flows of execution. → `23`

**real mode** — the original 8086 16-bit mode; every x86 CPU, including
modern 64-bit ones, still powers on here. → `04`, `08`

**register renaming** — a CPU mapping architectural register names
onto a larger pool of physical registers, avoiding false dependencies
under out-of-order execution. → `03`, `33`

**ring (see privilege ring)**

**ROP (Return-Oriented Programming)** — chaining existing, legitimately
executable code via controlled return addresses, bypassing non-
executable-memory defenses. → `05`, `31`

**scheduler** — the kernel component deciding which Ready process/
thread runs next. → `20`

**Secure Boot** — a chain of cryptographic signatures (PK → KEK → db/
dbx) verifying every boot-stage binary before allowing it to run.
→ `34`

**segment descriptor** — an 8-byte GDT (or IDT gate) entry describing a
memory region's base, limit, and access rights. → `08`, `14`

**SIMD** — Single Instruction, Multiple Data; one instruction operating
on several values at once (SSE/AVX). → `04`, `33`

**SMEP / SMAP** — CPU features faulting if the kernel executes code
from, or accesses, user-mode memory even by mistake. → `08`, `31`

**SMP (Symmetric Multiprocessing)** — multiple CPU cores running as
equal peers under one OS over one shared memory. → `24`

**spinlock** — a lock acquired by busy-waiting (staying Running) rather
than sleeping; appropriate for short critical sections. → `23`

**stack canary** — a known value planted between locals and the saved
return address, checked before `ret`, catching overflow before misuse.
→ `10`, `31`

**stack frame** — one function call's region of the stack: arguments,
return address, saved base pointer, locals. → `05`, `10`

**syscall / system call** — a deliberate, controlled transfer from
ring 3 to ring 0, requesting a kernel service. → `21`

**thread** — the actual unit a scheduler runs; a process can own
several, sharing its address space. → `19`

**TLB (Translation Lookaside Buffer)** — a cache of recent virtual-to-
physical translations, avoiding a full page-table walk on most
accesses. → `22`, `24`

**TPM (Trusted Platform Module)** — hardware holding tamper-evident
measurement registers (PCRs) used for measured boot and disk-
encryption key binding. → `34`

**trap** — an exception reported after the triggering instruction
completes (contrasted with a fault). → `14`

**TSS (Task State Segment)** — a GDT-described structure telling the
CPU which trusted kernel stack to switch to during a ring 3→0
transition. → `08`, `21`

**unikernel** — an application linked directly against a minimal
kernel library into one binary, with no internal process isolation at
all. → `01`, `36`

**VFS (Virtual File System)** — Linux's common filesystem interface,
letting ext4, FAT, and others share one `open`/`read`/`write` syscall
surface. → `18`

**virtual address** — the address a program actually uses; translated
by the MMU to a physical one. → `09`, `22`

**VMCS (Virtual Machine Control Structure)** — the descriptor-like
structure controlling which operations cause a VM exit under hardware
virtualization. → `28`

**VM exit / VM entry** — the hardware-enforced transition from a
guest's non-root execution to the hypervisor's root mode, and back.
→ `28`

**word** — in Intel's formal usage, always 16 bits regardless of CPU
mode; the book uses it more loosely as "the current mode's native
size." → `02`

---

*Every definition above points to a specific course file. For the
underlying book-vs-constructed status of each concept, cross-reference
against `book-to-course-map.md`.*
