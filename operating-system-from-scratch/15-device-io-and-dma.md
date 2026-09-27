# 15 — Device I/O and DMA

## Layer map for this file

```
┌────────────────────────────────────────────────┐
│ 1. BOOK: memory-mapped vs. port I/O, a real    │
│    floppy-controller register example, the     │
│    book's own port_byte_in/out wrappers,       │
│    and DMA as a one-paragraph analogy (§6.1)   │
├────────────────────────────────────────────────┤
│ 2. SUPPORTING: the I/O bus hierarchy, ISA vs.  │
│    PCI address assignment                      │
├────────────────────────────────────────────────┤
│ 3. MODERN: real DMA controller mechanics,      │
│    scatter-gather, the IOMMU                   │
└────────────────────────────────────────────────┘
```

Book coverage: **Full** for port I/O, **Seed** for DMA (one paragraph,
an analogy, no code at all).

## Prerequisites

Files `01`–`14`.

---

## [BOOK] Two Ways a CPU Talks to Hardware

You already used **memory-mapped I/O** without the name for it — file
`08`'s direct writes to `0xb8000` *are* memory-mapped I/O: an address
range that looks like ordinary memory but actually reaches a device's
internal buffer. **Port I/O** is the other mechanism, for controllers
whose registers live in a completely separate address space from main
memory, reached only by dedicated `in`/`out` instructions.

The book grounds this in a real datasheet, not an abstraction: Intel's
82077AA floppy controller has a physical pin (pin 57, `ME0`) that
spins the drive motor when high. That pin is wired to bit 4 of the
controller's **Digital Output Register (DOR)** — and the DOR itself is
reached at **I/O port `0x3F2`**:

