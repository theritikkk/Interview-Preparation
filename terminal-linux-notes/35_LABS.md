# 35 — Labs

Every lab referenced across this handbook, consolidated. Each follows the same five-part format: what you're testing, exactly what to run, what you should see, why, and what to think about afterward. Do them roughly in order — each assumes the files before its listed reference are already read.

---

## Foundations

### Lab 1 — Trace `echo hello` by hand
**Objective:** walk the full keyboard-to-screen path without running anything.
**Commands:** none — work it out on paper or out loud.
**Expected observation:** a complete ordered list: keyboard → terminal emulator → PTY master → line discipline → PTY slave → shell `read()` → tokenize → fork → `execve("echo")` → `write(1,...)` → PTY slave → master → terminal emulator → screen.
**Internally:** this is files 01, 02, 06, 07 end to end.
**Questions:** Which steps happen in the shell's own process? Which happen only after `fork()`?

### Lab 2 — `strace ls`
**Objective:** see the real syscalls behind a familiar command.
**Commands:** `strace ls`
**Expected observation:** `execve`, several `mmap`/`mprotect`, `openat`, `getdents64`, `write`, `close`, in that general order.
**Internally:** file 32's annotated trace — compare line by line.
**Questions:** Which lines are `ld.so` doing dynamic linking, before `ls`'s own code runs at all?

### Lab 3 — `strace cat file.txt`
**Objective:** watch the read-until-EOF loop directly.
**Commands:** `strace cat somefile.txt`
**Expected observation:** `openat`, `fstat`, a `read`/`write` pair repeated, a final `read` returning `0`, `close`.
**Internally:** file 32, Section 3 — the same EOF mechanism as a pipe (file 13) or `Ctrl+D` (file 23).
**Questions:** What would change in this trace for a file larger than the read buffer size?

---

## Processes and pipes, in C

### Lab 4 — Create a pipe manually in C
**Objective:** call `pipe()` and prove data flows one direction only.
**Commands:** a small C program: `pipe(fds); write(fds[1], "hi", 2); char buf[3]; read(fds[0], buf, 2);`
**Expected observation:** `buf` contains `"hi"`.
**Internally:** file 13, Section 1-2.
**Questions:** What happens if you `read()` from `fds[1]` instead of `fds[0]`?

### Lab 5 — `fork` + `exec` in C
**Objective:** the minimal process-creation pattern.
**Commands:** `fork()`, check the return value, `execvp()` in the child, `waitpid()` in the parent.
**Expected observation:** parent and child print different PIDs; the child's `printf` never runs after a successful `execvp`.
**Internally:** file 08, Sections 6-9.
**Questions:** What happens if you forget the `if (pid == 0)` check and both branches run the same code?

### Lab 6 — Redirect stdout with `dup2()`
**Objective:** implement `> file` entirely by hand.
**Commands:** `open()` a file, `dup2(fd, STDOUT_FILENO)`, then `execvp()` any command.
**Expected observation:** the command's output lands in the file, not your terminal.
**Internally:** files 10, 12 — the exact mechanism behind shell redirection.
**Questions:** Does the order of `dup2()` vs `open()` matter? What if you `dup2()` before the file is successfully opened?

### Lab 7 — Build a 3-stage pipeline in C
**Objective:** wire three processes together by hand.
**Commands:** two `pipe()` calls, three `fork()`s, careful `dup2`/`close` per child — the full file 13 Section 7 pattern.
**Expected observation:** equivalent output to `cmd1 | cmd2 | cmd3` run from a real shell.
**Internally:** file 13, in full — this lab **is** that file, typed out.
**Questions:** What happens if you forget to close an unused pipe fd in the middle process? (Predict first, then actually remove a `close()` and watch it hang.)

