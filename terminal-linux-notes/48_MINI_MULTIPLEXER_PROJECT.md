# 48 — Mini Multiplexer Project

> **Where this fits in the stack**
> ```
> Files 02, 13, 14, 16, 23, 27, 28, 45-47  →  [ ASSEMBLED, BY YOU, IN CODE ]  →  a persistent terminal multiplexer
>                                              ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
>                                              this file
> ```
> File 34 built a shell. This file builds the other half of the picture: a **multiplexer** — `minimux` — in nine stages, each introducing one mechanism the handbook has already explained. It is a *teaching model*: small enough to read in one sitting, real enough that its tests attach to a live shell, kill clients, cut escape sequences in half, and resize a PTY. **All of the code in this file was run; the full test suite (53 checks) passes, and negative controls and mutation tests confirm the newest tests can fail.** Section 14 is candid about what those tests do and don't prove. Where something was *not* tested, the file says so.

---

## 1. What you are building

```
 your terminal ── minimux attach ─┐
                                   ├── [ Unix socket, framed messages ] ── SERVER (a daemon) ── PTY master ── sh -i
 another terminal ── attach ───────┘                                          └─ terminal model (file 45's state machine)
```

- A **session** is one shell on one PTY, owned by a **server** process that survives any client.
- A **client** attaches over a Unix domain socket, gets the current screen, then live output; its keystrokes go to the shell.
- `Ctrl+B` then `d` **detaches**; `Ctrl+B Ctrl+B` sends a literal `Ctrl+B` — the in-band prefix key from file 46, reproduced.

It's written in Python for brevity (the standard library's `pty`, `socket`, `select`, `termios` map one-to-one onto the C calls named in each stage). Written and tested on Linux (macOS: see file 49 and run `code/macos_probe.py` first). The same design translates directly to C if you want that as an exercise (Section 15).

| Stage | You build | Primitive | Files |
|---|---|---|---|
| 1 | A shell running on a PTY you own | `forkpty` = `openpty` + `fork` + `setsid` + `TIOCSCTTY` + `dup2` | 02, 08, 16 |
| 2 | A server on a Unix domain socket | `socket(AF_UNIX)`, `select` | 27, 13 |
| 3 | Message framing | Stream sockets have no message boundaries | 13, 30 |
| 4 | Daemonizing | Double fork + `setsid()` | 16, 28, 44 |
| 5 | A terminal model in the server | The parser state machine | 45 |
| 6 | A snapshot on attach | Consistent snapshots; parser state | 47 |
| 7 | Fan-out and input ordering | Tee raw bytes; one ordering point | 47 |
| 8 | Resize | `TIOCSWINSZ` → `SIGWINCH`; one PTY, one size | 14, 47 |
| 9 | The client | Raw mode; in-band prefix key | 23, 46 |

---

## 2. Stage 1 — Own a PTY

**Objective:** start a shell whose terminal is a PTY *you* hold the master end of.
**Primitive:** `pty.fork()` is `forkpty(3)`: it opens a PTY pair, forks, and in the child calls `setsid()`, makes the slave its controlling terminal (`TIOCSCTTY`), and `dup2`s it onto fds 0, 1 and 2 — exactly the sequence from files 02 and 16.

```python
pid, master = pty.fork()                                 # a PTY pair, shell on the slave (file 02)
if pid == 0:
    os.environ['TERM'] = 'xterm'
    os.execvp(cmd[0], cmd)
emu = Emu(); set_winsize(master, emu.rows, emu.cols)
```

In the parent, `master` is the fd you read the shell's output from and write its input to. Everything else in this project is built around that one file descriptor. Note that the *kernel's line discipline* (file 02) already sits on this PTY — it echoes typed characters and translates `\n` to `\r\n` before the server sees any output; the model in Stage 5 has to cope with that.

---

## 3. Stage 2 — A server on a Unix domain socket

**Objective:** accept clients and relay between them and the PTY.
**Primitive:** `socket(AF_UNIX, SOCK_STREAM)` bound to a filesystem path (file 27 — local-only by construction, nothing to do with the network stack), and `select()` to wait on the listener, the PTY master, and every client at once (the same multiplexing idea the whole project is named for, one level down).

```python
for s in select.select([lst, master] + list(clients), [], [])[0]:
    if s is lst:                                 # new client: snapshot first, then live bytes
        c, _ = lst.accept(); clients[c] = Reader()
        send(c, b'O', emu.snapshot(with_pending=not NO_PENDING))
```

`select` accepts a mix of raw file descriptors (the PTY master, an `int`) and socket objects. The socket file is created with mode `0600` inside a `0700` directory, so only your user can connect — the *only* access control this project has.

---

## 4. Stage 3 — Framing

**Objective:** send discrete messages over a byte stream.
**Primitive:** a `SOCK_STREAM` socket, like a pipe (file 13), delivers a stream of bytes with **no message boundaries** — one `send()` may arrive as two `recv()`s, or two `send()`s as one. So the protocol adds its own: a 1-byte type, a 4-byte big-endian length, then the payload.

```python
def send(sock, typ, payload=b''):
    sock.sendall(typ + struct.pack('>I', len(payload)) + payload)

class Reader:
    def __init__(self): self.buf = b''
    def feed(self, data):
        self.buf += data; frames = []
        while len(self.buf) >= 5:
            n = struct.unpack('>I', self.buf[1:5])[0]
            if len(self.buf) < 5 + n: break
            frames.append((self.buf[:1], self.buf[5:5 + n])); self.buf = self.buf[5 + n:]
        return frames
```

The message types: `I` input (client→server), `R` resize (client→server), `O` output (server→client), `X` session ended, `D` debug dump of the server's model (used by the tests), `M` a query for which terminal modes are active (used on detach, Stage 9).

---

## 5. Stage 4 — Daemonize

**Objective:** make the server survive your terminal closing.
**Primitive:** the double-fork recipe from file 28, in full.

```python
def start(name, cmd):
    os.makedirs(SOCKDIR, mode=0o700, exist_ok=True)
    if os.fork() > 0:                                        # the command you typed: wait for the socket, return
        for _ in range(100):
            if os.path.exists(sockpath(name)): print('started', name); return
            time.sleep(0.05)
        sys.exit('server did not start')
    os.setsid()                                              # new session, no controlling terminal (file 16)
    if os.fork() > 0: os._exit(0)                            # second fork: can never reacquire one (file 28)
    os.chdir('/'); fd = os.open(os.devnull, os.O_RDWR)
    for n in (0, 1, 2): os.dup2(fd, n)
    serve(name, cmd)
```

The first `fork` lets the command you typed return; `setsid()` detaches from your controlling terminal (file 16); the **second** `fork` produces a process that is *not* a session leader, so it can never reacquire a controlling terminal (file 28's point). The tests check all of it on the running server: no controlling terminal (`tty=?`), **not** a session leader, and adopted by PID 1 (`ppid=1` in my run — file 44's orphan adoption, observed).

> **Common mistake — mine, while writing the tests**
> My first assertion was that the daemon would be a session *leader* (`sid == pid`). The test failed with `sid=87 pid=88`: the daemon is a *member* of session 87, not its leader. The implementation was right and my expectation was backwards — a vivid reminder of why the second fork exists.

---

## 6. Stage 5 — A terminal model in the server

**Objective:** let the server know what the screen currently looks like.
**Primitive:** file 45's state machine, applied to every byte the shell writes. The `Emu` class holds a grid of `(character, style)` cells, a cursor, a style "pen" — and, critically, the **parser's own state** (`state`, `params`, and `seq`, the bytes of any half-received sequence). This is the multiplexer-is-a-terminal-emulator point from file 46, in miniature.

