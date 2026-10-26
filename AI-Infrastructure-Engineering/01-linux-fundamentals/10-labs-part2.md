# Labs L1.5–L1.8

> Continue in the disposable VM. Snapshot before L1.5. Do not run the disk commands on your Mac or on a shared host.

## Lab L1.5 — Disks, LVM, inodes, deleted-open files

### 1. Learning objective

Add a virtual disk, create an LVM volume, mount it, fill it, extend it, exhaust inodes on a small filesystem, and free space held by a deleted-open file.

### 2. Prerequisites

- L1.1–L1.4.
- A VM where you can add a 2 GB virtual disk.
- Root/sudo.

### 3. Environment and setup

Add a second disk in your hypervisor (2 GB is enough). Then:

```bash
lsblk
sudo dmesg -T | tail -20
```

Identify the new device (e.g. `/dev/sdb` or `/dev/nvme0n2`). **Confirm it is not the root disk.**

### 4. Commands

```bash
# 1. partition (optional; LVM can use the whole disk)
sudo parted -s /dev/sdb mklabel gpt
sudo parted -s /dev/sdb mkpart primary 0% 100%
lsblk /dev/sdb

# 2. LVM
sudo pvcreate /dev/sdb1
sudo vgcreate labvg /dev/sdb1
sudo lvcreate -L 1G -n lablv labvg
sudo mkfs.ext4 /dev/labvg/lablv
sudo mkdir -p /mnt/lab
sudo mount /dev/labvg/lablv /mnt/lab
findmnt /mnt/lab

# 3. persist in fstab (use UUID)
sudo blkid /dev/labvg/lablv
echo "UUID=$(sudo blkid -s UUID -o value /dev/labvg/lablv) /mnt/lab ext4 defaults 0 2" | sudo tee -a /etc/fstab
sudo mount -a
findmnt /mnt/lab

# 4. fill it (safe: lab volume only)
sudo dd if=/dev/zero of=/mnt/lab/fill bs=1M status=progress || true
df -h /mnt/lab

# 5. extend the LV and the filesystem
sudo lvextend -L +512M /dev/labvg/lablv
sudo resize2fs /dev/labvg/lablv
df -h /mnt/lab

# 6. inode exhaustion on a tiny loop filesystem
dd if=/dev/zero of=/tmp/inode.img bs=1M count=8 status=none
mkfs.ext4 -N 64 /tmp/inode.img
mkdir -p /mnt/inodes
sudo mount -o loop /tmp/inode.img /mnt/inodes
for i in $(seq 1 100); do sudo touch /mnt/inodes/f$i 2>/dev/null || break; done
df -i /mnt/inodes
df -h /mnt/inodes

# 7. deleted-open file
python3 - <<'PY' &
import time
f = open('/mnt/lab/open.log', 'w')
for i in range(1000000):
    f.write('x' * 1024 + '\n')
    if i % 1000 == 0:
        f.flush()
        time.sleep(0.01)
PY
WRITER=$!
sleep 2
sudo rm /mnt/lab/open.log
df -h /mnt/lab
sudo lsof +L1 | grep open.log
sudo truncate -s 0 /proc/$WRITER/fd/3 2>/dev/null || true
kill $WRITER 2>/dev/null || true
```

### 5. Expected observations

- `lsblk` shows `sdb → sdb1 → labvg-lablv → /mnt/lab`.
- `df -h` grows after `lvextend` + `resize2fs`; `lvextend` alone does not grow the filesystem.
- `df -i` reaches 100% while `df -h` still has free blocks.
- `lsof +L1` shows the deleted log with `NLINK 0`; truncating through `/proc/<pid>/fd/3` frees space without killing the writer.

### 6. What happens internally

LVM maps logical extents to physical extents. `mkfs` writes filesystem metadata (including inode tables). The VFS resolves a path to an inode; deleting a name decrements `i_nlink`; the data blocks remain allocated while a file descriptor is open. `truncate` on the open fd changes the inode size and frees blocks.

### 7. Troubleshooting task

Fill the **inode** filesystem (`df -i` 100%) while blocks remain. Try to create a file; observe `No space left on device`. Then delete half the files and prove creation works. Explain why adding a bigger disk would not have fixed it.

### 8. Verify the fix

```bash
df -i /mnt/inodes
sudo find /mnt/inodes -type f -delete
df -i /mnt/inodes
sudo touch /mnt/inodes/after && echo ok
```

