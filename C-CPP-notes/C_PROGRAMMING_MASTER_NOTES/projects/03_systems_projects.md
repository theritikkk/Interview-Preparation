# Systems Projects

> **Phase 12 · Projects & Interview Preparation — final projects file.** These nine
> projects need Phases 8–10 (file descriptors, the UNIX/POSIX interface, debugging
> tooling). Four of them — the mini shell, a `cat`-alike, a Unix pipeline, and a custom
> allocator — already have full, staged practice exercises earlier in the vault; they're
> given a shorter entry here with pointers back, so this file's depth goes toward the five
> genuinely new ones: directory listing, hexdump, a grep-like utility, a process launcher,
> and file-copy-via-syscalls.

---

## 1. Mini Shell — already built; extend it here

`[Cross-reference]` The core read-parse-`fork`-`execvp`-`waitpid` loop was built across
`systems/05_processes_fork_exec_wait.md` §7 and extended with pipelines in
`06_pipes_and_signals.md` §8 and signal handling in that same file's §8.4. **If you
haven't built this yet, do that first** — it's the vault's central systems-programming
capstone. Extensions once it exists:
- Built-in commands that *don't* fork (`cd`, `exit`) — these must run in the shell's own
  process, since a `cd` executed in a forked child would only change that child's working
  directory, not the shell's.
- I/O redirection (`>`, `<`, `2>`) via the `dup2` pattern from
  `systems/04_system_calls_vs_libc_vs_posix.md` §2.
- A simple job-control model (`&` for background jobs) using the `SIGCHLD`-reaping pattern
  from `06_pipes_and_signals.md` §4.

---

## 2. `cat`-Alike — already built; extend it here

