# 07 — Field Reference: The Whole Course, Compressed

> **What this file is.** The field reference for files 01–08: everything you have learned, compressed into lookup tables, checklists, comparison matrices, and one exam-day card. **Nothing here is new.** If a row surprises you, that is a signal — follow its file reference back and re-read that section; the surprise means the instrument has a gap, and this file's density is designed to expose gaps fast. The compression is deliberate: sentences that took pages to earn appear here as table cells, and their *earned* meanings are recalled rather than re-argued.
>
> **The rule of this file:** *read top to bottom for inventory; jump by table for recall; finish at §12's card.* The course's reading order remains **01 → 02 → 03 → 04 → 05 → 08 → 06 → 09** — this file and the index are the only two meant for random access at any time.

---

## 0. How to Use This File — Three Modes and One Glossary

### 0.1 The three reading modes

| Time before the interview | Mode | What to read | Why |
|---|---|---|---|
| **~1 week out** | diagnosis | this file top to bottom, *with a pencil*: every row you cannot expand into a sentence from the source file is a re-read target | the field reference doubles as a gap detector; the surprises are the syllabus |
| **~48 hours out** | consolidation | §3–§10 (the tables), then §11's tiers to sequence final practice; re-read 06 §6–§8 (selection + ledger) once | instruments over details: the room tests judgment, and judgment decays slower than minutiae but needs rehearsal |
| **the morning of** | calibration | §0.2 (skeleton), §12 (the card), §9.4–§9.7 (checklists) — then stop | anchors, not content: the last hour is for the protocol and the refusal sentences, never for new reading |

### 0.2 The interview answer skeleton — the whole course as one procedure

This is the *spoken order* of every design answer, condensed from file 06 §6.3 and rehearsed at length in file 09. Say each step's name out loud as you enter it; the names are half the signal:

| # | Step (say the name) | The work | The artifact you produce aloud |
|---|---|---|---|
| 1 | **Clarify** | 3–5 questions that change the design: scale, consistency, clients, timeline, failure tolerance | a requirements sentence per answer — *written down* |
| 2 | **Nouns & ownership** | the objects, their data, their invariants — "for each fact, who keeps it true?" | the object list with ownership |
| 3 | **Axes of variation** | what is *known* to differ: families, policies, phases, reaction sets, timing, destinations | the axis table (axis + evidence sentence) |
| 4 | **Cross-cutting & contracts** | what wraps everything (auth, retries, audit) and what must hold across *time and failure* (idempotency, ordering, commit) | the wrap list with order; the contract list |
| 5 | **Select — and refuse** | cast §7's matrix at each axis; then the receipt pass: cite, price, refuse, record the trigger | the design + **spoken refusals** with reasons |
| 6 | **Deepen** | walk the interior of one or two components: state, concurrency, storage, the failure window | the follow-up resistance the interview actually grades |

Two permanent laws, from files 05/06: **contracts before patterns** (idempotency, commit-order, terminal verdicts — the L1/L2 work — come before any structural pattern, always), and **representation before behavior** (enums/ids/types first; dispatch second).

### 0.3 The judgment glossary — one line each (file 06 §0.3 expanded in-repo)

| Term | One-line meaning | Source |
|---|---|---|
| **Requirement sentence** | a stated need, quotable, that a structure must cite to justify itself | 06 §0.3 |
| **Receipt** | the citation bundle: change served → bill named → alternative considered → wrong-signal recorded | 06 §8.5 |
| **Refusal** | a pattern considered and declined *with its reason spoken* — scored in interviews | 06 §6.3 |
| **Deferred ledger** | the written list of what you did *not* build + the trigger that will justify building it | 06 §8.3 |
| **Axis of variation** | a dimension the future is known to differ along (family, policy, phase, set, timing) | 06 §6.2 |
| **Wrong-variation-axis** | values treated as structure, or structure treated as values — the most repeated design error | 06 §3.2 |
| **Seam** | an injection boundary chosen where a *cited* change could arrive | 06 §8.4 |
| **Clause** | one of the four cited justifications for a seam (2nd impl, stated swap, testability, multi-consumer) | 06 §4.2 |
| **Bill** | what a pattern adds that every future reader/change/test pays — named before proposing | 06 §8.2 |
| **Shadow (degeneration)** | what a pattern becomes when its justifying force dies | 06 §1.2 |
| **Smell** | a perception that reliably predicts rot — investigation prompt, not verdict | 06 §0.3/§1.1 |
| **Anti-pattern** | a commonly occurring *solution* with decidedly negative consequences — pays early, bills late | 06 §0.3 |
| **Technical debt** | deliberate simplification paying interest until refactored — traced in comments/ledger, not scar tissue | 06 §0.3 |
| **MVD** | least structure for stated requirements + testability + known drivers — *plus the ledger* | 06 §8.3 |
| **Ladder level** | where a decision sits: L0 naive → L1 representation → L2 contracts → L3 seams → L4 patterns → L5 platforms | 06 §8.4 |
| **Second case** | the named second implementation/consumer/change — the evidence that unlocks abstraction | 06 §4.2 |
| **Discriminator** | the question that separates two candidate patterns on the same symptom | 06 §6.4 |
| **Tax** | the cost paid when the discriminator was skipped | 06 §6.4 |

---

## 1. OOP, Compressed (file 01)

### 1.1 The four pillars — and their industrial meanings