For the deleted-open file, `df` should drop after truncation; `lsof +L1` should no longer show the large file.

### 9. Cleanup

```bash
kill $WRITER 2>/dev/null || true
sudo umount /mnt/inodes 2>/dev/null || true
sudo umount /mnt/lab 2>/dev/null || true
sudo sed -i '\#/mnt/lab#d' /etc/fstab
sudo lvremove -f /dev/labvg/lablv
sudo vgremove labvg
sudo pvremove /dev/sdb1
rm -f /tmp/inode.img
sudo rmdir /mnt/lab /mnt/inodes 2>/dev/null || true
```

Remove the virtual disk in the hypervisor after the VM is shut down.

### 10. Write-up template

```markdown
# Lab L1.5 — Disks, LVM, inodes
Date:
Device topology (lsblk):
LVM extend sequence:
Inode exhaustion evidence:
Deleted-open file evidence and fix:
What I would alert on:
```

---

## Lab L1.6 — systemd service, drop-in, and timer

### 1. Learning objective

Write a service unit, run it as a dedicated user, inspect its cgroup and journal, break it, fix it with a drop-in, and schedule it with a timer.

### 2. Prerequisites

- L1.3, L1.5.
- A VM with systemd.

### 3. Environment and setup

```bash
sudo useradd --system --home /opt/labapp --shell /usr/sbin/nologin labapp 2>/dev/null || true
sudo mkdir -p /opt/labapp /var/lib/labapp
sudo chown -R labapp:labapp /opt/labapp /var/lib/labapp
```

### 4. Commands

```bash
# 1. app script
sudo tee /opt/labapp/app.sh >/dev/null <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
echo "$(date -Is) labapp starting pid=$$"
while true; do
  echo "$(date -Is) tick"
  sleep 10
done
EOF
sudo chmod 755 /opt/labapp/app.sh
sudo chown labapp:labapp /opt/labapp/app.sh

# 2. service unit
sudo tee /etc/systemd/system/labapp.service >/dev/null <<'EOF'
[Unit]
Description=Lab App
After=network.target

[Service]
Type=simple
User=labapp
Group=labapp
WorkingDirectory=/opt/labapp
ExecStart=/opt/labapp/app.sh
Restart=on-failure
RestartSec=3
TimeoutStopSec=10
NoNewPrivileges=true
PrivateTmp=true

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl daemon-reload
sudo systemctl enable --now labapp.service
systemctl status labapp --no-pager
journalctl -u labapp -n 10 --no-pager

# 3. cgroup and process tree
systemctl show labapp -p MainPID -p MemoryCurrent -p ControlGroup
systemd-cgls /system.slice/labapp.service

# 4. break it: wrong ExecStart
sudo systemctl edit labapp --full
# change ExecStart to /opt/labapp/missing.sh, save
sudo systemctl daemon-reload
sudo systemctl restart labapp
systemctl status labapp --no-pager
journalctl -xeu labapp --no-pager | tail -20

# 5. fix with a drop-in
sudo mkdir -p /etc/systemd/system/labapp.service.d
sudo tee /etc/systemd/system/labapp.service.d/fix.conf >/dev/null <<'EOF'
[Service]
ExecStart=
ExecStart=/opt/labapp/app.sh
EOF
sudo systemctl daemon-reload
sudo systemctl restart labapp
systemctl status labapp --no-pager
systemctl cat labapp

# 6. timer
sudo tee /etc/systemd/system/labreport.service >/dev/null <<'EOF'
[Unit]
Description=Write a lab report timestamp

[Service]
Type=oneshot
User=labapp
ExecStart=/usr/bin/bash -c 'date -Is >> /var/lib/labapp/report.log'
EOF

sudo tee /etc/systemd/system/labreport.timer >/dev/null <<'EOF'
[Unit]
Description=Run lab report every minute

[Timer]
OnCalendar=*-*-* *:*:00
Persistent=true
Unit=labreport.service

[Install]
WantedBy=timers.target
EOF

sudo systemctl daemon-reload
sudo systemctl enable --now labreport.timer
systemctl list-timers --all | grep labreport
sudo systemctl start labreport.service
cat /var/lib/labapp/report.log
```

### 5. Expected observations

