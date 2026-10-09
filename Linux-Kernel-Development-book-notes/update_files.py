import os
import re

base_dir = "/Users/ritikraj/Documents/GitHub/Interview-Preparation/Linux-Kernel-Development-book-notes"

# 1. glossary.md
glossary_path = os.path.join(base_dir, "glossary.md")
with open(glossary_path, "r") as f:
    g_content = f.read()

# Extract preamble and closing
preamble = g_content.split("---")[0] + "---\n"
closing = "\n---\n\n*(This glossary will grow" + g_content.split("*(This glossary will grow")[1]

# Extract existing terms
terms_text = g_content.split("---")[1].split("---")[0].strip()
entries = {}
current_term = ""
current_desc = []
for line in terms_text.split('\n'):
    if line.startswith('### '):
        if current_term:
            entries[current_term.lower().strip('`')] = current_term + '\n' + '\n'.join(current_desc)
        current_term = line
        current_desc = []
    elif current_term:
        current_desc.append(line)
if current_term:
    entries[current_term.lower().strip('`')] = current_term + '\n' + '\n'.join(current_desc)

new_terms = [
    ("clone()", "**Definition:** system call that creates a new process with fine-grained control over what is shared\n**Why it matters:** Unifies thread and process creation in Linux\n**Related concepts:** fork(), kernel_clone()\n**Introduced:** Chapter 3"),
    ("Copy-on-Write (COW)", "**Definition:** memory optimization where forked processes share pages until one writes\n**Why it matters:** Makes fork() extremely fast and efficient\n**Related concepts:** fork(), virtual memory\n**Introduced:** Chapter 3"),
    ("do_exit()", "**Definition:** kernel function that performs process termination cleanup\n**Why it matters:** The internal implementation of exit()\n**Related concepts:** EXIT_ZOMBIE\n**Introduced:** Chapter 3"),
    ("EXIT_ZOMBIE", "**Definition:** process state after exit but before parent acknowledges via wait()\n**Why it matters:** Prevents process descriptors from disappearing before parents can read exit statuses\n**Related concepts:** do_exit(), wait()\n**Introduced:** Chapter 3"),
    ("for_each_process()", "**Definition:** macro to iterate over all processes in the system\n**Why it matters:** Provides a way to walk the entire process list\n**Related concepts:** task_struct\n**Introduced:** Chapter 3"),
    ("fork()", "**Definition:** system call that creates a child process as a copy of the parent\n**Why it matters:** The fundamental UNIX process creation mechanism\n**Related concepts:** clone(), Copy-on-Write (COW)\n**Introduced:** Chapter 3"),
    ("init process (PID 1)", "**Definition:** the first user-space process, ancestor of all other processes\n**Why it matters:** Forms the root of the process tree and reaps orphaned zombies\n**Related concepts:** Reparenting\n**Introduced:** Chapter 3"),
    ("Kernel thread", "**Definition:** a process that runs exclusively in kernel-space with no user address space (mm = NULL)\n**Why it matters:** Allows the kernel to perform background operations\n**Related concepts:** kthread_create(), kthread_run()\n**Introduced:** Chapter 3"),
    ("kernel_clone() / do_fork()", "**Definition:** internal kernel function implementing fork/clone\n**Why it matters:** The actual workhorse behind all process creation syscalls\n**Related concepts:** fork(), clone(), vfork()\n**Introduced:** Chapter 3"),
    ("kthread_create() / kthread_run()", "**Definition:** API for creating kernel threads\n**Why it matters:** Standard interface for kernel subsystems to spawn background workers\n**Related concepts:** Kernel thread\n**Introduced:** Chapter 3"),
    ("PID (Process ID)", "**Definition:** unique integer identifying a process\n**Why it matters:** The user-space identifier for tasks\n**Related concepts:** task_struct\n**Introduced:** Chapter 3"),
    ("Process", "**Definition:** an active, running program plus its associated resources\n**Why it matters:** The fundamental abstraction of execution in the OS\n**Related concepts:** task_struct, Thread\n**Introduced:** Chapter 3"),
    ("Process descriptor", "**Definition:** see `task_struct`\n**Why it matters:** The kernel's representation of a process\n**Related concepts:** task_struct\n**Introduced:** Chapter 3"),
    ("Reparenting", "**Definition:** reassigning orphaned children to init or a subreaper\n**Why it matters:** Prevents orphaned processes from becoming permanent zombies\n**Related concepts:** init process, Zombie process\n**Introduced:** Chapter 3"),
    ("task_struct", "**Definition:** the kernel's central data structure representing a process/task\n**Why it matters:** The single most important structure in process management\n**Related concepts:** Process descriptor, thread_info\n**Introduced:** Chapter 3"),
    ("thread_info", "**Definition:** small structure linked to task_struct, historically at the base of the kernel stack\n**Why it matters:** Contains low-level, architecture-specific process state\n**Related concepts:** task_struct\n**Introduced:** Chapter 3"),
    ("vfork()", "**Definition:** variant of fork() that shares address space and suspends parent until child execs/exits\n**Why it matters:** Historic optimization before COW, still used in memory-constrained environments\n**Related concepts:** fork()\n**Introduced:** Chapter 3"),
    ("wait() / waitpid()", "**Definition:** system calls for a parent to collect a child's exit status and release its process descriptor\n**Why it matters:** The required counterpart to fork() and exit() to prevent zombies\n**Related concepts:** Zombie process, EXIT_ZOMBIE\n**Introduced:** Chapter 3"),
    ("Zombie process", "**Definition:** a terminated process whose task_struct still exists because the parent hasn't called wait()\n**Why it matters:** A common resource leak if parents don't clean up correctly\n**Related concepts:** wait(), EXIT_ZOMBIE\n**Introduced:** Chapter 3")
]

