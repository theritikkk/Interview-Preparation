# 02 — TTY and PTY

> **Where this fits in the stack**
> ```
> User  →  Terminal Emulator  →  [ PTY / TTY ]  →  Shell  →  Programs  →  Syscalls  →  Kernel  →  Hardware
>                                  ^^^^^^^^^^^^
>                                  this file
> ```
> File 01 established that the terminal emulator and the shell are separate processes connected by "a PTY," without saying precisely what that word means. This file opens that box completely: how Linux represents terminal devices on disk, how the master/slave split actually works, and the exact byte-by-byte journey your keystrokes take to become `ls`'s output on your screen.

---

## 1. How Linux represents terminal devices

Every terminal-like thing on a Linux system is a **device file** — an entry under `/dev` that user-space programs `open()`, `read()`, and `write()` like an ordinary file, but which the kernel routes to a specific driver instead of a disk.

| Device | What it actually is |
|---|---|
| `/dev/tty1` … `/dev/tty6` | **Virtual consoles** — kernel-native text displays, driven directly onto your physical screen, switched with `Ctrl+Alt+F1`–`F6`. No terminal-emulator software involved (recap from file 01). |
| `/dev/tty0` | Refers to "whichever virtual console is currently active." |
| `/dev/console` | The device that receives kernel boot messages and system-level diagnostics — the modern, precise meaning of "console" from file 01. |
| `/dev/pts/0`, `/dev/pts/1`, `/dev/pts/2`, … | **PTY slave devices.** Each open terminal-emulator window/tab/pane gets one of these, numbered in allocation order. This is where your shell's stdin/stdout/stderr actually point when you're in a graphical terminal or an SSH session. |
| `/dev/ptmx` | The **PTY multiplexer**. Not a terminal itself — opening this device is how a program (a terminal emulator, `tmux`, `sshd`) *creates a brand-new PTY pair* and gets a master file descriptor back. This is the actual API entry point behind everything in Section 4. |
| `/dev/tty` | A **magic alias**, not a real device with its own driver in the usual sense. Opening `/dev/tty` from any process connects you to *that process's own controlling terminal*, whatever it currently is. It's a convenience the kernel resolves per-process, not a fixed piece of hardware. |

> **Under the hood**
> Older Unix and early Linux used a fixed pool of statically named device pairs for pseudo-terminals — `/dev/ptyXX` for the master, `/dev/ttyXX` for the matching slave, XX drawn from a limited alphabet-and-digit combination, which put a hard ceiling on how many PTYs could exist at once. The modern scheme (`/dev/ptmx` + dynamically numbered `/dev/pts/N`, standardized as "Unix98 PTYs") replaced this specifically to remove that ceiling and simplify permission handling. If you ever see `/dev/ptyp0`-style names in very old documentation, that's the legacy scheme.

> **Try this**
> ```
> ls /dev/pts/
> ```
> Every currently-open terminal session on your machine (every terminal-emulator tab/window, every SSH connection, every `tmux` pane) shows up here as a number. Open a new terminal tab and run it again — a new number appears.

---

## 2. Controlling terminal, sessions, and process groups — the vocabulary you need right now

These get their full, deep treatment in `16_SESSIONS_AND_PROCESS_GROUPS.md`. You need a working definition of each *now*, though, because the data-flow trace in Section 5 refers to them.

- **Controlling terminal** — the one specific terminal device a session is associated with. It's what makes `Ctrl+C` know which processes to interrupt, and it's what `/dev/tty` resolves to when a process opens it.
- **Session** — a collection of related processes, typically everything descending from one login or one terminal-emulator window, created via `setsid()`. A session has *at most one* controlling terminal.
- **Foreground process group** — within a session, the one group of processes currently "holding the floor" on the controlling terminal: able to read from it, and the target of signals like the `SIGINT` your `Ctrl+C` generates.
- **Background process group** — everything else in the session. If a background process tries to *read* from the controlling terminal, it's suspended (`SIGTTIN`); depending on settings, writing can be restricted too.

> **Remember**
> Exactly one process group per session gets to be "in the foreground" of the controlling terminal at any moment. This single fact is the entire mechanism behind job control (file 15) — `Ctrl+Z` and `bg`/`fg` are really just changing *which* group currently holds that slot.

---

## 3. Commands that reveal this: `tty`, `who`, `ps`

> **Try this**
> ```
> tty
> ```
> Prints the device path of *your own controlling terminal* — something like `/dev/pts/3`. Internally this is just `ttyname()` called on file descriptor 0.

> **Try this**
> ```
> who
> ```
> Lists every logged-in user and, critically, **which terminal device** their session is attached to. Run it while you have two terminal tabs open — you'll see two entries, two different `/dev/pts/N` devices, same or different users.

