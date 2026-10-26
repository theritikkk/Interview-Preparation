# Phase 1 Checkpoint — Linux Fundamentals

> Closed book. 45 minutes. Write answers before scrolling to the key. A score below 80% means repeat the weak lab, not the whole phase.

## Part A — Rapid recall (10 × 2 points)

1. Which inode metadata changes when you `mv` a file within the same filesystem? Which does not?
2. A directory is `drw-r--r--`. Can a user with no other permissions `cat` a world-readable file inside? Why?
3. What does `umask 027` produce for a new regular file and a new directory?
4. What is the difference between `SIGTERM` and `SIGKILL`? Why does `kill -9` sometimes not remove a process?
5. What does `available` in `free -h` estimate that `free` does not?
6. What is the difference between `df -h` and `df -i`?
7. What does `findmnt -T /var/log` tell you that `ls -ld /var/log` does not?
8. What is the difference between `After=` and `Requires=` in a systemd unit?
9. What does `ss -tulpn` show, and what does it not show?
10. Why is `journalctl -b -1` useful after a crash?

## Part B — Read the output (6 × 5 points)

### B1

```text
$ ls -li a b c
131500 -rw-r--r-- 2 alice devs 12 ... a
131500 -rw-r--r-- 2 alice devs 12 ... b
131501 lrwxrwxrwx 1 alice devs  1 ... c -> a
```

What is `c`? What happens to `b` if `a` is deleted? What happens to `c`?

### B2

```text
$ df -h /var
Filesystem      Size  Used Avail Use% Mounted on
/dev/sda1        40G   40G     0 100% /
$ sudo du -xsh /var/* | sort -h | tail -3
120M    /var/lib
80M     /var/log
20M     /var/tmp
```

Give two plausible explanations and the command that distinguishes them.

### B3

```text
$ ps -o pid,stat,wchan:30,cmd -p 2041
  PID STAT WCHAN                          CMD
 2041 D    rpc_wait_bit_killable          rsync /mnt/nfs/data
```

What is the process doing? Why does `kill -9 2041` not help immediately? What would you check?

### B4

```text
$ free -h
               total        used        free      shared  buff/cache   available
Mem:           7.7Gi       2.1Gi       1.2Gi        28Mi       4.4Gi       5.2Gi
$ vmstat 1
 r  b   swpd   free   buff  cache   si   so ...
 1  0      0 812340  20112 920144    0    0 ...
```

Is the host under memory pressure? Justify with two numbers.

### B5

```text
$ systemctl status app
● app.service - App
     Active: activating (auto-restart) (Result: exit-code)
    Process: 2210 ExecStart=/opt/app/server (code=exited, status=1/FAILURE)
$ journalctl -u app -n 5
app[2210]: Error: listen EADDRINUSE: address already in use 0.0.0.0:8080
```

What is the root cause hypothesis? What is the next command? What is the safest fix?

### B6

```text
$ ip route
default via 10.0.0.1 dev eth0
10.0.0.0/24 dev eth0 proto kernel scope link src 10.0.0.10
$ ip route get 8.8.8.8
8.8.8.8 via 10.0.0.1 dev eth0 src 10.0.0.10
$ ping -c1 8.8.8.8
# timeout
$ ping -c1 10.0.0.1
# reply
```

What does this establish? What are three plausible causes? Which command would you run next?

## Part C — Scenarios (5 × 8 points)

### C1 — Shared directory

A team needs `/srv/reports` so that:
- `alice` and `bob` can create/delete files,
- new files are group-owned by `reports`,
- `auditor` can read all files but not write,
- others have no access.

Write the commands. Explain why `chmod 777` is wrong.

### C2 — Full disk

At 03:00, `/` is 100% full. `du -x /` finds only 12 GB of 40 GB. The app is a Node process writing `/var/log/app/app.log`. What are the three most likely causes, and how do you confirm each?

### C3 — Service fail

A systemd service works when you run `/opt/app/server` manually as `app`, but fails under systemd with `status=200/CHDIR`. What does that code mean, and what are the first five things you check?

### C4 — Permission denied

`sudo cat /var/www/html/index.html` returns `Permission denied`, but `ls -l` shows `-rwxrwxrwx`. What are the possible causes, and in what order do you test them?

### C5 — Logs

A service failed two hours ago and restarted. `journalctl -u app` shows only the last 10 minutes. The host rebooted 30 minutes ago. What happened to the earlier logs, and what would you check?

## Part D — Practical (20 points)

On a fresh VM, without notes:

1. Create a 100 MB file, then delete it while a process holds it open. Prove the space is not freed and free it without killing the process.
2. Create a user `deploy`, a group `web`, and a directory `/srv/web` with SGID and a default ACL. Prove a new file inherits the group.
3. Write a systemd service that runs `sleep infinity` as `deploy`, then break it with a wrong `ExecStart` and fix it via a drop-in.
4. Find the top three directories by size under `/var` and the top three by inode count.
5. Explain, in 3 minutes, the path from `open("/etc/passwd")` to a permission check.

---

## Answer key

### Part A

