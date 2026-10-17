# 47 — Separating the Session from the View

> **Where this fits in the stack**
> ```
> tmux:   server owns [ PTY + screen + layout + geometry ]      client = a window onto the server's screen
> new:    server owns [ PTY + session state + geometry ]        client owns [ layout + scroll + selection + its own emulator ]
>                       ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
>                       this file
> ```
> File 46 showed what a multiplexer owns and what that costs. This file examines a proposed redesign — the server owns the *session*, each client owns its *view* — which is the thesis behind a wave of new multiplexers and the architecture reported for Superlogical's **Rex**. It also does something the earlier files didn't need to: it separates what is **established** from what is merely **claimed**, because most of the detail about Rex comes from a demo, not from published documentation.

---

## 0. How to read the evidence

Every statement about Rex or the new multiplexers in this file carries one of four marks:

| Mark | Meaning |
|---|---|
| **[P]** | **Primary** — in text Superlogical or Mitchell Hashimoto published (the homepage, the October 5 update, his July 29 post, his replies on X). |
| **[R]** | **Reported** — described in the video walkthrough, which is its narrator's retelling of Hashimoto's demo. I could not watch the original demo, so this is second-hand. |
| **[A]** | **My analysis** — a consequence derived from files 02, 13, 45 and 46. Nobody claimed it; check my reasoning rather than trusting my word. |
| **[T]** | **Tested** — demonstrated by the code in Section 11, which I ran. |

---

## 1. The problem the new multiplexers respond to

The traditional multiplexer assumes a person sitting at the terminal. The motivating change is that **work now runs without anyone watching**: an agent can work for hours, someone wants to check it from a phone, a colleague wants to see what it did, another agent may need to read its output. Several independent writers (Peter Pistorius, who is building a multiplexer for his own product, Machinen; the video; the Superlogical homepage) converge on the same requirements: the session should survive closing the terminal, restarting an app, or losing SSH; output should exist **separately from the terminal currently displaying it**, so it can be searched and replayed; and several people and agents should be able to see and use the same work at once.

---

## 2. The thesis: the server owns the session, the client owns the view

Pistorius's formulation, which the video adopts, splits what a multiplexer holds into two kinds of state:

| **Session** (the server owns it) | **View** (each client owns it) |
|---|---|
| The PTY and process lifetime | Which sessions are visible |
| A stable session identity | How they are arranged (tabs, splits, grid) |
| Terminal output and checkpoints | Focus |
| Connected participants and their input | Scroll position |
| Reconnection and recovery | Selection and viewport |
| A protocol other software can use | |

tmux **fuses** the two: its server owns windows, panes, focus and layout, so every attached client has to reproduce the one shared view (file 46, Section 5). The proposal is to cleave them apart so that a desktop can show spatial windows, a phone one terminal, a browser a grid, and an agent reading through an API no windows at all — all over the same running sessions.

> **Remember**
> Pistorius's own conclusion is blunt about where the line goes: "the server should stop at the session." His critique of Herdr, tmux and WezTerm is that each puts layout (or, for WezTerm, a whole window/tab model) on the wrong side of that line. These are *his* characterizations; I have not independently verified how each of those projects behaves today.

---

## 3. The one thing that cannot be per-client: geometry

A tempting version of the thesis says every client should see the session at its own terminal size. It can't work, and the reason is mechanical, not a limitation of any one product.

A PTY has **exactly one size** (file 46: set with `TIOCSWINSZ`, announced to the program by `SIGWINCH` — file 14). The program draws for that size: a full-screen monitor like `btop` computes every box and cursor position for, say, 120 columns. **[A]** The only party able to redraw for 50 columns is the *program itself*, and it has been told one size, produces one output stream, and cannot draw two versions at once. Simple text can sometimes be reflowed, but full-screen programs address the screen by absolute position, so reinterpreting their output at a different width yields a different — and wrong — screen. Section 11's fifth demonstration shows the underlying fact: the *same bytes* wrap differently at different widths.

Pistorius lays out the possible ways out, none free:

- **A canonical grid per session, with a separate viewport per client** — the server fixes the size; a large client scales or pads; a small client crops or pans; a phone zooms. A compromise: a wide terminal shrunk onto a phone becomes unreadable, so at some point the client must pan instead.
- **One terminal process per client**, connected to shared application state — requires the application to cooperate; cannot work for existing programs.
- **Structured state rendered separately for each client** — same problem.

