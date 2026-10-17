# Low-Level Design — Senior Engineer's Notes

### From the metal up, to the staff table

> These notes are written for engineers operating at **SDE3 level and above** — people who have shipped systems, sat in incident reviews, and now need their design vocabulary to be as precise as their code. Every topic is explained **from the ground up** (no prerequisites assumed, every term given its word-by-word meaning) and then pushed to the depth a senior interview or a design review will actually demand: cost models, failure modes, and the judgment to know **when not** to apply what you know.

Seed material: **Mukul Raina's 5-hour LLD course** (linked in §8). The slides are used as a **topic checklist and reference only** — the structure, explanations, code, diagrams, and judgment layers in these files are written independently, in the shape a working senior engineer would write them for a peer.

---

## 1. Who These Notes Are For

| Reader | What you get |
|---|---|
| **SDE3 / senior interview candidate** | Every pattern and principle stated precisely enough to defend under follow-up questioning, with the trade-offs and failure modes interviewers probe for |
| **Working engineer moving up a level** | The judgment layer: why designs fail in production, what reviews flag, where patterns pay rent and where they don't |
| **Engineer returning to fundamentals** | The bottom layer: memory layout, dispatch mechanics, ownership — explained without hand-waving, in three languages (C, C++, Java) |

**What these notes deliberately do not do:** they do not transcribe slides, they do not present patterns as recipes, and they do not pretend every decision has a right answer. Every non-trivial claim comes with its cost stated.

---

## 2. The Two Layers of Every Topic

Every concept in every file is written in two layers. You can read either one first; they interleave.

**Layer 1 — Fundamentals, from zero.**
- **Word-by-word meaning** of the term (roots, parts, what each contributes)
- A one-sentence precise definition
- The problem the concept exists to solve — and what breaks without it
- Mechanics with diagrams (memory layouts, call flows, class structures)
- Tri-language code: **C, C++, Java**, with the part that matters (ownership, lifetimes, dispatch, thread-safety) explained rather than the syntax

**Layer 2 — Judgment.**
- Production failure modes: what this looks like during an incident
- Cost models: what you pay in complexity, latency, indirection, test surface
- Code-review heuristics: how a senior reviewer catches misuse in one look
- The interview layer: what gets asked at SDE3 level, and the model answers

Example of a "word-by-word" block, so you know what to expect:

> **Word by word — *polymorphism*:** from Greek **polys** (many) + **morphē** (form). "Many forms." One name, many concrete behaviors. In languages: **one interface, one call site — the object decides which code runs.** Two flavors exist and interviewers care about the difference: *compile-time* polymorphism (overloading — the compiler picks) and *runtime* polymorphism (overriding/dispatch — the object picks).

---

## 3. Conventions in These Files

**Markers used inline:**

| Marker | Meaning |
|---|---|
| **[INTERVIEW]** | How this surfaces in senior interviews; model answers and follow-ups |
| **[PRODUCTION]** | What this looks like in real systems; failure modes, scale notes |
| **[KERNEL & SYSTEMS]** | The same idea as it appears in operating systems, databases, runtimes — cross-domain anchors |
| **[MISCONCEPTION]** | The wrong model people carry, and the correction |
| **[DRILL]** | Self-test prompt; model answer given |
| **[DEEP DIVE]** | Optional depth — read when you want the mechanism, skip when time-boxed |

**Code conventions:**
- **C** represents "no language-level OOP": structs + function-pointer tables — the way kernels actually implement polymorphism. C code is written the way real systems C is written (explicit ownership, error codes, no exceptions).
- **C++** shows RAII and value semantics where they matter: the language you reach for when you control the layout and the lifetime.
- **Java** shows the managed-runtime view: interfaces, dynamic dispatch, GC pressure.
- Every code block is compilable-as-written unless marked `// sketch`. Where a snippet is intentionally abbreviated, the important parts (invariants, edge cases) are commented rather than silently dropped.

**Diagram conventions:**
- **Mermaid** for structure and sequence: class diagrams, call flows, state machines, sequence diagrams.
- **ASCII** for memory-level reality: object layouts, vtables, stacks, buffers. Mermaid cannot show a byte layout; ASCII can.

**The example ladder:** examples climb three rungs where it helps — a **toy** (minimal, clear), a **production-grade** slice (payment flows, notification systems, file storage — the shapes interview questions use), and a **systems/kernel** analogue (VFS, TCP, allocators). The ladder is not decoration: seeing the same idea at three scales is how the *judgment* layer forms.

---

## 4. File Map

