# 37 — Debugging Scenarios

Five realistic situations, each walked as an actual investigation — an ordered checklist of what to check and why, not just a list of relevant concepts. Real bugs usually span several mechanisms at once; that's exactly why each scenario below pulls from multiple earlier files rather than one.

---

## Scenario 1: "My program works when run manually but fails from a shell script"

**Investigate, in this order:**

1. **The shebang line, against what `/bin/sh` actually is.** If the script starts `#!/bin/sh` but uses Bash-only syntax (arrays, `[[ ]]`), and your system's `/bin/sh` is `dash` (common on Debian/Ubuntu), it will fail or misbehave — even though `bash script.sh` run explicitly works fine (file 03, Section 7).
2. **`PATH`, specifically compared between interactive and non-interactive.** A script run via `./script.sh` or `bash script.sh` is a **non-interactive, non-login** shell — it doesn't read `~/.bashrc` (file 18's decision table). If your `PATH` customizations live only there, the script sees a different, more minimal `PATH` than your interactive prompt does.
3. **Working directory assumptions.** A relative path the script relies on assumes a specific `cwd` — which depends entirely on *where you were* when you invoked it, not where the script file itself lives (file 07-08's cwd-is-per-process point).
4. **Exported vs. unexported variables.** A variable your interactive shell has might simply never have been `export`ed — invisible to any child process, script included (file 17).
5. **File permissions.** Is the script itself executable (`chmod +x`)? Do the files/directories it touches have the permissions it assumes (file 20)?

> **Interview answer**
> "This is almost always one of two things: the script's shebang doesn't match the shell it was actually written for, or it's relying on PATH/environment state that only exists in interactive, login-shell startup files the script's own invocation never reads."

---

## Scenario 2: "Command works in terminal but not through SSH"

**Investigate, in this order:**

1. **How the remote command is being invoked.** `ssh host 'mycommand'` triggers non-interactive shell startup rules on the remote side — PATH customizations living only in `.bashrc` may be missing, exactly per file 18's decision table, now applied remotely.
2. **Which startup files actually ran.** File 18's full table, remote side — login vs. non-login, interactive vs. non-interactive, determines the exact file set.
3. **Whether a PTY was allocated.** Without `-t`, no PTY at all (file 26, Section 7) — if the remote command checks `isatty()` and behaves differently, or refuses to run without a real terminal (many `sudo` configurations do exactly this), that's the cause.
4. **Whether the environment variable you need was even forwarded.** Not everything crosses an SSH connection by default — `sshd` configurations commonly restrict which environment variables are even allowed through (`AcceptEnv`/`SendEnv`), independent of the login/interactive distinction from point 1-2.

---

## Scenario 3: "`Ctrl+C` doesn't stop my program"

**Investigate, in this order:**

1. **Is it catching `SIGINT` and not actually exiting in its handler?** File 14, Section 8's example — a handler that sets a flag but whose main loop never checks it will absorb every `Ctrl+C` silently.
2. **Is the target process actually in the foreground process group right now?** If it was backgrounded at some point (file 15), or is a child that ended up in a different process group than you'd expect, `SIGINT` never reaches it at all — it's delivered to a process group, not to "whatever you're looking at" (file 16).
3. **In a pipeline, is the *right* member of the group the problem?** `Ctrl+C` reaches every process in the foreground group (file 13-14) — but if one member deliberately ignores it while another keeps feeding it work, the overall pipeline can appear to survive even though the signal was delivered correctly.

---

## Scenario 4: "Output is missing when piped"

**Investigate, in this order:**

1. **Buffering.** stdout is typically **fully block-buffered** when not connected to a terminal — i.e., exactly when piped (file 09, 11). Output can sit in an internal buffer, never flushed, especially if the program crashes or is killed before a natural flush point.
2. **Is the missing output actually going to stderr?** A plain pipe (`cmd1 | cmd2`) only carries stdout — stderr stays connected to the original destination (file 11). What looks like "missing" output may simply be going somewhere you're not looking.
3. **Is the *consuming* program actually reading what was sent?** Its own buffering, or a crash on its side, can look identical to "the producer never sent it" from the outside (file 13).

**The fix, for the buffering case specifically:** `stdbuf -oL command` (force line buffering even when piped), or the program's own explicit `fflush()`/flush calls.

---

## Scenario 5: "Program works interactively but hangs in a pipeline"

**Investigate, in this order:**

1. **EOF — is every copy of the relevant pipe's write end actually getting closed?** File 13, Sections 6 and 8 — this is the single most common cause of exactly this symptom.
2. **Stray, forgotten file descriptors.** In a multi-stage pipeline, a child can inherit and forget an unrelated pipe's fds through multiple levels of `fork` (file 13, Section 7's specific multi-pipe subtlety) — one leftover copy anywhere is enough.
3. **A genuinely different cause: mutual write deadlock.** If both sides of a pipeline are simultaneously trying to **write** large amounts of data to each other, each can fill the other's buffer and block, with neither able to make progress — a real deadlock, distinct from the unclosed-fd cause, worth ruling in or out separately (file 13, Section 4).

> **Remember**
> "Hangs, but doesn't crash or print an error" is close to a signature for file 13's unclosed-write-end bug specifically — check that first, every time, before looking anywhere else.

---

## Summary

- Scenario 1 and 2 both hinge on file 18's login/interactive startup-file distinction — the single most common root cause of "works here, not there" across this whole chapter.
- Scenario 3 is almost always either a handler that doesn't actually exit, or a signal reaching a process group that doesn't contain what you think it does.
- Scenario 4's buffering-vs-stderr-vs-consumer triage is the standard order to check, fastest-to-rule-out first.
- Scenario 5's unclosed-write-end cause is common enough to check first, before considering the rarer mutual-deadlock case.

---

## What's next

`38_COMMON_MISCONCEPTIONS.md` consolidates every misconception callout from every file into one reference table — a last pass before the master execution trace.