> **Try this**
> ```
> ps -o pid,ppid,tty,stat,comm
> ```
> The `TTY` column shows each process's controlling terminal. Background daemons and kernel threads typically show `?` here — they have no controlling terminal at all, which is precisely what makes them daemons (full treatment in file 28). The `STAT` column's `+` suffix marks foreground process-group membership on that terminal.

---

## 4. PTY architecture: master and slave

File 01 established *why* this needs to exist: a terminal emulator has to convincingly impersonate the old wire-and-hardware terminal setup, in software, for the shell's benefit. The mechanism that makes that impersonation work is the **PTY** — a pseudo-terminal, meaning "a fake terminal device, implemented as a connected pair, with a real terminal driver's behavior attached."

```
Terminal Emulator
       |
       v
   PTY master        <-- the terminal emulator holds this end
       |
       v
   PTY slave         <-- the shell (and its children) hold this end;
       |                  this becomes their controlling terminal,
       v                  their stdin/stdout/stderr
     Shell
       |
       v
   Programs   (ls, grep, vim, ...)
```

**What makes this different from an ordinary pipe** (this distinction matters enough that file 30 dedicates a full comparison table to it): writing into one end and reading from the other is pipe-like, but a PTY pair also has a **kernel line discipline** attached to it (Section 7) — the same canonical-mode buffering, automatic echo, and special-character signal generation a real serial terminal driver would provide. A plain `pipe()` has none of that. This is the feature that lets `bash`, `vim`, and every other terminal-aware program treat a PTY exactly as if it were a genuine hardware terminal, with zero code changes.

- The **master** side is what the terminal emulator opens and holds. Writing to it is "as if a keystroke arrived at the terminal." Reading from it is "receiving whatever the terminal would display."
- The **slave** side is what the shell (and everything it runs) opens as its controlling terminal. From the shell's point of view, `/dev/pts/3` genuinely *is* its terminal — it has no way to tell, and no reason to care, that the other end is software rather than a serial cable to a VT100.

---

## 5. The complete data flow: typing `ls` and pressing Enter

This is the trace file 01 promised. Follow it once, slowly — every later file in this handbook builds on exactly this sequence.

```
 1. You press l, s, Enter on your physical keyboard
 2. Keyboard hardware → kernel input subsystem → display server (X11/Wayland)
 3. Terminal emulator's event loop receives these as GUI key-press events
 4. Terminal emulator converts them to bytes: 'l', 's', '\n'
 5. Terminal emulator write()s these bytes to the PTY MASTER fd
 6. Kernel's line discipline (attached to this pty pair, governed by the
    SLAVE's termios settings) receives them on the input side
 7. In canonical mode (the default): each byte is ECHOED immediately
    back out via the master — the terminal emulator reads this and
    renders 'l', 's' on screen, WITHOUT the shell doing anything yet
 8. On '\n', the line discipline finalizes the buffered line "ls\n"
    and makes it available for reading on the SLAVE side
 9. The shell — blocked in read() on fd 0, which is the PTY slave —
    wakes up and receives the complete line "ls\n"
10. Shell tokenizes it (file 06), determines "ls" isn't a builtin,
    searches PATH (file 07)
11. Shell calls fork() — a new child process is created
12. Child calls execve("/bin/ls", ...) — the child process image
    becomes ls; fds 0/1/2 are inherited unchanged, still pointing
    at the PTY slave
13. ls does its work (openat/getdents on the target directory —
    previewed in file 19) and write()s formatted output to fd 1,
    which is still the PTY slave
14. Kernel's line discipline passes this OUTPUT straight through
    (canonical-mode processing applies to INPUT; output isn't
    buffered/edited the same way) and makes it available on the
    MASTER side
15. Terminal emulator — blocked in read() on the master fd — wakes
    up, receives ls's output bytes
16. Terminal emulator interprets any escape sequences (e.g. color
    codes from `ls --color`) and renders the result on screen
    (file 24)
17. ls exits; the shell's waitpid() (file 08) wakes up with its
    exit status
18. Shell writes its next prompt — same fd 1 → slave → master →
    terminal emulator → screen path as step 13-16
```

> **Remember**
> Step 7 already put characters on your screen — **before the shell has read a single byte**. This is the single most important fact in this file, and it's the entire subject of Section 8.

---

## 6. Canonical mode vs raw mode

The line discipline (Section 7) operates in one of two fundamentally different modes, set via `termios`:

