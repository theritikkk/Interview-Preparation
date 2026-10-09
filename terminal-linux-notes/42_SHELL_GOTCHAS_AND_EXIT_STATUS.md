# 42 — Shell Gotchas and Exit Status

> **Where this fits in the stack**
> ```
> Shell language  →  [ the traps ]  →  exit status  →  waitpid()  →  $?
>                     ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
>                     this file
> ```
> This file collects the things that make experienced programmers pause — each one a consequence of a mechanism already explained in an earlier file, shown here as a compact trap plus a tiny example. Exit status gets its own section because it's the thread connecting `exit()`, `waitpid()`, `$?`, `&&`/`||`, and `set -e` into one picture.

---

## 1. Things That Make Experienced Programmers Pause

### Spaces matter, in both directions

```bash
x=5          # assignment
x = 5        # runs a command named "x" with arguments "=" and "5"
[ "$x" = 5 ] # works: [ is a command, so it needs spaces around every token
["$x" = 5]   # fails: bash looks for a command literally named [5 ... 
```
`[` is a command (file 04), not syntax — its arguments need separating like any other command's.

### Unquoted expansions split and glob (file 06)
```bash
f="my file.txt"
rm $f        # rm gets TWO arguments: "my" and "file.txt"
rm "$f"      # one argument — what you meant
```
Word splitting is controlled by `IFS` (file 17); globbing then applies to each resulting word. Quote every expansion by default.

### Command substitution needs quoting too
```bash
echo $(pwd)      # exposed to splitting + globbing on the result
echo "$(pwd)"    # safe
```

### `$@` vs `$*`
```bash
set -- "a b" c      # two positional parameters: "a b" and "c"
for x in $*;   do echo "[$x]"; done     # [a] [b] [c]   — resplit
for x in "$*"; do echo "[$x]"; done     # [a b c]       — ONE word, joined with the first char of IFS
for x in $@;   do echo "[$x]"; done     # [a] [b] [c]   — resplit
for x in "$@"; do echo "[$x]"; done     # [a b] [c]     — each argument preserved exactly
```
**`"$@"` is almost always what you want.** It's the only form that preserves argument boundaries.

### The special parameters
| Parameter | Meaning |
|---|---|
| `$?` | Exit status of the most recently completed foreground command |
| `$!` | PID of the most recently backgrounded command |
| `$$` | PID of the shell — **cached; unchanged inside subshells** (file 04; use `$BASHPID` for the real current PID) |
| `$0` | Name of the shell or script |
| `$#` | Number of positional parameters |
| `$@` / `$*` | All positional parameters — see above |

### `$?` is overwritten by every command
```bash
false
echo "status: $?"   # status: 1
false
echo hello
echo $?             # 0 — this is echo's status, not false's
```
Capture it immediately if you need it: `rc=$?`.

### `local` swallows the status
```bash
f() { local x=$(false); echo $?; }   # prints 0 — local's own status, not false's
f() { local x; x=$(false); echo $?; } # prints 1 — assignment separated from the declaration
```

### Pipelines run in subshells (Bash)
```bash
echo hi | read x
echo "[$x]"      # [] in Bash — read ran in a subshell (file 04), x died with it
```
Zsh runs the last pipeline segment in the current shell by default, so the same code works there — a genuine Bash/Zsh behavioral difference (file 05). In Bash, `shopt -s lastpipe` fixes it, but only when job control is off (typically in scripts, not interactively).

### `(cmd)` vs `{ cmd; }` (file 04)
`(cmd)` forks a subshell — its `cd` and variable changes vanish; `{ cmd; }` runs in the current shell. Also note `exit` inside `(...)` exits only the subshell.

### Redirection order (file 11)
`cmd >file 2>&1` sends both to `file`; `cmd 2>&1 >file` leaves stderr on the terminal — each redirection acts on the fd table's *current* state, left to right.

### Command-name resolution order (files 04, 07)
Alias → keyword → function → builtin → `PATH`. `type -a name` shows every match.

### `read` loops mangle input by default
```bash
while read line; do ...; done < file         # strips leading/trailing whitespace, interprets backslashes
while IFS= read -r line; do ...; done < file # preserves the line exactly
```

---

## 2. Exit status — the full path

```
child calls exit(n)  →  kernel records n in the zombie's task_struct
        →  parent's waitpid() retrieves it  →  shell stores it in $?
```