| Pillar | Textbook line | The industrial meaning (what interviews test) |
|---|---|---|
| **Encapsulation** | bundling state and behavior | *every invariant has exactly one guardian*; the interface is intentional acts, not field access (no leaked collections — 01 §4, 06 §2.3) |
| **Abstraction** | hiding complexity behind a model | naming things by the *decisions* they serve; an abstraction is a promise to its callers, and the promise's cost is part of it |
| **Inheritance** | deriving types from types | two contracts, keep them straight: **subtyping** (is-a substitutability — LSP's territory) and **reuse** (implementation sharing); conflating them is the classic fresh-graduate error (01 §3) |
| **Polymorphism** | one interface, many forms | dynamic dispatch: the call site names the *contract*, the runtime picks the implementation — the mechanism under Strategy, State, Factory Method, and every ops table in C |

### 1.2 The five OOP decisions (the v1 list, kept — it survives every interview)

| Decision | The question | The one-line test | Deep dive |
|---|---|---|---|
| 1. **Nouns first** | what are the objects, and who owns each fact? | for each fact: *who keeps it true?* — a fact owned twice is a bug with a visa | 01 §2, 06 §6.3 step 1 |
| 2. **Relationship kind** | is-a, has-a, or uses? | "is-a" must survive LSP's contracts; has-a when the thing is *part of* the whole's invariant; uses-a otherwise | 01 §3–§4 |
| 3. **Contract shape** | what exactly does the interface promise? | pre/postconditions + invariants + exceptions named; the contract's *absences* are part of it (04 §4.6) | 01 §5, 02 §3 |
| 4. **Variation placement** | where does change enter — polymorphism or data? | values as data, families as types; if the set varies at runtime, dispatch; if it churns as content, tables | 06 §3.2 |
| 5. **Ownership & lifetime** | who creates, who destroys, who may hold references? | a reference that outlives its owner's promise is a leak or a dangling alias waiting | 01 §6, 05 §7.5 |

### 1.3 The type map — one object, three languages (compressed from file 01)

```text
THE SAME OBJECT IN EACH LANGUAGE — memory-truth, because the interview's "what IS an object?" question

C++ (classes: vtable = per-class, object = vptr + fields)     Java (reference semantics + header)
  Circle object:                                                Circle reference ————► Circle object:
  ┌──────────────────────┐                                     ┌──────────────────────────┐
  │ vptr ──────────────► │   vtable (one per class):            │ object header (class ptr,  │
  ├──────────────────────┤   ┌────────────────────────┐         │   flags/lock/GC bits)      │
  │ radius (int)         │   │ ~Circle                │         ├──────────────────────────┤
  │ x, y (int)           │   │ draw()  → Circle::draw │         │ radius, x, y              │
  └──────────────────────┘   │ area()  → Circle::area │         ├──────────────────────────┤
                             └────────────────────────┘         │ (methods live in the class │
  dispatch: object→vptr→slot→code   (one extra load vs C)        │  metadata, NOT the object) │
                                                                 └──────────────────────────┘
C (no classes: the ops table IS the vtable, hand-built)
  ┌──────────────────────┐   struct shape_ops (per TYPE — one static instance):
  │ ops ───────────────► │   ┌────────────────────────┐
  ├──────────────────────┤   │ .draw = circle_draw    │
  │ radius (int)         │   │ .area = circle_area    │
  │ x, y (int)           │   │ .destroy = circle_free │
  └──────────────────────┘   └────────────────────────┘
  dispatch: s->ops->draw(s)     ← the "this" is EXPLICIT; C's object-orientation in one line
```

The three recall lines: **C++ pays one indirection for dispatch and gets it in the language; Java pays header + reference-dereference and gets GC + reflection; C pays nothing hidden and passes `this` by hand.** Every "how does polymorphism work under the hood?" question is answered by this picture (01 §2.4–§2.6).

### 1.4 Composition vs inheritance — the decision table

| Choose | When | The tell |
|---|---|---|
| **Inheritance** | the subtype *is-a* full, substitutable member of the contract (all LSP clauses hold) | code that never writes `instanceof`/`switch` on subtype; superclass authors *expect* extension (file 05 §6.7's test) |
| **Composition** | behavior/state is *assembled*; the vehicle may change at runtime; the "part" is not substitutable for the whole | you'd otherwise inherit to reuse two methods; the part is chosen per instance (DI) |
| **Delegation** (composition + forwarding) | reusing behavior behind a *narrower* interface than the reused object's | wrapper/wrapper-ish shapes: Adapter, Decorator, Proxy, Strategy (file 04's skills) |

The tie-break sentence, worth having verbatim: ***"Inheritance is a statement about substitutability, never a reuse mechanism; if I'm inheriting for code, I'm composing instead."***

### 1.5 Cohesion, coupling, and the sharing protocol (file 01 §5–§6, one page)

| Concept | The one-liner | The interview use |
|---|---|---|
| **Cohesion** (functional → logical, descending) | one unit, one kind of reason to exist | file 06's class smells are cohesion failures wearing names (God Class = lowest cohesion) |
| **Afferent/efferent coupling** (Ca/Ce, unstable = Ce/(Ca+Ce)) | how many depend on me vs how many I depend on | the *stable dependency* direction argument (DIP, file 02 §6) — instability belongs at the edges |
| **The sharing protocol** | sharing an interface is a promise: your contract's changes are breakages for every implementor/caller | who owns an abstraction decides who may change it — the DIP ownership rule (01 §6, 02 §6.4) |
| **Law of Demeter** | talk to friends, not to friends' friends — `a.b().c().d()` is a coupling chain | not a religion: fluent/builder APIs are *designed* chains; the smell is *reaching through* to mutate another's state (06 §2.2) |

### 1.6 The three questions that convert "SOLID-sounding code" into real design (recall from 01/02)

1. **For each class: list its reasons to change** (the change-driver test — SRP's actual metric, simplest at this size).
2. **For each seam: cite its clause** (second implementation? stated swap? testability? multi-consumer? — 06 §4.2).
3. **For each dependency: who owns the abstraction?** (if the answer is "the implementor," the dependency is upside-down — DIP's real content).

---

## 2. SOLID, Compressed (file 02)

### 2.1 One-line violation tests

| Principle | The one-line violation test | The smell when violated | The fix move |
|---|---|---|---|
| **S** — Single Responsibility | "list the class's reasons to change — more than one *unrelated* driver means violated" | God Class; Divergent Change (06 §2.1, §3.2) | split along the change drivers, not along nouns; one guardian per fact |
| **O** — Open/Closed | "a foreseeable extension required editing existing stable code" | the growing `switch`/`if`-ladder (the second occurrence is the alarm — 06 §3.1) | dispatch seam at the axis: interface + registry; ops tables in C |
| **L** — Liskov Substitution | "can any client break when given the subtype?" (pre/postconditions, invariants, exceptions) | override that throws `NotSupported`; subtype weakening the contract | fix the hierarchy, or drop inheritance (07 §1.4); a subtype that can't substitute is a *has-a* |
| **I** — Interface Segregation | "does every implementor/caller use every method?" | fat interfaces with `throw new UnsupportedOperationException` halves (06 §2.4) | split by *client usage*; compose interfaces rather than merging |
| **D** — Dependency Inversion | "who owns the abstraction — the high-level module or the low-level detail?" | imports of concrete infra inside domain logic; cycles via instantiation (06 §4.1) | abstractions owned by the higher module; injected concretes |

### 2.2 The SOLID triad — violation, smell, pattern help (one lookup row per principle)

| Principle | The smell that reports it | Quantity to watch | Which patterns serve it | Anti-pattern when overcooked |
|---|---|---|---|---|
| SRP | Divergent Change | reasons-to-change count | Facade (separates entry from parts), Command (work as objects), the file 06 split discipline | class-per-method fragmentation; anemic domain |
| OCP | Shotgun Surgery; growing switch | files-touched per feature | Strategy, Factory Method, Template Method, (C: ops tables) | speculative seam for a change that never arrives (06 §4.2) |
| LSP | `instanceof`/`getClass()` in clients; unsupported ops | subtype-check count | composition over inheritance; Adapter for legacy types | hierarchy denial: "prefer composition" used to dodge a real is-a |
| ISP | unused methods; unsupported-operation throws | methods != used methods per client | interfaces per client role; Decorator (layered adds) | interface explosion; one-method interfaces for everything |
| DIP | concrete imports in policy code; untestable units | direct `new`s of infra in domain | Adapter (ours the port), Factory (defer creation), DI at composition roots | abstraction-per-class; the "interface for the mock" ceremony (06 §4.2) |

### 2.3 Beyond SOLID — the other design principles, with their misreadings

| Principle | The one-liner | The common misreading (say the correction) |
|---|---|---|
| **DRY** | every *piece of knowledge* has one authoritative home | "no two lines resembling" — code duplication is cheaper than the wrong abstraction (06 §5.4); knowledge, not text |
| **KISS** | the simplest thing that meets the requirement | read as "no abstractions ever" — it means *proportionality*, which the ladder (06 §8.4) operationalizes |
| **YAGNI** | don't build for named-never changes | read as "never future-proof" — the ledger *records the trigger* (06 §8.3); it defers, it doesn't forget |
| **LoD** | talk to friends, not friends of friends | read as "no chains" — *designed* chains (fluent builders, streams) are contracts; reaching-through to mutate is the smell |
| **Composition over inheritance** | assemble behavior; inherit only for genuine is-a | read as "inheritance is bad" — it is a tool for substitutability, and sometimes that's the design (05 §6) |
| **Fail fast** | broken invariants surface at the earliest representation | read as "crash on anything" — it means *validate at boundaries*, not defensively everywhere (01 §5) |

### 2.4 The economics summary — why SOLID is "change economics" (file 02's thesis in one table)

| Principle recast | The economic statement | The measurable |
|---|---|---|
| SRP | specialize the unit to the *change stream* | files touched per feature (falling) |
| OCP | make the expected change a *new file*, not an edited one | diff size per feature (falling) |
| LSP | keep substitutability, or pay guards forever | `instanceof`/guard count in clients (zero) |
| ISP | size interfaces to their users' needs | unused-method count (zero) |
| DIP | point dependencies at the party who changes less | build/test time for a policy change (minutes) |

> **[INTERVIEW]** The compressed answers when asked "what does SOLID actually buy?" — ***one table cell each:*** SRP buys *localized change*; OCP buys *additive change*; LSP buys *blind substitution*; ISP buys *honest interfaces*; DIP buys *testable policy*. Then the honesty clause that marks a senior: *"and all five are costs too — every split, seam, and inversion is something a reader pays; I invoke them with receipts (file 06) rather than as laws."*

---

## 3. The 23 Patterns — Master Recall Tables

Read each row as four sentences compressed into cells: *what it promises, the words that announce it, how it works in one line, and what it becomes when the justification dies.* If a row cannot be expanded to those four sentences aloud, that row is a re-read target for its source file.

### 3.1 Creational — "how do things get made?" (files 03, 08 §1)

| Pattern (source) | The one-line intent | The signal, as spoken | The mechanic, one line | The shadow (force dies →) |
|---|---|---|---|---|
| **Factory Method** (03 §3) | let subclasses/framework decide which concrete product is created | "the framework must create the right object as events arrive; it's never told which" | a virtual creation hook the framework's fixed flow calls at the moment of creation | a factory whose product set has one member — indirection with no variation |
| **Abstract Factory** (03 §3) | create *families* of related products that must match | "related objects must be chosen together — this family, not that one" | one factory port per family; concrete factories return matched sets | a matrix factory for independent axes — classes multiplying for combinations nobody asked for |
| **Builder** (03 §4) | stage complex construction; deliver immutability | "constructor has 12 parameters, half optional; the result should be frozen" | director sequences builder calls; `build()` validates and freezes | ceremony around `new` — a builder for an object with three plain fields |
| **Prototype** (08 §1) | clone existing objects, polymorphism for copying | "duplicate this — it might be any subtype" | polymorphic `copy()`; per-field shallow/deep decision | unowned copy semantics — silent shared-mutable children |
| **Singleton** (03 §6) | exactly one instance, with a controlled access point | "exactly one X, globally; everyone needs it" | private construction + static/scoped instance (prefer DI container scope) | global state: hidden coupling, tests fighting over the instance |

### 3.2 Structural — "how do things connect to things?" (files 04, 08 §2–§4)

| Pattern (source) | The one-line intent | The signal, as spoken | The mechanic, one line | The shadow (force dies →) |
|---|---|---|---|---|
| **Adapter** (04 §1) | convert an existing interface into the one your code wants | "vendor SDK's interface doesn't match ours; we can't change either side" | a wrapper translating calls (and datum shapes) at the boundary | translating interfaces you own — paying for what a rename would do |
| **Bridge** (08 §2) | two hierarchies that vary independently | "kinds × platforms both keep growing — the class count is exploding" | abstraction holds an implementor; M×N becomes M+N | a "bridge" where one side never varies — a Strategy in costume |
| **Composite** (08 §3) | part-whole trees with uniform treatment | "the document/UI/scene is a tree of containers and leaves" | component + leaf + composite; the recursion is polymorphism | lying leaf methods; missing cycle prevention |
| **Decorator** (04 §2) | layer responsibilities by wrapping the same interface | "features combine per tenant; must stack at runtime" | same-interface wrappers around a core; stack order becomes contract | wrapper-stack mystery — five layers, unpredictable order bugs |
| **Facade** (04 §3) | one calm entry point over a many-part subsystem | "callers drown in eight objects; give them one door" | thin delegation API; power users may still reach in | god facade (every feature adds a method) or leaky facade (callers bypass it) |
| **Flyweight** (08 §4) | share the repeated (intrinsic) parts of many objects | "millions of items, only a dozen distinct kinds — memory is exploding" | pool keyed by intrinsic state; extrinsic state passed per call | dedup machinery where no measurement justified it |
| **Proxy** (04 §4) | same interface, control access (lazy/cache/remote/lock/guard) | "same interface, but access must be lazy/cached/remote/guarded" | a same-interface stand-in that adds access semantics | invisible semantics: the "same" call now caches/queues/guards — contract undeclared |

### 3.3 Behavioral — "how does behavior vary over time and set?" (files 05, 08 §5–§9)

| Pattern (source) | The one-line intent | The signal, as spoken | The mechanic, one line | The shadow (force dies →) |
|---|---|---|---|---|
| **Strategy** (05 §1) | one operation, N interchangeable algorithms chosen at runtime | "same operation, several implementations, chosen at runtime" | behavior interface + injected implementation; selection has one home | strategy costume: selection buried in an `if` inside the consumer |
| **Observer** (05 §2) | one-to-many notification as the set grows and shrinks | "many parties must react when X happens; the set grows" | registration list + fan-out with failure isolation | leaks, invisible control flow, ordering surprises |
| **Command** (05 §3) | requests as objects: queue, retry, audit, undo | "work must outlive the caller — queued/retried/audited/undone" | request object + transport + identity; receivers idempotent | god command; or command-shaped ceremony for a synchronous call whose result is needed now |
| **State** (05 §4) | behavior *and legality* vary with the object's phase | "behavior (and what's legal) changes with the phase" | phase objects/tables + guards; representation-first (enum+guard is the entry tier) | cosplay (stubs) or the invisible machine (strings + scattered `if`s) |
| **Chain of Responsibility** (05 §5) | a request passes ordered stages; any may stop or handle it | "a request passes through stages; each may stop or enrich it" | ordered handlers returning verdicts; terminal is explicit | terminal-less chains — silent drops; the distributed switch |
| **Template Method** (05 §6) | fixed sequence, varying steps, framework owns the flow | "one fixed sequence; steps differ per context; the sequence must hold" | base skeleton + overridable hooks (Hollywood principle) | fragile base class; hook sprawl — the hook set is API forever |
| **Iterator** (08 §5) | sequential access without exposing representation | "walk the collection without exposing its internals" | cursor (`hasNext`/`next`) or internal pipeline; invalidation semantics as contract | ceremony iterators over built-ins; unstated invalidation heisenbugs |
| **Mediator** (08 §6) | centralize many-to-many interaction into a protocol owner | "N modules all need to talk to each other; the graph is a web" | a hub that holds the interaction state and rules; participants know only the hub | the god mediator — the protocol ate the class |
| **Memento** (08 §7) | capture/restore state without breaking encapsulation | "undo/checkpoint — restore to an earlier state" | opaque token: originator makes and restores; caretaker stores without reading | snapshotting state that isn't self-contained (connections, handles) |
| **Visitor** (08 §8) | new operations over a stable structure, without editing it | "add an operation to every node — without touching the node classes" | double dispatch: `accept(visitor)` → `visitor.visitX(this)` | the compile-alarm becomes a tax — types still arriving while ops grow |
| **Interpreter** (08 §9) | a small closed language: grammar as classes, sentences evaluated | "users want to write their own conditions/rules" | grammar → tree → evaluate (compile-once for hot paths); sandboxed | the accidental language; the grammar that keeps growing unbounded |

---

## 4. Family Decision Grids — Choosing Within, and Refusing

### 4.1 Creational — the construction decision in four questions

| Question | If yes → | If no ↓ |
|---|---|---|
| Is the *product choice* made by a framework's flow (not by the caller)? | **Factory Method** (the hook the flow calls) | next |
| Must several products be chosen *as a matched set*? | **Abstract Factory** | next |
| Is construction itself a *process* — staged, validated, variable, immutable result? | **Builder** | next |
| Is the act "make another one like this" — with the subtype unknown to the caller? | **Prototype** | plain `new`/constructor — then ask the singleton question below |
| Is the constraint a *resource fact* (one hardware, one registry) — not convenience? | **Singleton** (via DI scope, not global access) | plain instance, injected — the default |

### 4.2 Structural — the connection decision in five questions

| Question | If yes → |
|---|---|
| Do *two independent axes* both grow (kinds × platforms)? | **Bridge** (M+N, not M×N) |
| Is the interface being converted one you **don't control**? | **Adapter** (else: rename/redesign your own interface) |
| Is it a part-whole *tree* needing uniform treatment? | **Composite** |
| Do many objects *share repeated state* (measured)? | **Flyweight** |
| Same interface, but *access semantics* change (lazy/cache/remote/lock/guard)? | **Proxy** (contract must be re-declared — 04 §4.6) |
| Layered responsibilities that *stack at runtime*? | **Decorator** (else subclass/flag if static) |
| Callers need *one simplified entry* over many parts? | **Facade** (keep parts reachable) |

Order matters: identity of interface → then *translation / addition / access / simplification* → then trees and sharing. If none fire: **direct references and plain calls** — the most common correct answer.

### 4.3 Behavioral — the variation triage (one pass, in order)

| The variation, as found | The pattern | The discriminator that must also hold |
|---|---|---|
| The *whole operation* differs per case, chosen at runtime | **Strategy** | runtime selection; ≥2 named implementations |
| The *set of reactors* is runtime data | **Observer** | registration/removal genuinely happens |
| The work must survive *time/process* boundaries (queue/retry/audit/undo) | **Command** | the caller doesn't need the result now; receivers can be made idempotent |
| Behavior *and legality* differ by phase | **State** | illegal transitions exist; behavior (not just labels) varies |
| Ordered stages *adjudicate* (stop/handle/enrich) | **Chain of Responsibility** | verdicts; a terminal exists |
| A *fixed sequence* with varying steps, owned by a framework | **Template Method** | the framework relation is real — else a step-list pipeline |
| Walking a structure without exposing it | **Iterator** | multiple structures or consumers—else plain loops |
| Many-to-many coordination with *interaction state* | **Mediator** | the hub holds conversation state — else Observer/bus |
| Restore to earlier state (undo/checkpoint) | **Memento** | state is self-contained — else command inverses |
| Growing operations over a *stable* type set | **Visitor** | types closed, ops growing — the expression problem's chosen side |
| Users write rules/conditions (a small language) | **Interpreter** | grammar closed and sandboxable — else buy an engine |
| Nothing above fires | **None** — direct call, data, enum+guard | the refusal is the most common right answer (file 06) |

---

## 5. The Inverted Lookup — From Words in the Problem to Structures

§3 lists signals per pattern; this section inverts the index: **given a phrase an interviewer (or a requirement doc) actually says, which structures are in play?** Read it as a search index over the problem statement — column two is not always a pattern; sometimes the honest answer is a representation or a contract (file 06's ladder), and the table says so.

| The words in the problem | What they activate | File |
|---|---|---|
| "…can be swapped / multiple providers / we may change X later (stated)" | port + Adapter (boundary you don't control) or Strategy (behavior swap) — pin down which kind of change | 04 §1–2, 06 §4.2 |
| "…must retry / survive restarts / deliver eventually" | durable queue (transport) + **Command with identity** + **idempotent receiver** — the design half first | 05 §3, 05 §7.5 |
| "…exactly once / no double-charge / double-click" | idempotency keys + dedupe at the receiver + unique constraint — *not a pattern*: a contract (L2) | 05 §3.6, 06 §7.2 |
| "…audit every X / who did what" | Command (record the request) + append-only log; audit as outer wrap | 05 §3, 04 §2 |
| "…undo / redo / revert" | Memento (snapshot) or Command inverses (replay) — choose by state size and determinism | 08 §7.4, 05 §3.6 |
| "…notify / alert / subscribe / webhook" | Observer (+ outbox for commit-order) | 05 §2 |
| "…several stages / pipeline / middleware / filters" | CoR (stages adjudicate) vs pipeline list (we own both sides) vs Decorator (wrappers stack) | 05 §5–6, 04 §2 |
| "…per-tenant / per-region / rules change quarterly" | Strategy + composition; the rules *values* stay data | 05 §1, 06 §3.2 |
| "…phases / states / lifecycle / transitions" | State (enum+guard minimum; full only when behavior+legality vary) | 05 §4 |
| "…types of X are growing" | Factory Method (creation follows types) *or* Visitor (operations follow types) — mind the axis | 03 §3, 08 §8 |
| "…operations over the same nodes are growing" | Visitor (if types stable) — else plain methods | 08 §8 |
| "…families / variants that must match" | Abstract Factory (matched sets) — check they really vary together | 03 §3 |
| "…complex construction / many options / immutable result" | Builder | 03 §4 |
| "…copy / duplicate / template / clone" | Prototype + the shallow/deep per-field decision | 08 §1 |
| "…one instance / registry / shared config" | Singleton via DI scope — or just inject one | 03 §6 |
| "…tree / nested / folders / containers of containers" | Composite (+ Visitor for operations; Iterator for walking) | 08 §3 |
| "…walk / iterate / stream / page through" | Iterator (+ the invalidation and bounded-memory contracts) | 08 §5 |
| "…millions of / memory pressure / mostly identical" | Flyweight — after a measurement; find the intrinsic/extrinsic split | 08 §4 |
| "…lazy / cache / remote / guard access to the same API" | Proxy (re-declare the contract) | 04 §4 |
| "…talk to each other / chat room / coordination" | Mediator (hub with interaction state) vs event bus (stateless) | 08 §6 |
| "…uniform entry / callers drown / simplify a subsystem" | Facade (keep parts reachable) | 04 §3 |
| "…same shape across two platforms / kinds × OSes" | Bridge — verify both axes grow | 08 §2 |
| "…users write conditions / rules / filters" | Interpreter (closed grammar, sandboxed, compile-once) or buy an engine | 08 §9 |
| "…plugin / extension point / hooks" | Strategy/registry for behavior; Visitor for operations; TM hooks only if the framework relation is real | 05 §1/§6, 08 §8 |
| "…fixed sequence, teams implement steps" | Template Method (API-governed hooks) or step-list pipeline | 05 §6 |
| "…first one wins / interceptor / may stop the request" | Chain of Responsibility with typed verdicts + explicit terminal | 05 §5 |
| "…state-dependent tokens / sessions" | State or a scoped registry — representation-first | 05 §4 |
| "…clone a running thing / snapshot a service" | Memento at system scale — mind non-self-contained state (connections) | 08 §7 |
| "…fan-out to devices / presence / receipts" | Observer + per-recipient watermark (monotonic: enum + guard, not State) | 05 §2, 06 §7.3 |
| "…money / payment / inventory — partial failures" | saga coordinator + compensations (Command inverses) + idempotent steps | 06 §7.2, 05 §3.6 |
| "…ensure it happened / unknown outcome (timeout)" | the unknown-outcome contract: idempotency key + status query + reconcile — never blind retry | 05 §7.5 |
| "…after the transaction commits, then notify" | outbox (write intent with the state change; a relay delivers) | 06 §7.2 |
| "…read your own writes / ordered events per entity" | per-entity sequence + version; consumer-side ordering contract | 05 §2/§3 |
| "…search / query over changing data" | indexing consumer (Observer + idempotent upserts) — not a new source of truth | 06 §7 |

The meta-row that governs the table: **reliability adjectives ("retryable," "exactly once," "auditable," "reversible") are not pattern triggers — they are contract triggers** (idempotency, outbox, command identity, compensation). File 06 §6.3 step 4 exists precisely to stop candidates from pattern-shopping these words.

---

## 6. Combinations, and the Never-Confuse Matrix

### 6.1 Combinations that reliably work (the team-up map)

| Pairing | Why they team up | Where you've seen it |
|---|---|---|
| **Factory Method + Strategy** | the framework creates the varying algorithm at its call moment | file 03 §3's game framework; every plugin registry |
| **Abstract Factory + Builder** | matched family chosen by config; staged construction per family | UI theming; test fixture factories |
| **Observer + Command** | events carry work to be done later (or the listener's reaction is itself a command) | notification systems; queues behind listeners |
| **Observer + outbox** | reactions must fire *iff* the transaction committed | checkout → email/analytics (06 §7.2) |
| **Strategy + Decorator** | the algorithm varies; the wrappers (retry, audit, metrics) wrap *whatever* algorithm | §7.1's notification senders |
| **Composite + Visitor** | structure stable, operations growing | compilers, report renderers (08 §3+§8) |
| **Composite + Iterator** | uniform trees + uniform walking (trees need traversal objects) | filesystem walkers; scene graphs |
| **Bridge + Strategy** | the *implementation side* is exactly a strategy injected at construction | UI renderers (08 §2) |
| **Mediator + Observer** | the hub keeps interaction state; participants subscribe to hub-emitted events | chat servers (08 §6) |
| **Mediator + State** | when the interaction protocol is itself phased (turn-taking, negotiation stages) | auctions, workflows (08 §6.3's warning) |
| **Memento + Command** | undo's two halves in one system: snapshots for bulk state, inverse commands for interactions | IDEs, editors (08 §7.4) |
| **Prototype + Flyweight** | deep-copy the structure, share the immutable leaves | form templates (08 §1.3) |
| **Interpreter + Composite + Visitor** | grammar as a composite tree; evaluation as a visitor; or compile-once for hot paths | rule engines (08 §9) |
| **Proxy + Factory** | the factory can hand back proxies (lazy/remote) with the right identity semantics | RPC clients, ORMs |
| **Singleton + registry pattern** | the one thing is a lookup table of named strategies/providers | service locators, plugin systems |

**Combos that fight (say these too):** *Observer + strict step ordering* (fan-out has no place to encode sequence — use CoR/pipeline instead); *TM + Strategy at the same seam* (one inversion owns the flow; pick), *Command + State on the same object* (request objects vs phase objects — often one is bookkeeping, see 06 §7.3's refusals).

### 6.2 The never-confuse matrix — the pairs the interviews actually ask about

| Pair | The one-line discriminator |
|---|---|
| **Strategy vs State** | Strategy: the *client* selects an algorithm; State: the object's *own transitions* select behavior — and State adds legality |
| **Strategy vs Bridge** | Strategy has *one* varying axis (behavior); Bridge has *two* varying hierarchies (what × how) — the class-count arithmetic decides (08 §2.6) |
| **Adapter vs Bridge vs Facade vs Proxy vs Decorator** | **Converts** (existing mismatched interfaces) / **divides** (two axes, designed up front) / **simplifies** (one entry over many) / **guards** (same interface, access semantics) / **layers** (same interface, stacked responsibilities) |
| **Proxy vs Decorator** | Proxy controls *access to one subject* (lazy/remote/lock) and usually creates or knows it; Decorator *composes behaviors* the caller stacks — and Decorator's stack order matters to callers |
| **Command vs Strategy** | Command: *what to do, later* (object + transport + identity); Strategy: *how to do it, now* (algorithm selection) |
| **Command vs Memento (undo)** | Command undoes via *inverse operations* (replay-shaped); Memento restores via *state snapshots* (copy-shaped) — memory vs determinism (08 §7.4) |
| **CoR vs Decorator** | Both chain same-interface objects; CoR *adjudicates* (may stop; verdicts, a terminal), Decorator *augments* (always passes through, in order) |
| **CoR vs Pipeline/TM** | CoR: stages may terminate/replace the request; pipeline: stages transform and pass on; TM: the *skeleton* is the contract (inversion of control) |
| **Observer vs Mediator** | Observer: notification fan-out (one source → many listeners, no protocol); Mediator: coordination with *interaction state* (the hub decides who-may-what-next) |
| **Mediator vs Facade** | Facade simplifies *access* one-way; Mediator owns *interactions* many-way — and holds conversation state |
| **TM vs Strategy** | Inverted control: TM calls your steps (you extend inside a fixed flow); Strategy is a plug the caller selects (you supply the whole behavior) |
| **Factory Method vs Abstract Factory** | One product, chosen by an overriding hook; a *matched set*, chosen as a family |
| **Builder vs Prototype** | Construct from a *specification* (staged, validated); copy from an *exemplar* (cloned, mutated) |
| **Prototype vs Flyweight** | One duplicates; the other deduplicates (08 §1.6) |
| **Visitor vs pattern matching (modern)** | Matching collapses the *dispatch ceremony*; Visitor's *economics* (closed types, pluggable operations) remain the same trade (08 §8.6) |
| **Iterator vs plain for-loop** | Iterator pays an object to decouple traversal from representation — only worth it when structures/algorithms/consumers genuinely vary independently (08 §5.6) |
| **Composite vs Decorator** | Composite: many children, recursion, tree; Decorator: one wrapped subject, stacked layers, order = contract |
| **Singleton vs "just a static"** | Singleton is about *instance* control (lifetime + one-ness); a static utility is about stateless functions — different beasts, different tests |

> **[INTERVIEW]** The matrix above is the *exact* content of the classic mid-interview probe ***"what's the difference between X and Y?"*** — and the graded form is not the definition pair but the **discriminator sentence**, delivered in one breath, then optionally deepened. Practice them as single sentences: *"'State versus Strategy: strategy is an algorithm the client picks; state is behavior the object's own transitions drive — and only state has legality to enforce.'"*

---

## 7. Anti-Patterns — Recognition in Three Seconds (file 06)

### 7.1 The degeneration map — every pattern's shadow, and the renewal question

One question renews or retires any pattern in a review: ***"Does the force that justified this still exist?"*** (06 §1.2). These are the shadows to name when it doesn't:

| Pattern | Its shadow | The signature you'll actually see in code |
|---|---|---|
| Factory Method / Abstract Factory | factory ceremony | factories whose product set has one member; matrix factories for independent axes |
| Builder | `new` in costume | director+builder for three fields; `build()` returning mutable internals |
| Singleton | global state | every call site reaching the accessor; tests resetting the instance between cases |
| Adapter | translation over owned interfaces | an adapter layer translating between two interfaces *both in your repo* |
| Bridge | Strategy in costume | a "bridge" whose implementation side has exactly one member — forever |
| Composite | lying interfaces | leaf methods throwing `UnsupportedOperationException`; missing cycle checks |
| Decorator | wrapper-stack mystery | five anonymous layers; bugs reproduce only at specific stack depths |
| Facade | god facade / leaky facade | every feature adds a method; callers holding raw subsystem refs "just for one case" |
| Flyweight | premature dedup | pools and keys where no measurement showed duplication; identity bugs |
| Proxy | invisible semantics | the "same" call now caches/queues/guards; contracts undeclared (04 §4.6) |
| Strategy | strategy costume | selection buried in an `if` inside the consumer; algorithms with one implementation |
| Observer | leaks and invisible flow | listeners never unregistered; "who reacts to X?" not answerable without grep |
| Command | god command / sync ceremony | one command class growing variants; commands whose executor returns needed results inline |
| State | cosplay / invisible machine | state subclasses with stub methods; or strings + scattered `if`s with no guards |
| CoR | terminal-less chain | handlers that can all pass; requests silently dropped |
| TM | fragile base / hook sprawl | base-class edits breaking subclasses; 20 protected hooks as public API |
| Iterator | ceremony / heisenbugs | hand-built iterators over plain lists; `ConcurrentModificationException` archaeology |
| Mediator | god mediator | the protocol consumed the class; every new feature edits the hub |
| Memento | snapshot of the unsnapshotable | open sockets, hardware handles "restored" by struct copy |
| Visitor | compile-alarm tax | every new node type edits every visitor — while types are still arriving |
| Interpreter | accidental language | the "simple condition field" now has operators, precedence, and no tests (08 §9.2) |

### 7.2 The catalogue — smell → detection → fix, at every scale (compressed from 06 §2–§5)

| Scale | The smell | Detection (fast, evidence-based) | The fix move |
|---|---|---|---|
| Class | **God Class** | 3+ unrelated reasons to change; 8+ dependencies; contention in VCS | split along the *change drivers* (the dependency list is the disassembly diagram); extract guardians per fact |
| Class | **Feature Envy** | method touches 3+ getters of another object, 0–1 of its own | move the method to the data; or ask the object to do it (intentional operation) |
| Class | **Inappropriate Intimacy** | leaked collections (`getItems()` returning the live list); two classes mutating each other's fields | intentional operations + read-only views; co-ownership becomes explicit API |
| Change | **Shotgun Surgery** | one logical change edits N files (read the last quarter's diffs *by feature*) | the mechanism must enforce completeness: interface + registry / ops table; the reviewer-forgets problem doesn't fix by discipline |
| Change | **Divergent Change** | one file edited by unrelated features/teams, month after month | split along the change streams — same instrument as God Class, read from the commit log |
| Change | **Copy-Paste** | identical *logic* (not text) in 3+ places with drift already visible | extract on shared *change driver* — duplication is cheaper than the wrong abstraction (Metz); never extract on similarity alone |
| Dependency | **Circular Dependency** | A→B→A over sensible edges (types, files, modules); construction order puzzles; mock-of-a-mock in tests | fix by edge nature: **interface** (data edges), **event** (reaction edges), **coordinator** (genuine bidirectional orchestration) — 06 §4.1 |
| Dependency | **Premature Abstraction** | single-implementation interfaces with "for the future" comments; config encoding what should be code | the four clauses test (§9.2); extract-*when*-evidence arrives (extraction is cheap; un-invention isn't) |
| System | **Big Ball of Mud** | nobody can explain the whole; features land by legend; tests untrusted | freeze → carve (behind tests) → instrument → delete; the paying-operations-first order; *not a rewrite* (06 §5.1) |
| System | **Lava Flow** | commented-out path guarded by `if (false)`; "someone might need it" | delete; git is the archive; uncertainty gets a *dated ticket* |
| System | **Golden Hammer** | one tool/pattern as every answer; "we do it at <bigco>"; résumé-driven designs | problem-shape first; two candidate solutions; fit receipts; normalize the boring correct answer |
| System | **speculative generality** | hooks/params/config for scenarios that never shipped | deletion + the deferred ledger (write the trigger instead of the code) |

### 7.3 The repair doctrine — the order that makes repairs actually land

| The law | Its content | Why the order is load-bearing |
|---|---|---|
| **Contracts before patterns** | idempotency, commit-order, terminal verdicts, guard coverage (L2) come before any structural extraction | pattern surgery on top of broken contracts moves the bugs into more files |
| **Representation before behavior** | types, enums, ids, value objects (L1) first | illegal states become unrepresentable — and every later extraction gets smaller |
| **Cheap before structural** | splits and renames before seams; seams before patterns | each tier's success shrinks the next tier's blast radius |
| **Evidence before urgency** | instrument the pain (file-touch counts, incident history) before carving | the loudest subsystem is not always the costliest to change |
| **Freeze → carve → instrument → delete** | the BBoM repayment shape: stop the spread (new work on a stable API), extract behind tests, make remaining pain visible, delete the corpse | rewrites discard the encoded business history and rebuild the swamp with newer syntax |

**Refactoring's three clauses** (Fowler, via 06 §0.3): behavior-preserving, incremental, test-anchored — if a proposal fails any clause, it is not refactoring (it may still be right — but it's a different strategy with different risks, and it needs stating as such).

---

## 8. The Language Anchors — Implementation Recall (files 01–05, 08)

The pattern mechanisms you'll be asked *how* about, per language. Each row: the mechanism → how each language says it. This is prose-recall, not code-compilation: if you can't sketch the *shape*, the row is a re-read.

### 8.1 The five C idioms that carry every pattern

| Idiom | The shape | Where it carried the course |
|---|---|---|
| **Ops table (the manual vtable)** | `struct ops { fptr1; fptr2; };` + an instance holding `const struct ops *`; dispatch `x->ops->f(x)` | every C implementation of Strategy/State/Visitor/CoR; §8.4's mechanism map |
| **`container_of` / intrusive lists** | embed a list node in the struct; recover the struct by pointer arithmetic | Iterator (kernel lists), Observer (listener lists) |
| **Opaque struct (forward declaration)** | the implementation file defines the struct; the header promises only `struct t;` + functions | Memento's consent mechanics; encapsulation in C (01 §4) |
| **Callback + `void *ctx`** | `typedef int (*fn)(void *ctx, ...)` — the context is explicit | Observer listeners, CoR handlers, internal iteration (`filldir`) |
| **Macro iterator** | `list_for_each_entry(...)` expands to a typed loop | Iterator without objects; kernel's bread and butter |

### 8.2 The five C++ anchors

| Anchor | The shape | The pattern(s) it typically implements |
|---|---|---|
| **Abstract class + virtual functions (+ virtual destructor)** | pure-virtual methods; dispatch through vptr; *always* `virtual ~Base()` | Strategy, State, Visitor elements, TM, Factory Method |
| **Ownership by type** | `unique_ptr` = exclusive (composition); `shared_ptr` = shared (flyweights, observables); references = non-owning | ownership decisions per pattern; Flyweight pooling |
| **`std::variant` + `std::visit`** | closed type set + exhaustive visitor overloads | Visitor, Interpreter (modern form — 08 §8.4) |
| **Range-for / iterator pairs** | `begin()/end()`; algorithms take iterators | Iterator; the STL's decoupling argument |
| **`friend` / nested types** | the consent instrument for opaque tokens | Memento; Builder's access to the built type |

### 8.3 The five Java anchors

| Anchor | The shape | The pattern(s) it typically implements |
|---|---|---|
| **Interface + constructor injection** | contract type in, implementation chosen at the composition root | Strategy, Observer, Adapter, DIP everywhere |
| **Functional interfaces (lambdas)** | `Function`, `Supplier`, `Predicate`, `Runnable` — behavior as values | Strategy/Command's lightweight form — when one method suffices, *this* is the design |
| **`record` + immutability** | value carriers; mementos; intrinsic state | Memento tokens, Flyweight intrinsic state, events |
| **Sealed types + exhaustive switch** | closed hierarchies with compiler-checked handling | Visitor's modern simplification; State machine dispatch; Interpreter nodes |
| **Streams / `Iterable`** | internal iteration; lazy pipelines | Iterator (internal form); composition over loops |

### 8.4 The mechanism → implementation map (the cross-language recall row set)

| The mechanism a pattern needs | C | C++ | Java |
|---|---|---|---|
| dynamic dispatch | ops table member | virtual function | interface method |
| algorithm swap | ops pointer assignment | injected reference | constructor injection |
| copy polymorphism | clone entry in ops | virtual `clone()` / copy ctor | `copy()` / copy constructor |
| notification registration | linked list of callbacks + ctx | `vector<Observer*>` | `CopyOnWriteArrayList<Listener>` |
| ordered adjudication | array of handlers + status enum | chain of `unique_ptr<Handler>` | `List<Filter>` + loop with verdicts |
| state as types | enum + switch + guards | class per state / variant | sealed interface + switch |
| state as table | arrays of transitions | table arrays | enum + `EnumMap` tables |
| opaque snapshot | opaque struct + memcpy | `friend`-guarded nested class | nested `record` / package-private |
| shared intrinsic state | refcounted pool + intern table | `shared_ptr<const T>` pool | `ConcurrentHashMap` intern |
| visitor dispatch | switch over kind (+ `-Wswitch`) | `std::visit` / accept() | sealed + `accept`/overloads |
| durable request | struct + queue + unique id | `Command` object + serialization | `record` command + idempotency key |

---

## 9. The Checklists — The Reusable Instruments

### 9.1 The five-step selection process (06 §6.3, compressed)

| Step | Ask | Produce | Classic error |
|---|---|---|---|
| 1 | who are the objects; **who owns each fact**? | object + ownership list | verbs before nouns ("a Service does it") |
| 2 | what is *known* to differ — axis by axis, with the requirement sentence? | axis table (axis, evidence, timing) | values-as-structure (wrong-variation-axis) |
| 3 | what wraps everything (auth, retries, audit, metrics)? | wrap list with **order** | weaving concerns into every operation |
| 4 | what must survive time and failure (undo, queue, pipeline)? | time/failure inventory | infrastructure without the design half |
| 5 | which requirement cites each structure — and what is **refused**? | design + spoken refusals + triggers | missing refusals (looks like pattern-shopping) |

### 9.2 The four questions before any abstraction (06 §8.5)

1. **What named change does this serve?** (a requirement sentence — never "flexibility")
2. **What does it cost forever?** (the §3-table bill, said aloud)
3. **What is the simplest sufficient alternative?** (data → enum+guard → direct call → cited seam → pattern)
4. **How will we know it was wrong?** (the signal/trigger, written into the deferred ledger)

No answer to #1 → stop. Can't answer #2 → you don't understand the abstraction yet. No answer to #4 → it will never be revisited, which is how abstraction debt accretes.

### 9.3 MVD and the deferred ledger — the template

**The design decision:** for each axis from step 2, buy the *lowest sufficient tier*: data → enum+guard → direct call → cited seam → pattern → platform (06 §8.4's ladder).
**The ledger entry per deferred item:** *"[item] — serves [change]; bill would be [cost]; delayed because [clause absent]; trigger: [the evidence event that upgrades it]."*
**Examples of finished entries:**
- *"StoragePort — serves the stated GCS migration; bill = one indirection + translation tests; trigger fired already (swap is named)."*
- *"SharePolicy framework — declined: future features unnamed; trigger: two genuinely different policy shapes ship."*
- *"Fraud rule chain — declined at launch; trigger: fraud rules exceed ~3 and start churning; then promote the predicate to an ordered verdict list."*

### 9.4 The design-review checklist (06 §10.5's instrument, itemized)

| # | Check | The question that reveals it |
|---|---|---|
| 1 | **Evidence counts** | what do the diffs/history say is painful — versus what the diagram claims? (file-touch counts, incident list) |
| 2 | **Contracts** (highest yield) | idempotency keys? commit-order/outbox? terminal paths explicit? guards for illegal transitions? |
| 3 | **Proportion** | per structure: requirement cited? bill named? (the one-sentence test) |
| 4 | **Refusals** | were alternatives considered? is anything built for a change that died? |
| 5 | **Failure walks** | walk two failure windows aloud (timeout mid-flow; crash mid-commit) — the design's honesty shows there |
| 6 | **The artifact** | what did this review *change* — one receipt added, one ledger entry written, one deletion proposed? |

### 9.5 The reliability contract checklist (the cross-file spine — threads from 03–06)

| Situation | The contract | The mechanism |
|---|---|---|
| any retried operation | **idempotent receiver** | identity key + dedupe record + "same answer on re-delivery" |
| outcome unknown (timeout after send) | **status query + reconcile** | never blind-retry; check terminality first; then apply the retry policy |
| side effects must follow state | **outbox** | write intent with the transaction; a relay delivers; consumers dedupe |
| at-least-once delivery | consumer-side dedupe + per-entity ordering | sequence numbers per entity; tombstone-aware upserts |
| partial multi-step failure | **compensation, not rollback** | inverse actions (Command inverses); semantic undo; state the limit: compensation ≠ rollback |
| exactly-once *business* effect | at-least-once + idempotence *is* the exactly-once | say it in that order — it's the only honest order |
| read-your-writes | per-session consistency routing or version waits | make the guarantee explicit; don't hope |

### 9.6 The concurrency quick rules (05 §7.5, compressed)

| Rule | Meaning |
|---|---|
| **Immutability first** | the cheapest thread-safety is state nobody can change; share immutable values freely |
| **Confinement** | state used by one thread (or one actor) needs no locks at all |
| **Ownership transfer** | passing mutable state across threads is a *handover*, not a share — say who owns it after |
| **Publish safely** | visibility is a discipline (volatile/atomics/locks or immutable publication); *working on my machine* is not a happens-before |
| **Don't share mutable state across threads** — and when you must: one consistent lock order, no lock held across external calls | deadlock and convoy's two sources |

### 9.7 The interview delivery checklist (the room — expanded in 09)

1. **Clarify** — 3–5 questions that would *change the design*; write the answers as requirement sentences.
2. **Frame** — say the nouns and their ownership; say the axes; *say that you're doing this* ("before structures: what varies, who owns the call, who pays").
3. **Design** — the skeleton first (interfaces + flow), fleshed inward (state, storage, failure).
4. **Contracts** — the reliability checklist (§9.5) applied out loud to the flow.
5. **Deepen** — pick one component and walk it to the code level.
6. **Refuse on purpose** — one considered refusal with its reason and trigger; it demonstrates the process more than any pattern.

---

## 10. Trade-Offs — the Pairs They Ask About, and the Choosing Criteria

### 10.1 The core pairs

| Choice | Choose the left when | Choose the right when |
|---|---|---|
| **Composition / Inheritance** | behavior assembles or changes at runtime; the part isn't substitutable for the whole | genuine is-a; LSP's clauses hold; extension is the point |
| **Interface seam / functional value** (Strategy vs lambdas) | stateful algorithms, multiple methods, identity matters | one method, stateless behavior — a lambda *is* the design (don't wrap it) |
| **Enum + guard / State classes** | legality and representation suffice; few phases | behavior *and* legality vary per phase; transitions are the object's life |
| **Snapshot (Memento) / Replay (Command log)** | state self-contained, copyable; restore must be O(1) | commands small and pure; audit/time-travel wanted; side effects replay-safe |
| **Safety / Transparency** (composite add/remove) | type safety at design time wins | uniform framework-like treatment required; document runtime failure |
| **External / Internal iteration** | caller needs mid-walk control (break, interleave, branch) | pipelines/composition/parallelism; fewer invalidation footguns |
| **Eager / Lazy** | simplicity; bounded data; failure-at-start | unbounded/expensive pipelines; avoid work not needed |
| **Queue (async) / Direct call** | work outlives the caller; retry/audit/backpressure needed | caller needs the result now; failure semantics are simple |
| **Extract abstraction / Tolerate duplication** | shared *change driver* proven; clause cited | similarity without shared change — duplication is cheaper than the wrong abstraction |
| **Validate at boundary / Defensive checks everywhere** | fail fast at ingress with typed errors (the default position) | genuinely untrusted fields crossing multiple layers — still at boundaries, not "everywhere" |
| **Data / Structure** (values vs types) | content that churns (rules, ratios, templates) — rows, not classes | families that dispatch at runtime — types, not strings (06 §3.2) |
| **Global / Injected** (Singleton vs DI scope) | one-ness is a *resource fact* | almost always: inject the instance; the container owns one-ness if needed |

### 10.2 How to state a trade-off aloud (v1's format, kept — it works)

> ***"X buys me [the requirement it serves]; it costs [the bill]; here I'd choose X because [the requirement sentence], and the trigger to revisit is [the evidence event]."***

Because: every clause of the sentence is an artifact from earlier in this file — the requirement, the bill (§3 rows), the trigger (the ledger). Delivered in that shape, a trade-off answer is unfalsifiable by taste and auditable by anyone: it *is* the course's judgment, spoken in one breath.

**The seven durable trade-offs to have at reflex speed** (the ones that appear regardless of problem): composition vs inheritance; interface vs lambda; sync vs async; snapshot vs replay; strategy vs state; extract vs duplicate; data vs structure. If a mock answer doesn't naturally include one of these, the design space probably wasn't explored.

---

## 11. Frequency and Study Sequence — Where Practice Time Pays

### 11.1 The tiers (files 06 §9.2 + 08 §10.2, merged)

| Tier | Patterns | Sightings in real code | Interview airtime | The standard to hold |
|---|---|---|---|---|
| **1 — fluent or bust** | Strategy, Factory Method, Singleton, Observer, Builder, Adapter | constant | majority | *code from memory, refusals, bills, comparisons* — all four, without notes |
| **2 — know cold** | Command, State, Decorator, Template Method, Facade, Chain of Responsibility | frequent | heavy at senior levels | decision-fluent: signal → discriminator → bill → shadow, plus one prong of tri-language |
| **3 — recognition** | Abstract Factory, Proxy, + the nine of file 08 (Prototype, Bridge, Composite, Flyweight, Iterator, Mediator, Memento, Visitor, Interpreter) | often *unnamed* (Iterator/Composite/Visitor everywhere) | follow-up questions, systems loops | recognize on sight; name the force; refuse by default; for Iterator/Composite/Visitor — they're infrastructure, not tier-3 trivia |

### 11.2 The practice ladder — per tier, in order

| Step | The drill | Done when |
|---|---|---|
| 1 | **blank-page recall**: write the intent, signal, mechanic, shadow, and a 10-line implementation (any one language) for each Tier-1 pattern | no notes, under 3 minutes each |
| 2 | **refusal drills**: for five arbitrary requirements sentences, run the §9.1 five steps aloud and produce at least one refusal with a trigger | the refusals are the *first* thing you say, not an afterthought |
| 3 | **comparison drills**: the §6.2 matrix pairs, one sentence each, cold | mid-sentence interruptions don't break the discriminator |
| 4 | **repair drills**: the file 06 §10.1 ReportManager, or your own service's last quarter — findings, order, one deletion | a one-page critique in 20 minutes |
| 5 | **Tier-2 second language**: for each Tier-2 pattern, sketch the C or C++ form once | the ops-table/`std::variant` shapes are reflex, not translation |
| 6 | **Tier-3 recognition sweep**: read the nine's drives once a week; say the follow-up map (08 §0.3) from memory | the *second question* never surprises you |

### 11.3 The calendar

| When | Do |
|---|---|
| **7+ days out** | this file with a pencil (diagnosis mode, §0.1) → fix gaps at the source files → drill ladder steps 1–4 |
| **48 hours out** | §3–§10 tables + 06 §6/§8 re-read + drill steps 2 and 3 |
| **the night before** | §12's card only; sleep is worth more than any table (a real engineering decision) |
| **the morning** | §12 at breakfast; then close the notes — the room rewards the process, and the process is now yours |

---

## 12. The Exam-Day Card

*One screen. If you read nothing else on the day: this.*

**THE SKELETON (say the steps)**
**Clarify** (3–5 design-changing questions → requirement sentences) → **Nouns & ownership** ("who keeps each fact true?") → **Axes** ("what is known to differ — with evidence?") → **Contracts & cross-cuts** (idempotency, commit-order, retries, auth — before any pattern) → **Select + refuse** (cite, price, refuse, trigger) → **Deepen** (walk one component to code).

**THE THREE QUESTIONS (selection)**
1. What varies — and what must stay stable? 2. Who owns the timeline (who calls whom)? 3. Who pays — and is the change **named**?

**THE FOUR QUESTIONS (before any abstraction)**
Named change? → Cost forever? → Simplest sufficient alternative? → How will we know it was wrong (the trigger)?

**THE LADDER** (buy the lowest sufficient tier)
data → enum+guard → direct call → cited seam (2nd impl / stated swap / testability / multi-consumer) → full pattern (bill paid) → platform (API governance).

**REFUSAL TEMPLATES (say them out loud)**
- *"No Strategy here — one implementation, no runtime selection; the trigger would be a second algorithm named."*
- *"No State machine — the phases don't change behavior, only a label; representation: enum + guard, trigger: phase-dependent rules."*
- *"No chain — nothing adjudicates; it's a predicate evaluated once. A chain without verdicts is cosplay."*
- *"No abstract factory — the products vary independently; no matched families to enforce."*
- *"I'd refuse the framework here: one consumer, no second case; boundary + ledger entry, extraction when evidence arrives."*

**TOP SIGNALS → STRUCTURES (one-line recall)**
multi providers/swap (stated) → port+Adapter/Strategy · retry/outlive → Command+idempotent receiver · notify set → Observer+outbox · phases+legality → State · stages may stop → CoR (terminal explicit) · fixed sequence+others' steps → TM/pipeline · families must match → AF · complex staged construction → Builder · duplicate/clone → Prototype · tree uniform ops → Composite (+Visitor for ops) · walk hidden → Iterator (+contracts) · millions sharing state → Flyweight (measured) · many-to-many with interaction state → Mediator · undo → Memento or inverses · rules users write → Interpreter (closed, sandboxed, compile-once) · two axes both grow → Bridge · same interface, access semantics → Proxy · one calm entry → Facade.

**CONTRACTS (say these before infrastructure)**
idempotency keys + dedupe · outbox for post-commit side effects · unknown outcome → status/reconcile, never blind retry · at-least-once + idempotence = the only honest exactly-once · compensation ≠ rollback · per-entity ordering + tombstones.

**TRADE-OFF FORMULA**
*"X buys [requirement]; costs [bill]; I choose X because [sentence]; trigger to revisit: [event]."*

**CLOSING LINES**
- *"That's the skeleton — the parts I'd deepen next are [the two riskiest]: want me to walk one?"*
- *"The design has [N] structures, each citing a requirement, and [M] refusals — here's the ledger of what I deliberately didn't build and what would unlock it."*

---

## 13. Closing — Where This File Sits

This file is the course's **compression layer**: files 01–08 taught the same content at earning-depth (etymology, tri-language, incidents, drills); §1–§10 here re-index it for recall, and §12 exists so that the last morning's load is one screen instead of two thousand lines. Use it accordingly: **as an index (jump to sources when a row surprises you), as an audit (practice until every row expands to a sentence), and as the on-ramp to the final file.**

**File 09 — the room.** Everything here is *instrument*; file 09 is *performance*: the full case studies assembled end-to-end (the marketplace checkout, the chat system, the notification platform — built whole, with the narration), the five-phase interview delivery with timed drills, the follow-up barrages modeled on the shapes in §6.2 and 08 §10.3, and the grading rubric's view of all of it. If this file is what you *know*, 09 is what you *do* with it — and then the course has done its job.

**One last compression of the compression:** principles became patterns; patterns became judgments; judgments became refusals; refusals became ledgers. **The patterns are how the course started; the receipts are what it leaves you.** Carry the receipts.

---











