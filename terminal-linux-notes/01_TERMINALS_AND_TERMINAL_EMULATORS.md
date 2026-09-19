# 01 — Terminals and Terminal Emulators

> **Where this fits in the stack**
> ```
> User  →  [ TERMINAL EMULATOR ]  →  PTY/TTY  →  Shell  →  Programs  →  Syscalls  →  Kernel  →  Hardware
>                  ^^^^^^^^^^^^
>                  this file
> ```
> This file answers one question precisely: **what is the box you're typing into, and what is it not?**
> The PTY mechanism that connects this box to a shell is the *next* file. Escape-sequence rendering (colors, cursor movement) is deferred to file 24. Nothing here requires those — this file is self-contained.

---

## 1. The vocabulary, historically

These five words get used interchangeably in casual speech. They are not interchangeable. Getting them precise now prevents confusion in every file that follows.

### Terminal

Originally: a physical piece of hardware — a keyboard plus some output device (paper or screen) — sitting at the *end* of a communication line, used by a human to interact with a computer that was physically elsewhere (often in another room, sometimes another building). The computer was the expensive, shared resource; terminals were the cheap things scattered around it so many people could use it at once.

### Console

A specific kind of terminal: the one **physically attached to, and often built into, the computer itself**, used by the operator for administration, boot messages, and system-level control — distinct from the many ordinary terminals users connected with remotely. On Linux today, "console" survives as a precise technical term: `/dev/console` is the device that receives kernel boot and diagnostic messages, and Linux's virtual consoles (`/dev/tty1`–`/dev/tty6`, reached via `Ctrl+Alt+F1`–`F6` on many distros) are the direct modern descendant of this concept — full-screen text displays driven straight by the kernel, no terminal emulator software involved at all.

### Teletype

