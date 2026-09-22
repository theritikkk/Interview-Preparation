# System Design — Interview Framework

How to actually run a 45-minute system-design interview, not just what to know.

---

## Time Allocation (45-Minute Interview)

| Time | Activity |
|---|---|
| 0-5 min | Clarify requirements (functional + non-functional) — ask, don't assume |
| 5-10 min | Scale estimation (back-of-envelope, `FUNDAMENTALS.md`) |
| 10-15 min | API design — define the contract |
| 15-25 min | High-level architecture — draw the boxes and arrows, narrate as you go |
| 25-40 min | Deep dive — the interviewer will usually steer you toward 1-2 components; go genuinely deep here, this is where you're actually evaluated |
| 40-45 min | Bottlenecks, failure modes, wrap-up — proactively name weaknesses before being asked |

### Interview Questions
- **L2:** explain why the deep-dive phase (25-40 min) is where most of your evaluation actually happens — the high-level architecture phase demonstrates breadth, but the deep dive demonstrates whether you can reason rigorously about one thing under scrutiny, which is a much stronger signal; don't rush past the high-level phase only to run out of time before the deep dive.

---

## Common Pitfalls

- **Jumping to a solution before clarifying requirements** — designing a beautifully scalable system for the wrong problem.
- **Silent design** — not narrating your thinking; the interviewer needs to hear your reasoning, not just see a final diagram.
- **Over-engineering the first pass** — starting with a fully sharded, multi-region, cache-everywhere design for a problem that doesn't need it yet; start simple, then scale up explicitly as requirements demand it — this also gives you natural material for the whole interview instead of front-loading everything.
- **Ignoring non-functional requirements** — a design that handles the "happy path" functionally correctly but was never asked to consider latency, consistency, or availability targets is an incomplete answer.
- **Not managing time** — spending 30 minutes on API design and leaving no time for architecture or deep dive.
- **Avoiding tradeoffs** — presenting a design as if it has no weaknesses; every real design has one, and naming it proactively is a strong signal, not an admission of failure.

### Interview Questions
- **L2:** if you're specifically asked to design a job scheduler or an event-driven workflow engine (`COMMON_DESIGNS.md`), the "start simple, then scale up" pitfall-avoidance strategy maps directly onto how you'd *narrate* your actual Task Scheduler or Hermes projects if asked to walk through their evolution — a strong technique either way.

### Quick Revision
- Roughly: 5 min requirements, 5-10 min estimation, 5-10 min API, 10 min architecture, 15 min deep dive, 5 min wrap-up.
- Narrate constantly — silence during a system-design interview is a lost opportunity to demonstrate reasoning.
- Start simple, scale up explicitly as you state new requirements — better use of time than over-engineering the first draft.
- Name your own design's weaknesses before being asked.
