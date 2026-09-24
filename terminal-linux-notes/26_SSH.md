# 26 — SSH

> **Where this fits in the stack**
> ```
> Local Terminal Emulator → Local SSH Client → Network → Remote SSH Daemon → PTY → Remote Shell → Remote Programs
>                            ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
>                            this file
> ```
> File 25 previewed `ssh` as a transparent conduit without explaining authentication, the channel itself, or how the remote side gets its own PTY. This file completes it.

---

## 1. The architecture

```
Local terminal emulator
        ↓
Local SSH client
        ↓
network
        ↓
Remote SSH daemon (sshd)
        ↓
PTY
        ↓
Remote shell
        ↓
Remote programs
```

**Local SSH client**, precisely: it puts *your* terminal into raw mode (file 23) — so every keystroke, unbuffered, gets forwarded — opens a network connection (a TCP socket, file 30) to the remote `sshd`, and from that point on shuttles bytes: your keystrokes, encrypted, across the network, to the remote side; the remote's output, decrypted, to your terminal emulator. From your local terminal emulator's own point of view, the `ssh` client process is just another ordinary program connected to its PTY slave — file 02's model, completely unchanged, whether the program on the other end is `ls` or a transcontinental network connection.

---

## 2. Authentication, conceptually

Two broad approaches:

- **Password authentication** — your password travels over the *already-encrypted* channel (Section 4), and `sshd` verifies it via the system's normal authentication mechanism — **PAM**, exactly the same framework `sudo` delegates to (file 20). `sshd` doesn't implement its own password-checking logic any more than `sudo` does.
- **Public key authentication** — the more secure, more commonly used approach for real work, covered precisely below.

---

## 3. Public/private key authentication, precisely

