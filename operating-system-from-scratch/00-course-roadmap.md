# 00 — Course Roadmap

## How to read this map

Every file number below carries a **Book Coverage** rating. This is
the Layer-1 honesty check described in `README.md`, made concrete:

| Rating | Meaning |
|---|---|
| **Full** | Blundell's draft covers this topic completely and correctly |
| **Seed** | The book introduces the idea in passing but doesn't develop it |
| **Stub** | The book's TOC names this topic; the draft body is empty |
| **None** | The book never mentions this at all — pure course construction |

`Full` and `Seed` files open with real Layer-1 content. `Stub` and
`None` files open by saying so, then move straight into Layers 2–3.

---

## Master dependency graph

```
Binary/Hex representation
        │
        v
   CPU & ISA basics ──────────────┐
        │                         │
        v                         │
   x86 architecture               │
        │                         │
        v                         │
 Assembly language ◄──────────────┘
        │
        v
   Boot process (BIOS, MBR, magic number)
        │
        v
 16-bit Real Mode ──► Stack / calling basics
        │
        v
 Protected Mode + GDT
        │
        v
 C compilation & machine code ──► Linking, loading, ELF
        │
        v
 Kernel entry & minimal kernel architecture
        │
        ├──────────────┬───────────────────┐
        │              │                   │
        v              v                   v
 Device I/O & DMA   Interrupts/IDT   Memory & addressing
        │              │                   │
        │              │                   │
        v              v                   v
 Device drivers   Exceptions/traps   Virtual memory (paging)
        │              │                   │
        │              │                   │
        └──────┬───────┴───────────────────┘
               │
               v
     Storage & block I/O ──► Filesystems
               │
               │
               v
     Processes & threads ──► Scheduling & context switching
               │
               │
               v
     System calls / user-kernel boundary
               │
               │
               ├──────────────┬───────────────┐
               │              │               │
               v              v               v
        Concurrency      Networking      Security/protection
               │              │               │
               │              │               │
               └──────┬───────┴───────────────┘
                      │
                      v
             SMP / multiprocessor systems
                      │
                      │
                      v
             Linux kernel architecture (bridge)
                      │
                      │
        ┌─────────────┼──────────────────┐
        │             │                  │
        v             v                  v
 Virtualization   Containers      eBPF / observability / perf
        │             │                  │
        │             │                  │
        └─────────────┴──────────────────┘
                      │
                      v
               Modern hardware & boot (UEFI / ARM64 / NVMe)
                      │
                      │
                      v
               Capstone: staged custom OS
```

---

## The big progression, annotated

This is the shape of the whole course, end to end. Compare it against
"just finish an OS tutorial" — this is what makes the difference.

```
C / C++                              → files 01, 04, 11
   ↓
Machine representation               → 02
   ↓
CPU + ISA                            → 03, 04, 33
   ↓
Assembly                             → 05
   ↓
Machine code                         → 05, 11
   ↓
Compiler                             → 11
   ↓
Assembler                            → 05, 11
   ↓
Linker                               → 12
   ↓
Executable / ELF                     → 12
   ↓
Bootloader                           → 06, 07, 34
   ↓
Kernel entry                         → 13
   ↓
CPU privilege                        → 08, 09, 31
   ↓
Memory management                    → 09, 22
   ↓
Interrupts                           → 14
   ↓
Drivers                              → 15, 16, 17
   ↓
Processes                            → 19
   ↓
Scheduling                           → 20
   ↓
System calls                         → 21
   ↓
Virtual memory                       → 22
   ↓
Filesystems                          → 18
   ↓
Networking                           → 30
   ↓
Concurrency / SMP                    → 23, 24
   ↓
Linux kernel                         → 25, 26, 27
   ↓
Virtualization                       → 28
   ↓
Containers                           → 29
   ↓
eBPF / observability                 → 35, 32
   ↓
Performance + debugging              → 27, 32, debugging-playbook.md
```

---

## Course parts (essential / adjacent / advanced, per group)

Files are grouped into nine parts. Each part lists its difficulty
range, what's *Essential* to grasp before moving on, what's
*Important Adjacent Knowledge* worth having, and what's *Advanced /
Optional* for later or for specialization.

### Part I — Machine Foundations (01–05)
Book coverage: Seed → Full. Difficulty: ★1–★2.
- **Essential:** binary/hex representation, general-purpose registers, instruction/assembler/machine-code relationship, why hex represents binary.
- **Adjacent:** ISA vs microarchitecture, register-width history (16→32→64-bit).
- **Advanced:** microcode, pipelines (deferred to Part VIII).

### Part II — Booting the Machine (06–08)
Book coverage: Full (historical) + None (long mode). Difficulty: ★2–★3.
- **Essential:** boot sector mechanics, `0xAA55` signature, real→protected mode switch, GDT structure.
- **Adjacent:** MBR partition table, CPU reset state.
- **Advanced:** UEFI, GPT, Secure Boot, measured boot, long mode paging prerequisites.

