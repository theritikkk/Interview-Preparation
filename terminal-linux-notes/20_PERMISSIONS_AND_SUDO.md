# 20 — Permissions and sudo

> **Where this fits in the stack**
> ```
> User  →  ...  →  Syscalls  →  VFS/Filesystem  →  [ PERMISSIONS ]  →  Kernel enforcement
>                                                    ^^^^^^^^^^^^^^^
>                                                    this file
> ```
> File 19 established that every file has an inode carrying its metadata. This file covers one specific piece of that metadata in depth — the permission bits — and follows the chain all the way to how `sudo` actually gets a process running with root privileges in the first place.

---

## 1. `rwx`, owner/group/other

Every inode (file 19) carries a permission bitmask: three sets of three bits — read, write, execute — one set each for the **owner**, the **group**, and **other** (everyone else).

**On a regular file:** `r` = read its content, `w` = modify its content, `x` = execute it as a program or script.

**On a directory** (recap file 19 — a directory is itself a file): `r` = list its entries (`getdents` its content), `w` = add or remove entries within it (create/delete/rename things *inside* it — distinct from being able to modify the *contents* of files inside it, which depends on those files' own permissions, not the directory's).

> **Remember**
> `x` on a **directory** means something different from `x` on a file: it means "permission to traverse into this directory as a path component." You need execute permission on **every** directory in a path to reach anything inside it — even to `stat` a single deeply-nested file — regardless of whether you have *read* permission to list that directory's own contents. This is genuinely the most commonly confused permission bit in the whole set.

```
-rwxr-xr--
```
Leading character: `-` (regular file), `d` (directory), `l` (symlink). Then three groups of `rwx`: owner, group, other.

---

## 2. `chmod`, `chown`, `umask`

```bash
chmod u+x file        # symbolic: add execute for owner
chmod 755 file          # octal: rwx r-x r-x  (7=rwx, 5=r-x, 4=r--)
chown alice file        # change owner
chown alice:staff file  # change owner AND group
```

> **Common mistake**
> "Any user can give away their own files by changing the owner." On most systems, **only root** can change a file's owner to a different user — an ordinary user reassigning their own files to someone else is disallowed by default, largely for disk-quota and accounting integrity.

**`umask`** is not a permission itself — it's a **mask applied at creation time**. New files default to `666` (rw-rw-rw-) and new directories to `777` (rwxrwxrwx) *before* the mask is applied; the umask's bits are then subtracted (bitwise AND with its complement):

```
umask 022
new file:      666 & ~022 = 644   (rw-r--r--)
new directory: 777 & ~022 = 755   (rwxr-xr-x)
```

`022` clears the write bit for group and other on everything you create — the extremely common default.

---

## 3. How the kernel actually enforces this

Every relevant syscall — `open`, `unlink`, `chdir`, `execve`, and others — passes through a permission check inside the kernel's VFS/filesystem code before proceeding at all. The kernel compares the calling process's credentials (its UID, GID, and supplementary groups — all part of its `task_struct`, file 08) against the target inode's owner/group/mode bits, and either allows the operation or fails it with `EACCES`/`EPERM`.

**Precisely, and in this exact order:**
1. Is the caller's UID the same as the file's owner UID? → apply the **owner** bits.
2. Otherwise, does the caller's GID (or a supplementary group) match the file's group? → apply the **group** bits.
3. Otherwise → apply the **other** bits.

> **Common mistake**
> "Permission checking is cumulative — my broadest applicable permission wins." It is **not**. Only the *first matching category* is ever consulted. If you're the file's owner, and the owner bits happen to deny something the group or other bits would have permitted, you are still denied — the kernel never falls through to check the more permissive categories once it's established which one applies to you.

