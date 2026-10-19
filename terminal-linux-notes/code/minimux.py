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