`[Cross-reference]` Built twice over — once with `stdio`, once with raw file descriptors
— as `systems/03_file_descriptors_and_unix_io.md`'s phase project (§9). Extend it to accept
multiple filenames (concatenating each in sequence, matching real `cat`'s behavior) and a
`-n` flag to number output lines.

---

## 3. Unix Pipeline (`cmd1 | cmd2`) — already built; extend it here

`[Cross-reference]` Built in full in `systems/06_pipes_and_signals.md` §3 and §8. Extend it
to chain **three or more** commands (`cmd1 | cmd2 | cmd3`) — this requires generalizing
from one `pipe` to a loop creating `N-1` pipes and correctly `dup2`-ing and closing each
child's specific pair of ends, a genuinely good test of whether the two-command version was
understood structurally or just pattern-matched.

---

## 4. Custom Memory Allocator — already built; see `projects/02_intermediate_projects.md` §6

`[Cross-reference]` Covered there in full, including coalescing and defensive-check
extensions, to keep this file's remaining space for new material.

---

## 5. Directory Listing (`ls`-alike)

**Requirements:** list the names of every entry in a given directory (or the current
directory, if none given).

**Architecture:** `opendir` → loop on `readdir` → `closedir`.

```c
#include <dirent.h>
DIR *d = opendir(path);
struct dirent *entry;
while ((entry = readdir(d)) != NULL)
    printf("%s\n", entry->d_name);
closedir(d);
```

**Concepts exercised:** a new, small POSIX API (`<dirent.h>`), directly parallel in shape
to the file-descriptor APIs from `systems/03_file_descriptors_and_unix_io.md` — `opendir`
returns an opaque handle (`DIR *`, conceptually a sibling of a file descriptor), `readdir`
yields entries one at a time until `NULL`, `closedir` releases it. `[External, verified
against POSIX's own `<dirent.h>` specification]` One precise, real portability warning
worth internalizing here: **`d_name`'s array size is unspecified** — some implementations
declare it with a placeholder size of 1 and allocate the real entry larger than the struct
suggests. POSIX's own documentation states this directly: *"Use of `sizeof(d_name)` is
incorrect; use `strlen(d_name)` instead."* This is exactly
`memory/04_arrays_and_strings.md` §5's `sizeof`-vs-`strlen` distinction, resurfacing as a
real, named, standards-documented gotcha in a brand new API.

**Implementation strategy:** start with the loop above (everything, including `.` and
`..`); add filtering (skip entries starting with `.`, matching real `ls`'s default
behavior) once the basic listing works.

**⚠️ Pitfalls:**
- Using `sizeof(entry->d_name)` anywhere — per the warning above, this is specifically
  flagged as incorrect by POSIX itself.
- Not checking `opendir`'s return for `NULL` (nonexistent directory, permissions) before
  looping.

**Testing:** a directory with a handful of files; an empty directory (should list nothing
beyond `.`/`..`); a nonexistent path (confirm your error handling fires correctly).

**Extension ideas:** sort entries alphabetically before printing (read all entries into a
dynamic array, `projects/02_intermediate_projects.md` §1, then sort with `qsort`,
`libraries/02_stdlib.md` §3); add `-l`-style detail via `stat` (`[External]`, beyond this
vault's core scope).

---

## 6. Hexdump

**Requirements:** print a file's raw bytes in hexadecimal, alongside their ASCII
representation where printable — the classic `hexdump -C` / `xxd` layout:

```
00000000  48 65 6c 6c 6f 2c 20 77  6f 72 6c 64 21 0a        |Hello, world!.|
```

**Architecture:** read the file in fixed-size chunks (16 bytes per output line is the
conventional width); for each chunk, print the byte offset, each byte in hex, then the
same bytes again as characters (substituting `.` for anything non-printable).

**Concepts exercised:** this project is a direct, practical payoff of
`libraries/04_ctype_and_locale.md`'s `isprint` (deciding which bytes get their real
character shown vs. a `.` placeholder) and `fundamentals/02_types.md`'s representation
material — you are, quite literally, looking at a file's raw memory representation,
byte by byte, exactly what this vault's "What happens in memory" framing has been building
intuition for since Phase 1.

**Implementation strategy:** read with `fread` into a 16-byte buffer; for each byte, print
`%02x ` (zero-padded two-digit hex); after all 16 (or however many were actually read, for
a short final line), print the ASCII column, using `isprint((unsigned char) c) ? c : '.'`
— note the `unsigned char` cast, `libraries/04_ctype_and_locale.md` §2's exact warning,
directly relevant here since you're processing genuinely arbitrary binary bytes.

**⚠️ Pitfalls:**
- Forgetting the `unsigned char` cast before `isprint` — this project processes arbitrary
  binary data, making it a realistic case (not a contrived one) for that exact UB trap.
- Mishandling the final, short line (fewer than 16 bytes) — the hex column needs
  consistent padding so the ASCII column still aligns.

**Testing:** a short text file (confirm the ASCII column matches); a binary file (confirm
non-printable bytes show as `.`); a file whose length is an exact multiple of 16 (confirm
no spurious empty final line); an empty file.

**Extension ideas:** an offset-range option (`-s`/`-l` style, start offset and length);
a diff-mode comparing two files' hex dumps.

---

## 7. Grep-Like Utility

**Requirements:** print every line of a file that contains a given substring —
`mygrep pattern file.txt`.

**Architecture:** read the file line by line (`fgets`, `libraries/01_stdio.md`); for each
line, search for the pattern as a substring; print matching lines, optionally with their
line number.

**Concepts exercised:** `<string.h>`'s `strstr` (a function this vault hasn't built a
dedicated exercise around yet — worth implementing yourself first, since it's a direct,
approachable application of the `memory/04_arrays_and_strings.md` §6 pointer-marching
idioms: an outer loop over the text's start positions, an inner comparison against the
pattern at each); `fgets`'s own buffer-size-aware contract (unlike `gets`, which doesn't
bound its reads at all and is exactly why it was removed from the language in C11 —
`[External]`, worth knowing as a concrete "the standard library learns from its mistakes
too" data point alongside `advanced/04_modern_c_vs_historical_c.md`'s other deltas).