So the design rule is: **the server owns *geometry* (rows and columns inside one PTY); the client owns *layout* (how several PTYs are arranged).** The video states the same split. This is also why a regular terminal emulator is a hard client for a canonical grid: it interprets output using *its own* rows and columns, so it can show the grid correctly only if it matches the session's size or runs a separate renderer to crop and pan.

---

## 4. What the other new multiplexers do (as characterized by Pistorius)

| Project | Where the session/view line sits, per Pistorius |
|---|---|
| **zmx** | Persistent sessions and multiple attachments; explicitly **no** windows, tabs or splits — "close to the separation" he wants. |
| **cmux** | A native terminal workspace (tabs, splits, notifications, a browser) that calls itself "a primitive, not a solution," exposing a CLI and socket API. |
| **Herdr** | Workspaces, tabs, panes, agent awareness, plugins, a socket API; the **server owns layout and persists it**; direct attachment to one terminal exists, but only one direct client can own input and resize. |
| **WezTerm's multiplexer** | Persistent sessions over Unix sockets, SSH and TLS; a "multiplexing domain" is a set of windows and tabs, and a remote one needs a compatible WezTerm on the server — so a client adopts WezTerm's whole window/tab/pane model. |

Treat this table as one author's survey, written by someone with a stake in the outcome, not as a spec for each tool.

---

## 5. What is actually established about Rex

Superlogical's own pages and Hashimoto's posts — **[P]** — establish this, and no more:

- **Rex** is the multiplexer's name. On **October 5, 2026** Superlogical began **public testing**, inviting people from its mailing list in waves; the first test is limited to the **macOS application**, which **self-hosts its own Rex server** and can connect to other Rex servers.
- Rex **servers already run on Linux and Windows**; clients for iOS, Linux and Windows exist "at varying stages of readiness" but are not in testing yet.
- A private program of about **50 people** and months of internal daily use preceded this.
- Rex **"is and will always be free,"** needs **no account**, can be **fully self-hosted**, and Superlogical says it does not see or share your data. The **commercialization plan has not been shared**; they say the clients and servers themselves won't be directly monetized.
- The model is **"terminal blocks inside a long-lived session"** you can close, reconnect to from another device, and resume; sessions reachable via web and native macOS/iOS apps; **live sharing built in**; **native scrollback, selection and scrolling** as a stated goal.
- It builds on **libghostty** as "a public building block"; Ghostty itself remains a nonprofit's project.
- Hashimoto's replies on X add: scrollback currently arrives **newest-to-oldest**, though he's "not sure that's what we'll ship"; a **synced view** (everyone seeing the same viewport) is **opt-in** and works by sending additional frames of the synced viewport; the **custom binary protocol** exists because they're "on a mix of machines, local and remote" (for local, avoiding network overhead but still using IPC) and "terminals aren't the only thing we support"; and his libghostty stance — others are encouraged to build their own emulators, with the official app as a reference — **stays the same**.

Everything in Section 6 is **[R]** unless marked otherwise.

---

## 6. The reported architecture, claim by claim

### 6.1 Connection: the consistent snapshot **[R]**

When a client connects, the server **pauses its own processing of the PTY**, sends a **snapshot** — dimensions, cursor, and the *visible* screen, "just enough data to start drawing" — then a **ready frame**, and resumes. The client is now live and can render, type and select. **Scrollback is not in the snapshot**; it is **streamed afterward in the background, newest chunks first** (confirmed in direction by **[P]** above), with a brief loading state if you scroll faster than it arrives.

**Why a pause: [A]** a snapshot is only meaningful if it corresponds to one exact position in the byte stream — everything before it is inside the snapshot, everything after arrives live. This is the same idea as a database backup recording a log position so a replica knows where to resume.

> **Common mistake**
> Assuming the cut only matters for the *screen*. Section 11's demo (cases 3a/3b) shows the sharper version **[T]**: if the cut lands in the middle of an escape sequence, a snapshot of only the *grid* loses the **parser's own state** — and the client, resuming from a `ground` state it never left, prints the sequence's tail (`2m`) as literal text. A snapshot has to include the parser state or be taken only at a sequence boundary. **The sources don't say which Rex does** — an open question, not a flaw I'm asserting.

