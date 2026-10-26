# Processes, Threads, Signals, and Jobs

> **The one-sentence model:** a process is a running program with an address space, a set of threads, a credential, open file descriptors, and a scheduler state. Debugging a process means finding out **what state it is in and what it is waiting for**.

---

## 1. Process vs thread

| | Process | Thread |
|---|---|---|
| Address space | Its own | Shared with sibling threads |
| Credentials | Its own | Shared |
| File descriptors | Its own table | Shared |
| Scheduling | Threads are scheduled; process is the container | Individual kernel task |
| Isolation | Strong (with namespaces) | Weak; a crash can take the process down |
| Creation cost | Higher (`fork` + `exec`) | Lower (`clone`) |

On Linux, both are tasks in the kernel (`task_struct`). `ps -eLf` shows threads (`LWP` = lightweight process).

```bash
$ ps -eLf | head
UID     PID  PPID   LWP  C NLWP STIME TTY      TIME CMD
root      1     0     1  0    1 10:01 ?    00:00:02 /sbin/init
root    812     1   812  0    4 10:02 ?    00:00:00 /usr/bin/containerd
root    812     1   815  0    4 10:02 ?    00:00:00 /usr/bin/containerd
```

`NLWP` is the number of threads. A Java or Node process often has many.

---

## 2. The process table

```bash
$ ps aux | head
USER       PID %CPU %MEM    VSZ   RSS TTY      STAT START   TIME COMMAND
root         1  0.0  0.1 167432 12800 ?        Ss   10:01   0:02 /sbin/init
root       812  0.1  0.4 1423300 34000 ?      Ssl  10:02   0:01 /usr/bin/containerd
app       1042  2.3  6.1 2845120 492000 ?     Ssl  10:05   0:14 node /opt/app/server.js
app       1180  0.0  0.0      0     0 ?       Z    10:07   0:00 [worker] <defunct>
```

| Column | Meaning | How to read it |
|---|---|---|
| `USER` | Effective user | If it is `root`, ask why |
| `PID` | Process ID | Stable until exit; PIDs wrap |
| `%CPU` | CPU used since last update (top) or lifetime average (ps) | Can exceed 100% for multithreaded processes in `top` |
| `%MEM` | RSS as a percentage of physical memory | Not the same as virtual size |
| `VSZ` | Virtual memory size (KB) | Can be huge and harmless (reservations, mappings) |
| `RSS` | Resident set size (KB) | Actual physical pages, includes shared pages |
| `TTY` | Controlling terminal | `?` for daemons |
| `STAT` | State + modifiers | The most important column |
| `START` | Start time | Correlate with deploys/changes |
| `TIME` | CPU time consumed | Low CPU time + long runtime = waiting |
| `COMMAND` | Command line | `[name]` in brackets often means kernel thread |

Useful custom formats:

```bash
ps -eo pid,ppid,user,stat,ni,pri,psr,wchan:30,etime,cmd --sort=-%cpu | head
ps -eo pid,ppid,stat,etime,cmd --forest
ps -p 1042 -o pid,ppid,stat,wchan:25,cmd
```

---

## 3. Process states

| State | Name | Meaning | Action |
|---|---|---|---|
| `R` | Running / runnable | On CPU or in the run queue | Normal; if many, CPU pressure |
| `S` | Interruptible sleep | Waiting for an event (I/O, timer, socket) | Normal; most processes |
| `D` | Uninterruptible sleep | Waiting for kernel I/O, often disk/NFS | Investigate; cannot be killed until I/O completes |
| `T` | Stopped | Suspended by a signal (`SIGSTOP`) or debugger | `SIGCONT` to resume |
| `Z` | Zombie | Exited, waiting for parent to reap | Cannot be killed; fix the parent |
| `I` | Idle kernel thread | No work | Normal |
| `X` | Dead | Should not appear in `ps` | Kernel bug/rare |

Modifiers:

| Modifier | Meaning |
|---|---|
| `s` | Session leader |
| `l` | Multithreaded |
| `+` | Foreground process group |
| `<` | High priority |
| `N` | Low priority (nice) |
| `L` | Locked memory pages |

A process in `D` is the classic "unkillable" process:

```bash
$ ps -o pid,stat,wchan:30,cmd -p 2041
    PID STAT WCHAN                          CMD
   2041 D    rpc_wait_bit_killable            /usr/bin/rsync /mnt/nfs/...
```

