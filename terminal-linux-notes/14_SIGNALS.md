# 14 — Signals

> **Where this fits in the stack**
> ```
> User  →  Terminal Emulator  →  PTY/TTY  →  Shell  →  Processes  →  [ SIGNALS ]  →  Kernel
>                                                                     ^^^^^^^^^^^^
>                                                                     this file
> ```
> File 02 previewed that the line discipline turns `Ctrl+C` into `SIGINT`. File 13 ended with `SIGPIPE` appearing out of nowhere. This file is the full mechanism: what a signal actually is, the complete catalog you need, both keyboard shortcuts traced to the exact kernel action, and why two of these signals are deliberately impossible to intercept.

---

## 1. What is a signal?

**L1 — intuition:** a small, asynchronous interruption telling a process "something happened," independent of whatever it was doing at that instant.

**L2 — systems detail:** signals are identified by small integers with symbolic names — `SIGINT` is 2, `SIGKILL` is 9, `SIGTERM` is 15 on Linux (worth knowing these three numbers specifically, since `kill -9` is such a common idiom people use without ever connecting "9" to "SIGKILL"). Delivered by the kernel to a target process, which handles each signal according to its current **disposition**:

- **Default** — the kernel's built-in behavior for that signal (often terminate, sometimes stop, sometimes do nothing)
- **Ignore** — explicitly told to do nothing
- **Caught** — a registered handler function runs instead

---

## 2. The signal catalog

| Signal | Meaning | Default action | Catchable? |
|---|---|---|---|
| `SIGINT` (2) | "Interrupt" — sent by the terminal driver on `Ctrl+C` | Terminate | Yes |
| `SIGTERM` (15) | The polite request to exit; `kill`'s default | Terminate | Yes |
| `SIGKILL` (9) | The unblockable kill | Terminate | **No** — Section 5 |
| `SIGSTOP` | Pause the process | Stop | **No** — Section 5 |
| `SIGCONT` | Resume a stopped process | Continue | Yes |
| `SIGCHLD` | Sent to a parent when a child changes state | Ignore | Yes |
| `SIGHUP` (1) | "Hangup" | Terminate | Yes |
| `SIGPIPE` | Wrote to a pipe/socket with no reader left | Terminate | Yes |

**`SIGCHLD`** is worth a specific note: it's what lets a process supervisor (or a shell's own job-control bookkeeping) find out a child has exited or changed state **without polling `waitpid()` in a busy loop** — it can simply wait to be signaled, then go check.

**`SIGHUP`** carries its name literally from file 01's teletype history — originally sent when a phone-line connection physically dropped. Today, it's exactly what a terminal emulator sends to its session leader shell when the window closes — file 01's "closing the terminal window" scenario, now precise. `nohup` works by making the wrapped command explicitly **ignore** `SIGHUP`, so it survives the terminal disappearing.

**`SIGPIPE`** is file 13's `yes | head -5` example, formalized: writing to a pipe whose read end has been closed generates this. Some programs deliberately ignore it and check `write()`'s return value/`errno` (`EPIPE`) instead, for a more graceful "the reader went away" response than abrupt termination.

---

## 3. `Ctrl+C`, traced precisely

```
Keyboard
 ↓
terminal driver
 ↓
foreground process group
 ↓
SIGINT
 ↓
process
```

Precisely: keystroke → terminal emulator → PTY master → the kernel's line discipline (file 02) recognizes the `VINTR` character (`Ctrl+C` by default, from `termios`'s `c_cc[VINTR]`) → the line discipline generates `SIGINT` and delivers it to **every process in the controlling terminal's current foreground process group** (file 02's preview, now paying off) → each process reacts per its own disposition.

> **Common mistake**
> "`Ctrl+C` only affects the one program I 'see' running." It's delivered to the **entire foreground process group** — in `sleep 100 | cat`, both `sleep` and `cat` sit in the same foreground process group (file 13), and both receive `SIGINT` when you press `Ctrl+C`, not just whichever one happens to be producing visible output at that moment.

---

## 4. `Ctrl+Z`, traced the same way — with one crucial distinction

Same path, different character: `VSUSP` (`Ctrl+Z` by default) generates **`SIGTSTP`** — **not** `SIGSTOP`.

> **Remember**
> `SIGTSTP` and `SIGSTOP` are not the same signal, despite having near-identical *default* effects (both stop the process). `SIGTSTP` is what the terminal driver actually sends for `Ctrl+Z`, and it **can** be caught or ignored — some programs catch it deliberately, to restore terminal state (file 23) or finish a critical section *before* actually suspending, then reset things when `SIGCONT` resumes them. `SIGSTOP` is the unconditional, unblockable version — Section 5.

---

## 5. Why `SIGKILL` and `SIGSTOP` can never be caught

This is a deliberate design decision, not an oversight. The kernel reserves exactly these two signals as **unblockable, uncatchable, unignorable** — guaranteeing that *something* — an administrator, a supervising process — can always forcibly stop or terminate any process, no matter how broken, hung, or deliberately adversarial its own code is, even if that code is actively trying to ignore termination requests. This is a genuine safety valve: if every signal were interceptable, a sufficiently broken program could make itself effectively unkillable short of rebooting the machine.

