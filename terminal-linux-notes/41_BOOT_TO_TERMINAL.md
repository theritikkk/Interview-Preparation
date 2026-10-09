# 41 — Boot to Terminal

> **Where this fits in the stack**
> ```
> Power → Firmware → Bootloader → Kernel → initramfs → PID 1 → getty / display manager → login → Shell → Terminal
> ```
> Everything in this handbook so far started *after* a shell already existed. This file zooms out to the question underneath: how does a shell ever come to exist? It also finally answers a question files 08, 16, and 28 deferred here: what *is* PID 1?

---

## 1. The full chain

```
Power button
 ↓
firmware (UEFI, or legacy BIOS)
 ↓
bootloader (e.g. GRUB)
 ↓
kernel
 ↓
initramfs
 ↓
kernel initialization completes
 ↓
PID 1 (systemd, on most distros)
 ↓
getty / login manager
 ↓
user login
 ↓
shell
 ↓
terminal
```

---

## 2. Each stage, precisely

**Firmware.** On power-up, the CPU begins executing firmware stored on the motherboard. UEFI (or legacy BIOS) runs hardware self-tests, finds a boot device, and loads the **bootloader** from it — on UEFI systems, from a dedicated EFI System Partition. No operating system exists yet; this is pure firmware.

**Bootloader (e.g. GRUB).** Loads two things into memory: the **kernel image** and the **initramfs**. Passes the kernel a **command line** (boot parameters — including, optionally, `init=` to override which program becomes PID 1), then jumps to the kernel.

**Kernel.** Decompresses itself, initializes the CPU, memory management, scheduler (file 22), and core drivers. At this stage it has no real root filesystem yet — only what the bootloader loaded.

**initramfs.** A small, temporary root filesystem loaded into RAM, containing just enough drivers and tools to find and mount the *real* root filesystem — necessary when that filesystem sits on something the kernel can't reach unaided (an encrypted volume, LVM, RAID, a driver built as a loadable module). Once the real root is mounted, the system switches over to it and the initramfs is discarded.

**PID 1.** The kernel then starts the first userspace process by executing a program — `/sbin/init` by default, which on most modern distros is a symlink to `systemd` (the `init=` parameter above can override this). This process gets **PID 1**.

---

## 3. What PID 1 actually is

PID 1 is the **first and ancestral userspace process** — every other userspace process descends from it, directly or indirectly (file 44 shows the full tree). It has several genuinely special properties:

- **It's the orphan reaper.** Processes whose parent exits before they do are re-parented to PID 1 (or to a designated "subreaper" — file 44), so something is always around to `wait()` for them (files 08, 29).
- **It's protected from signals.** The kernel only delivers a signal to PID 1 *from within its own PID namespace* if PID 1 has installed a handler for it — an ordinary `kill` that would terminate any other process does nothing to an init that hasn't asked to catch it. This is exactly the same special treatment file 29 described for a container's PID 1.
- **If it dies, the kernel panics.** The system cannot continue without it — the kernel reports something like "Attempted to kill init!" and halts.
- **It has no controlling terminal** from the moment of its creation — nothing started it from a terminal (file 16, Section 6; file 28).

> **Under the hood**
> Two other kernel-internal tasks exist before and beside PID 1, though you won't see the first in `ps`: PID 0 (the idle/swapper task, never a normal process) and PID 2 (`kthreadd`, the parent of all kernel threads — the `[bracketed]` entries in `ps`). PID 1 is the *userspace* ancestor specifically.

---

## 4. From PID 1 to a login prompt

systemd (file 28) brings up services in dependency order, eventually reaching a target that starts one of two things:

**Text console login (physical TTY).** systemd starts a `getty` instance per virtual console (`/dev/tty1`, and so on — file 01). `getty` calls `setsid()`, opens the TTY device so it becomes the session's controlling terminal (file 16), prints the login prompt, reads a username, and `exec`s `login`. `login` authenticates the user via **PAM** (file 20), sets up the session, and `exec`s the user's shell — as a **login shell**, which is exactly the `argv[0]`-starts-with-`-` convention from file 18, and why startup files like `/etc/profile` run here. Notably, `getty` → `login` → shell are the *same process*, replaced twice via `exec` (file 08), never forked into a chain.

**Graphical login.** systemd instead starts a **display manager** (GDM, SDDM, and so on), which draws the login screen, authenticates via PAM, and starts the user's desktop session. There is no text console and no `getty` involved.

---

## 5. Where terminal emulators enter the picture

Only on the **graphical** path — and only after you're already logged in. The desktop session is running; you launch a terminal emulator as an ordinary GUI program; it allocates a PTY (file 02) and forks a shell onto it. That shell is typically a **non-login, interactive** shell (file 18) — which is why opening a terminal window doesn't re-run your login-shell startup files on most Linux setups.

---

## 6. Physical TTY login vs graphical terminal — directly compared

| | Physical TTY (`Ctrl+Alt+F3`) | Graphical terminal emulator |
|---|---|---|
| What draws the screen | The kernel's own console driver (file 24's virtual-console exception) | A userspace GUI program |
| Device | `/dev/tty3`, a kernel-native console | `/dev/pts/N`, a PTY slave (file 02) |
| Started by | `getty`, via systemd | You, launching a GUI application |
| Shell type | Login shell | Usually non-login interactive |
| Terminal-emulator process involved | None | Yes |

---

## 7. Common misconceptions

| Misconception | Reality |
|---|---|
| "The kernel starts your shell." | The kernel starts exactly one userspace program, PID 1; everything else, including your shell, descends from it through `fork`/`exec` chains. |
| "PID 1 is just the oldest ordinary process." | It has genuinely special kernel-level properties — orphan reaping, signal protection, and a kernel panic if it exits. |
| "`getty`, `login`, and the shell are three separate processes." | The same process, `exec`'d into each program in turn. |
| "A terminal emulator is part of booting." | It only enters after a graphical session already exists — a plain console login never involves one. |

---

## 8. Try this

> **Try this**
> ```
> ps -o pid,ppid,comm 1
> cat /proc/cmdline
> ```
> Confirm PID 1's identity, and see the exact boot parameters the bootloader handed the kernel — including any `init=` override.

> **Try this**
> Press `Ctrl+Alt+F3`, log in at the text console, then run `tty` and `echo $0`. You should see a `/dev/tty3` device and a leading `-` on the shell name — a live login shell, exactly as Section 4 describes. `Ctrl+Alt+F1` or `F2` (depending on the distro) returns you to the graphical session.

---

## 9. macOS note

macOS boots differently — Apple Silicon starts from Boot ROM and iBoot rather than UEFI/GRUB, loads the XNU kernel, and the equivalent of PID 1 is `launchd`, not systemd. Full comparison: file 43.

---

## 10. Summary

- Firmware loads a bootloader; the bootloader loads the kernel and an initramfs; the initramfs finds the real root filesystem; the kernel then executes PID 1.
- PID 1 is the ancestral userspace process: orphan reaper, signal-protected, fatal-if-lost, and terminal-less from birth.
- A text login goes `getty` → `login` → shell, all one process `exec`'d repeatedly; a graphical login goes through a display manager instead.
- A terminal emulator only appears on the graphical path, after login, as an ordinary GUI program.

---

## What's next

`42_SHELL_GOTCHAS_AND_EXIT_STATUS.md` returns to the shell itself: the specific traps that catch experienced programmers, and exactly how exit status flows from `exit()` through `waitpid()` into `$?`.
