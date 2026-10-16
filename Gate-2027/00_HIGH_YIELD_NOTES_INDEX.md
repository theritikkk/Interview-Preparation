# GATE CS 2027 — High-Yield Topics: Notes & Practice Index

**Source:** topic list from Gothi's high-yield breakdown — ~30 topics across 10 subjects that consistently account for **~60 marks**. Strategy from the video: cover these first, practice them *thoroughly* (practice > passive coverage), and if time is short, prioritize **C Programming, Data Structures, TOC** (most predictable), tackling **Discrete Math + Engineering Math last** (13 marks combined but slowest to prepare).

This index tracks the **explanatory notes** (concept-first, worked examples, traps, verified practice sets) that complement the existing 88-file / 880-question question banks in each subject folder.

---

## How the two layers fit together

| Layer | What it is | Where |
|---|---|---|
| Notes | Explanation, derivations, worked examples, traps | `<Subject>/Notes/*.md` |
| Question banks | 10 questions + answer key per file, GATE-style | `<Subject>/*.md` (flat) |
| Source handbooks | PW GATE subject handbooks, extracted to text (one per subject) | `Reference/PW_Subject_Handbooks/PW_*.txt` |

Every notes file ends with a practice set whose numeric answers were **verified by simulation/computation**, and points to 1–3 question-bank files for more reps. Notes are grounded in the PW handbooks where they exist (each file cites its `PW_*.txt` section).

---

## Status dashboard

### Operating Systems — COMPLETE (4 notes files)
| Video topic | Notes file | Practice set | Question banks |
|---|---|---|---|
| Memory Management | `OS/Notes/OS_Notes_01_Memory_Management_Paging.md` | 10 Q verified | `OS_05`, `OS_06`, `OS_07` |
| (Virtual memory / paging) | `OS/Notes/OS_Notes_02_Page_Replacement_Virtual_Memory.md` | 10 Q verified | `OS_08` |
| Process Synchronization | `OS/Notes/OS_Notes_03_Process_Synchronization.md` | 10 Q verified | `OS_03` |
| Scheduling | `OS/Notes/OS_Notes_04_CPU_Scheduling.md` | 10 Q verified | `OS_02` |

### Computer Networks — COMPLETE (2 notes files)
| Video topic | Notes file | Practice set | Question banks |
|---|---|---|---|
| IP & IP Addressing | `CN/Notes/CN_Notes_01_IP_and_IP_Addressing.md` | 10 Q verified | `CN_06`, `CN_07` |
| TCP + Flow Control | `CN/Notes/CN_Notes_02_TCP_and_Flow_Control.md` | 10 Q verified | `CN_08`, `CN_09` |

### Computer Organization & Architecture — COMPLETE (3 notes files)
| Video topic | Notes file | Practice set | Question banks |
|---|---|---|---|
| Instruction Pipelining | `COA/Notes/COA_Notes_01_Instruction_Pipelining.md` | 10 Q verified | `COA_06` |
| Cache Memory | `COA/Notes/COA_Notes_02_Cache_Memory.md` | 10 Q verified | `COA_04` |
| IEEE 754 Floating Point | `COA/Notes/COA_Notes_03_IEEE754_Floating_Point.md` | 10 Q verified | `DL_03` |

### C Programming & Data Structures — COMPLETE (4 notes files)
| Video topic | Notes file | Practice set | Question banks |
|---|---|---|---|
| C (pointers, functions, recursion, storage classes) | `DSA/Notes/DSA_Notes_01_C_Programming.md` | 10 Q verified (compiled) | `DSA_08`, `DSA_01` |
| Trees (traversals, BST, heaps, AVL) | `DSA/Notes/DSA_Notes_02_Trees.md` | 10 Q verified | `DSA_03`, `DSA_04` |
| Hashing | `DSA/Notes/DSA_Notes_03_Hashing.md` | 10 Q verified | `DSA_05` |
| Stack & Queue | `DSA/Notes/DSA_Notes_04_Stacks_Queues.md` | 10 Q verified | `DSA_01` |

