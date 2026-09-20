# 04 — Bash

> **Where this fits in the stack**
> ```
> User  →  Terminal Emulator  →  PTY/TTY  →  [ BASH ]  →  Programs  →  Syscalls  →  Kernel  →  Hardware
>                                             ^^^^^^^^
>                                             this file
> ```
> File 03 gave you the shell landscape in the abstract. This file takes Bash specifically — the one you'll actually spend your time in — and goes deep on its language, with one rule enforced throughout: **every construct gets tied back to what it actually costs or does at the process level.** This is not a scripting tutorial. If you want the mechanics of redirection, pipelines, quoting, globbing, jobs, signals, or exit status in full depth, those have their own files (12, 13, 06, 15, 14, 42) — this file covers what's genuinely Bash's own.

---

## 1. Bash is one process, with state living in its memory

Everything in this file — variables, functions, aliases, the current directory, the job table — is bookkeeping inside **one long-lived process's memory**. That single fact explains an enormous amount of Bash's behavior that otherwise looks arbitrary, starting immediately with variables.

---

## 2. Variables: shell variable vs environment variable

```bash
x=5              # a SHELL variable — bookkeeping inside this bash process, nothing more
export x=5       # an ENVIRONMENT variable — additionally flagged for inclusion in
                 # envp[] the next time this shell calls execve() for a child
```

This is not a stylistic distinction — it's the difference between "Bash remembers this" and "Bash will hand this to any program it launches." Prove it to yourself:

> **Try this**
> ```bash
> x=5
> bash -c 'echo $x'        # prints nothing — the new bash process never received x
> 
> export x=5
> bash -c 'echo $x'        # prints 5 — x was in envp[] this time
> ```
> `bash -c '...'` is itself an ordinary external command from your current shell's point of view — it gets `fork`+`execve`'d like anything else (file 08). The only way information crosses that boundary is through the `envp[]` array `execve` receives, and `export` is the sole mechanism that puts a shell variable into it. Full depth on this boundary: file 17.

> **Interview answer**
> "A shell variable exists only inside the shell process that defined it. An environment variable is a shell variable additionally marked with `export`, which causes it to be copied into the `envp[]` array handed to every subsequent child process via `execve`. Ordinary variables are never automatically inherited — `export` is what makes inheritance happen."

---

## 3. Quoting and globbing — just enough to read the rest of this file

`"double quotes"` suppress word-splitting and globbing but still allow `$variable`/`$(...)` expansion. `'single quotes'` suppress everything, including variable expansion. Unquoted `*`/`?`/`[...]` are glob patterns, expanded by Bash against actual filenames *before* the command ever runs. That's the minimum you need for the examples ahead — the full tokenization order, escaping rules, and every expansion type get their complete treatment in `06_SHELL_PARSING.md`.

---

## 4. Command substitution vs arithmetic expansion — the fork line drawn precisely

These look superficially similar and behave completely differently at the process level.

```bash
$(command)          # COMMAND SUBSTITUTION — forks a subshell to run `command`,
                     # captures its stdout, substitutes that text in
`command`            # the older, harder-to-nest equivalent syntax
$(( expression ))    # ARITHMETIC EXPANSION — evaluated entirely INSIDE this
                     # bash process's own integer-arithmetic engine — no fork
```

> **Try this — proving the fork exists, correctly**
> ```bash
> echo $$                   # your shell's PID, e.g. 4021
> echo $BASHPID              # the SAME number at top level
> 
> echo $(echo $$)             # STILL 4021 — see the callout below
> echo $(echo $BASHPID)       # a DIFFERENT number, e.g. 4055 — the subshell's REAL pid
> ```
> `$(( 2 + 2 ))` by contrast never produces a second PID at all under any variable — there's no subprocess to have one.

> **Common mistake**
> "`$$` inside `$(...)` proves there's no fork, since it doesn't change." The opposite is true — there *is* a fork, but `$$` is a deliberately special case: Bash caches it from the top-level shell's startup and every subshell continues reporting that cached value, preserving traditional Bourne-shell behavior. `$BASHPID` (Bash-specific, not POSIX) always reports the *actual* current process's real PID via `getpid()`, and that's what changes inside a subshell. If you only ever check `$$`, you will conclude — wrongly — that command substitution doesn't fork.

---

## 5. Subshells — what `(...)` actually is

```bash
( commands )      # forks a NEW bash process to run `commands` in
{ commands; }     # runs in the CURRENT process — no fork
                  # (note the required space after `{`, and the `;` or
                  #  newline before `}` — both are real syntax, not style)
```

This is the exact mechanism behind the canonical example from your own original brief:

```bash
(cd /tmp; pwd)
pwd
```

The `(...)` forks a child bash process. `cd` inside it changes *that child's* working directory — a per-process attribute (Linux's `fs_struct`, given to the child as its own independent copy at `fork()`, not shared with the parent). The child prints `/tmp`, then exits. The parent shell's own working directory was never touched, because it was never the same process. The second `pwd`, running in the parent, prints wherever you started.