**Implementation strategy:** `fgets(line, sizeof line, fp)` in a loop; call your own
`strstr`-alike on each line; track and print a line counter alongside any match.

**⚠️ Pitfalls:**
- A fixed-size line buffer (`char line[256]`) silently truncating longer lines — `fgets`
  itself won't overflow the buffer (unlike `gets`), but a truncated line can still produce
  a false negative if the pattern happens to span the truncation point. A production-
  quality version would detect a missing trailing `\n` and read more.
- Off-by-one errors in the substring search's inner loop — classic territory for exactly
  the pointer-arithmetic care `memory/02_pointers.md` §4 emphasized.

**Testing:** a pattern that matches multiple lines; a pattern that matches nothing; a
pattern at the very start/end of a line; case sensitivity (document which your version
does, and consider a `-i` flag as an extension).

**Extension ideas:** basic regular-expression support (well beyond this vault's scope — a
good motivating look at why `<regex.h>`/POSIX regex functions exist at all); recursive
directory search, combining this project with §5's directory listing.

---

## 8. Process Launcher

**Requirements:** a program that takes a command and arguments, runs it as a child
process, and reports its exit status — essentially the non-interactive, single-command
core of §1's mini shell, useful as a standalone building block.

**Architecture:** `fork` → `execvp` in the child → `waitpid` in the parent → decode and
report the status via `WIFEXITED`/`WEXITSTATUS`/`WIFSIGNALED`/`WTERMSIG`
(`systems/05_processes_fork_exec_wait.md` §5).

**Concepts exercised:** this is close to a minimal, standalone version of
`systems/05_processes_fork_exec_wait.md` §7's shell loop, worth building **before** the
full mini shell specifically because it isolates the `fork`/`exec`/`wait` core from the
parsing/prompting logic around it.

**Implementation strategy:** `argv[1..]` (your launcher's own arguments, past the program
name) become the command and its arguments to `execvp` directly — no parsing needed, since
the shell that invoked *your* launcher already did the tokenizing.

**⚠️ Pitfalls:** the same ones already catalogued in
`systems/05_processes_fork_exec_wait.md` §8 — most commonly, forgetting that `execvp`
never returns on success, so any code after a successful `execvp` call is dead code by
construction, and any code reached after it indicates failure.

**Testing:** launch a command that succeeds (confirm exit code 0 reported correctly);
launch a command that exits with a specific nonzero code (confirm it's reported exactly);
launch a command that doesn't exist (confirm `execvp`'s failure is handled, not left to
crash); launch a command that segfaults (confirm `WIFSIGNALED`/`WTERMSIG` correctly report
`SIGSEGV`).

**Extension ideas:** a timeout (kill the child via `kill` if it doesn't finish within N
seconds — needs `alarm`/`SIGALRM`, `[External]`, a natural next POSIX topic beyond this
vault's core scope); capturing the child's output via a pipe instead of letting it inherit
the launcher's own stdout.

---

## 9. File Copy via Raw System Calls — already built; see `projects/01_beginner_projects.md` §3

`[Cross-reference]` The `stdio`-based version is there; its raw-`open`/`read`/`write`/
`close` sibling is `systems/03_file_descriptors_and_unix_io.md`'s own phase project (§9) —
build both and compare their speed on a large file directly, the single clearest personal
demonstration of this vault's buffering argument from Phase 8.

---

## 10. 🎯 What ties all nine of these together

`[Synthesis]` Every project in this file ultimately composes the same small set of
primitives this vault spent Phases 8–9 building: file descriptors, `fork`/`exec`/`wait`,
and (for several of them) pipes. **That's the actual lesson of "systems programming" as a
discipline** — not a large set of distinct techniques, but a small, composable set of
primitives, combined in different arrangements to solve different problems. A shell, a
pipeline, and a process launcher are not three unrelated programs; they're three different
compositions of the same four or five system calls.