`wchan` shows the kernel function it is sleeping in. A `D` state on an NFS mount often means the server is unreachable; `kill -9` will not help until the I/O returns or the mount is recovered.

---

## 4. Signals

A signal is an asynchronous notification to a process. The kernel delivers it; the process can catch, ignore, or be terminated.

| Signal | Number | Default | Typical use |
|---|---:|---|---|
| `SIGHUP` | 1 | Terminate | Reload config (many daemons) or terminal hangup |
| `SIGINT` | 2 | Terminate | Ctrl-C |
| `SIGQUIT` | 3 | Core dump | Quit with a core |
| `SIGKILL` | 9 | Terminate | Cannot be caught/ignored; last resort |
| `SIGTERM` | 15 | Terminate | Graceful shutdown (default for `kill`) |
| `SIGSTOP` | 19 | Stop | Cannot be caught; pause |
| `SIGCONT` | 18 | Continue | Resume after stop |
| `SIGUSR1` | 10 | Terminate | Application-defined (often reopen logs) |
| `SIGUSR2` | 12 | Terminate | Application-defined (often reload) |
| `SIGCHLD` | 17 | Ignore | Sent to parent when a child exits |
| `SIGSEGV` | 11 | Core dump | Invalid memory access |
| `SIGPIPE` | 13 | Terminate | Write to a closed pipe/socket |

```bash
kill -TERM 1042      # graceful
kill -HUP 1042       # reload, if supported
kill -KILL 1042      # force; no cleanup, no flush
kill -l              # list signals
```

**Signal choice matters:**

- `SIGTERM` lets the process close listeners, flush buffers, and deregister.
- `SIGKILL` can corrupt data, leave locks, and orphan children.
- `SIGHUP` is not universally "reload"; check the application.
- `SIGSTOP`/`SIGCONT` are for debugging, not for routine restarts.

`systemctl stop` sends `SIGTERM`, waits `TimeoutStopSec`, then `SIGKILL`. If your service takes 40 seconds to shut down, configure `TimeoutStopSec`, not `kill -9`.

---

## 5. Jobs and job control

A shell job is a pipeline or command managed by the shell. Job control is a terminal convenience, not a service manager.

```bash
$ sleep 300 &
[1] 2210

$ jobs -l
[1]+  2210 Running                 sleep 300

$ fg %1        # bring to foreground
$ bg %1        # continue in background
$ Ctrl-Z       # send SIGTSTP (stop)
$ disown %1    # remove from shell job table
```

When the terminal closes, the shell sends `SIGHUP` to its jobs. `nohup`, `setsid`, `tmux`, or systemd are the correct ways to keep a process alive. Do not use `&` for production services.

---

## 6. CPU, load average, and scheduling

```bash
$ uptime
 10:22:14 up 21 min,  2 users,  load average: 0.42, 0.18, 0.05

$ nproc
4
```

Load average counts runnable (`R`) **plus uninterruptible (`D`)** tasks, averaged over 1, 5, and 15 minutes. On a 4-CPU host:

- `4.0` ≈ all CPUs busy with runnable work.
- `8.0` ≈ queueing; latency rises.
- `2.0` with low CPU utilization may mean `D`-state I/O waits, not CPU work.

```bash
$ vmstat 1
procs -----------memory---------- ---swap-- -----io---- -system-- ------cpu-----
 r  b   swpd   free   buff  cache   si   so    bi    bo   in   cs us sy id wa st
 1  0      0 812340  20112 920144    0    0     4    18  220  410  3  1 95  1  0
 0  2      0 802110  20112 918220    0    0     0  2048  180  390  1  1 20 78  0
```

`r` = runnable, `b` = blocked (uninterruptible). `wa` = I/O wait. A high `b` with high `wa` points at storage/NFS, not CPU.

Scheduling controls:

```bash
nice -n 10 command          # start with lower priority
renice 5 -p 1042            # change priority (higher nice = lower priority)
chrt -f 50 command          # real-time FIFO (dangerous; can starve the system)
taskset -c 0-1 command      # CPU affinity
```

Nice values range from `-20` (highest priority) to `19` (lowest). Only root can decrease nice (increase priority).

### CPU accounting

