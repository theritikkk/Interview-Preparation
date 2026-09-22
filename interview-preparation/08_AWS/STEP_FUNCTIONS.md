# Step Functions

🔴 Orchestrates Hermes's multi-step pipelines, with X-Ray tracing attached.

---

## Cluster 1: State Machines & Orchestration

### What is it?
A Step Functions state machine explicitly defines a workflow as states (Task, Choice, Parallel, Wait, etc.) and transitions between them, with built-in retry/catch policies per state — the **orchestration** counterpart to EventBridge's choreography (`EVENTBRIDGE.md`).

### How does it work?
```json
{
  "StartAt": "ProcessTask",
  "States": {
    "ProcessTask": {
      "Type": "Task", "Resource": "arn:aws:lambda:...:processTask",
      "Retry": [{ "ErrorEquals": ["States.TaskFailed"], "IntervalSeconds": 2, "MaxAttempts": 3, "BackoffRate": 2.0 }],
      "Catch": [{ "ErrorEquals": ["States.ALL"], "Next": "HandleFailure" }],
      "Next": "NotifySuccess"
    },
    "HandleFailure": { "Type": "Task", "Resource": "...", "End": true },
    "NotifySuccess": { "Type": "Task", "Resource": "...", "End": true }
  }
}
```

### Interview Questions
- **L1:** "Step Functions defines a workflow explicitly as a state machine, with retry/catch policies built into each state — good for multi-step processes where order and failure-handling per step matter."
- **L2:** explain **why Step Functions over hand-rolling orchestration logic inside a single Lambda**: built-in retry/backoff per step, visual execution history for debugging, and — critically — a long-running workflow isn't bound by a single Lambda's maximum execution timeout (15 minutes), since Step Functions can wait indefinitely between steps.
- **L3:** explain why Step Functions was likely chosen over Kafka-style choreography *for this specific piece* of Hermes: a multi-step pipeline where step 3 must only run after step 2 succeeds, with explicit compensating logic on failure, is exactly the "must happen in this order, with explicit error branches" shape that orchestration tools are built for — versus a reactive choreography model where that ordering/compensation logic would have to be reconstructed implicitly across multiple independently-reacting consumers, which is much harder to reason about and debug.

### DO NOT SAY THIS
"Step Functions and EventBridge do the same thing." — They solve genuinely different coordination problems (see `EVENTBRIDGE.md`'s orchestration-vs-choreography discussion) — conflating them is a quick way to signal shallow understanding of why Hermes uses both.

### Quick Revision
- State machine = explicit states + transitions + per-state retry/catch, visually debuggable.
- Not bound by a single Lambda's 15-minute timeout — can span long-running, multi-step workflows.
- Choose orchestration (Step Functions) when steps have strict ordering + explicit failure handling; choreography (EventBridge) when consumers should react independently and be addable without touching the producer.
