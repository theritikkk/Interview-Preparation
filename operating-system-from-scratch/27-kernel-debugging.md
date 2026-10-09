# 27 — Kernel Debugging

## Layer map for this file

```
┌──────────────────────────────────────────────┐
│ 1. BOOK: §3.5 — no debugger exists, so the    │
│    book designs print_hex, and leaves the     │
│    actual implementation as Question 5         │
├──────────────────────────────────────────────┤
│ 2. CORE CONCEPT: Bochs/QEMU's own debuggers,   │
│    used properly instead of just mentioned      │
├──────────────────────────────────────────────┤
│ 3. MODERN: GDB over qemu -s -S, printk/dmesg,  │
│    ftrace, kdump                                │
└──────────────────────────────────────────────┘
```

Book coverage: **Seed**. §3.5's honest premise: there's no GUI
debugger for this code, and the best feedback a mistake gives you "is
visibly to do nothing at all" — so the book builds its own tool.

## Prerequisites

Files `01`–`26`. This file consolidates debugger use that earlier
files (`08`, `14`) already asked you to do informally.

---

## [BOOK] print_hex, Designed But Not Finished

The book wants `print_hex(0x1fb6)` to print `0x1fb6` to the screen,
using `dx` as the parameter (file `03`'s register-passing pattern,
again). Its trick: keep a literal template string in memory —

```asm
HEX_OUT: db '0x0000', 0
```

— and have `print_hex` overwrite the four digit characters in place to
reflect `dx`, then call the already-written `print_string` (file `05`)
on the result. What's genuinely notable: **the book designs the
approach and stops.** Actually converting each nibble of `dx` into the
correct ASCII hex digit — using `and` to isolate 4 bits and `shr` to
shift the next nibble into place — is left as **Question 5, marked
Advanced.** This is the book's hardest exercise, and its own
"debugger" is something you have to finish building yourself.

---

## [CORE CONCEPT] The Debugger You Already Half-Used

Files `08` and `14` both said "use Bochs's debugger" without teaching
it properly. Here's the actual workflow:

```
b 0x7c50          set a breakpoint at an address
c                 continue running until it's hit
s                 step one instruction
info registers    dump general registers
sreg              dump segment registers (file 09's world)
x /4xb 0x7c00     examine 4 bytes at an address, in hex
```

This is print_hex's entire purpose, done properly by a tool instead of
by hand: **inspect state at a chosen point**, without editing your code
to print anything. Every "watch `cr0`'s PE bit flip" and "confirm `cs`
now holds `0x8`" instruction in files `08` and `14` was this workflow,
used without being named.

