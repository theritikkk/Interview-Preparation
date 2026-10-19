#!/usr/bin/env python3
"""Probe the OS-level assumptions that minimux.py relies on. Run this on your Mac BEFORE trusting
minimux there:   python3 macos_probe.py

Each line is PASS, FAIL, or INFO (INFO = a difference worth knowing about, not a failure).
It was written and verified on Linux; what it prints on macOS is exactly the information I could not get."""
import os, sys, pty, time, select, socket, struct, fcntl, termios, tty, signal, tempfile, platform, subprocess

results = []
def report(status, label, detail=''):
    print('%-5s %s%s' % (status, label, ('   -   ' + detail) if detail else ''))
    results.append(status)

def read_for(fd, seconds, stop_on=None):
    buf, end = b'', time.time() + seconds
    while time.time() < end:
        if select.select([fd], [], [], 0.1)[0]:
            try: d = os.read(fd, 4096)
            except OSError: return buf, 'oserror'
            if not d: return buf, 'eof'
            buf += d
            if stop_on and stop_on in buf: return buf, 'found'
    return buf, 'timeout'

print('platform:', platform.platform(), '| machine:', platform.machine(), '| python', platform.python_version())
print()

# 1. fork a child onto a PTY and read its output through select()
pid, fd = pty.fork()
if pid == 0:
    os.execvp('printf', ['printf', 'hello_pty'])
buf, how = read_for(fd, 3, b'hello_pty')
os.waitpid(pid, 0)
report('PASS' if b'hello_pty' in buf else 'FAIL', 'pty.fork + select() on the PTY master', repr(buf[:30]))

# 2. what does reading the master do AFTER the child has exited? (minimux handles both outcomes)
pid, fd = pty.fork()
if pid == 0:
    os.execvp('printf', ['printf', 'x'])
os.waitpid(pid, 0); time.sleep(0.2)
buf, how = read_for(fd, 2)
report('INFO', 'reading the PTY master after the child exits', 'ended with: %s (minimux handles both "oserror" and "eof")' % how)

# 3. window size round trip: set on the master, read back on the slave
m, s = os.openpty()
fcntl.ioctl(m, termios.TIOCSWINSZ, struct.pack('HHHH', 33, 101, 0, 0))
rows, cols = struct.unpack('HH', fcntl.ioctl(s, termios.TIOCGWINSZ, b'\0' * 8)[:4])
report('PASS' if (rows, cols) == (33, 101) else 'FAIL', 'TIOCSWINSZ on the master is visible on the slave', 'got %dx%d' % (rows, cols))
os.close(m); os.close(s)

# 4. does resizing the PTY deliver SIGWINCH to the program on it? (files 14 and 48 claim so)
child_code = ("import signal,sys,time\n"
              "signal.signal(signal.SIGWINCH, lambda *a: (sys.stdout.write('GOT_WINCH\\n'), sys.stdout.flush()))\n"
              "sys.stdout.write('READY\\n'); sys.stdout.flush(); time.sleep(3)\n")
pid, fd = pty.fork()
if pid == 0:
    os.execvp(sys.executable, [sys.executable, '-c', child_code])
buf, how = read_for(fd, 5, b'READY')
fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack('HHHH', 40, 120, 0, 0))
buf2, how = read_for(fd, 3, b'GOT_WINCH')
os.kill(pid, signal.SIGKILL); os.waitpid(pid, 0)
report('PASS' if b'GOT_WINCH' in buf2 else 'FAIL', 'resizing the PTY delivers SIGWINCH to the program', repr(buf2[-20:]))

# 5. Unix domain socket paths: minimux's default path, and where this OS's length limit falls
d = tempfile.mkdtemp(dir='/tmp')
default = '/tmp/minimux-%d/probe.sock' % os.getuid()
report('PASS' if len(default) < 100 else 'FAIL', 'minimux default socket path is short enough', '%d characters' % len(default))
ok = []
for L in (90, 100, 103, 104, 107, 108):
    path = os.path.join(d, 'x' * (L - len(d) - 1))
    sk = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    try: sk.bind(path); ok.append(L); os.unlink(path)
    except OSError: pass
    finally: sk.close()
os.rmdir(d)
report('INFO', 'longest AF_UNIX path that bound, from the lengths tried', '%s (the usual limits: 108 on Linux, 104 on macOS)' % (max(ok) if ok else 'none'))

# 6. a daemonized (double-forked) process: no controlling terminal, survives its launcher
pidfile = tempfile.mktemp()
if os.fork() == 0:
    os.setsid()
    if os.fork() == 0:
        open(pidfile, 'w').write(str(os.getpid())); time.sleep(3); os._exit(0)
    os._exit(0)
for _ in range(50):
    if os.path.exists(pidfile) and open(pidfile).read(): break
    time.sleep(0.05)
dpid = int(open(pidfile).read()); os.unlink(pidfile)
out = subprocess.run(['ps', '-o', 'ppid=,tty=', '-p', str(dpid)], capture_output=True, text=True).stdout.split()
alive = True
try: os.kill(dpid, 0)
except ProcessLookupError: alive = False
report('PASS' if alive and out[1:] and out[1] in ('?', '??') else 'FAIL', 'a double-forked daemon has no controlling terminal and survives',
       'ppid=%s tty=%s (Linux prints "?", macOS prints "??")' % (out[0] if out else '?', out[1] if len(out) > 1 else '?'))

# 7. an interactive sh on a PTY: what does it print, and does it behave?
pid, fd = pty.fork()
if pid == 0:
    os.environ['TERM'] = 'xterm'; os.execvp('sh', ['sh', '-i'])
fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack('HHHH', 27, 91, 0, 0))   # a fresh PTY is 0x0 until a size is set
first, _ = read_for(fd, 0.8)
os.write(fd, b'echo probe_ok\n'); out1, _ = read_for(fd, 1.5, b'\r\nprobe_ok')
os.write(fd, b'stty size\n'); out2, _ = read_for(fd, 1.5)
os.kill(pid, signal.SIGHUP); os.waitpid(pid, 0)
report('INFO', 'what `sh -i` prints at startup', repr(first[:70]))
report('PASS' if b'probe_ok' in out1 else 'FAIL', '`sh -i` runs commands on a PTY')
report('PASS' if b'27 91' in out2 else 'FAIL', '`stty size` inside the PTY reports exactly the size that was set (27x91)', repr(out2[-24:]))

# 8. raw mode on a PTY (what attach does to your terminal), and the constants the tests use
m, s = os.openpty()
before = termios.tcgetattr(s)[3]; tty.setraw(s); after = termios.tcgetattr(s)[3]
report('PASS' if (before & termios.ICANON) and not (after & termios.ICANON) else 'FAIL', 'tty.setraw clears ICANON on a PTY')
os.close(m); os.close(s)
report('PASS' if hasattr(termios, 'TIOCSCTTY') else 'FAIL', 'termios.TIOCSCTTY exists (used by the test harness)')
pg = subprocess.run(['pgrep', '-P', str(os.getpid())], capture_output=True, text=True)
report('PASS' if pg.returncode in (0, 1) else 'FAIL', '`pgrep -P` is available (used by the tests)')

print()
n = {k: results.count(k) for k in ('PASS', 'FAIL', 'INFO')}
print('%d PASS, %d FAIL, %d INFO' % (n['PASS'], n['FAIL'], n['INFO']))
print('If there are no FAIL lines, try the quick start in file 48. Otherwise paste the FAIL lines back to me.')
sys.exit(1 if n['FAIL'] else 0)
