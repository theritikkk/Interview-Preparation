# 12 — Redirection

> **Where this fits in the stack**
> ```
> User  →  Terminal Emulator  →  PTY/TTY  →  Shell  →  [ REDIRECTION ]  →  fork/exec  →  Programs
>                                                        ^^^^^^^^^^^^^^^
>                                                        this file
> ```
> File 11 used `dup2()` to explain `2>&1` specifically. This file widens out to the complete redirection operator set — and makes sure one point lands with full weight, because everything else in this file is really just a variation on it.

---

## 1. The critical point: redirection happens entirely in the shell, before `exec`

```
shell
 ↓
open output.txt
 ↓
dup2(fd, STDOUT_FILENO)
 ↓
fork/exec
 ↓
ls writes to fd 1
 ↓
file
```

For `ls > output.txt`: the shell opens `output.txt` and calls `dup2()` to repoint fd 1 at it — **entirely within its own process**, before `fork()` even runs. By the time `ls`'s image is loaded and its `main()` starts, fd 1 *already* points at the file. `ls` itself never opens `output.txt`, never calls `dup2()`, and has **no way to tell** whether fd 1 is a terminal, a file, or a pipe — it just writes to "fd 1" exactly as it always does.

> **Remember**
> **Redirection is invisible to the program being redirected.** No program needs to be specially written to "support" being redirected — there's nothing for it to support. This single fact is the entire reason essentially every Unix program works correctly with `>`, `<`, and `|` without a single line of redirection-aware code in any of them: the shell did all the work before the program ever started running.

---

## 2. The full operator table

| Operator | Effect |
|---|---|
| `> file` | Redirect stdout, **truncating** the file (or creating it) |
| `>> file` | Redirect stdout, **appending** to an existing file (creating it if needed) |
| `< file` | Redirect stdin, opening the file for reading |
| `2> file` | Redirect stderr, truncating |
| `2>> file` | Redirect stderr, appending |
| `2>&1` | Duplicate fd 2 from wherever fd 1 currently points (file 11, in full) |
| `&> file` | **Bash-specific**: redirect both stdout and stderr to `file` in one step |
| `<<<` | Here-string — Section 3 |
| `<<` / `<<EOF` | Here-document — Section 3 |

> **Under the hood — appending is atomic**
> `>>` sets the `O_APPEND` flag on the underlying open file description (file 10), which means every single `write()` automatically seeks to the current end-of-file **atomically**, immediately before writing. This is exactly what makes it safe for multiple processes to append to the same log file concurrently without a race condition clobbering each other's lines — a plain "seek to end, then write" done as two separate steps wouldn't have that guarantee.

**`&>` vs `>file 2>&1`**: functionally similar in modern Bash, but `&>` is a single, dedicated operator that points both streams at the same new destination directly — it **sidesteps the order-dependence trap from file 11 entirely**, since there's no separate left-to-right sequence of two operations to get wrong. If you want "both streams, one file" and don't need the flexibility of doing it manually, `&>` is the safer choice specifically because there's no ordering to reason about.

---

## 3. Here-documents and here-strings

```bash
cat <<< "hello"
```
A **here-string**: feeds a single string as stdin. The shell arranges for that exact text (plus a trailing newline) to be readable from stdin — typically via a temporary file or an anonymous pipe under the hood — without you needing to create a file yourself.

```bash
cat <<EOF
line one
line two
EOF
```
A **here-document**: the shell reads every line of your *own* input following this point, up until it sees a line containing **exactly** the delimiter word — here, `EOF` — and feeds everything in between as stdin to the command. `EOF` is a pure **convention**, not special syntax; `<<DONE ... DONE` works identically.

**Quoting the delimiter changes what happens inside:**

```bash
cat <<EOF
Hello $USER
EOF
# → expands $USER, exactly like double-quoted text

cat <<'EOF'
Hello $USER
EOF
# → prints the literal text "Hello $USER" — no expansion at all
```

An **unquoted** delimiter allows variable/command/arithmetic expansion inside the here-document body — exactly the double-quote rules from file 06. A **quoted** delimiter (`<<'EOF'` or `<<"EOF"`) suppresses all of it, treating the body as fully literal text, exactly like single quotes.

