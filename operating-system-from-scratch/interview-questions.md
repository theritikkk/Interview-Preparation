# Interview Questions

A curated set across the whole course, by category. Every file's own
"Interview Questions" and "Active Recall" sections have the complete
set with full answers — this is the practice index, not a replacement.
The point is reasoning, not recall: for each one, answering "why,"
mechanically, matters more than the one-line answer.

---

## Basic

1. What is the fetch-decode-execute cycle? — `01`
2. Why isn't `0x50` equal to decimal `50`? — `02`
3. What's the difference between a CPU register and a RAM location? — `03`
4. What does `[org 0x7c00]` actually do for you? — `05`
5. Why can't BIOS just "open the operating system file"? — `06`
6. What single bit, in what register, switches real mode to protected mode? — `08`
7. What's the difference between an object file and a final, runnable binary? — `11`
8. What's the difference between a program, a process, and a thread? — `19`
9. What's the difference between memory-mapped I/O and port I/O? — `15`
10. What does the eBPF verifier check before a program is allowed to run? — `35`

## Intermediate

1. Why does the stack grow downward on x86, and how would you prove it? — `05`
2. Why must a bare-metal kernel's final binary strip out the metadata a normal Linux executable keeps? — `11`
3. Using the book's own disassembly, explain how a callee retrieves an argument passed on the stack. — `10`
4. Why is the 8259 PIC's default vector mapping dangerous, and what fixes it? — `14`
5. Why can't an interrupt handler use a mutex? — `23`
6. What causes a VM exit, and what happens during one? — `28`
7. What's the difference between what a namespace does and what a cgroup does? — `29`
8. Why doesn't a high-throughput NIC just raise one interrupt per packet? — `30`
9. Why does `qemu -s -S` freeze the guest CPU before you attach GDB? — `27`
10. What's the practical difference between Secure Boot and measured boot? — `34`

## Advanced

1. Why is x86 called a variable-length instruction set, and what does that cost a hardware decoder? — `03`
2. Why must the far jump immediately after enabling protected mode be far, not near? — `08`
3. Why does `ebp+0x8` reach into the *caller's* stack frame, not the callee's own locals? — `10`
4. Why can a single-instruction `add [mem], 1` still race on a multi-core machine? — `23`
5. How does register renaming let out-of-order execution avoid false dependencies on the same architectural register? — `33`
6. Why is VMX root/non-root mode an axis orthogonal to rings 0–3, rather than just "the guest runs at a lower ring"? — `28`
7. Why can a single information leak defeat ASLR for an entire process's remaining lifetime? — `31`
8. Why is an eBPF program considered safer than a kernel module, given both run inside the kernel? — `35`
9. What's the practical difference between the 80286's protected mode and the 80386's? — `04`
10. Why does a kernel need to validate every user-supplied pointer before dereferencing it? — `21`

## Systems Design

1. If you were designing a new ISA today, would you choose fixed-width or variable-width instruction encoding — what are you optimizing for either way? — `03`
2. Using only the GDT (no paging), how would you configure segments for genuine memory protection, and what would it cost? — `08`
3. With far more registers than original x86, would you still pass arguments on the stack? What does System V AMD64 suggest? — `10`
4. Why do real bootloaders and embedded firmware still use `--oformat binary`-style raw extraction, decades after richer formats exist? — `11`
5. Why does Linux keep a run queue per core instead of one global queue? — `24`
6. Why would a Kubernetes networking layer (Cilium) prefer eBPF/XDP over traditional `iptables` rules? — `35`
7. Why might a full-disk-encryption tool bind its automatic unlock to specific TPM PCR values instead of trusting Secure Boot alone? — `34`
8. Why is round-robin a reasonable first scheduler to build, given Linux's own scheduler goes much further? — `20`, `38`
9. Where would a unikernel and an exokernel sit on the isolation-vs-speed spectrum, and why do they differ despite both being "minimal"? — `36`
10. Why does a Firecracker microVM boot so much faster than a full traditional VM? — `28`

## Debugging

1. A boot sector shows a blank screen, no output, no crash — what are the first two things you check? — `06`
2. A protected-mode switch causes an immediate reboot (triple fault) — name three specific things to check. — `08`
3. A program crashes with "stack smashing detected" — what mechanism catches this, and what layout fact does it rely on? — `10`
4. An ISR fires exactly once and never again — what did it forget? — `14`, `16`
5. A context switch resumes a process with corrupted registers — what's the most likely cause? — `20`
6. A disk read reports success (no carry flag) but the buffer contains garbage — what's the first thing to check? — `17`
7. A parallel job scales to 4 threads and gets slower at 16 — list three causes and how to tell them apart. — `24`
8. A crash's return address doesn't correspond to any real call site in the source — what does that suggest? — `31`
9. You're told "the app is slow" with no other detail — what do you measure first, and why? — `32`
10. A containerized process is silently killed under load — what's the first thing to check, and where? — `29`

## "Explain what happens when..."

The course's best full-chain questions — each one traces something
from one end to the other, using nothing invented along the way.

1. ...you power on a PC with a bootable floppy inserted, through to your own first instruction executing? — `06`
2. ...an x86-64 Linux machine boots, through real mode, protected mode, and finally long mode? — `08`
3. ...a C function with two parameters and one local variable is called, from just before `call` to just after `leave`? — `10`
4. ...a Linux program makes a syscall — how is the register setup conceptually the same as the book's BIOS interrupt pattern? — `03`, `21`
5. ...a keypress happens on a real machine, from the electrical signal to a character appearing in a terminal? — `16`
6. ...a page fault occurs for memory that was lazily allocated but never touched? — `22`
7. ...a timer interrupt fires while process A is running and process B resumes instead? — `20`
8. ...you run `curl example.com` — trace the outgoing request from the syscall to the wire, and the response back? — `30`
9. ...a guest OS executes `cli` inside a KVM-accelerated VM — trace it through a VM exit and back? — `28`
10. ...you run `docker run <image>` — trace it through containerd, runc, and the kernel features actually created? — `29`
11. ...a CPU speculatively executes down a mispredicted branch and then discovers the misprediction — what's discarded, and what, subtly, isn't? — `33`
12. ...a program protected by a stack canary suffers a buffer overflow — trace it to "stack smashing detected"? — `10`, `31`
13. ...you run `perf record -g` on a running program — trace it from the timer interrupt to the flame graph you view? — `32`
14. ...a machine with both Secure Boot and TPM-bound disk encryption boots a tampered bootloader? — `34`

---

## How to Use This

Pick one question. Answer it out loud, from memory, tracing every step
by name — not just the destination. If you reach for a term you can't
define precisely, that's `glossary.md`'s job. If you reach for a step
you can't explain *why* it happens, that's a sign to reopen that
question's home file, not just its answer.
