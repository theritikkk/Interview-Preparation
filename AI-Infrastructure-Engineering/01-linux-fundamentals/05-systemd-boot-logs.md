# systemd, Boot, Logs, and Log Rotation

> **The one-sentence model:** the kernel starts PID 1 (`systemd`), which brings the system to a target by starting units in dependency order; every service is a unit with a process, a cgroup, and a journal stream.

---

## 1. The boot chain

```text
power on
  ↓
firmware (BIOS/UEFI)
  ↓
bootloader (GRUB / systemd-boot) reads /boot
  ↓
kernel + initramfs
  ↓
kernel mounts the real root filesystem, starts /sbin/init (systemd, PID 1)
  ↓
systemd: sysinit.target → basic.target → multi-user.target (or graphical.target)
  ↓
getty / sshd / your services
```

Useful evidence:

```bash
$ systemd-analyze
Startup finished in 2.114s (firmware) + 1.020s (loader) + 3.442s (kernel) + 8.112s (userspace) = 14.689s

$ systemd-analyze blame | head
8.112s cloud-init.service
1.204s NetworkManager-wait-online.service
0.882s app.service

$ systemd-analyze critical-chain
multi-user.target @8.112s
└─app.service @7.220s +882ms
  └─network-online.target @7.210s
    └─NetworkManager-wait-online.service @6.000s +1.204s

$ systemctl get-default
multi-user.target
```

Previous boot (after a crash/reboot):

```bash
journalctl -b -1 -p err
journalctl --list-boots
```

Rescue modes:

```bash
systemctl rescue          # single-user, most services stopped
systemctl emergency       # minimal, root fs read-only
```

---

## 2. Units and targets

| Unit type | Purpose | Example |
|---|---|---|
| `.service` | A process/daemon | `nginx.service` |
| `.socket` | Socket activation | `docker.socket` |
| `.timer` | Scheduled activation | `logrotate.timer` |
| `.target` | Grouping/synchronisation point | `multi-user.target` |
| `.mount` | Mount point | `data.mount` |
| `.automount` | On-demand mount | `home.automount` |
| `.path` | Watch a path | `cups.path` |
| `.slice` | cgroup hierarchy | `system.slice` |
| `.device` | Kernel device | `dev-sda.device` |
| `.swap` | Swap file/device | `swap.swap` |

Targets replace SysV runlevels:

| Target | Rough runlevel | Use |
|---|---|---|
| `poweroff.target` | 0 | Shutdown |
| `rescue.target` | 1 | Single-user |
| `multi-user.target` | 3 | Normal server |
| `graphical.target` | 5 | Desktop |
| `reboot.target` | 6 | Reboot |

```bash
systemctl list-units --type=service --state=running
systemctl list-units --type=target
systemctl list-dependencies multi-user.target
```

---

## 3. Anatomy of a service unit

```ini
[Unit]
Description=App API
Documentation=https://internal.example/app
After=network-online.target
Wants=network-online.target
Requires=postgresql.service

[Service]
Type=simple
User=app
Group=app
WorkingDirectory=/opt/app
EnvironmentFile=-/etc/app/env
ExecStart=/opt/app/server --port 8080
ExecReload=/bin/kill -HUP $MAINPID
Restart=on-failure
RestartSec=5
TimeoutStopSec=20
LimitNOFILE=65535
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ReadWritePaths=/var/lib/app
UMask=0027

[Install]
WantedBy=multi-user.target
```

### `[Unit]` dependencies

| Directive | Meaning |
|---|---|
| `After=X` | Start after X, **if both are started**; does not pull X in |
| `Before=X` | Start before X |
| `Wants=X` | Weak dependency: try to start X; failure does not stop this unit |
| `Requires=X` | Strong dependency: if X fails, this unit fails/stops |
| `BindsTo=X` | Stronger: if X disappears, this unit stops |
| `PartOf=X` | Restart/stop propagates from X |
| `Conflicts=X` | Cannot run at the same time |

A common mistake: `After=network.target` does **not** guarantee the network is usable. For a service that needs an address, use `network-online.target` and `Wants=network-online.target` (and ensure the network manager waits).

### `[Service]` types

| Type | Meaning | When |
|---|---|---|
| `simple` | `ExecStart` forks and stays in foreground | Most modern apps |
| `exec` | Like simple, but waits for `execve` to succeed | Stricter startup detection |
| `forking` | Process forks and parent exits; use `PIDFile=` | Legacy daemons |
| `oneshot` | Runs to completion, then inactive | Scripts, setup |
| `notify` | Service sends `READY=1` via `sd_notify` | Apps with readiness support |
| `idle` | Delayed until jobs finish | Rare |

`Type=simple` with an app that daemonises will look like it exited immediately. Match the type to the program.

