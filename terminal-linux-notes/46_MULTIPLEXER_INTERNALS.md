# 46 — Multiplexer Internals

> **Where this fits in the stack**
> ```
> Terminal Emulator  ←→  [ tmux client ]  ←socket→  [ tmux SERVER ]  ←→  pane PTY  ←→  shell / program
>                                                    ^^^^^^^^^^^^^^^^
>                                                    a full terminal emulator, run once per pane
> ```
> File 27 explained *why* tmux survives a dropped connection. This file opens the box: what the server actually does with every byte, why that makes a multiplexer a terminal emulator in its own right, what the server therefore ends up *owning*, and where the design starts to strain. It builds directly on file 45 — the parser state machine — because a multiplexer is that machine, running on every pane. File 27 has also been corrected: its description of how the client and server share the terminal was imprecise, and Section 2 gives the accurate version.

---

## 1. Why multiplexers exist

An ordinary remote shell lives and dies with its connection (files 16, 27): when SSH drops, the PTY closes, the session leader gets `SIGHUP`, and a job you started two hours ago dies with it. The fix — insert a long-lived server that owns the shell's PTY, so the connection becomes disposable — is the idea behind **GNU Screen (1987)** and, later, **tmux (2007, by Nicholas Marriott — the copyright header in tmux's `input.c` carries that year)**. The same server can hold many terminals at once, which is the "multiplex" in the name: windows and panes, all carried over one connection.

---

## 2. The client–server mechanics, stated precisely

File 27 gave the outline: a tmux **client** talks to a tmux **server** over a **Unix domain socket** (file 27, Section 4), and because that socket is local, the multiplexer itself never crosses the network — SSH (file 26) carries your bytes to the remote machine, and the client and server then talk locally. That much stands. Here is the accurate detail underneath, drawn from tmux's own commit history:

- The messages on the socket use tmux's `imsg` layer.
- When a client attaches, it **identifies itself** — and **passes its terminal's file descriptor to the server** over the socket, using ordinary Unix file-descriptor passing (`SCM_RIGHTS`). A tmux commit records this as having the client pass its stdin fd to the server "and have the server use that rather than reopening the tty."
- From then on, **the server reads your keystrokes from that terminal and writes screen updates to it directly.** The client process is not a byte relay in the middle of that path; it mostly sits there holding the terminal open, and forwards events like window resizes and its own exit.
- Commands that produce output for the *shell* (for example `tmux capture-pane -p`) are different: there the client handles the standard I/O and the data travels in messages.

> **Remember**
> The "thin client" description is right about *weight* and wrong about *mechanism*. The client is thin because the server has taken over the terminal itself — not because the client shuttles bytes to it.

Each pane has its own **PTY pair** (file 02), allocated and owned by the server, and the pane's shell is the server's child — exactly the process tree file 27 showed with `pstree`.

---

## 3. A multiplexer is a terminal emulator

When a program inside a tmux pane prints something, this is the path it takes:

```
program writes bytes
   → pane PTY slave → pane PTY master
   → tmux server reads them
   → server's PARSER (input.c) runs the state machine from file 45
   → results applied to the PANE's grid — the server's own copy of the screen, scrollback included
   → server GENERATES new escape sequences describing the change, shaped for the CLIENT's terminal
   → written to the client's terminal (the fd it was handed)
   → the OUTER terminal emulator runs ITS parser on those bytes
   → its own grid → pixels
```

This is not a metaphor. tmux's `input.c` opens with a comment that it is "Based on the description by Paul Williams" — pointing at exactly the document file 45 is built on — listing four changes: **7-bit only, UTF-8 support, an added APC state (similar to OSC), and a state for GNU screen's `ESC k … ESC \` window-rename sequence.** That is a Williams-derived parser, inside the multiplexer, running on every byte every pane produces.

**It isn't unique to tmux.** Zellij's server crate has the same shape: a `grid.rs` implementing a terminal grid driven by an ANSI parser, a `terminal_pane.rs` that feeds pane output through a `vte::Parser`, and Sixel handling. (`vte` is the Rust parser crate descended from the same Williams state machine.) GNU Screen likewise implements its own terminal emulation, and that's what its `TERM=screen` is about.

**And in reverse**, for input:

```
your keystrokes → outer terminal emulator → client's tty → tmux server reads them
   → server's KEY PARSER decodes the byte sequences into key events
   → prefix handling: is this Ctrl+B? then the NEXT key is for tmux (Section 6)
   → otherwise re-encoded for the application, written to the pane's PTY master
