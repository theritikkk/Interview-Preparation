# 00 — Master Mental Model

> **How to use this file**
> Numbered first, written last. Every diagram below was checked against the 39 files that follow it — read this first as your entry point, and come back to it afterward as the single-page refresher. If you can redraw all twelve of these from memory, unprompted, you have the mental model this entire handbook exists to build.

---

## 1. Terminal → shell → process → kernel

```
Terminal Emulator  →  PTY/TTY  →  Shell  →  Processes  →  Syscalls  →  Kernel  →  Hardware
```
A userspace GUI program renders pixels and owns a PTY master. The kernel's line discipline sits on the PTY pair. A shell — equally ordinary userspace — sits on the slave end, launching further processes via `fork`/`exec`. Every one of them reaches the kernel only through the narrow, numbered interface of a system call. *(Files 01-09.)*

## 2. stdin / stdout / stderr

```
process
 |
 +-- fd 0  (stdin)
 +-- fd 1  (stdout)
 +-- fd 2  (stderr, kept SEPARATE so pipelines never get corrupted by diagnostics)
```
Pure convention, not kernel-enforced — but universal. *(Files 10-11.)*

## 3. Pipe

```
cat        [ KERNEL PIPE BUFFER, bounded, in-memory ]        grep
  \_______________ write() ___________  read() _______________/
```
Two genuinely concurrent processes; EOF only when **every** copy of the write end, everywhere, is closed. *(File 13.)*

## 4. Process creation

```
Shell  --fork()-->  Shell  (parent, unchanged)
              \
               --> Child  (copy-on-write, diverges from here)
```
One call, two return values — `0` in the child, the child's PID in the parent. *(File 08.)*

## 5. fork + exec

```
Shell
 |
fork()
 |
 +--------+
 |        |
 Shell   Child
          |
        execve()
          |
          v
         ls
```
`fork` makes a new process; `exec` replaces an existing one's image, same PID. Kept separate specifically to leave room for fd setup — redirection, pipes — in between. *(File 08.)*

## 6. File descriptor table

```
fd (per-process index) → open file description (offset + flags, maybe SHARED) → inode (the actual file)
```
Three layers. `fork` shares the middle layer; a fresh `open()` never does. *(File 10.)*

## 7. PTY

```
Terminal Emulator → PTY master → [line discipline] → PTY slave → Shell → Programs
```
The line discipline is what makes this different from a plain pipe — canonical-mode editing, echo, signal generation, all living here. *(File 02.)*

## 8. Signals

```
Keyboard → terminal driver → ENTIRE foreground process group → SIGINT → every member
```
Not "the process you're watching" — the whole group. *(File 14.)*

## 9. Job control

```
Session
 ├── Foreground process group  (owns the terminal right now)
 └── Background process group(s)  (everything else)
```
`fg`/`bg` just reassign which group holds that one slot, via `tcsetpgrp()`. *(Files 15-16.)*

## 10. SSH

```
Local Terminal Emulator → Local SSH Client → network → sshd → NEW remote PTY/session → Remote Shell
```
A second, fully independent session hierarchy, on a different machine, triggered by a network connection instead of a GUI click. *(File 26.)*

## 11. tmux

```
Terminal Emulator → tmux CLIENT (thin, disposable) → [Unix domain socket] → tmux SERVER (persistent)
```
The server detached from any controlling terminal at startup — a dropped connection has nothing of its own to kill inside the server's hierarchy. *(File 27.)*

## 12. Container terminal

```
docker exec -it container bash
          |
   new PTY, across a NAMESPACE boundary (not a network one) — otherwise identical to ssh -t
```
Namespaces isolate what a process can see; cgroups bound what it can use. Both required to justify "container ≠ VM." *(File 29.)*

---

## The pressure test

Cover this file. Redraw each of the twelve from memory, in any order. Wherever you hesitate, that's not a gap in this file — it's a pointer to exactly which earlier file deserves one more pass.

---

## What's next

Start at `01_TERMINALS_AND_TERMINAL_EMULATORS.md`, or jump straight to whichever numbered file covers what you actually need right now — every file is self-contained enough to read out of order, and cross-references the rest.
