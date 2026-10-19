#!/usr/bin/env python3
"""Tests for minimux. Drives real servers over the real socket protocol, and the real
interactive client through a pty. Each check prints PASS or FAIL."""
import os, sys, time, ast, socket, struct, signal, subprocess, shutil, fcntl, termios, select

DIR = '/tmp/mm_test_run'
os.environ['MINIMUX_DIR'] = DIR
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import minimux as mm

HERE = os.path.dirname(os.path.abspath(__file__))
results = []
def check(label, ok, detail=''):
    results.append(ok); print(('PASS  ' if ok else 'FAIL  ') + label + (('   ' + detail) if detail else ''))

def wait_for(cond, timeout=5.0):
    end = time.time() + timeout
    while time.time() < end:
        if cond(): return True
        time.sleep(0.05)
    return cond()

SHELL_CMD = ('dash', '-i') if shutil.which('dash') else ('sh', '-i')   # override: MINIMUX_TEST_SHELL='bash --noprofile --norc -i'
if os.environ.get('MINIMUX_TEST_SHELL'): SHELL_CMD = tuple(os.environ['MINIMUX_TEST_SHELL'].split())
ON_LINUX = sys.platform.startswith('linux')

def start_server(name, env_extra=None, cmd=None):
    cmd = cmd or SHELL_CMD
    env = dict(os.environ, **(env_extra or {}))
    subprocess.run([sys.executable, os.path.join(HERE, 'minimux.py'), 'start', name, *cmd],
                   env=env, check=True, stdout=subprocess.DEVNULL)
def server_pid(name): return int(open(mm.pidpath(name)).read())
def alive(pid):
    try: os.kill(pid, 0); return True
    except ProcessLookupError: return False

class Client:
    """A protocol-level client with its own replica terminal (what a real terminal would show)."""
    def __init__(self, name):
        self.s = socket.socket(socket.AF_UNIX); self.s.connect(mm.sockpath(name)); self.s.settimeout(0.1)
        self.rd, self.replica, self.raw, self.dump_reply = mm.Reader(), mm.Emu(), b'', None
    def pump(self, t=0.3):
        end = time.time() + t
        while time.time() < end:
            try: d = self.s.recv(65536)
            except socket.timeout: continue
            if not d: return
            for typ, pl in self.rd.feed(d):
                if typ == b'O': self.raw += pl; self.replica.feed(pl)
                elif typ == b'D': self.dump_reply = ast.literal_eval(pl.decode())
    def until(self, needle, t=5.0):
        end = time.time() + t
        while time.time() < end:
            self.pump(0.1)
            if needle in self.raw.decode('latin1'): return True
        return False
    def type(self, text): mm.send(self.s, b'I', text.encode())
    def model(self):
        self.dump_reply = None; mm.send(self.s, b'D')
        end = time.time() + 2
        while self.dump_reply is None and time.time() < end: self.pump(0.1)
        return self.dump_reply
    def close(self): self.s.close()

print('platform: %s | python %s | session shell: %s' % (sys.platform, sys.version.split()[0], ' '.join(SHELL_CMD)))
shutil.rmtree(DIR, ignore_errors=True)
print('--- 1. persistence: the session outlives its client -----------------------------------------')
start_server('t1')
spid = server_pid('t1')
a = Client('t1'); a.pump(0.5); a.type('echo persist_me\n')
check('shell output reaches the client', a.until('persist_me'))
a.close(); time.sleep(0.3)
check('server process survives the client disconnecting', alive(spid))
kids = subprocess.run(['pgrep', '-P', str(spid)], capture_output=True, text=True).stdout.split()
check('the shell (its child) is still running', len(kids) >= 1, 'child pids=%s' % kids)
ps = lambda col: subprocess.run(['ps', '-o', col + '=', '-p', str(spid)], capture_output=True, text=True).stdout.strip()
check('server has NO controlling terminal (file 16)', ps('tty') in ('?', '??'), 'tty=%s' % ps('tty'))
if ON_LINUX:
    check('server is NOT a session leader - the double fork\'s purpose (file 28)', ps('sid') != str(spid),
          'sid=%s pid=%s ppid=%s' % (ps('sid'), spid, ps('ppid')))
