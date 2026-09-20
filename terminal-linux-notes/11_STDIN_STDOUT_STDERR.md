# 11 — stdin, stdout, stderr

> **Where this fits in the stack**
> ```
> User  →  Terminal Emulator  →  PTY/TTY  →  Shell  →  Processes  →  Syscalls  →  File Descriptors  →  [ 0 / 1 / 2 ]
>                                                                                                       ^^^^^^^^^^^^
>                                                                                                       this file
> ```
> File 10 established fd 0/1/2 as convention and gave you the exact mechanics `dup2()` uses to repoint a file descriptor. This file applies that machinery specifically to *why stderr is separate at all*, and to the single most commonly misunderstood piece of shell syntax: `2>&1`.

---

## 1. Why does stderr exist as a genuinely separate channel?

The core reason: a program's **actual output data** (meant to be consumed — by a human, or by the next program in a pipeline) needs to stay separate from its **diagnostic messages** (meant for the human operator, never meant to be fed into anything downstream).

Consider `grep hello file.txt | sort`. If `grep` hits a permission error and that message went out on the *same* channel as its normal matching-line output, the text `"file.txt: Permission denied"` would flow straight into `sort` as if it were data to be sorted — silently corrupting the pipeline's real output with what should have been operator-facing noise. Keeping stderr **separate** — and, critically, **excluded from the pipe** when you build a pipeline with `|` — means error messages bypass the pipeline entirely and reach the terminal directly, while stdout's actual data flows cleanly downstream. This is exactly why `cmd1 | cmd2` only ever redirects stdout into the pipe (full pipeline wiring: file 13).

> **Under the hood**
> stdout is typically **line-buffered** when connected to a terminal and **fully block-buffered** when redirected to a file or pipe (block buffering being more efficient for bulk output); stderr is traditionally **unbuffered**, precisely so a diagnostic message — especially one printed right before a crash — actually reaches you instead of being lost in a buffer that never gets flushed. This is the exact `printf`-buffering mechanism from file 09, applied deliberately differently depending on which stream you're writing to.

---

## 2. Tracing each redirection, precisely

### `echo hello`

fd 1 still points wherever it was inherited from — the terminal, via the PTY. `hello` appears on screen.

### `echo hello > output.txt`

Before `echo` ever runs, the **shell** opens `output.txt` (creating or truncating it) and calls `dup2(new_fd, 1)`, repointing fd 1 at that file's open file description. `echo` then writes to what it believes is "stdout," landing in `output.txt`. Nothing appears on screen. **fd 2 is untouched** — still connected to the terminal, so any error `echo` produced would still be visible.

### `echo hello 2> error.txt`

The shell instead opens `error.txt` and `dup2`s **fd 2** to point at it. fd 1 is untouched, so `hello` still appears on screen as normal. Since a literal `echo hello` essentially never errors, `error.txt` ends up **empty** — worth stating explicitly, since it's easy to expect *something* to land there and be confused when nothing does.

### `command >out 2>err`

Two entirely separate redirections, applied in order: fd 1 → a new open file description for `out`; fd 2 → a **different**, new open file description for `err`. Two independent files, two independent offsets (file 10's Scenario A) — nothing shared between them.

### `command >out 2>&1` — in detail

This is the one worth slowing all the way down for.

**`2>&1` means:** "make fd 2 a duplicate of *whatever fd 1 currently points at*" — a `dup2(1, 2)`, run by the shell, at the exact moment this redirection is processed. Bash processes redirections **left to right**, and each one acts on the fd table's **current state at that point** — not the state before the command line started.

So, for `command >out 2>&1`:
1. **`>out`** runs first: fd 1 is repointed at a new open file description for `out`.
2. **`2>&1`** runs second: fd 2 is made to duplicate whatever fd 1 is *right now* — which is `out`'s open file description, because step 1 already happened.

**Result: fd 1 and fd 2 end up pointing at the exact same open file description** (file 10's Scenario B — shared offset), so their writes correctly interleave in true write-order into one file, with no risk of one overwriting the other, because there's only one offset advancing.

**Now reverse the order: `command 2>&1 >out`**
1. **`2>&1`** runs first: fd 2 is made to duplicate whatever fd 1 is *right now* — which, at this point, is still the **original** destination (the terminal), because no redirection has happened yet.
2. **`>out`** runs second: fd 1 is repointed at a new open file description for `out`.

**Result: stdout goes to `out`. Stderr still goes to the terminal** — because it captured a copy of wherever fd 1 was *before* the `>out` redirection ever happened, and reassigning fd 1 afterward has no retroactive effect on a duplicate fd 2 already made.

```
command >out 2>&1          command 2>&1 >out

  fd1 → out                  fd1 → terminal (still, at this instant)
  fd2 → (dup of fd1) → out    fd2 → (dup of fd1 AT THIS MOMENT) → terminal
                              fd1 → out                                   (this line runs AFTER, too late for fd2)

  BOTH end up in "out"        stdout → out, stderr → STILL the terminal
```

> **Interview answer**
> "`2>&1` duplicates whatever fd 1 currently points to onto fd 2, at the moment it's processed — not some fixed final destination. Since Bash processes redirections left to right, `>out 2>&1` sends both streams to `out`, because fd 1 was already pointed there first. `2>&1 >out` does something different: stderr duplicates fd 1 while it's still the terminal, and only *then* does fd 1 get moved to `out` — so stderr is left behind on the terminal."

> **Common mistake**
> "`2>&1` always means 'send stderr and stdout to the same place,' regardless of where it appears in the command." The mechanism is order-dependent by nature — it's a snapshot of fd 1's *current* target, taken at that exact point in left-to-right processing, not a standing rule applied at the end.

---

## 3. Common misconceptions

| Misconception | Reality |
|---|---|
| "`2>&1`'s position in the command doesn't matter." | It matters enormously — Section 2's side-by-side shows two genuinely different outcomes from the same two redirections in opposite order. |
| "stderr is just a less-important duplicate of stdout." | It's a deliberately separate channel specifically so error/diagnostic text never contaminates a pipeline's actual data. |
| "If I don't see error output, nothing went wrong." | Or stderr was redirected somewhere you're not currently looking — a real, common debugging trap, expanded on in file 37. |

---

## 4. Try this

> **Try this**
> ```bash
> (echo out; echo err >&2) > both.txt 2>&1
> cat both.txt        # both lines present, correctly interleaved
> ```

> **Try this — proving the order dependence yourself**
> ```bash
> (echo out; echo err >&2) 2>&1 > both2.txt
> cat both2.txt        # only "out" — "err" went to your terminal, not the file
> ```
> Watch `err` print directly to your screen *while the command runs*, rather than ending up in `both2.txt` — the live, observable version of Section 2's trace.

---

## 5. Summary

- stdin/stdout/stderr are convention, not kernel-enforced meaning — but universally relied upon.
- stderr is kept separate specifically so diagnostic messages never corrupt pipeline data, and pipelines only ever redirect stdout by default.
- `N>file` and `2>&1` are both just the shell calling `open`+`dup2` on your behalf, before `exec`, exactly per file 10's mechanics.
- `2>&1`'s effect depends entirely on **where fd 1 currently points at the moment it runs** — which is why its position in the command line changes the outcome.

---

## What's next

`12_REDIRECTION.md` widens out from `2>&1` specifically to the complete redirection operator set — `>`, `>>`, `<`, `2>>`, `&>`, here-documents — all built on exactly the `open`+`dup2`-before-`exec` mechanism this file just walked through in detail.