# 18 — Shell Startup Files

> **Where this fits in the stack**
> ```
> User  →  Terminal Emulator  →  PTY/TTY  →  Shell  →  [ STARTUP FILES ]  →  Environment
>                                                        ^^^^^^^^^^^^^^^^
>                                                        this file
> ```
> File 17 covered *what* environment variables are. This file covers *where they actually get set* — the full Bash and Zsh startup-file sets, and the two-axis login/interactive classification that determines exactly which files run for any given shell invocation.

---

## 1. Login shell vs interactive shell — two independent axes, not one spectrum

**Login shell:** invoked as the first process of a user's session — by `login`, `getty`, `sshd`, or a terminal emulator explicitly configured to start one. Recognizable by `argv[0]` beginning with a `-` (e.g. `-bash`), or by the explicit `--login`/`-l` flag.

**Interactive shell:** one reading commands from a terminal and presenting a prompt, as opposed to running a script non-interactively. Determined by whether stdin/stdout are connected to a terminal and no script/command argument was given.

These combine into **four** genuine possibilities:

| | Login | Non-login |
|---|---|---|
| **Interactive** | Logging into a physical console; plain `ssh user@host` | Opening a new terminal-emulator window/tab (the common case on Linux) |
| **Non-interactive** | `bash --login -c "cmd"` (rare) | Running a script: `./script.sh`, `bash script.sh` |

> **Common mistake**
> "Opening a new terminal window always starts a login shell." Not by default on Linux — most terminal emulators just fork/exec your shell directly as an ordinary non-login interactive session. (macOS's stock Terminal.app is a notable historical exception — it has defaulted to starting login shells for new windows, unlike most Linux terminal emulators — worth knowing if you're moving between the two.)

---

## 2. Bash's startup files

| Shell type | Files read |
|---|---|
| **Login** (interactive or not) | `/etc/profile` always first, then the **first one found** among `~/.bash_profile`, `~/.bash_login`, `~/.profile`, in that priority order — stops at the first match |
| **Interactive, non-login** | `~/.bashrc` (and, on Debian/Ubuntu specifically, `/etc/bash.bashrc` — a distro convention, not Bash's own universal default) |
| **Non-interactive** (a script) | **Neither**, by default — unless the script explicitly sources one, or `BASH_ENV` is set to a file to source for non-interactive shells specifically |

**The standard idiom worth knowing:** since login shells read `~/.bash_profile` and interactive non-login shells read `~/.bashrc` — two *different* files — and most people want the same setup regardless, the conventional fix is to make `~/.bash_profile` simply source `~/.bashrc`:
```bash
# inside ~/.bash_profile
[ -f ~/.bashrc ] && source ~/.bashrc
```

---

## 3. Zsh's startup files

Zsh's set is more elaborate, and — unlike Bash's "stop at the first match" — **multiple files typically all run together** for a given shell type.

| File | Runs when |
|---|---|
| `.zshenv` | **Every** zsh invocation, unconditionally — including fully non-interactive ones (a cron job running a zsh script) |
| `.zprofile` | Login shells, **before** `.zshrc` |
| `.zshrc` | Interactive shells |
| `.zlogin` | Login shells, **after** `.zshrc` |
| `.zlogout` | Login shells, on exit |

**The ordering is intentional**, not arbitrary: `.zprofile` is for things that should be ready *before* interactive setup runs; `.zlogin` is for things meant to run *after* it.

> **Remember**
> `.zshenv` runs for absolutely every invocation, interactive or not — it's the Zsh analog to Bash's `BASH_ENV`, except built-in and automatic rather than opt-in via an environment variable.

---

## 4. The decision table

| Shell type | Bash reads | Zsh reads |
|---|---|---|
| Login + interactive | `/etc/profile`, then first of `.bash_profile`/`.bash_login`/`.profile` | `.zshenv`, `.zprofile`, `.zshrc`, `.zlogin` |
| Non-login + interactive | `.bashrc` (+ `/etc/bash.bashrc` on some distros) | `.zshenv`, `.zshrc` |
| Login + non-interactive | Same as login+interactive (Bash doesn't distinguish further) | `.zshenv`, `.zprofile` |
| Non-login + non-interactive (script) | None, unless `BASH_ENV` is set | `.zshenv` only |

---

## 5. Why `source ~/.zshrc` changes the *current* shell

Recap file 04/07's builtin argument, applied directly: `source` (or `.`) executes the named file's commands **in the current shell process, with no fork** — exactly like `cd`/`export`, this has to be a builtin for the same underlying reason (file 07).

So `source ~/.zshrc` after editing it doesn't launch anything new — it re-runs every line in that file (variable assignments, aliases, function definitions, `PATH` changes) directly against your **already-running** shell's own state, updating it in place. Running plain `zsh` instead would *also* pick up the edited file, but that leaves you inside a **new, nested** shell — a child of the one you were already in — rather than refreshing the original.

---

## 6. `$SHELL` vs the shell you're actually running right now

Expanding the flag from files 03/05/17: `$SHELL` is set **once, at login time** — by `login`, `sshd`, or a display manager's session setup — reading the login-shell field for your user from `/etc/passwd`. It does **not** update just because you later, manually, launch a different shell interactively.

> **Try this**
> ```bash
> # assuming your login shell is bash:
> echo $SHELL          # /bin/bash
> zsh                   # start a NESTED zsh, interactively, manually
> echo $SHELL           # STILL /bin/bash — unchanged
> ps -p $$ -o comm=      # correctly reports zsh — what's ACTUALLY running now
> ```

**Practical relevance:** tools and scripts that want to know "what shell should I use here" frequently default to checking `$SHELL` — which can be actively misleading in exactly this nested-shell scenario, since it reports your *login* shell, never your *current* one.

---

## 7. Common misconceptions

| Misconception | Reality |
|---|---|
| "Login and interactive are the same axis." | Independent — four real combinations, not two. |
| "A new terminal window is always a login shell." | Not by default on Linux; macOS's Terminal.app is a notable exception. |
| "`$SHELL` always tells me what shell I'm currently in." | It's your login shell specifically, set once, which can diverge from what's actually running. |
| "`source` starts a new shell to run the file." | The opposite — zero forking, executes directly in the current process. |

---

## 8. Try this

> **Try this**
> ```
> echo $0
> ```
> For an interactive shell, typically its own name — with a leading `-` specifically if it's a **login** shell (`-bash` vs `bash`), the exact `argv[0]` convention from Section 1.

> **Try this**
> Temporarily add `echo "sourced!"` to your `~/.bashrc` (or `~/.zshrc`), then compare opening a plain new terminal tab against running `bash --login` explicitly — see which one actually triggers it, confirming Section 1's table on your own machine.

---

## 9. Summary

- Login and interactive are two independent yes/no axes, giving four real shell-invocation types.
- Bash stops at the first matching login-shell file; Zsh runs multiple files, in a deliberate order, for a given invocation type.
- `source` re-runs a file's commands in your current, already-running shell process — no new process, no new layer.
- `$SHELL` is fixed at login and can silently diverge from whatever shell you're actually sitting inside right now.

---

## What's next

`19_FILESYSTEM_AND_VFS.md` moves from shell configuration to what `ls` was actually doing when it ran in file 07's trace: the VFS abstraction, inodes, directory entries, and why a directory is a file, not a fundamentally different kind of object.