else:
    print('SKIP  session-leader check (BSD ps has no portable session-id column); ppid=%s' % ps('ppid'))

print('--- 2. a late client gets the screen back, and converges --------------------------------------')
b = Client('t1'); b.pump(0.6)
check('snapshot restores earlier output on reattach', 'persist_me' in b.replica.text(), repr(b.replica.text()[:50]))
b.type("ls /etc | head -3; printf 'END_%s\\n' MARK\n"); b.until('END_MARK', 5); time.sleep(0.3); b.pump(0.3)
srv = b.model()
check('client replica == server model after more output', b.replica.dump() == srv)

print('--- 3. fan-out: two clients, input from either ------------------------------------------------')
c = Client('t1'); c.pump(0.5)
b.type('echo from_b\n')
check('client C sees what B typed', c.until('from_b'))
c.type('echo from_c\n')
check('client B sees what C typed', b.until('from_c'))
b.pump(0.3); c.pump(0.3)
check('both replicas equal the server model', b.replica.dump() == c.replica.dump() == c.model())
b.close(); c.close()

print('--- 4. resize: one PTY, one size ----------------------------------------------------------------')
r = Client('t1'); r.pump(0.3)
mm.send(r.s, b'R', struct.pack('>HH', 30, 100)); time.sleep(0.2); r.type('stty size\n')
check('the PROGRAM sees the new size (via TIOCSWINSZ)', r.until('30 100'))
r.pump(0.3); m = r.model()
check('server model resized to 30 rows', len(m[0]) == 30 and len(m[0][0]) == 100, 'rows=%d cols=%d' % (len(m[0]), len(m[0][0])))
r.close()
subprocess.run([sys.executable, os.path.join(HERE, 'minimux.py'), 'kill', 't1'], stdout=subprocess.DEVNULL)
check('kill removes socket and pid file', wait_for(lambda: not os.path.exists(mm.sockpath('t1'))))

print('--- 5. the hazard from file 47: a snapshot cut MID-SEQUENCE, on a live shell --------------------')
CMD = r"printf '\033[3'; sleep 2; printf '1mred\033[0m\n'" + '\n'
def mid_sequence_run(name, env_extra):
    start_server(name, env_extra)
    drv = Client(name); drv.pump(0.5); drv.type(CMD); time.sleep(0.9)        # shell is now inside the sleep
    cut = drv.model()
    late = Client(name); late.pump(0.5)
    after_snapshot = (late.replica.state, late.replica.params)
    late.until('red', 4); time.sleep(0.6); late.pump(0.5)
    final_server = late.model(); final_replica = late.replica.dump()
    drv.close(); late.close()
    subprocess.run([sys.executable, os.path.join(HERE, 'minimux.py'), 'kill', name], stdout=subprocess.DEVNULL)
    return cut, after_snapshot, final_server, final_replica, late.replica.text()

cut, snap_state, srv, rep, txt = mid_sequence_run('t5a', {})
check('server really was mid-sequence when the client attached', cut[4] == 'C' and cut[5] == '3', 'state=%s params=%r' % (cut[4], cut[5]))
check('WITH the fix: client terminal ends in the same parser state', snap_state == ('C', '3'), 'client state after snapshot=%s' % (snap_state,))
check('WITH the fix: client screen == server model at the end', rep == srv)
cut, snap_state, srv, rep, txt = mid_sequence_run('t5b', {'MINIMUX_NO_PENDING': '1'})
check('WITHOUT the fix: client terminal starts in ground state', snap_state == ('G', ''), 'client state after snapshot=%s' % (snap_state,))
check('WITHOUT the fix: client screen DIVERGES from the server', rep != srv)
check('WITHOUT the fix: the sequence tail is printed as literal text', '1mred' in txt, repr(txt[-40:]))

