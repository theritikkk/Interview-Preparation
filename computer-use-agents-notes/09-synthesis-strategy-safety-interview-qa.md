# 09 — Synthesis: Strategy, Safety, and the Interview Q&A

> **Where this file fits**
> ```
>   01 loop        the mental model
>   02–03 pixels   capture → tokens → grounding → synthetic input
>   04–06 tree     AT-SPI / AX / UIA / DOM → refs → native actions
>   07–08 cloud    microVMs, snapshots, clones — the machines underneath
>   ─────────────────────────────────────────────────────────────
>   09             you are here: how it all fits together,
>                  what breaks at scale, and how to answer for it
> ```
> This file is the series' capstone: the decision framework for choosing channels, the hybrid architecture production agents converge on, reliability and safety engineering, the economics of a task, **15 interview questions with model answers**, and the cheat sheet of every number worth remembering.

---

## 1. The decision framework: which path, and why

The whole series has been building one question: *given a task, which perception channel and which control channel should the agent use?* The answer is never ideology ("pixels only" or "tree only") — it's five axes evaluated in order.

| Axis | Question | What it decides |
|---|---|---|
| **Exposure** | Does the target expose a DOM? A rich a11y tree? Nothing but pixels? | first-choice channel |
| **Precision** | How exact must the action be? | whether fallback metrology (zoom crops, SoM) is needed |
| **Co-existence** | May the agent steal the cursor and focus, or must a human keep working? | native actions vs synthetic input; background operation |
| **Budget** | tokens per observation, VM-seconds per task, latency per step | model tiering, resolution, pruning, machine size |
| **Trust** | is the content on screen trusted, or could it be adversary-controlled? | observation formatting, egress policy, isolation level |

![The control spectrum](diagrams/control-spectrum.svg)

### 1.1 The strategy tree

```mermaid
flowchart TD
    S[What must the agent operate?] --> B{only the web?}
    B -- yes --> C[DOM / CDP path: act by selector or ref]
    B -- no --> D{apps expose rich a11y trees?}
    D -- yes --> E[structure-first hybrid: tree refs, pixel fallback]
    D -- no --> F[pixel path: screenshot + Set-of-Mark + small grounder]
    C --> G{many parallel tasks or untrusted content?}
    E --> G
    F --> G
    G -- yes --> H[cloud microVM fleet: templates, snapshots, clones]
    G -- no --> I[one VM or local machine with a virtual display]
```

The tree is deliberately boring, and deliberately ordered by *cheapness of the channel*: DOM is text-cheap, a tree is text-cheap, pixels are expensive in tokens and fragile in grounding. You fall down the ladder only when the app forces you to.

### 1.2 The channel ladder (first choice → last resort)

| Priority | Channel | Precision | Cost | Blind spots |
|---|---|---|---|---|
| 1 | **DOM / CDP** (browsers) | exact (selector/ref) | ~free (text) | only web content; iframes and canvas holes |
| 2 | **A11y tree refs** (OS apps) | high (role+name+action) | cheap (text) | poorly-labeled / custom-rendered controls; staleness |
| 3 | **Tree + Set-of-Mark** | high (ref chosen visually) | medium (one annotated image) | topmost-window only; draw+re-capture pass |
| 4 | **Set-of-Mark on pixels** | medium | medium | needs an element detector to draw the boxes |
| 5 | **Pure coordinate grounding** | low-medium | expensive + retries | everything — this is the fallback of the fallback |
| 6 | **Shell / file APIs** | exact | ~free | not a GUI channel at all — but always prefer it for file work |

> **Remember**
> The ladder is about *reliability per token*, not architecture purity. A production agent mixes all six rungs inside one task, and the mixing is a feature: the loop from file 01 doesn't care which channel an observation or action came from, only that observations are fresh and actions are verifiable.

### 1.3 Reading the failure taxonomy forward

File 01 gave the four failure layers; the strategy above is how you *choose* them, and the choice fixes which failures you'll fight:

| Layer | Pixel path's characteristic failure | Structure path's characteristic failure |
|---|---|---|
| Perception | grounding error: wrong element, off-by-scale | stale ref: right element, changed world |
| Control | event lost / wrong window / overlay ate it | action refused: element disabled, no matching pattern |
| Planning | loses constraints in a long task (either path) | same (either path) |
| Environment | animation not settled, slow render | tree flapping during redraws, degenerate nodes |

The practical reading: **pixels fail at the element level; trees fail at the snapshot level.** The mitigations in files 03 and 06 — zoom-and-confirm, generation counters — are each fitted to one of those.

