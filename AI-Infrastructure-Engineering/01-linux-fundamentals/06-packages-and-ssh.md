# Package Management and SSH

> **Two operational skills that show up in every interview:** can you change a system safely, and can you reach it securely? Package management is change control for binaries; SSH is your hands on the machine.

---

## Part 1 — Package management

### The mental model

```text
repository metadata (what is available)
        ↓
dependency solver (what must be installed together)
        ↓
package files (.deb / .rpm) + GPG signature
        ↓
local database (dpkg / rpm) records what is installed
```

A package manager is a **state reconciler**: you ask for a target state (`nginx installed`, `version 1.24`), and it computes the changes. It is not a downloader.

| Family | Low-level | High-level | Database | Repo config |
|---|---|---|---|---|
| Debian/Ubuntu | `dpkg` | `apt` | `/var/lib/dpkg/status` | `/etc/apt/sources.list`, `/etc/apt/sources.list.d/` |
| RHEL/Rocky/Alma | `rpm` | `dnf` (or `yum`) | `/var/lib/rpm/` | `/etc/yum.repos.d/*.repo` |

`yum` on RHEL 8+ is a compatibility wrapper around `dnf`. Learn `dnf`; recognise `yum` in older docs.

---

### Debian/Ubuntu: `apt` and `dpkg`

```bash
sudo apt update                     # refresh metadata
apt search nginx
apt show nginx
sudo apt install nginx
sudo apt install nginx=1.24.0-1ubuntu1
sudo apt remove nginx               # remove package, keep config
sudo apt purge nginx                # remove package + config
sudo apt autoremove                 # remove unused dependencies
sudo apt upgrade                    # upgrade installed packages
sudo apt full-upgrade               # allow removals if needed
```

Query:

```bash
dpkg -l | grep nginx
dpkg -L nginx                       # files installed by the package
dpkg -S /usr/sbin/nginx             # which package owns this file
apt-cache policy nginx              # installed vs candidate vs versions
apt-mark hold nginx                 # prevent upgrades
apt-mark unhold nginx
```

Install a local `.deb`:

```bash
sudo apt install ./package.deb
# or
sudo dpkg -i package.deb && sudo apt -f install
```

`dpkg -i` does not resolve dependencies; `apt -f install` fixes the missing ones. Prefer `apt install ./file.deb`.

If a previous install was interrupted:

```bash
sudo dpkg --configure -a
sudo apt -f install
```

---

### RHEL/Rocky: `dnf` and `rpm`

```bash
sudo dnf check-update
dnf info nginx
sudo dnf install nginx
sudo dnf install nginx-1.24.0
sudo dnf remove nginx
sudo dnf upgrade
sudo dnf distro-sync
```

Query:

```bash
rpm -qa | grep nginx
rpm -qi nginx
rpm -ql nginx                       # files
rpm -qf /usr/sbin/nginx             # owner
rpm -V nginx                        # verify installed files against metadata
dnf provides /usr/sbin/nginx
dnf list installed | grep nginx
```

History and rollback:

```bash
sudo dnf history
sudo dnf history info 42
sudo dnf history undo 42
sudo dnf history rollback 40
```

`dnf history undo` reverses a transaction; it is not a filesystem snapshot. If the transaction changed data or ran scriptlets with side effects, rollback may be incomplete.

Version lock:

```bash
sudo dnf install 'dnf-command(versionlock)'
sudo dnf versionlock add nginx
sudo dnf versionlock list
sudo dnf versionlock delete nginx
```

Repositories:

```bash
dnf repolist
dnf repoinfo epel
cat /etc/yum.repos.d/*.repo
```

A `.repo` file:

```ini
[internal]
name=Internal Packages
baseurl=https://repo.example.com/rhel/9/x86_64/
enabled=1
gpgcheck=1
gpgkey=https://repo.example.com/keys/RPM-GPG-KEY-internal
```

---

### GPG, metadata, and the errors you will see

| Error | Meaning | Fix |
|---|---|---|
| `404 Not Found` | Repo URL/path wrong or package removed | Check `baseurl`, release version, architecture |
| `GPG error: ... NO_PUBKEY` | Missing/rotated signing key | Import the correct key from the repo owner |
| `Release file expired` | Metadata stale or clock wrong | `apt update`; check `chronyd`/NTP; repo issue |
| `Could not resolve host` | DNS/network/proxy | `getent hosts`, `curl -I`, proxy config |
| `dpkg was interrupted` | Unfinished transaction | `dpkg --configure -a` |
| `file ... conflicts with ...` | Package conflict | Remove/replace the conflicting package; understand why |
| `nothing provides ...` (dnf) | Missing dependency/repo | Enable the right repo/module; do not force |
| `Permission denied` on lock | Another package process running | Wait; check `ps` for `apt`/`dnf`; do not delete the lock casually |

**Do not use `--force` / `--nodeps` to "fix" a dependency problem** unless you can explain the consequence. It produces a system that looks installed but is broken.

### Pinning and rollback