print('--- 6. the real interactive client, driven through a pty ---------------------------------------')
start_server('t6')
def spawn_client(name):
    m, sl = os.openpty()
    fcntl.ioctl(sl, termios.TIOCSWINSZ, struct.pack('HHHH', 24, 80, 0, 0))
    pid = os.fork()
    if pid == 0:
        os.setsid(); fcntl.ioctl(sl, termios.TIOCSCTTY, 0)
        for n in (0, 1, 2): os.dup2(sl, n)
        os.execv(sys.executable, [sys.executable, os.path.join(HERE, 'minimux.py'), 'attach', name])
    os.close(sl); return pid, m
def drain(m, emu, t):
    end = time.time() + t
    while time.time() < end:
        if select.select([m], [], [], 0.1)[0]:
            try: d = os.read(m, 65536)
            except OSError: return
            if not d: return
            emu.feed(d)
pid, m = spawn_client('t6'); term = mm.Emu(); drain(m, term, 0.8)
os.write(m, b'echo viapty\r'); drain(m, term, 0.8)
check('keystrokes travel client -> server -> shell and back', term.text().count('viapty') >= 2, repr(term.text()[:60]))
os.write(m, b'cat -v\r'); drain(m, term, 0.4); os.write(m, b'\x02\x02\r'); drain(m, term, 0.5)
check('prefix pressed twice sends a literal Ctrl+B (shown as ^B)', '^B' in term.text(), repr(term.text()[-40:]))
os.write(m, b'\x04'); drain(m, term, 0.3)
os.write(m, b'\x02d'); drain(m, term, 0.5)
_, status = os.waitpid(pid, 0); os.close(m)
check('Ctrl+B then d detaches the client cleanly', os.WIFEXITED(status) and os.WEXITSTATUS(status) == 0)
check('...and the session keeps running', alive(server_pid('t6')))
pid, m = spawn_client('t6'); term2 = mm.Emu(); drain(m, term2, 0.8)
check('a NEW client attaches and sees the earlier session', 'viapty' in term2.text(), repr(term2.text()[:50]))
os.write(m, b'\x02d'); drain(m, term2, 0.3); os.waitpid(pid, 0); os.close(m)
subprocess.run([sys.executable, os.path.join(HERE, 'minimux.py'), 'kill', 't6'], stdout=subprocess.DEVNULL)

print('--- 7. the alternate screen: what vim, less and htop do ------------------------------------------')
# Markers are built with printf arguments, so the ECHOED command line can never contain them.
ALT_CMD = (r"printf 'main_%s\n' line; printf '\033[?1049h\033[2J\033[1;1HALT_%s' SCREEN; "
           r"sleep 2; printf '\033[?1049l'; printf 'back_%s\n' on_main") + '\n'

def alt_run(name, env_extra):
    start_server(name, env_extra)
    drv = Client(name); drv.pump(0.5); drv.type(ALT_CMD); time.sleep(0.9)       # the program is inside the alt screen
    during_srv = drv.model()
    late = Client(name); late.pump(0.5)
    during = (late.replica.alt, late.replica.text(), late.replica.main_text(), late.raw)
    late.until('back_on_main', 5); time.sleep(0.3); late.pump(0.4)
    after_srv, after_rep = late.model(), late.replica.dump()
    fresh = Client(name); fresh.pump(0.6); fresh_text = fresh.replica.text()
    for x in (drv, late, fresh): x.close()
    subprocess.run([sys.executable, os.path.join(HERE, 'minimux.py'), 'kill', name], stdout=subprocess.DEVNULL)
    return during_srv, during, after_srv, after_rep, fresh_text

d_srv, during, a_srv, a_rep, fresh = alt_run('t7a', {})
check('server tracks that the program entered the alternate screen', d_srv[6] is True)
check('a client attaching MID-program is put in the alternate screen too', during[0] is True)
check('...which shows the program\'s screen', 'ALT_SCREEN' in during[1], repr(during[1][:40]))
check('...while the MAIN screen is preserved underneath', 'main_line' in during[2], repr(during[2][:60]))
raw = during[3]
check('snapshot paints the main screen BEFORE entering the alternate one',
      raw.index(b'main_line') < raw.index(b'\x1b[?1049h') < raw.index(b'ALT_SCREEN'))