- `status` shows `active (running)`, a Main PID, a cgroup, and journal lines.
- Changing `ExecStart` to a missing path produces `status=203/EXEC` and a failed unit.
- The drop-in appears in `systemctl cat` and fixes the service.
- `list-timers` shows `NEXT` and `LAST`; the report file gets a timestamp.

### 6. What happens internally

systemd reads unit files, builds a dependency graph, creates a cgroup, forks/execs the process, and captures stdout/stderr into the journal. `daemon-reload` re-reads units; `restart` stops and starts. A timer activates the `.service` unit on schedule; `Persistent=true` runs a missed job after downtime.

### 7. Troubleshooting task

Make the service fail with `status=200/CHDIR` by setting `WorkingDirectory` to a nonexistent path. Diagnose from `systemctl status` and `journalctl`, then fix it. Explain why the process exit code alone was not enough.

### 8. Verify the fix

```bash
systemctl is-active labapp
systemctl show labapp -p WorkingDirectory -p ExecStart
curl -fsS http://127.0.0.1:8080/health 2>/dev/null || true
journalctl -u labapp --since "2 min ago" --no-pager
```

### 9. Cleanup

```bash
sudo systemctl disable --now labreport.timer labapp.service
sudo rm -f /etc/systemd/system/labreport.service /etc/systemd/system/labreport.timer
sudo rm -rf /etc/systemd/system/labapp.service.d
sudo rm -f /etc/systemd/system/labapp.service
sudo systemctl daemon-reload
sudo rm -rf /opt/labapp /var/lib/labapp
sudo userdel labapp 2>/dev/null || true
```

### 10. Write-up template

```markdown
# Lab L1.6 — systemd
Date:
Unit file:
Why the first start failed:
Drop-in that fixed it:
Timer schedule and evidence:
What I would monitor:
```

---

## Lab L1.7 — Logs, journald, and logrotate

### 1. Learning objective

Generate and query logs, build a timeline, configure rotation, force a rotation, and explain why rotation can fail to free space.

### 2. Prerequisites

- L1.6.
- Root/sudo.

### 3. Environment and setup

```bash
sudo mkdir -p /var/log/labapp
sudo chown labapp:labapp /var/log/labapp 2>/dev/null || true
```

### 4. Commands

```bash
# 1. journal: generate structured messages
logger -t labapp "startup"
logger -t labapp -p err "simulated upstream error"
journalctl -t labapp --since "5 min ago" -o short-iso

# 2. kernel and unit filters
journalctl -k -b | tail -10
journalctl -p err -b --no-pager | tail -20

# 3. file logs
sudo tee /etc/logrotate.d/labapp >/dev/null <<'EOF'
/var/log/labapp/*.log {
    size 1M
    rotate 3
    compress
    delaycompress
    missingok
    notifempty
    create 0640 labapp labapp
    sharedscripts
    postrotate
        systemctl reload labapp >/dev/null 2>&1 || true
    endscript
}
EOF

# generate ~3 MB
sudo bash -c 'for i in $(seq 1 30000); do echo "$(date -Is) request=$i status=200"; done > /var/log/labapp/app.log'
ls -lh /var/log/labapp/

# 4. debug and force rotation
sudo logrotate -d /etc/logrotate.d/labapp
sudo logrotate -f /etc/logrotate.d/labapp
ls -lh /var/log/labapp/

# 5. prove the app still writes to the old file if it does not reopen
# (use a writer that keeps the fd open; see L1.5)
```

### 5. Expected observations

- `journalctl -t labapp` shows the messages with timestamps and priority.
- `logrotate -d` prints what it would do without changing files.
- `-f` rotates to `app.log.1`, compresses older rotations after `delaycompress`.
- If a process keeps the old file open, `df` does not drop; `lsof +L1` shows the deleted inode.

### 6. What happens internally

journald receives messages over `/dev/log`/native protocol and stores them in indexed journal files. `logrotate` renames the current file, optionally creates a new one, and runs `postrotate`. Apps that cache the file descriptor keep writing to the old inode until they reopen it.

### 7. Troubleshooting task

Configure `copytruncate` instead of `create`, rotate under continuous writes, and look for lost lines. Then switch back to `create` + a reopen signal and compare. Explain the race.

### 8. Verify the fix

```bash
ls -lh /var/log/labapp/
sudo lsof +L1 | grep labapp || echo "no deleted-open labapp files"
journalctl -u labapp --since "10 min ago" --no-pager
```

### 9. Cleanup

```bash
sudo rm -f /etc/logrotate.d/labapp
sudo rm -rf /var/log/labapp
```