| | `( commands )` | `{ commands; }` | `$(commands)` |
|---|---|---|---|
| Forks a process? | Yes | No | Yes |
| Variable/`cd` changes visible after? | No | Yes | No (output is captured, not effects) |
| Typical use | Isolate side effects | Group commands, e.g. for one redirection | Capture output as text |

> **Interview answer**
> "`(commands)` runs in a forked subshell — a genuinely separate process — so any `cd` or variable assignment inside it dies with that process and never affects the parent shell. `{ commands; }` runs in the current shell process with no fork, so its effects persist. This is precisely why `(cd /tmp; pwd); pwd` leaves you in your original directory."

---

## 6. The four things Bash can run, and how each differs at the process level

| | Where it lives | Runs in a new process? |
|---|---|---|
| **Function** | Defined in Bash's own memory | No — runs in the current process (though it may itself launch external commands, which *do* fork) |
| **Alias** | A pure text substitution table, expanded *before parsing proceeds* | N/A — it's not "run," it's rewritten into other text first |
| **Builtin** | Implemented inside the `bash` binary itself | No — executes directly in the current process |
| **External command** | A real file on disk, found via `PATH` | Yes — `fork` + `execve` |

**Bash's actual lookup order**, precisely: alias expansion and reserved-word (keyword) recognition happen at the *parsing* stage, before "command lookup" in the execution sense even begins. For what remains as a simple command to execute, Bash checks: does the name contain a `/` (then it's a direct path, no search)? Otherwise — is it a shell **function**? Then a **builtin**? Only then does it fall through to a **`PATH` search** for an external executable. File 07 restates this precisely when covering `PATH` lookup and command execution end to end.

> **Common mistake**
> "An alias is basically a tiny function." Not quite — an alias is pure *text substitution*, applied before the shell has even finished parsing the line, with no real parameter handling of its own (anything after the aliased word is just appended text, not passed arguments in a function-call sense). It's also, by default, **not expanded inside non-interactive scripts** unless `shopt -s expand_aliases` is set — a genuinely common source of "this alias works at my prompt but not in my script" confusion.

> **Try this**
> ```bash
> type cd        # cd is a shell builtin
> type ls        # ls is /usr/bin/ls (or similar) — an external command
> type test       # test is a shell builtin
> type echo       # echo is a shell builtin
> ```
> `type` tells you exactly which of the four categories above a given name resolves to on your system right now — the single fastest way to answer "will this fork or not?"

---

## 7. `exec` — the builtin, not to be confused with the `execve` syscall

These share a name and are genuinely different things, and mixing them up is a real, recurring interview stumble.

- **`execve()`** (file 09) is the **system call** that replaces a process's image with a new program, in place, keeping the same PID.
- **`exec`** is a **Bash builtin** that calls `execve` **on behalf of the current shell process itself** — with no fork first.

```bash
exec ls          # the CURRENT bash process becomes ls, right now, in place
                 # when ls finishes, there is no shell left — nothing to return to
```

Compare this to ordinary execution: typing plain `ls` makes Bash **fork** a child first, and only the *child* execs into `ls` — the parent Bash process survives, waits, and shows you another prompt. `exec ls` skips the fork entirely, so Bash itself ceases to exist the moment `ls`'s image is loaded, and the terminal (or script) ends when `ls` does.

**A second, very common form** — `exec` used only for redirection, with no command:

```bash
exec > logfile.txt      # redirects the CURRENT shell's stdout, permanently,
                        # for the rest of this script/session
```

No program name is given here, so there's nothing to replace the process image with — this form applies the redirection to the shell itself rather than to some subsequently launched child. You'll see this at the top of logging-heavy scripts specifically because it means *every* command from that point on inherits the redirected stdout, without redirecting each one individually.

> **Interview answer**
> "`execve` is the kernel system call that replaces a process's image in place. Bash's `exec` builtin calls it on the shell's own process, without forking first — so `exec someprogram` doesn't create a new process, it *turns the current shell into* that program, and the shell is gone once it finishes. Used with only a redirection and no command, `exec` instead permanently redirects the current shell's own file descriptors."

---

## 8. Conditionals, loops, `case` — tied back to what actually executes

```bash
if [ "$x" = "5" ]; then ...; fi          # [ is TEST — a builtin in Bash
if [[ $x == 5* ]]; then ...; fi          # [[ is a KEYWORD, parsed specially — never external
if (( x == 5 )); then ...; fi            # (( )) — arithmetic, evaluated internally, never external
for i in a b c; do ...; done
while read -r line; do ...; done < file
case "$x" in
    5) ... ;;
    *) ... ;;
esac
```

> **Common mistake**
> "`[[ ]]` is faster than `[ ]` because `[ ]` forks a new process." Not in Bash — `[` and `test` are **builtins** in Bash (verify with `type '['`), so plain `[ ... ]` doesn't fork here either. The real, practical advantages of `[[ ]]` are **safety and expressiveness**: it doesn't word-split or glob-expand unquoted variables inside it (a common source of `[ ]` bugs when a variable is empty or contains spaces), and it supports pattern matching and `&&`/`||` directly without escaping. The fork-avoidance claim is a genuinely widespread myth — worth being precise about, since it's repeated constantly online. (The claim *would* be true in a minimal shell where `test`/`[` are only available as the external `/usr/bin/[` binary — which coreutils does ship, for exactly that kind of environment.)