```python
def byte(self, b):
    if self.state != 'G': self.seq += bytes([b])
    if b == 0x1B:                                       # ESC cancels anything, anywhere
        self.state, self.params, self.seq = 'E', '', b'\x1b'; return
    if b in (0x18, 0x1A): self.ground(); return         # CAN / SUB
    if self.state != 'G' and b < 0x20 and self.state != 'S':
        self.ctl(b); self.seq = self.seq[:-1]; return   # C0 executes mid-sequence and is NOT part of it
    if self.state == 'G':
        if b < 0x20: self.ctl(b)
        elif b < 0x7F: self.put(chr(b))                 # (bytes >= 0x80 not modelled)
    elif self.state == 'E':
        if b == ord('['): self.state = 'C'
        elif b in b'P]X^_': self.state = 'S'            # DCS/OSC/SOS/PM/APC strings
        elif 0x20 <= b <= 0x2F: self.state = 'I'        # intermediate, e.g. ESC ( B  or  ESC # 8
        else:
            if b == ord('M'): self.ri()
            elif b == ord('D'): self.lf()
            elif b == ord('E'): self.col = 0; self.lf()
            elif b == ord('7'): self.cursor_saved = (self.row, self.col, self.pen)
            elif b == ord('8'): self.restore_cursor()
            elif b == ord('='): self.keypad = True
            elif b == ord('>'): self.keypad = False
            self.ground()
    elif self.state == 'I':
        if 0x30 <= b <= 0x7E or b >= 0x7F: self.ground()   # final byte of an ESC sequence we don't act on
    elif self.state == 'C':
        if b <= 0x3F: self.params += chr(b)             # parameters, private markers, intermediates
        elif b <= 0x7E: self.csi(chr(b)); self.ground()
    elif self.state == 'S':                             # swallow until BEL (ESC \ arrives via the ESC rule)
        if b == 0x07: self.ground()
```

What the model understands: printable ASCII; CR, LF, VT, FF, BS and TAB; SGR (one active style code); cursor movement and absolute positioning; erase in display and in line; **scroll regions** (`CSI t;b r`), with line feeds, reverse index (`ESC M`), `ESC D`/`ESC E` and scroll up/down all confined to the region; **insert and delete lines** (`CSI L`/`M`) and **characters** (`CSI @`/`P`/`X`); **save and restore cursor** (`ESC 7`/`ESC 8`); application keypad (`ESC =`/`ESC >`). It swallows OSC/DCS/APC strings, and ESC sequences with intermediates such as the `ESC ( B` that ncurses programs send. It follows file 45's rules that matter here: ESC cancels anything in progress, CAN/SUB cancel, and C0 controls execute even in the middle of a sequence — and, importantly, **are not part of the pending sequence afterwards**, or a snapshot replay would run them a second time (a bug the property test in Section 14 caught the moment it was possible to look for it).

These are sequences full-screen programs commonly rely on, which is why they are modelled; I have **not** run the real programs (Section 11). Several DEC *private modes* are tracked as well, because a client re-attaching to a running program has to be told about them:

```python
def private(self, f):                                   # CSI ? Pm h  /  CSI ? Pm l
    if f not in 'hl': return
    on = (f == 'h')
    for p in self.params[1:].split(';'):
        n = int(p) if p.isdigit() else 0
        if n in self.pm: self.pm[n] = on                # cursor keys, cursor visibility, mouse, bracketed paste
        elif n == 1049 and not NO_ALT: self.set_alt(on) # xterm: save cursor + alternate screen (cleared)
def set_alt(self, on):
    if on and not self.alt:
        self.saved, self.saved_margins = (self.grid, self.row, self.col, self.pen), (self.top, self.bot)
        self.grid, self.top, self.bot, self.alt = self.blank_grid(), 0, self.rows - 1, True
    elif not on and self.alt:
        self.grid, self.row, self.col, self.pen = self.saved; self.top, self.bot = self.saved_margins
        self.saved = self.saved_margins = None; self.alt = False
```

`CSI ? 1049 h/l` is xterm's **alternate screen** — what `vim`, `less` and `htop` use so that your shell's screen comes back untouched when they exit (file 24). The model keeps the *main* grid, cursor, style and scroll margins aside while the alternate screen is active and restores them on exit (resetting the margins on entry is my simplification; I have not checked it against xterm). The other tracked modes are `?1` (**application cursor keys**, which changes what the Up arrow sends — file 45 Section 8), `?25` (**cursor visibility**), the **mouse-reporting** modes `?1000`, `?1002`, `?1003`, `?1006`, **focus reporting** `?1004`, and `?2004` (**bracketed paste**). What it still does **not** understand is listed in Section 11.

---

## 7. Stage 6 — Snapshot on attach

**Objective:** a client that connects late sees the current screen, then live output.
**Primitive:** file 47's *consistent snapshot* — and its sharpest hazard: the cut can land **mid-sequence**.

```python
if s is lst:                                 # new client: snapshot first, then live bytes
    c, _ = lst.accept(); clients[c] = Reader()
    send(c, b'O', emu.snapshot(with_pending=not NO_PENDING))
```

The snapshot is *regenerated bytes* that make a fresh terminal display the server's grid: clear the screen, redraw each row with cursor positioning and style changes, put the cursor back:

```python
def snapshot(self, with_pending=True):
    """Bytes that make a fresh terminal show this screen - and, crucially, leave ITS parser in the same
    state as ours, by replaying any half-received sequence (file 47, Section 6.1)."""
    full = (0, self.rows - 1)
    out = b'\x1b[0m\x1b[r\x1b[2J\x1b[H'
    if self.alt:                                        # paint the MAIN screen first, then enter the alternate one,
        g, r, c, p = self.saved                         # so the client's terminal saves the right main screen
        out += self.rows_bytes(g)
        if self.saved_margins != full: out += b'\x1b[%d;%dr' % (self.saved_margins[0] + 1, self.saved_margins[1] + 1)
        out += self.cursor_bytes(r, c, p, g) + b'\x1b[?1049h\x1b[0m\x1b[2J\x1b[H'
    out += self.rows_bytes(self.grid)
    if self.cursor_saved: out += self.cursor_bytes(*self.cursor_saved, self.grid) + b'\x1b7'
    if (self.top, self.bot) != full: out += b'\x1b[%d;%dr' % (self.top + 1, self.bot + 1)
    out += self.cursor_bytes(self.row, self.col, self.pen, self.grid)
    for n, on in self.pm.items(): out += b'\x1b[?%d%s' % (n, b'h' if on else b'l')   # say every mode EXPLICITLY
    out += b'\x1b=' if self.keypad else b'\x1b>'
    if with_pending and self.state != 'G': out += self.seq
    return out
```

The last two lines are the point of this stage. If the shell was in the middle of writing an escape sequence when the client attached, the server's model is in a non-`ground` state, with `seq` holding the bytes received so far. The snapshot **replays those pending bytes after the screen**, so the client's real terminal ends up in the *same parser state* as the server's, ready to receive the rest of the sequence from the live stream.

### The test that matters

The suite types `printf '\033[3'; sleep 2; printf '1mred\033[0m\n'` into a live shell — which writes the first three bytes of `ESC[31m`, waits, then writes the rest — and attaches a second client during the sleep. Results, from the run:

| | Server mid-sequence? | Client's parser state after the snapshot | Client's final screen | Literal `1mred` printed? |
|---|---|---|---|---|
| **With** the pending-bytes replay | yes (`state=C params='3'`) | `('C', '3')` — matches | **identical** to the server's | no |
| **Without** it (`MINIMUX_NO_PENDING=1`) | yes | `('G', '')` — wrong | **diverges** | **yes**: `1mred` |