```

### What this costs

Count the work for one line of program output: the program writes it **once**; it is parsed **twice** by two different state machines (the server's, then your terminal's) into **two** different grids; and a **brand-new byte stream** is invented in between. For a human typing or reading ordinary output you will not feel it. The cost appears under **high-throughput output** — a build log, a huge `cat` — where every byte takes the whole trip.

The video walkthrough (which retells Hashimoto's architecture argument) says that however fast a modern GPU-accelerated terminal gets, it cannot outrun the multiplexer sitting in front of it, and puts the gap at "hundreds of times." **I could not verify that figure**, I can't tell from the transcript whether it is the narrator's or Hashimoto's own, and it depends heavily on workload; treat it as an unbenchmarked claim. What *is* structurally certain is the extra parse-and-regenerate step.

### The benefit side — a fair accounting

The server-side grid is also what makes some of tmux's best features possible: **instant redraw on reattach** (the server already has the screen), **scrollback and search that don't depend on whichever terminal happens to be attached**, and the ability to serve **terminals with different capabilities** from one session. The cost buys real things; the question the next file asks is whether it buys *too much* ownership.

---

## 4. Seeing it: a toy model, and a real experiment

### 4.1 A model of the mechanism (tested)

This is **not tmux** — it's a ~60-line model of the data path in Section 3: run a program on a PTY, parse its output into a grid, then regenerate a fresh stream describing the grid.

```python
#!/usr/bin/env python3
"""Toy model of a multiplexer's data path (NOT tmux): run a program on a PTY,
parse its output into a grid (the 'server-owned screen'), then REGENERATE a
fresh byte stream that describes that grid for an attached 'client terminal'."""
import os, pty, sys

ROWS, COLS = 3, 24
grid = [[(' ', 0) for _ in range(COLS)] for _ in range(ROWS)]   # (char, sgr_code)
row = col = pen = 0
state = 'GROUND'; params = ''

def newline():
    global row, col
    col = 0
    if row == ROWS - 1:
        grid.pop(0); grid.append([(' ', 0) for _ in range(COLS)])   # scroll
    else:
        row += 1

def feed(b):                          # a 3-state parser: ground / escape / csi
    global state, params, row, col, pen
    if state == 'GROUND':
        if b == 0x1B: state = 'ESCAPE'
        elif b == 0x0D: col = 0
        elif b == 0x0A: newline()
        elif 0x20 <= b < 0x7F:
            if col >= COLS: newline()
            grid[row][col] = (chr(b), pen); col += 1
    elif state == 'ESCAPE':
        if b == ord('['): state = 'CSI'; params = ''
        else: state = 'GROUND'
    elif state == 'CSI':
        if 0x30 <= b <= 0x3F: params += chr(b)
        else:
            if b == ord('m'): pen = int(params or 0)       # only SGR is modelled
            state = 'GROUND'

# --- run a program on a PTY and capture exactly what it writes -------------
pid, fd = pty.fork()
if pid == 0:
    os.execvp('printf', ['printf', r'\033[31mhello\033[0m world\nline2\n'])
raw = b''
while True:
    try: chunk = os.read(fd, 1024)
    except OSError: break
    if not chunk: break
    raw += chunk
os.waitpid(pid, 0)
for b in raw: feed(b)

# --- regenerate: a brand-new stream describing the grid --------------------
out = b'\x1b[2J'                                              # clear screen
for r in range(ROWS):
    out += f'\x1b[{r+1};1H'.encode()                          # position cursor
    cur = 0
    for ch, p in grid[r]:
        if p != cur: out += f'\x1b[{p}m'.encode(); cur = p
        out += ch.encode()
    if cur: out += b'\x1b[0m'
print('PROGRAM WROTE (read from the PTY master):'); print('  ', raw)
print('MULTIPLEXER SENDS to the client terminal :'); print('  ', out)
print('identical bytes?', raw == out)
```

Its actual output when run:

```
PROGRAM WROTE (read from the PTY master):
   b'\x1b[31mhello\x1b[0m world\r\nline2\r\n'
MULTIPLEXER SENDS to the client terminal :
   b'\x1b[2J\x1b[1;1H\x1b[31mhello\x1b[0m world             \x1b[2;1Hline2                   \x1b[3;1H                        '
identical bytes? False
```

Three observations, each real:

1. **The streams differ.** The regenerated stream adds a screen clear, absolute cursor positioning, and padding spaces the program never wrote — and omits nothing the screen needs. Same visual result, entirely different bytes.
2. **The program wrote `\n`; the multiplexer read `\r\n`.** The PTY's line discipline already translated it (`ONLCR` output processing — the `c_oflag` flags from file 23) *before* any multiplexer saw the data. The PTY is not a transparent pipe, even on the program side.
3. A real multiplexer regenerates *per operation as it applies output to its grid*, and fully on events like a window switch or reattach — a more efficient scheme than this toy's redraw-everything. The principle is identical.

### 4.2 The real experiment — **not run by me**

My sandbox has no tmux and no network to install it, so I have **not** run the following against real tmux. These commands use documented tmux features; run them yourself and compare:

```bash
# Terminal A — create a detached session and capture what the PROGRAM writes into its pane:
tmux new -d -s lab
tmux pipe-pane -t lab -o 'cat > /tmp/pane.out'