---

## 9. Arrays — Bash-specific, not POSIX (recap from file 03)

```bash
arr=(a b c)
echo "${arr[0]}"        # a
echo "${arr[@]}"        # a b c
echo "${#arr[@]}"        # 3

declare -A map           # associative array (Bash 4+)
map[key]=value
echo "${map[key]}"
```

No portable equivalent exists in POSIX `sh` — a script relying on either form of array will not run under `dash` or any other strict POSIX shell.

---

## 10. Parameter expansion — the `${...}` family, as reference

| Form | Meaning |
|---|---|
| `${var}` | Plain expansion (braces disambiguate from surrounding text) |
| `${var:-default}` | Use `default` if `var` is unset or empty, without changing `var` |
| `${var:=default}` | Same, but also **assigns** `default` to `var` |
| `${var:+alt}` | Use `alt` if `var` **is** set and non-empty, else nothing |
| `${var:?message}` | Print `message` and exit if `var` is unset or empty |
| `${#var}` | Length of `var`'s value |
| `${var#pattern}` / `${var##pattern}` | Remove shortest / longest matching **prefix** |
| `${var%pattern}` / `${var%%pattern}` | Remove shortest / longest matching **suffix** |
| `${var/pattern/repl}` | Replace first match of `pattern` with `repl` |
| `${var//pattern/repl}` | Replace **all** matches |

All of this is evaluated entirely inside the current Bash process — no forking involved anywhere in this table.

---

## 11. `trap` — Bash's hook into signal delivery (preview — full depth: file 14)

```bash
trap 'echo caught SIGINT' INT
```

This registers a shell command to run when the shell receives the named signal — Bash's user-facing entry point into the exact kernel signal-delivery mechanism file 14 covers from the ground up. Press `Ctrl+C` after running the line above and you'll see your handler fire instead of the shell's default `SIGINT` behavior.

---

## 12. Builtins — a concrete, non-exhaustive list

`cd`, `export`, `unset`, `alias`, `source` (or `.`), `read`, `pwd`, `echo`, `printf`, `exit`, `trap`, `wait`, `jobs`, `fg`, `bg`, `kill`, `set`, `shopt`, `declare`, `local`, `return`.

> **Try this**
> ```bash
> help                # Bash's own builtin listing every builtin it has
> man bash             # the BUILTIN COMMANDS section is the authoritative reference
> ```

---

## 13. Common misconceptions

| Misconception | Reality |
|---|---|
| "Aliases are lightweight functions." | Pure text substitution at parse time, no real argument handling, and disabled by default in non-interactive scripts. |
| "`[[ ]]` avoids a process fork that `[ ]` doesn't." | Both are builtins in Bash; neither forks. `[[ ]]`'s real advantages are safer unquoted-variable handling and pattern matching. |
| "A subshell shares variables with its parent — it's just for grouping." | A subshell is a genuinely separate forked process; variable and directory changes inside it vanish when it exits. |
| "`$$` proves whether a fork happened." | `$$` is deliberately cached from the top-level shell for compatibility and never changes in subshells — use `$BASHPID` to see the real, current PID. |
| "`exec ls` runs `ls` like any other command, just with different syntax." | It replaces the *current shell process itself* with `ls` — no fork, and the shell is gone afterward. |

---

## 14. macOS note

The `bash` Apple ships (frozen at the old GPLv2-licensed 3.2 branch — recap from file 03) **lacks associative arrays entirely** (`declare -A` doesn't exist in Bash 3.2) and is missing over a decade of other upstream features. If you're following Bash-specific examples from this handbook on stock macOS and something doesn't exist, check `bash --version` first — installing a current Bash via Homebrew (and often putting it on your `PATH`, or in `/etc/shells`, ahead of the system one) is the usual fix.

---

## 15. Summary

- Bash's state — variables, functions, aliases, current directory, jobs — lives in one process's memory; `export` is the only bridge that copies a variable into `envp[]` for children.
- `$(...)` forks a subshell to capture output; `$(( ))` never forks — it's internal arithmetic.
- `(...)` forks; `{ ...; }` doesn't — which is the entire explanation for why `(cd /tmp; pwd)` doesn't change your shell's directory.
- Functions and builtins run in the current process; external commands always fork+exec.
- `exec someprogram` replaces the current shell in place, with no fork — different from both ordinary execution and from `execve` the syscall, though built directly on top of it.
- `[[ ]]`'s advantages over `[ ]` in Bash are safety and expressiveness, not fork-avoidance — both are builtins here.

---

## What's next

`05_ZSH.md` takes everything in this file as the baseline and asks: what does Zsh do differently, why was it created, and where do the two genuinely diverge rather than just spell things differently?