# 17 — Environment and PATH

> **Where this fits in the stack**
> ```
> User  →  Terminal Emulator  →  PTY/TTY  →  Shell  →  Processes  →  [ ENVIRONMENT ]  →  execve()
>                                                                     ^^^^^^^^^^^^^^^
>                                                                     this file
> ```
> File 04 showed you that `export x=5` makes a child process able to see `$x`, and 08/09 named `envp[]` as one of `execve`'s three parameters without opening it up. This file makes the whole mechanism precise: what `environ` actually is in a running process's own memory, exactly how `export` bridges into it, and the standard variable list every shell session relies on.

---

## 1. `environ` — a process's own private copy

Every running process has its own in-memory representation of its environment: a **NULL-terminated array of strings**, each of the form `"NAME=value"`, pointed to by a global variable named `environ` (declared in `<unistd.h>`; accessible from C as `extern char **environ;`).

> **Remember**
> This array is **private, per-process memory**. Modifying your own process's `environ` — via `setenv()`/`putenv()`/`unsetenv()` in C, or `export`/`unset` in a shell — has **zero effect** on any other already-running process, including your own parent. It only changes what *your* process currently holds, and therefore what *your* process will pass along the next time it creates a child.

---

## 2. `argv` and `envp` — the two arrays `execve()` hands to a new program

```c
int execve(const char *pathname, char *const argv[], char *const envp[]);
```

- **`argv[]`** — the argument vector. `argv[0]` is *conventionally* the program's own name, but it's really just whatever string the caller chose to put there — programs can, and occasionally do, lie about their own `argv[0]`. NULL-terminated.
- **`envp[]`** — the environment vector: NULL-terminated `"NAME=value"` strings, which becomes the **new** program's initial `environ`.

`main()` can technically be declared `int main(int argc, char *argv[], char *envp[])`, receiving this exact `envp[]` array directly as a third parameter — valid and widely supported, though C code more commonly reads the environment via the global `environ` variable or `getenv()` instead.

---

## 3. The standard variables, precisely

| Variable | Precise meaning |
|---|---|
| `PATH` | Colon-separated executable search list (file 07, in full) |
| `HOME` | The current user's home directory — used by argument-less `cd`, `~` expansion (file 06), and as the traditional fallback config-file location |
| `USER` (or `LOGNAME`) | The current user's login name |
| `SHELL` | The user's **login** shell — set once at login from `/etc/passwd`; does **not** update just because you manually launch a different shell interactively (file 03/05's macOS-note gotcha, generalized) |
| `PWD` | The shell's **own** notion of the current directory, maintained as a string *by the shell itself* — see the callout below |
| `OLDPWD` | The previous working directory, updated by `cd`, used by `cd -` |
| `TERM` | Identifies the terminal emulator's escape-sequence dialect, e.g. `xterm-256color` (file 01) |
| `LANG` | Default locale — language, character encoding, date/number formatting — for programs that respect it |
| `EDITOR` (often `VISUAL` too) | Which text editor other programs should invoke on your behalf (`git commit` with no `-m`, `crontab -e`, `visudo`) — a convention, honored voluntarily |

> **Common mistake**
> "`PWD` is always a perfectly live, freshly re-queried reflection of the kernel's actual current directory." It's a **string the shell maintains itself**, kept in sync through its own `cd` bookkeeping — not something re-derived via `getcwd()` on every single reference. In ordinary use these never diverge, but they are, structurally, two different things: one is kernel-tracked per-process state (file 08's `fs_struct`), the other is a shell variable the shell is disciplined about updating.

---

## 4. `export VAR=value`, at the full mechanism level

Recap the demonstration from file 04 — now precisely: `export` marks a shell variable with an internal **export flag**, distinguishing it from ordinary shell bookkeeping variables. The next time this shell process calls `execve()` for *any* child, it walks its variable table and includes **only** the flagged ones when constructing that child's `envp[]`.

---

## 5. Why plain shell variables aren't automatically inherited — the reason, not just the fact

If every shell variable were automatically passed to every child, there would be no way to keep **temporary, internal bookkeeping** — loop counters, scratch values inside a script — from leaking into every single program that script ever runs. Requiring an explicit `export` is a deliberate design choice for cleanliness and predictability, not an oversight.