### Part III — Memory & the Toolchain (09–13)
Book coverage: Full → Seed. Difficulty: ★3.
- **Essential:** segment:offset math, GDT-based flat addressing, stack frames, C→ASM→object→link pipeline, symbol resolution.
- **Adjacent:** calling conventions/ABI, ELF section/segment anatomy.
- **Advanced:** linker scripts, LTO, PIE/ASLR relocations.

### Part IV — Interrupts, I/O & Drivers (14–17)
Book coverage: Seed/Stub. Difficulty: ★3–★4.
- **Essential:** IDT structure, PIC remapping, port I/O vs MMIO, ISR mechanics.
- **Adjacent:** DMA transfer pattern, scatter-gather, IRQ sharing.
- **Advanced:** APIC/IOAPIC, MSI-X, IOMMU, NVMe queueing.

### Part V — Filesystems & Processes (18–20)
Book coverage: Stub (entirely constructed). Difficulty: ★4.
- **Essential:** FAT12 layout, process abstraction, context-switch mechanics, round-robin scheduling.
- **Adjacent:** VFS abstraction, process states, preemption vs cooperation.
- **Advanced:** journaling filesystems, CFS/EEVDF-style scheduling.

### Part VI — The User/Kernel Contract (21–24)
Book coverage: None. Difficulty: ★4–★5.
- **Essential:** ring transitions, syscall gate mechanics, page tables, TLB, race conditions.
- **Adjacent:** copy-on-write, demand paging, lock-free basics.
- **Advanced:** NUMA, memory ordering, lock elision.

### Part VII — Linux as the Real System (25–27)
Book coverage: None (bridge). Difficulty: ★3–★4.
- **Essential:** `task_struct`, Linux IRQ subsystem, VFS, GDB/QEMU kernel debugging.
- **Adjacent:** kernel source-tree navigation, `/proc` and `/sys`.
- **Advanced:** kernel module development, live patching.

### Part VIII — Modern Systems (28–35)
Book coverage: None (except QEMU/Bochs as reused tools). Difficulty: ★3–★4.
- **Essential:** VT-x/KVM basics, namespaces/cgroups, TCP/IP path through the kernel, `perf`/`bpftrace` basics.
- **Adjacent:** overlayfs, container runtimes, NUMA-aware performance tuning.
- **Advanced:** eBPF verifier concepts, microVMs (Firecracker), speculative-execution security.

### Part IX — Synthesis (36–38)
Book coverage: Full (37, as a checkpoint) → constructed (38). Difficulty: ★2 (review) → ★5 (capstone).
- **Essential:** monolithic vs microkernel tradeoffs, the full staged build.
- **Adjacent:** design comparisons (Minix, seL4).
- **Advanced:** everything in the capstone's later stages (ring 3, syscalls, user-mode loader).

---

## Per-file table: coverage, prerequisites, difficulty, project