| | **Canonical mode** ("cooked") | **Raw mode** ("non-canonical") |
|---|---|---|
| Delivery unit | Whole lines, only after Enter | Individual bytes, essentially immediately |
| Local editing | Kernel handles backspace/kill-line *before the process ever sees the removed characters* | None — the reading process must implement everything itself |
| Special characters (`Ctrl+C`, `Ctrl+Z`, …) | Recognized by the kernel, converted to signals automatically | Only if the process asks for that specifically; usually disabled entirely so the raw bytes reach the program |
| Who uses it | The default for ordinary shell interaction — `cat`, `ls`, non-interactive command entry | `vim`, `less`, `top`, `ssh`'s interactive session, Bash's own line editor (see the callout below) |

**Why raw mode has to exist at all:** canonical mode's entire value proposition is "the kernel buffers and edits a full line before your program sees it" — which is actively *hostile* to any program that wants to react to individual keystrokes as they happen (arrow keys moving a cursor, `vim` responding to `hjkl` instantly, `less` scrolling on a single spacebar press). Those programs explicitly disable canonical mode — and usually echo too — and take on the entire burden of rendering feedback themselves. File 23 covers the exact `termios` calls that do this, and file 25 covers the broader pattern of interactive terminal programs built this way.

> **Common mistake**
> "If canonical mode buffers whole lines until Enter, how does Bash let me press the Up arrow to browse history *before* pressing Enter?" — It doesn't rely on canonical mode for that at all. Bash puts the terminal into **its own raw-ish mode** via GNU Readline and does line editing entirely in user-space, byte by byte, rendering its own visual feedback as you type. The simple "kernel buffers the line" story in this file describes the *default* terminal behavior a non-interactive-editing program gets for free — Bash specifically opts out of it to implement something richer. Full mechanics: file 23.

---

## 7. Terminal line discipline

The **line discipline** ("ldisc") is the kernel layer sitting between the raw tty/PTY driver and the process reading/writing it, responsible for everything described above. The default one on Linux is called **N_TTY**, and its behavior is entirely controlled by a `termios` structure associated with the device:

```c
struct termios {
    tcflag_t c_iflag;   /* input flags   */
    tcflag_t c_oflag;   /* output flags  */
    tcflag_t c_cflag;   /* control flags */
    tcflag_t c_lflag;   /* LOCAL flags — canonical mode, echo, signals live here */
    cc_t     c_cc[NCCS]; /* special control characters */
    /* ... */
};
```

The flags and characters that matter most for everything in this file:

| Field | Meaning |
|---|---|
| `ICANON` (in `c_lflag`) | Canonical mode on/off |
| `ECHO` (in `c_lflag`) | Automatic echo on/off |
| `ISIG` (in `c_lflag`) | Whether special characters generate signals at all |
| `c_cc[VINTR]` | The character that generates `SIGINT` — `Ctrl+C` by default |
| `c_cc[VQUIT]` | Generates `SIGQUIT` — `Ctrl+\` by default |
| `c_cc[VSUSP]` | Generates `SIGTSTP` — `Ctrl+Z` by default |
| `c_cc[VERASE]` | The backspace character, handled entirely inside the kernel in canonical mode |
| `c_cc[VEOF]` | End-of-file character — `Ctrl+D` by default |
| `c_cc[VMIN]`, `c_cc[VTIME]` | In raw mode, control how many bytes / how much time a `read()` waits for |

So: **the line discipline is what turns `Ctrl+C` into `SIGINT`** — the kernel recognizes the `VINTR` byte and delivers the signal to the terminal's foreground process group directly, without any process needing to inspect the raw byte stream for it. Full signal semantics: file 14. Exactly *which* process group receives it: file 16.

---

## 8. Echo: why characters appear without the shell printing them

This is the direct answer to the question step 7 raised.

**The kernel echoes, not the shell.** When `ECHO` is set (the default for an interactive terminal) and the line discipline is in canonical mode, every input byte it receives is automatically copied back out — for a PTY, that means back out through the master, where the terminal emulator reads it and renders it. This happens **entirely inside the kernel**, before the shell has read anything, indeed even if no process is currently reading the slave at all.

**Proof this is real, not just a simplification:** password prompts. When `sudo` or `ssh` asks for your password, your keystrokes produce no visible characters at all — not even asterisks, typically. That's the program explicitly clearing the `ECHO` flag on the terminal before reading the password, then restoring it afterward. If echo were something the *shell* did by printing what you type, a password prompt run from a different program entirely (not the shell) would have no way to suppress it. Because echo is a kernel/line-discipline feature, any program can turn it off for the duration of a sensitive read.

> **Interview answer**
> "Terminal echo — the fact that you see characters as you type them — is implemented by the kernel's tty line discipline, which copies input bytes back out to the display side automatically in canonical mode. It has nothing to do with the shell explicitly printing what you type. Programs that need silent input, like password prompts, temporarily disable the `ECHO` flag via `tcsetattr`."

> **Common mistake**
> "Bash echoes my typing." Ordinary line entry, yes — but that's the *kernel* doing it underneath Bash, not Bash's own code. And the moment Bash is doing rich interactive line editing via Readline (arrow keys, tab completion), it's typically running with the kernel's automatic echo *turned off* and doing its own explicit rendering instead — because it needs to redraw things the kernel's simple echo can't express, like moving the cursor mid-line. Both "the kernel echoes for you" and "the program renders its own feedback" are real, mutually exclusive modes — which one is active depends entirely on how the foreground program configured the terminal. Full detail: file 23.

---

## 9. Under the hood: allocating a PTY in C

This is the mechanism file 01 previewed. Here it is in full:

```c
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

