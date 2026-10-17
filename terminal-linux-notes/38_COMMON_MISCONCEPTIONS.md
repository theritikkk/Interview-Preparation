# 38 — Common Misconceptions

This file does two things: corrects your original 14 misconceptions precisely, then curates roughly 20 more of the highest-value ones from across the whole handbook. It doesn't mechanically re-list every callout from all 37 prior files — that would be long and redundant with material already in context there. Each entry here points back to its file for the full reasoning.

---

## Your original list

| Misconception | Reality |
|---|---|
| "Terminal = shell" | Two separate processes, one the parent of the other (file 01). |
| "Shell = kernel" | The shell is an ordinary userspace program, no kernel privilege at all (file 03). |
| "`fork` creates an entirely independent process memory immediately" | Copy-on-write defers the actual copy until a write happens — often never, if `exec` follows soon after (file 08). |
| "`exec` creates a new process" | Replaces an *existing* process's image, in place — same PID throughout (file 08). |
| "Pipe is just a file" | An in-memory, bounded kernel object with real blocking semantics — no disk involved anywhere (file 13). |
| "`Ctrl+C` kills the terminal" | Sends `SIGINT` to the foreground process *group* of whatever's running inside it — the terminal emulator process itself is entirely unaffected (file 14). |
| "`cd` is an executable" | Must be a builtin — a forked child's `chdir()` could never affect the parent shell (file 07). |
| "stdout always means the screen" | Whatever fd 1 currently points at — a file, a pipe, anything (file 11). |
| "stdin always comes from the keyboard" | Whatever fd 0 currently points at (file 11). |
| "SSH is just remote terminal display" | Transports a byte stream; rendering is still done entirely by *your local* terminal emulator (file 26). |
| "tmux is another shell" | A multiplexer sitting between a terminal and one or more actual shells, not a shell itself (file 27). |
| "`sudo` is a shell" | An ordinary `setuid-root` executable (file 20). |
| "`PATH` is controlled by the kernel" | An ordinary environment variable — entirely a userspace/shell-level convention (files 07, 17). |
| "Environment variables are global variables" | Per-process, copied at `fork`, replaced entirely at `exec` — never live-shared between processes (file 17). |

---

## Twenty more, curated by area

### Terminal / TTY
| Misconception | Reality |
|---|---|
| "A PTY is basically just a pipe." | A pipe has no line discipline at all — no echo, no signal generation (files 02, 30). |
| "The shell explicitly prints every character you type." | The kernel's line discipline echoes automatically in canonical mode — the shell isn't involved until Enter (file 02). |

### Shell
| Misconception | Reality |
|---|---|
| "`#!/bin/sh` always runs Bash." | Runs whatever `/bin/sh` is symlinked to — `dash` on Debian/Ubuntu, not Bash (file 03). |
| "`[[ ]]` avoids a fork that `[ ]` doesn't." | Both are builtins in Bash — neither forks; `[[ ]]`'s real advantage is safer unquoted-variable handling (file 04). |
| "`$$` proves whether a fork happened." | `$$` is deliberately cached from the top-level shell and never changes in a subshell — use `$BASHPID` to see the real PID (file 04). |

### Parsing
| Misconception | Reality |
|---|---|
| "Word splitting happens on what I typed." | It happens on the *result* of expansion, in a later pipeline phase — your original quoting already fixed word boundaries earlier (file 06). |
| "An unmatched glob just disappears." | Bash's default is to leave it as the literal, unexpanded pattern text (file 06). |

### Processes & syscalls
| Misconception | Reality |
|---|---|
| "`fork` and `exec` are basically one operation." | Deliberately separate, specifically to leave a window for fd setup in between (file 08). |
| "System calls are slow from ordinary function-call overhead." | The real cost is the hardware privilege-level transition itself — exactly why `stdio` buffering exists (file 09). |
| "`dup2()` preserves the original fd's close-on-exec flag." | Always clears it on the duplicate — only `dup3()` with `O_CLOEXEC` can set it explicitly (file 10). |

### Redirection & pipes
| Misconception | Reality |
|---|---|
| "A program needs special code to support redirection." | Invisible to it entirely — the shell does all the work before the program ever starts (file 12). |
| "`cat` finishes, then `grep` starts." | Both run concurrently, streaming — contrast with `$(...)`'s run-to-completion-then-capture behavior (file 13). |

### Signals & job control
| Misconception | Reality |
|---|---|
| "`SIGSTOP` and `SIGTSTP` are the same signal." | `SIGTSTP` (`Ctrl+Z`) is catchable; `SIGSTOP` is unconditionally unblockable (file 14). |
| "`kill PID` force-terminates a process." | Sends `SIGTERM` by default — a catchable, ignorable request, not a command (file 14). |
| "`&` means 'run this faster.'" | Means "don't wait synchronously, don't take terminal foreground" — nothing about execution speed (file 15). |
| "'Foreground process group' is a loose descriptive phrase." | A literal, kernel-tracked value per terminal, read/set via `tcgetpgrp`/`tcsetpgrp` (file 16). |

### Memory & scheduling
| Misconception | Reality |
|---|---|
| "ASLR randomizes everything about memory layout." | Randomizes *base addresses* of major regions only — internal layout within one region stays fixed relative to its base (file 21). |
| "Each process gets its own CPU." | Only true if you have at least as many free cores as runnable processes — not a safe general assumption (file 22). |

### Terminal internals & rendering
| Misconception | Reality |
|---|---|
| "Bash's arrow-key history is the kernel's canonical mode being smart." | The opposite — Readline explicitly disables canonical mode and reimplements editing entirely itself (file 23). |
| "The kernel draws my screen." | True only for the Linux virtual console specifically — false for every PTY-based terminal, which is nearly all of them (file 24). |

### Containers & /proc
| Misconception | Reality |
|---|---|
| "Containers run their own kernel." | Share the host kernel entirely — the actual distinction from a VM (file 29). |
| "`/proc/self` refers to my shell." | Refers to whichever process performs the lookup — usually the command you just ran, not the shell that launched it (file 33). |

---

## Summary

Thirty-four misconceptions total, every one of them a genuine, recurring source of confusion rather than a strawman — many are repeated as established fact across tutorials and forum answers, which is exactly why they're worth having corrected precisely, with a file reference, rather than just asserted here in isolation.

---

## What's next

`39_MASTER_EXECUTION_TRACE.md` is the final synthesis: one command, `grep "hello" file.txt | sort > result.txt`, traced through literally every layer this handbook has covered, start to finish.