![The kernel debugging ladder: print_hex at the bottom, then Bochs/QEMU's own monitor, then GDB attached via qemu -s -S, then printk/dmesg, then ftrace/kprobes, then kdump and crash for post-mortem analysis \u2014 all answering the same question: what is the state right now](images/27-debugging-ladder.svg)

---

## [MODERN SYSTEM] GDB, Attached to Your Own Kernel

`qemu -s -S ...` starts a GDB stub on port 1234 and freezes the CPU
before the first instruction (`-S`). Attach with:

```
gdb
(gdb) target remote localhost:1234
(gdb) break *0x7c00
(gdb) continue
```

This is real source-level debugging of the exact kernel you've been
building since file `13` — breakpoints by symbol name once you load
debug info, watchpoints on memory, and a proper backtrace (file `10`'s
saved-`ebp` chain, walked automatically instead of by hand).

## [MODERN SYSTEM] printk(): print_hex, Industrialized

Here's the validating fact this file exists to deliver: **`printk()`
plus `dmesg` is still, in 2026, the single most common way real Linux
kernel developers debug real problems.** The book's instinct — when in
doubt, print something and look at it — didn't get replaced by
sophistication. It got a ring buffer, log levels, and timestamps, and
stayed exactly that useful. `dmesg -w` streams it live; `pr_debug()`
adds prints that compile out entirely unless explicitly enabled.

## [MODERN SYSTEM] Watching a Live Kernel Without Recompiling

**ftrace** and **kprobes** attach tracing to a running kernel's
functions dynamically — no rebuild, no reboot. `trace-cmd` and `perf
probe` are common front ends. This is print_hex's idea taken to its
logical extreme: insert observation points anywhere, on demand, without
ever touching source.

## [MODERN SYSTEM] When the Kernel Has Already Crashed

Live debugging assumes the system is still running. A real kernel
panic often isn't reproducible on demand — **kdump** configures a
second, minimal kernel to boot automatically after a crash and capture
the first kernel's memory to disk; the **`crash`** tool then lets you
inspect that dump afterward exactly as if it were live: walk
`task_struct`s (file `19`), read stack traces, examine memory — all
after the fact, on a machine that's already rebooted.

---

## [ADVANCED / BEYOND THE BOOK] Catching Bugs Automatically

**KASAN** (Kernel Address Sanitizer) instruments memory accesses to
catch out-of-bounds and use-after-free bugs (file `22`) the instant
they happen, not whenever they happen to crash something visibly.
**Lockdep** (file `23`) does the equivalent for locking — watching lock
acquisition order at runtime and reporting a potential deadlock cycle
before it ever actually occurs.

---

## Practical Experiment

1. **Finish Question 5.** Implement `print_hex` for real — extract each
   nibble of `dx` with `and`/`shr`, convert to an ASCII hex digit,
   write it into `HEX_OUT`. Confirm `print_hex(0x1fb6)` prints exactly
   `0x1fb6`.
2. **Do the same inspection with Bochs's debugger instead**, on the same
   value in `dx`, with `info registers` — no `print_hex` call needed at
   all. Compare the two experiences directly.
3. **Attach GDB properly.** `qemu -s -S -fda os-image`, then `gdb`,
   `target remote localhost:1234`, set a breakpoint at your kernel's
   entry point, step through a few instructions.
4. On a real Linux machine: `dmesg -T | tail -30`; then trigger something
   observable (plug in a USB device, or just `sudo modprobe -r
   <harmless module>; sudo modprobe <it>`) and watch new lines appear
   live with `dmesg -w`.
5. If available: `sudo perf probe --add do_sys_open` then `perf record
   -e probe:do_sys_open -a sleep 5` while opening some files — a taste
   of dynamic tracing with zero recompilation.

## Debugging

This file *is* the debugging chapter — its own summary is the method:
pick the cheapest rung of the ladder that can answer your question.
Know a rough address and just need one value right now? Bochs's `x`
command. Need to step through your own kernel's logic? GDB. Need to
see what happened during normal operation, cheaply, on an ongoing
basis? `printk`. Need to catch something intermittent without
recompiling? `ftrace`. Need to understand a crash that already
happened and won't happen again on demand? `kdump`.

## Source Code

Your completed `print_hex` (Question 5, finally solved) — genuinely
the book's own unfinished exercise, closed out.

---

## Common Misconceptions

- **"Real kernel developers don't use print statements."** They do,
  constantly — `printk` remains the dominant technique in practice.
- **"A GUI debugger is always better than print debugging."** Each
  suits different problems: a debugger excels at "what's the state
  right here, right now"; prints excel at "what happened over time,
  cheaply, without stopping anything."
- **"You need the system still running to debug a kernel crash."**
  `kdump`/`crash` exist specifically because that's often not true.

## Interview Questions

**Basic:** What does the book's own `print_hex` design leave
unfinished, and why?
**Intermediate:** What does `qemu -s -S` actually set up?
**Advanced:** Why does `printk`/`dmesg` remain heavily used despite
sophisticated tracing tools existing?
**Systems Design:** What problem does `kdump` solve that live
debugging fundamentally can't?
**Debugging:** You need to see whether a specific kernel function is
ever called, without rebuilding or rebooting — which tool?
**Explain what happens when...:** a kernel panics on a machine
configured with kdump — what happens next, and how do you examine it?

## Active Recall

1. What does the book's own §3.5 leave as an exercise?
2. What are `b`, `s`, and `info registers` in Bochs's debugger?
3. What does `-S` do when starting QEMU for GDB debugging?
4. Why does `kdump` exist, given GDB and ftrace already exist?

## Answers

1. The actual bit-manipulation implementation of `print_hex` —
   Question 5, marked Advanced.
2. Set a breakpoint, single-step one instruction, and dump general
   registers, respectively.
3. Freezes the virtual CPU before executing the first instruction, so
   a debugger can attach before anything has run.
4. Both assume the system is still running; `kdump` captures a crashed
   kernel's memory automatically for after-the-fact analysis on a
   machine that has already rebooted.

---

## What You Should Now Understand

- The book's own honest admission that no debugger exists for this
  code, and the tool it designs (but doesn't finish) in response.
- Bochs/QEMU's debugger, used as a proper named workflow instead of an
  informal instruction.
- GDB attached to your own kernel over `qemu -s -S`.
- That `printk` isn't a beginner's crutch — it's still how most real
  kernel debugging gets done, and where `ftrace` and `kdump` extend it.

---

**Next:** file `28-virtualization.md`.