![Port I/O chain: al register, through port 0x3f2, to the DOR register's motor bit, to physical pin 57 on the floppy controller, with the four-instruction sequence that reads, sets, and writes it back; below, a DMA contrast panel showing the CPU handling every byte itself versus delegating to a DMA controller and getting one interrupt when done](images/15-port-io-and-dma.svg)

```asm
mov dx, 0x3f2      ; port address must go through dx
in  al, dx         ; read the DOR's current value
or  al, 00001000b  ; set the motor bit
out dx, al         ; write it back
```

**A real bit, on a real chip, toggled by four instructions** — this is
what "device driver" concretely means at the bottom.

## [BOOK] Port I/O Needs Inline Assembly

`in`/`out` have no C equivalent — the book's fix is GCC inline
assembly, collected into `kernel/low_level.c` for every future driver
to share:

```c
unsigned char port_byte_in(unsigned short port) {
    unsigned char result;
    __asm__("in %%dx, %%al" : "=a" (result) : "d" (port));
    return result;
}
void port_byte_out(unsigned short port, unsigned char data) {
    __asm__("out %%al, %%dx" : : "a" (data), "d" (port));
}
```

That inner string is **GAS syntax**, not NASM's — file `03`'s
Intel-vs-AT&T warning, showing up for real: operands reversed, and
`%%` because a single `%` is C's own escape character, needing to
escape the escape so GCC emits a literal `%` into the assembly.

## [BOOK] DMA: One Paragraph, No Code (§6.1.3)

The book's entire treatment: moving large amounts of data one byte at
a time via port I/O wastes CPU time, so a **DMA controller** exists to
take that job over — an architect (the CPU) telling a builder (the DMA
controller) to move the bricks, and only wanting to be interrupted
when the wall's done or something's gone wrong. That's the complete
DMA content in the draft — no registers, no code, no configuration
sequence. Everything past this point is genuine construction.

---

## [CORE CONCEPT] The I/O Bus Hierarchy

The CPU doesn't talk to every device directly — that would force it to
slow down to the speed of the slowest device on the system. Instead it
issues I/O instructions to a fast, top-level bus controller, which
relays them onward; slower bus technologies attach *as devices* on
that faster bus, forming the layered hierarchy every real motherboard
has.

## [CORE CONCEPT] Static vs. Dynamic Port Assignment

Older buses (ISA) assigned port addresses to devices statically and
permanently. Modern plug-and-play buses (PCI) let BIOS allocate I/O
and memory-mapped address ranges to each device dynamically at boot,
based on configuration data the device itself reports over the bus —
including a unique hardware ID an OS uses later to find the right
driver.

---

## [MODERN SYSTEM] What the Book's DMA Paragraph Skips Entirely

A real DMA transfer needs the CPU to program the controller with a
source address, destination address, and length — then the controller
generates its own bus cycles to move the data, completely independent
of the CPU, raising an interrupt (file `14`) on completion. **Scatter-
gather DMA** extends this to a whole list of buffers in one setup,
instead of one contiguous block — essential for real-world I/O where
data isn't always contiguous in memory.

## [MODERN SYSTEM] The IOMMU: DMA's Trust Problem

A device performing DMA writes directly to physical memory, with no
inherent concept of "which process owns this," which a naive
implementation would let a malicious or buggy device corrupt or read
memory it has no business touching. The **IOMMU** sits between DMA-
capable devices and physical memory, translating and restricting their
accesses the way the CPU's own MMU restricts a process's memory access
(file `22`) — a real security boundary the book's world never needs.

---

## Practical Experiment

1. Type the book's `port_byte_in`/`port_byte_out` wrappers into
   `kernel/low_level.c` and use them for something observable in your
   own kernel from file `13`.
2. Compile with `-S` and compare the generated assembly's inline
   `in`/`out` against the GAS snippet shown above — confirm the
   operand order matches AT&T syntax, not NASM's.
3. Look up any device's datasheet you have easy access to (or search
   for the 82077AA's) and find one more register beyond the DOR — note
   its address and what setting a specific bit does.

## Debugging

A driver that "does nothing and reports no error" is often a port
I/O bug hiding as a hardware bug: writing to the wrong port address,
or the right port with the wrong bit, produces no crash and no
message — the device just silently doesn't respond. The fix is always
to go back to the datasheet, confirm the exact register and bit, and
verify with a known-working read before assuming the hardware itself
is faulty.

## Source Code

`kernel/low_level.c` with all four port I/O wrappers — this file
becomes a dependency of every driver in file `16` onward.

---

## Common Misconceptions

- **"Memory-mapped I/O and port I/O are the same mechanism."** Two
  genuinely different address spaces on x86 — one reached by ordinary
  memory instructions, one only by `in`/`out`.
- **"You can write `in`/`out` directly in C."** No equivalent exists —
  inline assembly is mandatory, which is why `low_level.c` exists at all.
- **"DMA means the CPU is no longer involved at all."** It still sets
  up the transfer and handles the completion interrupt — it's relieved
  of moving the bytes themselves, not removed from the process entirely.

## Interview Questions

**Basic:** Memory-mapped I/O vs. port I/O — what's the structural difference?
**Intermediate:** Why does `port_byte_in`'s inline assembly need `%%`
instead of `%`?
**Advanced:** Why can't the CPU talk to every device on a fast,
top-level bus directly?
**Systems Design:** What problem does an IOMMU solve that a DMA
controller alone doesn't?
**Debugging:** A driver writes to a port and nothing happens, with no
error — where do you look first?
**Explain what happens when...:** a driver spins up a floppy motor,
from the C call down to the physical pin?

## Active Recall

1. What are the two mechanisms a CPU uses to talk to hardware?
2. What does the book's DMA paragraph actually cover, and skip?
3. Why does port I/O require inline assembly in C?
4. What does an IOMMU add that plain DMA lacks?

## Answers

1. Memory-mapped I/O (device buffer reachable via ordinary memory
   addresses) and port I/O (a separate address space, reached only by
   `in`/`out`).
2. Covers: the CPU-delegates-to-a-controller idea, via analogy. Skips
   entirely: registers, setup sequence, completion handling, any code.
3. `in`/`out` have no C-language equivalent at all — GCC's inline
   assembly is the only way to emit them from C.
4. Address translation and access restriction for DMA-capable devices
   — without it, a device can read or write any physical memory with
   no isolation at all.

---

## What You Should Now Understand

- Port I/O grounded in a real chip's real register, not an abstraction.
- Why the book needed inline assembly, and how its GAS syntax connects
  back to file `03`'s syntax warning.
- Exactly how thin the book's DMA coverage is, and what real DMA
  actually requires.

---

**Next:** file `16-device-drivers.md`.