```bash
# Debian: hold a version
sudo apt-mark hold nginx
apt-cache policy nginx

# Debian: install a specific version
sudo apt install nginx=1.24.0-1ubuntu1

# RHEL: versionlock
sudo dnf versionlock add nginx

# RHEL: undo a transaction
sudo dnf history undo <id>
```

Before a risky upgrade: snapshot the VM, record `dpkg -l`/`rpm -qa`, and know the rollback path. A package manager is not a backup system.

---

## Part 2 — SSH

### How SSH works (simplified)

```text
client                                              server
  |-- TCP connect to :22 ----------------------------->|
  |<-- server host key, key exchange ------------------|
  |-- verify host key in known_hosts ------------------|
  |-- authenticate (public key / password / cert) ----->|
  |<-- session: shell, command, or forwarded channel --|
```

Two different keys:

| Key | Purpose | Where |
|---|---|---|
| **Host key** | Proves the server's identity | `/etc/ssh/ssh_host_*`, client `known_hosts` |
| **User key** | Proves your identity | `~/.ssh/id_ed25519`, server `~/.ssh/authorized_keys` |

Do not confuse "host key verification failed" with "permission denied (publickey)".

### Key setup

```bash
ssh-keygen -t ed25519 -a 100 -C "ritik@laptop"
ssh-copy-id -i ~/.ssh/id_ed25519.pub user@server
ssh user@server
```

Permissions:

```bash
chmod 700 ~/.ssh
chmod 600 ~/.ssh/id_ed25519
chmod 644 ~/.ssh/id_ed25519.pub
chmod 600 ~/.ssh/authorized_keys
chmod 700 ~/.ssh
```

The server's `sshd` refuses a private key or `authorized_keys` that are group/world-writable. This is intentional.

### `~/.ssh/config`

```sshconfig
Host web01
    HostName 10.0.1.10
    User deploy
    IdentityFile ~/.ssh/id_ed25519
    IdentitiesOnly yes
    ServerAliveInterval 30
    ServerAliveCountMax 3

Host bastion
    HostName bastion.example.com
    User ops
    IdentityFile ~/.ssh/id_ed25519

Host private-*
    ProxyJump bastion
    User deploy

Host *
    AddKeysToAgent yes
    HashKnownHosts yes
```

Then:

```bash
ssh web01
ssh private-db
```

`ProxyJump` is the modern replacement for `ProxyCommand nc ...`. It connects through a bastion without exposing the private host.

### Copying files

```bash
scp file user@host:/path/
scp -r dir user@host:/path/
rsync -av --progress dir/ user@host:/path/dir/
rsync -av --delete dir/ user@host:/path/dir/   # destructive: deletes extra files on target
```

`rsync` is resumable, efficient, and preserves metadata. `scp` is simpler but less capable. Use `-n`/`--dry-run` with `--delete` first.

### Port forwarding

```bash
# local: access remote service on localhost:8080
ssh -L 8080:localhost:80 user@host

# remote: expose local service on the remote host
ssh -R 9000:localhost:3000 user@host

# dynamic: SOCKS proxy
ssh -D 1080 user@host
```

Local forwarding is useful for reaching a database or dashboard that listens only on the remote loopback. It does not change the remote service's bind address.

### Hardening `sshd`

`/etc/ssh/sshd_config.d/99-hardening.conf`:

```ini
PermitRootLogin no
PasswordAuthentication no
PubkeyAuthentication yes
PermitEmptyPasswords no
X11Forwarding no
AllowAgentForwarding no
MaxAuthTries 4
ClientAliveInterval 300
ClientAliveCountMax 2
```

Test before reloading:

```bash
sudo sshd -t
sudo sshd -T | grep -E 'permitrootlogin|passwordauthentication|pubkeyauthentication'
sudo systemctl reload sshd
```

Keep an existing session open while testing a new one. If you lock yourself out, you need console access.

### Troubleshooting SSH

```bash
ssh -vvv user@host
ssh -o BatchMode=yes -o ConnectTimeout=5 user@host
```

| Symptom | Likely cause | Check |
|---|---|---|
| `Connection refused` | sshd not listening / firewall reject | `ss -tulpn \| grep :22`, `systemctl status sshd` |
| `Connection timed out` | Firewall drop / wrong route / host down | `ip route`, `ping`, `tcpdump`, security group |
| `Permission denied (publickey)` | Wrong key, permissions, user, `AuthorizedKeysFile` | `ssh -vvv`, `ls -ld ~/.ssh`, `auth.log` |
| `Host key verification failed` | Server rebuilt or MITM | Verify fingerprint out-of-band; remove old entry only if sure |
| `Too many authentication failures` | Agent offers too many keys | `IdentitiesOnly yes`, `-i key` |
| `Broken pipe` | Network/NAT idle timeout | `ServerAliveInterval` |
| `Agent forwarding` risk | Remote root can use your agent | Avoid forwarding to untrusted hosts |

Host key change:

```bash
ssh-keygen -F host
ssh-keygen -R host        # remove only after verifying the new fingerprint
```

On the server:

```bash
sudo journalctl -u sshd --since "10 min ago"
sudo tail -f /var/log/auth.log      # Debian/Ubuntu
sudo tail -f /var/log/secure        # RHEL
```

---

## Worked scenarios