### 10. Write-up template

```markdown
# Lab L1.7 — Logs and rotation
Date:
Journal query examples:
Rotation config:
What `logrotate -d` showed:
Why disk space did/did not drop:
What I would alert on:
```

---

## Lab L1.8 — Packages and SSH

### 1. Learning objective

Install, query, pin, and remove a package on your distribution; set up key-based SSH to localhost; debug a key-permission failure; inspect `sshd` effective config.

### 2. Prerequisites

- L1.1, L1.2, L1.6.
- A VM with `sshd` installed (Ubuntu Server/Rocky minimal usually has it).

### 3. Environment and setup

```bash
# Debian/Ubuntu
sudo apt update
# RHEL/Rocky
sudo dnf check-update || true
```

### 4. Commands

```bash
# 1. package install and query
sudo apt install -y jq tree        # Debian/Ubuntu
# sudo dnf install -y jq tree      # RHEL/Rocky

dpkg -L jq | head                  # Debian
rpm -ql jq | head                  # RHEL
dpkg -S "$(command -v jq)"         # Debian
rpm -qf "$(command -v jq)"         # RHEL

# 2. pin/hold (choose one)
sudo apt-mark hold jq              # Debian
# sudo dnf versionlock add jq      # RHEL (plugin may be needed)
apt-cache policy jq                # Debian
dnf info jq                        # RHEL

# 3. history / rollback evidence
tail -20 /var/log/apt/history.log  # Debian
sudo dnf history | head            # RHEL

# 4. SSH key to localhost
ssh-keygen -t ed25519 -f ~/.ssh/lab_ed25519 -N '' -C lab
ssh-copy-id -i ~/.ssh/lab_ed25519.pub "$USER"@localhost
ssh -i ~/.ssh/lab_ed25519 "$USER"@localhost 'hostname; id'

# 5. config entry
cat >> ~/.ssh/config <<'EOF'

Host lab-local
    HostName localhost
    User YOUR_USER
    IdentityFile ~/.ssh/lab_ed25519
    IdentitiesOnly yes
EOF
ssh lab-local 'echo ok'

# 6. break: too-open private key
chmod 644 ~/.ssh/lab_ed25519
ssh -i ~/.ssh/lab_ed25519 "$USER"@localhost 'echo should-fail'
chmod 600 ~/.ssh/lab_ed25519

# 7. inspect sshd
sudo sshd -t
sudo sshd -T | grep -E 'permitrootlogin|passwordauthentication|pubkeyauthentication|authorizedkeysfile'
ss -tulpn | grep ':22'
```

### 5. Expected observations

- `jq --version` works; package ownership commands identify the package.
- `apt-mark hold` shows the package as held; `dnf versionlock` lists it.
- Key-based SSH works without a password; `ssh -vvv` shows the key offer and acceptance.
- `chmod 644` on the private key produces `UNPROTECTED PRIVATE KEY FILE` and SSH refuses it.
- `sshd -T` prints the effective config, including defaults.

### 6. What happens internally

The package manager resolves dependencies, downloads signed packages, unpacks them, and records metadata. SSH performs a host-key exchange, verifies the server, then authenticates the user with a signature over a session-specific challenge. The private key must be readable only by the user; `sshd` enforces permissions on `authorized_keys`.

### 7. Troubleshooting task

Create a second user with a valid key but a home directory mode `750` owned by root; attempt SSH. Diagnose with `ssh -vvv` and `/var/log/auth.log` (`/var/log/secure` on RHEL). Fix ownership/mode, then verify. Explain why the key was valid but authentication still failed.

### 8. Verify the fix

```bash
ssh -o BatchMode=yes -o ConnectTimeout=5 lab-local 'echo ssh-ok'
sudo sshd -T | grep -E 'permitrootlogin|passwordauthentication'
```

### 9. Cleanup

```bash
sudo apt-mark unhold jq 2>/dev/null || true
# sudo dnf versionlock delete jq 2>/dev/null || true
rm -f ~/.ssh/lab_ed25519 ~/.ssh/lab_ed25519.pub
# remove the lab-local block from ~/.ssh/config manually
```

### 10. Write-up template

```markdown
# Lab L1.8 — Packages and SSH
Date:
Package manager commands used:
Pin/hold evidence:
SSH key setup:
Why the 644 key failed:
How I would harden sshd:
```
