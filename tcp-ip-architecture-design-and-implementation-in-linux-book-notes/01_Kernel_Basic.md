# 1. Kernel Basics and TCP/IP System Calls
### *(Covers the book's Chapter 1 — "Linux Kernel and TCP/IP Application Interfaces")*

Before touching a single networking data structure, you need the two or three kernel-architecture facts that everything else in this book leans on: how your application's function calls actually reach kernel code at all, and what "context" that kernel code is running in when it gets there.

## 1.1 Kernel 2.4.20: Book's-eye view

Linux is a **monolithic kernel**: unlike a microkernel design (where drivers, filesystems, and networking might run as separate, isolated server processes passing messages to each other), all of this — process scheduling, memory management, filesystems, and the entire networking stack — runs together in one privileged address space, one single "kernel." This matters immediately for networking: it means the TCP/IP stack has direct, fast access to everything else in the kernel (memory allocators, the scheduler, other subsystems) without needing message-passing overhead — but it also means a bug in networking code can, in principle, destabilize the whole system, which is a large part of why kernel networking code is written so defensively and reviewed so carefully.

At the 2.4.20 era specifically, Linux is **not yet preemptible in kernel mode** — once a process enters kernel space (via a system call, for instance) and isn't blocked waiting on something, it keeps running until it either voluntarily yields the CPU or returns to user space; the scheduler can't forcibly switch it out mid-way through a system call the way it could in user mode. (Kernel preemption was introduced later — the mainline `CONFIG_PREEMPT` option arrived with kernel 2.6.) This distinction is exactly why the networking stack's own internal locking (`spin_lock_bh()`/`spin_lock_irq()` variants you'll meet in later chapters) exists — the concern isn't (yet) about a preemptible kernel thread interrupting itself, but specifically about a hardware interrupt or softIRQ (which *can* happen at any time, even mid-instruction, regardless of kernel preemption) touching the same data.

## 1.2 Process context vs. interrupt context — the distinction the rest of this book assumes

This is the single most important mental model to carry forward into every later chapter, because it explains *why* the networking stack is architected the way it is (and it's the direct reason Chapter 17's softIRQ mechanism exists at all).