The actual hardware that gave the whole subsystem its name. A **teletype** (Teletype Corporation's electromechanical printing terminals, the Model 33 ASR being the most common in early Unix history) was originally built for telegraph networks: you typed on a keyboard, it clattered out your own keystrokes on paper *and* transmitted them down a wire, and it could also receive characters from the wire and print those too. No screen — everything was printed on a continuous paper roll. When Bell Labs' early Unix systems (early 1970s) needed a human-interface device, teletypes were what was available, and they were plugged in as-is.

### TTY

Literally an abbreviation of **teletype**. This is not a coincidence or a cute nickname — it's a direct historical fossil. Unix's entire terminal-handling subsystem in the kernel is called the **tty subsystem** to this day, device files are named `/dev/tty*`, and the C library still has functions like `tcgetattr`/`isatty` because Unix was originally written to drive literal teletype machines. The hardware disappeared decades ago; the name, and a great deal of the interface contract, did not.

### Terminal emulator

A **software program** that behaves the way a physical terminal used to behave, closely enough that the rest of the Unix stack — which still assumes a "terminal" is out there — can't tell the difference. This is the thing you actually launch when you double-click "Terminal" on your desktop today. It emulates a device that, in almost every case, no longer physically exists.

> **Remember**
> Terminal = historical hardware concept. Console = the specific terminal attached to the machine itself. Teletype = the actual hardware that started it all. TTY = the abbreviation that survives in every Linux device name and API. Terminal emulator = the modern software standing in for all of the above.

---

## 2. Physical terminals: what they actually were

Before you can understand why software needs to *emulate* a terminal, you need a concrete picture of the real thing.

**Teletypes (1960s–70s):** A Teletype Model 33 was essentially an electric typewriter wired to a communication line. Keystrokes were converted to electrical signals and sent down a serial line (originally a 20mA current loop, later RS-232) to a computer, often at a punishing 110 baud (about 10 characters per second). The computer's response came back down the same line and was printed on the same paper roll your typing appeared on. There was no way to "clear the screen" — there was no screen. Scrolling was literal: paper feeding upward.

**Video terminals / "glass ttys" (late 1970s onward):** Devices like the DEC VT52 and, especially, the **VT100** (1978) replaced paper with a CRT screen. This was a huge leap, but the terminal itself was still "dumb" in the sense that mattered: it had just enough local logic to interpret a stream of bytes from the host computer — some of them printable characters, some of them special *escape sequences* meaning things like "move the cursor to row 5, column 10" or "clear the screen" — and to send raw keystrokes back up the wire. All the real computation still happened on the host. The VT100's specific escape-sequence dialect became so widely copied that "VT100-compatible" is still the baseline every modern terminal emulator supports, and the ANSI escape codes you'll meet in file 24 are its direct descendants.

**The physical connection:** a serial cable (RS-232), sometimes running to a **terminal concentrator** that multiplexed many terminals' serial lines onto fewer connections back to a shared mainframe or minicomputer. One computer, potentially dozens of terminals, each used by a different person, all completely unaware of each other.

> **Under the hood**
> The reason Unix has a notion of "**line speed**" and historically cared about things like flow control in the terminal driver is not an abstraction someone invented for elegance — it's a direct consequence of these devices being genuinely slow, genuinely serial, and genuinely limited in local buffering. The API surface you'll meet in later files (`termios`, baud-rate-flavored constants like `B9600` still defined in `<termios.h>`) is this history, preserved.

---

## 3. Why your computer doesn't have "a terminal" plugged into it

The terminal/host split made sense when computers were rare, room-sized, and shared. It stopped making sense once computers became personal: a single-user machine with its own built-in keyboard and its own built-in screen has no need for a separate device at the end of a wire to let a human interact with it. The keyboard and screen you already own **are** the interaction hardware; there is no gap for a physical terminal to fill.

But an enormous amount of existing software — shells, editors, compilers' interactive prompts, essentially all of Unix's textual interface — was written assuming a terminal was there, understood escape sequences, and could be configured via the tty API. Rather than rewrite all of that, the industry did something more economical: **write a program that pretends, convincingly, to be a physical terminal**, while actually drawing to a graphical window using your PC's own screen and keyboard.

That program is a **terminal emulator**, and this is precisely why the name is literal rather than cute: it is not a new kind of terminal, it is software *emulating* the old kind, closely enough that decades of terminal-assuming software never had to change.

**Where a physical terminal-like device still exists on modern machines:**
- **Serial console ports** on servers and embedded hardware, often paired with out-of-band management (IPMI, BMC, "serial-over-LAN") — the direct professional-grade survivor of the original concept, used specifically because it works even when the main OS and network stack are down.
- **Linux virtual consoles** (`/dev/tty1`–`tty6`) — not emulated in userspace software at all, but a genuine kernel-native text display driven directly onto your screen, switchable with `Ctrl+Alt+F1`–`F6`. This is the one case on an ordinary desktop Linux machine where you get something console-like without any terminal-emulator *application* involved.
- Everything else — literally every terminal window you open on a desktop, in a browser, over SSH — is a terminal emulator, full stop.

> **Common mistake**
> "My laptop doesn't have a terminal attached, so terminals are basically obsolete and terminal emulators are just historical nostalgia." The mechanism (PTYs, `termios`, escape sequences, the whole tty API) is not nostalgia — it is *the actual live interface* every shell, editor, and interactive program on your system uses today. What's obsolete is the physical wire. The protocol and the software architecture it produced are very much still running, right now, underneath the window you're reading this in.

---

## 4. The modern terminal emulator zoo

All of the following are userspace applications you install like any other program. None of them are part of the kernel. None of them are shells.

| Terminal emulator | Platform | Rendering | Notable characteristics |
|---|---|---|---|
| **GNOME Terminal** | Linux (GNOME desktop) | CPU-rendered, via the VTE library | The historical default on many Ubuntu/GNOME installs; VTE is shared by several other GTK-based terminal apps |
| **Konsole** | Linux (KDE desktop) | CPU-rendered | KDE's equivalent of GNOME Terminal; also used as an embeddable widget by other KDE apps |
| **Alacritty** | Linux / macOS / Windows | **GPU-accelerated** (OpenGL) | Minimalist by design — no built-in tabs; leans on external tools like tmux (file 27) for multiplexing; config as a plain text file |
| **Kitty** | Linux / macOS | **GPU-accelerated** | More built-in features than Alacritty (tabs, layouts, its own image-display protocol) |
| **iTerm2** | macOS | CPU-rendered | The most widely used third-party replacement for Apple's stock terminal; adds split panes, extensive shell integration, search |
| **Terminal.app** | macOS | CPU-rendered | Apple's stock, default terminal emulator |

**What actually differs between them:** rendering technology (does drawing text use the GPU or not — this affects scrollback/redraw speed, especially with heavy output), configuration philosophy, and bundled convenience features (tabs, splits, image protocols, GUI preference panels).

**What is identical between every single one of them:** their fundamental job in the stack. A GPU-accelerated Rust terminal and a 20-year-old CPU-rendered one are, from the operating system's point of view, doing exactly the same thing.

---

## 5. What does a terminal emulator actually do?

Stated precisely, a terminal emulator's job is:

1. **Obtain a PTY** (pseudo-terminal) — a kernel-provided master/slave device pair. (Full mechanics: file 02.)
2. **Start a shell as a child process**, arranged so that the shell's controlling terminal is the PTY's slave side.
3. **Read bytes from the PTY master** — whatever the shell and any programs it runs write to their stdout/stderr eventually arrives here — and **render them**: printable characters become glyphs on screen, and specific byte sequences (escape sequences) are interpreted as instructions like "change color," "move the cursor," "clear the line" (file 24).
4. **Capture keyboard input** from the windowing system/GUI toolkit and **write those bytes into the PTY master**, which the kernel then makes available on the slave side for the shell (or whatever program currently owns the terminal) to read.
5. **Own the presentation layer**: the window itself, tabs, split panes, the scrollback buffer, font rendering, text selection, copy/paste, bell/notification behavior.
6. **Track window size** and communicate resizes down through the PTY (this is what eventually produces the `SIGWINCH` signal a program can catch — previewed in file 14).
7. **Set the initial environment** for the shell it launches — most notably the `TERM` variable, which tells every program running inside which escape-sequence dialect this particular terminal emulator understands.

**What a terminal emulator explicitly does not do**, despite how it might feel from the outside:

- It does **not** parse `ls -lah` and know what that means.
- It does **not** know what `PATH` is, or search it.
- It does **not** fork or exec `ls` when you type it and press Enter. Your keystrokes were written into the PTY; the **shell**, already running as a child process and reading those bytes from its stdin, is the thing that parsed the line and did the fork/exec work. The terminal emulator's involvement in running `ls` was finished the moment it delivered your keystrokes.
- It does **not** implement aliases, environment variable inheritance rules, job control logic, or command history. Those are shell features (files 04–07, 15, 42), not terminal-emulator features.

> **Interview answer**
> "A terminal emulator is a userspace program that presents a pseudo-terminal to a shell, renders whatever the shell (and programs run from it) write to that PTY, and forwards keyboard input into it. It has no knowledge of shell syntax, PATH, or process management — that's entirely the shell's responsibility, running as a separate child process."

---

## 6. The central question: is the terminal emulator the shell?

**No.** They are two separate programs, running as two separate operating-system processes, with one being the parent of the other. This is worth internalizing precisely, because almost every confusing "why did closing my window kill my script" or "why does my shell behave differently over SSH" question in later files traces back to *not* keeping these two processes distinct in your head.

### Terminal ≠ Shell — the stack view

```
+--------------------------------------------------+
| You                                               |
+--------------------------------------------------+
| TERMINAL EMULATOR   (e.g. Alacritty, iTerm2)      |
|   - a GUI application                              |
|   - renders text, interprets escape sequences      |
|   - owns the PTY master                            |
|   - has ZERO knowledge of shell syntax             |
+--------------------------------------------------+
| PTY  (kernel mechanism — see file 02)             |
+--------------------------------------------------+
| SHELL   (e.g. bash, zsh)                          |
|   - parses commands, expands variables            |
|   - forks and execs programs                       |
|   - has ZERO knowledge of pixels, fonts, windows   |
+--------------------------------------------------+
| PROGRAMS  (ls, grep, vim, gcc, ...)               |
+--------------------------------------------------+
```

Two adjacent boxes, two completely different kinds of program, communicating through a narrow, well-defined channel (the PTY) that neither needs to understand the other's internals to use.

### Terminal ≠ Shell — the process-tree view

This is the same fact, seen the way the kernel sees it — as a plain parent/child process relationship:

```
alacritty            (PID 2001)   <- the terminal emulator process
 └── bash            (PID 2015)   <- the shell: a CHILD process of the terminal emulator
      └── ls         (PID 2088)   <- a command: a CHILD process of the shell, briefly
```

Three genuinely separate operating-system processes, each with its own PID, its own memory, its own code. The terminal emulator is not "inside" the shell and the shell is not "inside" the terminal emulator in any code-level sense — the only thing connecting them is that one forked the other, and they happen to share a PTY.

> **Under the hood**
> Some terminal emulators complicate this picture slightly for their own internal reasons — GNOME Terminal, for instance, actually runs as a persistent `gnome-terminal-server` process with thin client processes per window, for faster window creation. That's an implementation detail of *that one emulator*; the parent-of-the-shell relationship still holds, it's just that the direct parent may be a server process rather than the window process you clicked on. `pstree` will show you the real tree on your machine (see the lab below) — don't take any single diagram as universal across every terminal emulator's internal plumbing.

### A concrete scenario that only makes sense once you see them as separate processes

**Closing the terminal window** (clicking the X): the terminal emulator process typically sends `SIGHUP` ("hang up" — another teletype-era name, for when a phone line literally hung up) to its child, the shell. The shell, and typically anything it left running in the foreground, receives this and usually terminates. The terminal emulator process then exits too, and the PTY is torn down. *(Full signal mechanics: file 14. Why backgrounded jobs sometimes survive this and sometimes don't: file 16.)*

**Typing `exit` in the shell**: this is the *opposite* direction. The shell process calls `exit()` on itself, voluntarily. The terminal emulator notices its child has terminated — typically by reading end-of-file from the PTY master, or by receiving `SIGCHLD` — and, depending on its configuration, closes the window itself.

Same visible outcome (the window disappears) from two completely different causal directions — one initiated by the terminal emulator killing the shell, the other by the shell dying on its own and the terminal emulator reacting. You cannot explain the difference without already knowing these are two separate processes watching each other.

---

## 7. Under the hood preview: how a terminal emulator is actually built

This is a **preview only** — every syscall named here (`posix_openpt`, `grantpt`, `unlockpt`, `ptsname`) is explained fully, with working code, in `02_TTY_AND_PTY.md`. The point right now is just to make the diagram above concrete enough that it stops feeling like hand-waving.

```c
/* Extremely simplified skeleton of what every terminal emulator does.
 * Real ones add GUI rendering, escape-sequence interpretation, resize
 * handling, and a great deal more around this core. PTY API details
 * are fully explained in the next file — this is only here to show
 * WHERE that mechanism fits into what you just read above. */

int master_fd = /* obtain a PTY master — posix_openpt() and friends */;

pid_t pid = fork();
if (pid == 0) {
    /* --- child: this becomes the shell --- */
    setsid();                              /* new session, detailed in file 16 */
    int slave_fd = /* open the PTY slave's path */;
    dup2(slave_fd, STDIN_FILENO);
    dup2(slave_fd, STDOUT_FILENO);
    dup2(slave_fd, STDERR_FILENO);
    execlp("/bin/bash", "bash", NULL);     /* bash now believes slave_fd
                                               genuinely IS its terminal */
} else {
    /* --- parent: this loop IS the terminal emulator's core job --- */
    for (;;) {
        /* bytes arrive from master_fd  -> interpret + render on screen */
        /* keystrokes arrive from the GUI -> write them into master_fd  */
    }
}
```

Notice this matches the process tree from Section 6 exactly: `fork()` produces the parent (terminal emulator, keeps running its render/input loop) and the child (which `execve`s into `bash`, becoming the shell). Nothing here is terminal-emulator magic — it's the same `fork`+`exec` pattern that will reappear, formally, in file 08.

---

## 8. Common misconceptions

| Misconception | Reality |
|---|---|
| "The terminal is the black window." | That's the **terminal emulator** — a specific GUI application. "Terminal," precisely, names the historical hardware concept the software is emulating. Fine to blur in casual speech; not fine in your systems mental model. |
| "Bash and Terminal are the same program." | Two separate processes. The terminal emulator starts the shell as a child and then gets out of the way, communicating only through the PTY. |
| "If I close my terminal, everything I ran is gone forever, no exceptions." | Usually true for foreground jobs (`SIGHUP`), but not a universal law — `nohup`, `disown`, and `tmux`/`screen` all exist specifically to change this behavior. Covered in files 15, 16, 27. |
| "The terminal draws pixels; the shell tells it what pixels to draw." | Neither the shell nor the kernel draws anything. The shell writes plain bytes (some of them escape sequences) to its stdout. Only the terminal emulator — a GUI program — turns any of that into actual pixels on your screen. |
| "GPU-accelerated terminals like Alacritty work fundamentally differently from the OS's perspective." | They render differently (GPU vs CPU), but their relationship to the PTY, the shell, and the kernel is identical to every other terminal emulator on this page. |

---

## 9. Try this — verify it yourself

> **Try this**
> ```
> echo $TERM
> ```
> Prints the terminal-type identity string your shell believes it's running under — set by the terminal emulator when it launched your shell. Different emulators (and SSH sessions) may set this differently (`xterm-256color`, `screen-256color` inside tmux, etc.) — file 24 explains what this value is actually used for.

> **Try this**
> ```
> pstree -p $$
> ```
> `$$` is your shell's own PID (mechanics in file 04). `pstree` walks the process tree upward and shows you the real parent-child chain on your specific machine — you should see your terminal emulator's process sitting directly above your shell, exactly matching Section 6's diagram (modulo the server/client wrinkle noted above for some emulators).

> **Try this**
> Open two tabs (or two windows) in your terminal emulator and run `echo $$` in each. You'll get two different PIDs — two genuinely separate shell processes — both children of the same terminal-emulator process. One emulator, multiple independent shell sessions, each an ordinary, fully independent operating-system process.

---

## 10. macOS note

Terminal.app and iTerm2 fill exactly the role described in this entire file — no conceptual difference. macOS's PTY implementation is BSD-derived (part of the XNU/Darwin lineage covered in file 43) and behaves closely enough to Linux's that essentially everything above transfers directly. Where real differences start to appear is deeper in the stack — specific `ioctl`s, some line-discipline details, and process-inspection tooling (`ps`/`lsof` flag differences) — all deferred to the dedicated comparison in `43_MACOS_COMPARISON.md`.

---

## 11. Summary

- **Terminal**, **console**, **teletype**, **TTY**, and **terminal emulator** are five historically distinct things that happen to get used as loose synonyms today.
- Physical terminals were real hardware, connected by wire to a distant, shared computer. That hardware is essentially gone from ordinary desktop use.
- A **terminal emulator** is software that convincingly imitates the old hardware's *interface contract* so that decades of terminal-assuming software never had to change.
- Every terminal emulator you've heard of — GNOME Terminal, Konsole, Alacritty, Kitty, iTerm2, Terminal.app — does the identical job architecturally: obtain a PTY, launch a shell as a child connected to it, render what comes out, forward keystrokes in.
- **The terminal emulator is not the shell.** They are two separate processes, one the parent of the other, communicating only through the narrow channel of a PTY.

---

## What's next

`02_TTY_AND_PTY.md` opens up exactly the mechanism this file kept deferring: what a PTY actually is, how the kernel implements the master/slave split, and the full keystroke-to-screen data-flow trace this file's diagrams only sketched the outline of.