### Restart and limits

```ini
Restart=on-failure
RestartSec=5
StartLimitIntervalSec=60
StartLimitBurst=5
```

A fast crash loop will hit the start limit and enter `failed`. Read the journal, fix the cause, then `systemctl reset-failed app`.

### Hardening (use incrementally)

| Directive | Effect |
|---|---|
| `NoNewPrivileges=true` | Cannot gain privileges via SUID |
| `PrivateTmp=true` | Private `/tmp` and `/var/tmp` |
| `ProtectSystem=strict` | Most of `/usr`, `/boot`, `/etc` read-only |
| `ProtectHome=true` | `/home`, `/root`, `/run/user` inaccessible |
| `ReadWritePaths=/var/lib/app` | Explicit writable paths |
| `PrivateDevices=true` | No raw device access |
| `RestrictAddressFamilies=AF_INET AF_INET6` | Limit socket families |
| `CapabilityBoundingSet=` | Drop capabilities |
| `SystemCallFilter=@system-service` | Seccomp allowlist |

Test hardening in staging; an over-restrictive unit fails in confusing ways. Check `systemd-analyze security app.service`.

---

## 4. Managing services

```bash
systemctl start app
systemctl stop app
systemctl restart app
systemctl reload app          # re-read config without restart, if supported
systemctl try-restart app     # only if running
systemctl status app
systemctl is-active app
systemctl is-enabled app
systemctl enable --now app
systemctl disable app
systemctl mask app            # prevent any start
systemctl unmask app
systemctl reset-failed app
systemctl cat app             # show unit + drop-ins
systemctl show app -p Restart -p MainPID -p MemoryCurrent
systemctl list-dependencies app
```

`reload` is safer than `restart` when supported: it keeps connections and state. Verify the app actually implements reload (`ExecReload=`); otherwise `reload` is a no-op or fails.

### Drop-in overrides

Do not edit vendor units in `/usr/lib/systemd/system`; they are overwritten by package updates. Use a drop-in:

```bash
sudo systemctl edit app
```

```ini
[Service]
Environment=LOG_LEVEL=debug
LimitNOFILE=65535
```

This creates `/etc/systemd/system/app.service.d/override.conf`. Then:

```bash
sudo systemctl daemon-reload
sudo systemctl restart app
```

`systemctl cat app` shows the merged result, including drop-ins.

### Example status output

```bash
$ systemctl status app
● app.service - App API
     Loaded: loaded (/etc/systemd/system/app.service; enabled; preset: enabled)
     Drop-In: /etc/systemd/system/app.service.d
              └─override.conf
     Active: active (running) since Sun 2026-03-15 10:05:12 UTC; 2h 14min ago
   Main PID: 1042 (node)
      Tasks: 11 (limit: 9444)
     Memory: 492.0M
        CPU: 14.2s
     CGroup: /system.slice/app.service
             ├─1042 node /opt/app/server.js
             └─1180 node /opt/app/worker.js
```

Read it in order: unit path, drop-ins, active state, main PID, resource use, cgroup tree. The cgroup tree tells you which child processes belong to the service — important for cleanup.

---

## 5. Timers

A timer activates a service. It replaces cron for system-managed jobs and gives you journal integration, dependencies, and `Persistent=` catch-up.

`/etc/systemd/system/backup.service`:

```ini
[Unit]
Description=Nightly backup

[Service]
Type=oneshot
User=backup
ExecStart=/opt/backup/run.sh
```

`/etc/systemd/system/backup.timer`:

```ini
[Unit]
Description=Run backup nightly

[Timer]
OnCalendar=*-*-* 02:30:00
Persistent=true
RandomizedDelaySec=300
Unit=backup.service

[Install]
WantedBy=timers.target
```

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now backup.timer
systemctl list-timers --all
systemctl status backup.timer
journalctl -u backup.service --since yesterday
```

| Directive | Meaning |
|---|---|
| `OnCalendar=` | Wall-clock schedule (`systemd-analyze calendar '*-*-* 02:30:00'`) |
| `OnBootSec=` | Relative to boot |
| `OnUnitActiveSec=` | Relative to last activation |
| `Persistent=true` | Run missed jobs after downtime |
| `RandomizedDelaySec=` | Spread load |
| `AccuracySec=` | Coalescing window |

A common trap: enabling the **service** instead of the **timer**, or forgetting `daemon-reload`. Check `list-timers` for `NEXT` and `LAST`.

---

## 6. journald and `journalctl`

`systemd-journald` collects kernel, userspace, and service logs into a structured, indexed journal.

```bash
journalctl                         # all
journalctl -u app                  # one unit
journalctl -u app -f               # follow
journalctl -u app --since "10 min ago"
journalctl -u app --since "2026-03-15 10:00" --until "2026-03-15 10:30"
journalctl -p err -b               # errors this boot
journalctl -k                      # kernel
journalctl -b -1                   # previous boot
journalctl _PID=1042               # by PID
journalctl _SYSTEMD_UNIT=app.service
journalctl -o json-pretty          # structured
journalctl --disk-usage
journalctl --vacuum-time=7d
```

Useful output:

```bash
$ journalctl -u app --since "10 min ago" -o short-iso
2026-03-15T10:22:01+0000 web01 app[1042]: listening on 0.0.0.0:8080
2026-03-15T10:22:03+0000 web01 app[1042]: connected to postgres
2026-03-15T10:23:11+0000 web01 app[1042]: ERROR upstream timeout after 3000ms
```

### Persistent vs volatile journal

By default, some distributions keep the journal only in `/run/log/journal` (volatile, lost on reboot). Persistent journal lives in `/var/log/journal`.

```bash
$ ls -ld /var/log/journal
drwxr-sr-x+ 3 root systemd-journal 4096 ... /var/log/journal