for term, desc in new_terms:
    formatted_term = f"### {term if '`' in term else f'{term}'}\n{desc}"
    if not term.startswith('`') and term.lower() not in ['interrupt', 'interrupt context', 'kernel', 'kernel-space', 'microkernel', 'monolithic kernel', 'operating system', 'privilege level ("ring," on x86)', 'process context', 'system call', 'trap instruction', 'unix', 'user-space', 'gnu c', 'inline function', 'oops', 'panic', 'copy-on-write (cow)', 'init process (pid 1)', 'kernel thread', 'pid (process id)', 'process', 'process descriptor', 'reparenting', 'zombie process']:
        formatted_term = f"### `{term}`\n{desc}"
    entries[term.lower().strip('`')] = formatted_term

# sort keys alphabetically ignoring case and backticks
sorted_keys = sorted(entries.keys())
sorted_content = "\n\n".join([entries[k].strip() for k in sorted_keys])

with open(glossary_path, "w") as f:
    f.write(preamble + "\n" + sorted_content + closing)

# 2. source-code-map.md
scm_path = os.path.join(base_dir, "source-code-map.md")
with open(scm_path, "r") as f:
    scm_content = f.read()

ch2_ch3 = """
### Chapter 2's Contribution to This Map
- `MAINTAINERS` — who maintains each subsystem
- `scripts/get_maintainer.pl` — query MAINTAINERS for a specific file
- `Documentation/` — in-tree documentation (Sphinx-based on modern kernels)
- `lib/string.c` — kernel's own string functions (replacing libc)
- `include/linux/printk.h` — printk, pr_err, pr_info, etc.
- `Makefile` (top-level) — drives the entire build
- Build artifacts: `vmlinux`, `bzImage`, `System.map`

### Chapter 3's Contribution to This Map
Key files:
- `include/linux/sched.h` — task_struct definition (the single most important header in the kernel)
- `include/linux/sched/task.h` — task lifecycle helpers
- `kernel/fork.c` — fork, clone, copy_process, dup_task_struct implementation
- `kernel/exit.c` — do_exit, exit_notify, release_task
- `kernel/pid.c` — PID allocation and management
- `arch/x86/include/asm/current.h` — current macro (x86-64)
- `arch/x86/include/asm/thread_info.h` — thread_info structure (x86)
- `include/linux/kthread.h` — kernel thread creation API
- `kernel/kthread.c` — kernel thread implementation

Key structures: task_struct, thread_info, pid
Key functions: copy_process, dup_task_struct, do_fork/kernel_clone, do_exit, release_task, alloc_pid, kthread_create, kthread_run

Navigation commands:
```bash
rg -n "struct task_struct {" include/linux/sched.h
rg -n "copy_process" kernel/fork.c
rg -n "do_exit" kernel/exit.c
rg -n "DEFINE_PER_CPU.*current_task" arch/x86/
git grep -n "for_each_process" -- '*.h'
```

Call chain:
```
User: fork()
  → libc wrapper
    → syscall (clone/clone3)
      → kernel_clone() [kernel/fork.c]
        → copy_process()
          → dup_task_struct()
          → copy_mm() / copy_files() / copy_fs() / ...
          → alloc_pid()
        → wake_up_new_task()
```

"""

scm_content = scm_content.replace("*(Every subsequent chapter appends its own section here, in the same format: which files, which structures, which functions, and how they call into each other.)*", ch2_ch3 + "*(Every subsequent chapter appends its own section here, in the same format: which files, which structures, which functions, and how they call into each other.)*")
with open(scm_path, "w") as f:
    f.write(scm_content)


