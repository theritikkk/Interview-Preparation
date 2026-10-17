# 21 — Process Memory

> **Where this fits in the stack**
> ```
> User  →  ...  →  Processes  →  [ MEMORY ]  →  Kernel  →  Hardware (MMU, RAM)
>                                 ^^^^^^^^^^
>                                 this file
> ```
> File 08 described `execve()` replacing a process's "image" without detailing what that image actually looks like in memory. This file lays it out precisely, connects `malloc()` down to the two syscalls underneath it, and explains what ASLR actually randomizes — and what it deliberately doesn't.

---

## 1. Virtual memory, briefly reframed

Every process has its own **virtual address space** (file 08). A process never touches a physical RAM address directly — every memory reference goes through the CPU's MMU (memory management unit), which translates virtual addresses to physical ones via page tables the kernel maintains **per process**. This is exactly why two different processes can both "have" something at virtual address `0x400000` with zero conflict — those are different physical pages, mapped completely independently by each process's own page tables.

**Why virtual memory exists at all:** isolation (a process can't read or write another's memory just by guessing an address — there's no address it could guess that maps to anyone else's physical pages) and flexibility (a process's memory needn't be physically contiguous, and parts of it can even be transparently swapped to disk without the running program ever noticing).

---

## 2. The layout, segment by segment

```
High addresses
+-------------------+
|       stack        |  <- grows DOWN
|         |          |
+-------------------+
|   (unmapped gap)   |
|  shared libraries   |  <- mmap'd in by ld.so
+-------------------+
|        heap         |  <- grows UP
+-------------------+
|        bss          |
+-------------------+
|        data         |
+-------------------+
|       rodata        |
+-------------------+
|        text         |
+-------------------+
Low addresses
```

- **Text (code)** — the actual machine instructions. Typically read-only, and the kernel can map the **same physical pages** of it into multiple processes running the same program (or the same shared library) simultaneously — real memory saved by never duplicating instructions that never change.
- **Rodata** — string literals, `const` globals. Read-only, shareable, same reasoning as text.
- **Data** — initialized global/static variables (`int x = 5;` at file scope) — **not** shared; each process needs its own, independently-modifiable copy.
- **BSS** ("Block Started by Symbol" — an odd, genuinely historical name from old assembler directives) — *uninitialized* global/static variables (`int y;`, no initializer). The key optimization: these occupy **no space in the executable file on disk at all**, since they're all zero initially — the kernel just allocates zeroed pages for this region at load time, rather than the ELF file needing to literally contain a block of zero bytes for every uninitialized global array.
- **Heap** — dynamically allocated memory, traditionally growing upward from just after BSS.
- **Stack** — function call frames and locals, traditionally growing downward from near the top of the address space.
- A large **unmapped gap** sits between heap and stack, giving each room to grow for a long time without colliding — and it's also where shared libraries typically get `mmap`'d in.

---

## 3. How shell-created processes actually get their address space

Recap file 08, now in full memory-layout terms: `fork()` initially gives the child a copy-on-write version of the **parent's entire address space** — so immediately after forking, a not-yet-`exec`'d child temporarily has the exact same layout as its parent shell. Then `execve()` **discards all of that** and builds a completely new address space from the target ELF file: the loader maps text/rodata/data per the ELF's program headers, sets up a fresh zeroed BSS, builds a fresh stack (with `argv`/`envp` placed on it), and — for dynamically linked binaries — `ld.so` subsequently `mmap`s in the needed shared libraries. This is file 08's "process image replacement," now shown at the memory-layout level.

---

## 4. `malloc()` traced to `brk()`/`mmap()`

`malloc()` is a **library function** — file 09's `printf`-vs-`write` lesson, generalized. It does **not** make a syscall on every allocation. Instead, it manages its own user-mode memory pool, tracking free and used chunks within memory it's already obtained from the kernel, and only reaches out to the kernel occasionally, when its existing pool can't satisfy a request.

- **Small allocations** typically extend the **heap** via `brk()` — moving the "program break" (the current end of the heap segment) further up — getting a chunk of additional address space `malloc` then subdivides internally for many future small allocations.
- **Large allocations** (above a threshold — commonly around 128 KB by default in glibc) typically go straight to **`mmap()`** instead, for a separate, standalone mapping just for that one allocation. This also simplifies freeing: `munmap()` the whole thing at once, rather than risking fragmenting the main heap.

**`free()`, correspondingly:** for heap-based (`brk`) chunks, it marks them free within `malloc`'s own bookkeeping — it does **not** necessarily shrink the heap back down immediately; the memory often stays reserved from the kernel's perspective, ready for a future `malloc()` call to reuse without asking the kernel again. For `mmap`-based large allocations, `free()` typically does call `munmap()` immediately.