# /etc/systemd/journald.conf
[Journal]
Storage=persistent
SystemMaxUse=2G
SystemKeepFree=1G
MaxRetentionSec=30day
```

### Rate limiting

journald rate-limits messages to protect the system:

```ini
[Journal]
RateLimitIntervalSec=30s
RateLimitBurst=10000
```

If a service logs a burst and the journal says `Suppressed N messages due to rate-limiting`, raise the burst temporarily or fix the log spam. Do not disable rate limiting permanently on a busy host.

### rsyslog and `/var/log`

Many systems still run `rsyslog` alongside journald:

| Path | Contents |
|---|---|
| `/var/log/syslog` | General messages (Debian/Ubuntu) |
| `/var/log/auth.log` | Authentication/sudo/SSH (Debian/Ubuntu) |
| `/var/log/messages` | General messages (RHEL) |
| `/var/log/secure` | Authentication (RHEL) |
| `/var/log/kern.log` | Kernel |
| `/var/log/nginx/` | Web server |
| `/var/log/audit/audit.log` | auditd |

Use `journalctl` first for systemd units; use the files when the app writes there directly or when you need a specific parser.

---

## 7. logrotate

`logrotate` renames, compresses, deletes, and recreates log files on a schedule.

```text
/var/log/app/*.log {
    daily
    rotate 14
    compress
    delaycompress
    missingok
    notifempty
    create 0640 app adm
    sharedscripts
    postrotate
        systemctl reload app >/dev/null 2>&1 || true
    endscript
}
```

| Directive | Meaning |
|---|---|
| `daily`/`weekly`/`monthly` | Rotation interval |
| `rotate N` | Keep N old files |
| `compress` | gzip old files |
| `delaycompress` | Compress the previous, not the just-rotated |
| `missingok` | No error if the file is absent |
| `notifempty` | Do not rotate empty files |
| `create mode owner group` | Recreate the file with these attributes |
| `copytruncate` | Copy then truncate; for apps that do not reopen |
| `postrotate` | Run after rotation (signal the app to reopen) |

```bash
logrotate -d /etc/logrotate.conf      # debug, no changes
logrotate -f /etc/logrotate.d/app     # force
cat /var/lib/logrotate/status
```

**`copytruncate` risk:** messages written between the copy and the truncate can be lost. Prefer `create` + a reload signal if the app supports it.

A common incident: an app holds the old file open after rotation, so disk usage does not drop. Check `lsof | grep deleted` and make the app reopen its log.

---

## 8. Worked scenarios

### Scenario A — service fails to start after a config change

```bash
$ systemctl status app
● app.service - App API
     Active: failed (Result: exit-code) since ...
    Process: 2210 ExecStart=/opt/app/server (code=exited, status=1/FAILURE)

$ journalctl -xeu app --no-pager | tail -30
app[2210]: Error: listen EADDRINUSE: address already in use 0.0.0.0:8080
```

Check the port owner:

```bash
ss -tulpn | grep 8080
```

If a stale process owns it, stop that process or fix the unit's `ExecStop`/`KillMode`. Do not simply restart until the conflict is understood.

### Scenario B — works manually, fails under systemd

```bash
$ sudo -u app /opt/app/server
# works
$ systemctl start app
$ journalctl -u app -n 20
app[2311]: Error: Cannot find module 'dotenv'
```

Differences to check:

| Manual shell | systemd |
|---|---|
| Your `PATH` | `Environment=PATH=...` or absolute paths |
| Your cwd | `WorkingDirectory=` |
| Your exported vars | `Environment=` / `EnvironmentFile=` |
| Your shell functions/aliases | systemd does not run a shell |
| Your user | `User=` / `Group=` |
| Your umask | `UMask=` |
| Your limits | `LimitNOFILE=` |

Run the exact command as the service user from the service working directory:

```bash
sudo -u app env -i PATH=/usr/bin:/bin HOME=/var/lib/app /opt/app/server
```

### Scenario C — timer did not run

```bash
$ systemctl list-timers --all
NEXT                        LEFT     LAST                        PASSED  UNIT
n/a                         n/a      n/a                         n/a     backup.timer

$ systemctl status backup.timer
● backup.timer - Run backup nightly
     Active: inactive (dead)
```

The timer is not enabled. `systemctl enable --now backup.timer`. If it is enabled but `LAST` is old, check `OnCalendar`, `Persistent=`, and whether the service itself failed (`journalctl -u backup.service`).

### Scenario D — logs are missing

Possible causes:

1. Journal is volatile and the host rebooted (`/var/log/journal` missing).
2. Rate limiting suppressed the messages.
3. The app logs to a file, not stdout/stderr, and the file rotated.
4. The unit name is wrong (`journalctl -u app` vs `app.service`).
5. The process runs in a container with its own logging driver.
6. Permissions: the user cannot read the journal (`adm`/`systemd-journal` group).

```bash
journalctl --disk-usage
journalctl -u app --no-pager | tail
journalctl -k | grep -i 'rate limit\|suppressed'
ls -l /var/log/app/
```

### Scenario E — journal fills the disk

```bash
$ df -h /var
Filesystem      Size  Used Avail Use% Mounted on
/dev/sda1        40G   40G     0 100% /var

$ journalctl --disk-usage
Archived and active journals take up 3.2G in the file system.
```

Immediate:

```bash
sudo journalctl --vacuum-size=500M
```

Prevent:

```ini
# /etc/systemd/journald.conf
[Journal]
SystemMaxUse=2G
SystemKeepFree=1G
MaxRetentionSec=30day
```

Then `systemctl restart systemd-journald`. Also check for a log spam loop; vacuuming without fixing the producer is temporary.

---

## 9. Common mistakes

| Mistake | Why it is wrong | Better |
|---|---|---|
| Editing `/usr/lib/systemd/system/*.service` | Overwritten by package updates | `systemctl edit` drop-in |
| Forgetting `daemon-reload` | Changes are not applied | `systemctl daemon-reload` after unit edits |
| `ExecStart` with shell syntax | systemd does not run a shell | Use a wrapper script or `bash -c` deliberately |
| `After=network.target` for a network-dependent app | Does not wait for an address | `network-online.target` + `Wants=` |
| `Type=simple` for a forking daemon | systemd thinks it exited | `Type=forking` + `PIDFile=` |
| `Restart=always` on a fast crash loop | Endless restarts, noise | `on-failure`, `StartLimitBurst`, fix the cause |
| Enabling the service, not the timer | Timer never schedules | `systemctl enable --now foo.timer` |
| `copytruncate` for a high-volume log | Loses lines; race | `create` + reopen signal |
| Vacuuming the journal without fixing spam | Disk fills again | Fix the producer; set a cap |
| Ignoring `journalctl -b -1` | The previous boot has the crash evidence | Read the previous boot |

---

## 10. Command reference (quick)

```bash
# services
systemctl status app
systemctl start|stop|restart|reload app
systemctl enable --now app
systemctl disable app
systemctl cat app
systemctl edit app
systemctl show app -p MainPID -p MemoryCurrent -p Restart
systemctl list-units --failed
systemctl reset-failed app

# boot
systemd-analyze
systemd-analyze blame
systemd-analyze critical-chain
systemctl get-default
systemctl isolate rescue.target

# timers
systemctl list-timers --all
systemd-analyze calendar '*-*-* 02:30:00'
systemctl status backup.timer
journalctl -u backup.service --since yesterday

# logs
journalctl -u app -f
journalctl -u app --since "10 min ago" -o short-iso
journalctl -p err -b
journalctl -k
journalctl -b -1
journalctl --disk-usage
journalctl --vacuum-size=500M

# logrotate
logrotate -d /etc/logrotate.conf
logrotate -f /etc/logrotate.d/app
```

---

## 11. Checkpoint

1. What is the difference between `After=` and `Requires=`?
2. Why does `After=network.target` not guarantee a working network?
3. What does `Type=simple` assume about the process?
4. How do you override one directive without editing the vendor unit?
5. Why does a service work manually but fail under systemd? Name five differences.
6. How do you find why a timer did not run?
7. Where is the persistent journal, and how do you cap its size?
8. What does `journalctl -b -1` show, and when is it useful?
9. Why can log rotation fail to free disk space?
10. What is the safe sequence after editing a unit file?

If you miss more than two, redo Labs L1.6 and L1.7.
