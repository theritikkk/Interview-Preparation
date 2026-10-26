# Labs L1.1–L1.4

> **Safety:** every lab assumes a disposable Linux VM. Snapshot before starting. Do not run disk, user, or permission experiments on your Mac or a shared server. If a command can destroy data, it is marked **DESTRUCTIVE**.

## Lab L1.1 — Filesystem, links, and inodes

### 1. Learning objective

Prove that a filename is a directory entry pointing to an inode; observe hard links, symlinks, and inode reuse; use `find` and `stat` to investigate a path.

### 2. Prerequisites

- A Linux VM with a normal sudo user.
- Basic shell navigation.

### 3. Environment and setup

```bash
mkdir -p ~/lab/l1.1 && cd ~/lab/l1.1
pwd
df -hT .
findmnt -T .
```

### 4. Commands

```bash
# 1. inode identity
echo "hello" > a.txt
ls -li a.txt
stat a.txt

# 2. hard link and symlink
ln a.txt b.txt
ln -s a.txt c.txt
ls -li a.txt b.txt c.txt

# 3. edit through one name
echo "world" >> b.txt
cat a.txt
cat c.txt

# 4. delete the original name
rm a.txt
cat b.txt
cat c.txt          # symlink now dangling
ls -l c.txt

# 5. find by inode and link count
find . -inum "$(stat -c %i b.txt)"
find . -type f -links +1 -ls

# 6. inode numbers are reused
touch x.txt
ls -i x.txt
rm x.txt
touch y.txt
ls -i y.txt        # may reuse the freed inode
```

### 5. Expected observations

- `a.txt` and `b.txt` share an inode and link count 2.
- `c.txt` has a different inode and contains the path `a.txt`.
- Writing through `b.txt` changes the content seen by `a.txt` and `c.txt` while `a.txt` exists.
- After `rm a.txt`, `b.txt` still has the data (link count 1) and `c.txt` is dangling.

Representative:

```text
$ ls -li a.txt b.txt c.txt
131500 -rw-r--r-- 2 user user 6 ... a.txt
131500 -rw-r--r-- 2 user user 6 ... b.txt
131501 lrwxrwxrwx 1 user user 5 ... c.txt -> a.txt
```

### 6. What happens internally

`ln` creates a second directory entry with the same inode number and increments `i_nlink`. `ln -s` allocates a new inode whose data is the target path; the kernel resolves it on access. `rm` removes the directory entry and decrements the link count; the inode/data are freed only at zero links and zero open file descriptors.

### 7. Troubleshooting task

Create a symlink with an absolute path, then rename the target directory. Observe the breakage. Fix the link without recreating it incorrectly.

```bash
mkdir -p dir1 && echo data > dir1/file
ln -s "$PWD/dir1/file" link1
mv dir1 dir2
cat link1          # fails
readlink -f link1  # shows old path
```

### 8. Verify the fix

```bash
ln -sfn "$PWD/dir2/file" link1
readlink -f link1
cat link1
```

### 9. Cleanup

```bash
cd ~ && rm -rf ~/lab/l1.1
```

### 10. Write-up template

```markdown
# Lab L1.1 — Filesystem, links, inodes
Date:
Environment:
Objective:
Commands and output:
What the inode/link counts proved:
Break/fix:
What I would check first next time:
```

---

## Lab L1.2 — Permissions, umask, ACLs, special bits

### 1. Learning objective

Build a shared directory that behaves correctly for a team; explain why a file can be `777` and still unreadable; use ACLs and `namei -l`.

### 2. Prerequisites

- L1.1.
- Two local test users (`alice`, `bob`) or use `sudo -u` with existing users.

### 3. Environment and setup

```bash
sudo groupadd -f devs
sudo useradd -m -G devs alice 2>/dev/null || sudo usermod -aG devs alice
sudo useradd -m -G devs bob   2>/dev/null || sudo usermod -aG devs bob
sudo mkdir -p /srv/project
```

### 4. Commands

```bash
# 1. default permissions and umask
umask
touch /tmp/umask-test && ls -l /tmp/umask-test

# 2. shared directory: SGID + mode
sudo chown root:devs /srv/project
sudo chmod 2770 /srv/project
ls -ld /srv/project

# 3. default ACL for future files
sudo setfacl -d -m g:devs:rwx /srv/project
sudo setfacl -d -m o::- /srv/project
getfacl /srv/project

# 4. create as alice, inspect group inheritance
sudo -u alice touch /srv/project/alice.txt
ls -l /srv/project/alice.txt
getfacl /srv/project/alice.txt

# 5. named user ACL on one file
echo secret | sudo tee /srv/project/report.txt >/dev/null
sudo setfacl -m u:bob:r-- /srv/project/report.txt
sudo -u bob cat /srv/project/report.txt
getfacl /srv/project/report.txt

# 6. break it: remove directory execute
sudo chmod o-x /srv/project
sudo -u bob ls /srv/project      # still works? bob is in devs; test a non-member
sudo -u nobody ls /srv/project   # permission denied
namei -l /srv/project/report.txt

# 7. restore
sudo chmod 2770 /srv/project
```

