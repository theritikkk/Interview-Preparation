# How LLM Computer-Use Agents Work — A Study Series

> **What this is**
> A structured, multi-file set of notes on how LLM-based computer-use agents (CUAs) actually operate a computer: how they see the screen, how they click, how they read app structure, and how the cloud machines they live on are built. Follow the files in order, or jump to whichever path you need.

![The agent loop](diagrams/agent-loop.svg)

---

## The two questions this series answers

Every computer-use agent, no matter which product it ships in, has to answer exactly two questions on repeat:

1. **"What is on the screen right now, and where?"** — the *perception* problem. The model's knowledge ends at its context window; everything it knows about the current machine state must be pushed into that window as tokens — as an image, an accessibility tree, DOM text, or all three.
2. **"How do I turn that knowledge into an action that the operating system honors?"** — the *control* problem. The model cannot touch hardware. Something outside it must translate `click(512, 384)` or `DoAction("click")` into real events that the window manager, apps, and the kernel accept.

The first half of this series follows how each question is answered "the hard way" (pixels + synthetic input) and "the smart way" (accessibility trees + native action APIs). The second half follows the machines those agents run on — because a fleet of agents that click for a living needs hundreds of disposable computers, and getting one from *off* to *clicking* in a second is its own engineering problem.

![The control spectrum](diagrams/control-spectrum.svg)

---

## Series map

| # | File | What it covers |
|---|------|----------------|
| 01 | [Foundations: the agent loop](01-foundations-the-agent-loop.md) | The perceive → think → act → verify loop; tool calls; observation and action spaces; harness vs machine; failure taxonomy |
| 02 | [Pixel path I: display, capture, tokens](02-pixel-path-i-display-capture-tokens.md) | Virtual displays (Xvfb); screenshot capture; **how an image becomes a token bill** across model families; the coordinate-scaling minefield |
| 03 | [Pixel path II: grounding and control](03-pixel-path-ii-grounding-control-economics.md) | UI grounding (SeeClick, UGround, UI-TARS, Set-of-Mark); zoom/retry strategies; XTEST, SendInput, CGEvent; the executor; caching and pruning; OSWorld numbers |
| 04 | [Structure path I: AT-SPI on Linux](04-structure-path-i-at-spi-linux.md) | The AT-SPI architecture over D-Bus, grounded in the at-spi2-core source: bus, registryd, interfaces, roles/states/relations, `DoAction`, caching, staleness |
| 05 | [Structure path II: macOS, Windows, browsers](05-structure-path-ii-macos-windows-browsers.md) | `AXUIElement`/`AXPress`; UI Automation control patterns; the CDP/Playwright fast lane; cross-platform comparison |
| 06 | [Structure path III: practice and safety](06-structure-path-iii-practice-and-safety.md) | Snapshot→ref→act loops; stale refs; background operation; small grounding models; UFO2; OSWorld as harness; OS-Harm safety results |
| 07 | [Cloud path I: why agents get computers](07-cloud-path-i-why-agents-get-computers.md) | Isolation, parallelism, persistence; containers vs VMs; Firecracker microVMs; the anatomy and network of one agent computer |
| 08 | [Cloud path II: snapshots, clones, disks](08-cloud-path-ii-machines-snapshots-forks.md) | qcow2 copy-on-write chains; direct kernel boot; VM snapshots; userfaultfd restore; the VMGenID clone trap; stop/start/restart/clone semantics; templates, hooks, secrets; cgroup v2 |
| 09 | [Synthesis: strategy + interview Q&A](09-synthesis-strategy-safety-interview-qa.md) | Decision frameworks; hybrid architectures; reliability engineering; safety; 15 interview questions with model answers; cheat sheet of numbers |
| — | [Glossary](GLOSSARY.md) | Every term in the series, one line each, with the file where it's explained |

---

## The one-paragraph mental model

An agent is **a language model wrapped in a loop by a harness**. On each iteration the harness feeds the model **observations** of a real machine (a screenshot, an accessibility-tree dump, or both), the model replies with **one action** (or a short batch) expressed in a tool schema, the harness's **executor** translates that action into synthetic input events or native API calls and delivers them to the machine, and then the loop captures a fresh observation and appends it to the conversation. The conversation — tokens, not the machine — is the agent's memory. Everything else in these notes is detail hanging off that sentence:

- the *pixel path* is one way to produce observations and deliver actions;
- the *structure path* is another;
- the *cloud path* is the machine at the other end of the executor;
- and the *model* — planner, grounder, or end-to-end — is the bias term that decides which of the two paths to lean on.

---

## How to read this series

- **If you have one hour:** file 01, then the diagrams in 04 and 08, then the cheat sheet in file 09.
- **If you are building an agent:** files 02–03 (perception/control), then 06 (practice), then 07–08 (where it will run).
- **If you are preparing for an interview:** file 09's Q&A section, using the other files as the explanation of record.
- **Layout conventions:** `> **Try this**` boxes are runnable or semi-runnable experiments; `> **Under the hood**` boxes go one level deeper than the surrounding text; `> **Gotcha**` boxes mark failure modes that bite real implementations; every file ends with a short "key ideas" recap.
- **No citations:** these notes name tools, projects, and benchmarks in plain text (Firecracker, AT-SPI, OSWorld, …) — those names are the searchable trailheads if you want the primary sources.

Every diagram in this series lives in [`diagrams/`](diagrams/) as an SVG, and several files also carry inline Mermaid diagrams (which render on GitHub and in VS Code with the Mermaid extension). The `diagrams/build_diagrams.py` script regenerates all SVGs from scratch.