int master_fd = posix_openpt(O_RDWR | O_NOCTTY);  /* opens /dev/ptmx,
                                                        returns a NEW
                                                        PTY master fd  */
grantpt(master_fd);                                /* fixes ownership/
                                                        permissions on
                                                        the slave      */
unlockpt(master_fd);                               /* the slave starts
                                                        locked; this
                                                        releases it    */
char *slave_path = ptsname(master_fd);             /* e.g. "/dev/pts/4" */

int slave_fd = open(slave_path, O_RDWR);           /* now open the
                                                        slave side too */
```

Four calls, and you have a genuine master/slave PTY pair — exactly what every terminal emulator, `tmux`, and `sshd` does at the moment they need to hand a program a "terminal" that doesn't correspond to real hardware. Lab 17 has you do exactly this and inspect the result with `ls -l /dev/pts/`.

---

## 10. Common misconceptions

| Misconception | Reality |
|---|---|
| "A PTY is basically just a pipe with a different name." | A pipe moves bytes; a PTY moves bytes **and** applies a full line discipline — canonical-mode editing, automatic echo, special-character signal generation. Full three-way comparison (TTY / PTY / pipe / socket): file 30. |
| "The shell prints each character as you type it." | The kernel's line discipline does, automatically, whenever canonical mode + echo are active — which is most of the time, but not universally (password prompts, Readline's own editing mode). |
| "`/dev/tty` and `/dev/pts/3` are the same kind of device." | `/dev/pts/3` is a specific, real PTY slave. `/dev/tty` is a per-process magic alias that resolves to *whichever* terminal is currently that process's controlling terminal — opening it from two different processes can lead to two entirely different real devices. |
| "Canonical mode is just a technicality — every program experiences the terminal the same way." | No — it's the entire reason `vim` and `ls` behave so differently when you press a key. One reads whole edited lines from the kernel; the other reads raw keystrokes it must interpret itself. |

---

## 11. macOS note

macOS's PTY implementation (part of the BSD layer inside Darwin/XNU — file 43) uses the same conceptual master/slave split and the same POSIX allocation API (`posix_openpt`/`grantpt`/`unlockpt`/`ptsname`) described in Section 9 — code written against this API is portable between the two essentially unchanged. Device naming differs (`/dev/ttys000`-style slaves rather than `/dev/pts/N`), and some `termios` control-character defaults and extension flags differ slightly. The line-discipline concept, canonical/raw mode split, and echo behavior are the same story on both.

---

## 12. Summary

- Linux represents terminals as device files: `/dev/pts/N` (PTY slaves), `/dev/ptmx` (the allocation entry point), `/dev/tty1`-`6` (virtual consoles), `/dev/tty` (the per-process controlling-terminal alias), `/dev/console` (kernel messages).
- A **PTY** is a master/slave pair with a genuine kernel line discipline attached — this is what lets software convincingly stand in for real terminal hardware.
- The full data-flow trace for `ls`: keyboard → terminal emulator → PTY master → line discipline → PTY slave → shell reads a line → fork/exec → `ls` writes output → PTY slave → line discipline → PTY master → terminal emulator → screen.
- **Canonical mode** buffers and edits whole lines in the kernel; **raw mode** delivers bytes essentially immediately, pushing all editing responsibility onto the reading program.
- **Echo is a kernel feature**, not a shell feature — proven by the fact that any program (password prompts) can disable it independent of the shell.

---

## What's next

`03_SHELLS_OVERVIEW.md` moves one layer down the stack from the PTY into the process sitting on its slave end: what a shell actually is, why it exists as separate software at all, and the landscape of `sh`/`dash`/`bash`/`zsh`/`ksh`/`fish` this handbook will keep referring back to.