| # | File | Book Coverage | Prereqs | Difficulty | Core Practical Project |
|---|---|---|---|---|---|
| 01 | computer-systems-foundation | Seed (Ch.1) | — | ★1 | Map "layers of abstraction" onto your own laptop |
| 02 | binary-hex-memory-representation | Full (§2.2, §2.4) | 01 | ★1 | Hand-convert the boot sector's own bytes |
| 03 | cpu-and-instruction-set | Seed (§3.3.2) | 02 | ★2 | Identify registers in a raw disassembly |
| 04 | x86-architecture | Seed (implicit) | 03 | ★2 | Compare 16/32/64-bit views of the same register |
| 05 | assembly-language | Full (§3.3–3.4) | 04 | ★2 | Hand-assemble the book's "Hello" boot sector |
| 06 | boot-process | Full (§2.1–2.2) | 05 | ★2 | Boot a 16-byte infinite loop in QEMU |
| 07 | bios-uefi-bootloaders | Seed/None | 06 | ★3 | Check whether your own machine boots BIOS or UEFI |
| 08 | real-protected-long-mode | Full/None | 06,07 | ★3 | Step through the mode switch in Bochs's debugger |
| 09 | memory-and-addressing | Full (§3.6.1,§4.2) | 08 | ★3 | Compute segment:offset by hand, then a GDT-flat address |
| 10 | stack-heap-calling-conventions | Full (§5.1.2–5.1.4) | 09 | ★3 | Watch `ebp`/`esp` move across a call in GDB |
| 11 | c-compilation-and-machine-code | Full (§5.1) | 10 | ★3 | Reproduce the book's C → `objdump` exercise |
| 12 | linking-loading-and-elf | Full (§5.2.1–5.2.3) | 11 | ★3 | `readelf -S` a real binary vs the book's raw-binary link |
| 13 | kernel-entry-and-architecture | Full (§5.2–5.3) | 12 | ★3 | Build the `kernel_entry.asm` + Makefile chain |
| 14 | interrupts-exceptions-traps | Seed/Stub (§3.3.1/§6.3) | 13 | ★4 | Build a real IDT and remap the 8259 PIC |
| 15 | device-io-and-dma | Full/Seed (§6.1) | 13 | ★3 | Extend `port_byte_in/out`; sketch a DMA transfer |
| 16 | device-drivers | Full/Stub (§6.2/§6.4-5) | 14,15 | ★4 | Write a keyboard driver on IRQ1 |
| 17 | storage-and-block-io | Full/Stub (§3.6.2-3/§6.5) | 16 | ★4 | Read sectors two ways: BIOS `int 0x13` vs your own ATA PIO |
| 18 | filesystems | Stub (§6.6) | 17 | ★4 | Implement a minimal FAT12 reader |
| 19 | processes-and-threads | Stub (§7.1) | 18 | ★4 | Manual two-task context switch |
| 20 | scheduling-and-context-switching | Stub (§7.2) | 19 | ★4 | Round-robin scheduler driven by a PIT tick |
| 21 | system-calls-and-user-kernel-boundary | None | 20 | ★5 | Ring-3 jump + an `int 0x80`-style gate |
| 22 | virtual-memory | None | 21 | ★5 | Enable paging; identity-map, then remap |
| 23 | concurrency-and-synchronization | None | 20 | ★4 | Reproduce a race condition, then fix it with a spinlock |
| 24 | multiprocessor-and-smp-systems | None | 23 | ★4 | Inspect `/proc/cpuinfo`; pin a process with `taskset` |
| 25 | linux-kernel-architecture | None (bridge) | 14–24 | ★4 | Map every toy-OS concept to its Linux counterpart |
| 26 | linux-source-code-navigation | None (bridge) | 25 | ★3 | Find the real IDT setup in the Linux source tree |
| 27 | kernel-debugging | Seed (§3.5 spirit) | 25 | ★4 | `qemu -s -S` + GDB attached to your own kernel |
| 28 | virtualization | Seed (Bochs/QEMU reused) | 25 | ★4 | Boot your OS under KVM-accelerated QEMU vs TCG |
| 29 | containers-and-namespaces | None | 25 | ★4 | Build a minimal container with `unshare` |
| 30 | networking-from-hardware-to-socket | None | 25 | ★4 | Trace a packet with `tcpdump` + NAPI concepts |
| 31 | security-and-protection | Seed (ring 0, null descriptor) | 21 | ★4 | Trigger, then fix, a toy null-descriptor exploit |
| 32 | performance-and-observability | None | 26 | ★4 | `perf stat` on a real workload |
| 33 | modern-hardware | None | 04 | ★3 | Read your own CPU's cache/topology via `lscpu` |
| 34 | modern-boot-and-efi | None | 07 | ★3 | Inspect your own EFI boot entries |
| 35 | ebpf-and-kernel-observability | None | 26,32 | ★4 | Trace syscalls with `bpftrace` |
| 36 | operating-system-design | Seed (implicit) | 25 | ★3 | Compare monolithic vs microkernel vs your toy OS |
| 37 | building-a-simple-os | Full (Ch.1–6.2 checkpoint) | 06–17 | ★2 | Build the whole book-grounded OS in one sitting |
| 38 | capstone-project | Full → Stub/None | all | ★5 | The 15-stage OS (see below) |

---

## Capstone preview (full detail lands in `38-capstone-project.md`)

| Stage | Content | Book Coverage |
|---|---|---|
| 1 | Boot sector, magic number | Full |
| 2 | Print to screen (BIOS teletype) | Full |
| 3 | Assembly routines: stack, functions, disk read | Full |
| 4 | GDT + enter protected mode | Full |
| 5 | Load a linked C kernel | Full |
| 6 | Port I/O + scrolling screen driver | Full |
| 7 | IDT + PIC remap + real ISR handling | **Stub** |
| 8 | Keyboard driver | **Stub** |
| 9 | PIT timer | **None** |
| 10 | Minimal paging + `kmalloc` | **None** |
| 11 | Round-robin scheduler / context switch | **Stub** |
| 12 | Minimal FAT12 read | **Stub** |
| 13 | Ring-3 syscall gate | **None** |
| 14 | User-mode program loader + toy shell | **None** |
| 15 | Debugging/observability pass | Seed |

Stages 1–6 are stages you can check, line by line, against Blundell's
PDF. Stage 7 onward is where the book's table of contents stops
making promises it keeps — everything from there is built using
standard OS-development references, clearly separate from "the book."

---

## What's next

`book-to-course-map.md` (a section-by-section index into the PDF) or
file `01-computer-systems-foundation.md` — say **NEXT** to continue
sequentially, or name a specific file to jump to it instead.
