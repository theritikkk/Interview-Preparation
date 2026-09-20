# 08 — Processes: fork() and exec()

> **Where this fits in the stack**
> ```
> User  →  Terminal Emulator  →  PTY/TTY  →  Shell  →  [ PROCESSES ]  →  Syscalls  →  Kernel  →  Hardware
>                                                       ^^^^^^^^^^^^^
>                                                       this file
> ```
> File 07's trace used `fork`, `execve`, the loader, and `waitpid` as named steps without opening any of them up. This file opens every one of them: what a process actually is to the kernel, exactly what `fork()` copies versus shares, why `execve()` never returns on success, where the ELF file it loads even comes from, and how dynamic linking actually gets a program running.

---

## 1. What is a process?

**L1 — intuition:** a running program.

**L2 — systems detail:** a process is the kernel's unit of resource ownership and scheduling — an independent virtual address space, a table of open file descriptors, an execution context (register state, including the instruction pointer), and a large amount of kernel-tracked bookkeeping (Section 4). Two processes running the exact same program are still two completely independent processes — separate memory, separate everything, coincidentally running identical code.

**L3 — implementation:** Linux represents every process (and, per the note below, every *thread*) with one `task_struct`.

> **Under the hood**
> Linux doesn't have a fully separate kernel structure for "thread" versus "process" the way some textbooks imply — both are `task_struct`s. `clone()` (the more general syscall `fork()` is built on) takes flags controlling exactly *how much* gets shared with the new task: share the address space and you've made something that behaves like a thread; share nothing and you've made an ordinary forked process. This handbook stays scoped to process-level `fork`, not threading, but it's worth knowing there's one underlying mechanism, not two.

---

## 2. PID and PPID

Every process gets a unique **PID** from the kernel at creation. Every process except PID 1 (file 41 covers what PID 1 is) has a **PPID** — the PID of whoever `fork()`'d it. `getpid()`/`getppid()` are the syscalls; `ps -o pid,ppid` (files 01-02) is how you observe them from outside.

> **Common mistake**
> "A PID uniquely and permanently identifies a specific run of a specific program, forever." PIDs are **reused**. Once a process exits and its parent has reaped it (Section 10), the kernel is free to hand that same number to a completely unrelated future process. A PID you saw in yesterday's log almost certainly belongs to something else entirely today.

---

## 3. Process vs process image — the distinction that makes `execve` make sense

- **Process** — the kernel-tracked entity: a PID, a parent, open files, a scheduling state. This *persists* across an `execve()` call.
- **Process image** — the actual code, data, heap, and stack currently loaded and executing — in other words, *which program* is running. This is exactly what `execve()` replaces.

Keep these two concepts distinct and `execve()` stops being confusing: it doesn't create anything, and it doesn't destroy the process — it swaps out *what's running* while the process's identity (PID, parent, open files) carries straight through.

---

## 4. `task_struct` — the kernel's own bookkeeping

Conceptually (exact field names shift across kernel versions), every `task_struct` tracks at least:

- **PID**, **state** (Section 5), a **parent pointer** and a **list of children**
- A **file descriptor table** (file 10) — or, for threads, a pointer to one *shared* with siblings
- A **memory descriptor** (`mm_struct`) describing the virtual address space
- **Filesystem info** — including the `fs_struct` holding the current working directory. **This is the exact per-process attribute file 07's `cd` argument rested on** — it's real, it's here, and it's why a forked child's `chdir()` can never reach back into its parent.
- **Signal state** — pending signals, handler dispositions (file 14)
- **Scheduling info** — priority and the like (file 22)

---

## 5. Process states

| State | Meaning |
|---|---|
| **R** — Running | Actually executing, or ready to run and waiting for the scheduler |
| **S** — Sleeping | Blocked, waiting for something (I/O, a signal), interruptibly |
| **D** — Uninterruptible sleep | Blocked in a way that can't even be interrupted by a signal — typically waiting on I/O |
| **T** — Stopped | Suspended, e.g. by `SIGSTOP`/`Ctrl+Z` (file 15) |
| **Z** — Zombie | Exited, but not yet reaped by its parent (Section 10; full treatment: file 40) |

`ps -o stat` shows these directly. Full scheduler mechanics: file 22.

---

## 6. `fork()`

```c
pid_t pid = fork();
```

Creates a new process — the **child** — that is, at the instant of the call, an almost-exact duplicate of the calling process (the **parent**): same code, same data, same open file descriptors, same working directory, same environment.