# 3. debugging-playbook.md
dp_path = os.path.join(base_dir, "debugging-playbook.md")
with open(dp_path, "r") as f:
    dp_content = f.read()

new_scenarios = """
## Scenario from Ch.2: printk() Message Never Appears

| Step | Content |
|---|---|
| **Symptom** | Added printk() to kernel code, rebuilt, booted, but message never shows in dmesg |
| **Hypotheses** | (1) code guarded by CONFIG_* not enabled; (2) booted wrong kernel (uname -r mismatch); (3) printk priority filtered by console log level; (4) log buffer wrapped |
| **Tools** | grep for #ifdef around your edit, uname -r, cat /proc/sys/kernel/printk, dmesg -T |
| **Fix** | Enable the CONFIG option, or lower the console log level, or confirm correct kernel is booted |

## Scenario from Ch.3: Zombie Process Accumulation

| Step | Content |
|---|---|
| **Symptom** | ps shows many processes in Z (zombie) state, eventually PIDs may be exhausted |
| **Hypotheses** | (1) parent process not calling wait(); (2) parent ignoring SIGCHLD; (3) parent itself is stuck |
| **Tools** | ps aux \| grep Z, /proc/PID/status (check PPid), strace parent process |
| **Fix** | Fix parent to call wait()/waitpid(), or set SIGCHLD to SIG_IGN, or kill parent (init will reap orphaned zombies) |

## Scenario from Ch.3: Process Stuck in D (Uninterruptible Sleep) State

| Step | Content |
|---|---|
| **Symptom** | process shows D state in ps, cannot be killed even with SIGKILL |
| **Hypotheses** | (1) waiting on I/O that never completes (disk failure, NFS timeout); (2) kernel bug causing indefinite wait on a resource; (3) deadlock in kernel code holding the process |
| **Tools** | cat /proc/PID/wchan, /proc/PID/stack (if CONFIG_STACKTRACE), echo t > /proc/sysrq-trigger (shows all task states) |
| **Fix** | Address the underlying I/O/resource issue; if disk, check dmesg for I/O errors; if NFS, check network/mount options |

## Scenario from Ch.3: Fork Bomb (System Unresponsive)

| Step | Content |
|---|---|
| **Symptom** | System becomes unresponsive, many processes spawning rapidly, OOM killer may trigger |
| **Hypotheses** | A process is recursively forking without bound |
| **Tools** | ps aux (if system is responsive), /proc/sys/kernel/pid_max, ulimit -u (per-user process limit) |
| **Fix** | Kill the offending process tree (kill -9 -PGID), set process limits with ulimit or cgroups, use RLIMIT_NPROC |

---
"""

dp_content = dp_content.replace("---\n\n*(This file will grow", new_scenarios + "\n*(This file will grow")
with open(dp_path, "w") as f:
    f.write(dp_content)


# 4. interview-questions.md
iq_path = os.path.join(base_dir, "interview-questions.md")
with open(iq_path, "r") as f:
    iq_content = f.read()

new_qs = """
## Chapter 2 — Getting Started with the Kernel

**Basic**
1. Why doesn't kernel code use the standard C library?
2. What is `.config` and why does it matter?

**Intermediate**
3. Explain the tradeoff `likely()`/`unlikely()` represent.
4. Why is "no memory protection" specifically dangerous in the kernel?

**Advanced / Kernel-Development**
5. List the four reasons kernel code must always be concurrency-aware.
6. Why can't the kernel easily use floating-point arithmetic?

**Design Question**
7. Which of the book's seven kernel programming constraints would you keep as hard requirements for a new OS today?

*(Full detail in `02-getting-started-with-kernel.md`, Section 11.)*

---

## Chapter 3 — Process Management

**Basic**
1. What is the difference between a process and a program?
2. What is `task_struct` and where is it defined?
3. What does the `current` macro give you?

**Intermediate**
4. Explain the five process states. Why does TASK_UNINTERRUPTIBLE exist separately from TASK_INTERRUPTIBLE?
5. How does copy-on-write optimize fork()? Walk through what happens when the child writes to a shared page.
6. Why does Linux not distinguish between threads and processes at the kernel level?

**Advanced / Kernel-Development**
7. Walk through what copy_process() does, step by step.
8. What happens during do_exit()? Why does the process become a zombie instead of being immediately freed?
9. Explain the reparenting mechanism — what happens to a child when its parent exits?
10. How does clone() unify fork() and thread creation? What CLONE_* flags differentiate them?

**Design Question**
11. If you were designing a new OS kernel today, would you use Linux's approach of unifying threads and processes into one abstraction (task_struct), or would you use separate kernel objects? Defend your choice.

*(Full detail in `03-process-management.md`, Interview Questions section.)*

---
"""

