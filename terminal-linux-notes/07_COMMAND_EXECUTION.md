# 07 — Command Execution

> **Where this fits in the stack**
> ```
> User  →  Terminal Emulator  →  PTY/TTY  →  Shell  →  [ EXECUTION ]  →  Kernel  →  Hardware
>                                                       ^^^^^^^^^^^^^
>                                                       this file
> ```
> This is the synthesis chapter. Files 01-06 each explained one layer in isolation; this file runs one real command — `ls -lah /tmp` — straight through every one of those layers as a single continuous trace, then goes deep on two things the trace glosses over: how the shell finds `ls` on disk at all, and why some commands (`cd`, `export`) can never be found on disk in the first place.

---

## 1. The complete trace: `ls -lah /tmp`

Each step names **which component is responsible** and points to wherever that component gets its full treatment.

**1. Keyboard input** — Physical hardware generates a scancode; the kernel's input subsystem turns it into an event. *(File 01.)*

**2. Terminal emulator** — Receives the key event from the display server, converts it to the bytes `l`, `s`, ` `, `-`, `l`, `a`, `h`, ` `, `/`, `t`, `m`, `p`, `\n`, and `write()`s them to the PTY master. *(File 01.)*

**3. PTY** — The kernel's line discipline, attached to this master/slave pair, receives the bytes on the input side. *(File 02.)*

**4. Shell receives characters** — In canonical mode, the line discipline buffers and locally echoes each character (you see them appear on screen *here*, before the shell has read anything), then hands the complete line to the shell's blocked `read()` call on fd 0 once `\n` arrives. *(File 02.)*

**5. Shell parses command** — Bash's parser begins working on the line `ls -lah /tmp`. *(File 06.)*

**6. Tokenization** — Phase 1 of the parsing pipeline: three words, `ls`, `-lah`, `/tmp` — no quotes here to track, so this is the simplest case. *(File 06.)*

**7. Quote handling** — Nothing to do; no quote characters are present in this particular command. *(File 06 — the mechanism that *would* apply if there were.)*

**8. Expansion** — Phase 2 of the pipeline runs on each word: no braces, no `~`, no `$`, no glob characters. Every sub-phase finds nothing to expand, and all three words pass through unchanged. *(File 06.)*

**9. Command lookup** — The shell now has to decide what `ls` actually refers to: an alias? A function? A builtin? Only if none of those match does it become a `PATH` search. *(Section 2 of this file.)*

**10. `PATH` search** — `ls` isn't a builtin, so the shell searches its `PATH` directories in order for an executable file named `ls`. *(Section 2.)*

**11. Builtin vs external decision** — Having found `/bin/ls` (or wherever it lives on your system) as a real external file, the shell now knows this requires launching a new process — unlike, say, `cd`, which never gets this far because it's resolved as a builtin at step 9. *(Section 3.)*

**12. `fork()`** — The shell process calls `fork()`. A new child process is created, an almost-exact copy of the shell, sharing the shell's file descriptors (including fd 0/1/2, all still pointing at the PTY slave). *(Full mechanics: file 08.)*

**13. `execve()`** — In the child only, the shell's code calls `execve("/bin/ls", ["ls", "-lah", "/tmp"], envp)`. *(File 08, file 09.)*

**14. Process image replacement** — The child's entire process image — code, data, heap, stack — is discarded and replaced with `ls`'s. The PID stays the same; everything else about what's running under that PID changes completely. *(File 08.)*

**15. Loader** — The kernel reads `/bin/ls`'s ELF header, maps its code and data segments into the new address space, sets up the initial stack with `argv`/`envp`. *(File 08.)*

**16. Dynamic linker** — Before `ls`'s own `main()` runs, `ld.so` resolves `ls`'s shared library dependencies (`libc.so` and others) and completes the necessary relocations. *(File 08.)*

**17. Program starts** — `ls`'s `main()` begins executing, now running as the process that used to be a copy of the shell.