check('after the program exits, client screen == server model', a_rep == a_srv)
check('GROUND TRUTH: a fresh client sees the main screen restored',
      'main_line' in fresh and 'back_on_main' in fresh and 'ALT_SCREEN' not in fresh, repr(fresh[:70]))
d_srv, during, a_srv, a_rep, fresh = alt_run('t7b', {'MINIMUX_NO_ALT': '1'})
check('WITHOUT alt tracking: the main screen is LOST and the program\'s text is left behind',
      'main_line' not in fresh and 'ALT_SCREEN' in fresh, repr(fresh[:70]))

print('--- 8. modes a re-attaching terminal must be told about -----------------------------------------')
MODE_CMD = r"printf '\033[?1h\033[?2004h'; sleep 2; printf '\033[?1l\033[?2004l'" + '\n'
start_server('t8')
drv = Client('t8'); drv.pump(0.5); drv.type(MODE_CMD); time.sleep(0.9)
mid = Client('t8'); mid.pump(0.5)
check('snapshot turns application cursor keys ON for a client attaching mid-program', b'\x1b[?1h' in mid.raw)
check('snapshot turns bracketed paste ON as well', b'\x1b[?2004h' in mid.raw)
time.sleep(2.2); drv.pump(0.3)
after = Client('t8'); after.pump(0.5)
after_model = after.model()
want_paste = b'\x1b[?2004h' if after_model[8] else b'\x1b[?2004l'      # index 8 = the model's bracketed-paste flag
check('after the program resets them, a new client is told cursor-keys are OFF',
      b'\x1b[?1l' in after.raw and b'\x1b[?1h' not in after.raw)
check('...and is told bracketed paste is whatever the shell has left it as (dash: off; bash\'s readline turns it back ON at the prompt)',
      want_paste in after.raw, 'model says bpaste=%s' % after_model[8])
for x in (drv, mid, after): x.close()
subprocess.run([sys.executable, os.path.join(HERE, 'minimux.py'), 'kill', 't8'], stdout=subprocess.DEVNULL)

print("--- 9. detaching must not leave YOUR terminal in the program's modes -------------------------------")
start_server('t9')
drv = Client('t9'); drv.pump(0.5)
drv.type(r"printf '\033[?1049h\033[?1h\033[?2004h'; sleep 4" + '\n'); time.sleep(0.9)
pid, m = spawn_client('t9'); term9 = mm.Emu(); drain(m, term9, 0.8)
check('a real client attaching mid-program lands in the alternate screen with the modes on',
      term9.alt and term9.app_cursor and term9.bpaste, 'alt=%s app_cursor=%s bpaste=%s' % (term9.alt, term9.app_cursor, term9.bpaste))
os.write(m, b'\x02d'); drain(m, term9, 1.0); os.waitpid(pid, 0); os.close(m)
check('after Ctrl+B d the client has put the terminal back (alternate screen and modes OFF)',
      not term9.alt and not term9.app_cursor and not term9.bpaste,
      'alt=%s app_cursor=%s bpaste=%s' % (term9.alt, term9.app_cursor, term9.bpaste))
drv.close()
subprocess.run([sys.executable, os.path.join(HERE, 'minimux.py'), 'kill', 't9'], stdout=subprocess.DEVNULL)

print('--- 10. the model against VT semantics derived by hand (no server involved) ---------------------')
def screen(e): return [''.join(ch for ch, _ in line).rstrip() for line in e.grid]
def labelled(rows=6, cols=10):
    e = mm.Emu(rows, cols)
    for i in range(1, rows + 1): e.feed(b'\x1b[%d;1HL%d' % (i, i))
    return e
