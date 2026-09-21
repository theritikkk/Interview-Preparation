# OS → Processes, Threads & System Calls

**GATE 2027 syllabus:** Operating System — system calls, processes, threads, inter-process communication.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** MCQ = exactly one correct option. MSQ = one or more correct options. NAT = numerical answer. Unless stated otherwise, every `fork()` succeeds and `printf` output appears immediately (unbuffered) and atomically.

---

## Questions

### Q1 · MSQ · Fundamental
Which of the following is/are private to each thread of a multithreaded process (that is, **not** shared with the sibling threads)?

- (A) Program counter
- (B) Runtime stack
- (C) Global (static) variables
- (D) Contents of the CPU registers (saved register set)

---

### Q2 · MCQ · Fundamental
Which of the following events that transfer control from a user process to the kernel is **asynchronous**, i.e. NOT caused by the instruction the process is currently executing?

- (A) Execution of a trap instruction to invoke a system call
- (B) A page fault raised by a load instruction
- (C) Arrival of a timer interrupt
- (D) A division-by-zero exception

---

### Q3 · MSQ · Standard
A system's TLB has **no** address-space identifiers (ASIDs). The CPU switches from thread T1 of process A to thread T2 of process B. Which of the following actions is/are required for this switch but are **NOT** required when the CPU switches from T1 to another thread T3 of the *same* process A?

- (A) Loading the page-table base register with that of the new address space
- (B) Invalidating (flushing) the TLB contents
- (C) Restoring the stack pointer of the incoming thread
- (D) Restoring the general-purpose registers of the incoming thread

---

### Q4 · NAT · Standard
```c
#include <stdio.h>
#include <unistd.h>
int main() {
    if (fork() || fork())
        fork();
    printf("X\n");
    return 0;
}
```
The number of times `X` is printed (over all processes) is ______.

---

### Q5 · MCQ · Standard
A process uses a **many-to-one** threading model (many user-level threads mapped onto a single kernel thread). One of its user-level threads makes a blocking `read()` system call on a slow device. Which one of the following is TRUE?

- (A) Only the calling thread blocks; the other threads of the process continue to run
- (B) The entire process blocks, because the kernel schedules only one entity for the process
- (C) The kernel automatically creates a new kernel thread so that the other user threads can continue
- (D) All other user-level threads of the process are terminated

---

### Q6 · MSQ · Standard
Which of the following statements about inter-process communication is/are TRUE?

- (A) Once a shared-memory segment has been set up, every data transfer between the communicating processes requires a system call.
- (B) An anonymous pipe created by `pipe()` before a `fork()` can be used for communication between the parent and the child.
- (C) Message passing can be used between processes running on different machines, whereas plain shared memory cannot.
- (D) A named pipe (FIFO) can be used by two processes that have no parent–child relationship.

---

### Q7 · NAT · Moderate
```c
int main() {
    pid_t pid = fork();
    if (pid == 0) printf("C");
    else          printf("P");
    printf("E");
    return 0;
}
```
The parent and the child may be scheduled in any relative order (each `printf` call is atomic). The number of **distinct output strings** that can appear on the terminal is ______.

---

### Q8 · MSQ · Moderate
A process performs a successful `execve()` to run a new program. Assume default settings (no close-on-exec flags). Which of the following is/are preserved across the call?

- (A) The process ID
- (B) The open file descriptors
- (C) The contents of the old heap
- (D) The contents of the old user stack

---

### Q9 · NAT · Hard
```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main() {
    for (int i = 0; i < 4; i++) {
        pid_t p = fork();
        if (p == 0 && i % 2 == 1)
            exit(0);          /* child terminates immediately */
    }
    printf("X\n");
    return 0;
}
```
Let **C** be the total number of child processes ever created (counting every `fork`, whether or not the child later terminates) and **L** the number of lines printed. Then C − L = ______.

---

### Q10 · MSQ · Killer
The file `data.txt` contains the six bytes `ABCDEF`. A process opens it with `fd = open("data.txt", O_RDONLY)` and then calls `fork()`. Afterwards the parent and the child each execute exactly once:

```c
read(fd, buf, 2);      /* then use the two bytes read */
```

Let **c** be the two bytes read by the child and **p** the two bytes read by the parent. Each `read()` is atomic and nobody else touches the file. Which of the following pairs (c, p) is/are possible?

- (A) c = "AB", p = "CD"
- (B) c = "AB", p = "AB"
- (C) c = "CD", p = "AB"
- (D) c = "CD", p = "CD"

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | A, B, D | Globals and heap are shared |
| 2 | C | Timer interrupt is external; the others are caused by the running instruction |
| 3 | A, B | Only a change of address space needs new page-table base and TLB flush |
| 4 | 5 | Parent of 1st fork short-circuits to true and forks again (2 processes); child of 1st fork runs the 2nd fork: its parent branch forks again (2), the 2nd-fork child skips (1), so 2 + 3 = 5 |
| 5 | B | Kernel sees one schedulable entity |
| 6 | B, C, D | Shared memory needs the kernel only for setup |
| 7 | 4 | CEPE, CPEE, PCEE, PECE |
| 8 | A, B | New program replaces heap and stack |
| 9 | 5 | C = 1 + 2 + 2 + 4 = 9, L = 4 (children forked at odd i exit) |
| 10 | A, C | Parent and child share one open-file description, hence one offset |
