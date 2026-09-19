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

---

## How to Use This File for Review

Once you have 3+ chapters done, pick 2-3 questions from each completed chapter at random and answer them cold, out loud or in writing, before checking your answer against the source chapter. This spaced-repetition style review is more valuable than re-reading — the goal is retrieval, not recognition.

*(Chapters 2-20 will each append their own Basic / Intermediate / Advanced / Design sections here as they are completed.)*