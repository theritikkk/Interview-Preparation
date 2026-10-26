# Filesystem, Inodes, and Permissions

> **The one-sentence model:** a pathname is a name in a directory that points to an inode; the inode holds metadata and data pointers; permissions are metadata checked by the kernel on every access.

If you understand that sentence, `ls -l`, `stat`, `chmod`, `chown`, ACLs, hard links, and most "permission denied" incidents become the same investigation.

---

## 1. The filesystem hierarchy (FHS)

Linux presents one tree, even when many filesystems are mounted into it.

| Path | What lives there | Operational note |
|---|---|---|
| `/` | Root of the tree | The root filesystem; if it fills, the host is in trouble |
| `/bin`, `/sbin` | Essential binaries (often symlinks to `/usr/bin`, `/usr/sbin`) | Usable in early boot / single-user mode |
| `/usr` | Most programs, libraries, documentation | Read-mostly; large |
| `/etc` | Host-specific configuration | Back it up before changes; no binaries |
| `/var` | Variable data: logs, spools, databases, containers | Most disk-full incidents happen here |
| `/var/log` | System and service logs | `journal`, `syslog`, `auth.log`, `messages` |
| `/home` | User home directories | Often a separate filesystem or network mount |
| `/root` | Root's home | Not `/home/root` |
| `/tmp` | Temporary files; often `tmpfs`; sticky bit | Reboot may clear it; do not store state |
| `/opt` | Optional/add-on software | Common for vendor apps |
| `/srv` | Data served by the system (web, FTP) | Site-specific |
| `/mnt`, `/media` | Temporary and removable mounts | Do not put permanent config here |
| `/proc` | Kernel and process state as files | `/proc/<pid>/`, `/proc/meminfo`, `/proc/sys` |
| `/sys` | Kernel devices, cgroups, power, firmware | `/sys/fs/cgroup`, `/sys/class/net` |
| `/dev` | Device nodes | `/dev/sda`, `/dev/nvme0n1`, `/dev/null` |
| `/run` | Runtime state since boot (PID files, sockets) | `tmpfs`; cleared on reboot |
| `/boot` | Kernel, initramfs, bootloader config | Do not fill; keep old kernels until verified |

```bash
$ findmnt -T /var/log
TARGET   SOURCE         FSTYPE FSTYPE OPTIONS
/        /dev/mapper/ubuntu--vg-ubuntu--lv ext4 rw,relatime

$ lsblk -o NAME,SIZE,FSTYPE,MOUNTPOINTS
NAME                      SIZE FSTYPE MOUNTPOINTS
nvme0n1                   40G
├─nvme0n1p1                1G vfat   /boot/efi
├─nvme0n1p2                2G ext4   /boot
└─nvme0n1p3               37G LVM2_member
  └─ubuntu--vg-ubuntu--lv 37G ext4   /
```

`findmnt` answers "which filesystem is this path on?" — the first question in any disk-space incident.

---

## 2. Inodes, links, and why `df` and `du` disagree

An **inode** stores:

- file type, mode, owner, group
- size, timestamps (atime, mtime, ctime)
- link count
- pointers to data blocks (and extended attributes/ACLs)

It does **not** store the filename. The filename is an entry in a directory that maps `name → inode number`.

```bash
$ ls -li /etc/hosts
131220 -rw-r--r-- 1 root root 221 2026-03-14 10:12 /etc/hosts
```

The first column is the inode number. `ls -i` is how you prove two paths are the same file.

### Hard link vs symbolic link

```bash
$ echo "hello" > a.txt
$ ln a.txt b.txt          # hard link
$ ln -s a.txt c.txt       # symbolic link
$ ls -li a.txt b.txt c.txt
131500 -rw-r--r-- 2 user user 6 ... a.txt
131500 -rw-r--r-- 2 user user 6 ... b.txt
131501 lrwxrwxrwx 1 user user 5 ... c.txt -> a.txt
```

| | Hard link | Symbolic link |
|---|---|---|
| Inode | Same inode | New inode containing a path |
| Across filesystems | No | Yes |
| To a directory | No (normally) | Yes |
| If target deleted | Data survives until last link is removed | Link becomes dangling |
| Permission checks | On the target inode | On the target after resolution |