**The return value is the entire trick that makes this usable**: `fork()` is called exactly *once*, but returns *twice* — because immediately after the call, there are two separate processes, each getting their own return value:

```c
pid_t pid = fork();

if (pid == 0) {
    /* This code runs in the CHILD. fork() returned 0 here. */
    printf("child, pid=%d\n", getpid());
} else if (pid > 0) {
    /* This code runs in the PARENT. fork() returned the child's PID here. */
    printf("parent, child's pid=%d\n", pid);
} else {
    /* fork() returned -1: no child was created at all. */
    perror("fork failed");
}
```

> **Remember**
> One function call, one line of source code, two processes afterward, each seeing a *different* return value from that same call. This is the single most important thing to internalize about `fork()` before anything else in this file makes sense.

---

## 7. What's copied, what's shared, and copy-on-write

**Copied (each process gets its own independent version):** the contents of memory (code, data, heap, stack) as of the moment of the call; the current working directory (`fs_struct`) — exactly why the child example in file 07 fails; most signal dispositions.

**Shared (both processes point at the same underlying kernel object):** open file descriptors. Precisely: each process has its own fd *table* (its own numbered slots), but right after `fork()`, a given fd number in the child points at the **identical open file description** — the kernel object tracking the file's offset and status flags — that the same fd number pointed at in the parent. This is the exact mechanism `pipe()` relies on in file 13, and file 10 formalizes the fd-vs-open-file-description distinction this sentence is quietly relying on.

**Copy-on-write (COW)** is what makes duplicating even a multi-gigabyte process's memory *fast*: rather than actually copying every page at `fork()` time, the kernel makes the child's page tables point at the **same physical memory pages** as the parent's, marking them read-only in both processes. Only when *either* process later tries to **write** to one of these shared pages does a page fault trigger the kernel to allocate a real, private copy of *just that one page*, and the write proceeds against the copy.

> **Under the hood**
> This is precisely why `fork()` immediately followed by `execve()` — the single most common pattern in this entire handbook — is cheap even for a shell with a large memory footprint: the vast majority of "copied" memory is never actually duplicated at all, because `execve()` discards the child's entire address space moments later, before anything had a chance to be written to. The kernel keeps the process abstraction perfectly consistent (every process behaves as if it has fully independent memory) while implementing it as lazily as it can get away with.

---

## 8. `execve()`

```c
int execve(const char *pathname, char *const argv[], char *const envp[]);
```

Replaces the **calling process's entire image** — code, data, heap, stack — with a new program's, while **keeping the same PID**, the same open file descriptors (unless individually marked close-on-exec), the same parent, and the same working directory.

> **Remember**
> `execve()` **only returns on failure** (`-1`). On success, it never returns at all — because the code that called it doesn't exist anymore, having just been replaced by the new program's image. This is genuinely one of the odder-sounding facts in Unix systems programming the first time you hear it, and it's exactly true.

**`execve` the syscall vs the `exec*` family of library functions:** `execl`, `execlp`, `execle`, `execv`, `execvp`, `execvpe` are all convenience **wrappers** around this one underlying syscall — differing in whether arguments are passed as a list or an array, and whether `PATH` is searched for you (`execvp`, notably, since plain `execve` requires a full path). Bash's `exec` builtin (file 04) ultimately calls one of these — typically `execve` or `execvp` — on the shell's own process, with no fork first.

---

## 9. `fork` ≠ `exec` — and why keeping them separate is a deliberate design choice

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

`fork()` creates a **new process** — a second PID, a copy. `execve()` creates **no new process at all** — it transforms an *existing* one's image, same PID. These are two entirely separate operations, and Unix deliberately never merged them into one atomic "create a process running a new program" syscall.

**Why the separation matters:** it gives you a window, *between* the fork and the exec, to modify the child — most importantly, to set up file descriptor redirection (`dup2`, file 12) or close unneeded pipe ends (file 13) — while the child is still running the *shell's own code*, before the new program image ever starts. If fork-and-exec were one atomic step, there would be no moment to do this setup at all, and redirection and pipes as this handbook describes them simply couldn't exist in their current form.

> **Interview answer**
> "`fork` and `exec` are kept as separate syscalls specifically so a program can adjust the child's file descriptors — for redirection or pipes — after creating it but before replacing its image. Merging them into one atomic operation would remove that window entirely."