There's a real security dimension too: not everything a shell knows should necessarily be visible to arbitrary child processes. A script might deliberately compute something into a plain, unexported variable specifically because it doesn't want that value inspectable via a child's environment — worth knowing, because **any process's environment genuinely is inspectable** by anyone with permission to read `/proc/<pid>/environ` (file 33), which is exactly why sensitive values (credentials, tokens) are a poor fit for environment variables in the first place, exported or not.

---

## 6. Process inheritance, generalized — and a sharp contrast with file 10

`fork()` gives the child an **identical copy** of the parent's `environ` at the moment of the call (file 08). From that instant, the two are fully independent — the child changing its own environment afterward has no effect on the parent's, and vice versa.

> **Remember — this is genuinely different from file 10's file descriptor behavior**
> File descriptors are **shared** after `fork()` — both processes' fd-table entries point at the *same* underlying open file description, so changes (like the read offset) are visible to both. **Environment is copied**, not shared — each process gets independent state from the moment of the fork, with no further connection at all. Two different answers to "what does fork() do with X," for two different kinds of X, worth keeping straight.

`execve()` then **replaces** `environ` entirely with whatever `envp[]` was passed (files 08-09). So `fork()` → (optionally modify the child's environment) → `execve()` is exactly how a shell hands a *customized* environment to one specific child, without ever touching its own environment or any sibling process's.

---

## 7. `IFS` — the mechanism behind "word splitting"

File 06 described "word splitting" as an abstract pipeline phase. `IFS` (**Internal Field Separator**) is the actual, inspectable, changeable variable controlling it — default value: space, tab, newline — determining exactly which characters count as separators when an unquoted expansion result gets split into multiple words.

```bash
IFS=,
for x in $(echo "a,b,c"); do echo "[$x]"; done
# [a]  [b]  [c]  — split on commas instead of whitespace
```

`IFS` is a shell variable, typically **not exported** — it only needs to affect the current shell's own parsing, not any child's.

> **Common mistake**
> Changing `IFS` globally, rather than scoping it to one command, silently breaks every other piece of word-splitting-dependent code later in the same script that assumed the default whitespace behavior. The safer pattern scopes the assignment to a single command:
> ```bash
> IFS=, read -ra parts <<< "a,b,c"    # IFS=, applies ONLY to this one command
> echo "${parts[1]}"                    # b
> ```

---

## 8. Common misconceptions

| Misconception | Reality |
|---|---|
| "`environ` is a live view into some kernel-tracked environment." | It's the process's own private, in-memory copy — modifying it affects nothing outside that one process. |
| "Setting a variable in a shell affects its parent shell too." | Environment only flows downward, parent to child, at `fork`/`exec` time — never upward. |
| "fd sharing and environment 'sharing' after `fork()` work the same way." | fds are truly shared (same open file description); environment is copied — independent from the instant of the fork. |
| "`PWD` is always freshly re-derived from the kernel." | It's a string the shell maintains through its own `cd` bookkeeping, not re-queried via `getcwd()` on every reference. |

---

## 9. Try this

> **Try this**
> ```
> env
> declare -x        # bash: lists ONLY exported variables
> declare -p         # bash: lists ALL shell variables, exported or not
> ```
> Compare the two `declare` outputs directly to see the export flag distinction concretely.

> **Try this**
> ```
> cat /proc/$$/environ | tr '\0' '\n'
> ```
> `$$` is your shell's own PID (file 04) — this shows *your shell's* environment exactly as the kernel stores it, note it's **NUL-separated**, not newline-separated, hence the `tr`. (`/proc/self/environ` would instead show this `cat` process's own environment, not your shell's — file 33 explains precisely why.) A direct preview of file 33's `/proc` exploration.

---

## 10. macOS note

`environ`/`argv`/`envp` mechanics and the standard variable conventions (`HOME`, `USER`, `SHELL`, and the rest) are identical on macOS — this file transfers directly.

---

## 11. Summary

- `environ` is a private, per-process, in-memory array of `"NAME=value"` strings — not a live kernel view.
- `execve(path, argv, envp)`'s `envp[]` becomes the new program's initial `environ`; `export` is what makes a shell variable eligible for inclusion in it.
- `fork()` copies environment (independent afterward); `execve()` replaces it entirely — two very different operations from file 10's fd-sharing behavior.
- `IFS` is the real, inspectable variable behind "word splitting," normally left at its default and scoped narrowly when it needs to change.

---

## What's next

`18_SHELL_STARTUP_FILES.md` covers exactly *when and where* variables like these actually get set in the first place — the full Bash and Zsh startup-file sets, and the login-vs-interactive-shell decision table that determines which ones run.