### 5. Expected observations

- `2770` shows `drwxrws---` (SGID).
- New files are group-owned by `devs`, not by alice's primary group.
- `getfacl` shows a default ACL and a `+` on `ls -l`.
- Removing `x` from a directory blocks traversal; `namei -l` shows which component failed.

### 6. What happens internally

The kernel checks the inode mode, owner, group, and ACL mask for each path component. Directory `x` is required to traverse; `r` is required to list. SGID on a directory causes new entries to inherit the directory's group. Default ACLs are copied into new inodes at creation.

### 7. Troubleshooting task

Make a file `777` but unreadable by a normal user, without changing the file mode. Use a parent directory without `x` or an SELinux/AppArmor denial. Prove it with `namei -l` and `strace`.

```bash
sudo mkdir -p /srv/blocked
echo hi | sudo tee /srv/blocked/file >/dev/null
sudo chmod 777 /srv/blocked/file
sudo chmod 700 /srv/blocked
sudo -u nobody cat /srv/blocked/file
namei -l /srv/blocked/file
```

### 8. Verify the fix

```bash
sudo chmod 755 /srv/blocked
sudo -u nobody cat /srv/blocked/file
```

### 9. Cleanup

```bash
sudo rm -rf /srv/project /srv/blocked
sudo userdel -r alice 2>/dev/null || true
sudo userdel -r bob 2>/dev/null || true
sudo groupdel devs 2>/dev/null || true
rm -f /tmp/umask-test
```

### 10. Write-up template

```markdown
# Lab L1.2 — Permissions and ACLs
Date:
Objective:
The shared-directory design (owner/group/mode/ACL):
Why 777 was not the fix:
`namei -l` evidence:
What I would do differently:
```

---

## Lab L1.3 — Processes, signals, and jobs

### 1. Learning objective

Observe process states, parent/child relationships, signal handling, zombies, and job control; explain the difference between `SIGTERM` and `SIGKILL`.

### 2. Prerequisites

- L1.1, basic OS process concepts.

### 3. Environment and setup

```bash
sudo apt install -y procps psmisc   # Debian/Ubuntu
# or: sudo dnf install -y procps-ng psmisc
```

### 4. Commands

```bash
# 1. process tree and states
sleep 300 &
SLEEP_PID=$!
ps -o pid,ppid,stat,wchan:20,cmd -p $SLEEP_PID
pstree -aps $$

# 2. signals: TERM is catchable, KILL is not
bash -c 'trap "echo TERM received; exit 0" TERM; while :; do sleep 1; done' &
TRAP_PID=$!
sleep 1
kill -TERM $TRAP_PID
wait $TRAP_PID; echo "exit=$?"

bash -c 'trap "echo TERM received" TERM; while :; do sleep 1; done' &
TRAP2=$!
kill -KILL $TRAP2
wait $TRAP2; echo "exit=$?"

# 3. stop/continue
sleep 300 &
STOP_PID=$!
kill -STOP $STOP_PID
ps -o pid,stat,cmd -p $STOP_PID
kill -CONT $STOP_PID
ps -o pid,stat,cmd -p $STOP_PID

# 4. zombie: child exits, parent does not wait
python3 - <<'PY' &
import os, time
pid = os.fork()
if pid == 0:
    os._exit(0)
time.sleep(60)
PY
ZPARENT=$!
sleep 1
ps -eo pid,ppid,stat,cmd | awk -v p="$ZPARENT" '$2==p || $1==p'
kill $ZPARENT 2>/dev/null

# 5. job control
sleep 300 &
jobs -l
fg %1
# Ctrl-Z, then:
bg %1
jobs -l
```

### 5. Expected observations

- `sleep` is `S` (interruptible sleep).
- The trap process prints `TERM received` and exits 0 on `SIGTERM`.
- The second process dies on `SIGKILL`; the trap does not run.
- `SIGSTOP` shows `T`, `SIGCONT` returns it to `S`.
- The zombie shows `Z` and `<defunct>`; it disappears when the parent exits.

### 6. What happens internally

The kernel scheduler tracks task state. Signals are delivered on return from kernel mode; `SIGKILL` is handled by the kernel and cannot be caught. A child that exits becomes a zombie until the parent calls `wait()`/`waitpid()`; `SIGCHLD` notifies the parent. `SIGSTOP`/`SIGCONT` change the task state without terminating it.