**Operational consequence:** deleting a file removes a directory entry and decrements the link count. The data blocks are freed only when the link count reaches zero **and** no process has the file open.

That is the deleted-but-open file case:

```bash
$ df -h /var
Filesystem      Size  Used Avail Use% Mounted on
/dev/sda1        40G   40G     0 100% /

$ du -sh /var/* | sort -h | tail -3
120M    /var/lib
80M     /var/log
20M     /var/tmp

$ sudo lsof +L1 | head
COMMAND   PID USER   FD   TYPE DEVICE SIZE/OFF NLINK   NODE NAME
app       912 app    3w   REG  253,0 38G      0 131500 /var/log/app.log (deleted)
```

The file has no name, but the process still holds the inode open, so the space is not freed. Restart/reload the process or truncate the file through `/proc/<pid>/fd/<fd>`.

---

## 3. The permission model

```bash
$ ls -l /etc/passwd /etc/shadow /tmp /usr/bin/passwd
-rw-r--r-- 1 root root   2894 ... /etc/passwd
-rw-r----- 1 root shadow  932 ... /etc/shadow
drwxrwxrwt 1 root root   4096 ... /tmp
-rwsr-xr-x 1 root root  68208 ... /usr/bin/passwd
```

Ten characters:

```text
- rwx r-x r-x
│  │   │   └── other
│  │   └────── group
│  └────────── owner
└───────────── type: - file, d dir, l symlink, c char, b block, s socket, p fifo
```

### What `rwx` means for a file vs a directory

| Bit | On a file | On a directory |
|---|---|---|
| `r` | Read contents | List names |
| `w` | Modify contents | Create, delete, rename entries |
| `x` | Execute | Traverse / access entries |

A directory without `x` blocks access to everything inside, even if the files are `777`. This is the most common reason a permission fix "does not work".

```bash
$ ls -ld /srv/project /srv/project/report.txt
drw-r--r-- 2 root root 4096 ... /srv/project
-rw-rw-r-- 1 root root   12 ... /srv/project/report.txt

$ cat /srv/project/report.txt
cat: /srv/project/report.txt: Permission denied

$ namei -l /srv/project/report.txt
f: /srv/project/report.txt
drwxr-xr-x root root /
drwxr-xr-x root root srv
drw-r--r-- root root project
-rw-rw-r-- root root report.txt
```

`namei -l` is the fastest way to see which component lacks `x`. Use it before changing anything.

### Octal and symbolic modes

| Octal | Symbolic | Meaning |
|---:|---|---|
| 7 | `rwx` | read + write + execute |
| 6 | `rw-` | read + write |
| 5 | `r-x` | read + execute |
| 4 | `r--` | read |
| 0 | `---` | none |

```bash
chmod 750 file          # owner rwx, group r-x, other none
chmod u+x script.sh     # add execute for owner
chmod g-w,o-rwx file    # remove group write, all other
chmod -R g+rX /srv/app  # X = execute only on dirs / already-executable files
```

`chmod -R 777` is not a fix. It is a security incident with a delayed alert.

### Umask

`umask` is the set of bits **removed** from the default mode when a file is created.

```bash
$ umask
0022
$ touch new.txt && mkdir newdir
$ ls -ld new.txt newdir
-rw-r--r-- 1 user user 0 ... new.txt
drwxr-xr-x 2 user user 4096 ... newdir
```

Default file base is `666`, directory base is `777`. With `umask 022`:

- file: `666 & ~022 = 644`
- dir: `777 & ~022 = 755`

A restrictive operational umask is `027` (files `640`, dirs `750`). Set it in `/etc/profile.d/` or the service's `UMask=` directive.

### Ownership

```bash
chown alice file
chown alice:devs file
chgrp devs file
chown -R alice:devs /srv/project
```

Only root can change the owner. A normal user can change the group only to a group they belong to, and only if they own the file.

---

## 4. Special bits

| Bit | Octal | On a file | On a directory | Risk |
|---|---:|---|---|---|
| SUID | 4000 | Run with the file owner's effective UID | Ignored on Linux | Privilege escalation if the binary is unsafe |
| SGID | 2000 | Run with the file's group | New files inherit the directory's group | Group sprawl if misused |
| Sticky | 1000 | Ignored on Linux | Only owner/root can delete entries | Low; standard on `/tmp` |