`ssh-keygen` generates a **key pair**: a **private key** (stays on your machine, kept secret, ideally itself protected by a passphrase) and a **public key** (safe to distribute — copied to the server's `~/.ssh/authorized_keys` file).

> **Remember**
> Authentication does **not** involve sending your private key anywhere, ever. Instead, it's a challenge-response process: the server, already knowing your public key from `authorized_keys`, can verify that whoever is connecting **possesses** the matching private key — the client cryptographically signs data the server provides, using the private key, and the server verifies that signature using the public key. This mathematically confirms private-key possession **without the private key ever leaving your machine or crossing the network at all.**

**A useful analogy:** the public key is a lock anyone can install, distributed widely; the private key is the only thing that can prove it opens that specific lock, and it's never handed to anyone. The server never needs to see it to be convinced you have it.

---

## 4. The encrypted channel

Once the connection is established — after an initial key exchange negotiating a shared session key (the cryptographic mechanics are out of scope here) — **every byte in either direction** is encrypted (confidentiality: no one observing the network traffic can read the actual content) and integrity-protected (tampering or corruption in transit is detectable).

> **Under the hood**
> This is precisely why SSH replaced older tools like `telnet` and `rsh`: those sent passwords and *all* traffic in plain text, trivially interceptable by anyone sharing the same network segment. SSH's entire reason for existing, historically, was closing exactly that hole.

---

## 5. `sshd` — the remote side, and remote PTY allocation

Recap and complete file 16's preview: for each incoming connection needing a shell, `sshd` forks a child. That child calls `setsid()` — a fresh fork, not yet a process group leader, exactly satisfying file 16's precondition — **allocates a new PTY pair** (file 02's `posix_openpt`/`grantpt`/`unlockpt` mechanics, now applied server-side), connects itself to the slave as its new session's controlling terminal, and *then* execs your login shell.

Your SSH session ends up with a **genuinely independent session/PTY/process-group hierarchy on the remote machine** — architecturally identical to opening a local terminal window (file 01), just triggered by a network connection landing at `sshd` instead of a click on a terminal emulator icon.

---

## 6. `ssh -t`

By default, `ssh` allocates a remote PTY only when it believes you want an interactive session — primarily by checking whether **your own local stdin is itself a terminal**, and whether you gave it a remote command to run at all.

**Plain `ssh user@host`** (no command): PTY allocated, interactive session, exactly as described above.

**`ssh user@host somecommand`**: by default, **no remote PTY** — `ssh` assumes you just want the command's output, non-interactively, much like a script would.

`-t` **forces** PTY allocation even when running an explicit remote command — needed whenever that remote command is itself interactive: `ssh user@host vim somefile`, or `ssh user@host sudo somecommand` (where `sudo` specifically wants to prompt for a password interactively, and many configurations explicitly refuse to do so without a real terminal present, for security reasons).

> **Interview answer**
> "`ssh -t` forces the remote side to allocate a pseudo-terminal even when you've specified a command to run, which `ssh` would otherwise treat as a non-interactive, pipe-connected invocation. It's needed whenever the remote command itself needs a real terminal — running an editor, or anything that checks `isatty()` and refuses to proceed without one."

---

## 7. What happens when no PTY is allocated

The remote command's stdin/stdout/stderr are connected to **ordinary pipes** (file 13) over the SSH connection — not a PTY at all.

**Consequence:** none of file 02/14's line-discipline machinery applies — no canonical-mode editing, no kernel-side echo, no signal generation from special characters, because there's no line discipline involved in the first place, just a plain byte stream. A remote program that expects a terminal — checks `isatty()`, tries `termios` calls, wants to do full-screen rendering (file 25) — will typically detect the absence and either refuse to run, or fall back to simpler, non-interactive behavior.

> **Under the hood**
> This is exactly the mechanism behind a detail from file 24: many CLI tools **automatically disable colored output** when their stdout isn't a terminal, precisely because `isatty()` tells them so. Piping a normally-colorful command's output through `ssh` without `-t` (or into a file, or into another pipe) commonly produces plain, uncolored text for this exact reason — nothing broke, the program is behaving correctly by its own design.

This is also exactly why `ssh user@host command` (no `-t`) is the common form used in **scripts**: no interactive-terminal machinery is wanted or needed — just the command's plain output, streamed back cleanly.

---

## 8. Common misconceptions

| Misconception | Reality |
|---|---|
| "SSH sends my private key to the server." | Never, in either authentication method — the private key stays on your machine throughout. |
| "`ssh -t` is basically always needed, or never needed." | Depends specifically on whether the remote command is interactive, and whether `ssh`'s own heuristic already gets it right without help. |
| "Running a command over SSH gives it full terminal behavior automatically." | Not by default when a command is specified — plain pipes, unless `-t` forces a PTY. |
| "SSH is essentially a remote screen-sharing protocol." | It transports a byte stream — if the remote side happens to draw a text UI with escape sequences (file 24), that's what creates the "remote control" impression; SSH itself has no rendering logic at all. |

---

## 9. Try this

> **Try this**
> ```
> ssh user@host 'tty'
> ssh -t user@host 'tty'
> ```
> Without `-t`, typically reports `not a tty`. With it, an actual `/dev/pts/N` path — direct, observable proof of Section 6/7.

> **Try this**
> If you have SSH access anywhere:
> ```
> echo $TERM
> ssh user@host 'echo $TERM'
> ```
> Usually matches — `TERM` is commonly forwarded/preserved through the connection, a direct callback to file 24's escape-sequence dialect discussion.

---

## 10. macOS note

OpenSSH (client and server) behaves essentially identically on macOS — this file transfers directly.

---

## 11. Summary

- The full chain is local terminal emulator → local SSH client (raw mode, network conduit) → network → `sshd` → a genuinely new remote PTY/session/process-group hierarchy → remote shell → remote programs.
- Public key authentication proves private-key possession cryptographically — the private key itself never crosses the network.
- The channel is encrypted and integrity-protected end to end, which is precisely what SSH replaced cleartext tools like `telnet` for.
- `ssh -t` forces remote PTY allocation for interactive remote commands that would otherwise get plain pipes and likely misbehave.
- No PTY means no line discipline at all — no echo, no canonical-mode editing, no terminal-triggered signals — just an ordinary byte stream.

---

## What's next

`27_TMUX.md` covers a different kind of persistent session — one that survives *local* disconnection entirely, built on the Unix domain socket this handbook flagged as a genuine gap in the original plan.