This is exactly the hazard file 47 derived — and tested only in a toy — now reproduced against a **live shell over a real socket**. Two honest framings: this is *my* design for a *model*, not a statement about how Rex handles it (the sources don't say); and the snapshot here is **regenerated** (tmux-like) while the stream after it is **raw** (Rex-like), a hybrid chosen because the client is the user's own real terminal, not an embedded emulator.

### The same idea for the alternate screen and modes

A client that attaches while `vim` is running must end up in the alternate screen *with the main screen preserved underneath*, or `vim`'s exit will restore garbage. The snapshot therefore **paints the main screen first, positions the main cursor, and only then sends `CSI ? 1049 h`** — so the client's terminal saves the right main screen when it switches — and then paints the alternate screen. It also carries the **scroll margins**, the **saved cursor** (`ESC 7`), the **keypad mode**, and every tracked mode **explicitly, on or off**, so a terminal never inherits a stale setting. A cursor in a **pending wrap** (just past the last column) cannot be placed with a positioning command, so the snapshot re-writes the last cell, which puts the terminal back into that state.

The suite drives this with a scripted stand-in for such a program — it prints `main_line`, enters the alternate screen, draws `ALT_SCREEN`, sleeps, leaves, and prints `back_on_main` (markers are built with `printf` arguments so the shell's *echo* of the command line can never contain them):

| Check | Result |
|---|---|
| Server tracks that the program entered the alternate screen | pass |
| A client attaching **mid-program** is put in the alternate screen too, seeing `ALT_SCREEN` with `main_line` preserved underneath | pass |
| The snapshot bytes paint the main screen **before** `CSI ? 1049 h`, and `ALT_SCREEN` after it | pass |
| **Ground truth:** after the program exits, a fresh client sees `main_line` and `back_on_main` and **no** `ALT_SCREEN` | pass |
| **Control** — with alternate-screen tracking switched off: the main screen is **lost** and the program's text is left behind (`ALT_SCREENback_on_main`) | confirmed — the bug the feature fixes |
| A client attaching mid-program is told cursor-keys and bracketed paste are **on**; after the program resets them, a new client is told they are **off** | pass |

---

## 8. Stage 7 — Fan-out and input ordering

**Objective:** several clients see the same session; any of them can type.
**Primitive:** file 47's *tee* and its single ordering point.

```python
elif s == master:                            # the program wrote something
    try: data = os.read(master, 65536)
    except OSError: data = b''               # EIO: the child closed the slave, i.e. exited
    if not data:
        for c in list(clients):
            try: send(c, b'X')
            except OSError: pass
        return
    emu.feed(data)
    for c in list(clients):                  # fan out the RAW bytes (file 47, 6.2)
        try: send(c, b'O', data)
        except OSError: drop(c)
```

Output is fanned out as the **raw bytes**, unmodified, to every client after it has updated the server's own model — the server runs one emulator (authoritative), each client's terminal runs another (a replica), and determinism (file 45) keeps them equal. Input goes the other way, through one loop:

```python
for typ, pl in clients[s].feed(data):
    if typ == b'I': os.write(master, pl)               # input: ordered by this one loop
    elif typ == b'R':
        rows, cols = struct.unpack('>HH', pl)
        emu = emu.resized(rows, cols); set_winsize(master, rows, cols)
    elif typ == b'D': send(s, b'D', repr(emu.dump()).encode())   # debug: dump the model
```

Because one loop handles one frame at a time, **a frame is never interleaved with another client's frame**. What this does *not* give you is ordering at the granularity of *key events*: a frame is whatever one `read()` on the client's keyboard returned, so a very large paste can be split across frames and interleaved with a second person's typing. That is file 47's input-granularity question (Section 6.3), reproduced rather than solved.

**Tested:** two clients attached to one session; each saw what the other typed, and both replicas matched the server's model.

---

## 9. Stage 8 — Resize

**Objective:** make the shell's idea of its window size match a client's.
**Primitive:** `TIOCSWINSZ` on the PTY master sets the PTY's size; the kernel then sends `SIGWINCH` to the PTY's foreground process group (file 14), and the *program* redraws itself — the only thing that can.

```python
def set_winsize(fd, rows, cols):                            # TIOCSWINSZ: the kernel then sends SIGWINCH (file 14)
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack('HHHH', rows, cols, 0, 0))
```
```python
def resized(self, rows, cols):                          # crop or pad; the program then redraws itself
    def fit(g):
        ng = [[(' ', 0)] * cols for _ in range(rows)]
        for r in range(min(rows, self.rows)):
            for c in range(min(cols, self.cols)): ng[r][c] = g[r][c]
        return ng
    e = Emu(rows, cols)
    e.grid = fit(self.grid)
    e.row, e.col = min(self.row, rows - 1), min(self.col, cols - 1)
    e.pen, e.state, e.params, e.seq = self.pen, self.state, self.params, self.seq
    e.alt, e.pm, e.keypad = self.alt, dict(self.pm), self.keypad
    if self.saved:
        g, r, c, p = self.saved; e.saved, e.saved_margins = (fit(g), min(r, rows - 1), min(c, cols - 1), p), (0, rows - 1)
    if self.cursor_saved:
        r, c, p = self.cursor_saved; e.cursor_saved = (min(r, rows - 1), min(c, cols - 1), p)
    return e                                            # (margins reset on resize, as xterm does)
```

The server applies each `R` message it receives to both the PTY and its own model (cropping or padding the grid). **Tested:** after a client reports 30×100, a `stty size` typed into the shell prints `30 100`, and the server's model has 30 rows and 100 columns.

> **Remember**
> The policy here is "**the last client to report a size wins**" — like tmux's `latest` (file 46). Two clients with *different* terminal sizes will fight, because there's only one PTY size and `minimux` has no per-client viewport. That is file 47 Section 3 — geometry is per-session, and only layout/viewport can be per-client — as an unfixed limitation you can feel. I did not test two clients of different sizes.

---

## 10. Stage 9 — The client

**Objective:** a real, interactive client.
**Primitive:** raw mode (file 23) so every key reaches the server unprocessed; a prefix key handled **in-band** (file 46, Section 6); and `SIGWINCH` handled the way file 14 warns — the handler only sets a flag, and the main loop does the work.

```python
def attach(name):
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM); s.connect(sockpath(name))
    fd = sys.stdin.fileno(); saved = termios.tcgetattr(fd)
    resize = [True]                                          # set by the handler; acted on in the main loop
    signal.signal(signal.SIGWINCH, lambda *_: resize.__setitem__(0, True))
    tty.setraw(fd)                                           # raw mode (file 23): every key straight through
    reader, prefix, done, why = Reader(), False, False, 'detached'
    try:
        while not done:
            if resize[0]:
                resize[0] = False
                rows, cols = struct.unpack('HH', fcntl.ioctl(1, termios.TIOCGWINSZ, b'\0' * 8)[:4])
                send(s, b'R', struct.pack('>HH', rows, cols))
            ready = select.select([fd, s], [], [], 0.2)[0]
            if fd in ready:
                keys = os.read(fd, 4096); fwd = b''
                if not keys: break
                for b in keys:
                    if prefix:                               # the byte after the prefix is for US (file 46, in-band)
                        prefix = False
                        if b == ord('d'): done = True; break
                        if b == 0x02: fwd += b'\x02'         # prefix twice = a literal Ctrl+B
                    elif b == 0x02: prefix = True
                    else: fwd += bytes([b])
                if fwd: send(s, b'I', fwd)
            if s in ready:
                data = s.recv(65536)
                if not data: why = 'server closed'; break
                for typ, pl in reader.feed(data):
                    if typ == b'O': os.write(1, pl)
                    elif typ == b'X': why = 'session ended'; done = True
    finally:
        if why == 'detached': reset_modes(s, reader)
        termios.tcsetattr(fd, termios.TCSAFLUSH, saved); print('\r\n[%s]' % why)
```

On a detach, the client first undoes whatever the program switched on, so it doesn't leak into **your** terminal:

```python
def reset_modes(s, reader):
    """On detach, undo whatever the program switched on, so it doesn't leak into YOUR terminal."""
    try:
        send(s, b'M'); s.settimeout(0.5); undo = None
        while undo is None:
            data = s.recv(65536)
            if not data: return
            for typ, pl in reader.feed(data):
                if typ == b'O': os.write(1, pl)               # output that was already in flight
                elif typ == b'M': undo = pl                   # the server says exactly what to undo
        os.write(1, undo)
    except OSError:                                          # includes the socket timeout
        pass
```

The server computes the undo bytes itself (`Emu.undo_bytes`): leave the alternate screen, switch off the tracked modes, show the cursor again, return the keypad to numeric, and reset any scroll margins — saving and restoring the cursor around that reset so it doesn't move. The client needs no knowledge of which modes exist.

Without this, detaching from a session running `vim` would leave your terminal stuck in the alternate screen with the program's key modes still active.

Pressing `Ctrl+B` makes the *next* key belong to the client: `d` detaches, a second `Ctrl+B` forwards a literal `Ctrl+B`. This is the same in-band ambiguity as tmux's prefix, and the same fix. The loop wakes every 0.2 seconds to notice a pending resize — the simple alternative to a self-pipe or a wakeup file descriptor.

**Tested, through a real pty** (the harness spawns the actual `attach` command attached to a pty and types into it): typed keystrokes round-trip; `Ctrl+B Ctrl+B` makes `cat -v` print `^B`; `Ctrl+B d` exits the client cleanly with status 0 while the session keeps running; a *new* client attaches and sees the earlier session. A further test attaches the real client mid-way through a program that has enabled the alternate screen, application cursor keys and bracketed paste — the harness's terminal ends up in all three — then detaches: afterwards all three are **off**. Removing the reset call in a scratch copy makes exactly that check fail, so the test is real.

> **Not tested:** using the client by hand in an interactive terminal. The pty-driven harness exercises the same code path, but it is not a person at a keyboard. If a client is killed with `SIGKILL`, its `finally` block never runs and your terminal stays in raw mode — run `reset` (file 23).

---

## 11. What it deliberately does not do

Read this before extending it — most of these are the interesting exercises.

- **Only some of the terminal is modelled.** The sequences listed in Stage 5 are; **not** modelled are the older `?47`/`?1047` screen switches, autowrap (`?7`) and origin (`?6`) modes, tab-stop setting, character sets, double-width/height lines, and the many other modes and sequences real terminals support. A program that depends on them can still leave the model — and so a reattached terminal — wrong.
- **Not tested with the real programs.** My sandbox has no `vim`, `less`, `htop` or `top` in a usable form, so the alternate-screen and scroll-region tests use scripted programs that emit the same sequences — which proves the mechanism, not that those programs will behave. **Running real `vim` inside `minimux` on a Mac is the experiment I have not done.**
- **ASCII only**; **one** SGR style code at a time; no wide characters (file 45 Section 8, file 24 Section 6).
- **No scrollback**, no copy mode, no search.
- **No backpressure.** `send()` uses `sendall` on a blocking socket, so one stalled client can stall the server — file 47's open question about slow clients, left open. (By code inspection; I did not test a stalled client.)
- **No divergence detection** (file 47, Section 9) — a client could be silently wrong.
- **No layout at all:** one session, one PTY; no windows or panes.
- **Security is just filesystem permissions** (`0600` socket, `0700` directory). It is local-only by construction; for remote use, run it over SSH (file 26), exactly as file 27 describes for tmux.

---

## 12. minimux, tmux, and Rex — where it sits

| | tmux (file 46) | **minimux** | Rex, as reported (file 47) |
|---|---|---|---|
| Server runs a terminal model | yes | **yes** | yes (authoritative) |
| What a new client first receives | A regenerated screen | **A regenerated screen** (+ pending bytes) | A snapshot, then raw bytes |
| What it receives afterward | Regenerated updates | **Raw PTY bytes** | Raw PTY bytes |
| Client-side emulator | None (your terminal only) | None (your terminal is the replica) | Embedded libghostty |
| Geometry | One PTY size; policy `latest` | **One PTY size; last client wins** | One canonical grid per PTY |
| Layout | Server | **None** | Client |
| Transport | Unix socket (+SSH) | **Unix socket (+SSH)** | Custom binary protocol |

`minimux` is a hybrid: tmux-style regenerated snapshot, Rex-style raw tee. It is *not* an implementation of either design, and it would need a per-client viewport and an embedded emulator to become one.

---

## 13. Running it

```bash
python3 minimux.py start work          # start a session in the background
python3 minimux.py attach work         # attach; type commands; Ctrl+B then d to detach
python3 minimux.py attach work         # attach again (or from a second terminal): same screen, same shell
python3 minimux.py kill work           # stop it
```

Set `MINIMUX_DIR` to change where sockets live (default `/tmp/minimux-<uid>`). Run the suite with `python3 test_minimux.py`. The files are in the `code/` folder next to this one. **On macOS**, run `python3 macos_probe.py` first and read file 49; the suite is portable (it prefers `dash`, accepts macOS's `??` tty spelling, and uses `pgrep -P`), and `MINIMUX_TEST_SHELL` selects the shell.

---

## 14. Test results — and what they do and don't prove

The complete output of the suite, as run:

```
platform: linux | python 3.12.3 | session shell: dash -i
--- 1. persistence: the session outlives its client -----------------------------------------
PASS  shell output reaches the client
PASS  server process survives the client disconnecting
PASS  the shell (its child) is still running   child pids=['99']
PASS  server has NO controlling terminal (file 16)   tty=?
PASS  server is NOT a session leader - the double fork's purpose (file 28)   sid=97 pid=98 ppid=1
--- 2. a late client gets the screen back, and converges --------------------------------------
PASS  snapshot restores earlier output on reattach   '# echo persist_me|persist_me|#'
PASS  client replica == server model after more output
--- 3. fan-out: two clients, input from either ------------------------------------------------
PASS  client C sees what B typed
PASS  client B sees what C typed
PASS  both replicas equal the server model
--- 4. resize: one PTY, one size ----------------------------------------------------------------
PASS  the PROGRAM sees the new size (via TIOCSWINSZ)
PASS  server model resized to 30 rows   rows=30 cols=100
PASS  kill removes socket and pid file
--- 5. the hazard from file 47: a snapshot cut MID-SEQUENCE, on a live shell --------------------
PASS  server really was mid-sequence when the client attached   state=C params='3'
PASS  WITH the fix: client terminal ends in the same parser state   client state after snapshot=('C', '3')
PASS  WITH the fix: client screen == server model at the end
PASS  WITHOUT the fix: client terminal starts in ground state   client state after snapshot=('G', '')
PASS  WITHOUT the fix: client screen DIVERGES from the server
PASS  WITHOUT the fix: the sequence tail is printed as literal text   "sleep 2; printf '1mred\\033[0m\\n'|1mred|#"
--- 6. the real interactive client, driven through a pty ---------------------------------------
PASS  keystrokes travel client -> server -> shell and back   '# echo viapty|viapty|#'
PASS  prefix pressed twice sends a literal Ctrl+B (shown as ^B)   '# echo viapty|viapty|# cat -v|^B|^B'
PASS  Ctrl+B then d detaches the client cleanly
PASS  ...and the session keeps running
PASS  a NEW client attaches and sees the earlier session   '# echo viapty|viapty|# cat -v|^B|^B|#'
--- 7. the alternate screen: what vim, less and htop do ------------------------------------------
PASS  server tracks that the program entered the alternate screen
PASS  a client attaching MID-program is put in the alternate screen too
PASS  ...which shows the program's screen   'ALT_SCREEN'
PASS  ...while the MAIN screen is preserved underneath   "# printf 'main_%s\\n' line; printf '\\033[?1049h\\033[2J\\033[1;"
PASS  snapshot paints the main screen BEFORE entering the alternate one
PASS  after the program exits, client screen == server model
PASS  GROUND TRUTH: a fresh client sees the main screen restored   "# printf 'main_%s\\n' line; printf '\\033[?1049h\\033[2J\\033[1;1HALT_%s' "
PASS  WITHOUT alt tracking: the main screen is LOST and the program's text is left behind   'ALT_SCREENback_on_main|#'
--- 8. modes a re-attaching terminal must be told about -----------------------------------------
PASS  snapshot turns application cursor keys ON for a client attaching mid-program
PASS  snapshot turns bracketed paste ON as well
PASS  after the program resets them, a new client is told cursor-keys are OFF
PASS  ...and is told bracketed paste is whatever the shell has left it as (dash: off; bash's readline turns it back ON at the prompt)   model says bpaste=False
--- 9. detaching must not leave YOUR terminal in the program's modes -------------------------------
PASS  a real client attaching mid-program lands in the alternate screen with the modes on   alt=True app_cursor=True bpaste=True
PASS  after Ctrl+B d the client has put the terminal back (alternate screen and modes OFF)   alt=False app_cursor=False bpaste=False
--- 10. the model against VT semantics derived by hand (no server involved) ---------------------
PASS  LF at the BOTTOM margin scrolls only the region   ['L1', 'L3', 'L4', 'L5', '', 'L6']
PASS  LF BELOW the region does not scroll anything   ['L1', 'L2', 'L3', 'L4', 'L5', 'L6']
PASS  reverse index at the TOP margin scrolls the region down   ['L1', '', 'L2', 'L3', 'L4', 'L6']
PASS  insert 2 lines pushes the rest down (and off the bottom)   ['L1', 'L2', '', '', 'L3', 'L4']
PASS  delete 2 lines pulls the rest up and blanks the bottom   ['L1', 'L4', 'L5', 'L6', '', '']
PASS  insert 2 characters shifts the line right   'AB  CDEFGH'
PASS  delete 2 characters shifts the line left   'ABEFGH'
PASS  erase 2 characters blanks them in place   'AB  EFGH'
PASS  ESC ( B (charset select, emitted by ncurses) is swallowed, not printed   'xy'
PASS  ESC 7 / ESC 8 save and restore cursor and pen   (2, 3, 31)
PASS  a control character inside a CSI runs once and is NOT part of the pending sequence   row=1 seq=b'\x1b[2'
PASS  hidden cursor, mouse modes and application keypad are all carried by a snapshot
PASS  ...and undone on detach (cursor shown again, mouse off, keypad numeric)   b'\x1b[?25h\x1b[?1000l\x1b[?1006l\x1b>'
PASS  a snapshot taken at a PENDING WRAP (cursor past the last column) still wraps correctly   ['0123456789', 'Z']
--- 11. property test: a snapshot at ANY offset reproduces the model --------------------------------
PASS  400 random streams x up to 14 cut points: snapshot + remainder == uninterrupted replay   5218 trials

53/53 checks passed
```

**What these 53 checks establish.** The server and its daemon properties; persistence across client disconnects; that a late client's screen *equals the server's model*; fan-out and input from either client; resize reaching the program; the mid-sequence and alternate-screen behaviours **against ground truth derived from what the program means to do** (not from the model); that the real client works through a real pty; and, via a negative control, that the detach-cleanup test can fail.

**What they do not establish — read this.**

- In the comparisons that say "client screen == server model", the client's "terminal" is **the same `Emu` class the server uses**. That shows the *protocol is self-consistent* under this model; it does **not** show that xterm, Ghostty or Terminal.app would render the snapshot identically. My sandbox had no independent emulator (`pyte`, tmux, screen and xterm were all unavailable), so I could not cross-check. The snapshot uses only ordinary sequences (cursor positioning, erase, style, the private modes above), so I expect real terminals to agree — but that is an expectation, not a result. **Exercise 3 below closes this gap if you have a real terminal.**
- The ground-truth checks (the main screen restored, `1mred` not printed literally) are independent of the model — they are why the alternate-screen and mid-sequence results are worth trusting more than the "== server model" ones.
- No stalled client, no two clients of different sizes, no long-running session, no large output, and no UTF-8 were tested.

**The model-level tests (sections 10 and 11 of the suite).** Fourteen targeted checks cover the newer sequences. Nine of them assert expected screens for scroll regions, insert/delete, character editing and `ESC ( B`, with the expected results **derived by hand from the VT rules** rather than from the model; the other five cover the saved cursor, control characters inside sequences, carrying and undoing the tracked modes, and the pending wrap. A property test generates 400 random streams (mixing text, cursor movement, scroll regions, insert/delete, saved cursor, alternate-screen toggles, modes, and control characters *inside* escape sequences), takes a snapshot at up to 14 random byte offsets in each, and requires that *snapshot plus the rest of the stream* equals an uninterrupted replay — 5,218 trials with the fixed random seed. It is a self-consistency test (the replica is the same model), so it shows the snapshot loses nothing the model tracks, not that the model matches a real terminal.

**Mutation tests.** Because a property test that passes first time deserves suspicion, I re-introduced seven bugs one at a time in scratch copies and confirmed each is caught: keeping a mid-sequence control character in the pending sequence; not re-creating a pending wrap; not recognising `ESC ( B`; not carrying the saved cursor; not carrying the main screen's margins while the alternate screen is active; ignoring the scroll region on line feed; and not carrying the tracked modes. **All seven were caught**, most by a check written for that bug and several also by the property test.

---

## 15. Exercises

1. **Scrollback.** Keep lines that scroll off the top, and include some in the snapshot.
2. **More of the terminal.** Add autowrap (`?7`), origin mode (`?6`), tab stops, character sets, and the older `?47`/`?1047` screen switches — and extend the property test's generator so it exercises them. (Mouse reporting, cursor visibility and scroll regions are already modelled.)
3. **Check against a real terminal.** Add a `--dump-snapshot FILE` option, save a snapshot taken mid-`vim`, then `cat` the file into a real terminal (or into tmux) and compare what it shows to what `vim` was displaying. This is the cross-check my sandbox couldn't do.
4. **A viewport.** Stop clients from resizing the PTY; give each client a crop of one canonical grid (file 47 Section 3).
5. **Divergence detection.** Add sequence numbers (or a periodic checksum of the grid) so a client can tell its replica went wrong — the gap file 47's Case 4b exposed.
6. **Backpressure.** Make client sockets non-blocking; give each a bounded queue; when it overflows, drop the client's backlog and send a fresh snapshot instead.
7. **Input at event granularity.** Buffer each client's input until a whole key event or a whole bracketed paste has arrived before writing it.
8. **Panes.** Several PTYs in one server, with a client that composes them — layout in the client (file 47).
9. **Rewrite the server in C** with `posix_openpt`/`grantpt`/`unlockpt`, `fork`, `setsid`, `poll`, and `SCM_RIGHTS` — then try tmux's trick from file 46 Section 2 and pass the client's terminal fd to the server instead of relaying its bytes.
10. **A control-mode protocol.** Add a text protocol with `%output` lines (file 46 Section 7) alongside the binary one.

---

## 16. Sources and full code

Design from files 02, 13, 14, 16, 23, 27, 28, 44, 45, 46 and 47; the system calls behind `pty.fork` from the `forkpty(3)` interface. Everything in this file was run on Linux with Python 3.12; the suite is in `code/test_minimux.py`. The complete program, exactly as tested:

```python
#!/usr/bin/env python3
"""minimux - a tiny persistent terminal multiplexer. One session = one PTY.
Developed and tested on Linux. On macOS, run code/macos_probe.py first (see file 49).

  minimux.py start  NAME [cmd...]   start a session server in the background (default: sh -i)
  minimux.py attach NAME            attach; Ctrl+B then d detaches; Ctrl+B Ctrl+B sends a literal Ctrl+B
  minimux.py kill   NAME            stop the session

A teaching model. The server's terminal model understands only what a plain shell session
needs (printables, CR/LF/BS/TAB, SGR, cursor moves, erase) - see file 48 for what is not modelled."""
import os, sys, pty, socket, select, struct, fcntl, termios, tty, signal, time

SOCKDIR = os.environ.get('MINIMUX_DIR', '/tmp/minimux-%d' % os.getuid())
NO_PENDING = bool(os.environ.get('MINIMUX_NO_PENDING'))    # test switch: disable the pending-sequence fix
NO_ALT = bool(os.environ.get('MINIMUX_NO_ALT'))            # test switch: disable alternate-screen tracking

def sockpath(name): return os.path.join(SOCKDIR, name + '.sock')
def pidpath(name):  return os.path.join(SOCKDIR, name + '.pid')

# ---- framing: a stream socket has no message boundaries (file 13), so we add them -------------
def send(sock, typ, payload=b''):
    sock.sendall(typ + struct.pack('>I', len(payload)) + payload)

class Reader:
    def __init__(self): self.buf = b''
    def feed(self, data):
        self.buf += data; frames = []
        while len(self.buf) >= 5:
            n = struct.unpack('>I', self.buf[1:5])[0]
            if len(self.buf) < 5 + n: break
            frames.append((self.buf[:1], self.buf[5:5 + n])); self.buf = self.buf[5 + n:]
        return frames

# ---- the terminal model: grid + cursor + pen + PARSER STATE (file 45) --------------------------
class Emu:
    # Private modes whose state a re-attaching terminal must be told about (number -> default).
    TRACKED = {1: False, 25: True, 1000: False, 1002: False, 1003: False, 1004: False, 1006: False, 2004: False}

    def __init__(self, rows=24, cols=80):
        self.rows, self.cols = rows, cols
        self.grid = self.blank_grid()
        self.row = self.col = self.pen = 0
        self.state, self.params, self.seq = 'G', '', b''   # seq = the bytes of the sequence in progress
        self.top, self.bot = 0, rows - 1                    # scroll region, inclusive, 0-based
        self.alt, self.saved, self.saved_margins = False, None, None
        self.pm = dict(self.TRACKED)                        # tracked DEC private modes
        self.keypad = False                                 # application keypad (ESC =) vs numeric (ESC >)
        self.cursor_saved = None                            # DECSC (ESC 7): (row, col, pen)

    @property
    def app_cursor(self): return self.pm[1]
    @property
    def bpaste(self): return self.pm[2004]
    def blank_line(self): return [(' ', 0)] * self.cols
    def blank_grid(self): return [self.blank_line() for _ in range(self.rows)]

    # -- scrolling, always confined to the scroll region ------------------------------------------
    def scroll_up(self, n=1):
        for _ in range(min(n, self.bot - self.top + 1)):
            del self.grid[self.top]; self.grid.insert(self.bot, self.blank_line())
    def scroll_down(self, n=1):
        for _ in range(min(n, self.bot - self.top + 1)):
            del self.grid[self.bot]; self.grid.insert(self.top, self.blank_line())
    def lf(self):                                           # LF, VT, FF, IND
        if self.row == self.bot: self.scroll_up()
        elif self.row < self.rows - 1: self.row += 1
    def ri(self):                                           # reverse index
        if self.row == self.top: self.scroll_down()
        elif self.row > 0: self.row -= 1
    def put(self, ch):
        if self.col >= self.cols: self.col = 0; self.lf()   # a pending wrap happens now
        self.grid[self.row][self.col] = (ch, self.pen); self.col += 1
    def ctl(self, b):                                       # C0 controls
        if b == 0x0D: self.col = 0
        elif b in (0x0A, 0x0B, 0x0C): self.lf()
        elif b == 0x08: self.col = max(0, self.col - 1)
        elif b == 0x09: self.col = min(self.cols - 1, (self.col // 8 + 1) * 8)
    def ground(self): self.state, self.params, self.seq = 'G', '', b''
    def restore_cursor(self):
        if self.cursor_saved: self.row, self.col, self.pen = self.cursor_saved
        else: self.row = self.col = self.pen = 0

    # -- the parser (file 45) -----------------------------------------------------------------------
    def feed(self, data):
        for b in data: self.byte(b)
    def byte(self, b):
        if self.state != 'G': self.seq += bytes([b])
        if b == 0x1B:                                       # ESC cancels anything, anywhere
            self.state, self.params, self.seq = 'E', '', b'\x1b'; return
        if b in (0x18, 0x1A): self.ground(); return         # CAN / SUB
        if self.state != 'G' and b < 0x20 and self.state != 'S':
            self.ctl(b); self.seq = self.seq[:-1]; return   # C0 executes mid-sequence and is NOT part of it
        if self.state == 'G':
            if b < 0x20: self.ctl(b)
            elif b < 0x7F: self.put(chr(b))                 # (bytes >= 0x80 not modelled)
        elif self.state == 'E':
            if b == ord('['): self.state = 'C'
            elif b in b'P]X^_': self.state = 'S'            # DCS/OSC/SOS/PM/APC strings
            elif 0x20 <= b <= 0x2F: self.state = 'I'        # intermediate, e.g. ESC ( B  or  ESC # 8
            else:
                if b == ord('M'): self.ri()
                elif b == ord('D'): self.lf()
                elif b == ord('E'): self.col = 0; self.lf()
                elif b == ord('7'): self.cursor_saved = (self.row, self.col, self.pen)
                elif b == ord('8'): self.restore_cursor()
                elif b == ord('='): self.keypad = True
                elif b == ord('>'): self.keypad = False
                self.ground()
        elif self.state == 'I':
            if 0x30 <= b <= 0x7E or b >= 0x7F: self.ground()   # final byte of an ESC sequence we don't act on
        elif self.state == 'C':
            if b <= 0x3F: self.params += chr(b)             # parameters, private markers, intermediates
            elif b <= 0x7E: self.csi(chr(b)); self.ground()
        elif self.state == 'S':                             # swallow until BEL (ESC \ arrives via the ESC rule)
            if b == 0x07: self.ground()

    def nums(self):                                         # the parameters as ints; a blank is 0
        if not self.params: return []
        return [int(''.join(c for c in p if c.isdigit()) or 0) for p in self.params.split(';')]
    def args(self, default):
        return [x or default for x in self.nums()] or [default]
    def csi(self, f):
        if self.params[:1] == '?': self.private(f); return  # DEC private modes: some are modelled
        if self.params[:1] in ('>', '<', '='): return       # other private-marker sequences: not modelled
        n = self.args(1)[0]; c0 = min(self.col, self.cols - 1)
        if f == 'm':
            for p in (self.params.split(';') if self.params else ['0']):
                self.pen = int(p) if p.isdigit() else 0      # model keeps ONE active SGR code (0 = reset)
        elif f in 'Hf':
            a = self.args(1) + [1]
            self.row, self.col = min(a[0], self.rows) - 1, min(a[1], self.cols) - 1
        elif f == 'A': self.row = max(0, self.row - n)
        elif f == 'B': self.row = min(self.rows - 1, self.row + n)
        elif f == 'C': self.col = min(self.cols - 1, c0 + n)
        elif f == 'D': self.col = max(0, c0 - n)
        elif f == 'G': self.col = min(n, self.cols) - 1
        elif f == 'd': self.row = min(n, self.rows) - 1
        elif f == 'J':
            k = self.args(0)[0]
            if k in (2, 3): self.grid = self.blank_grid()
            elif k == 0:
                for c in range(c0, self.cols): self.grid[self.row][c] = (' ', 0)
                for r in range(self.row + 1, self.rows): self.grid[r] = self.blank_line()
            elif k == 1:
                for r in range(self.row): self.grid[r] = self.blank_line()
                for c in range(c0 + 1): self.grid[self.row][c] = (' ', 0)
        elif f == 'K':
            lo, hi = {0: (c0, self.cols), 1: (0, c0 + 1), 2: (0, self.cols)}.get(self.args(0)[0], (0, 0))
            for c in range(lo, hi): self.grid[self.row][c] = (' ', 0)
        elif f == 'r':                                      # set scroll region; homes the cursor
            v = self.nums(); t = v[0] if len(v) > 0 and v[0] else 1; b = v[1] if len(v) > 1 and v[1] else self.rows
            if 1 <= t < b <= self.rows: self.top, self.bot = t - 1, b - 1; self.row = self.col = 0
        elif f == 'L' and self.top <= self.row <= self.bot:  # insert lines
            for _ in range(min(n, self.bot - self.row + 1)): del self.grid[self.bot]; self.grid.insert(self.row, self.blank_line())
            self.col = 0
        elif f == 'M' and self.top <= self.row <= self.bot:  # delete lines
            for _ in range(min(n, self.bot - self.row + 1)): del self.grid[self.row]; self.grid.insert(self.bot, self.blank_line())
            self.col = 0
        elif f == 'S': self.scroll_up(n)
        elif f == 'T': self.scroll_down(n)
        elif f == '@':                                      # insert blank characters
            line = self.grid[self.row]; self.grid[self.row] = (line[:c0] + [(' ', 0)] * n + line[c0:])[:self.cols]
        elif f == 'P':                                      # delete characters
            line = self.grid[self.row]; self.grid[self.row] = (line[:c0] + line[c0 + n:] + [(' ', 0)] * n)[:self.cols]
        elif f == 'X':                                      # erase characters
            for c in range(c0, min(self.cols, c0 + n)): self.grid[self.row][c] = (' ', 0)

    def private(self, f):                                   # CSI ? Pm h  /  CSI ? Pm l
        if f not in 'hl': return
        on = (f == 'h')
        for p in self.params[1:].split(';'):
            n = int(p) if p.isdigit() else 0
            if n in self.pm: self.pm[n] = on                # cursor keys, cursor visibility, mouse, bracketed paste
            elif n == 1049 and not NO_ALT: self.set_alt(on) # xterm: save cursor + alternate screen (cleared)
    def set_alt(self, on):
        if on and not self.alt:
            self.saved, self.saved_margins = (self.grid, self.row, self.col, self.pen), (self.top, self.bot)
            self.grid, self.top, self.bot, self.alt = self.blank_grid(), 0, self.rows - 1, True
        elif not on and self.alt:
            self.grid, self.row, self.col, self.pen = self.saved; self.top, self.bot = self.saved_margins
            self.saved = self.saved_margins = None; self.alt = False

    # -- snapshots ------------------------------------------------------------------------------------
    def rows_bytes(self, grid):                             # bytes that paint one grid onto a cleared screen
        out = b''
        for r, line in enumerate(grid):
            last = max((c for c, (ch, _) in enumerate(line) if ch != ' '), default=-1)
            if last < 0: continue
            out += b'\x1b[%d;1H' % (r + 1)
            cur = 0
            for ch, p in line[:last + 1]:
                if p != cur: out += (b'\x1b[%dm' % p) if p else b'\x1b[0m'; cur = p
                out += ch.encode()
            if cur: out += b'\x1b[0m'
        return out
    def cursor_bytes(self, r, c, p, grid):
        """Place the cursor and pen. A pending wrap (c == cols) can only be re-created by writing the last
        cell again, so that is what we do."""
        pen = lambda q: (b'\x1b[%dm' % q) if q else b'\x1b[0m'
        if c >= self.cols:
            ch, cp = grid[r][self.cols - 1]
            return b'\x1b[%d;%dH' % (r + 1, self.cols) + pen(cp) + ch.encode() + pen(p)
        return b'\x1b[%d;%dH' % (r + 1, c + 1) + pen(p)
    def snapshot(self, with_pending=True):
        """Bytes that make a fresh terminal show this screen - and, crucially, leave ITS parser in the same
        state as ours, by replaying any half-received sequence (file 47, Section 6.1)."""
        full = (0, self.rows - 1)
        out = b'\x1b[0m\x1b[r\x1b[2J\x1b[H'
        if self.alt:                                        # paint the MAIN screen first, then enter the alternate one,
            g, r, c, p = self.saved                         # so the client's terminal saves the right main screen
            out += self.rows_bytes(g)
            if self.saved_margins != full: out += b'\x1b[%d;%dr' % (self.saved_margins[0] + 1, self.saved_margins[1] + 1)
            out += self.cursor_bytes(r, c, p, g) + b'\x1b[?1049h\x1b[0m\x1b[2J\x1b[H'
        out += self.rows_bytes(self.grid)
        if self.cursor_saved: out += self.cursor_bytes(*self.cursor_saved, self.grid) + b'\x1b7'
        if (self.top, self.bot) != full: out += b'\x1b[%d;%dr' % (self.top + 1, self.bot + 1)
        out += self.cursor_bytes(self.row, self.col, self.pen, self.grid)
        for n, on in self.pm.items(): out += b'\x1b[?%d%s' % (n, b'h' if on else b'l')   # say every mode EXPLICITLY
        out += b'\x1b=' if self.keypad else b'\x1b>'
        if with_pending and self.state != 'G': out += self.seq
        return out
    def undo_bytes(self):
        """What a client writes to its own terminal on detach, to undo what the program switched on."""
        out = b''
        if self.alt: out += b'\x1b[?1049l'
        for n, on in self.pm.items():
            if n == 25:
                if not on: out += b'\x1b[?25h'
            elif on: out += b'\x1b[?%dl' % n
        if self.keypad: out += b'\x1b>'
        if (self.top, self.bot) != (0, self.rows - 1) or self.saved_margins not in (None, (0, self.rows - 1)):
            out += b'\x1b7\x1b[r\x1b8'                       # reset margins without moving the cursor
        return out

    def resized(self, rows, cols):                          # crop or pad; the program then redraws itself
        def fit(g):
            ng = [[(' ', 0)] * cols for _ in range(rows)]
            for r in range(min(rows, self.rows)):
                for c in range(min(cols, self.cols)): ng[r][c] = g[r][c]
            return ng
        e = Emu(rows, cols)
        e.grid = fit(self.grid)
        e.row, e.col = min(self.row, rows - 1), min(self.col, cols - 1)
        e.pen, e.state, e.params, e.seq = self.pen, self.state, self.params, self.seq
        e.alt, e.pm, e.keypad = self.alt, dict(self.pm), self.keypad
        if self.saved:
            g, r, c, p = self.saved; e.saved, e.saved_margins = (fit(g), min(r, rows - 1), min(c, cols - 1), p), (0, rows - 1)
        if self.cursor_saved:
            r, c, p = self.cursor_saved; e.cursor_saved = (min(r, rows - 1), min(c, cols - 1), p)
        return e                                            # (margins reset on resize, as xterm does)
    def text(self):                                         # the screen currently being shown
        return '|'.join(''.join(ch for ch, _ in line).rstrip() for line in self.grid).rstrip('|')
    def main_text(self):                                    # the MAIN screen, even while an alternate one is active
        g = self.saved[0] if self.saved else self.grid
        return '|'.join(''.join(ch for ch, _ in line).rstrip() for line in g).rstrip('|')
    def dump(self):
        def d(g): return tuple(tuple((ch, p if ch != ' ' else 0) for ch, p in line) for line in g)
        saved = None if not self.saved else (d(self.saved[0]),) + self.saved[1:]
        return (d(self.grid), self.row, self.col, self.pen, self.state, self.params,
                self.alt, self.app_cursor, self.bpaste, saved,
                self.top, self.bot, tuple(sorted(self.pm.items())), self.keypad, self.cursor_saved, self.saved_margins)

# ---- the server ---------------------------------------------------------------------------------
def set_winsize(fd, rows, cols):                            # TIOCSWINSZ: the kernel then sends SIGWINCH (file 14)
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack('HHHH', rows, cols, 0, 0))

def serve(name, cmd):
    os.makedirs(SOCKDIR, mode=0o700, exist_ok=True)
    path = sockpath(name)
    if os.path.exists(path): os.unlink(path)
    lst = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)  # AF_UNIX: a filesystem path, never the network (file 27)
    lst.bind(path); os.chmod(path, 0o600); lst.listen(8)
    with open(pidpath(name), 'w') as f: f.write(str(os.getpid()))
    pid, master = pty.fork()                                 # a PTY pair, shell on the slave (file 02)
    if pid == 0:
        os.environ['TERM'] = 'xterm'
        os.execvp(cmd[0], cmd)
    emu = Emu(); set_winsize(master, emu.rows, emu.cols)
    clients = {}                                             # socket -> Reader

    def drop(c):
        clients.pop(c, None)
        try: c.close()
        except OSError: pass
    def terminate(*_): raise SystemExit
    signal.signal(signal.SIGTERM, terminate)
    try:
        while True:
            for s in select.select([lst, master] + list(clients), [], [])[0]:
                if s is lst:                                 # new client: snapshot first, then live bytes
                    c, _ = lst.accept(); clients[c] = Reader()
                    send(c, b'O', emu.snapshot(with_pending=not NO_PENDING))
                elif s == master:                            # the program wrote something
                    try: data = os.read(master, 65536)
                    except OSError: data = b''               # EIO: the child closed the slave, i.e. exited
                    if not data:
                        for c in list(clients):
                            try: send(c, b'X')
                            except OSError: pass
                        return
                    emu.feed(data)
                    for c in list(clients):                  # fan out the RAW bytes (file 47, 6.2)
                        try: send(c, b'O', data)
                        except OSError: drop(c)
                else:                                        # a client sent something
                    try: data = s.recv(65536)
                    except OSError: data = b''
                    if not data: drop(s); continue
                    for typ, pl in clients[s].feed(data):
                        if typ == b'I': os.write(master, pl)               # input: ordered by this one loop
                        elif typ == b'R':
                            rows, cols = struct.unpack('>HH', pl)
                            emu = emu.resized(rows, cols); set_winsize(master, rows, cols)
                        elif typ == b'D': send(s, b'D', repr(emu.dump()).encode())   # debug: dump the model
                        elif typ == b'M': send(s, b'M', emu.undo_bytes())
    finally:
        for c in list(clients): drop(c)
        try: os.kill(pid, signal.SIGHUP)
        except ProcessLookupError: pass
        try: os.waitpid(pid, os.WNOHANG)
        except ChildProcessError: pass
        for f in (path, pidpath(name)):
            try: os.unlink(f)
            except FileNotFoundError: pass

def start(name, cmd):
    os.makedirs(SOCKDIR, mode=0o700, exist_ok=True)
    if os.fork() > 0:                                        # the command you typed: wait for the socket, return
        for _ in range(100):
            if os.path.exists(sockpath(name)): print('started', name); return
            time.sleep(0.05)
        sys.exit('server did not start')
    os.setsid()                                              # new session, no controlling terminal (file 16)
    if os.fork() > 0: os._exit(0)                            # second fork: can never reacquire one (file 28)
    os.chdir('/'); fd = os.open(os.devnull, os.O_RDWR)
    for n in (0, 1, 2): os.dup2(fd, n)
    serve(name, cmd)

def kill(name):
    os.kill(int(open(pidpath(name)).read()), signal.SIGTERM); print('killed', name)

# ---- the client ---------------------------------------------------------------------------------
def reset_modes(s, reader):
    """On detach, undo whatever the program switched on, so it doesn't leak into YOUR terminal."""
    try:
        send(s, b'M'); s.settimeout(0.5); undo = None
        while undo is None:
            data = s.recv(65536)
            if not data: return
            for typ, pl in reader.feed(data):
                if typ == b'O': os.write(1, pl)               # output that was already in flight
                elif typ == b'M': undo = pl                   # the server says exactly what to undo
        os.write(1, undo)
    except OSError:                                          # includes the socket timeout
        pass

def attach(name):
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM); s.connect(sockpath(name))
    fd = sys.stdin.fileno(); saved = termios.tcgetattr(fd)
    resize = [True]                                          # set by the handler; acted on in the main loop
    signal.signal(signal.SIGWINCH, lambda *_: resize.__setitem__(0, True))
    tty.setraw(fd)                                           # raw mode (file 23): every key straight through
    reader, prefix, done, why = Reader(), False, False, 'detached'
    try:
        while not done:
            if resize[0]:
                resize[0] = False
                rows, cols = struct.unpack('HH', fcntl.ioctl(1, termios.TIOCGWINSZ, b'\0' * 8)[:4])
                send(s, b'R', struct.pack('>HH', rows, cols))
            ready = select.select([fd, s], [], [], 0.2)[0]
            if fd in ready:
                keys = os.read(fd, 4096); fwd = b''
                if not keys: break
                for b in keys:
                    if prefix:                               # the byte after the prefix is for US (file 46, in-band)
                        prefix = False
                        if b == ord('d'): done = True; break
                        if b == 0x02: fwd += b'\x02'         # prefix twice = a literal Ctrl+B
                    elif b == 0x02: prefix = True
                    else: fwd += bytes([b])
                if fwd: send(s, b'I', fwd)
            if s in ready:
                data = s.recv(65536)
                if not data: why = 'server closed'; break
                for typ, pl in reader.feed(data):
                    if typ == b'O': os.write(1, pl)
                    elif typ == b'X': why = 'session ended'; done = True
    finally:
        if why == 'detached': reset_modes(s, reader)
        termios.tcsetattr(fd, termios.TCSAFLUSH, saved); print('\r\n[%s]' % why)

if __name__ == '__main__':
    a = sys.argv[1:]
    if len(a) >= 2 and a[0] == 'start': start(a[1], a[2:] or ['sh', '-i'])
    elif len(a) == 2 and a[0] == 'attach': attach(a[1])
    elif len(a) == 2 and a[0] == 'kill': kill(a[1])
    else: sys.exit(__doc__)
```

---

## 17. Summary

- A multiplexer is a **PTY owner plus a server plus a terminal model**: nine stages built exactly that, from `forkpty` to a prefix-key client.
- Daemonizing needs the **double fork**; the tests confirmed the result on a running server — no controlling terminal, not a session leader, adopted by PID 1.
- A snapshot must account for the **parser's state**: replaying the pending bytes made a live mid-sequence attach converge, and omitting that replay printed `1mred` as literal text.
- Fan-out is a **tee of raw bytes** to replicas; input goes through **one loop**; resize is `TIOCSWINSZ` and a program that redraws itself — and **geometry stays per-session**.
- A multiplexer must also carry **terminal modes**: the snapshot paints the main screen, *then* enters the alternate one, and states cursor-key and paste modes explicitly; on detach the client undoes them so your terminal isn't left in `vim`'s state.
- The tests are strongest where they check against *ground truth* and weakest where the "client" is the same model as the server — Section 14 says which is which.
- The model now covers the sequences full-screen programs commonly use (scroll regions, insert/delete, saved cursor, mouse and cursor-visibility modes) and is checked three ways: expected screens derived by hand from the VT rules, a randomized test that a snapshot at *any* byte offset reproduces the model, and mutation tests showing each bug I fixed is caught when re-introduced.
- `minimux` still covers a limited part of the terminal; its remaining limits (other modes, backpressure, divergence detection, layout) and the untried real-`vim` experiment are listed honestly.

---

## What's next

That completes the multiplexer arc (files 45-48). Natural continuations: extend `minimux` with exercises 3 and 5 (the real-terminal cross-check and divergence detection), or revisit file 34's mini shell and run it *inside* `minimux` — your own shell, in your own multiplexer, on a PTY you allocated yourself.
