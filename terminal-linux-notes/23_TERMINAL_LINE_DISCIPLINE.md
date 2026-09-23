# 23 — Terminal Line Discipline

> **Where this fits in the stack**
> ```
> User  →  Terminal Emulator  →  PTY/TTY  →  [ LINE DISCIPLINE / READLINE ]  →  Shell
>                                              ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
>                                              this file
> ```
> File 02 introduced canonical mode, raw mode, and kernel-side echo, and flagged one open question every time: *if canonical mode buffers whole lines, how does Bash give you arrow-key history editing before you've even pressed Enter?* This file finally answers it — and gives the full `termios` API, a working raw-mode C program, and the precise mechanics of `Ctrl+D`, which neither file 02 nor file 14 fully covered.

---

## 1. Recap from file 02

Canonical mode: the kernel buffers and edits a full line before a process sees it. Raw mode: bytes are delivered essentially immediately, with the reading process responsible for everything. Echo is a kernel-side line-discipline feature, not something the shell does. All established — this file goes deeper into the exact API and the one behavior file 02 didn't cover: Readline overriding all of this by default in an interactive shell.

---

## 2. The `termios` struct, in full

```c
struct termios {
    tcflag_t c_iflag;   /* INPUT processing flags  */
    tcflag_t c_oflag;   /* OUTPUT processing flags */
    tcflag_t c_cflag;   /* CONTROL flags            */
    tcflag_t c_lflag;   /* LOCAL flags — canonical mode, echo, signals */
    cc_t     c_cc[NCCS]; /* special control characters */
};
```

- **`c_iflag`** — input processing, e.g. `ICRNL` (translate carriage return to newline on input).
- **`c_oflag`** — output processing, e.g. `OPOST` (enable output post-processing at all).
- **`c_cflag`** — control settings: baud rate, character size — historically meaningful for real serial lines, still present and consulted even though a PTY isn't a physical wire.
- **`c_lflag`** — local flags: `ICANON`, `ECHO`, `ISIG` (file 02).
- **`c_cc[]`** — the array of special control characters, expanded fully in Section 4.

---

## 3. `tcgetattr()` / `tcsetattr()` — the actual API

```c
int tcgetattr(int fd, struct termios *termios_p);
int tcsetattr(int fd, int optional_actions, const struct termios *termios_p);
```

`tcgetattr` reads the current settings for a terminal fd into a struct. `tcsetattr` writes new ones — `optional_actions` controls exactly when:

| Value | Meaning |
|---|---|
| `TCSANOW` | Apply immediately |
| `TCSADRAIN` | Wait for all pending output to finish, then apply |
| `TCSAFLUSH` | Discard any pending, not-yet-read **input** too, then apply |

`TCSAFLUSH` is the one almost always used when switching into raw mode specifically — you don't want keystrokes typed *before* the mode switch to suddenly get delivered and misinterpreted *after* it.

---

## 4. Special characters, completed — and `Ctrl+D` done properly

| Character | Default key | Effect |
|---|---|---|
| `VINTR` | `Ctrl+C` | `SIGINT` (file 14) |
| `VQUIT` | `Ctrl+\` | `SIGQUIT` — a *third* keyboard-generated signal, with a different default than `SIGINT`: terminate **and dump core**, rather than a plain exit |
| `VSUSP` | `Ctrl+Z` | `SIGTSTP` (file 14) |
| `VERASE` | Backspace | Kernel-side line editing — erase one character (file 02) |
| `VKILL` | `Ctrl+U` (commonly) | Kernel-side — erase the **entire** current input line, canonical mode only |
| `VEOF` | `Ctrl+D` | Not a signal at all — Section below |

**`VEOF`/`Ctrl+D`, precisely:** this is fundamentally different in kind from `VINTR`/`VQUIT`/`VSUSP` — it generates **no signal whatsoever**. It's purely a canonical-mode line-discipline behavior: when the line discipline sees this character, it **immediately** delivers whatever's been typed so far on the current line to the reading process's `read()` call — without waiting for a newline — and the `VEOF` character itself is **not** included in the delivered bytes (unlike a newline, which is).

**If pressed on a completely empty line:** there's nothing to deliver, so `read()` returns **0 bytes**.

> **Remember**
> This is exactly file 13's pipe-EOF mechanism, triggered by a keyboard character instead of a closed pipe. At the `read()` level, **`Ctrl+D` on an empty line and a pipe's natural EOF are the indistinguishable same event** — a successful `read()` returning zero bytes. The only difference is what caused it: a special input character, versus every write end of a pipe being closed. This is precisely why `Ctrl+D` on an empty shell prompt exits your shell — its own `read()` of the next command line gets EOF, and the shell interprets "nothing more is coming on stdin" as "time to exit," exactly as it would if its stdin were a pipe whose writer had gone away.

---

## 5. A complete raw-mode program

```c
#include <termios.h>
#include <unistd.h>
#include <stdio.h>

