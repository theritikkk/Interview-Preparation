# C Language vs. C Library vs. POSIX vs. Linux System Calls

> **Phase 9 · UNIX/POSIX System Interface — the brief's heaviest external-synthesis load.**
> Depends on: `systems/03_file_descriptors_and_unix_io.md` (file descriptors — POSIX's
> foundational abstraction), `libraries/10_limits_float_stddef.md` §1 (freestanding vs.
> hosted — the C-language/C-library boundary this file extends).
> Feeds into: `05_processes_fork_exec_wait.md`, `06_pipes_and_signals.md` (this phase).
> Sources: **Book A** §20.1/20.5/20.6 (the C standard library's real history, and its own
> direct discussion of where POSIX functions like `fork` actually live). **Everything about
> `fork`/`exec`/pipes/process control is entirely `[External]`** — none of the three books
> covers it at all; this phase exists specifically to fill that gap, using vetted external
> sources (man7.org, POSIX itself) throughout.

---

## 1. Four layers, four different standards bodies

`[Synthesis, organizing this vault's scattered pieces of this distinction — Phase 1's
program-structure discussion, Phase 7's stdio design history, and Phase 7's freestanding/
hosted split — into one explicit model]` This is the distinction the whole brief insists on
keeping sharp, stated as directly as possible:

| Layer | Governed by | Example | Exists on Windows? |
|---|---|---|---|
| **The C language** | ISO/IEC C standard (WG14) | `if`, `struct`, pointer arithmetic | Yes — the language itself is platform-agnostic |
| **The C standard library** | Same ISO C standard | `printf`, `malloc`, `strlen` | Yes — required by *any* hosted implementation |
| **POSIX** | The Open Group / IEEE 1003.1 — a *different* standard | `fork`, `pipe`, `dup2`, `read`/`write`/`open` (`systems/03…md`) | Only via a compatibility layer (Cygwin, WSL) — not natively |
| **Linux-specific kernel syscalls** | The Linux kernel itself — not a standards body at all | The real `clone`/`execve` entry points glibc's `fork`/`exec` wrappers ultimately invoke | N/A — this layer *is* Linux |

**Why this matters practically, not just terminologically:** code using only the top two
layers is portable to *any* conforming C implementation, anywhere. Code using POSIX
functions is portable to any POSIX-conformant system (Linux, macOS, BSD, and — historically
— genuine UNIX) but **not** portable to native Windows without an explicit compatibility
layer. Code that assumes a *specific* Linux kernel syscall's exact behavior is the least
portable of all — not even guaranteed across other POSIX systems, let alone other OS
kernels.

### 🧠 A concrete illustration, straight from Book A

`[Book-derived, A]` glibc (the GNU C Library — Linux's most common C standard library
implementation) implements `fork` **inside `libc.so`** — the same shared library that
provides `printf` and `malloc` — even though `fork` is a POSIX function, not an ISO C one.
Historically, some POSIX functionality (thread-related functions, before NPTL was merged
into glibc) lived in a genuinely **separate** library with its own link flag
(`-lpthread`), only later folded into the main C library. `[Synthesis]` This is worth
sitting with: **"is it part of libc" is a packaging/distribution detail, not a reliable
signal for "is it part of the C standard."** The only reliable way to know is checking
*which standard* actually documents a given function — ISO C, or POSIX.

### 🎯 Interview point
"Is `fork` a C function?" — It's a function *callable from* C, declared in a header
(`<unistd.h>`) and commonly implemented inside the same shared library as genuine C
standard library functions — but it is a **POSIX** function, not part of the ISO C standard
at all. The distinction matters the moment you (or your build) targets a non-POSIX platform.

---

## 2. `dup`/`dup2` — POSIX's file-descriptor duplication primitive

`[External]` Building directly on `systems/03_file_descriptors_and_unix_io.md`'s file-
descriptor model:

```c
#include <unistd.h>
int dup(int oldfd);              /* returns the LOWEST unused fd, now referring to the
                                     same open file description as oldfd */
int dup2(int oldfd, int newfd);   /* makes newfd refer to the same open file description
                                     as oldfd, closing newfd first if it was already open */
```

**The key concept: two file descriptors can refer to the *same underlying open file*,**
sharing the same current position and status — not two independent copies. This is exactly
the mechanism behind shell I/O redirection: when a shell runs `prog > output.txt`, it
(conceptually) opens `output.txt`, then uses `dup2` to make file descriptor `1` (standard
output) refer to that same open file, **before** calling `exec` to run `prog` — `prog`
itself writes to fd `1` exactly as always, with no idea its output is now going to a file
instead of a terminal, precisely the "the program doesn't know or care" design point from
`systems/03_file_descriptors_and_unix_io.md` §2.

### Worked example — the classic `2>&1` pattern, at the `dup2` level

```c
dup2(1, 2);   /* make fd 2 (stderr) refer to whatever fd 1 (stdout) currently refers to —
                 exactly what the shell's "2>&1" redirection means */
```

`[Synthesis]` This is worth internalizing precisely because `2>&1`'s shell syntax is
famously confusing to newcomers — seeing it as "just a `dup2` call" removes the mystery:
it's not doing anything magical, just pointing one file descriptor at whatever another one
currently references.

