# Final Knowledge Map

## The Final Audit

Checked against the full course, honestly:

- [x] Every major book concept covered — `book-to-course-map.md`: 38
      Full, 7 Seed sections, all cross-referenced (the 7 Stub sections
      are covered as explicitly-labeled constructed material)
- [x] Important adjacent concepts covered — every file's [RELATED
      KNOWLEDGE] section
- [x] Modern context added — every file's [MODERN SYSTEM] section
- [x] Historical concepts clearly labeled — the layer-map box at the
      top of every file, maintained without exception across 38 files
- [x] Concepts connected to one another — the recurring-pattern
      callbacks (below), plus the explicit breadcrumb-collection files
      (`21`, `25`, `31`, `33`)
- [x] Practical experiments included — `experiments.md`, indexed by
      difficulty across all 38 files
- [x] Debugging included — `debugging-playbook.md`, nine symptom-first
      chains
- [x] Linux connections included — file `25`'s Rosetta table, and
      continuous reference afterward
- [x] Source-code navigation included — file `26`, `source-code-map.md`
- [x] Interview questions included — every file, plus the curated
      `interview-questions.md`
- [x] Active recall included — every file, no exceptions
- [x] Glossary complete — `glossary.md`, ~90 terms
- [x] Knowledge graph complete — `00-course-roadmap.md`'s dependency
      graph, and this file's final map
- [x] Capstone complete — file `38`, nine staged projects
- [x] No major conceptual gaps — the course covers CPU/ISA through
      containers, networking, and eBPF, each at the depth its tier
      (Essential/Adjacent/Advanced) warrants
- [x] No outdated concept presented as current fact — every file
      distinguishes `[BOOK]`/`[BOOK — STUB]` from constructed and
      modern content; file `04` corrected its own under-attribution
      mid-course when new evidence (§3.2) was found
- [x] No unnecessary encyclopedia-style expansion — tiering held
      throughout; files stayed scoped to what their title promised

---

## The Complete Mental Model

![Final knowledge map: a vertical spine from hardware through CPU/ISA, firmware, bootloader, kernel entry, interrupts and I/O, drivers, memory management, processes and scheduler, system calls, filesystem, networking, concurrency and security, virtualization and containers, up to user space and applications, each stage tagged with its course file numbers; alongside it, a panel of seven patterns that recur at multiple layers throughout the course](images/final-knowledge-map.svg)

Read top to bottom for a request moving down through the system; read
bottom to top for a hardware event moving up to an application — file
`01`'s very first diagram, now with every rung built.

## The Patterns, Spelled Out

These aren't coincidences — each one is the same underlying idea,
solving a structurally similar problem at a different layer:

- **Descriptor table, bit-packed.** The GDT (`08`) taught you to read
  a byte-packed table of permission/location records. The IDT (`14`),
  TSS (`21`), VMCS (`28`), and page table entry (`22`) are all the same
  idea — a fixed-size, densely-packed record describing something the
  CPU consults on every relevant access.
- **Load registers, transfer control.** BIOS's `mov ah,0x0e`/`int 0x10`
  (`03`) is the same shape as your own `call`/`ret` convention (`05`),
  a Linux `syscall` (`21`), and a VM exit (`28`): agree on what
  registers mean, then hand control somewhere more capable.
- **Index/data port pair.** The VGA cursor's two-port addressing (`16`)
  is the same trick real disk controllers use (`17`) — more internal
  registers than available port addresses, solved by addressing them
  indirectly.
- **A function pointer decides who runs.** `file_operations` (`18`,
  `26`) is why `vfs_read()` doesn't know or care which filesystem it's
  calling into; the same indirection is exactly how a socket (`30`)
  reuses the same `read()`/`write()` syscalls as an ordinary file.
- **A `.h`/`.c` split enables swapping the implementation.** The screen
  driver (`16`) hides VGA specifics behind a stable interface; the VFS
  (`18`) does the identical thing at filesystem scale; Linux's driver
  model (`25`) is this idea formalized across every device category.
- **Reserve slot 0 to fail loudly.** The GDT's null descriptor (`08`)
  turns a forgotten segment register into an immediate, loud exception
  instead of silent corruption; an unmapped page at virtual address 0
  (`22`) does the identical thing for a null pointer in C.
- **Prevent, then detect.** Secure Boot prevents unsigned code from
  running at all; measured boot detects what ran regardless (`34`) —
  the same two-part defense shape as NX (prevent injected code from
  running) paired with stack canaries (detect corruption before it's
  used) in `10`/`31`.

---

## What You Actually Know Now

Not "finished a book." You can trace, by hand, with a reason at every
step: what happens from the instant power is applied to a visible
character on screen (`01`→`08`→`16`); from a C function call down to
`call`/`ret` and the stack (`05`→`10`); from a page fault to a resumed
instruction (`14`→`22`); from one process to another at a timer tick
(`14`→`19`→`20`); from a user program's `read()` to a NIC's DMA ring
and back (`21`→`30`); from a container's isolated view down to the one
kernel underneath it (`25`→`29`); from a guest's `cli` to a VM exit and
back (`28`); from a hunch about "it's slow" to a flame graph that
proves which function it actually is (`32`).

And, just as importantly: you know exactly where a 2010 teaching
draft's own promises ran out, and you built everything past that point
yourself, clearly labeled, never mistaking your own construction for
Blundell's.

---

## Where to Go From Here

- **Extend the capstone** (file `38`) — stages 13–14 (ring 3, a user
  loader, a toy shell) are the biggest remaining undertaking, and the
  most rewarding.
- **Read real kernel code** with file `26`'s skill, starting from file
  `25`'s Rosetta table — you now know enough to recognize what you're
  looking at.
- **Revisit `00-course-roadmap.md`** if you want the dependency graph
  and difficulty ratings fresh, now that every node in it is something
  you've actually built or studied, not just planned.

That's the whole course.