```bash
$ ls -l /usr/bin/passwd
-rwsr-xr-x 1 root root 68208 ... /usr/bin/passwd
#    ^ SUID: a normal user runs passwd with effective UID 0

$ mkdir /srv/shared && chmod 2775 /srv/shared
$ ls -ld /srv/shared
drwxrwsr-x 2 root devs 4096 ... /srv/shared

$ ls -ld /tmp
drwxrwxrwt 1 root root 4096 ... /tmp
#         ^ sticky
```

Audit SUID/SGID:

```bash
find / -xdev -type f \( -perm -4000 -o -perm -2000 \) -ls 2>/dev/null
```

Do not remove SUID bits blindly; `passwd`, `sudo`, `mount`, and others may need them. Investigate unexpected ones.

---

## 5. ACLs: when owner/group/other is not enough

POSIX ACLs let you grant permissions to multiple users/groups without changing the file's primary owner/group.

```bash
$ getfacl /srv/project/report.txt
# file: srv/project/report.txt
# owner: root
# group: devs
user::rw-
user:alice:rwx
group::r-x
mask::rwx
other::r--

$ setfacl -m u:alice:rwx /srv/project/report.txt
$ setfacl -m g:auditors:r-- /srv/project/report.txt
$ setfacl -x u:alice /srv/project/report.txt
```

Default ACLs on a directory are inherited by new files:

```bash
setfacl -d -m g:devs:rwx /srv/project
setfacl -d -m o::- /srv/project
```

The `mask` limits the maximum permissions for named users/groups and the group class. If `getfacl` shows `mask::r--`, a named user with `rwx` still only gets `r--`.

```bash
setfacl -m m::rwx /srv/project/report.txt   # recalculate mask
```

`ls -l` shows a `+` when an ACL is present:

```bash
-rw-rw-r--+ 1 root devs 12 ... report.txt
```

**Operational use:** shared project directories, CI runners that need write access to one path, auditors who need read-only access, and service accounts that should not be in the file's group.

---

## 6. `sudo` and privilege boundaries

`sudo` runs a command as another user (default root) according to `/etc/sudoers` and `/etc/sudoers.d/*`.

```bash
$ sudo -l
User alice may run the following commands on web01:
    (root) NOPASSWD: /usr/bin/systemctl restart nginx
    (root) /usr/bin/journalctl -u nginx
```

Rules:

- Edit with `visudo` (or `visudo -f /etc/sudoers.d/<file>`), never directly.
- Prefer specific commands over `ALL`.
- `NOPASSWD` is a risk; use only for automation with a constrained command.
- `sudo -i` gives a root shell; `sudo -u app` runs as another service user.
- `sudo` logs to the journal/auth log; use it as evidence.

```bash
$ sudo -u postgres psql -c 'select 1;'
$ sudo -u app /opt/app/bin/healthcheck
```

**Do not fix permissions by running the service as root.** Fix the ownership or ACL so the service can run as its own user.

---

## 7. Capabilities, SELinux, AppArmor

### Capabilities

Root's power is split into capabilities. A binary can be granted one capability without full root.

```bash
$ getcap /usr/bin/ping
/usr/bin/ping cap_net_raw=ep

$ sudo setcap cap_net_bind_service=+ep /opt/app/server
$ getcap /opt/app/server
/opt/app/server cap_net_bind_service=ep
```

`cap_net_bind_service` lets a non-root process bind ports below 1024. `cap_net_raw` allows raw sockets. `cap_sys_admin` is effectively root for many operations; do not hand it out casually.

### SELinux (RHEL family)

```bash
$ getenforce
Enforcing

$ ls -Z /var/www/html/index.html
unconfined_u:object_r:httpd_sys_content_t:s0 /var/www/html/index.html

$ sudo restorecon -Rv /var/www/html
Relabeled /var/www/html/index.html from unconfined_u:object_r:user_home_t:s0 to unconfined_u:object_r:httpd_sys_content_t:s0

$ sudo ausearch -m avc -ts recent
type=AVC msg=audit(...): avc: denied { read } for pid=... comm="nginx" name="index.html" ...
```