### Theory of Computation — COMPLETE (3 notes files)
| Video topic | Notes file | Practice set | Question banks |
|---|---|---|---|
| Finite Automata | `TOC/Notes/TOC_Notes_01_Finite_Automata_Regular_Languages.md` | 10 Q verified | `TOC_01`, `TOC_02` |
| PDA | `TOC/Notes/TOC_Notes_02_PDA_Context_Free_Languages.md` | 10 Q verified | `TOC_03`, `TOC_04` |
| Language Detection | `TOC/Notes/TOC_Notes_03_Language_Detection_Classification.md` | 10 Q verified | `TOC_02`, `TOC_04` |

### DBMS — COMPLETE (4 notes files)
| Video topic | Notes file | Practice set | Question banks |
|---|---|---|---|
| Relational Algebra & SQL | `DBMS/Notes/DBMS_Notes_01_Relational_Algebra_SQL.md` | 10 Q verified | `DB_03`, `DB_05`, `DB_06` |
| Normalization & FD (easiest marks) | `DBMS/Notes/DBMS_Notes_02_Functional_Dependencies_Normalization.md` | 10 Q verified (brute-force) | `DB_07` |
| Indexing / B & B+ trees (preferred) | `DBMS/Notes/DBMS_Notes_03_Indexing_BPlus_Trees.md` | 10 Q verified (incl. B+ simulation) | `DB_08`, `DB_09` |
| Transactions & Concurrency | `DBMS/Notes/DBMS_Notes_04_Transactions_Concurrency_Recovery.md` | 10 Q verified (exhaustive graphs) | `DB_10`, `DB_11` |

### Algorithms — COMPLETE (3 notes files)
| Video topic | Notes file | Practice set | Question banks |
|---|---|---|---|
| Sorting | `DSA/Notes/DSA_Notes_05_Sorting_Algorithms.md` | 10 Q verified (traced) | `DSA_06` |
| Time Complexity | `DSA/Notes/DSA_Notes_06_Time_Complexity_Recurrences.md` | 10 Q verified (computed) | `DSA_07` |
| Graph Algorithms | `DSA/Notes/DSA_Notes_07_Graph_Algorithms.md` | 10 Q verified (BFS/Dijkstra/BF/Kruskal/SCC simulated) | `DSA_12`, `DSA_13` |

### Digital Logic — COMPLETE (3 notes files)
| Video topic | Notes file | Practice set | Question banks |
|---|---|---|---|
| Number Systems | `DL/Notes/DL_Notes_01_Number_Systems_Complements_Codes.md` | 10 Q verified (conversions/complements/BCD simulated) | `DL_02` |
| Boolean Algebra | `DL/Notes/DL_Notes_02_Boolean_Algebra_Minimization.md` | 10 Q verified (Quine–McCluskey engine) | `DL_01` |
| Combinational Circuits | `DL/Notes/DL_Notes_03_Combinational_Circuits.md` | 10 Q verified (adders/MUX/decoder/codes simulated) | `DL_04` |

### Compiler Design — COMPLETE (3 notes files)
| Video topic | Notes file | Practice set | Question banks |
|---|---|---|---|
| Parsers (top-down / LL(1)) | `CD/Notes/CD_Notes_01_Parsing_TopDown_LL1.md` | 10 Q verified (LL(1) table builder + stack simulator) | `CD_02` |
| Parsers (bottom-up / LR family) | `CD/Notes/CD_Notes_02_Parsing_BottomUp_LR.md` | 10 Q verified (LR(0)/SLR/CLR/LALR simulator) | `CD_03` |
| Syntax-Directed Translation | `CD/Notes/CD_Notes_03_Syntax_Directed_Translation.md` | 10 Q verified (SDD evaluators) | `CD_04` |

