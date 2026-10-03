# Interview Questions — File I/O, Both Layers

> Consolidated from `systems/03_file_descriptors_and_unix_io.md`.

---

## Easy

1. **What does `open` return on success, and on failure?** A small non-negative file
   descriptor; `-1` on failure. (§3)
2. **What are file descriptors 0, 1, and 2, by convention?** Standard input, standard
   output, standard error. (§2)
3. **Are `open`/`read`/`write`/`close` part of the C standard?** No — POSIX, not ISO C
   (as already established in `libraries/01_stdio.md` §1, concretely revisited here).

## Medium

1. **Why might `read(fd, buf, 1024)` return fewer than 1024 bytes without that being an
   error?** `read` only guarantees returning *some* available data, not necessarily the
   full amount requested. (§3)
2. **Why does a `FILE` struct need to store a file descriptor as one of its own members?**
   `stdio`'s buffering layer is built on top of raw fd-based I/O; refilling/flushing the
   buffer requires the real fd to call `read`/`write` against. (§5)
3. **What's the practical difference between `open` and `creat`?** `open` accesses an
   existing file per the given flags; `creat` creates a new file or truncates an existing
   one to zero length. (§3)

## Hard

1. **Walk through exactly what happens, layer by layer, on a `getc(stdin)` call when the
   buffer is empty.** The macro's pointer-count check fails, calling `_fillbuf`, which
   (allocating a buffer on first use) calls `read` on `stdin`'s stored fd to refill the
   buffer, then returns the first character — only that one `read` touches the kernel;
   every other character in the refilled buffer comes from the cheap macro path. (§5)
2. **Why does casting to `unsigned char` matter in a raw `read`-based `getchar`
   implementation?** Without it, a signed `char` with its high bit set would sign-extend to
   a negative `int`, potentially colliding with `EOF`'s value. (§4)

## Systems-level

1. **A program using unbuffered, one-byte-at-a-time `read` calls is dramatically slower
   than the equivalent `stdio`-based version on the same file. Why, precisely?** The
   unbuffered version pays one system call's kernel-transition overhead per character; the
   `stdio` version pays it once per `BUFSIZ` characters — the number of system calls, not
   total bytes moved, dominates the cost difference. (§4)
2. **Two independently-written `<stdio.h>` implementations (Book B's and Book C's, from
   Phases 7–8) arrive at structurally identical `getc` macros. What does that convergence
   suggest?** It's evidence the "cheap pointer-check fast path, real-function fallback on
   buffer-empty" design isn't one author's idiosyncratic choice but close to the natural,
   necessary solution to the underlying problem (buffered I/O without per-character system-
   call overhead). (§5, cross-referencing `libraries/01_stdio.md` §3)