---

## 2. The hybrid reference architecture

Every serious shipping agent has converged on the same skeleton: a **big planner** reasoning over text and refs, a **small grounder** for pixels when structure runs out, an **executor** that owns the machine, and a **verifier** that checks the predicted change actually happened.

```mermaid
flowchart LR
    subgraph OBS[Observation layer]
        R{channel router}
        DOM[DOM / CDP]
        TREE[a11y tree snapshot]
        SHOT[screenshot / SoM]
    end
    subgraph BRAIN[Model layer]
        P[planner: big LLM over text]
        G[grounder: small VLM over pixels]
    end
    subgraph EXEC[Executor]
        VAL[validate + policy]
        MAP[translate to display frame]
        INJ[inject or native action]
    end
    R --> DOM
    R --> TREE
    R --> SHOT
    DOM --> P
    TREE --> P
    P -- "act(ref / selector)" --> VAL
    P -- "target ambiguous" --> G
    SHOT --> G
    G -- point --> MAP
    VAL --> MAP
    MAP --> INJ
    INJ --> M[machine state changes]
    M --> V[verifier: did the predicted change occur?]
    V --> R
```

**Why the split brain works.** The planner's job is reasoning — planning, constraints, memory — and reasoning is what big text models are good at and what pixels only distract. The grounder's job is one narrow conversion — "(description, screenshot) → point" — which a 2B–7B specialist does *better and 50× cheaper* than a frontier model asked to do both at once (files 03 and 06). The split is simultaneously a quality decision and a cost decision, which is why it keeps winning.

### 2.1 Routing table: event → channel → verification

| Situation | Action channel | Verification channel |
|---|---|---|
| Click a link/button in the browser we control | CDP/DOM by selector | URL + DOM state delta |
| Fill a form field in a native app with a tree | set `Value` / `EditableText` | re-read the field's value |
| Press a button with a usable tree node | AT-SPI `DoAction` / UIA Invoke / `AXPress` | tree state delta (e.g. dialog appeared) |
| Press a button that's pixels only | grounded coordinate click via XTEST/SendInput/CGEvent | localized pixel diff around the expected region |
| Read a value displayed on screen | tree text if exposed | screenshot crop only if not |
| Move/rename/convert files | shell / file API | filesystem check |
| Anything irreversible (send, pay, delete) | the above + human confirmation | machine-state check + audit log |

### 2.2 Cost tiers (who gets to see what)

- **Text tier (cheapest):** DOM serialization, tree snapshot with refs, step logs, file listings. Most steps should live here.
- **Annotated-image tier (medium):** a Set-of-Mark screenshot — one image whose tokens buy the model the ability to *point*, not just read.
- **Raw-pixel tier (expensive):** full screenshots for grounding/verification; pruned aggressively in history (file 03 §4).
- **Zoom-crop tier (targeted):** small crops of regions being disambiguated — cheap *per call*, invoked in the observe→propose→zoom→confirm loop.
- **VM tier (per task, not per step):** the microVM itself; its cost is amortized by snapshots and overlays (files 07–08).

> **Interview angle**
> "How do you keep a 250-step agent affordable?" is really "where does each token go?" The answer above — text-first, annotated images second, raw pixels last, all flattened by caching and pruning — is a complete answer for the cost half; the architecture answer (planner/grounder split) covers the rest.

---

## 3. Reliability engineering

### 3.1 The failure taxonomy as a checklist

From file 01, with the fixes this series has assembled:

| Layer | Failure | Fixes |
|---|---|---|
| **Perception** | grounding error | Set-of-Mark, zoom-and-confirm, small grounder, verify-after-act |
| **Control** | event lost/misplaced | executor contract (validate→translate→execute→settle→record), quiescence polls, re-capture even on failure |
| **Planning/memory** | constraint loss, stale state, no verification | anchoring summaries, append-only history, todo lists in context, ask-the-user policy, end-of-task verification pass |
| **Environment** | hostile machine | checkpointing (VM snapshots), fresh machines per task, wait-for-quiescence, detect lockscreens/modals and bail |

### 3.2 The long-horizon four horsemen

OSWorld 2.0's failure analysis (file 06) named the long-horizon killers; each has a standard countermeasure:

| Horseman | What it looks like | Countermeasure |
|---|---|---|
| **Constraints lost** | task said "don't touch the shared drive" at step 1; step 200 ignores it | periodic re-statement of the original constraints in-context; checklists the agent must tick |
| **Late information missed** | a mid-task dialog answers a question the plan assumed | treat every observation as potentially load-bearing; never skip reading dialogs |
| **Guessing instead of asking** | ambiguous name → the agent picks one silently | explicit ask-the-user policy with batched questions; ambiguity detection on proper nouns/amounts |
| **Verification skipped** | the agent declares success without looking | mandatory verification step: cheapest channel that can observe the predicted change |

### 3.3 Loop hygiene checklist

- **Fresh observations:** act only on the latest snapshot; refs carry generation counters; stale ref → refresh-and-retry, never blind retry (file 06 §2).
- **One observation before every branch:** batch only locally-verifiable, low-risk sequences (files 03 §4.3); writes that branch on state get their own step.
- **Action budgets and timeouts:** every task gets a max step count, max wall-clock, and per-action timeout; hitting any ⇒ clean stop with state report, not a spinning loop.
- **Idempotency for retried writes:** a retried "send" is a bug; a retried "set field" is fine. Classify actions once, use everywhere (validate job of the executor).
- **Retry policy:** one reflect-then-retry with a *new* observation; a second failure changes strategy, not stubbornness.
- **Settle discipline:** wait for quiescence (animation stopped, DOM stable, tree stable) before deciding; capture after, always.
- **Heartbeat/stall detection:** no screen change across N steps ⇒ escalate (scroll, switch window, ask, stop).
- **Recovery anchor:** long tasks checkpoint — the cloud path (file 08) makes "snapshot before the risky batch, restore on failure" an operations primitive, i.e. *cheap undo*.

### 3.4 Verification discipline

The single highest-leverage pattern in the whole series:

> **Verify writes with the cheapest channel that can observe the predicted change; trust reads until contradicted.**

- Predict first: "after this click, the dialog should say Success."
- Then look for exactly that, via tree delta → DOM state → localized pixel crop, in that order of cheapness.
- End of task: one full verification pass over the machine state — the same discipline OSWorld's *execution-based scoring* applies to the agent from outside (file 06 §5). If the harness checks machine state, the agent was never allowed to grade its own homework; build the same skepticism into the loop.

> **Gotcha**
> "It probably worked" is the most expensive sentence an agent can think. An unverified write compounds: step 12's silent failure turns step 140's reasoning into fiction, and the agent discovers it only at the end-of-task check it was designed to skip.

---

## 4. Safety engineering

### 4.1 The threat model: two channels, one model

The core safety fact from OS-Harm (file 06 §6): **the agent's context mixes two voices — the user's instructions and everything the world says — with no inherent firewall.** A web page's text, an email body, a document's content: all of it arrives as "observed text" through the same input path as the user's request.

```mermaid
flowchart TD
    U[User channel: the task] --> M[model context]
    W[World channel: screen content, pages, docs, emails] --> M
    M --> A{actions}
    A -- "read (no side effects)" --> OK[proceed freely]
    A -- "write (reversible)" --> P[policy check + maybe confirm]
    A -- "irreversible / public / credential-adjacent" --> C[confirmation + audit]
```

The classic failure this models is the **confused deputy**: the agent has *capabilities* the page doesn't (your files, your accounts, your session) and can be talked into using them by content it merely read. Prompt injection is not exotic — OS-Harm reproduced it with static text on a page, and the mitigation that works is architectural, not model-level.

### 4.2 Defense in depth (each layer catches what the one above misses)

| Layer | Mechanism | Catches |
|---|---|---|
| Model | refusal behavior, safety training | direct misuse (partially — OS-Harm says be honest about the leaks) |
| Prompt architecture | **instruction/data separation**: screen content is DATA; only the original user message instructs; injection denylist patterns | page-injected instructions |
| Harness policy | action classification, confirmations for irreversible/public actions, step budgets | spillover, compounding mistakes |
| Executor | validate job: policy checks before injection; no speculative writes | malformed or out-of-policy actions |
| Infrastructure | network egress policy on the agent computer, scoped accounts, secrets kept out of templates (file 08), per-task isolation | data exfiltration, blast radius |
| Audit | action logs + before/after frames + session recording | detection, forensics, dataset for improvement |

### 4.3 The confirmation ladder