### Scenario A — `apt update` fails with a 404

```bash
$ sudo apt update
Err:1 https://repo.example.com/ubuntu noble InRelease
  404  Not Found [IP: 203.0.113.10 443]
```

Check the distribution codename and architecture:

```bash
. /etc/os-release; echo "$ID $VERSION_CODENAME"
dpkg --print-architecture
curl -I https://repo.example.com/ubuntu/dists/noble/InRelease
```

The repo may not carry `noble` yet, or the URL path is wrong. Fix the source; do not add `[trusted=yes]` to bypass errors.

### Scenario B — GPG error after a key rotation

```bash
W: GPG error: https://repo.example.com stable InRelease: NO_PUBKEY 1234ABCD...
```

Import the repo's published key over a trusted channel, verify the fingerprint, then `apt update`. Do not pipe an untrusted URL directly into `apt-key`/`gpg`.

### Scenario C — a package upgrade broke a service

```bash
$ systemctl status nginx
● nginx.service - A high performance web server
     Active: failed (Result: exit-code)
$ journalctl -u nginx -n 20
nginx[2210]: nginx: [emerg] unknown directive "foo" in /etc/nginx/nginx.conf:12
```

The new binary rejects an old/unknown config directive. Options: restore the previous config, fix the config, or roll back the package. Record the transaction (`dnf history`, `/var/log/apt/history.log`) before changing more.

### Scenario D — SSH works for one user, not another

```bash
$ ssh deploy@host
deploy@host: Permission denied (publickey).
$ ssh admin@host
# works
```

On the server:

```bash
sudo tail -50 /var/log/auth.log
sudo -u deploy ls -ld /home/deploy /home/deploy/.ssh
sudo -u deploy cat /home/deploy/.ssh/authorized_keys
sudo sshd -T | grep -E 'authorizedkeysfile|allowusers|allowgroups|denyusers'
```

Common causes: home directory `750` owned by root, `.ssh` `775`, wrong key, `AllowUsers` excludes the user, or SELinux context on `authorized_keys` (`restorecon -Rv ~/.ssh`).

### Scenario E — host key changed after a rebuild

```bash
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@    WARNING: REMOTE HOST IDENTIFICATION HAS CHANGED!     @
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
```

Verify the new fingerprint through the cloud console or a known-good channel. If it matches, remove the old entry:

```bash
ssh-keygen -R 10.0.1.10
ssh-keyscan -t ed25519 10.0.1.10
```

If it does not match, stop. This is the one warning you do not click through.

---

## Common mistakes

| Mistake | Why it is wrong | Better |
|---|---|---|
| `apt upgrade` without reading what will change | Can restart services or remove packages | `apt -s upgrade`, review, snapshot |
| `dnf remove` a dependency blindly | Can remove the desktop/tooling | Read the transaction; use `--noautoremove` if needed |
| `--force`/`--nodeps` | Broken dependency graph | Fix the repo/dependency |
| Deleting `/var/lib/dpkg/lock*` while apt runs | Corrupts the database | Wait or kill the actual apt process |
| `PermitRootLogin yes` + password auth | Brute-force target | Keys only, no root login |
| `chmod 777 ~/.ssh` | sshd refuses; security hole | `700` dir, `600` keys |
| Disabling host key checking globally | MITM risk | Verify fingerprints; use `known_hosts` |
| Agent forwarding to untrusted hosts | Remote root can use your keys | Avoid; use `ProxyJump` |
| `rsync --delete` without dry run | Deletes target files | `-n` first |

---

## Command reference (quick)

```bash
# Debian
apt update
apt install pkg
apt install pkg=version
apt remove|purge pkg
apt autoremove
apt-cache policy pkg
apt-mark hold|unhold pkg
dpkg -l | grep pkg
dpkg -L pkg
dpkg -S /path
dpkg --configure -a

# RHEL
dnf check-update
dnf install pkg
dnf remove pkg
dnf history
dnf history undo ID
dnf versionlock add pkg
rpm -qa | grep pkg
rpm -ql pkg
rpm -qf /path
rpm -V pkg
dnf provides /path

# SSH
ssh-keygen -t ed25519 -a 100
ssh-copy-id user@host
ssh -vvv user@host
ssh -L 8080:localhost:80 user@host
ssh -J bastion user@host
scp file user@host:/path/
rsync -av --progress dir/ user@host:/path/
sshd -t
sshd -T
ssh-keygen -F host
ssh-keygen -R host
```

---

## Checkpoint

1. What is the difference between `apt` and `dpkg`, and between `dnf` and `rpm`?
2. What does `apt update` change, and what does it not change?
3. How do you install a specific package version and prevent upgrades?
4. How do you roll back a `dnf` transaction, and what are its limits?
5. What does a GPG `NO_PUBKEY` error mean? How do you fix it safely?
6. Why does `dpkg -i` sometimes leave a package unconfigured?
7. What is the difference between a host key and a user key?
8. Why does SSH refuse a private key with mode `0644`?
9. What is the difference between `ssh -L` and `ssh -R`?
10. How do you debug `Permission denied (publickey)` on the server side?

If you miss more than two, redo Lab L1.8.