| File | Title | What it covers | Layer-2 payoff |
|---|---|---|---|
| **00** | **This index** | How to read, conventions, study paths, progress | — |
| **01** | `01_OOP_PRINCIPLES.md` — *Objects and the Metal* | What objects actually are; stack/heap/pointers; class vs object vs instance; the four pillars word-by-word; dispatch mechanics (vtables, invokevirtual, ops tables); composition vs inheritance as an engineering decision; cohesion & coupling; relationship vocabulary | Why OOP arguments are usually memory-model arguments in disguise; how to settle "inheritance vs composition" in one sentence |
| **02** | `02_SOLID_PRINCIPLES.md` — *SOLID as Change Economics* | Each of the five principles word-by-word; violation detection in real code; tri-language refactorings; when SOLID advice is wrong | SOLID as a cost-allocation framework, not a moral code; how to detect violations in 30 seconds of reading |
| **03** | `03_CREATIONAL_PATTERNS.md` — *Controlling Construction* | Factory Method, Abstract Factory, Builder, Singleton (+ DI/registries); creation as a boundary you must own | When a factory is a real seam vs ceremony; why Singleton survives in production anyway and where it must never live |
| **04** | `04_STRUCTURAL_PATTERNS.md` — *Wrapping and Bridging* | Adapter, Decorator, Facade, Proxy (virtual, protection, remote, caching) | Detecting wrapper-debt in codebases; the layer-count rule; Adapter as the anti-corruption boundary |
| **05** | `05_BEHAVIORAL_PATTERNS.md` — *Orchestrating Behavior* | Strategy, Observer, Command, State, Chain of Responsibility, Template Method; pattern combinations | State machines that survive concurrency; event systems that don't melt during incidents; undo that works month two |
| **06** | `06_PATTERN_SELECTION_AND_ANTIPATTERNS.md` — *Judgment* | The selection process; over-engineering & minimum viable design; six+ anti-patterns with tri-language fixes; frequency data | The judgment file: how seniors choose, refuse, and refactor; anti-pattern vocabulary for interviews |
| **07** | `07_CHEATSHEET_CLOSING.md` — *Field Reference* | Dense recall tables: signals, combinations, trade-offs, checklists, exam-day card | The last-48-hours file; nothing new, everything compressed |
| **08** | `08_REMAINING_GOP_PATTERNS.md` — *The Other Nine* | Prototype, Bridge, Composite, Flyweight, Iterator, Mediator, Memento, Visitor, Interpreter | The follow-up-question file: patterns that surface as sub-questions in senior loops |
| **09** | `09_INTERVIEW_PLAYBOOK.md` — *The Room* | Senior interview mechanics; full case studies (parking lot, library, and more) end-to-end; follow-up barrages; defense drills; grading rubric | What "strong hire" sounds like out loud, with transcripts |

---

## 5. The Philosophy in One Paragraph

**Patterns are vocabulary; judgment is the product.** Anyone can name Strategy — the difference at senior level is knowing when a Strategy interface is a genuine seam and when it is ceremony, what the extra indirection costs during an incident at 2am, and how to get five engineers to converge on the same boundary without a meeting. Every pattern in these files is presented as **a coupling decision with a price tag**, because that is what it is. Every principle is presented as **economics: who pays, who benefits, and when the trade flips.** If you finish these notes able to name fewer patterns faster and justify each one cheaper, they have done their job.

---

## 6. Study Paths

**Path A — Linear mastery (2–3 weeks, working engineer pace).**
`01 → 02 → 03 → 04 → 05 → 08 → 06 → 09`, using `07` as a lookup table throughout. Do the drills in each file; they are written to be answered aloud in under two minutes.

**Path B — Interview sprint (10 days).**
Day 1–2: `06` (selection + anti-patterns) and `09` (playbook + case studies) — know the target first. Day 3–6: `03` and `05` (the two Tier-1 pattern files), drills only. Day 7: `04` + `08` skim with signals table. Day 8: `02` violation-detection tables. Day 9: full 45-minute timer simulations from `09`. Day 10: `07` end-to-end, plus your weakest drill set twice.

**Path C — Night before.**
`07` §signals + trade-offs, then `09`'s exam-day card. Nothing else. Sleep.

---

## 7. Progress Tracker (Rewrite v2)

This tracker records the **v2 rewrite** (this style). The v1 slide-anchored drafts are preserved in `archive-v1/` for reference and salvage — some of their material (kernel parallels, drills) is being carried forward in rewritten form.