### Lab 8 — Catch `SIGINT`
**Objective:** install a handler and watch `Ctrl+C` not kill your program.
**Commands:** `sigaction(SIGINT, &sa, NULL)` with a handler that sets a flag (file 14, Section 8's example).
**Expected observation:** pressing `Ctrl+C` prints your handler's message instead of terminating the process.
**Internally:** file 14, full signal catalog and the handler-safety callout.
**Questions:** What breaks if your handler calls `printf()` directly instead of just setting a flag?

### Lab 9 — Inspect a process tree
**Objective:** see real parent/child relationships.
**Commands:** `pstree -p $$`
**Expected observation:** your terminal emulator at the top, your shell below it, any running job below that — matching file 01's diagram exactly.
**Internally:** files 01, 08.
**Questions:** Does your terminal emulator show up as a single process, or does it look like a client/server pair (file 01's `gnome-terminal-server` note)?

---

## /proc

### Lab 10 — Explore `/proc` interactively
**Objective:** get comfortable browsing live kernel state.
**Commands:** `ls /proc/$$/`, then open a few entries with `cat`.
**Expected observation:** `status`, `maps`, `fd/`, `cwd`, `cmdline`, and more, all for your own shell.
**Internally:** file 33.
**Questions:** Which entries report a size of `0` in `ls -l` despite having real content when read?

### Lab 11 — Inspect `/proc/<pid>/fd` while a program holds files open
**Objective:** watch fds appear and disappear in real time.
**Commands:** `sleep 100 < somefile.txt &` then, in another terminal, `ls -l /proc/<that pid>/fd`
**Expected observation:** fd 0 is a symlink to `somefile.txt`; fds 1/2 point at a terminal device.
**Internally:** files 10, 33.
**Questions:** What does the fd 0 symlink look like for a process whose stdin is a pipe instead of a file?

---

## Terminal internals

### Lab 17 — Explore PTYs
**Objective:** allocate a PTY pair yourself.
**Commands:** either the `posix_openpt`/`grantpt`/`unlockpt` sequence from file 02 in a small C program, or the simpler `script` command.
**Expected observation:** a new entry appears under `/proc/<pid>/fd` or `/dev/pts/` for the new pair.
**Internally:** file 02, Section 9.
**Questions:** What's the minimum sequence of calls between allocating the master and having a usable slave path?

### Lab 21 — Toggle raw mode in C
**Objective:** build file 23's raw-mode example yourself, then break it on purpose.
**Commands:** file 23, Section 5's program, compiled and run.
**Expected observation:** typed characters no longer echo automatically; arrow keys print as three separate bytes.
**Internally:** file 23, in full.
**Questions:** Comment out the final `tcsetattr` restore call, run it, then exit — what does your shell look like afterward, and how do you fix it?

### Lab 22 — Trigger `SIGWINCH`
**Objective:** see the terminal-resize signal fire.
**Commands:** a small C program installing a `SIGWINCH` handler that just prints a message; run it, then resize your terminal window.
**Expected observation:** your handler fires every time you resize, with no keyboard input involved at all.
**Internally:** file 14, Section 7.
**Questions:** What does `vim` or `htop` do differently from your minimal handler once they receive this signal?

---

## Mini-shell stages (file 34)

### Lab 12 — Minimal shell: execute one command
**Objective:** Stage 1 of file 34, working end to end.
**Commands:** file 34, Stage 1's skeleton, filled in.
**Expected observation:** your shell runs `ls`, `echo hi`, and similar, correctly.
**Internally:** file 08.
**Questions:** What happens if the command doesn't exist — does your shell crash, or handle it gracefully?

### Lab 13 — Add pipes
**Objective:** Stage 5/6 of file 34.
**Commands:** extend your shell to handle `cmd1 | cmd2`.
**Expected observation:** `ls | grep somefile` works identically to a real shell.
**Internally:** file 13.
**Questions:** Does your implementation correctly close every unused pipe fd, in every child — test with `yes | head -1` and confirm it actually terminates.

### Lab 14 — Add redirection
**Objective:** Stage 4 of file 34.
**Commands:** extend your shell to handle `>`, `>>`, `<`.
**Expected observation:** `ls > out.txt` works identically to a real shell.
**Internally:** file 12.
**Questions:** Does your shell correctly truncate the target file even before the command's first write?

### Lab 15 — Add background jobs
**Objective:** Stage 8 of file 34.
**Commands:** extend your shell to handle a trailing `&`.
**Expected observation:** `sleep 10 &` returns your prompt immediately.
**Internally:** file 15.
**Questions:** Without Stage 9's `SIGCHLD` handler yet, what happens to a finished background job — does it become a zombie?

### Lab 16 — Add signal handling
**Objective:** Stage 9 of file 34.
**Commands:** add the `SIGCHLD` handler from file 34's Stage 9.
**Expected observation:** background jobs no longer accumulate as zombies; `Ctrl+C` no longer kills your shell itself.
**Internally:** file 14.
**Questions:** What's different about how your shell needs to treat `Ctrl+C` for a foreground job vs for itself?

---

## Remote and multiplexed sessions

### Lab 18 — SSH with and without `-t`
**Objective:** see the PTY-vs-no-PTY difference directly.
**Commands:** `ssh user@host 'tty'` then `ssh -t user@host 'tty'`
**Expected observation:** `not a tty` without `-t`; an actual `/dev/pts/N` path with it.
**Internally:** file 26, Section 6-7.
**Questions:** Try running `ssh user@host 'ls --color=auto'` without `-t` and compare the output's colors against running it locally — why the difference?

### Lab 19 — tmux: kill the terminal, reattach
**Objective:** prove the server survives client death.
**Commands:** `tmux new -s t1`, start `sleep 300` inside, close the entire terminal window (not just detach), reopen a terminal, `tmux attach -t t1`.
**Expected observation:** your `sleep` is still counting down, undisturbed.
**Internally:** file 27, Section 5.
**Questions:** What's different about this test versus a normal detach (`Ctrl+B`, `D`)?

### Lab 23 — Find tmux's Unix domain socket
**Objective:** see the actual IPC mechanism as a real file.
**Commands:** `ls -la /tmp/tmux-*/`
**Expected observation:** an entry with a leading `s` in its permissions — a genuine socket, not a regular file.
**Internally:** file 27, Section 4.
**Questions:** What does `ss -x` (or `lsof`) show about this socket while a client is attached?

---

## Resource limits and containers

### Lab 24 — Hit a `ulimit -n` limit on purpose
**Objective:** reproduce "too many open files" deliberately.
**Commands:** `ulimit -n 10` in a throwaway shell, then write a tiny script or C program that `open()`s more than 10 files without closing any.
**Expected observation:** an `EMFILE` error once you cross the limit.
**Internally:** file 21, Section 6.
**Questions:** Does closing the files afterward let a *new* process exceed the old count, or is the limit inherited by your current shell's children too?

### Lab 20 — Run a shell inside `docker exec -it`, inspect PID 1
**Objective:** see a container's isolated PID namespace directly.
**Commands:** `docker run -d --name t nginx`, then `docker exec -it t bash`, then inside: `echo $$` and `ps aux`.
**Expected observation:** a low PID number, and a process list containing **only** container-internal processes.
**Internally:** file 29, Sections 3, 5.
**Questions:** What does `ps aux` show on the **host**, for the same container, compared to what you saw from inside it?

---

## Extension labs: terminal parsing and multiplexers (files 45-48)

### Lab 25 — Build a VT parser
**Objective:** implement Williams' state machine and reproduce its error-recovery behaviors.
**Commands:** compile file 45, Section 10's parser; feed it `printf '\033[31Hello' | ./vtparse` and the other cases in Section 5.
**Expected observation:** the `H` is dispatched as a cursor-positioning command, not printed.
**Internally:** file 45 — final bytes dispatch; a missing final byte changes the meaning of the next one.
**Questions:** Add the `osc string` state. Should BEL terminate it? (DEC says no; xterm says yes.)

### Lab 26 — A toy multiplexer
**Objective:** see that a multiplexer's output is regenerated, not relayed.
**Commands:** run file 46, Section 4.1's script; then, with real tmux, Section 4.2's `pipe-pane` + `script` comparison.
**Expected observation:** the program's bytes and the client's bytes differ — the latter adds cursor positioning and clears.
**Internally:** file 46, Section 3 — parse, grid, regenerate.
**Questions:** Why does the program's `\n` arrive at the multiplexer as `\r\n`?

### Lab 27 — Replicated terminal state machines
**Objective:** test what keeps terminal replicas identical, and what breaks them.
**Commands:** run file 47, Section 11's script.
**Expected observation:** convergence under different chunkings; divergence for a grid-only mid-sequence snapshot, for a dropped control byte, and for different geometry.
**Internally:** file 47, Sections 6.1-6.2; file 45, Section 9.
**Questions:** Add a sequence counter so a client can detect loss — then reproduce Case 4b. Does your client notice?

---

### Lab 28 — Run and extend minimux
**Objective:** run a working multiplexer, then close one of its documented gaps.
**Commands:** from `code/`: `python3 test_minimux.py`; then `python3 minimux.py start work` and `python3 minimux.py attach work` (detach with `Ctrl+B d`, reattach from a second terminal).
**Expected observation:** 37/37 checks pass; the session survives detaching, and a second client sees the same shell.
**Internally:** file 48 — a PTY owner, a server, a terminal model, snapshot-on-attach, fan-out.
**Questions:** Pick one of file 48's exercises (alternate screen, divergence detection, backpressure) and make a failing test pass.

---

## Summary

Twenty-four core labs plus four extension labs (25-28), each tied to the specific file(s) that explain *why* the expected observation happens — not just *that* it does. If an expected observation doesn't match what you actually see, that's not necessarily a mistake on your part; it's a prompt to re-read the linked section and figure out what's different about your specific setup.

---

## What's next

`36_INTERVIEW_QUESTIONS.md` turns every mechanism covered so far into the interview format your spec requested: short answer, deep answer, common misconception, and a follow-up question, organized by category.