A file can have `rwx` for everyone and still be denied because the SELinux type is wrong. `restorecon` fixes context from the policy; `chmod` does not.

### AppArmor (Ubuntu/Debian)

```bash
$ sudo aa-status
apparmor module is loaded.
...
$ sudo journalctl -k | grep -i apparmor
```

AppArmor profiles restrict path access per program. A denial appears in `dmesg`/`journalctl` with `apparmor="DENIED"`.

### Immutable and append-only attributes

```bash
$ lsattr /etc/ssh/sshd_config
----i---------e---- /etc/ssh/sshd_config
$ sudo chattr -i /etc/ssh/sshd_config
```

An immutable file cannot be modified even by root until the attribute is removed. This is rare but explains "operation not permitted" on a root-owned file.

---

## 8. The "permission denied" decision tree

Work top to bottom. Stop when a test explains the symptom.

```text
1. What is the exact error?
   EACCES (permission denied) vs EPERM (operation not permitted) vs EROFS (read-only)
       ↓
2. Which path component fails?
   namei -l /the/full/path
       ↓
3. Who is the process, and which groups does it have?
   ps -o user,group,pid,cmd -p <pid>
   sudo -u <user> id
       ↓
4. Do the mode bits allow it?
   ls -l, stat
   Remember: directory needs x; file needs r/w/x as appropriate
       ↓
5. Is an ACL overriding the mode?
   getfacl -p /path; check mask
       ↓
6. Is the filesystem mounted read-only or noexec?
   findmnt -T /path; mount | grep ' /path '
       ↓
7. Is the file immutable?
   lsattr /path
       ↓
8. Is SELinux/AppArmor denying it?
   ls -Z; ausearch -m avc -ts recent; aa-status; journalctl -k | grep -i denied
       ↓
9. Is a capability missing?
   getcap /path; capsh --print
       ↓
10. Is it a network filesystem with root squash / id mapping?
   findmnt -T /path; NFS export options; idmap
```

The command that often shortcuts steps 1–4:

```bash
$ sudo strace -f -e trace=openat,access,statx -p <pid>
openat(AT_FDCWD, "/srv/project/report.txt", O_RDONLY) = -1 EACCES (Permission denied)
```

It tells you the exact syscall and path the process tried.

---

## 9. Worked scenarios

### Scenario A — a shared directory for a team

Requirement: `alice` and `bob` can read/write `/srv/project`; new files are group-owned by `devs`; others have no access; future files inherit the ACL.

```bash
sudo groupadd devs
sudo usermod -aG devs alice
sudo usermod -aG devs bob
sudo mkdir -p /srv/project
sudo chown root:devs /srv/project
sudo chmod 2770 /srv/project
sudo setfacl -d -m g:devs:rwx /srv/project
sudo setfacl -d -m o::- /srv/project

# verify as alice
sudo -u alice touch /srv/project/a.txt
ls -l /srv/project/a.txt
# -rw-rw-r--+ 1 alice devs 0 ... a.txt
getfacl /srv/project/a.txt
```

Why `2770`: SGID makes new files inherit the `devs` group; `770` keeps others out. The default ACL makes the inheritance explicit for files created by tools that do not honor the group in the way you expect.

### Scenario B — a web root

Requirement: nginx can read files; developers can edit via a deploy user; no world-writable files.

```bash
sudo chown -R deploy:www-data /var/www/site
sudo find /var/www/site -type d -exec chmod 750 {} \;
sudo find /var/www/site -type f -exec chmod 640 {} \;
sudo usermod -aG www-data deploy
```

Directories need `x` for traversal; files do not. `750/640` is usually enough. If the web server runs as `www-data`, it needs `r` on files and `x` on directories.

### Scenario C — SSH key permissions

```bash
$ ssh -i ~/.ssh/id_ed25519 user@host
Warning: UNPROTECTED PRIVATE KEY FILE!
Permissions 0644 for '/home/user/.ssh/id_ed25519' are too open.
```

Fix:

```bash
chmod 700 ~/.ssh
chmod 600 ~/.ssh/id_ed25519
chmod 644 ~/.ssh/id_ed25519.pub
```