int main() {
    struct termios orig, raw;
    tcgetattr(STDIN_FILENO, &orig);            /* save the original settings */
    raw = orig;
    raw.c_lflag &= ~(ICANON | ECHO);            /* turn off canonical mode AND echo */
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);    /* apply now, discard pending input */

    printf("Raw mode on. Press keys (q to quit):\r\n");
    char c;
    while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
        printf("You pressed: %c (0x%02x)\r\n", c, c);
    }

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);   /* ALWAYS restore before exiting */
    return 0;
}
```

> **Common mistake**
> Forgetting the final `tcsetattr` restore — or having the program crash or get killed before it runs — leaves the **user's actual terminal** stuck without canonical-mode line editing or echo, since those settings persist on the terminal device itself, not inside your program. The visible symptom: your shell stops echoing what you type and stops responding to Backspace normally, until you run `stty sane` or `reset`. This is exactly why well-behaved raw-mode programs register `atexit()` handlers and signal handlers specifically to guarantee this restoration runs on every exit path, not just the clean one.

---

## 6. Readline — closing the gap from file 02

**What it is:** a GNU library (`libreadline`) providing sophisticated line editing — arrow-key history browsing, `Ctrl+A`/`Ctrl+E` (start/end of line), `Ctrl+R` reverse history search, Tab completion, Emacs-style kill-ring/yank — to **any** program that links against it, not just Bash. `psql`, `gdb`, Python's classic REPL, and many other interactive tools use it (or its equivalent) for exactly the same reason Bash does.

**How it actually works:** Readline puts the terminal into its **own** semi-raw mode via `tcsetattr()` — canonical mode off, kernel echo off — and reads input **character by character** itself, maintaining its **own internal model** of the current line being edited and where the cursor sits within it. Every time something changes, Readline **explicitly, manually** rewrites the visible line using escape sequences (file 24) to reposition the cursor and redraw. Pressing the left arrow doesn't move any cursor concept the kernel knows about — it updates Readline's own position counter, and Readline itself issues the escape sequence that visually moves your terminal's actual cursor to match.

> **Under the hood**
> An arrow key doesn't send one byte — it sends a **multi-byte escape sequence** (Up arrow is commonly `ESC [ A`, the three bytes `0x1B 0x5B 0x41`). Readline's input loop specifically recognizes this exact byte sequence and interprets it as "move up in history." This is precisely why the naive raw-mode program in Section 5, which does no such interpretation, prints an arrow key as three separate, seemingly nonsensical characters instead of doing anything useful — go press an arrow key against it and see for yourself.

**History persistence:** Readline keeps an in-memory list of previous lines, and Bash specifically persists this to `~/.bash_history` on exit (or incrementally, depending on `HISTFILE` and related shell options) — the file behind the history you browse with the up arrow, finally connected to the mechanism that produces it.

**Zsh** uses **ZLE** (Zsh Line Editor) instead — recap file 05 — a completely independent implementation, not Readline, solving the identical problem (raw-mode-based, self-drawn line editing) with entirely separate code.

**macOS** ships `libedit`, a BSD-licensed, API-compatible replacement for GNU Readline, rather than the GPL-licensed original — the same licensing-avoidance pattern as file 03/05's frozen-Bash-3.2 story. Programs "linked against readline" on macOS are frequently actually linked against `libedit`'s readline-compatible shim, which can behave slightly differently in some edge cases — worth knowing if a Readline-dependent tool feels subtly different between a Linux machine and a Mac.

> **Interview answer**
> "Bash's interactive line editing has nothing to do with the kernel's canonical mode — Readline explicitly disables canonical mode and echo, then reimplements line editing entirely in user space: reading raw bytes, recognizing multi-byte escape sequences for special keys, and manually redrawing the line with its own escape sequences after every change. Zsh does the identical job with a completely separate implementation, ZLE."

---

## 7. `vim`, `nano`, `less`, `top` — why they need terminal control

All four put the terminal into raw (or near-raw) mode via their own `tcsetattr()` calls, for the same fundamental reason Readline does: they need to react to **individual keystrokes** immediately — a single `h`/`j`/`k`/`l` in `vim`, a spacebar in `less` — rather than waiting for a kernel-buffered, canonically-edited whole line.

**Beyond raw mode**, these full-screen programs typically also switch to the **alternate screen buffer** — a separate virtual "page" the terminal maintains — so that on exit, your original terminal content is precisely restored rather than permanently overwritten or scrolled past by whatever they drew. Full mechanics: files 24-25.

**`top`/`htop`** add one more pattern beyond the others: they also periodically re-read fresh data (from `/proc`, file 33) and redraw **on a timer**, independent of any keyboard input at all — a genuinely different trigger from `vim`/`less`/`nano`'s purely keystroke-driven redrawing.

---

## 8. Common misconceptions

| Misconception | Reality |
|---|---|
| "Bash's arrow-key history is the kernel's canonical mode being smart." | The opposite — Readline explicitly *disables* canonical mode and reimplements editing entirely itself. |
| "`Ctrl+D` is a signal, like `Ctrl+C`." | No signal at all — a canonical-mode line-discipline behavior (early delivery, or EOF if the line is empty). |
| "A raw-mode program that crashes leaves no lasting effect." | Without careful restoration, the user's actual terminal can be left broken, needing `stty sane`/`reset` to fix. |
| "Readline and ZLE are the same code, just configured differently." | Fully independent implementations solving the same problem — recap file 05. |

---

## 9. Try this

> **Try this**
> Compile and run Section 5's program, then press an arrow key. Watch it print three separate, meaningless-looking characters — direct, hands-on proof of the escape-sequence fact from Section 6's callout.

> **Try this**
> ```
> stty -a
> ```
> Every current `termios` setting for your terminal, in human-readable form.

> **Try this**
> On a fresh line at your prompt, press `Ctrl+D` immediately (empty line — watch your shell exit). Then type a few characters first and press `Ctrl+D` again (non-empty line — compare the behavior).

---

## 10. Summary

- `termios` has five fields; `c_lflag` and `c_cc[]` are the ones this handbook uses most.
- `tcgetattr`/`tcsetattr` read and write terminal settings; `TCSAFLUSH` is the standard choice when entering raw mode.
- `Ctrl+D`/`VEOF` generates no signal — it's the same "read() returns 0" EOF mechanism as a closed pipe, just triggered by a keyboard character.
- A raw-mode program must restore original settings on every exit path, or it leaves the user's terminal visibly broken.
- Readline (Bash) and ZLE (Zsh) are independent implementations that both disable canonical mode and kernel echo, reading raw bytes and redrawing manually — the actual mechanism behind arrow-key history and Tab completion.
- `vim`/`nano`/`less`/`top` need raw mode for the same reason Readline does, and additionally use the alternate screen buffer to leave your terminal exactly as they found it on exit.

---

## What's next

`24_TERMINAL_CONTROL_SEQUENCES.md` covers the escape sequences this file kept referencing without defining — how `printf("\033[31mHello\033[0m")` changes color, and why the kernel is never the thing interpreting any of it.