- **Read** (screenshot, tree, open pages): no confirmation. Reads cost nothing externally — except privacy: the agent can read *every* window, so screen scope is a product decision, not a detail.
- **Local, reversible write** (typing in a draft, opening apps, moving a scratch file): no confirmation, but full audit.
- **Persistent write** (editing a real document, settings changes): confirm-or-announce depending on stakes; always reversible-checkpointed when possible.
- **Irreversible / public / credentialed** (send, post, pay, delete, install, credential use): **confirmation required**, and prefer task-scoped credentials that can't be replayed.
- **Anything triggered by content the user didn't write** (a page says "click here to continue"): treat as adversarial until proven otherwise — this is the injection tripwire; require the instruction to trace back to the user channel.

> **Try this**
> Audit your own agent in twenty minutes: log every action with its *trigger* — which message or observation caused it. Any action whose trigger is observed content (not the user's task) and whose class is "irreversible" is a finding. Most real incidents live in exactly that intersection.

---

## 5. The economics of one task

The ledger for a computer-use task, all four lines:

| Line item | Drivers | Levers |
|---|---|---|
| **Model tokens** | observations per step × steps; history growth | caching + pruning (flatten per-step cost), resolution, channel tiering, planner/grounder split |
| **Grounder/vision calls** | disambiguation rate | SoM, zoom crops, a good small grounder |
| **Machine time** | task duration; boot per task | snapshot/restore instead of boot, overlays instead of copies, idle-as-snapshot (files 07–08) |
| **Human attention** | confirmation prompts | batch questions, only gate irreversible; trust levels per user |

The shape that matters: **per-step cost must stay flat while steps grow.** At naive pricing (~1.0M input tokens for a 40-step screenshot loop), long-horizon tasks are unaffordable; with caching (~0.25M equivalent) and pruning (~0.1M equivalent), the same policy runs at roughly constant per-step cost — which is precisely the regime OSWorld 2.0's 250+ step tasks demand (file 03 §4).

On the machine side, the mirror-image fact: compute is the cost when the agent is *working*; storage is the cost when it's *waiting*; the snapshot machinery exists so that "waiting" is nearly free and "working" starts in ~125 ms (file 08 §8).

---

## 6. Fifteen interview questions, with model answers

*Framing: each "model answer" is what a strong candidate says in 60–90 seconds — the core claim, the mechanism, and one number or trade-off. The follow-ups marked in italics are what a probing interviewer actually asks next.*

---

#### Q1. What are the two questions every computer-use agent must answer, and what's the one-paragraph mental model?

**Model answer.** Every CUA answers "what is on the screen right now, and where?" (perception) and "how do I turn that into an action the OS honors?" (control). The structure is: a language model wrapped in a loop by a harness. Each iteration the harness feeds observations of a real machine — screenshot, accessibility-tree dump, or both — the model replies with one action in a tool schema, the harness's executor translates it into synthetic input events or native API calls, and a fresh observation follows. The conversation is the agent's memory; the machine holds none of it.

*Follow-up: "Where does the model end and the system begin?" The model never touches hardware; everything between tokens and the operating system is plumbing owned by the executor — and that plumbing is where most production bugs live.*

---

#### Q2. Walk me through what happens when an agent clicks a button, end to end.

**Model answer.** Six steps. (1) Capture: screenshot of the virtual display or a DOM/tree snapshot. (2) Model: reasoning over the observation produces a tool call — `click(x, y)` or `click(ref=e12)`. (3) Validate: the executor checks the tool exists, coordinates are in bounds, policy allows it. (4) Translate: model-frame coordinates become display-frame — `display = model × (display_size / model_image_size)` — or a ref becomes a rectangle center. (5) Execute: XTEST on Linux, SendInput on Windows (coords normalized to 0–65535), CGEvent on macOS — or a native action like AT-SPI `DoAction`. (6) Settle & capture: wait for quiescence, re-capture *even if the action failed* — the model needs to see reality — and record the call, mapped coordinates, and both frames as the audit trail.

*Follow-up: "What's the classic bug?" Coordinate-space mismatch: up to four spaces exist (display, captured PNG, model's image frame, normalized 0–1000 grounding output); exactly one component owns the mapping, and it's the executor.*

---

#### Q3. How many tokens does a screenshot cost — and why should a systems person care?

**Model answer.** It's vendor-specific geometry. Anthropic-style: long edge capped at 1568 px, then ≈ pixels/750. OpenAI-style: 512-pixel tiles, 85 + 170 per tile. Qwen2-VL lineage: one token per 28×28 patch. The same 1024×768 frame prices at roughly 1,050 / 765 / 1,036 tokens respectively — and the ratios shift at 1080p because tiles quantize (~1,105) while the pixel-linear scheme grows to ~1,845. Why care? Because a screenshot is re-read every step unless you cache and prune: a 40-step naive loop burns ~1M input tokens; pruning old frames to text notes and caching the stable prefix flattens per-step cost — which is the difference between a demo and a long-horizon product.

*Follow-up: "So what resolution do your agents run?" 1024×768×24 on Xvfb is the classic default — good token/legibility trade-off; resolve *regions* via zoom crops rather than raising global resolution.*

---

#### Q4. Compare the pixel path and the structure path. When does each win?

**Model answer.** Pixels are universal and faithful: every app that renders can be operated with no cooperation, which is why screenshot-driven agents work on legacy tools, games, and remote desktops — but they're token-expensive, grounding-fragile, and need the cursor (XTEST/SendInput/CGEvent). Structure — DOM, AT-SPI, UIA, AXUIElement — is text-cheap, precise, verifiable, and can act in the background without stealing focus — but only where someone implemented an accessible tree, and refs go stale as the UI changes. So: structure first where it exists, pixels as fallback, and the interesting engineering is the hybrid — plus the fact that structure still *reads* the screen, which has its own privacy weight.

*Follow-up: "Which fails worse?" Pixels fail at the element level (wrong point); trees fail at the snapshot level (right point, changed world). Different mitigations: zoom-and-confirm vs generation-counted refs.*

---

#### Q5. What is UI grounding, and why do small specialist grounder models exist?

**Model answer.** Grounding is converting "the blue Send button" plus an image into coordinates — "(description, screenshot) → point". A frontier planner is bad-value at this: it's a narrow spatial task, so the SeeClick → UGround → UI-TARS lineage of 2B–7B VLMs does it better and vastly cheaper. The production pattern is split-brain: a big text model plans and reasons over cheap observations, and calls the small grounder only when structure runs out. Benchmarks like ScreenSpot exist to pick grounders; OSWorld's own documentation canonized the split by scoring planner and grounder separately. Set-of-Mark is the clever middle: draw numbered boxes on the screenshot, and pointing becomes classification — the model picks a number instead of estimating a pixel.

*Follow-up: "Where does SoM break?" It only sees the topmost window's rendered pixels; boxes for unlabeled custom canvases still need a detector; and it costs a draw + re-capture pass per observation.*

---

#### Q6. Tree-driven agents: what breaks, and how do you design the loop?

**Model answer.** Refs go stale — the UI changes between snapshot and action, and now `ref=e12` is a different element or gone. The production loop is **snapshot → ref → act with generation counters**: every snapshot increments a generation, refs carry it, and an action on a stale generation triggers refresh-and-retry instead of blind execution. Playwright's refs, OSWorld's tree mode, and serious harnesses converge on this independently. Beyond staleness: semantic healing (if the exact ref is gone, look for a node with the same role+name) and acting only on fresh reads. The trade is real but favorable: text observations at a fraction of screenshot tokens, background operation, and verification via tree deltas instead of pixel diffs.

*Follow-up: "How do you not regress to click-by-name?" Keep refs as the mechanism and semantics as the fallback; name matching is ambiguous when the UI has three "Save" buttons — the tree's position/shape is the disambiguator.*

---

#### Q7. On Linux, how does AT-SPI actually deliver a click to a GTK app?

**Model answer.** AT-SPI is a D-Bus protocol with an architecture: a session bus, a registryd that tracks the application tree, and per-app object trees of Accessible objects. The agent calls into the app's own accessibility interface — for a button, that's the Action interface's `DoAction`, invoked by index, or a Component interface query for geometry, or EditableText/Value for fields. The app then honors it natively — the same path screen readers use — so no cursor moves and it works in the background. Two engineering details that matter: bulk-fetch the tree via the Cache interface (`GetItems`) rather than one D-Bus round-trip per node, and handle staleness — objects emit change signals, but the agent's cached snapshot can still lag, so the generation-counter discipline applies here too.

*Follow-up: "Why not just synthesize events with XTEST?" You can, and it's the universal fallback — but it needs focus and a cursor position, races with the user, and gives the model no semantic names to reason about.*

---

#### Q8. Given a task, how do you choose the channel — and when do you mix?

**Model answer.** Five axes in order: does the target expose structure (DOM > tree > pixels); how precise is the action; may the agent steal cursor/focus; what's the token/latency budget; and how untrusted is the content it will read. The ladder: DOM/CDP → tree refs → tree+SoM → SoM on pixels → pure grounding → shell/file APIs. Mixing inside one task is the norm — e.g., navigate by DOM in the browser, ground-and-click in the native dialog that pops up, verify via tree state. The invariant is constant across channels: observe fresh, act once, verify with the cheapest channel that can observe the predicted change.

*Follow-up: "One task, three channels — what's the risk?" Channel mismatches: a verification channel that observes a different thing than the action channel changed. Keep prediction first, then verify exactly that prediction.*

---

#### Q9. Why do agent fleets run on microVMs instead of containers?

**Model answer.** Isolation and disposability, in that order. Containers share a kernel: a hostile page or a compromised app has a real escape surface, and the agent's job is literally to run untrusted content (web pages, documents). MicroVMs — Firecracker being the reference design — give a hardware-backed boundary via KVM with a minimalist device model, per-thread seccomp filters, a jailer process, and boots in a ~125 ms class at a few MiB overhead, so you get VM isolation at container-ish density. And because they're VMs, snapshots and clones work: templates as shared immutable images, each task a copy-on-write overlay, restore as the undo primitive. Containers with Xvfb are a fine dev environment; production fleets converge on microVMs.

*Follow-up: "Firecracker defaults?" 1 vCPU / 128 MiB when you don't specify; demand-fault paging and CPU oversubscription enabled by default — the density trick.*

---

#### Q10. Explain copy-on-write backing chains — and why they make an agent fleet cheap.

**Model answer.** An image template (say a fully-provisioned Ubuntu with browsers and tools) is a read-only qcow2 backing file. A task machine is a thin overlay whose writes go into a small diff file; reads fall through to the template. Creating a machine is kilobytes regardless of template size, clones share all unchanged bytes, and deleting a machine leaves no residue — just drop the overlay. That's the economic foundation of the fleet: N machines cost one template plus N (mostly small) diffs, and "a fresh computer per task" becomes a default rather than a luxury. Snapshot hygiene is the tax: diffs and overlays leak disk if you never garbage-collect, and snapshots contain RAM — secrets included.

*Follow-up: "Where's the cost trap?" Long-lived diffs: an overlay that keeps growing eventually costs like a disk; snapshot when idle and rebuild machines young.*

---

#### Q11. How can a VM resume from a snapshot in milliseconds — and what breaks on resume?

**Model answer.** Two mechanisms. The checkpoint itself: pause + snapshot RAM, device state, and disk (diff snapshots make repeat checkpoints cheap); free paging resumes with *zero* memory preloaded, and pages fault in on demand via `userfaultfd` as the guest touches them, so you pay for pages actually used — near-instant resume, slightly slow first touches. The breakage: identity. Every restore gets a fresh VMGenID by design, forcing the guest to reseed its RNG and re-identify on the network — because a restored VM is, from the outside, a duplicate machine, and cloned duplicates cause DHCP/MAC collisions, duplicate-outbound-connection bugs, and nonces/RNG reuse if the system doesn't know it was cloned. Sockets and wall-clock also need reconciliation. Restores are then *operations*, not magic: your agent's runtime must tolerate its machine having been teleported.

*Follow-up: "So what's a snapshot, concretely?" RAM + device state + disk, as a point-in-time timeline you can resume — and resume is a *paused-VM* operation, hence the speed.*

---

#### Q12. Stop, start, restart, clone, destroy — why does the distinction matter to an agent engineer?

**Model answer.** Because each maps to a different user-visible promise. Stop = hibernate: the machine's state is preserved, RAM included, and start resumes it — cheap and fast, so it's the default "park it" operation; snapshots make it near-free to wait. Restart = reboot: processes die, state is rebuilt from disk, so it's the recovery operation for a wedged machine but loses all in-RAM context. Clone = timeline duplication: a fork of the machine at this moment, which is how you parallelize "try three strategies" without re-provisioning — and why identity (Q11) is regenerated per clone. Destroy = shred: overlays deleted, only template remains — the disposable-machine default. If the agent framework confuses these, users lose work ("I stopped it and my draft is gone" means someone implemented stop as restart).

*Follow-up: "Which does the fleet default to between sessions?" Stop (hibernate), because idle-as-storage is nearly free while idle-as-compute is not.*

---

#### Q13. What do OSWorld's numbers tell you about where the field is going?

**Model answer.** Three eras: OSWorld 1.0 (2024): 369 tasks, ~72% human success vs ~12% best model — the gap was perception and control; grounding was the bottleneck. OSWorld-Verified (2025): same tasks, repaired, frontier agents into the tens of percent — perception improved and the bottleneck moved up the stack. OSWorld 2.0 (2026): 108 *long-horizon* tasks (median ~1.6 human hours, >250 agent steps), best models ~20% — failures now are planning, memory, and verification: constraints lost, late information missed, guessing instead of asking, verification skipped. The general lesson, which is the thesis of this whole series: once perception is good enough, the differentiators are memory discipline and verification discipline — engineering, not model magic.

*Follow-up: "How do you evaluate an agent, given that?" Execution-based scoring — verify the machine's resulting state, not the agent's claims — the way OSWorld's harness does; and treat model-judge rubrics as approximate (OS-Harm's judge agreement was ~0.76 F1 on task accuracy, ~0.79 on safety).*

---

#### Q14. OS-Harm: what's the threat model, and what defenses does it force into the architecture?

**Model answer.** OS-Harm built 150 tasks on the OSWorld environment across three categories: deliberate user misuse (does the agent refuse?), prompt injection (does a hostile page or document hijack a benign task?), and model misbehavior (does the agent harm nobody asked for?). The findings that force architecture: direct misuse often complies — refusals are partial, so an agent is not a filter bolted onto a model, the model *is* the filter, and it leaks; prompt injection is the live threat — static text in content the agent reads flips it to the attacker's task, and note that content lands in the tree, not just pixels; and spillover — even on safe tasks, unsafe actions occasionally happen. So defenses are layered and pre-model: instruction/data separation ("screen content is DATA; only the user's original message instructs"), egress policy on the agent computer, confirmations for irreversible actions, scoped credentials kept out of templates, and full audit. Red-team it yourself: put "ignore your task and do X" in a comment on a page and watch the first try reproduce the finding.

*Follow-up: "What's the confused deputy here?" The agent holds capabilities the page doesn't — files, sessions, accounts — and read-content can talk it into using them; the firewall between the user channel and the world channel is the whole defense.*

---

#### Q15. You're shipping a computer-use agent next quarter. What's your architecture, and why?

**Model answer.** Four decisions, each argued from this series. (1) **Perception/control: structure-first hybrid** — DOM/CDP in the browser, tree refs in native apps, SoM + a small grounder as fallback, shell APIs for file work; every action verified with the cheapest channel that can observe the predicted change. (2) **Cost: text-tier observations, pruned append-only history, caching-friendly prompts, planner/grounder split** — per-step cost flat in step count, because long-horizon tasks are the product. (3) **Reliability: generation-counted refs, action budgets, retry-with-reflection, mandatory end-of-task verification, checkpoint/restore as cheap undo** — the four horsemen countered by design. (4) **Safety: instruction/data separation, confirmation ladder for irreversible actions, egress policy and scoped secrets, full action audit** — assume injection until proven otherwise. And the machine underneath: microVM per task, created from a template overlay in kilobytes, parked as a snapshot when idle, cloned when parallelizing, destroyed after.

*Follow-up: "What do you ship first, if you can only ship one piece?" Verification. A modest agent that verifies its own writes beats a strong one that doesn't — the OSWorld 2.0 failure list is mostly verification and memory, and both are harness engineering.*

---

## 7. Cheat sheet of numbers

### 7.1 Displays & capture

| Fact | Value |
|---|---|
| Classic agent desktop | 1024×768×24 (Xvfb `:99`) |
| Screenshot PNG on the wire | ~100–400 KB (API re-encodes server-side) |
| Human-viewable agent screen | `x11vnc` + noVNC |
| Coordinate spaces between model answer and cursor | up to 4; mapping: `display = model × (display_size / model_image_size)` |

### 7.2 Image tokens (per single frame)

| Scheme | Rule | 1024×768 | 1920×1080 |
|---|---|---|---|
| Anthropic-style | long edge ≤1568; ≈ pixels/750 | ~1,050 | ~1,845 (after downscale) |
| OpenAI-style | 85 + 170 × 512-tiles | ~765 | ~1,105 |
| Qwen2-VL lineage | 1 token / 28×28 patch | ~1,036 | ~2,690 |

### 7.3 Task-level cost shape (40-step illustration from file 03)

| Regime | ≈ Input tokens (equiv.) |
|---|---|
| Naive full history | ~1.0M |
| + prompt caching | ~0.25M |
| + pruning to text notes | ~0.1M |

### 7.4 Input injection & native actions

| Platform | Mechanism |
|---|---|
| Linux/X11 | XTEST: `XTestFakeMotionEvent` / `FakeButtonEvent` / `FakeKeyEvent` |
| Windows | `SendInput` (absolute coords normalized 0–65535) |
| macOS | CoreGraphics `CGEventPost` |
| Linux a11y | AT-SPI `DoAction` / `Value` / `EditableText` over D-Bus (+ `Cache.GetItems`) |
| Windows a11y | UIA control patterns (Invoke, Value…) |
| macOS a11y | `AXUIElement` / `AXPress` |
| Browsers | CDP / Playwright refs |

### 7.5 The cloud machine

| Fact | Value |
|---|---|
| microVM boot | ~125 ms class |
| Per-VM overhead | ~5 MiB |
| Firecracker API defaults | 1 vCPU / 128 MiB |
| Overlay creation cost | kilobytes |
| Isolation kit | KVM, minimalist device model, per-thread seccomp, jailer |
| Snapshot contents | RAM + device state + disk |
| Resume | zero memory preloaded; `userfaultfd` demand paging |
| Identity per restore | fresh VMGenID ⇒ RNG reseed + network re-identification |
| Lifecycle | stop = hibernate, restart = reboot, clone = timeline fork, destroy = shred |
| Resource caps | cgroup v2: `cpu.max`, `memory.max`, `pids.max`, `io.max` |

### 7.6 Benchmarks

| Benchmark | Shape | Numbers |
|---|---|---|
| OSWorld 1.0 (2024) | 369 tasks (361 excl. 8 network-dependent) | human ~72.36% vs best model ~12.24% |
| OSWorld-Verified (2025) | same tasks, repaired | frontier into the tens of % |
| OSWorld 2.0 (2026) | 108 long-horizon tasks; ~1.6 h median human; >250 steps | best models ~20% |
| OS-Harm | 150 tasks; 3 categories: direct misuse, prompt injection, misbehavior | judge ~0.76 F1 accuracy / ~0.79 safety |

---

## 8. The ten sentences of this series

If everything else fades, keep these — one per layer of the stack:

1. **The loop** (01): a model wrapped in a harness — observations in, one tool call out, the conversation as memory, an executor as hands.
2. **Pixels are universal and expensive** (02): any GUI renders, and every screenshot is a token bill geometry — pixels/750, tiles, or 28×28 patches.
3. **Grounding is its own skill** (03): "(description, screenshot) → point" is best done by small specialists, and the executor owns the coordinate mapping absolutely.
4. **Structure is text-cheap and precise** (04): AT-SPI/UIA/AX expose apps as named objects with native actions — better to ask than to press.
5. **Every platform has the same three layers** (05): tree, actions, events — learn the pattern once, read the platform dock once.
6. **The production loop is snapshot → ref → act** (06): generation counters, semantic healing, verification via the cheapest channel — and OS-Harm says guard the doorway.
7. **Agents get their own computers** (07): isolation, parallelism, persistence — microVMs at container-like density is why fleets exist.
8. **Machines are timelines** (08): templates and overlays make them cheap, snapshots make them resumable, clones fork them, and VMGenID admits a clone happened.
9. **Verification is the product** (09): if you ship one discipline, ship verify-what-you-wrote — the benchmarks' failure lists are mostly this.
10. **Caching + pruning + tiering = flat per-step cost** (09): the economics that make long-horizon tasks affordable; the agent that can afford 250 steps beats the agent that can afford 40.

---

## 9. Key ideas to carry forward

- Choose channels on five axes — exposure, precision, co-existence, budget, trust — and mix them inside one task; the invariant across all channels is fresh observations and verified actions.
- The hybrid reference architecture is stable: big planner over text, small grounder over pixels, one executor, one verifier, routing by the channel ladder.
- Reliability is loop hygiene plus the four-horsemen counters: constraints re-stated, late info read, ambiguity asked about, verification mandatory; checkpoint/restore gives cheap undo.
- Safety is architecture, not vibes: two channels in one context (user vs world), instruction/data separation, a confirmation ladder for irreversible actions, egress policy, scoped secrets, full audit.
- The numbers worth carrying: ~1,050 tokens per 1024×768 screenshot; ~125 ms microVM boots; 12% → tens of % → ~20% on OSWorld's three eras; ~0.76/0.79 judge F1 on OS-Harm — and the direction of the field: perception is table stakes, so **memory and verification are the differentiators**.

---

***End of series.*** Back to the [series map](README.md) — or keep the [Glossary](GLOSSARY.md) open beside you, where every term in these notes gets one line and its home file.
