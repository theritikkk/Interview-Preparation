# 34 — Mini Shell Project

> **Where this fits in the stack**
> ```
> Everything files 04-23 explained  →  [ ASSEMBLED, BY YOU, IN C ]
>                                       ^^^^^^^^^^^^^^^^^^^^^^^^^^
>                                       this file
> ```
> This is the capstone. Every primitive below has already been fully explained elsewhere in this handbook — this file is a **build guide**, not a fresh explanation. Each stage should feel like assembling pieces you already understand, not learning something new. Write real code for each stage yourself; the skeletons here show the one *new* piece each stage introduces, not a complete program to copy.

---

## Stage 1 — Execute commands

**Objective:** read a line, split it into words, run it as one command, wait for it.
**Primitive:** `fork`/`execve`/`waitpid` (file 08).

```c
while (1) {
    printf("mini$ "); fflush(stdout);
    char line[1024];
    if (!fgets(line, sizeof line, stdin)) break;
    line[strcspn(line, "\n")] = 0;

    char *argv[64]; int i = 0;
    char *tok = strtok(line, " ");
    while (tok) { argv[i++] = tok; tok = strtok(NULL, " "); }
    argv[i] = NULL;
    if (!argv[0]) continue;

    pid_t pid = fork();
    if (pid == 0) {
        execvp(argv[0], argv);
        perror("exec failed");
        _exit(127);
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
}
```

---

## Stage 2 — PATH lookup

**Objective:** understand what `execvp` is already doing for you.
**Primitive:** `PATH` search (file 07).

`execvp` already performs the `PATH` walk internally — that's the `p`. As an exercise worth doing anyway: implement your own search (split `$PATH` on `:`, try each directory + `/` + `argv[0]`, check with `access()`) to genuinely internalize file 07's mechanism, then compare against what `execvp` was doing for you all along.

---

## Stage 3 — Builtins

**Objective:** recognize `cd`, `exit`, and similar names and handle them **without forking**.
**Primitive:** the process-hierarchy argument from file 07 — this is where you implement it, not just read about it.

```c
if (strcmp(argv[0], "cd") == 0) {
    chdir(argv[1] ? argv[1] : getenv("HOME"));
    continue;                          /* NO fork — stays in THIS process */
}
if (strcmp(argv[0], "exit") == 0) exit(0);
```

---

## Stage 4 — Redirection

**Objective:** recognize `>`, `>>`, `<` and wire them up before `exec`.
**Primitive:** open + `dup2` before `exec` — file 12's entire "critical point."

```c
/* in the CHILD, after fork(), BEFORE execvp(): */
if (redirect_out) {
    int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    dup2(fd, STDOUT_FILENO);
    close(fd);
}
```

---

## Stage 5 — Pipes

**Objective:** one pipe, two commands.
**Primitive:** file 13's `pipe`+`fork`+`dup2`+`close` dance, exactly as described — now typed out yourself.

```c
int fds[2]; pipe(fds);
pid_t p1 = fork();
if (p1 == 0) { dup2(fds[1], 1); close(fds[0]); close(fds[1]); execvp(cmd1, argv1); }
pid_t p2 = fork();
if (p2 == 0) { dup2(fds[0], 0); close(fds[0]); close(fds[1]); execvp(cmd2, argv2); }
close(fds[0]); close(fds[1]);          /* the SHELL's own copies — file 13 Section 6 */
waitpid(p1, NULL, 0);
waitpid(p2, NULL, 0);
```

---

## Stage 6 — Multiple pipelines

**Objective:** generalize Stage 5 to N commands.
**Primitive:** the multi-pipe close-discipline subtlety from file 13, Section 7.

**Structural hint:** N commands need N−1 pipes — an array of fd pairs. The part that actually tests whether you understood file 13: **every** child must close **every** pipe fd it isn't directly using, not just the fds from its own adjacent pipe. A child that inherits and forgets an unrelated pipe's fds is exactly file 13's "reader hangs forever" bug, now something you can actually trigger and fix yourself.

---

## Stage 7 — Environment variables

**Objective:** implement `export`/`unset` affecting what child processes see.
**Primitive:** `setenv`/`putenv`/`unsetenv`, `environ` (file 17).