| File | Status | Notes |
|---|---|---|
| `00_INDEX.md` | ✅ done | This file |
| `01_OOP_PRINCIPLES.md` | ✅ done | ~1,840 lines — machine model, vocabulary, four pillars, dispatch mechanics, relationships, cohesion/coupling, principle table, 14-question bank, drills |
| `02_SOLID_PRINCIPLES.md` | ✅ done | ~1,480 lines — each principle word-by-word with original sources, detection instruments, tri-language fixes, overshoot catalogue, interview bank, drills |
| `03_CREATIONAL_PATTERNS.md` | ✅ done | ~1,265 lines — Factory Method (5 flavors), Abstract Factory (families), Builder (gate + both eras), Singleton (unbundled: uniqueness vs access), creation decision matrix |
| `04_STRUCTURAL_PATTERNS.md` | ✅ done | ~1,250 lines — universal wrapper + receipt; Adapter (ownership rule, error/unit mapping); Decorator (2^N, chain order as semantics, middleware); Facade (sequence/transaction ownership, thinness rule, graduation); Proxy (access axes, self-invocation, transparency discipline, RPC honesty); wrapper anti-patterns |
| `05_BEHAVIORAL_PATTERNS.md` | ✅ done | ~1,895 lines — Strategy (kernel sched_class), Observer (notifier chains, leak discipline), Command (workqueues, outbox), State (TCP/codec locking, Gen 1 vs Gen 2 tables), Chain of Responsibility (netfilter verdicts), Template Method (driver probe, CRTP); inversion matrix, composition map, symptom→pattern table, behavioral anti-patterns, file-level interview bank, classifier/refactor/audit drills, rapid-fire |
| `06_PATTERN_SELECTION_AND_ANTIPATTERNS.md` | ✅ done | ~1,365 lines — anti-patterns at four magnifications (class/change/dependency/system: God Class, Envy, Intimacy, Shotgun vs Divergent, cycles, premature abstraction, BBoM + BKL case study, lava flow, golden hammer, copy-paste) with tri-language fixes; the degeneration map; the three questions + five-step selection; the master signal→pattern matrix (all 14); four worked walks (notification, checkout, chat, files) with refusals spoken; cost ledger for every pattern; MVD + deferred ledger; the six-level maturity ladder; frequency tiers; ReportManager spotting exercise; rapid-fire |
| `07_CHEATSHEET_CLOSING.md` | ✅ done | ~675 lines — field reference/index: three reading modes + judgment glossary; OOP and SOLID compressed (pillars, five decisions, type map, violation tests, triad); all 23 patterns in master recall tables (intent/signal/mechanic/shadow); family decision grids; the inverted word→structure lookup; combinations + never-confuse matrix; anti-pattern recognition + degeneration map + repair doctrine; per-language implementation anchors (C/C++/Java mechanism map); the six reusable checklists (selection, four questions, MVD/ledger, review, reliability contracts, concurrency, delivery); trade-off pairs + the statement format; frequency tiers + practice ladder + calendar; the one-screen exam-day card |
| `08_REMAINING_GOP_PATTERNS.md` | ✅ done | ~1,310 lines — the remaining nine GoF patterns (Prototype, Bridge, Composite, Flyweight, Iterator, Mediator, Memento, Visitor, Interpreter): word-by-word etymology + GoF verbatim per pattern, tri-language code, kernel parallels (fork/skb_clone, driver/bus model, filesystems/cgroups, page cache/inode interning/KSM, list_for_each/readdir/RCU, buses/netlink, hibernation/CRIU, eBPF verifier/ACPI walkers, BPF/SELinux/nftables bytecode), when-not tables, the Bridge/Strategy/Adapter and Facade/Observer/Mediator triangles, the safety/transparency and snapshot-vs-replay comparisons, expression problem, the nine-in-one priced table, follow-up interview bank, rapid-fire |
| `09_INTERVIEW_PLAYBOOK.md` | ✅ done | ~740 lines — the performance file: the six interview formats and what each grades; the rubric as sound (weak/strong/outstanding phrases); the five-phase delivery with timing tables and the four timing mistakes; clock strategy + stuck-recovery ladder; the eight narration habits; four full case studies (parking lot as the complete five-phase transcript with follow-up barrage and debrief; library with the title/copy lesson; checkout assembled whole with the five spine sentences; chat as the surviving-the-barrage study); the barrage bank (six attack families with response templates + the four fallback sentences); defense drills with transcripts (over-engineering critique, contract audit, hostile pushback with hold/update/concede, self-correction, interruptions); the LRU machine-coding round; the self-audit table (weakness→source→drill), the strong-hire paragraph, the final practice protocol, the red flags; course closing |

---

## 8. Source & Attribution

- **Seed material:** Mukul Raina's 5-hour Low-Level Design course — https://www.youtube.com/watch?v=VSABVEBDhf0 (channel: https://www.youtube.com/@mukulraina, slides: slides.mukulraina.com). The course's coverage — OOP → SOLID → 14 design patterns → selection framework → anti-patterns → interview closing — is treated as the **topic checklist** for files 01–07.
- **Beyond the seed:** files 08–09 and every `[KERNEL & SYSTEMS]`, `[PRODUCTION]`, `[DEEP DIVE]`, and tri-language section are written independently for this notes set. Kernel parallels (VFS ops tables, TCP state machines, notifier chains) draw on the Linux kernel; if you also keep kernel notes in this repo, the cross-references will be obvious.
- **This is v2.** v1 drafts (slide-anchored) live in `LLD-Course/archive-v1/`.

---

*Begin with `01_OOP_PRINCIPLES.md`. It starts where the machine does.*