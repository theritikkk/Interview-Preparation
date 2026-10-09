# Interview Questions — Aggregated Bank

> Every interview question from every chapter, collected here for review passes. Organized by chapter, then difficulty. Use this file once you've completed several chapters and want a mixed-review session rather than a chapter's own linear question list.

---

## Chapter 1 — Introduction to the Linux Kernel

**Basic**
1. What is the difference between an operating system and a kernel?
2. What is a system call?

**Intermediate**
3. Why is Linux considered "pragmatic" in its kernel design?
4. Explain process context vs. interrupt context, and why the distinction matters for what kernel code is allowed to do.

**Advanced / Kernel-Development**
5. Walk through what physically happens (privilege-level-wise) when a user-space program calls `read()`.
6. The book describes Linux 2.6's odd/even version-parity scheme. Explain what replaced it, and why the change was made.

**Design Question**
7. Design the versioning/release scheme you would use for a brand-new open-source kernel project today, given both the 2.6-era approach and the current time-based approach. What would you keep, what would you change?

*(Full expected-reasoning/deeper-follow-up detail for each question is in `01-introduction-to-linux-kernel.md`, Section 11 — this file is the compact index for review, not a replacement for that detail.)*


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

## Chapter 4 — Process Scheduling

**Basic**
1. What is the difference between cooperative and preemptive multitasking?
2. What is a nice value, and how does it affect scheduling priority?
3. What is the difference between an I/O-bound and a CPU-bound process?

**Intermediate**
4. Explain how CFS achieves fairness without fixed timeslices. What is vruntime?
5. Walk through what happens during a context switch (schedule → pick_next_task → context_switch).
6. What is kernel preemption, and why does it matter for responsiveness?
7. Explain the scheduler class hierarchy and how the scheduler picks which class to consult first.

**Advanced / Kernel-Development**
8. Explain the relationship between nice values, weights, and vruntime advancement rate in CFS.
9. Why did Linux replace CFS with EEVDF? What problem does EEVDF address that CFS did not?
10. Why can't you call schedule() (or any sleeping function) while holding a spinlock? Connect this to preemption.
11. Describe how wait queues work and the standard pattern for sleeping until a condition is true.

**Design Question**
12. Design a scheduler for a system with 1000 CPU cores running a mix of latency-sensitive microservices and batch computation jobs. What scheduling policies would you use, and how would you handle CPU affinity and NUMA topology?

*(Full detail in `04-process-scheduling.md`, Interview Questions section.)*


---

## Chapter 5 — System Calls

**Basic**
1. What is the difference between an API, a system call, and a C library function?
2. What is a system call number, and why can it never be changed once assigned?
3. How does user-space invoke a system call on x86-64?

**Intermediate**
4. Walk through the complete journey of a `read()` call from user-space C code to the kernel handler and back.
5. Why must the kernel use `copy_from_user()`/`copy_to_user()` instead of raw `memcpy()` when handling user-space pointers?
6. What is the vDSO, and which system calls does it accelerate?
7. What is `errno`, and who actually sets it — the kernel or the C library?

**Advanced / Kernel-Development**
8. Explain the `SYSCALL_DEFINEn()` macro family. Why are macros used instead of plain function definitions?
9. What hardware mechanisms (SMAP/SMEP) protect against the kernel accidentally accessing user-space memory?
10. Why is adding new system calls discouraged in modern Linux? What alternatives exist?

**Design Question**
11. Design the system call interface for a new IPC mechanism. What arguments would you choose, how would you handle the flags parameter for future extensibility, and how would you verify user-space parameters?

*(Full detail in `05-system-calls.md`, Interview Questions section.)*


---


---

## Chapter 6 — Kernel Data Structures

**Basic**
1. What is `struct list_head` and how is it different from a traditional linked list node?
2. What does `container_of()` do, and why is it so important in kernel code?
3. Name three kernel subsystems that use red-black trees.

**Intermediate**
4. Explain how `container_of()` works using `offsetof()` pointer arithmetic. Walk through a concrete example.
5. Why is `list_for_each_entry_safe()` needed? When would using the non-safe version cause a crash?
6. Why must kfifo sizes be a power of 2? What advantage does this provide over modulo arithmetic?
7. Compare the kernel's "intrusive" linked list design (embedding list_head in structs) with the traditional approach (list nodes wrapping data pointers). What are the advantages?

**Advanced / Kernel-Development**
8. How are red-black tree insertion and deletion different from a normal BST? Why does the rbtree API split insertion into rb_link_node() + rb_insert_color()?
9. Explain why the kernel's linked list operations provide NO locking. How does this interact with the concurrency requirements from Ch.2 and the synchronization primitives from Ch.9-10?
10. How does kfifo achieve lock-free operation for the single-producer/single-consumer case?

**Design Question**
11. You're designing a subsystem that needs to track 100,000 objects by both a unique integer ID and a sorted key. Which kernel data structures would you combine, and how would you handle the memory/performance tradeoffs?

*(Full detail in `06-kernel-data-structures.md`, Interview Questions section.)*

## How to Use This File for Review

Once you have 3+ chapters done, pick 2-3 questions from each completed chapter at random and answer them cold, out loud or in writing, before checking your answer against the source chapter. This spaced-repetition style review is more valuable than re-reading — the goal is retrieval, not recognition.

---

## Chapter 7 — Interrupts and Interrupt Handlers

**Basic**
1. What is an interrupt, and how does it differ from a system call?
2. What is the difference between an interrupt and an exception?
3. Why can't an interrupt handler sleep or call schedule()?

**Intermediate**
4. Explain the top-half / bottom-half split. Why is it necessary?
5. Walk through the journey of a hardware interrupt from device to handler and back.
6. What is a shared interrupt line, and how does the kernel handle it?
7. What is the difference between local_irq_disable() and disable_irq()?

**Advanced / Kernel-Development**
8. Explain interrupt context constraints in detail. What can and cannot be done?
9. What is request_threaded_irq() and how does it differ from request_irq()? Why was it introduced?
10. How do MSI/MSI-X interrupts differ from traditional pin-based interrupts? What advantages do they provide?
11. Explain the role of /proc/interrupts and how you would use it to debug an interrupt-related performance issue.

**Design Question**
12. You're designing a high-throughput network card driver. How would you split work between the top half and bottom half? What if the packet rate is so high that the interrupt handler itself becomes a bottleneck? (Hint: consider NAPI polling.)

*(Full detail in `07-interrupts.md`, Interview Questions section.)*

*(Chapters 2-20 will each append their own Basic / Intermediate / Advanced / Design sections here as they are completed.)*