> **Under the hood — a different kind of enforcement from file 09**
> This is worth contrasting directly with the user-mode/kernel-mode boundary from file 09. That boundary is **hardware-enforced** — the CPU itself refuses privileged instructions outside kernel mode. Permission checking is **software-enforced** — ordinary kernel code comparing numbers (UID/GID against an inode's stored owner/group/mode) and choosing to return an error. Both are "security," through completely different mechanisms.

---

## 4. `setuid`, conceptually

A special permission bit — separate from ordinary `rwx`, shown as an `s` in place of the owner's `x` in `ls -l` output — that, when set on an **executable** file, makes any process that `exec`s it run with the **effective UID of the file's owner**, rather than the UID of whoever actually launched it.

> **Remember — real UID vs effective UID**
> **Real UID** = who actually launched the process (who you logged in as). **Effective UID** = whose permissions are being used for permission-checking *right now*. Normally identical — a `setuid` executable is precisely what makes them diverge, for as long as that process runs or until it deliberately drops the elevated privilege. This divergence is the entire mechanism that makes `setuid` work at all.

**This is exactly how `sudo` (and `passwd`, and a handful of other specific utilities) let an ordinary, non-root user perform actions requiring root privileges:** the `sudo` binary itself is owned by root and carries the `setuid` bit, so the moment *any* user executes it, the resulting process's effective UID is immediately root — regardless of who started it. Well-written `setuid` programs carefully drop that elevated privilege as soon as they've done the specific thing that needed it, minimizing the window a bug could exploit.

---

## 5. `sudo` at a systems level

`sudo somecommand` unfolds as:

1. Your shell `fork`s and `execve`s `/usr/bin/sudo` — which is `setuid root`, so the moment this new process's image loads, its **effective UID is already root**, before a single line of `sudo`'s own logic has run.
2. `sudo`'s code checks `/etc/sudoers` (and `/etc/sudoers.d/`) to confirm **you're authorized to use `sudo` at all**, and for which commands.
3. `sudo` calls into **PAM** (Pluggable Authentication Modules) to actually **authenticate** you — typically, prompt for and verify your password.
4. Once PAM confirms, `sudo` forks and `exec`s the **target command**, with the process's UID set appropriately (usually root, or whichever user `-u` specified).
5. The target command now runs as a genuinely privileged process. `sudo` waits for it, then exits.
6. Control returns to your **original, still entirely unprivileged shell**.

**Why PAM specifically, rather than `sudo` implementing its own password check:** authentication — password hashing, integration with LDAP/Kerberos/2FA, account lockout policy — is needed by many independent programs (`login`, `sshd`, `sudo`, `su`, and others). PAM is a shared, pluggable, centrally-configurable framework so this logic lives in **one place**, rather than being separately (and inconsistently) reimplemented inside every program that needs to answer "is this really you."

> **Interview answer**
> "`sudo` is an ordinary `setuid-root` executable — running it gives that process root's effective UID immediately, before it does anything else. It then checks `/etc/sudoers` for authorization and delegates actual password verification to PAM, a shared authentication framework also used by `login` and `sshd`. Once authenticated, `sudo` forks and execs the target command with elevated privilege — critically, only that one command is privileged, and only for its own lifetime. Your original shell is never itself elevated."

---

## 6. Common misconceptions

| Misconception | Reality |
|---|---|
| "`sudo` makes my shell root for the rest of the session." | Only the one launched command is privileged, for its own lifetime — your interactive shell stays unprivileged before and after. |
| "`setuid` specifically means 'run as root.'" | It means "run with the effective UID of the file's owner" — generic; that owner just often happens to be root for system utilities. |
| "Permission enforcement happens inside userspace tools like `ls`/`cp`." | It's enforced by the kernel, inside the syscall path itself, for every program equally — no tool can opt out of it or implement its own version. |
| "Owner permissions stack additively with group/other." | Not cumulative — exactly one category applies, chosen by who you are relative to the file, even if a different category would have been more permissive. |

---

## 7. Try this

> **Try this**
> ```bash
> umask
> touch newfile
> ls -l newfile      # confirm it matches 666 minus your umask, per Section 2's arithmetic
> ```

> **Try this**
> ```
> ls -l /usr/bin/sudo
> ```
> Look for the `s` in the owner's execute position — the `setuid` bit, directly visible.

---

## 8. Summary

- Permission bits are per-inode, three categories (owner/group/other), checked in that strict order — never cumulative.
- `x` on a directory means "traverse," a genuinely different meaning from `x` on a file.
- `umask` subtracts from the default creation permissions (666/777), not an absolute setting of its own.
- Permission checking is kernel-enforced software logic, distinct in mechanism from file 09's hardware-enforced privilege rings.
- `setuid` makes a process's effective UID diverge from its real UID — the exact mechanism `sudo` relies on to gain root immediately upon `exec`, before delegating actual authentication to PAM.

---

## What's next

`21_PROCESS_MEMORY.md` moves from filesystem permissions to a process's own virtual address space — text/data/heap/stack, how `malloc()` actually connects to `brk()`/`mmap()`, and what ASLR randomizes.