### 7. Troubleshooting task

Create a process in `D` state using a slow/blocked I/O path (an NFS mount with the server paused, or a `dd` to a slow loop device). Try `kill -9`; observe that it does not disappear until the I/O completes. **Do this only in the VM.**

If you do not have NFS, use a FUSE/loop slowdown or simply inspect an existing `D` process. The point is the state, not the exact cause.

### 8. Verify the fix

For the zombie, ensure the parent exits or reaps; `ps` should no longer show `Z`. For the `D` process, restore the I/O path or reboot the VM; verify the PID is gone.

### 9. Cleanup

```bash
jobs -p | xargs -r kill 2>/dev/null || true
pkill -f 'sleep 300' 2>/dev/null || true
```

### 10. Write-up template

```markdown
# Lab L1.3 — Processes and signals
Date:
State table (R/S/D/T/Z) with one example each:
TERM vs KILL evidence:
Zombie cause and fix:
What `wchan` showed:
```

---

## Lab L1.4 — Memory, swap, and cgroup OOM

### 1. Learning objective

Distinguish page cache from anonymous memory; read `free`/`vmstat`; trigger an OOM kill **inside a cgroup**, not on the host; identify the evidence.

### 2. Prerequisites

- L1.3, basic virtual memory.

### 3. Environment and setup

**Critical:** do not run a host-wide memory bomb. Use a cgroup limit. On Ubuntu 24.04 with cgroup v2:

```bash
mount | grep cgroup2
systemd --version | head -1
```

### 4. Commands

```bash
# 1. baseline
free -h
cat /proc/meminfo | head -8
vmstat 1 3

# 2. page cache
dd if=/dev/zero of=/tmp/cache-test bs=1M count=512 status=none
free -h
# the file is in buff/cache; it is reclaimable
rm /tmp/cache-test

# 3. cgroup OOM (safe: 128 MiB limit, one process)
sudo systemd-run --scope -p MemoryMax=128M -p MemorySwapMax=0 \
  python3 -c 'a=[]; print("allocating"); 
while True: a.append(bytearray(1024*1024))'
echo "exit=$?"

# 4. evidence
sudo journalctl -k --since "2 min ago" | grep -i -E 'oom|killed process'
sudo dmesg -T | grep -i -E 'oom|killed process' | tail
```

If `systemd-run --scope` is unavailable, use Docker:

```bash
docker run --rm --memory=128m --memory-swap=128m python:3.12-slim \
  python -c 'a=[]; 
while True: a.append(bytearray(1024*1024))'
```

### 5. Expected observations

- After `dd`, `free` shows more `buff/cache`, less `free`; `available` stays high.
- The cgroup process is killed around 128 MiB; shell exit is `137`.
- `dmesg`/journal shows `Out of memory: Killed process ... python3` with `anon-rss` near the limit.
- `memory.events` for the scope shows `oom_kill`.

### 6. What happens internally

`dd` writes pages into the page cache; they are clean/reclaimable. The Python process allocates anonymous pages; the cgroup's `memory.max` forces reclaim, then the cgroup OOM killer selects a process and sends `SIGKILL`. The host kernel may not OOM because the cgroup contains the pressure.

### 7. Troubleshooting task

Run the allocation with `MemoryMax=128M` but `MemorySwapMax=64M`, and with `MemoryHigh=96M`. Observe the difference between throttling (`memory.high`) and a hard kill (`memory.max`). Read `memory.events` and `memory.stat` during the run from another terminal.

```bash
# terminal 1
sudo systemd-run --unit=memlab --scope -p MemoryMax=128M -p MemoryHigh=96M \
  python3 -c 'a=[]; 
while True: a.append(bytearray(1024*1024))'

# terminal 2
systemctl show memlab.scope -p MemoryCurrent -p MemoryMax -p MemoryHigh
cat /sys/fs/cgroup/system.slice/memlab.scope/memory.events 2>/dev/null
```

### 8. Verify the fix

For a real service, set a realistic `MemoryMax`, a `MemoryHigh` throttle, and an alert on `memory.events` `oom_kill`. Verify the service restarts cleanly and does not leak. For the lab, confirm no host-level OOM occurred and the cgroup was the boundary.

### 9. Cleanup

```bash
sudo systemctl stop memlab.scope 2>/dev/null || true
sudo systemctl reset-failed memlab.scope 2>/dev/null || true
rm -f /tmp/cache-test
```

### 10. Write-up template

```markdown
# Lab L1.4 — Memory and OOM
Date:
`free` before/after page cache:
`available` vs `free` explanation:
cgroup limit and evidence:
Host OOM vs cgroup OOM:
What I would alert on:
```