---

## 10. `wait()` and `waitpid()`

```c
pid_t wait(int *status);
pid_t waitpid(pid_t pid, int *status, int options);
```

`wait()` blocks until *any* child exits and reports which one. `waitpid()` is more flexible: wait for one *specific* PID, or pass `WNOHANG` for a non-blocking check, or `WUNTRACED` to also be told about stopped children (relevant to job control, file 15).

```c
int status;
pid_t child = fork();
if (child == 0) {
    execvp("ls", (char *[]){"ls", NULL});
    _exit(127);                     /* only reached if execvp itself failed */
} else {
    waitpid(child, &status, 0);
    if (WIFEXITED(status))
        printf("ls exited with %d\n", WEXITSTATUS(status));
}
```

`WIFEXITED`/`WEXITSTATUS`/`WIFSIGNALED`/`WTERMSIG` are the standard macros for pulling apart the packed `status` value — was this a normal exit, and with what code, or did a signal kill it, and which one.

**A process that has exited but hasn't yet been `wait`ed for by its parent is a zombie** (state `Z`, Section 5) — it's kept around purely so the parent can eventually collect its exit status. File 40 covers this, and the related case of an *orphan* (a child whose parent exited first), in full.

---

## 11. `exit()` vs `_exit()`

- **`exit(status)`** — the C library function: runs any `atexit()`-registered cleanup handlers, flushes stdio buffers, *then* calls the raw syscall below.
- **`_exit(status)`** — the raw syscall: terminates the process **immediately**, no cleanup, no buffer flushing.

> **Under the hood**
> This distinction matters in exactly the fork-child scenario above: if the parent had buffered, unflushed `stdio` output before calling `fork()`, the child inherits a *copy* of that same unflushed buffer. If the child later calls ordinary `exit()`, it flushes *its copy* — producing output that's already going to be printed again by the parent later, duplicated. Using `_exit()` in a child specifically after a failed `exec` call (as in the snippet above) sidesteps this exact trap.

---

## 12. Where does the ELF file `execve` loads even come from? The compilation pipeline

Given you're a C/C++ programmer, this is the missing link between "I wrote a `.c` file" and "the kernel just loaded a process image from it":

```
your_program.c
     |
     v  PREPROCESSOR   (expands #include, #define, macros)
expanded source
     |
     v  COMPILER        (translates C → assembly for the target architecture)
assembly (.s)
     |
     v  ASSEMBLER        (translates assembly → machine code)
object file (.o)          <- already a rudimentary ELF file, but with UNRESOLVED
     |                        references (e.g. to printf, defined elsewhere)
     v  LINKER
executable (ELF)          <- ready for execve()
```

The **linker**'s job is resolving those unresolved symbol references — either by copying the needed library code directly in (**static linking**) or by simply *recording* which shared libraries and symbols are needed, leaving the actual resolution for later (**dynamic linking**, Section 14). `gcc file.c -o a.out` runs this entire pipeline in one command; each stage can also be invoked individually (`gcc -E`, `-S`, `-c`) if you want to inspect the intermediate output.

---

## 13. ELF: Executable and Linkable Format

The standard binary format Linux uses for executables, shared libraries (`.so`), object files (`.o`), and even core dumps.

- **ELF header** — magic bytes identifying the file as ELF, target architecture, and the **entry point** address where execution should begin.
- **Program headers** — describe **segments**: contiguous regions to be mapped into memory when the file is *loaded and run*. This is what the kernel's loader actually reads.
- **Section headers** — describe **sections**: finer-grained named regions (`.text`, `.data`, `.bss`, `.rodata`, `.symtab`, and others) — used by linkers and inspection tools (debuggers, `objdump`, `readelf`), and, notably, **not strictly required for the kernel to run the program at all**. The kernel cares about segments; sections are a linking/tooling-level concept layered on top.

```
readelf -h /bin/ls      # ELF header — architecture, entry point
readelf -l /bin/ls      # program headers — the segments the kernel will map
readelf -S /bin/ls      # section headers — the finer linking/debugging view
```

---

## 14. Dynamic linking and `ld.so`

**Static linking**: all needed library code is copied *into* the final executable at link time. Self-contained, larger file, no runtime dependency resolution needed.