### Discrete Mathematics — COMPLETE (3 notes files)
| Video topic | Notes file | Practice set | Question banks |
|---|---|---|---|
| Graph Theory | `DM/Notes/DM_Notes_01_Graph_Theory.md` | 10 Q verified (Matrix–Tree / colouring / matching simulations) | `DM_06`, `DM_07`, `DM_08` |
| Propositional & Predicate Logic | `DM/Notes/DM_Notes_02_Logic_Propositional_Predicate.md` | 10 Q verified (truth-table + finite-model enumerators) | `DM_01`, `DM_02` |
| Sets, Relations & Functions | `DM/Notes/DM_Notes_03_Sets_Relations_Functions.md` | 10 Q verified (exhaustive relation enumeration) | `DM_03`, `DM_04` |

### Engineering Mathematics — COMPLETE (3 notes files) — the final subject
| Video topic | Notes file | Practice set | Question banks |
|---|---|---|---|
| Linear Algebra (matrices, systems, eigen, LU) | `EM/Notes/EM_Notes_01_Linear_Algebra.md` | 10 Q verified (exact fraction row-reduction / adjugate computation) | `EM_01`, `EM_02` |
| Calculus (limits, MVT, extrema, integration) | `EM/Notes/EM_Notes_02_Calculus.md` | 10 Q verified (symbolic + numerical quadrature) | `EM_03`, `EM_04`, `EM_05` |
| Probability & Statistics | `EM/Notes/EM_Notes_03_Probability_Statistics.md` | 10 Q verified (exact fractions / distribution computations) | `EM_06`, `EM_07`, `EM_08` |

### Warm-up / revision assets (outside the high-yield list, already exist)
- Deadlocks (`OS_04`), Processes & Threads (`OS_01`), File Systems (`OS_09`), Disk Scheduling (`OS_10`)
- CN performance, framing, MAC, routing, application layer (`CN_01`–`CN_05`, `CN_10`, `CN_11` mixed advanced)
- Full question-paper practice: `Question-Papers/`

---

## Suggested order (aligned with the video's advice)

1. **OS notes 1→4** (done — start reviewing; synchronization can be time-boxed)
2. **CN**: IP addressing first (easiest marks), then TCP + flow control (done)
3. **COA**: pipelining, cache, IEEE 754 (done)
4. **C + Data Structures** (video: highest scoring-per-hour; also helps placements) (done)
5. **TOC** (FA/PDA are practice-heavy but predictable) (done)
6. **DBMS** (normalization easiest → then relational algebra/SQL → indexing/transactions) (done)
7. **Algorithms** (sorting → complexity → graphs) (done)
8. **Digital Logic** (number systems/Boolean are quick wins; also feeds COA and CN) (done)
9. **Compiler Design** (2–3 mark subject; parsers + SDT only) (done)
10. **Discrete Math + Engineering Math LAST** per the video (13 marks, but lengthy) — Discrete Math done; Engineering Math done. ← ALL 10 SUBJECTS COMPLETE: switch to revision mode (banks → `Question-Papers/`)

---

## Conventions used in all notes files

- Reference strings, traces and numeric answers were **checked by running code** (simulators for page replacement/scheduling, exact computation for address sizes/EMAT, exhaustive interleaving for synchronization puzzles).
- Practice sets use the same format as the question banks: `Qn · MCQ/MSQ/NAT · difficulty`, then an answer key with condensed reasoning + full solutions.
- Where a convention matters (RR tie-breaks, initially-empty frames, "completed" semaphore ops), it is stated **inside the question** so the answer is unique.

---

*Notes built: OS (4 files), CN (2), COA (3), C + DS (4), TOC (3), DBMS (4), Algorithms (3), Digital Logic (3), Compiler Design (3), Discrete Mathematics (3), Engineering Mathematics (3) — 35 subject files, 350 verified practice questions, all 10 subjects complete. PW handbooks extracted for all 12 subjects in `Reference/PW_Subject_Handbooks/`. Next: revision — run the subject question banks cold, then full-length practice from `Question-Papers/`.*
