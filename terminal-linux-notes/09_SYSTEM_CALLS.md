# 09 — System Calls

> **Where this fits in the stack**
> ```
> User  →  Terminal Emulator  →  PTY/TTY  →  Shell  →  Processes  →  [ SYSCALLS ]  →  Kernel  →  Hardware
>                                                                     ^^^^^^^^^^^^^
>                                                                     this file
> ```
> Files 07-08 already used `fork`, `execve`, and `waitpid` as concrete tools without ever explaining what "a system call" fundamentally *is*. This file backs up to formalize exactly that: why the boundary between your program and the kernel exists at all, and precisely what crosses it when you make a call like `write()`.

---

## 1. User mode and kernel mode

**L1 — intuition:** the CPU itself has an "ordinary programs" mode and a "the operating system" mode, and it will not let code in the first mode do everything code in the second mode can.

**L2 — systems detail:** this is not a software convention the OS invented — it's a **hardware-enforced** privilege level, built into the CPU. On x86-64, this is implemented as protection rings (ring 0 = kernel, ring 3 = user, in practice — rings 1-2 exist but go essentially unused on Linux). The CPU itself refuses to execute certain instructions, or access certain memory, unless it is *currently* running at the required privilege level — checked automatically by the hardware on every relevant instruction, not by any cooperating software agreement.

**Why this has to exist:** if any user-mode program could freely execute privileged instructions — manipulate page tables directly, issue raw disk-controller commands, disable interrupts — there would be no isolation between programs at all. One buggy or malicious program could corrupt any other process's memory, or take down the entire machine. The hardware privilege boundary is what makes it *safe* for many independent, mutually-untrusting programs to share one machine.

---

## 2. Why user programs can't directly perform privileged operations

Reading or writing arbitrary physical memory, directly driving hardware (disk, network card), and creating new processes all require touching kernel-owned data structures and hardware interfaces that user-mode code is hardware-blocked from reaching directly. The **only** sanctioned way for a user-mode program to get one of these things done is to **ask the kernel to do it on its behalf**, through a narrow, well-defined interface: the **system call**.

---

## 3. What is a system call?

A strictly defined, numbered set of requests a user-mode program can make of the kernel. Conceptually, it's a function call — but one that has to cross the hardware privilege boundary via a special CPU mechanism, rather than an ordinary `call` instruction. Every syscall has a **number**, and the kernel keeps a table mapping numbers to the kernel-side handler function that actually performs the work.

---

## 4. The exact transition, step by step

```
user mode
   ↓
syscall instruction
   ↓
kernel
   ↓
system call handler
   ↓
kernel subsystem
   ↓
return to user mode
```

**On x86-64 Linux, specifically:**

1. User code places the **syscall number** in register `rax`, and its arguments in `rdi`, `rsi`, `rdx`, `r10`, `r8`, `r9`, in that order.
2. User code executes the dedicated `syscall` instruction — a hardware instruction whose *entire purpose* is this privilege transition (the older 32-bit x86 mechanism, `int 0x80`, worked similarly but through the slower general interrupt-descriptor-table path; `syscall`/`sysret` use dedicated, faster hardware support built specifically for this).
3. The CPU switches to kernel mode (ring 0) and jumps to a **fixed kernel entry point**.
4. That entry routine looks up the number in `rax` against the kernel's syscall table, and dispatches to the matching **handler function**.
5. The handler does the real work — for `write()`, this means finding the right **kernel subsystem** for that particular file descriptor (the VFS layer for an ordinary file, the network stack for a socket) and performing the operation there.
6. The handler places its return value back in `rax`, and the kernel executes `sysret`, switching the CPU back to user mode and resuming your program exactly where it left off.

> **Under the hood**
> Why `r10` for the fourth argument instead of the more conventional `rcx`? Because the `syscall` instruction itself uses `rcx` internally to save the return address for the trip back. Argument 4 had to move somewhere else to avoid being clobbered — a small, very concrete example of hardware-level constraints leaking directly into an ABI convention.

**All of this is hidden behind what looks like an ordinary function call**, because your C library (glibc, or another libc) provides thin wrapper functions — the `read()`, `write()`, `openat()` you actually call — that handle loading the registers and issuing the `syscall` instruction for you.

---

## 5. Concrete syscalls

| Syscall | What it does |
|---|---|
| `read(fd, buf, count)` | Read bytes from an open file descriptor |
| `write(fd, buf, count)` | Write bytes to an open file descriptor |
| `openat(dirfd, path, flags)` | Open a file, resolving `path` relative to an already-open directory fd |
| `close(fd)` | Close an open file descriptor |
| `fork()` | Create a new process (file 08) |
| `execve(path, argv, envp)` | Replace the calling process's image (file 08) |
| `mmap(...)` | Map a file, or anonymous memory, into the process's address space (file 21) |
| `brk(addr)` | Adjust the size of the process's heap (file 21) |
| `socket(domain, type, protocol)` | Create a new socket (file 30) |
| `connect(fd, addr, addrlen)` | Connect a socket to a remote address (file 30) |

> **Under the hood — why "openat", not just "open"?**
> The plain `open(path, ...)` syscall resolves `path` relative to the process's current working directory at the moment it runs — which creates a genuine race condition (time-of-check-to-time-of-use, "TOCTOU"): between deciding *which* path to open and the kernel actually resolving it, something else could swap a directory component out from under you (e.g., replace it with a symlink pointing somewhere unexpected), tricking your program into operating on a different file than intended. `openat(dirfd, path, ...)` resolves `path` relative to an **already-open, pinned** directory file descriptor instead of a mutable path string, closing that race. This is why modern glibc and most coreutils-style tools prefer the `*at()` family (`openat`, `fstatat`, `unlinkat`, and others) even for what looks like a perfectly ordinary absolute-path operation.

