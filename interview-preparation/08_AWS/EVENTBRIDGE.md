# EventBridge

🔴 Final hop of Hermes's Transactional Outbox chain and used for Step Functions orchestration triggers.

---

## Cluster 1: Event Bus, Rules, Pattern Matching

### What is it?
An event bus receives events (JSON) from producers; **rules** define a pattern to match against incoming events, and route matching events to one or more targets (Lambda, Step Functions, SQS, another event bus, etc.) — a pub/sub model where producers don't know or care who's listening.

### How does it work?
```json
// Event
{ "source": "hermes.events", "detail-type": "TaskCompleted", "detail": { "tenantId": "t1", "taskId": "..." } }

// Rule pattern — matches only TaskCompleted events for a specific detail-type
{ "source": ["hermes.events"], "detail-type": ["TaskCompleted"] }
```

### Interview Questions
- **L1:** "EventBridge is a pub/sub event bus — producers publish events, rules match on event content and route matching events to one or more targets, decoupling producers from consumers entirely."
- **L2:** explain why this decoupling matters architecturally for Hermes: the component that appends an event to the store doesn't need to know which downstream consumers (read-model updaters, notification services, audit logs) exist or care about that specific event — new consumers can be added later by adding a new rule, with zero changes to the producer.
- **L3:** explain the difference between this **choreography**-style coordination (each service reacts independently to events it cares about, no central conductor) and Step Functions' **orchestration** style (a central state machine explicitly directs each step) — and why Hermes plausibly uses *both*: EventBridge for loosely-coupled fan-out (the Outbox's final hop), Step Functions for tightly-sequenced, must-happen-in-this-order multi-step workflows. This choreography-vs-orchestration distinction is a very likely "why did you use both?" interview question.

### Quick Revision
- EventBridge = pub/sub, pattern-matched routing, producer/consumer fully decoupled.
- New consumers = new rules, zero producer changes — this is the direct payoff of decoupling.
- Choreography (EventBridge, reactive) vs. orchestration (Step Functions, directive) are complementary, not competing, patterns.