```c
if (strcmp(argv[0], "export") == 0) {
    putenv(argv[1]);      /* expects "NAME=value" form */
    continue;
}
```

---

## Stage 8 — Background jobs

**Objective:** recognize a trailing `&`; don't `waitpid` synchronously for it.
**Primitive:** fork without synchronous wait, plus a job table (file 15).

```c
if (background) {
    /* fork as usual, but do NOT waitpid here — */
    /* record the pid in a jobs array/table instead */
} else {
    waitpid(pid, &status, 0);
}
```

---

## Stage 9 — Signals

**Objective:** reap background jobs as they finish; don't let the shell itself die on `Ctrl+C`.
**Primitive:** `sigaction`, `SIGCHLD` (file 14).

```c
void sigchld_handler(int sig) {
    int status; pid_t p;
    while ((p = waitpid(-1, &status, WNOHANG)) > 0) {
        /* mark that job as done in your job table */
    }
}
/* in main(): */
signal(SIGCHLD, sigchld_handler);
signal(SIGINT, SIG_IGN);     /* the SHELL itself ignores Ctrl+C — only foreground jobs should die from it */
```

---

## Stage 10 — Job control

**Objective:** `fg`/`bg`/`jobs`, with genuine process groups and terminal foreground handoff.
**Primitive:** `setpgid()`, `tcsetpgrp()`/`tcgetpgrp()` — file 16's actual API.

```c
/* in the child, right after fork(), BEFORE exec: */
setpgid(0, 0);                              /* put this child in its OWN new process group */

/* for a FOREGROUND job, in the parent: */
pid_t child_pgid = /* the pgid you just set */;
tcsetpgrp(STDIN_FILENO, child_pgid);
waitpid(pid, &status, WUNTRACED);
tcsetpgrp(STDIN_FILENO, getpid());          /* shell reclaims the foreground when done */
```

> **Remember**
> This is genuinely the most intricate stage in the whole project — it's file 16's entire session/process-group/`tcsetpgrp` model, implemented by hand, for the first time with real consequences if you get the ordering wrong (a `setpgid` called too late, after the child has already started running, is a classic race in a first implementation).

---

## Stage 11 — History

**Objective:** remember previous commands.
**Primitive:** file 23's Readline discussion — read honestly.

A genuinely full implementation (arrow-key browsing, live editing) needs raw mode plus manual redrawing — file 23's entire Readline section, built from scratch. That's a substantial undertaking on its own. A reasonable, honest first pass: log each command into an array (and optionally a file on exit) and provide a `history` builtin that lists them — real functionality, without reimplementing Readline in full. Matching this handbook's own accuracy-first approach: know exactly which piece you're simplifying, rather than pretending a shortcut is the complete thing.

---

## Stage 12 — Basic quoting/parsing

**Objective:** handle `"quoted strings with spaces"` and `'single quotes'`, replacing Stage 1's naive `strtok`-based splitting.
**Primitive:** file 06's Phase 1 tokenization, implemented by hand.

`strtok` splits on whitespace only — it has no concept of quotes at all, so `echo "hello world"` would wrongly become two arguments. Writing your own small tokenizer that tracks quote-state character by character (in a quote? which kind? does this whitespace count as a separator right now?) is a genuinely good exercise in exactly the pipeline file 06 described in the abstract — now something you're implementing, bug by bug.

---

## Where to go from here

For the other half of the picture — a persistent *multiplexer* built the same staged way — see `48_MINI_MULTIPLEXER_PROJECT.md`.


The 12 stages above already exercise every major OS primitive this handbook covers. If you want to keep extending: `;`/`&&`/`||` sequencing (file 42 preview), `$VAR` expansion, and combined redirection-plus-pipe parsing are natural next steps — all built on mechanisms already fully covered, none requiring anything new conceptually.

---

## Summary

- Twelve stages, each introducing exactly one OS primitive already explained in depth elsewhere in this handbook.
- Stages 1-7 build the execution core; 8-10 add job control, ending at the genuinely hardest stage (real process groups and terminal handoff); 11-12 round out history and proper parsing.
- Every code skeleton here is deliberately partial — the point is writing the surrounding program yourself, with the mechanism already understood going in.

---

## What's next

`35_LABS.md` consolidates every lab referenced across this handbook into one indexed list, cross-referenced back to whichever file unlocks it.