**Dynamic linking**: the executable instead just records *which* shared libraries (`.so` files) and *which symbols* from them it needs, without including that code itself. Resolving this — finding the libraries, mapping them into memory, patching up the addresses — happens at **process startup**, performed by a dedicated program: the **dynamic linker/loader**, `ld.so` (on x86-64 Linux, typically `/lib64/ld-linux-x86-64.so.2`).

**How the kernel even knows to run `ld.so` first:** a dynamically-linked ELF executable's program headers include a `PT_INTERP` entry naming its required interpreter — `ld.so`'s own path. When the kernel's `execve` implementation loads such a binary, it loads **both** the target program *and* the named interpreter into the new process's address space, and hands control to **`ld.so`'s entry point first**, not the program's own. `ld.so` locates and `mmap`s the needed shared libraries, resolves symbols, applies relocations — and only *then* jumps to the actual program's real entry point. This is the precise mechanism behind step 16 of file 07's trace.

```
ldd /bin/ls      # lists the shared library dependencies ld.so will need to resolve
```

**Static vs dynamic — a genuine tradeoff, not a strict hierarchy:** static binaries are larger on disk and can't share library code in memory across multiple running programs, but have no runtime resolution cost and no risk of a missing/mismatched library version at deploy time; dynamic binaries are smaller, let the kernel share one physical copy of a library's code pages across every process using it, and can be patched (e.g., a security fix to `libc`) without recompiling everything that depends on it — at the cost of resolution overhead at startup and genuine "which version of this library is actually installed" fragility.

---

## 15. ASLR — a brief preview

**Address Space Layout Randomization**: the kernel randomizes *where* the stack, heap, shared libraries, and (for position-independent executables) even the program's own code get placed in virtual memory, freshly, on every run — specifically to make memory-corruption exploits harder to reliably target. Full memory-layout treatment, including exactly what gets randomized and what doesn't: file 21.

---

## 16. Common misconceptions

| Misconception | Reality |
|---|---|
| "`fork()` copies the entire process's memory, byte for byte, every time." | Copy-on-write defers the actual copying until a write happens to a shared page — often never, if `exec` follows immediately. |
| "`execve()` creates a new process." | It transforms an *existing* process's image in place — same PID throughout. |
| "`fork` and `exec` are basically one operation under two names." | Deliberately separate, specifically to leave a window for fd setup (redirection, pipes) between them. |
| "Static linking is simply worse (or simply better) than dynamic linking." | Real tradeoffs run both directions — self-contained and dependency-free vs. smaller and shareable-in-memory. |

---

## 17. Try this

> **Try this**
> ```bash
> echo 'int main(){return 0;}' > t.c
> gcc t.c -o t_dynamic
> gcc -static t.c -o t_static
> ls -la t_dynamic t_static      # compare file sizes
> ldd t_dynamic                   # real dependencies
> ldd t_static                    # "not a dynamic executable"
> ```

> **Try this**
> ```
> readelf -h /bin/ls
> ```
> Confirms the magic bytes, architecture, and prints the actual entry point address the kernel jumps to (or, for a dynamically linked binary, the entry point `ld.so` eventually hands off to).

---

## 18. macOS note

macOS/Darwin uses a genuinely different executable format, **Mach-O**, not ELF — conceptually similar in spirit (segments, a dynamic linking mechanism), but a different binary layout entirely. The dynamic linker's role is filled by **`dyld`** rather than `ld.so`, and `otool` is the macOS-side inspection tool roughly analogous to `readelf`/`ldd`. Full comparison: file 43.

---

## 19. Summary

- A process is a kernel-tracked entity (PID, parent, fds, memory, `task_struct`); a process *image* is the specific program currently running inside it — `execve()` replaces the image, never the process.
- `fork()` returns twice — 0 in the child, the child's PID in the parent — from one single call.
- Memory is copy-on-write after `fork()`; file descriptors point at genuinely shared open file descriptions.
- `execve()` never returns on success; it only returns, with `-1`, on failure.
- `fork` and `exec` are kept separate specifically to allow fd setup in between — the entire mechanism redirection and pipes rely on.
- An ELF binary comes from preprocessor → compiler → assembler → linker; the kernel loads it via its program headers (segments), and for dynamically linked binaries, runs `ld.so` first to resolve shared libraries before the real program ever starts.

---

## What's next

`09_SYSTEM_CALLS.md` steps back from processes specifically to the mechanism every syscall in this file has been quietly relying on: the user-mode/kernel-mode boundary itself, and exactly what happens at the moment a program crosses it.