### 🎯 Interview point
"Why does `newfd` get closed first inside `dup2`, if it was already open?" — Because a file
descriptor number can only refer to one open file at a time; making `newfd` refer to
`oldfd`'s file necessarily means it can no longer refer to whatever it referred to before —
`dup2` handles that bookkeeping atomically so you don't need a separate explicit `close`
call (and, avoiding a real race condition a manual close-then-dup could introduce).

---

## 3. The environment: `envp`, `environ`, and `getenv` revisited

`[Cross-reference, connecting `libraries/02_stdlib.md` §6's `getenv` and
`systems/02_linking_and_loading.md` §6's `envp` mention]` `main` is commonly given a third,
non-ISO-C parameter: `int main(int argc, char *argv[], char *envp[])` — an array of
`"NAME=value"` strings, one per environment variable, terminated by a `NULL` pointer
(exactly the same "array of pointers with a null sentinel" pattern as `argv` itself,
`memory/04_arrays_and_strings.md` §7). `[External]` This third parameter is a common
extension, not guaranteed by ISO C; the fully portable (POSIX-guaranteed) way to access the
same data from *anywhere* in a program, not just `main`, is the global `extern char
**environ;` variable — `getenv` (`libraries/02_stdlib.md` §6) is simply a convenience
search over exactly this same array.

---

## 4. ⚠️ Misconceptions to unlearn now

- **"If a function is declared in a standard-looking header and lives in `libc`, it's part
  of the C standard."** Not necessarily — POSIX functions (`fork`, `pipe`, `read`) are
  routinely packaged in the same library as genuine C standard functions (§1).
- **"POSIX code is exactly as portable as ISO C code."** It's portable across POSIX-
  conformant systems, which is a real but *narrower* guarantee than ISO C's — native
  Windows is the everyday counterexample (§1).
- **"`dup2` just copies a file descriptor's current position, like a snapshot."** It makes
  the two descriptors share the *same* underlying open file — a write through either one
  advances the position both descriptors observe, not an independent copy (§2).

---

## 5. 🎯 Interview questions

**Easy**
- Is `fork` part of the ISO C standard? → No — it's a POSIX function (§1).
- What does `dup2(a, b)` do, at a high level? → Makes file descriptor `b` refer to the same
  open file as `a`, closing `b` first if needed (§2).

**Medium**
- Why can code using only ISO C run natively on Windows, while code calling `fork` cannot?
  → ISO C's language and standard library are platform-agnostic requirements any hosted
  implementation must provide; `fork` is POSIX-specific, and Windows isn't natively
  POSIX-conformant (§1).
- How does a shell implement `prog > output.txt` in terms of file descriptors? → It opens
  `output.txt`, then uses `dup2` to make fd 1 refer to that same open file, before calling
  `exec` to run `prog` (§2).

**Hard**
- Explain precisely why "this function lives in `libc.so`" is not a reliable test for "this
  function is part of the C standard," using a concrete example. → glibc implements `fork`
  (a POSIX function) inside the same shared library as genuine ISO C functions like
  `printf`; some POSIX functionality has even moved between separate and merged libraries
  over time (pre-NPTL threading) — packaging is a distribution decision, not a standards
  classification (§1).
- Why does `dup2` need to atomically close its target descriptor rather than requiring the
  caller to `close` it first? → A separate `close` then `dup` sequence has a race window
  where the descriptor number is free and could be reused by an unrelated, concurrently-
  running part of the program (e.g., another thread opening a file) before the intended
  `dup` claims it — `dup2`'s atomicity closes that window (§2).

**Systems-level**
- A program behaves correctly on Linux and macOS but fails to compile on Windows because it
  calls `fork`. Where exactly does the portability boundary lie, and what would the fix
  involve? → The boundary is the POSIX layer specifically — Windows doesn't natively
  implement it; a fix requires either a POSIX compatibility layer (WSL, Cygwin) or
  replacing the POSIX-specific process-creation logic with the Windows-native equivalent
  (`CreateProcess`, an entirely different, non-portable API) (§1).

---

## 6. External references

- [The Open Group Base Specifications, Issue 8 (POSIX.1-2024)](https://pubs.opengroup.org/onlinepubs/9799919799/)
  `[External]` — the actual current POSIX standard text for `fork`, `dup`/`dup2`, and
  everything else this phase covers.
- [man7.org — `dup(2)`](https://man7.org/linux/man-pages/) `[External]` — current, precise
  `dup`/`dup2`/`dup3` semantics on Linux specifically.
- [man7.org — `environ(7)`](https://man7.org/linux/man-pages/) `[External]` — the
  `environ` global variable and environment-array conventions in full.

## 7. 🛠 Practice

1. Write a small program that opens a file, then uses `dup2` to redirect its own standard
   output to that file *before* calling `printf` — confirm the output lands in the file,
   not the terminal, without `printf` itself changing at all.
2. Print `environ` (or the third `main` parameter, if your platform supports it) to see the
   full environment as a `NULL`-terminated array of strings, then confirm `getenv` is
   findable as "just a search over this same array" by implementing your own `my_getenv`
   using `environ` directly.
3. Reproduce the `2>&1` pattern from §2 both from the shell (`prog 2>&1`) and via an
   explicit `dup2(1, 2)` call inside a small C program, and confirm they produce
   observably identical results.
