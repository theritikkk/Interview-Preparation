# 27 — tmux

> **Where this fits in the stack**
> ```
> Terminal Emulator → tmux CLIENT → [Unix domain socket] → tmux SERVER → shell(s) in panes
>                                    ^^^^^^^^^^^^^^^^^^^^
>                                    this file's real subject
> ```
> Your original brief drew tmux as a single linear layer — terminal emulator → tmux → shell. The real architecture is client-server, and the mechanism connecting them — a **Unix domain socket** — is genuinely absent from the rest of this handbook, whose only networking example so far (`curl`, file 30) is a completely different kind of socket. This file covers both.

---

## 1. Why tmux exists

An ordinary shell session's lifetime is tied directly to its controlling terminal's (file 16). If your terminal emulator crashes, your SSH connection drops (file 26), or you simply want to walk away and reconnect later from a different machine, an ordinary shell session is **gone** — along with everything running inside it, per file 16's `SIGHUP` discussion.

**tmux's solution:** decouple "the programs actually running" from "the terminal you happen to be looking at them through right now," by inserting a persistent layer that outlives any one terminal connection entirely.

---

## 2. The real architecture

```
Terminal emulator (local)
      |
  tmux CLIENT   (thin, local — dies the instant you detach or disconnect)
      |
  [Unix domain socket]        <- the key mechanism, Section 4
      |
  tmux SERVER   (persistent — keeps running independent of any client at all)
      |
  shell(s) / program(s) running in each pane
```

---

## 3. Server, session, window, pane — precisely, and how they nest

| Term | What it is |
|---|---|
| **Server** | One tmux server process, typically per user — the persistent thing, holding all your tmux state, running independent of whether any terminal is currently attached to it. |
| **Session** | A named collection of windows — you can run multiple independent sessions on one server ("work," "personal"). This is the unit you attach and detach. |
| **Window** | Like a tab — one full-screen view within a session; you switch between windows inside one session. |
| **Pane** | A subdivision of one window — splitting it into side-by-side or stacked regions, each running its own independent shell or program. |

**Nesting:** server → sessions → windows → panes → (a real shell or program in each pane). Structurally parallel in *shape* to file 16's session → process-group → process hierarchy — though these are distinct, tmux-specific concepts layered **on top of**, not replacing, the underlying kernel mechanics: each pane's shell is still a genuine process, in a genuine process group, in a genuine session (owned by the tmux server), exactly per file 16's real rules.

---

## 4. The Unix domain socket — the actual IPC mechanism

**What it is, precisely:** a form of inter-process communication using the **same socket API** (`socket()`, `connect()`, `send()`, `recv()` — file 09) as network sockets, but addressed via a **file path on the local filesystem** instead of an IP address and port. The address family is `AF_UNIX` (equivalently `AF_LOCAL`), rather than `AF_INET`/`AF_INET6` for genuine network sockets.

> **Remember**
> A Unix domain socket **never touches the network stack at all** — no IP routing, no TCP/UDP overhead, and no possibility of remote access, since it's addressed by a local file path that only processes on the *same machine* could ever reference. This makes it faster and simpler than even a loopback TCP connection for purely local IPC — exactly tmux's situation, since its client and server are always on the same machine, with never a legitimate reason for that communication to touch an actual network.

**Where it actually lives:** typically under `/tmp/tmux-<uid>/default` (naming can vary/be configured) — a real filesystem entry, of a distinct **socket** type, visible in `ls -l` with a leading `s` (the same convention that shows `d` for directories and `l` for symlinks). Both the server (listening on it) and any client (connecting to it) reference this same path to find each other.

**Attach/detach, now mechanically precise:** running `tmux attach` connects a client process to the server's *existing* socket — from that point, the client forwards your terminal's input to the server (which routes it to whichever pane is focused) and renders whatever the server sends back. Detaching (`Ctrl+B`, `D` by default) simply **disconnects the client from the socket and exits it** — the server, and everything running in its panes, is completely unaffected, since it was never dependent on that one connection to keep running. It goes back to having zero attached clients, patiently listening on that same socket, until something connects again.

---

## 5. Why tmux survives SSH disconnects — now fully precise

Recap file 16's tmux preview, made concrete: the tmux **server** itself called `setsid()` at some point (typically when first started), detaching it from whatever terminal or session started it. It was **never actually part of any particular SSH connection's session hierarchy** to begin with.