**Mechanically**, this isn't "the process could catch it but the default just always wins" — the kernel refuses to let a process register a handler for `SIGKILL`/`SIGSTOP` at all; `sigaction()` returns an error if you try.

**The practical corollary:** `kill -9 PID` bypasses **any** custom cleanup logic entirely — no handler runs, no chance to flush buffers, close files, or release locks gracefully. It's the reason `kill -9` is a genuine last resort rather than a routine tool: it can leave a program's on-disk or external state inconsistent precisely because the process never got the opportunity to run a single line of its own shutdown code.

---

## 6. `kill PID` does not mean "force kill"

The command's name is genuinely misleading. `kill`'s actual job is simply **"send a signal"** — any signal, not necessarily a fatal one.

```bash
kill PID          # sends SIGTERM by default — a polite, catchable REQUEST to exit
kill -9 PID       # sends SIGKILL — the actual, unblockable forceful one
kill -STOP PID    # sends SIGSTOP
kill -l           # lists every signal name/number available on your system
```

> **Interview answer**
> "`kill` without a signal specified sends `SIGTERM`, a catchable, ignorable request the target is free to respond to however it wants — including not dying at all. Only `kill -9` sends `SIGKILL`, which removes that choice entirely. The command's name suggests forceful termination, but by default it's closer to politely asking."

---

## 7. `SIGWINCH` — the terminal resize signal

Not in most introductory signal lists, but directly relevant to a question this whole handbook has been building toward: **how do `vim`, `htop`, `tmux`, and `less` know to redraw themselves when you resize your terminal window?**

When the terminal emulator's window is resized, it tells the kernel the PTY's new dimensions via the `TIOCSWINSZ` ioctl — and the kernel responds by sending **`SIGWINCH`** to the foreground process group of that terminal. A program that cares (full-screen interactive programs, file 25) catches it, queries the new size (`TIOCGWINSZ`), and re-renders its entire display to fit. A program that doesn't care (`ls`, `grep`) simply ignores it by default — which is fine, since it was never drawing anything size-dependent to begin with.

---

## 8. Registering a handler, concretely

```c
#include <signal.h>
#include <stdio.h>

volatile sig_atomic_t got_sigint = 0;

void handler(int sig) {
    got_sigint = 1;      /* signal-safe: just set a flag, do real work later */
}

int main() {
    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    while (!got_sigint) {
        /* ordinary work */
    }
    printf("caught SIGINT, cleaning up\n");
    return 0;
}
```

> **Under the hood**
> A handler can interrupt your program **at literally any point**, mid-instruction-sequence, which is exactly why handlers should do as little as possible — setting a flag, as above, is the canonical safe pattern. Calling non-async-signal-safe functions inside a handler — `printf`, `malloc` — is a real bug source: if the signal arrives while the main program is mid-way through updating a `stdio` buffer (file 09's buffering mechanism), and the handler *also* calls `printf`, you can corrupt that buffer's internal state.

---

## 9. Common misconceptions

| Misconception | Reality |
|---|---|
| "`kill PID` immediately force-terminates a process." | Sends `SIGTERM` by default — catchable, ignorable, a request rather than a command. |
| "`SIGSTOP` and `SIGTSTP` are the same signal." | `SIGTSTP` (`Ctrl+Z`) is catchable; `SIGSTOP` is unconditionally unblockable. |
| "`Ctrl+C` only reaches the process I'm actively watching." | It goes to the entire foreground process group, every member. |
| "Signals behave like ordinary function calls into the target." | They're asynchronous interruptions that can occur at any point in execution — which is exactly why signal-handler code has to be so conservative. |

---

## 10. Try this

> **Try this**
> ```bash
> trap 'echo caught!' INT
> # now press Ctrl+C
> ```
> Recap from file 04 — your custom handler fires instead of the shell's default `SIGINT` behavior.

> **Try this**
> ```bash
> kill -l
> ```
> Every signal name and number available on your system, in one list.

> **Try this**
> Run `htop` or `less` on a long file, then resize your terminal window — watch it redraw instantly, a directly visible `SIGWINCH` in action.

---

## 11. macOS note

The core POSIX signal set and numbering are essentially identical to Linux. Full platform comparison: file 43.

---

## 12. Summary

- A signal is an asynchronous kernel-to-process notification; a process's disposition for it is default, ignore, or a registered handler.
- `Ctrl+C` → the line discipline recognizes `VINTR` → `SIGINT` delivered to the **entire** foreground process group.
- `Ctrl+Z` → `VSUSP` → `SIGTSTP` — catchable, unlike the unconditional `SIGSTOP`.
- `SIGKILL`/`SIGSTOP` are deliberately uncatchable by kernel design — a guaranteed way to control any process, no matter how broken.
- `kill` sends `SIGTERM` by default, not a forced termination — the name is misleading.
- `SIGWINCH` is how full-screen terminal programs know to redraw on a resize.

---

## What's next

`15_JOB_CONTROL.md` picks up `SIGTSTP` and `SIGCONT` specifically and builds the full picture: foreground/background jobs, `fg`/`bg`, and exactly what `&` changes about how a command runs.