SSH refuses a private key that others can read. This is a security feature, not a bug.

### Scenario D — "root gets permission denied"

```bash
$ sudo cat /var/www/html/index.html
cat: /var/www/html/index.html: Permission denied
$ ls -l /var/www/html/index.html
-rwxrwxrwx. 1 root root 12 ... /var/www/html/index.html
$ ls -Z /var/www/html/index.html
unconfined_u:object_r:user_home_t:s0 ...
```

Root bypasses DAC, not SELinux. The type `user_home_t` is wrong for web content. `restorecon -Rv /var/www/html` fixes it.

### Scenario E — SUID audit

```bash
$ find / -xdev -type f -perm -4000 -ls 2>/dev/null | head
  131500  68 -rwsr-xr-x  1 root root  68208 /usr/bin/passwd
  131501  44 -rwsr-xr-x  1 root root  44528 /usr/bin/chsh
  131502  60 -rwsr-xr-x  1 root root  59736 /usr/bin/sudo
```

An unexpected SUID binary in `/tmp` or `/home` is a red flag. Investigate ownership, package membership (`rpm -qf`, `dpkg -S`), and hashes before deleting.

---

## 10. Common mistakes

| Mistake | Why it is wrong | Better |
|---|---|---|
| `chmod -R 777` | Grants write/execute to everyone; hides the real ownership problem | Fix owner/group, use `750/640`, ACLs |
| `chown -R user:user /` | Can break setuid binaries and system services | Change only the app path; use `rpm -Va` to verify |
| Forgetting directory `x` | Files are readable but unreachable | `namei -l`; add `x` to the directory |
| Changing a symlink's mode | Linux ignores symlink permissions | Change the target, or use `chmod -h` only where supported |
| Ignoring the ACL mask | Named user gets less than expected | `getfacl`, `setfacl -m m::...` |
| Setting `umask 000` | New files are world-writable | `027` or `022`; set per service |
| Running the service as root | Turns a bug into a host compromise | Run as a dedicated user; grant a capability if needed |
| Using `sudo` to test | Masks the service user's real permissions | `sudo -u <service-user> <command>` |
| Deleting a "large" file that is still open | Space is not freed | `lsof +L1`; restart/truncate the writer |
| Ignoring SELinux | `chmod` cannot fix a type mismatch | `ls -Z`, `restorecon`, `ausearch` |

---

## 11. Command reference (quick)

```bash
# identity and groups
id
id alice
groups alice
sudo -u app id

# mode and ownership
ls -l
ls -ld /path
stat /path
chmod 750 /path
chmod g+rX,o-rwx /path
chown alice:devs /path

# umask
umask
umask 027

# ACL
getfacl -p /path
setfacl -m u:alice:rwx /path
setfacl -m g:devs:r-x /path
setfacl -d -m g:devs:rwx /dir
setfacl -x u:alice /path
setfacl -b /path

# path resolution
namei -l /full/path
readlink -f /path

# special bits and attributes
find / -xdev -type f -perm -4000 -ls 2>/dev/null
lsattr /path
chattr +i /path

# capabilities / MAC
getcap /path
setcap cap_net_bind_service=+ep /path
getenforce
ls -Z /path
restorecon -Rv /path
ausearch -m avc -ts recent
aa-status

# links and inodes
ls -li
ln target hardlink
ln -s target symlink
find /path -inum 131500
```

---

## 12. Checkpoint

Without notes:

1. Why can a file be `777` but still unreadable by a normal user?
2. What does `namei -l` show that `ls -l` does not?
3. What is the difference between a hard link and a symlink when the target is deleted?
4. A process deleted `/var/log/app.log`, but `df` is still full. What happened, and how do you fix it without rebooting?
5. What does `umask 027` produce for a new file and a new directory?
6. When would you use an ACL instead of changing the group?
7. Why does `chmod 777` not fix an SELinux denial?
8. What is the risk of a SUID binary in a user's home directory?
9. How do you run a command as a service account to test its real permissions?
10. What is the first command you run for "permission denied" on a path you can see?

If you miss more than two, redo Labs L1.1 and L1.2.