```bash
$ pidstat -p 1042 1 3
Linux 6.8.0-31-generic ... 
Average:      UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
Average:     1000      1042    2.10    0.40    0.00    0.00    2.50     2  node
```

`%wait` is time waiting for CPU. `top -H` shows per-thread CPU; `perf top` shows hot functions. Use `pidstat` before `perf`; it is cheaper and answer-oriented.

---

## 7. `/proc`: the process's public record

```bash
$ ls /proc/1042
attr  cgroup  cmdline  cwd  environ  exe  fd  fdinfo  maps  mem  mountinfo
net  ns  oom_score  root  smaps  stack  stat  status  syscall  task  wchan

$ cat /proc/1042/status | head -20
Name:   node
State:  S (sleeping)
Tgid:   1042
Pid:    1042
PPid:   1
Uid:    1000 1000 1000 1000
Gid:    1000 1000 1000 1000
Threads: 11
VmPeak: 2845120 kB
VmSize: 2845120 kB
VmRSS:  492000 kB
RssAnon: 310000 kB
RssFile: 182000 kB
```

Useful files:

| File | Answers |
|---|---|
| `cmdline` | Exact command line (NUL-separated) |
| `cwd` | Current working directory (symlink) |
| `exe` | Binary path (symlink) |
| `fd/` | Open file descriptors |
| `environ` | Environment (NUL-separated; may contain secrets) |
| `maps` | Memory mappings |
| `smaps_rollup` | Accurate memory breakdown |
| `status` | State, UID/GID, threads, memory |
| `stat` | CPU times, start time, state |
| `wchan` | Kernel wait channel |
| `stack` | Kernel stack (root; useful for `D` state) |
| `limits` | Resource limits |
| `cgroup` | Which cgroup(s) the process belongs to |
| `oom_score` | OOM killer likelihood |

```bash
$ sudo cat /proc/1042/stack
[<0>] __rpc_wait_for_completion_task+0x...
[<0>] rpc_wait_bit_killable+0x...
[<0>] rpc_execute+0x...
[<0>] nfs_file_read+0x...
```

This is direct evidence of an NFS read wait.

---

## 8. Finding and inspecting processes

```bash
pgrep -a node
pgrep -u app -f 'server.js'
pstree -aps 1042
ps -eo pid,ppid,stat,cmd --forest | less

lsof -p 1042
lsof -i :8080
lsof +L1                 # deleted-but-open files
lsof /var/log/app.log

sudo strace -p 1042 -f -e trace=network,openat -s 128
sudo strace -f -c -p 1042   # summary of syscalls
```

`strace` attaches to a process and prints system calls. It is powerful but adds overhead; use it on a single PID, for a short window, and never on a latency-sensitive production process without approval.

```bash
$ sudo strace -p 1042 -e trace=accept4,recvfrom,sendto -s 100
accept4(7, {sa_family=AF_INET, sin_port=htons(51234), sin_addr=inet_addr("10.0.0.5")}, [16], SOCK_CLOEXEC) = 8
recvfrom(8, "GET /health HTTP/1.1\r\nHost: app\r\n\r\n", 8192, 0, NULL, NULL) = 38
sendto(8, "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK", 40, 0, NULL, 0) = 40
```

This tells you the process is alive, accepting, and responding — so the problem may be above or below it.

---

## 9. Worked scenarios

### Scenario A — high load, low CPU

```bash
$ uptime
 10:40:01 up 1 day, load average: 12.40, 11.80, 9.20
$ nproc
4
$ vmstat 1
 r  b   swpd   free   buff  cache   si   so    bi    bo   in   cs us sy id wa st
 1 11      0 210340  10200 640120    0    0     0  8192  120  280  2  1  8 89  0
```

Evidence: 11 blocked tasks, 89% I/O wait, almost no user CPU. Hypothesis: storage or NFS. Check `ps -eo stat,wchan,cmd | grep '^D'`, `iostat -x 1`, `dmesg`, and NFS mount health. Do not add CPU.

### Scenario B — zombie process

```bash
$ ps -eo pid,ppid,stat,cmd | grep Z
 1180  1042 Z    [worker] <defunct>
```

A zombie has already exited; it holds only a PID and exit status until the parent calls `wait()`. You cannot kill it. Fix the parent (restart it, or fix its signal handling). If the parent is `init`/PID 1, the zombie should be reaped; if not, the parent is buggy.