**A consequence of the pause: [A]** if the server stops *reading* the PTY master, the program's writes will eventually block once the kernel's PTY buffer fills. A pause of a moment is harmless; a long one would stall the program. (The reported claim that the handshake is imperceptible is consistent with this.)

### 6.2 Streaming: tee the raw bytes, let every client run the machine **[R]**

After the ready frame, the server **tees the raw PTY output to every connected client, unmodified**. Each client runs its **own terminal state machine** (libghostty's core) and renders its own grid. The server *also* parses the same bytes into an **authoritative** state — not for display, but as the source of truth for snapshots and recovery. This is what Hashimoto's demo line means by *"a custom binary protocol where the server is maintaining N replica distributed terminal state machines"*.

No synchronization protocol keeps them identical — **determinism** (file 45, Section 9) does it: same initial state, same bytes, same order, same geometry ⇒ same screen. **[T]** Case 1 of the demo confirms convergence even when two replicas receive the bytes in entirely different chunk sizes.

**What to call it, precisely: [A]** this is **primary–backup replication of a deterministic state machine**: one sequencer (the server) defines the order — the PTY's output stream *is* the log — and followers apply it. The video links this to database replication and consensus. The first comparison is exact; the second needs care: **consensus protocols (Paxos, Raft) solve agreeing on an order among peers**, and here the order is simply dictated by one authority, so no consensus problem arises. Consensus would only enter if the *server itself* were replicated — nothing reported says it is, and a single server remains a single point of failure.

**The conditions determinism imposes: [A][T]**

- **Reliable, ordered, complete delivery.** Case 4a shows one dropped control byte leaving the client permanently wrong (`reen` instead of `green` — the `g` consumed as the end of a dangling sequence, exactly file 45's cascade).
- **Identical geometry.** Case 5 shows the same bytes wrapping differently at 24 and 10 columns. So **every replica must run at the PTY's size** — which is exactly why geometry has to live with the server (Section 3) and why a small client views a *canonical grid* through a viewport rather than getting its own size.
- **Same snapshot start.** Cases 2 and 3.
- **Equivalent emulator behavior.** If two clients run emulators that disagree on an unspecified or extended sequence (file 45, Section 8), their replicas can differ even with perfect delivery. Whether Rex addresses version skew between clients isn't stated.

**Recovery, as reported: [R]** a client whose state has gone wrong is simply **thrown away** — reconnect, take a fresh snapshot, resume. Cheap, because the handshake already exists.

**The missing piece: [A][T]** *detection.* Case 4b shows a replica can be visibly wrong **mid-stream** yet identical **at the end** — the damage scrolled away and a later style reset healed it. A client can therefore be silently wrong for a while with nothing in the data to say so. Whether Rex uses sequence numbers, checksums, or periodic resync points isn't described in anything I could find.

### 6.3 Input: one writer, many readers **[R]**

Output fans out to everyone; **input is serialized through the server**. If two clients type at once, the server decides the order. The video calls this "basic distributed systems 101": **reads scale out, writes funnel through one ordering point.** Pistorius adds that the server can record *who* sent each input even though the application receives a single byte stream, and that independent cursors per participant would need application support.

**A subtlety worth stating: [A]** ordering must be at the granularity of whole **input events**, not bytes. A single Up-arrow press is the three bytes `ESC [ A` (file 23); if bytes from two people were interleaved inside one such sequence, the application's input parser (file 45's in-band hazard, now on the input side) would see garbage. The same applies to a pasted block. Nothing reported addresses how this is handled.

### 6.4 The view is the client's **[R]**

Because each client holds the full state machine and its own copy of history, **scroll position, text selection, and splits are per-client**: your scrolling doesn't move your teammate's. Contrast file 46's demonstration, where scrolling on one tmux client moved the other's. Each terminal stream is its own PTY with its own connection; tabs, windows and splits are **arrangements** with nothing to do with connections. The server multiplexes sessions; the client composes the view. Sharing a viewport is **opt-in** **[P]**.

### 6.5 The wire protocol **[P][R]**

A **custom binary protocol, designed from scratch**, which the video says anything can implement — libghostty first. The stated reasons **[P]** are the local/remote mix and non-terminal streams; the video adds that tmux's design made sense when text commands were fine for human use, but that today's high-volume agent traffic argues for binary. Compare tmux control mode (file 46, Section 7): text-framed, octal-escaped pane bytes, layout still in the server.

**A design pressure to watch: [A]** a tee to many clients needs a **backpressure policy** for slow ones — buffer, drop, or pause. tmux control mode needed exactly that (its `pause-after` flow control). Nothing reported says what Rex does.

### 6.6 Compatibility: smart clients and the fallback **[R]**

All of this assumes a **smart client** — one that embeds libghostty and speaks the binary protocol (a native macOS or iOS app; a web client via WebAssembly). An ordinary terminal can't parse the binary protocol, so two fallbacks are planned:

1. **Attach to a single stream by ID** — the server renders *that one terminal* into your terminal.
2. **The full multi-window experience** — the server puts a libghostty emulator **in the middle and regenerates the byte stream**.

The narrator himself observes that option 2 "is exactly what tmux does," now with a faster core optimized for the binary protocol — and that it is *not* the ideal path. **[A]** That means a client on this path inherits tmux's structural properties for itself: double parsing and a server-built view.

### 6.7 Feature parity from shared code **[R]**

Because the server's emulator and a native client's emulator are the **same code**, a capability added to libghostty — the Kitty graphics protocol is the example — would reach both. **[A]** Two caveats: both sides need versions containing the feature, and a client on the Section 6.6 fallback path is again behind a regenerating middleman. And since graphics ride in ordinary string sequences (APC, file 45), a raw tee carries them like any other bytes — which is why this design doesn't need tmux's passthrough hack.

---

## 7. Three designs, side by side

| | **tmux** (file 46) | **tmux `-CC` control mode** | **Rex, as reported** |
|---|---|---|---|
| Where the emulator runs | Server only | Client (server runs one too) | Server (authoritative) **and** every smart client |
| What reaches the client | Bytes *regenerated* from the server's grid | Raw pane bytes, octal-escaped, in a text protocol | Raw PTY bytes, in a binary protocol |
| Scrollback lives | Server | Server | Server, streamed to clients; each client keeps its own copy |
| Geometry | Server (one size; policy `latest`) | Server | Server (one canonical grid per PTY) |
| Layout | Server | **Server** (sent as `%layout-change`) | **Client** |
| Scroll / selection | Shared | Client-side | Client-side |
| Input | Client's tty read by the server | Commands over the protocol | Serialized through the server |
| Transport | Unix socket (+ SSH for remote) | Same | Custom binary protocol over local IPC or network |

---

## 8. The evidence ledger

| Claim | Level | Note |
|---|---|---|
| Rex exists; public testing began Oct 5, 2026; macOS first | **P** | Superlogical's own update |
| Free, no account, self-hostable, no data seen | **P** | Their stated policy; I can't audit it |
| Commercial plan | — | **Not shared** — stated explicitly |
| Servers on macOS, Linux, Windows | **P** | |
| Built on libghostty | **P** | Hashimoto's July 29 post |
| Scrollback newest-first, not final | **P** | His own reply |
| Synced viewport is opt-in | **P** | His own reply |
| Custom binary protocol, and why | **P** (reasons), **R** (detail) | |
| Pause-snapshot-ready handshake | **R** | Detail of snapshot contents unconfirmed |
| Raw tee to clients; client and server emulators | **R** | The core of the design; not in any text I could find |
| Input serialized through the server | **R** | |
| Fallbacks for ordinary terminals | **R** | |
| "Hundreds of times" multiplexer overhead | **Unverified** | Stated in the video; the transcript doesn't show whether it is Hashimoto's or the narrator's; no benchmark seen |
| Open source status of Rex itself | — | Only "OSS releases along the way" is stated; Rex's own licence isn't |

---

## 9. Open questions the sources leave unanswered

1. **Snapshot consistency** — does the snapshot carry parser state, or is the cut made only at sequence boundaries? (Section 6.1.)
2. **Divergence detection** — how does a client learn its replica went wrong? (6.2.)
3. **Version skew** between clients' emulators and the server's. (6.2.)
4. **Input interleaving granularity** and conflict policy between people and agents. (6.3.)
5. **Backpressure** for slow clients receiving the tee. (6.5.)
6. **Who decides the canonical size** when several clients with different windows connect — a fixed size, or resize authority passed around. (Section 3; Pistorius flags this as unresolved even for his own design.)
7. **Unbounded scrollback and search** — a thread participant asked how jumping to the top would work; Hashimoto said newest-first is how it works *now*.
8. **Persistence across a host reboot** — a PTY dies with its machine; Pistorius notes his own design cannot resurrect one either.
9. **Security and authorization** for multiplayer sessions and agent participants — the homepage's own "make it safe and operable in production" is step three of a three-step plan, not yet delivered.

---

## 10. What the discussion got right and wrong

A few recurring points from the Hacker News thread are technically substantive, offered as perspectives:

- **"A multiplexer only matters if you connect remotely."** True for persistence — locally, tabs and windows already exist (a thread participant's argument). But it ignores the *view* complaints in files 46-47: tmux's scrolling, selection and copy are awkward because the multiplexer's emulator sits between you and your real terminal. A design whose client owns the emulator addresses that directly — which is what Rex's "native scrollback, selection and scrolling" goal is about.
- **"Sessions will live on the vendor's servers."** A natural worry; the October 5 update answers it for Rex's client and server — self-hostable, no account, no data seen — though "a vendor-run service might come later" can't be ruled out from a post that says its commercialization plan is unshared.
- **"Sharing a live session is bad UX because terminals must be the same size."** This is Section 3 exactly, stated by a user who had tried it. The reported design doesn't make that go away; it moves the cost to a canonical grid plus per-client viewports.
- **"tmux is good enough for agents."** Agents *can* drive tmux, as participants noted. The real argument isn't capability — it's whether the **ownership boundary** (Section 2) is in the right place.

---

## 11. Lab: replicated terminal state machines

This is the script behind every **[T]** mark above. It models the terminal as a state machine (including its parser state, per file 45) and checks what keeps replicas identical and what breaks them. It is a **model**, not Rex.

```python
#!/usr/bin/env python3
"""Replicated terminal state machines: convergence, late-join snapshots, and what breaks."""
import copy

class Emu:
    def __init__(self, rows=3, cols=24):
        self.rows, self.cols = rows, cols
        self.grid = [[(' ', 0) for _ in range(cols)] for _ in range(rows)]
        self.row = self.col = self.pen = 0
        self.state, self.params = 'GROUND', ''          # PARSER state is part of the state!
    def newline(self):
        self.col = 0
        if self.row == self.rows - 1:
            self.grid.pop(0); self.grid.append([(' ', 0) for _ in range(self.cols)])
        else: self.row += 1
    def feed(self, data):
        for b in data: self._byte(b)
    def _byte(self, b):
        if self.state == 'GROUND':
            if b == 0x1B: self.state = 'ESCAPE'
            elif b == 0x0D: self.col = 0
            elif b == 0x0A: self.newline()
            elif 0x20 <= b < 0x7F:
                if self.col >= self.cols: self.newline()
                self.grid[self.row][self.col] = (chr(b), self.pen); self.col += 1
        elif self.state == 'ESCAPE':
            if b == ord('['): self.state, self.params = 'CSI', ''
            else: self.state = 'GROUND'
        else:                                            # CSI
            if 0x30 <= b <= 0x3F: self.params += chr(b)
            else:
                if b == ord('m'): self.pen = int(self.params or 0)
                self.state = 'GROUND'
    def snapshot(self, with_parser_state=True):
        s = copy.deepcopy(self)
        if not with_parser_state: s.state, s.params = 'GROUND', ''
        return s
    def view(self):
        return (tuple(tuple(r) for r in self.grid), self.row, self.col, self.pen)
    def text(self):
        return ' | '.join(''.join(c for c, _ in r).rstrip() or '.' for r in self.grid)

STREAM = b'\x1b[31mhello\x1b[0m world\r\nline2\r\n\x1b[32mgreen\x1b[0m\r\n'
def same(a, b): return a.view() == b.view()

# 1. Determinism: same bytes, different chunking, no coordination
a = Emu(); a.feed(STREAM)
b = Emu()
for i in range(0, len(STREAM), 5): b.feed(STREAM[i:i+5])
print("1. same bytes, different chunking, no sync      ->", "IDENTICAL" if same(a, b) else "DIVERGED")

# 2. Late joiner: snapshot at a clean boundary, then stream the remainder
k = STREAM.index(b'line2')
server = Emu(); server.feed(STREAM[:k]); snap = server.snapshot()
late = snap; late.feed(STREAM[k:])
print("2. late join from snapshot at clean boundary    ->", "IDENTICAL" if same(a, late) else "DIVERGED")

# 3. Snapshot cut MID-SEQUENCE (just after 'ESC [ 3' of the green sequence)
k = STREAM.index(b'\x1b[32m') + 3
server = Emu(); server.feed(STREAM[:k])
print("   (parser state at the cut: %s, params=%r)" % (server.state, server.params))
grid_only = server.snapshot(with_parser_state=False); grid_only.feed(STREAM[k:])
full      = server.snapshot(with_parser_state=True);  full.feed(STREAM[k:])
print("3a. mid-sequence cut, GRID ONLY snapshot        ->", "IDENTICAL" if same(a, grid_only) else "DIVERGED")
print("    server  :", a.text()); print("    client  :", grid_only.text())
print("3b. mid-sequence cut, snapshot WITH parser state ->", "IDENTICAL" if same(a, full) else "DIVERGED")

# 4a. One control byte lost where the damage is still on screen at the end
#     (the final 'm' of the GREEN sequence: the next byte, 'g', gets eaten as that sequence's final)
i = STREAM.index(b'32m') + 2
lossy = Emu(); lossy.feed(STREAM[:i] + STREAM[i+1:])
print("4a. dropped control byte, damage still visible  ->", "IDENTICAL" if same(a, lossy) else "DIVERGED")
print("    server  :", a.text()); print("    client  :", lossy.text())

# 4b. The same kind of loss in the FIRST line, which later scrolls off: wrong mid-stream, right at the end
j = STREAM.index(b'31m') + 2
k2 = STREAM.index(b'line2')
mid_ok, mid_bad = Emu(), Emu()
mid_ok.feed(STREAM[:k2]); mid_bad.feed((STREAM[:j] + STREAM[j+1:])[:k2-1])
end_bad = Emu(); end_bad.feed(STREAM[:j] + STREAM[j+1:])
print("4b. loss in a line that later scrolls away      -> mid-stream:",
      "IDENTICAL" if same(mid_ok, mid_bad) else "DIVERGED", "| at the end:",
      "IDENTICAL" if same(a, end_bad) else "DIVERGED")
print("    mid-stream server:", mid_ok.text()); print("    mid-stream client:", mid_bad.text())

# 5. Same bytes, different geometry (compared mid-stream, while the wrapped line is still on screen)
k2 = STREAM.index(b'line2')
wide, narrow = Emu(rows=3, cols=24), Emu(rows=3, cols=10)
wide.feed(STREAM[:k2]); narrow.feed(STREAM[:k2])
print("5. same bytes, 24 vs 10 columns (mid-stream)    -> wrapping differs")
print("    24 cols :", wide.text()); print("    10 cols :", narrow.text())
```

Its output, as run:

```
1. same bytes, different chunking, no sync      -> IDENTICAL
2. late join from snapshot at clean boundary    -> IDENTICAL
   (parser state at the cut: CSI, params='3')
3a. mid-sequence cut, GRID ONLY snapshot        -> DIVERGED
    server  : line2 | green | .
    client  : line2 | 2mgreen | .
3b. mid-sequence cut, snapshot WITH parser state -> IDENTICAL
4a. dropped control byte, damage still visible  -> DIVERGED
    server  : line2 | green | .
    client  : line2 | reen | .
4b. loss in a line that later scrolls away      -> mid-stream: DIVERGED | at the end: IDENTICAL
    mid-stream server: hello world | . | .
    mid-stream client: ello world | . | .
5. same bytes, 24 vs 10 columns (mid-stream)    -> wrapping differs
    24 cols : hello world | . | .
    10 cols : hello worl | d | .
```

How to read it:

- **Case 1** — determinism: replicas fed the same bytes in different chunkings agree, with no coordination.
- **Case 2** — a late-joining client restored from a snapshot at a clean boundary matches the server.
- **Case 3** — the sharp one: cut mid-sequence, a *grid-only* snapshot yields `2mgreen` (the sequence tail printed literally); a snapshot **with parser state** matches exactly.
- **Case 4a** — a dropped control byte leaves a persistent divergence (`reen`).
- **Case 4b** — a similar loss that is visibly wrong **mid-stream** but identical **at the end**: damage can heal, so a replica can be silently wrong in between.
- **Case 5** — identical bytes, different geometry, different wrapping: replicas must share the PTY's size.

> **Try this**
> Extend the demo: add a **sequence counter** to the stream and have the client verify it; then reproduce Case 4b and see whether your client now notices the loss. That is the "detection" gap from Section 9 — closed by hand.

> **Try this**
> In Section 3's terms, resize your own terminal while running `btop` or `htop` and watch the *program* redraw in response to `SIGWINCH` — proof that only the program can produce a screen for a new size.

---

## 12. Common misconceptions

| Misconception | Reality |
|---|---|
| "Rex is just tmux with a nicer UI." | Reported to move the emulator, layout, scroll and selection to the client — a different ownership boundary, not a skin. |
| "Replicas need a synchronization protocol." | Determinism does the work — but it requires an ordered, complete stream, a consistent starting snapshot, and identical geometry. |
| "Replicated state machine means consensus." | Not here: one sequencer orders everything. Consensus appears only if the sequencer itself is replicated. |
| "Each client can have its own terminal size for the same session." | A PTY has one size and only the program can redraw — geometry is per-session; layout is per-client. |
| "A snapshot is just a copy of the screen." | It must also capture or sidestep the parser's state, or a mid-sequence cut corrupts the client (Case 3). |
| "Raw tee means there's no middleman at all." | The server still parses (for the authoritative copy); it just doesn't *regenerate* output — except on the fallback path, where it does. |
| "A diverged replica is obviously broken." | It can look right later (Case 4b), so detection needs a mechanism. |
| "Every claim about Rex is documented." | The architecture detail is reported from a demo; only the product facts and a few protocol statements are published (Section 8). |

---

## 13. Sources

All fetched or provided on **October 14, 2026**. **Published by Superlogical/Hashimoto:** the Superlogical homepage; *Public testing is beginning* (October 5, 2026); Mitchell Hashimoto, *Superlogical* (July 29, 2026); his replies on X (July 31, 2026), as provided. **Third-party:** Peter Pistorius, *Why are there so many new terminal multiplexers?* (July 30, 2026) — an author building a competing-in-spirit product, so read his characterizations of other tools as one view; the Hacker News discussion; the video walkthrough transcript and chapter list, **which is the narrator's account of Hashimoto's demos, not the demos themselves** — I could not view the video; Paul Flo Williams' parser document and Hashimoto's libghostty post (September 22, 2025), via file 45. **Technical background:** files 02, 13, 14, 23, 45, 46. The Section 11 script was run.

---

## 14. Summary

- The proposal: the **server owns the session** (PTY, process, history, identity, input ordering, geometry); each **client owns its view** (layout, focus, scroll, selection, its own emulator).
- **Geometry can't be per-client** — a PTY has one size and only the program can redraw; **layout can be**.
- Reported design: pause-and-snapshot on connect, raw-byte tee to every client, an emulator in each client plus an authoritative one in the server, input serialized through the server, a custom binary protocol, and a tmux-like fallback for ordinary terminals.
- The convergence works because terminals are deterministic — under conditions the demo tests: ordered complete delivery, identical geometry, and a snapshot that includes parser state.
- The accurate name is **primary–backup replication of a deterministic state machine**, not consensus.
- Published facts about Rex are few (free, self-hostable, macOS beta, libghostty-based); the architecture detail is **reported**, and nine concrete questions remain open.

---

## What's next

The three multiplexer files (45-47) are complete. Next I'll add pointers to them from files 23 and 24 and add all three to the file index in file 40. The mini-multiplexer project this paragraph once proposed now exists: `48_MINI_MULTIPLEXER_PROJECT.md` builds a server, a terminal model, snapshot-on-attach, fan-out and a client using exactly the mechanisms in files 45-47, with a test suite that reproduces Section 11's mid-sequence hazard against a live shell.