L = ['L1', 'L2', 'L3', 'L4', 'L5', 'L6']
e = labelled(); e.feed(b'\x1b[2;5r\x1b[5;1H\n')
check('LF at the BOTTOM margin scrolls only the region', screen(e) == ['L1', 'L3', 'L4', 'L5', '', 'L6'], str(screen(e)))
e = labelled(); e.feed(b'\x1b[2;4r\x1b[6;1H\n')
check('LF BELOW the region does not scroll anything', screen(e) == L and e.row == 5, str(screen(e)))
e = labelled(); e.feed(b'\x1b[2;5r\x1b[2;1H\x1bM')
check('reverse index at the TOP margin scrolls the region down', screen(e) == ['L1', '', 'L2', 'L3', 'L4', 'L6'], str(screen(e)))
e = labelled(); e.feed(b'\x1b[3;1H\x1b[2L')
check('insert 2 lines pushes the rest down (and off the bottom)', screen(e) == ['L1', 'L2', '', '', 'L3', 'L4'], str(screen(e)))
e = labelled(); e.feed(b'\x1b[2;1H\x1b[2M')
check('delete 2 lines pulls the rest up and blanks the bottom', screen(e) == ['L1', 'L4', 'L5', 'L6', '', ''], str(screen(e)))
e = mm.Emu(3, 10); e.feed(b'ABCDEFGH\x1b[1;3H\x1b[2@')
check('insert 2 characters shifts the line right', screen(e)[0] == 'AB  CDEFGH', repr(screen(e)[0]))
e = mm.Emu(3, 10); e.feed(b'ABCDEFGH\x1b[1;3H\x1b[2P')
check('delete 2 characters shifts the line left', screen(e)[0] == 'ABEFGH', repr(screen(e)[0]))
e = mm.Emu(3, 10); e.feed(b'ABCDEFGH\x1b[1;3H\x1b[2X')
check('erase 2 characters blanks them in place', screen(e)[0] == 'AB  EFGH', repr(screen(e)[0]))
e = mm.Emu(3, 10); e.feed(b'x\x1b(By')
check('ESC ( B (charset select, emitted by ncurses) is swallowed, not printed', screen(e)[0] == 'xy', repr(screen(e)[0]))
e = mm.Emu(6, 10); e.feed(b'\x1b[3;4H\x1b[31m\x1b7\x1b[1;1H\x1b[0m\x1b8')
check('ESC 7 / ESC 8 save and restore cursor and pen', (e.row, e.col, e.pen) == (2, 3, 31), str((e.row, e.col, e.pen)))
e = mm.Emu(6, 10); e.feed(b'\x1b[2\n')
check('a control character inside a CSI runs once and is NOT part of the pending sequence',
      e.row == 1 and e.state == 'C' and e.seq == b'\x1b[2', 'row=%d seq=%r' % (e.row, e.seq))
e = mm.Emu(6, 10); e.feed(b'\x1b[?25l\x1b[?1000h\x1b[?1006h\x1b=')
snap = e.snapshot(); r = mm.Emu(6, 10); r.feed(snap)
check('hidden cursor, mouse modes and application keypad are all carried by a snapshot',
      all(x in snap for x in (b'\x1b[?25l', b'\x1b[?1000h', b'\x1b[?1006h', b'\x1b=')) and r.pm == e.pm and r.keypad)
undo = e.undo_bytes()
check('...and undone on detach (cursor shown again, mouse off, keypad numeric)',
      b'\x1b[?25h' in undo and b'\x1b[?1000l' in undo and b'\x1b[?1006l' in undo and b'\x1b>' in undo, repr(undo))
e = mm.Emu(6, 10); e.feed(b'0123456789'); r = mm.Emu(6, 10); r.feed(e.snapshot()); r.feed(b'Z'); e.feed(b'Z')
check('a snapshot taken at a PENDING WRAP (cursor past the last column) still wraps correctly',
      r.dump() == e.dump() and screen(e)[1] == 'Z', str(screen(e)[:2]))

