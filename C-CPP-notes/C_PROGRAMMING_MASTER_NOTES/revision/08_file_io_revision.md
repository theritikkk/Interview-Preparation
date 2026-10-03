# Revision — Phase 8: File I/O, Both Layers

> Quick-reference companion to `systems/03_file_descriptors_and_unix_io.md`.

---

## 1-minute summary

A file descriptor is a small non-negative int the kernel uses to track an open file;
`open`/`read`/`write`/`close`/`lseek` are the five raw system calls operating on one — none
of them part of ISO C, all of them POSIX. `read`/`write` may transfer fewer bytes than
requested without that being an error. `FILE*` is a buffering layer built *on top of* a
file descriptor: a real, working `fopen`/`getc` implementation shows a `FILE` struct
literally storing an fd as one of its members, with exactly one line — `_fillbuf`'s
`read(fp->fd, ...)` call — where the two layers actually meet.

### 5-minute revision
- Standard descriptors 0/1/2 (stdin/stdout/stderr) are set up by the shell before your
  program runs; redirection changes what they point to without your program's knowledge.
- An unbuffered `getchar` built directly on `read` calls the kernel once per character; a
  buffered version calls it once per `BUFSIZ` characters — the exact, concrete reason
  `stdio`'s buffering exists at all (`libraries/01_stdio.md`'s claim, made empirical here).
- `getc`'s macro fast path (`--(p)->cnt >= 0 ? ... : _fillbuf(p)`) is structurally identical
  to Plauger's independent `<stdio.h>` implementation from Phase 7 — two authors, same
  design, for the same reason.
- `fopen` doesn't allocate a buffer itself — that's deferred to the first real `_fillbuf`
  call, the same "pay only when needed" laziness as heap growth via `sbrk`.

**Worth memorizing:** which five functions are the raw POSIX layer (`open`/`read`/`write`/
`close`/`lseek`) versus ISO C's `stdio` layer — this distinction gets tested constantly.
**Worth understanding, not memorizing:** tracing a single `getc(stdin)` call, buffer-empty
case, all the way down to its one `read` system call (file_descriptors.md §5) — you should
be able to narrate this from memory, not just recall that "buffering happens."

---

## Cross-topic traps worth drilling before an interview

1. Assuming `read(fd, buf, n)` always fills `buf` with exactly `n` bytes — it may return
   fewer, with no error.
2. Treating `FILE*` and file descriptors as interchangeable terms for "an open file" rather
   than two genuinely different layers, one built on the other.
3. A raw, `read`-based `getchar` returning a plain (possibly signed) `char` without an
   `unsigned char` cast — sign-extension can produce a value that collides with or
   misrepresents `EOF`.
4. Forgetting that `creat`/`open`-with-`O_WRONLY` truncate or require pre-existing files
   respectively — mixing up which call is for creating versus opening.