iq_content = iq_content.replace("---\n\n## How to Use This File for Review", new_qs + "\n## How to Use This File for Review")
with open(iq_path, "w") as f:
    f.write(iq_content)


# 5. final-knowledge-map.md
fkm_path = os.path.join(base_dir, "final-knowledge-map.md")
with open(fkm_path, "r") as f:
    fkm_content = f.read()

new_mermaid_nodes = """
    Kernel --> Build["Getting Started\\n(source, config, build)"]
    Build --> Constraints["7 Kernel Constraints\\n(no libc, GNU C, etc.)"]
    ProcCtx --> TaskStruct["task_struct\\n(process descriptor)"]
    TaskStruct --> ProcCreate["Process Creation\\n(fork, clone, COW)"]
    TaskStruct --> ProcTerminate["Process Termination\\n(exit, zombie, wait)"]
    TaskStruct --> Threads["Linux Thread Model\\n(threads = processes sharing resources)"]
    TaskStruct --> KernelThreads["Kernel Threads\\n(mm = NULL)"]
"""

fkm_content = fkm_content.replace('    Kernel --> Community["Development Community\\n(mailing lists + git)"]\n```', '    Kernel --> Community["Development Community\\n(mailing lists + git)"]' + new_mermaid_nodes + '```')

new_branches = """
## Chapter 2 Branch — Detail

```
Linux Kernel
+-- Getting Started
|    +-- Kernel Source Tree
|    |    +-- arch/, drivers/, fs/, include/, kernel/, mm/, scripts/
|    +-- Building the Kernel
|    |    +-- .config (configuration symbols)
|    |    +-- vmlinux, bzImage, System.map
|    +-- 7 Kernel Constraints
|    |    +-- No libc or standard headers
|    |    +-- GNU C (inline functions, likely/unlikely)
|    |    +-- No memory protection (oops, panics)
|    |    +-- No floating point easily available
|    |    +-- Small, fixed-size stack
|    |    +-- Synchronization and concurrency are required
|    |    +-- Portability is important
```

---

## Chapter 3 Branch — Detail

```
Linux Kernel
+-- Process Management
|    +-- Process vs. Program vs. Thread
|    +-- task_struct (Process Descriptor)
|    |    +-- State (TASK_RUNNING, TASK_INTERRUPTIBLE, TASK_UNINTERRUPTIBLE, __TASK_STOPPED, __TASK_TRACED)
|    |    +-- thread_info (low-level arch state)
|    |    +-- PID
|    +-- Process Creation
|    |    +-- fork(), vfork(), clone()
|    |    +-- Copy-on-Write (COW)
|    |    +-- do_fork() / kernel_clone() -> copy_process()
|    +-- Thread Implementation
|    |    +-- Threads are just processes sharing resources (clone flags)
|    |    +-- Kernel threads (no user address space, mm = NULL)
|    +-- Process Termination
|    |    +-- exit() -> do_exit()
|    |    +-- EXIT_ZOMBIE state
|    |    +-- wait() / waitpid() -> release_task()
|    |    +-- Reparenting (orphans go to init/subreaper)
```

---
"""

fkm_content = fkm_content.replace("\n---\n\n## Cross-Chapter Dependency Notes", "\n---\n\n" + new_branches + "\n## Cross-Chapter Dependency Notes")

new_deps = """
| Concept | Depends on (from earlier chapters) | Feeds into (later chapters) |
|---|---|---|
| Process context | User/kernel-space boundary (Ch.1), system calls (Ch.1 intro, Ch.5 full) | `task_struct` (Ch.3), scheduling (Ch.4), synchronization (Ch.9-10) |
| Interrupt context | User/kernel-space boundary (Ch.1) | Interrupts (Ch.7), bottom halves (Ch.8), synchronization (Ch.9-10) |
| Monolithic design | Kernel vs. OS (Ch.1) | Loadable modules (Ch.17) |
| `task_struct` | Kernel-space/user-space (Ch.1), process context (Ch.1) | Scheduling (Ch.4), synchronization (Ch.9-10), memory (Ch.12, 15) |
| `fork`/`clone` | `task_struct` (Ch.3), memory concepts (Ch.1) | COW/address space (Ch.15), threading (Ch.3) |
| Process termination | `task_struct` (Ch.3) | Wait queues (Ch.4), signal handling |
| 7 kernel constraints | Kernel vs OS (Ch.1) | ALL later chapters' code |
"""

import re
fkm_content = re.sub(r'\| Concept \| Depends on \(from earlier chapters\) \| Feeds into \(later chapters\) \|.*?(?=\n\n\*\()', new_deps.strip(), fkm_content, flags=re.DOTALL)


with open(fkm_path, "w") as f:
    f.write(fkm_content)

