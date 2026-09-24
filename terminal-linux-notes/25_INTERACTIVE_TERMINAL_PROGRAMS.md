# 25 — Interactive Terminal Programs

> **Where this fits in the stack**
> ```
> User  →  Terminal Emulator  →  PTY/TTY  →  [ vim / top / less / ssh ]  →  Kernel
>                                              ^^^^^^^^^^^^^^^^^^^^^^^^^^
>                                              this file
> ```
> Files 23 and 24 covered raw mode and escape sequences as separate mechanisms. This file is pure synthesis: putting them together into the recognizable "interactive full-screen program" pattern, contrasted directly against `ls`/`cat`'s much simpler one, and walking through exactly which pieces each of `vim`, `less`, `top`/`htop`, and `ssh` actually use — because they don't all use the identical combination.

---

## 1. Two patterns, contrasted

**The simple/batch pattern** (`ls`, `cat`, `grep`, most traditional Unix tools):
- Leaves the terminal in whatever mode it found it in — canonical mode, echo on, inherited straight from the shell. Never calls `tcsetattr` at all.
- Reads (if anything) and writes sequentially, top to bottom, never revisiting or overwriting something already printed.
- Exits, and the next prompt simply appears below whatever it printed — scrollback accumulates naturally, nothing is ever erased.

**The interactive/full-screen pattern** (`vim`, `top`, `less`, `htop`):
- Calls `tcsetattr` to enter raw or near-raw mode (file 23) — needs every keystroke immediately, can't wait for a buffered line.
- Typically switches to the alternate screen buffer (file 24), so it can freely redraw without permanently disturbing the user's scrollback.
- Uses cursor-positioning escape sequences (file 24) to update specific screen regions rather than just continuously printing new lines.
- Installs signal handlers (file 14) — `SIGWINCH` for resize-triggered redraws, often `SIGINT`/`SIGTSTP` too, so the program can restore terminal state gracefully rather than leaving it broken.
- On exit: restores original `termios` settings *and* leaves the alternate screen — the terminal looks exactly as it did before the program ran.

---

## 2. `vim` (and `nano`)

- **Raw mode:** essential — needs `hjkl`/arrows/every keystroke instantly, no line buffering.
- **Alternate screen:** yes — your file's content replaces the screen while editing; your original prompt and scrollback reappear untouched on `:q`.
- **Signals:** `SIGWINCH` (resize → reflow and redraw); often specifically catches `SIGINT` to *avoid* accidentally quitting, since `Ctrl+C` has vim-specific, mode-dependent meaning rather than the default "terminate."
- **Cursor positioning:** constantly, for every character typed or navigated to.

---

## 3. `less`

- **Raw mode:** yes — spacebar, arrows, `q` are single keystrokes, not lines.
- **Alternate screen:** yes, by default — though **configurable**; some setups (`less -X`, or certain terminal-specific configs) disable it, which is exactly why you occasionally see `less` leave its content visible after quitting rather than restoring the prior screen. Not a universal guarantee.
- **Signals:** `SIGWINCH`, to reflow long lines and recompute how many fit on the current screen size.
- **Cursor positioning:** for scrolling and redrawing the visible "window" into the underlying file.

---

## 4. `top` / `htop`

- **Raw mode:** yes — keystrokes for sorting columns, killing a process, and more.
- **Alternate screen:** yes.
- **Signals:** `SIGWINCH`, same as the others — but **genuinely different from `vim`/`less`**: it also redraws **on a timer** (typically every 1-3 seconds), entirely independent of any keyboard input, because its whole job is showing live, changing data (reading fresh numbers from `/proc` each refresh — preview: file 33).

> **Remember**
> `top`/`htop` are the clearest proof that "raw mode + alternate screen + escape sequences" isn't solely about keyboard reactivity. It's equally about being able to **cleanly and repeatedly redraw** an information-dense screen without flooding the terminal's actual scrollback with hundreds of near-duplicate snapshots.

---

## 5. `ssh` — a genuinely different case

`ssh`, running on **your local machine**, does something subtly different from the three programs above: it does *not* do its own elaborate screen-drawing at all. Instead, once connected, it puts **your local terminal** into raw mode — so it can forward every keystroke, unprocessed, to the remote side — and becomes, essentially, a **transparent byte conduit**: what you type goes, essentially unmodified, over the network to the remote shell or program; whatever the remote side sends back — which might itself be *full* of escape sequences, if you're running `vim` remotely — gets written, essentially unmodified, to your local terminal emulator, which is the thing actually interpreting all of it.

**When you run `vim` over `ssh`,** there are genuinely two layers of "terminal" involved:
- The **remote `vim`** process thinks it's talking to a remote PTY (allocated by `sshd` — full depth, file 26), and does all its normal raw-mode/alternate-screen/escape-sequence work exactly as if it were local.
- Those bytes travel over the network, through your **local `ssh` client** (itself in raw mode, transparently forwarding), and are finally interpreted by **your actual local terminal emulator** — which has no idea any of this originated remotely. It just sees bytes arriving on its PTY master, indistinguishable from a local program having written them.

This is files 01, 02, and 24's core lessons, now applied to a genuinely more layered, real-world case. Full depth: file 26.

---

## 6. Why `ls`/`cat` need none of this

They have nothing to react to per-keystroke — no interactive input expected mid-execution. Their output is naturally sequential and appendable — nothing about `ls`'s listing benefits from later overwriting a specific screen position. Leaving the terminal's existing settings alone isn't laziness — it's the *correct* choice for a program whose entire interaction model is "produce output, then exit," where the shell's own canonical-mode line-reading needs to resume immediately and normally the instant the program is done.

---

## 7. Common misconceptions

| Misconception | Reality |
|---|---|
| "`vim`/`top` use some fundamentally different terminal connection than `ls`/`cat`." | Identical PTY, identical fds — the difference is entirely in how each program *chooses* to configure and use it via `termios` and escape sequences. |
| "`ssh` renders `vim`'s interface itself." | `ssh` is a near-transparent conduit; the *remote* `vim` does the normal work, and *your local* terminal emulator does the actual rendering, exactly as it would for anything else. |
| "`less` always restores the original screen on exit." | Configurable, not universal — some setups disable the alternate-screen behavior entirely. |

---

## 8. Try this

> **Try this**
> Run `ls`, then immediately `vim somefile`, then `:q`. Watch `ls`'s output sit exactly where it was in your scrollback, completely untouched — while `vim`'s entire editing session leaves no trace at all once you quit. A direct, felt version of Section 1's contrast.

> **Try this**
> Run `htop` and watch it for a few seconds without touching a single key — the display keeps updating anyway, direct observation of Section 4's timer-driven redraw.

---

## 9. Summary

- Interactive full-screen programs combine four mechanisms — raw mode, alternate screen, cursor-positioning escapes, and signal handling — all built on exactly the plumbing files 02, 14, 23, and 24 already established.
- `vim` and `less` are purely keystroke-driven; `top`/`htop` add a timer-driven redraw on top, since their job is showing data that changes on its own.
- `ssh` is architecturally different from the other three — a transparent conduit, not a screen-drawing program itself, with the *remote* program doing the real work and your *local* terminal emulator doing the actual rendering.
- `ls`/`cat` correctly do none of this — their interaction model never needs it, and leaving the terminal's settings alone is the right choice, not an oversight.

---

## What's next

`26_SSH.md` goes fully deep on the layered case this file previewed: authentication, the encrypted channel, remote PTY allocation, and exactly what `ssh -t` changes.