- **`exit(n)` / returning from `main`** — only the low 8 bits survive: valid statuses are 0–255 (`exit 256` yields `0`).
- **`waitpid()`** returns a packed `status`; `WIFEXITED`/`WEXITSTATUS` extract a normal exit's code, `WIFSIGNALED`/`WTERMSIG` extract a fatal signal (file 08).
- **Convention:** `0` = success, non-zero = failure — not a kernel rule, a universal agreement.

**The shell's own conventions for special failures:**

| Status | Meaning |
|---|---|
| `126` | Command found, but not executable |
| `127` | Command not found |
| `128 + N` | Killed by signal `N` — so `130` = `SIGINT` (2), `137` = `SIGKILL` (9), `143` = `SIGTERM` (15) |

```bash
sleep 100        # press Ctrl+C
echo $?          # 130 — 128 + SIGINT's number, 2
```

> **Remember**
> A command killed by a signal has no "exit code" at all — `waitpid()` reports it as `WIFSIGNALED`. The shell *synthesizes* `128+N` into `$?` purely so scripts have a single number to test. That's a shell convention layered on top of what the kernel actually reports.

---

## 3. `&&`, `||`, `;`, `|`

| Operator | Meaning |
|---|---|
| `cmd1 ; cmd2` | Run `cmd2` regardless |
| `cmd1 && cmd2` | Run `cmd2` only if `cmd1` returned `0` |
| `cmd1 \|\| cmd2` | Run `cmd2` only if `cmd1` returned non-zero |
| `cmd1 \| cmd2` | **Pipe** — both run concurrently, stdout of one feeding the other (file 13) |

> **Common mistake**
> `||` and `|` are completely different operators despite looking alike: one is conditional sequencing based on exit status, the other is a concurrent data connection. `a && b || c` is **not** if/else — `c` also runs if `a` succeeds but `b` fails.

`|` binds tighter than `&&`/`||`, which have equal precedence and group left to right. `;` and `&` bind loosest.

```bash
command ; echo $?       # run it, then inspect its status
cmd || true             # force a "success" status, so it can't trip set -e
```

---

## 4. `set -e`, `set -u`, `set -o pipefail`

| Option | Effect | Caveat |
|---|---|---|
| `set -e` | Exit the shell when a command fails | **Not** triggered inside `if`/`while` conditions, in the non-final parts of `&&`/`||` lists, after `!`, or for any pipeline element except the last — surprising and inconsistent enough that many experts avoid relying on it |
| `set -u` | Treat expanding an unset variable as an error | Catches typos like `$HOEM`; breaks code assuming unset ≡ empty |
| `set -o pipefail` | A pipeline's status is the rightmost *non-zero* status, instead of just the last command's | Without it, `false \| true` reports success (file 39, step 18) |

The common combination is `set -euo pipefail` — useful, but worth knowing that each of the three has real edge cases, `set -e` most of all.

---

## 5. Common misconceptions

| Misconception | Reality |
|---|---|
| "`$?` holds the status of the command I care about." | It's overwritten by every command, including `echo` and `[`. |
| "A pipeline fails if any command in it fails." | By default, its status is only the **last** command's — that's what `pipefail` changes. |
| "`a && b \|\| c` is if/else." | `c` also runs when `a` succeeded and `b` failed. |
| "`$$` changes inside a subshell." | It's cached from the top-level shell; `$BASHPID` is the real current PID. |
| "`set -e` reliably aborts on any error." | It has numerous exemptions — conditions, `&&`/`||` lists, non-final pipeline elements. |
| "A signal-killed process exits with some code." | It has no exit code at all; the shell invents `128+N`. |

---

## 6. Try this

> **Try this**
> ```bash
> false; true; echo $?                # 0
> nosuchcommand; echo $?              # 127
> sleep 100                           # Ctrl+C, then:
> echo $?                             # 130
> false | true; echo $?               # 0
> set -o pipefail; false | true; echo $?   # 1
> ```

---

## 7. Summary

- Most shell gotchas are file 06's expansion pipeline and file 04's fork/no-fork rules surfacing in specific syntax: quote expansions, prefer `"$@"`, remember `[` is a command.
- Exit status flows `exit(n)` → zombie's `task_struct` → `waitpid()` → `$?`; only 8 bits survive, and signal deaths become `128+N` by shell convention.
- `&&`/`||` branch on exit status; `|` is concurrent data flow — unrelated despite the visual similarity.
- `set -e`/`-u`/`pipefail` each help, each with real edge cases.

---

## What's next

`43_MACOS_COMPARISON.md` collects every macOS note scattered through this handbook into one side-by-side comparison with Linux.