**18. System calls** — `ls` calls `openat()` on `/tmp`, then `getdents64()` repeatedly to read directory entries, then `stat()`/`lstat()` on each entry to get the size/permission/timestamp data `-lah` asked for. *(File 09 for the syscall mechanism, file 19 for what's happening filesystem-side.)*

**19. stdout** — `ls` formats its output and `write()`s it to fd 1 — still, at this point, the inherited PTY slave. *(File 11.)*

**20. Terminal** — Those bytes flow back through the PTY to the master, where the terminal emulator reads and renders them. *(File 02, and file 24 for how `ls --color` escape sequences specifically get interpreted.)*

**21. Shell waits** — The entire time steps 12-20 were happening, the **parent shell process** was blocked inside `waitpid()`, doing nothing else. *(File 08.)*

**22. Process exits** — `ls` finishes, returns from `main()`, and (via glibc's startup wrapper) this becomes an `exit()` call, tearing the process down and leaving behind an exit status.

**23. `wait`/`waitpid`** — The shell's blocked `waitpid()` call wakes up, retrieving that exit status into `$?`. *(File 08, and file 42 for `$?` mechanics.)*

**24. Shell displays prompt** — The shell writes its next (expanded) prompt to fd 1 — the same fd 1 → PTY slave → master → terminal emulator path as step 19 — and loops back to step 1, waiting for your next keystroke.

> **Remember**
> Notice how much of this list is either "the shell doing something in its own process" (5-11, 21, 23-24) or "a completely different process doing something after step 12" (13-20, 22). Nothing about `ls`'s execution ever runs *inside* the shell — the shell only launches it, waits, and picks back up afterward.

---

## 2. Command lookup and `PATH` search, in full

### What `PATH` actually is

A single environment variable holding a **colon-separated list of directories**, searched **in the given order**, stopping at the **first match**:

```
$ echo $PATH
/home/alice/bin:/usr/local/bin:/usr/bin:/bin
```

When you type `python` and it's not a builtin, alias, or function, the shell walks this list left to right — `/home/alice/bin/python`? `/usr/local/bin/python`? `/usr/bin/python`? — and executes the **first** one it finds that exists and is executable. It does not keep searching for "better" matches further down the list, and it does not care whether multiple matches exist.

### `which`, `type`, `command -v`, and why they can disagree

| Tool | What it is | What it actually knows about |
|---|---|---|
| `which` | An **external program** | Only searches `PATH` — has no visibility into your current shell's aliases, functions, or builtins |
| `type` | A **shell builtin** | Full visibility into the invoking shell's own state — aliases, functions, builtins, *and* the hashed `PATH` location, in the real resolution order from file 04 |
| `command -v` | A **POSIX-standardized builtin** | Same shell-state visibility as `type`, but with output specifically designed to be portable/scriptable across POSIX-compliant shells |

> **Common mistake**
> "`which` and `type` always agree — they're just two ways of asking the same question." They can genuinely diverge: define `alias ls='ls --color=auto'` and then compare `which ls` (reports the external binary's path, with no idea an alias exists) against `type ls` (correctly reports the alias, because it's asking the shell itself, not searching the filesystem independently). `which` answers "what's on `PATH`"; `type` answers "what would actually run right now, in this shell."

### Command hashing

Bash caches the full resolved path of external commands it has already looked up once, in an in-memory table, per shell session — so running the same command repeatedly doesn't re-scan every `PATH` directory each time.

```bash
hash            # show the current cache
hash -r         # clear it entirely
hash -d ls      # remove just this one cached entry
```

This caching is the exact explanation behind a genuinely common confusion: *"I just installed a newer version of a program earlier in my `PATH`, but my shell keeps running the old one."* The shell's hash table still points at the old, previously-found location and has no reason to re-search unless told to — `hash -r` (or simply starting a new shell session) is the fix.

### The conventional directories

| Directory | Conventional meaning |
|---|---|
| `/bin` | Core system binaries — historically needed even for single-user/recovery boot, before `/usr` might be mounted. On many modern distros, `/bin` is now just a symlink to `/usr/bin` (the "usrmerge" change), but the historical distinction is why the name exists at all. |
| `/usr/bin` | The bulk of ordinary, distro-package-manager-installed programs. |
| `/usr/local/bin` | Software installed **outside** the distro's package manager — compiled from source, or installed by some tool that isn't `apt`/`dnf`/etc. Conventionally takes precedence in `PATH` over `/usr/bin`, specifically so a locally-built version can override the distro's. |
| `~/bin` (or `~/.local/bin`) | A user's own personal scripts and binaries — not shared system-wide, added to `PATH` via shell startup files (file 18). |

### Why changing `PATH` can completely change what runs — including as a security concern

Because the search stops at the *first* match, whatever directory you put earliest in `PATH` effectively **shadows** every same-named program in every directory listed after it. This cuts both ways:

- **Useful:** put `~/bin` first, and your own custom `ls` script (however unwise) or a personal `python` symlink to a specific version takes precedence over the system one, with zero configuration beyond `PATH` order.
- **A genuine security risk:** if an attacker can place an executable named `ls` (or `sudo`, or anything else you'd routinely run) into a directory that appears *earlier* in your `PATH` than the legitimate one — a writable temp directory that somehow ended up on your `PATH`, for instance — every subsequent invocation of that name runs their code instead, silently. This is precisely why security guidance universally recommends never including a writable-by-others or relative (`.`) entry early in `PATH`.

---

## 3. Why `cd` cannot be an external program — the process-hierarchy argument

This is the direct answer to a question you can't fully appreciate until you've internalized `fork()`'s per-process independence from file 04 and the preview above.

**The core fact:** every process has its own, independent current-working-directory attribute, tracked in the kernel's per-process state. When a process forks, the child starts with a *copy* of the parent's cwd — but from that instant on, the child calling `chdir()` on itself has **zero effect on the parent**. They are now two separate processes with two separate, independently-mutable cwd values.

**Now suppose `/usr/bin/cd` existed and you ran `cd /tmp`:**

```
Shell (cwd: /home/alice)
   |
   fork()
   |
   +-------------------------+
   |                         |
Shell (cwd: /home/alice)   Child
   | (unchanged!)             |
   | waits...                execve("/usr/bin/cd", ["cd","/tmp"])
   |                         |
   |                        chdir("/tmp")   <- changes the CHILD's cwd to /tmp
   |                         |
   |                        exits           <- child, and its /tmp cwd, are destroyed
   |
   | (still /home/alice — nothing ever touched it)
```

The child successfully changes **its own** working directory to `/tmp` — and then immediately exits, taking that changed state down with it. The shell you're actually typing into, the one whose prompt you're watching, never called `chdir()` at all. You'd see no error, and your prompt would still show `/home/alice`. A hypothetical `/usr/bin/cd` would be, precisely, useless.

**The only way `cd` can have any effect at all is for `chdir()` to execute inside the shell's own, persistent process** — which means `cd` cannot be launched via `fork`+`exec`. It must be a **builtin**: a function call inside the running shell's own code, with no new process ever created.

### The same argument, applied to the rest of file 04's builtin list

| Command | Must be a builtin, because... |
|---|---|
| `cd` | Must call `chdir()` on the shell's own process |
| `export` | Must mark a variable, in the shell's own memory, for inclusion in future `envp[]` |
| `unset` | Must remove a variable from the shell's own memory |
| `alias` | Must modify the shell's own alias table |
| `source` (`.`) | Must run commands *in the current shell's process* — that's the entire point of sourcing rather than executing; if it forked, variable/function/directory changes would vanish exactly like the `cd` example |
| `read` | Must place input into variables that only exist in the current shell's own memory |

### `pwd` is the instructive exception

`pwd` only needs to **report** the current directory, never change it — and a freshly forked child correctly inherits an accurate cwd from its parent at the moment of `fork()`, before anything else happens. So a real `/bin/pwd` genuinely exists on your system (verify with `which pwd`), and it works completely correctly as an external program. Bash *also* provides `pwd` as a builtin — but purely as a speed optimization (skipping the `fork`+`exec` overhead for a trivially cheap operation), not because correctness demands it the way it does for `cd`.

> **Interview answer**
> "`cd` must be a shell builtin because the working directory is a per-process attribute. If `cd` forked a child to run `chdir()`, only that child's directory would change, and it would vanish the instant the child exited — the invoking shell's own directory would never be affected. Any command that needs to modify the current shell's own persistent state — `cd`, `export`, `unset`, `alias`, `source`, `read` — has this same constraint. `pwd`, by contrast, only reports state rather than changing it, so it works fine as either a builtin or a genuinely external program — and both exist."

---

## 4. Common misconceptions

| Misconception | Reality |
|---|---|
| "Every command you can run is a file on disk somewhere." | Builtins (`cd`, `export`, `[[`) exist only as code inside the shell binary — no corresponding executable file backs them, and for several of them (Section 3), none *could*. |
| "`which` and `type` are interchangeable." | `which` only knows `PATH`; `type` knows your actual shell's aliases/functions/builtins too, and answers the more useful question. |
| "PATH order is just a convention with no real consequence." | It determines exactly which program runs when names collide, and mismanaging it (writable/relative entries early) is a genuine security hole. |
| "`pwd` is a builtin for the same reason `cd` is." | `cd` *must* be — the whole mechanism breaks otherwise. `pwd` is a builtin purely for speed; a fully working external version exists and is used all the time (e.g., by other programs shelling out to it). |

---

## 5. Try this

> **Try this**
> ```
> echo $PATH | tr ':' '\n'
> ```
> Your actual search order, one directory per line.

> **Try this**
> ```bash
> alias ll='ls -la'
> which ll        # likely reports "no ll" or nothing useful — which doesn't see aliases
> type ll          # correctly reports the alias definition
> type -a ls        # shows EVERY place "ls" resolves — alias, function, and/or external binary,
>                   # in the actual order Bash would try them
> ```

---

## 6. macOS note

The same `PATH`-search, hashing, and builtin-vs-external mechanics apply identically on macOS. The one thing that trips people up in practice: Homebrew-installed software on Apple Silicon lives under `/opt/homebrew/bin`, which has to be added to `PATH` (usually by Homebrew's own installer editing a shell startup file — file 18) — forgetting this, or having it added in the wrong order relative to `/usr/bin`, is the macOS-specific version of the "PATH order changes what runs" lesson from Section 2.

---

## 7. Summary

- The 24-step trace is really three acts: the shell reading and parsing your line (files 02, 06), the shell deciding what to do about it and — if needed — launching a child process (this file, file 08), and the resulting output flowing back through the exact same PTY path it came in on.
- `PATH` search stops at the first match, in listed order — which is both a convenience (personal overrides) and a real security surface (hijackable if the wrong directory comes first).
- `which` only knows `PATH`; `type`/`command -v` know your actual shell's full resolution state and are the more reliable tools for "what will this name actually run."
- `cd`, `export`, `unset`, `alias`, `source`, and `read` are builtins **because they must be** — they need to mutate the current shell process's own state, and a forked child's changes die with it. `pwd` is a builtin purely for speed; a working external version genuinely exists.

---

## What's next

`08_PROCESSES_FORK_EXEC.md` goes deep on exactly the steps this file could only preview: `fork()`, `execve()`, what's copied versus shared, the ELF loader, the dynamic linker, and `wait`/`waitpid` — full syscall signatures and C code.