print('--- 11. property test: a snapshot at ANY offset reproduces the model --------------------------------')
import random
def random_stream(rng, n_ops):
    csi = lambda params, f: b'\x1b[' + params.encode() + f.encode()
    ops = []
    for _ in range(n_ops):
        k = rng.randrange(30)
        if k < 6: ops.append(''.join(rng.choice('abcdefghij XYZ0123') for _ in range(rng.randrange(1, 14))).encode())
        elif k == 6: ops.append(b'\r\n')
        elif k == 7: ops.append(b'\n')
        elif k == 8: ops.append(b'\r')
        elif k == 9: ops.append(csi('%d;%d' % (rng.randrange(1, 9), rng.randrange(1, 14)), 'H'))
        elif k == 10: ops.append(csi(str(rng.randrange(0, 4)), rng.choice('ABCD')))
        elif k == 11: ops.append(csi(str(rng.randrange(0, 3)), rng.choice('JK')))
        elif k == 12: ops.append(csi(rng.choice(['0', '31', '32', '1', '']), 'm'))
        elif k == 13: ops.append(csi('%d;%d' % (rng.randrange(1, 7), rng.randrange(1, 9)), 'r'))
        elif k == 14: ops.append(b'\x1bM')
        elif k == 15: ops.append(csi(str(rng.randrange(1, 4)), rng.choice('LMST')))
        elif k == 16: ops.append(csi(str(rng.randrange(1, 4)), rng.choice('@PX')))
        elif k == 17: ops.append(rng.choice([b'\x1b7', b'\x1b8', b'\x1bD', b'\x1bE', b'\x1b=', b'\x1b>']))
        elif k == 18: ops.append(b'\x1b[?%d%s' % (rng.choice([1, 25, 1000, 1006, 2004]), rng.choice([b'h', b'l'])))
        elif k == 19: ops.append(b'\x1b[?1049' + rng.choice([b'h', b'l']))
        elif k == 20: ops.append(b'\x1b]0;title\x07')
        elif k == 21: ops.append(b'\x1b(B')
        elif k == 22: ops.append(csi(str(rng.randrange(1, 14)), rng.choice('Gd')))
        elif k == 23: ops.append(b'\x1b[5t')
        elif k == 24: ops.append(b'\t')
        elif k == 25: ops.append(b'\b')
        elif k == 26: ops.append(rng.choice([b'\x1b[2\nC', b'\x1b[\r1;4H', b'\x1b[3\n\nA']))   # control characters INSIDE a CSI
        elif k == 27: ops.append(b'\x1b\nM')                                                    # ... and inside an ESC sequence
        elif k == 28: ops.append(b'0123456789ab')                                               # runs to the last column
        else: ops.append(b'\x1b[?25h')
    return b''.join(ops)

rng = random.Random(20261017); failure = None; trials = 0
for _ in range(400):
    stream = random_stream(rng, rng.randrange(5, 45))
    full = mm.Emu(6, 12); full.feed(stream)
    for k in sorted({rng.randrange(0, len(stream) + 1) for _ in range(14)}):
        server = mm.Emu(6, 12); server.feed(stream[:k])
        replica = mm.Emu(6, 12); replica.feed(server.snapshot()); replica.feed(stream[k:])
        trials += 1
        if replica.dump() != full.dump(): failure = (stream, k); break
    if failure: break
detail = '%d trials' % trials
if failure:
    s, k = failure
    srv = mm.Emu(6, 12); srv.feed(s[:k]); rep = mm.Emu(6, 12); rep.feed(srv.snapshot()); rep.feed(s[k:]); ful = mm.Emu(6, 12); ful.feed(s)
    names = ['grid', 'row', 'col', 'pen', 'state', 'params', 'alt', 'app_cursor', 'bpaste', 'saved', 'top', 'bot', 'pm', 'keypad', 'cursor_saved', 'saved_margins']
    diff = [n for n, a, b in zip(names, rep.dump(), ful.dump()) if a != b]
    detail = 'FIRST FAILURE: cut=%d differs in %s | stream=%r' % (k, diff, s)
check('400 random streams x up to 14 cut points: snapshot + remainder == uninterrupted replay', failure is None, detail)

print()
print('%d/%d checks passed' % (sum(results), len(results)))
shutil.rmtree(DIR, ignore_errors=True)
sys.exit(0 if all(results) else 1)