**`<<-` variant**: strips **leading tab characters** (not spaces) from each line before feeding it in — specifically so a here-document can stay visually indented inside an indented block of a script without that indentation ending up in the actual content.

---

## 4. Redirection order, generalized

File 11 worked through `2>&1`'s order-dependence in detail. The general rule covers **any** sequence of redirections on one command line: they're applied strictly **left to right**, and each one acts on the fd table's state **at that exact point** — not the state before the line started, and not some final combined result computed all at once.

**A related, easy-to-miss gotcha:**

```bash
ls > file1.txt > file2.txt
```

Only `file2.txt` ends up with `ls`'s output — the second redirection simply repoints fd 1 again, overwriting the first assignment. But `file1.txt` was still **opened and truncated** in the process of getting there. If it already existed with content, that content is gone — even though nothing was ever actually written to it.

---

## 5. Process substitution — `<(...)` and `>(...)`

A Bash/Zsh/ksh extension (recap: file 03's POSIX-extension table), completely absent from POSIX `sh`, that directly extends this file's `dup2`/pipe machinery into something you can hand to a command **as an ordinary filename argument** — not a redirect of the command's own stdin/stdout.

```bash
diff <(sort file1.txt) <(sort file2.txt)
```

`diff` has no built-in concept of "compare the output of two commands" — it only knows how to compare two **file paths**. `<(sort file1.txt)` runs `sort file1.txt` in the background, connects its output to a pipe, and hands `diff` something that *looks like* a file path (typically `/dev/fd/63`-style on Linux) but is actually the live read end of that pipe. `diff` opens it, reads from it, and has no idea it isn't reading an ordinary file.

```bash
some_command | tee >(process_a) >(process_b)
```

`>(...)` is the mirror image: it produces a path that, when **written to**, feeds that data as input to the named command — letting `tee` fan one stream out to two independent processes at once.

**Why this earns a place in the redirection file specifically:** ordinary redirection (`>`, `<`) repoints the fds of the command it's attached *to*. Process substitution instead produces a **standalone path-like token** usable anywhere an argument is expected — a materially different, and more flexible, extension of the exact same underlying fd/pipe mechanism.

---

## 6. Common misconceptions

| Misconception | Reality |
|---|---|
| "A program needs to be written to support redirection." | It's invisible to the program entirely — the shell does all the work before the program even starts (Section 1). |
| "`> file1 > file2` leaves `file1` completely untouched." | `file1` is still opened and truncated on the way to being overridden by `file2` — any prior content in it is gone. |
| "The here-document delimiter must be `EOF`." | Pure convention — any word works identically. |
| "`<(...)` redirects the outer command's stdin." | It produces a path-like *argument*, usable anywhere a filename is expected — a different mechanism from redirecting the command's own streams. |

---

## 7. Try this

> **Try this**
> ```bash
> echo old > file1.txt
> ls > file1.txt > file2.txt
> cat file1.txt      # empty — truncated, then overridden
> cat file2.txt       # has ls's output
> ```

> **Try this**
> ```bash
> diff <(echo a) <(echo b)
> ```
> A minimal, safe process-substitution demo — watch `diff` report a difference between two things that were never actual files on disk.

---

## 8. Summary

- Redirection is set up entirely inside the shell's own process, via `open` + `dup2`, **before** `fork`/`exec` — the program being redirected is never aware it happened.
- `>`/`>>`/`<`/`2>`/`2>>` cover truncate/append/read for each stream; `&>` sidesteps `2>&1`'s order-dependence for the common "both streams, one file" case.
- Here-documents feed multi-line input; quoting the delimiter is what decides whether the content inside gets expanded.
- Redirections on one line apply strictly left to right, each acting on the fd table's *current* state.
- Process substitution (`<(...)`/`>(...)`) is Bash-family-specific and produces an argument-usable path backed by a live pipe — a different shape of tool than ordinary stream redirection.

---

## What's next

`13_PIPES.md` takes the pipe mechanism process substitution just leaned on and goes fully explicit: `pipe()`, the fork+`dup2`+`close` dance that wires two processes together, and why both sides genuinely run concurrently.