# Terminal B — attach THROUGH script, so everything tmux sends this client is recorded:
script -q -c 'tmux attach -t lab' /tmp/client.out
#   inside the attached session run:   printf '\033[31mhello\033[0m\n'
#   then detach with Ctrl+B, D  (script exits when the attach command does)

# Either terminal — compare the two recordings:
cat -v /tmp/pane.out ; echo ---- ; cat -v /tmp/client.out
```

If tmux were a plain pipe, the two would match. Expect `client.out` to contain cursor-positioning sequences and status-bar updates that the program never wrote — the same shape as the toy's output.

---

## 5. What the server owns

Because the server holds the grid, three things that feel like "client" concerns are actually **server-owned and shared**:

**1. The screen — and scrolling it.** Scrollback lives in the server's grid, and copy mode is state on the *pane*. Two clients viewing the same pane see the same scroll position: scroll on one machine and the other's view moves too. (tmux's *session groups* can give each client its own *current window*, but the pane contents and copy-mode state are still shared.)

**2. The geometry.** A PTY has exactly **one** size at a time (set with the `TIOCSWINSZ` ioctl; the program learns of changes via `SIGWINCH` — file 14), so a window has one size, and every attached client has to live with it. How that size is chosen is the `window-size` option: `largest`, `smallest`, `manual`, or `latest`. In current releases the default is **`latest`** — the window follows the **most recently active client** (introduced after the original `smallest`/`largest`/`manual` choices; verify yours with `tmux show -gw window-size`). Clients with a *larger* terminal than the window get a dotted fill around it. A client can also attach with an `ignore-size` flag so it doesn't influence the size at all.

> **Common mistake**
> "tmux always shrinks the window to the smallest client." That describes the `smallest` policy, and is how it was often experienced in older versions. With the current default, the window tracks whoever was active most recently — so a small client that attaches last can shrink the view for everyone, which is exactly what a demo of two mismatched clients shows, even though the rule isn't literally "smallest wins."

**3. The feature set.** Your program talks to the *server's* emulator, not your terminal. A capability only works if **both** understand it. The Kitty graphics protocol (carried in APC strings, file 45) is the clear example: tmux does not implement it, so images simply don't appear. tmux offers an escape hatch, the **`allow-passthrough`** option — **off by default** (since tmux 3.3a) — which lets a program wrap its escape sequences in a DCS envelope (`ESC P tmux; … ESC \`, with every ESC inside doubled) that tmux relays to the outer terminal without interpreting. It is a hack: because the passthrough bytes never enter tmux's grid, tmux doesn't know the image exists, so it can't scroll, clip, or move it with the pane — which is why it works reliably only in narrow cases, and why Kitty's Unicode-placeholder mode (which makes images behave like text tmux *can* track) is the common workaround.

---

## 6. In-band control: the prefix key, and `escape-time`

How do you tell tmux to split a pane when every key you type is bound for the program? The same answer as always (file 45): **in-band**. The **prefix key** (default `Ctrl+B`) means "the *next* key is for tmux." Consequently, to send a genuine `Ctrl+B` to a program you must send the prefix *twice* — the program's keystroke and the multiplexer's command channel are the same stream, with nothing to label which is which.

The same ambiguity causes a second, subtler problem. When tmux receives a bare `ESC` byte, it cannot know whether you pressed the Escape key or whether it is the first byte of `ESC [ A` (Up arrow, file 23) that simply hasn't fully arrived. It must **wait** to find out — the **`escape-time`** option (default 500 ms) is how long. That delay is why `vim` users running inside tmux often notice a laggy Esc key, and why setting it near zero is such a common tweak. It's a direct, user-visible consequence of control and data sharing one stream.

---

## 7. tmux control mode: the raw-bytes alternative that already exists

tmux has a mode where it stops rendering for a client and instead speaks a **text protocol** to it: `tmux -C` (and `-CC`, the variant iTerm2 uses to give you native windows for tmux windows). Its defining properties, from tmux's own wiki:

- Commands go in; replies come back framed in `%begin … %end` (or `%error`) blocks, each with a timestamp and command number.
- Pane output arrives as **`%output %<pane-id> <data>`** — the **raw bytes the program wrote**, with bytes below `0x20` and the backslash **octal-escaped** (`\033`, `\015`, `\134`). The wiki notes that output tmux generates *itself* (copy mode and similar) is **not** sent to control clients.
- Other notifications (`%window-pane-changed`, `%pane-mode-changed`, `%layout-change`, …) describe structure.
- Flow control exists: with `refresh-client -f pause-after=N`, tmux can pause a pane's output to a slow client (`%pause`, `%continue`, and `%extended-output` carrying an age).

> **Remember**
> This matters for file 47. A control-mode client receives the *program's* bytes, not tmux's rendering — so it must run **its own terminal emulator** to make sense of them. tmux has had a form of "send raw pane bytes to a smart client" for years. What it does *not* do is move ownership of the **layout** or the **session** to that client: window and pane structure still live in the tmux server and arrive as `%layout-change` notifications. And Ghostty itself carries a parser for this protocol (under `src/terminal/tmux` in its source tree, as Hashimoto notes) — and when a user in his thread called `-CC` "somewhat horrible" and noted iTerm2 depends on it, Hashimoto replied "I'm painfully aware," linking that directory.

The protocol is serviceable but has real friction: it's text-framed, so binary pane data pays an octal-escaping tax, and the same user's verdict was that `-CC` is "somewhat horrible." Take that as one experienced user's opinion rather than a measurement.

---

## 8. Common misconceptions

| Misconception | Reality |
|---|---|
| "tmux is a pipe between your terminal and the shell." | It's a full terminal emulator: parse → grid → regenerate → send (Section 3). |
| "The tmux client forwards your keystrokes to the server." | The client hands the server its terminal's file descriptor; the server reads and writes the terminal directly (Section 2). |
| "Output reaches your terminal unchanged." | The client terminal receives a stream *generated from tmux's grid* — never the program's own bytes (Section 4). |
| "tmux shrinks the window to the smallest client." | The default policy is `latest` — the most recently active client (Section 5). |
| "Kitty graphics don't work in tmux because your terminal is wrong." | tmux's emulator doesn't implement the protocol; `allow-passthrough` is an opt-in hack with real limits. |
| "The prefix key is a tmux-only convention with no deeper cause." | It's forced by in-band signaling — and `escape-time` is the same root cause. |
| "tmux can't send raw pane bytes to a client." | Control mode does — but layout and session ownership stay in the server. |
| "Multiplexer overhead makes everything slow." | Typing and normal output are unaffected; it shows up under sustained high-throughput output. |

---

## 9. Try this

> **Try this — the toy**
> Save Section 4.1's script and run `python3 toymux.py`. Change the `printf` payload (add a second color, or more lines than `ROWS`) and watch the regenerated stream change while the *visual* result stays the same.

> **Try this — the real thing**
> Run Section 4.2's commands against actual tmux and compare `pane.out` with `client.out`.

> **Try this — read the socket and the policy**
> ```
> tmux show -gw window-size
> ls -l "$(tmux display -p '#{socket_path}')"
> ```
> The first shows your window-size policy; the second shows the Unix domain socket with its leading `s` (file 27).

> **Try this — control mode**
> ```
> tmux -C new -s ctl
> ```
> Type `list-windows` and press Enter: the reply arrives inside `%begin … %end`. Press `Ctrl+D` on an empty line to leave. This is the protocol Section 7 describes, spoken by hand.

---

## 10. Sources

tmux source, `input.c` header comment (Williams-derived parser, the four listed changes, the 2007 copyright); tmux's commit history (the change passing the client's stdin/tty file descriptor to the server); the tmux wiki's *Control Mode* page (`%begin`/`%end`/`%output`, octal escaping, `pause-after`, "output generated by tmux itself … is not sent"); tmux's `window-size` option documentation; Zellij's `zellij-server` source layout (`panes/grid.rs`, `terminal_pane.rs`, `vte::Parser`); the Kitty-graphics-through-tmux passthrough notes; the video for the throughput claim (flagged unverified above) and the `src/terminal/tmux` reference. The toy in Section 4.1 was run; Section 4.2's experiment was **not** run, as stated.

---

## 11. Summary

- The tmux client hands the server its terminal's file descriptor; the server reads and writes that terminal directly. The client is thin because of *that*, not because it relays bytes.
- A multiplexer is a terminal emulator: tmux's parser descends from the same Williams machine as file 45, and Zellij's grid/`vte` design is the same shape.
- Output is parsed twice into two grids, with a regenerated stream in between — invisible in ordinary use, costly under sustained high-throughput output (magnitude unverified here).
- The server therefore owns the screen (shared scroll), the geometry (one PTY size; default `latest`), and the feature set (Kitty graphics need an opt-in passthrough hack).
- The prefix key and `escape-time` are both direct consequences of control and data sharing one stream.
- Control mode already sends raw pane bytes to a smart client — but layout and session ownership stay with the server.

---

## What's next

`47_SESSION_VIEW_SEPARATION.md` asks what a multiplexer would look like if the server owned only the *session* and each client owned its *view* — the thesis behind the new wave of multiplexers, and a careful look at what Superlogical's Rex claims, what has actually been shown, and what remains open.