> **Common mistake**
> "`free()` always immediately returns memory to the kernel/OS." Heap-based frees are frequently just kept in `malloc`'s own pool for future reuse — visible, real memory usage from the OS's point of view can stay elevated well after your program has logically freed everything it's using.

---

## 5. ASLR, precisely

Recap and deepen file 08: **Address Space Layout Randomization** — the kernel randomizes the *base* addresses of the stack, the `mmap`'d region (shared libraries), and, for position-independent executables (PIE — the default compile mode on most modern distros), even the main executable's own code, freshly on every single `exec`. This is specifically to make memory-corruption exploits harder to reliably pull off — an attacker can't assume a fixed, predictable address for a useful piece of code or data to target.

> **Common mistake**
> "ASLR randomizes everything about a process's memory layout, right down to relative offsets." It randomizes the **base address** of each major region — the *internal* layout within one mapped region stays fixed relative to that base (functions inside one shared library keep the same offsets from each other; only the library's starting address moves).

```
cat /proc/sys/kernel/randomize_va_space
```
Shows the current ASLR mode: `0` = off, `1` = partial, `2` = full (the standard Linux numeric levels). Preview: file 33.

---

## 6. `ulimit` / `getrlimit` / `setrlimit`

Every process operates under a set of kernel-enforced **resource limits**: maximum open file descriptors, maximum stack size, maximum processes a user can create, maximum CPU time, and others — queried or set via `getrlimit()`/`setrlimit()`, or from the shell via the `ulimit` builtin.

Each limit has a **soft** limit (the currently enforced cap — a process can raise this itself, up to the hard limit, without special privilege) and a **hard** limit (the ceiling — only a privileged process can raise that one).

```bash
ulimit -n     # max open file descriptors
ulimit -a     # every current limit, at once
```

> **Remember**
> `ulimit -n` is exactly the limit behind the extremely common **"too many open files"** error (`EMFILE`) in real production systems. A server that leaks file descriptors over time — forgetting to `close()` them, exactly the discipline file 10 insisted on — will eventually hit this ceiling and start failing, often mysteriously, far from wherever the actual leak occurred. This is a direct, concrete instance of the kind of bug file 37's debugging scenarios are built around.

These limits are **inherited across `fork`/`exec`**, exactly like the environment (file 17) — a limit set in your interactive shell, or system-wide via `/etc/security/limits.conf` (itself enforced through **PAM**, file 20's authentication framework, doing double duty here), applies to every program that shell subsequently launches, unless something in between explicitly raises or lowers it further.

---

## 7. Common misconceptions

| Misconception | Reality |
|---|---|
| "`malloc()` always makes a syscall." | Usually served from its own user-mode pool — file 09's general libc-wraps-syscalls lesson applies here directly. |
| "ASLR randomizes everything, including offsets within one region." | Typically just the *base* address of each major region; internal layout stays fixed relative to that base. |
| "`ulimit` only matters for exotic edge cases." | "Too many open files" is a genuinely common real-world production bug, directly caused by hitting this exact limit. |
| "`free()` always hands memory straight back to the OS." | Heap-based frees are frequently retained in `malloc`'s own pool for reuse, not returned via `brk()` immediately. |

---

## 8. Try this

> **Try this**
> ```
> ulimit -a
> ```
> Your shell's complete current set of resource limits, in one place.

> **Try this**
> ```
> cat /proc/self/maps
> cat /proc/self/maps
> ```
> Run as two **separate** invocations of `cat` (each is its own process) — compare the base addresses of shared libraries between the two runs. Different every time: ASLR, directly observed.

---

## 9. Summary

- A process's virtual address space is laid out as text/rodata/data/BSS/heap/(gap)/shared-libraries/stack, with the MMU translating every reference through per-process page tables.
- `execve()` discards the old address space entirely and builds a new one from the target ELF's program headers — nothing about the old layout survives.
- `malloc()` manages its own pool, reaching the kernel only occasionally — small requests typically grow the heap via `brk()`, large ones typically get their own `mmap()`.
- ASLR randomizes the base address of major regions on every `exec`, not the internal layout within any one of them.
- `ulimit`/`getrlimit`/`setrlimit` enforce real, inherited resource ceilings — `ulimit -n` is the exact mechanism behind the common "too many open files" production failure.

---

## What's next

`22_SCHEDULER.md` covers what actually decides *when* each of these processes gets CPU time — runnable/running/sleeping/blocked states, context switching, and why a pipeline's processes can appear to run simultaneously without oversimplifying it into "each gets its own CPU."