So: when your SSH connection drops, file 26 tells us the remote PTY that session was using gets torn down, and `SIGHUP` (files 01, 14, 16) is sent to whatever was the session leader of *that specific* SSH session's hierarchy. **The tmux server has nothing to do with that hierarchy at all** — it detached from it, permanently, the moment it first called `setsid()`. Only the now-defunct tmux **client** process — which genuinely was part of that SSH session's hierarchy, since it was just an ordinary program you launched inside that shell — dies, exactly like any other program would when its terminal disappears.

The next time you SSH back in and run `tmux attach`, a **brand-new** client, inside this **brand-new** SSH session, simply connects to the **same still-running server** over the **same Unix domain socket** — and you're back exactly where you left off, because nothing inside the server or its panes ever actually stopped.

> **Interview answer**
> "tmux survives disconnection because its server detaches from any controlling terminal via `setsid()` when it starts, so it's never part of the session hierarchy that a dropped SSH connection's `SIGHUP` propagates through. Only the thin client dies; the server, and everything running in its panes, was never affected at all — reattaching is just a new client connecting to the same long-lived Unix domain socket."

---

## 6. Terminal emulator vs tmux vs shell

- **Terminal emulator** — the outermost layer (file 01): a GUI program, owns the PTY master, renders pixels.
- **tmux** — an intermediate layer that is genuinely two things at once. From the *outer* terminal emulator's perspective, the tmux client is just an ordinary program — like any shell would be, connected to that outer PTY's slave exactly the same way. But from the perspective of the programs running *inside* its panes, tmux is acting **as if it were a terminal emulator itself**: the server allocates its own internal PTYs, one per pane, and each pane's shell is connected to one of those — with tmux interpreting, multiplexing, and re-rendering everything for whichever client happens to currently be attached.
- **Shell** — the innermost layer, one per pane. An ordinary shell process, unaware at the kernel-mechanics level that it's running "inside tmux" specifically — though it can detect this deliberately, via the `$TMUX` environment variable tmux sets, if it wants to behave differently.

> **Remember**
> tmux is simultaneously "just a program" from the outside and "acting like a terminal emulator" from the inside — both true at once, from two different vantage points.

---

## 7. Common misconceptions

| Misconception | Reality |
|---|---|
| "tmux is basically another shell." | Not a shell at all — a multiplexer sitting between a terminal emulator (or SSH connection) and one or more actual shells. |
| "Detaching pauses or suspends everything inside the session." | Everything keeps running completely normally regardless of whether any client is attached — detaching only affects whether you're currently watching it. |
| "tmux's client-server link is a network socket pointed at localhost." | Genuinely different mechanism — `AF_UNIX`, addressed by a local file path, no network stack involvement at all, not merely "a network connection to yourself." |

---

## 8. Try this

> **Try this**
> ```bash
> tmux new -s mysession
> # inside tmux: sleep 300
> # detach: Ctrl+B, then D
> ls -la /tmp/tmux-*/
> ```
> See the actual socket file, with its distinct `s` type indicator in the long listing.

> **Try this**
> ```
> tmux attach -t mysession
> ```
> Reattach and confirm your `sleep 300` is still counting down, undisturbed — direct, hands-on proof of Section 5.

---

## 9. Summary

- tmux exists to decouple running programs from any one terminal's lifetime.
- The real architecture is client-server: a thin, disposable client, and a persistent server, communicating over a Unix domain socket — a local-filesystem-addressed IPC mechanism, never touching the network stack.
- Server → sessions → windows → panes, nesting structurally like (but distinct from) the kernel's own session/process-group hierarchy.
- Disconnect survival works because the server detached from any controlling terminal via `setsid()` at startup — a dropped connection's `SIGHUP` has nothing to reach inside the server's own hierarchy.
- tmux is both "just a program" (from the outer terminal emulator's view) and "acting as a terminal emulator" (from its panes' shells' view) — genuinely both, depending on which side you're looking from.

---

## What's next

`28_DAEMONS_SYSTEMD.md` contrasts tmux's persistence against a *true* daemon — the classic double-fork-and-`setsid()` recipe in full, and why modern `systemd`-managed services don't need to implement it themselves anymore.