1. **Same filesystem `mv`:** the directory entry changes; the inode number and link count usually stay the same (rename). Across filesystems, it is a copy + unlink, so the inode changes.
2. **No.** The directory lacks `x`, so the user cannot traverse it to reach the file. `r` on a directory only lists names. (`root` bypasses DAC, but the question says a normal user.)
3. **File `640`** (`666 & ~027`), **directory `750`** (`777 & ~027`).
4. `SIGTERM` is catchable and allows cleanup; `SIGKILL` is handled by the kernel and cannot be caught. A process in uninterruptible sleep (`D`) cannot leave the kernel until its I/O completes, so `SIGKILL` is pending.
5. `available` estimates memory that can be made available to new workloads (reclaimable cache + free), not just completely free pages.
6. `df -h` shows block space; `df -i` shows inode usage. A filesystem can be full in either dimension.
7. `findmnt` shows the filesystem type, source device, and mount options (`ro`, `noexec`, `nosuid`, NFS). `ls -ld` only shows the directory's metadata.
8. `After=` orders startup if both units are started; `Requires=` pulls the dependency in and propagates failure.
9. `ss -tulpn` shows sockets (TCP/UDP), listening state, local/peer addresses, and owning process. It does not show whether the application behind the socket is healthy or whether packets are being dropped.
10. It shows the previous boot's journal, which may contain the crash/OOM/kernel error that caused the reboot, even if the current boot's journal is clean.

### Part B

**B1:** `c` is a symlink to `a`. Deleting `a` leaves `b` with the data (link count 1); `c` becomes a dangling symlink. The inode is freed only when `b` is also removed and no process holds it open.

**B2:** (1) a deleted-but-open file held by a process; `sudo lsof +L1`. (2) inode exhaustion; `df -i`. (3) `du` is not seeing another mount/permission (less likely given the numbers); `findmnt`, run `du` as root. The two strongest are deleted-open and inodes.

**B3:** The process is in uninterruptible sleep waiting on an NFS RPC (`rpc_wait_bit_killable`). `SIGKILL` cannot take effect until the kernel I/O completes. Check the NFS server/network, the mount options (`hard`/`soft`, `intr`), and `dmesg`. Rebooting the client is a last resort.

**B4:** No. `available` is 5.2 GiB, `si/so` are 0, and there is no blocked (`b`) work. `buff/cache` is reclaimable. The host is using RAM as a cache.

**B5:** Another process already owns port 8080. `ss -tulpn | grep 8080` finds the owner. Safest fix: stop/relocate the conflicting process or change the app's port; then restart. Do not `kill -9` blindly until you know what owns the port.

**B6:** The route and source address are correct; the host can reach its gateway but not 8.8.8.8. Plausible: (1) upstream NAT/firewall blocks outbound; (2) the gateway is not forwarding/NATing; (3) DNS is irrelevant here (using an IP). Next: `traceroute -n 8.8.8.8`, `tcpdump -ni eth0 icmp`, and check the cloud security group/NAT.

### Part C

**C1:**

```bash
sudo groupadd reports
sudo usermod -aG reports alice
sudo usermod -aG reports bob
sudo mkdir -p /srv/reports
sudo chown root:reports /srv/reports
sudo chmod 2770 /srv/reports
sudo setfacl -m u:auditor:rX /srv/reports
sudo setfacl -d -m g:reports:rwx /srv/reports
sudo setfacl -d -m u:auditor:rX /srv/reports
sudo setfacl -d -m o::- /srv/reports
```

`777` gives everyone write/execute and removes accountability; it also lets any user delete or replace files. Use group ownership, SGID for inheritance, default ACLs, and least privilege.

**C2:**

1. **Deleted-open file** — `sudo lsof +L1`; truncate through `/proc/<pid>/fd/<fd>` or restart the writer.
2. **Inode exhaustion** — `df -i`; find the directory with millions of files.
3. **Reserved blocks / other mount** — `df -hT`, `findmnt`, `tune2fs -l` (`Reserved block count`); `du -x` to stay on one filesystem.
Also check `tmpfs` (`df -hT` shows `tmpfs`) and container overlay mounts.

**C3:** `200/CHDIR` means systemd could not change to `WorkingDirectory`. Check: (1) path exists; (2) `User=`/`Group=` can traverse it; (3) permissions/ACL; (4) SELinux/AppArmor; (5) typo/relative path. Then `systemctl cat`, `systemctl show -p WorkingDirectory`, `namei -l`.

**C4:** Order: (1) `namei -l` parent traversal; (2) `ls -Z`/SELinux or AppArmor; (3) `lsattr` immutable; (4) `findmnt` `ro`/`noexec`; (5) ACL mask; (6) NFS root squash/id mapping; (7) capability. Root bypasses DAC, not MAC/immutable/read-only.

**C5:** The journal was likely volatile (`/run/log/journal`, no `/var/log/journal`), so the reboot discarded the earlier boot's logs. Check `journalctl --list-boots`, `/var/log/journal` existence, `Storage=` in `journald.conf`, and any forwarded logs (rsyslog/remote). Enable persistent journal and forwarding.

### Part D rubric

| Item | Pass criteria |
|---|---|
| Deleted-open file | `lsof +L1` evidence; space freed without killing writer |
| Shared directory | SGID + default ACL; `getfacl` shows inheritance; `namei -l` clean |
| systemd | Unit runs as `deploy`; failure code read; drop-in fixes; `daemon-reload` used |
| Top dirs | `du -xhd1` and inode count pipeline; correct paths |
| Open path | Mentions `namei`, parent `x`, owner/group/other, ACL, LSM |

**Scoring:** 80+ ready for Phase 2. 60–79 repeat the two weakest labs. Below 60 repeat Phase 1 with the command field manual open only after each attempt.