### Scenario C — "kill -9 does nothing"

```bash
$ ps -o pid,stat,wchan:30,cmd -p 2041
 2041 D    rpc_wait_bit_killable   rsync /mnt/nfs/data
```

`SIGKILL` is delivered, but the process cannot leave the kernel until the uninterruptible I/O completes. Options: restore the NFS server/network, use a hard/soft mount strategy, reboot as a last resort. Restarting the NFS server is usually safer than rebooting the client.

### Scenario D — service restarts every 30 seconds

```bash
$ systemctl status app
● app.service - App
     Loaded: loaded (/etc/systemd/system/app.service; enabled; preset: enabled)
     Active: activating (auto-restart) (Result: exit-code) since ...
    Process: 2210 ExecStart=/opt/app/server (code=exited, status=1/FAILURE)
   Main PID: 2210 (code=exited, status=1/FAILURE)

$ journalctl -u app -n 30 --no-pager
app[2210]: Error: listen EADDRINUSE: address already in use 0.0.0.0:8080
```

The service is not "flapping randomly"; it cannot bind because another process owns the port. `ss -tulpn | grep 8080` finds the owner. Check `Restart=` and `StartLimitBurst=` to understand the cadence, but fix the port conflict.

### Scenario E — process is alive but not serving

```bash
$ ss -tulpn | grep 8080
LISTEN 0 128 127.0.0.1:8080 0.0.0.0:* users:(("node",pid=1042,fd=7))
```

It listens only on loopback. Remote clients cannot connect. Fix the bind address (`0.0.0.0` or the specific interface), not the firewall.

---

## 10. Common mistakes

| Mistake | Why it is wrong | Better |
|---|---|---|
| `kill -9` first | Skips cleanup; can corrupt state | `SIGTERM`, wait, then `SIGKILL` |
| Assuming high load = high CPU | `D` state and I/O wait also count | `vmstat` `r`/`b`/`wa`, `ps` states |
| Killing a zombie | Zombies are already dead | Fix/reap the parent |
| Using `&` for a service | Dies with the shell | systemd, tmux, `nohup`/`setsid` only for temporary work |
| Looking only at `%CPU` in `ps` | It is a lifetime average | `top`/`pidstat` for current |
| Ignoring threads | A process can be alive while all threads block | `top -H`, `ps -eLf`, thread dumps |
| `strace` on everything | Huge overhead and noise | One PID, one syscall class, short window |
| Confusing VSZ with memory use | Virtual size includes reservations | RSS/PSS, `smaps_rollup` |
| Restarting before reading logs | Destroys evidence | `systemctl status`, `journalctl`, `/proc` first |

---

## 11. Command reference (quick)

```bash
# list / find
ps aux
ps -eo pid,ppid,user,stat,ni,psr,wchan:30,etime,cmd --sort=-%cpu
ps -eLf
pgrep -a -f pattern
pstree -aps PID

# state / wait
ps -o pid,stat,wchan:30,cmd -p PID
cat /proc/PID/status
sudo cat /proc/PID/stack
cat /proc/PID/wchan

# signals / jobs
kill -l
kill -TERM PID
kill -HUP PID
kill -KILL PID
jobs -l
fg %1
bg %1

# CPU / scheduling
uptime
vmstat 1
pidstat -p PID 1 5
top -H -p PID
nice -n 10 cmd
renice 5 -p PID
taskset -cp PID

# inspect
lsof -p PID
lsof -i :PORT
lsof +L1
strace -p PID -f -e trace=network
cat /proc/PID/cmdline | tr '\0' ' '
readlink /proc/PID/cwd /proc/PID/exe
```

---

## 12. Checkpoint

1. What is the difference between `S` and `D` state, and why can `D` not be killed?
2. Why does load average include `D`-state tasks?
3. A process has 11 threads and is "running" but not serving. How do you find the blocked thread?
4. What does `SIGTERM` do that `SIGKILL` does not?
5. Why is a zombie unkillable, and what actually fixes it?
6. What does `wchan` tell you?
7. Why is `VSZ` a poor indicator of memory pressure?
8. How do you find which process owns port 8080?
9. What is the difference between `ps %CPU` and `top %CPU`?
10. What is the first thing you check when a systemd service restarts every 30 seconds?

If you miss more than two, redo Lab L1.3.