- **Process context**: code running because a specific process asked for it — directly, by making a system call (`write()`, `send()`, `read()`), or indirectly, kernel code doing work on that process's behalf. You know *which* process this code is running for; you can (usually) sleep here — block waiting for something, and let the scheduler run something else meanwhile.
- **Interrupt context**: code running because a hardware device (a network card, in this book's case) raised a hardware interrupt, demanding immediate attention. This code isn't running "on behalf of" any particular process — it preempts whatever *was* running, does a strictly limited amount of urgent work, and gets out, fast. **You cannot sleep in interrupt context** — there's no process to reschedule around; blocking here would, in a real sense, freeze the entire CPU.

Every networking event that originates from *hardware* (a packet arriving on the wire) starts in interrupt context and has to hand off to process context (or, as you'll see repeatedly starting in Chapter 17, to the intermediate softIRQ context invented specifically to bridge this gap) before any substantial processing happens. Every event that originates from an *application* (calling `send()`) starts in process context from the very beginning. Keeping straight which context a given piece of code you're reading is running in is the single habit that makes the rest of this book's kernel-walkthrough chapters click into place rather than feeling like an arbitrary maze of functions.

## 1.3 How a socket call actually reaches the kernel

You've called `socket()`, `bind()`, `connect()`, `send()` throughout the earlier two study packs in this series as if they were ordinary C library functions. They are — but only the first hop. Here's the layering, top to bottom:

```
 Your application               socket(), bind(), send(), recv(), ...
        │
 glibc (libc)                    thin wrapper — packs arguments, issues
        │                        the actual trap into kernel mode
        ▼
 ─────────────────── kernel/user boundary ───────────────────
        │
 System call entry              on 2.4-era x86, EVERY socket-related
        │                       call funnels through one single system
        │                       call number, sys_socketcall(), which
        │                       takes a "which operation is this"
        │                       subcode (SYS_SOCKET, SYS_BIND,
        │                       SYS_CONNECT, SYS_ACCEPT, SYS_SEND, ...)
        │                       as its first argument, then dispatches
        ▼
 sys_socket() / sys_bind() /     the real per-operation kernel functions
 sys_connect() / sys_accept()    — this is where Chapters 3–4 pick up
        │
        ▼
 BSD socket layer                struct socket, struct proto_ops —
        │                        generic, protocol-independent
        ▼
 INET / protocol-specific layer  struct sock, struct proto — TCP/UDP-
                                  specific implementation (Chapter 3)
```

**Why funnel everything through one syscall number (`sys_socketcall`) instead of giving `bind()`, `listen()`, `accept()`, etc. their own separate syscall numbers, the way `read()`/`write()`/`open()` each get their own?** Largely historical: syscall numbers were a genuinely scarce resource on early Linux/x86, and grouping every socket-related operation behind one multiplexed entry point (indexed by a subcode, the same trick BSD itself originally used) economized on that scarce space. `sys_socketcall`'s job is small and mechanical: read the subcode and a pointer to the real arguments out of user space, then jump to the actual handler (`sys_bind()`, `sys_listen()`, and so on) — all the genuinely interesting work happens one level deeper, which is exactly where Chapters 3 and 4 take you.

## 1.4 The system calls this book cares about, and where each leads

| System call | What it does | Where this book explains the kernel side |
|---|---|---|
| `socket()` | Allocate a `struct socket` + `struct sock`, wire up the right protocol's operations table | Chapter 3 |
| `bind()` | Associate a local address/port with a socket | Chapter 4 |
| `listen()` | Mark a socket as willing to accept incoming connections, size the SYN/accept queues | Chapter 4 |
| `connect()` | Actively initiate a TCP connection (or, for UDP, just record a peer address) | Chapter 4 |
| `accept()` | Pull a fully-established connection off the accept queue, handing back a new socket | Chapter 4 |
| `send()`/`write()` | Hand data to the kernel for transmission | Chapter 8 |
| `recv()`/`read()` | Retrieve data the kernel has received and queued | Chapter 7 |
| `close()` | Tear down a socket, potentially triggering TCP's connection-termination state transitions | Chapter 11 |

## 1.5 What's changed since kernel 2.4.20

- **Per-operation syscalls, at least on some architectures.** The `sys_socketcall()` multiplexer described above is genuinely still present on 32-bit x86 today, largely for ABI-compatibility reasons — but 64-bit x86 (`x86_64`) has given socket operations their own individual syscall numbers since it was first introduced, and doesn't use the multiplexer at all. If you look at a modern `x86_64` kernel's syscall table, you'll find `bind`, `listen`, `accept4`, `sendto`, and friends each with their own dedicated number — the "one multiplexed entry point" design in this chapter is specifically a 32-bit-x86-legacy pattern, not a universal one, even though it was the norm when this book was written.
- **Kernel preemption.** `CONFIG_PREEMPT` (mainlined in the 2.6 series, and effectively always enabled on distribution kernels since) means a process running in kernel mode genuinely can be preempted by the scheduler under the right conditions — the strict "runs until it yields or blocks" model in §1.2 is specifically a 2.4-era description of the *non-preemptible* kernel default.
- **RCU (Read-Copy-Update)**, introduced during the 2.5 development series and adopted heavily throughout networking code since, gives many of this era's read-heavy data structures (routing tables, socket hash tables) a lock-free-for-readers alternative to the spinlocks this book describes almost everywhere — readers traverse a structure without taking any lock at all, while writers atomically swap in a new version and defer freeing the old one until it's provably safe. This is one of the single biggest architectural shifts separating "how 2.4-era networking code was written" from "how a modern kernel's networking code is written," and you'll see it referenced again in several later chapters' "what's changed" sections.
- **io_uring** (added in kernel 5.1, 2019) offers a fundamentally different, batched, largely syscall-free I/O submission model that can be used for networking as well as disk I/O — worth knowing exists, even though the classic per-call syscall interface this chapter describes remains fully supported and is still what the vast majority of networked applications actually use.

---

## Quick recap (for revision)

- Monolithic kernel: networking, memory management, scheduling, and filesystems all share one address space — fast, but a bug anywhere is a bug everywhere.
- **Process context** (code running on a specific process's behalf, can sleep) vs. **interrupt context** (code running because hardware demanded attention, cannot sleep) is the single distinction the rest of this book's architecture is built around.
- On 2.4-era x86, every socket-related syscall funnels through one multiplexed entry point, `sys_socketcall()`, dispatching by subcode to the real handler — a scarce-syscall-numbers-era design choice, not a universal Linux trait.
- 2.4.20 predates kernel preemption (`CONFIG_PREEMPT`) and RCU — both are essential vocabulary for reading *any* modern kernel networking code, and their absence is exactly why 2.4-era code leans as heavily on plain spinlocks as it does.

Next: `02_Protocols_TCP_IP_ARP_ICMP.md` — the wire-format fundamentals every later implementation chapter assumes you already know.