---

## 6. Why libc functions are not necessarily system calls: `printf()` vs `write()`

**`write()` genuinely is (a thin wrapper directly around) a system call.** Every single call crosses into the kernel, no exceptions.

**`printf()` is not.** It's a **library function** that:
1. Formats your string according to the format specifiers — entirely in **user-mode** code, inside glibc.
2. Appends the formatted result into a **user-mode buffer** attached to the `stdio` `FILE*` stream (typically `stdout`).
3. Only actually calls `write()` — crossing into the kernel — when that buffer needs to be **flushed**: possibly after many separate `printf()` calls have quietly accumulated into one buffer, possibly immediately (unbuffered or line-buffered streams), or possibly *never*, if the process terminates abnormally before the buffer is flushed at all.

> **Interview answer**
> "Not every libc function is a system call. `write()` is a thin wrapper directly around one. `printf()` is a higher-level library function that formats text and buffers it in user space, only occasionally calling `write()` once enough output has accumulated — which is exactly why output can appear delayed, batched, or even lost entirely if a program crashes before flushing."

This is precisely why output can seem to appear "out of order" when you mix `printf()` with direct `write()` calls in the same program, and it's the exact mechanism behind file 08's `_exit()` vs `exit()` distinction — a child that inherits a copy of the parent's unflushed buffer and calls ordinary `exit()` flushes *that copy*, potentially duplicating output the parent will also print later.

**The general pattern, worth generalizing:** many libc functions are convenience layers doing real user-mode work of their own — formatting, buffering, memory-pool management — around a much smaller number of actual kernel crossings underneath. `malloc()` is another example: it manages a user-mode memory pool and only occasionally calls `brk()`/`mmap()` to grow it (file 21 goes deep on this specific case).

> **Common mistake**
> "System calls are slow because a function call is being made." The overhead isn't "calling a function" in the ordinary sense — it's the **privilege-level transition itself**: CPU pipeline effects, mode switches, and the mandatory argument/return marshaling through fixed registers. This overhead is real and measurable, which is exactly *why* buffering exists in `stdio` at all: to avoid paying a syscall's cost once per character.

---

## 7. Watching this yourself: a first taste of `strace`

```
strace -c ls
```

Runs `ls`, and instead of showing every individual syscall, prints a **summary**: which syscalls were made and how many times each. You'll see `openat`, `read`, `write`, `close`, `mmap`, and others, with counts — direct, observable proof that "running `ls`" really does mean "making a specific, countable sequence of these exact syscalls." File 32 is a full dedicated deep dive into reading `strace` output line by line; this is just enough to see the mechanism is real.

---

## 8. Common misconceptions

| Misconception | Reality |
|---|---|
| "Every libc function is a system call." | Many are user-mode logic (formatting, buffering, memory management) wrapped around occasional, much rarer kernel crossings. |
| "System calls are slow because of ordinary function-call overhead." | The real cost is the hardware privilege-level transition itself — meaningfully more expensive than a normal call, and the entire reason stdio buffering exists. |
| "User mode / kernel mode is a software convention the OS could ignore if it wanted to." | It's hardware-enforced by the CPU's privilege rings — the OS uses this capability, it doesn't invent it. |

---

## 9. Try this

> **Try this**
> ```
> man 2 write
> man 3 printf
> ```
> Man page **section 2** is system calls; **section 3** is library functions. This numbering convention directly encodes this file's core distinction — `man 2 <name>` and `man 3 <name>` for the same word can be two genuinely different pages describing two genuinely different things (try `man 2 chdir` vs thinking about where `cd` fits from file 07).

> **Try this**
> ```
> strace -e trace=write echo hello
> ```
> Filters `strace`'s output down to just `write()` calls — you'll see `echo` writing its output in exactly one `write()` call, giving you a concrete, minimal example to compare against a `printf`-heavy program's batched behavior.

---

## 10. macOS note

The exact hardware-level syscall mechanism differs — Darwin/XNU's syscall numbering and calling convention aren't the same as Linux's, and XNU's hybrid Mach/BSD design adds its own wrinkles (some "system calls" on macOS are actually Mach traps under the hood, layered differently from the pure Unix syscall model). The *concept* — a hardware-enforced user/kernel boundary, crossed via a dedicated instruction, with libc wrapping the details — is identical. Full comparison: file 43.

---

## 11. Summary

- User mode and kernel mode are a **hardware-enforced** privilege distinction, not a software convention — this is what makes safely sharing one machine among many programs possible at all.
- A system call is a numbered, well-defined request a user-mode program can make of the kernel, crossing the privilege boundary via a dedicated instruction (`syscall` on x86-64 Linux).
- The transition is: load a number and arguments into fixed registers → `syscall` instruction → kernel mode → dispatch via the syscall table → kernel subsystem does the work → `sysret` back to user mode.
- **Not every libc function is a system call.** `write()` is a thin wrapper around one; `printf()` does real user-mode formatting and buffering work, calling `write()` only occasionally — which is exactly why output ordering and buffering surprises happen.

---

## What's next

`10_FILE_DESCRIPTORS.md` takes the `read`/`write`/`open`/`close` syscalls from this file and goes deep on the object they all operate on: what a file descriptor actually is, the fd table, and the crucial three-way distinction between a file descriptor, an open file description, and an inode.
