# Troubleshooting Workbook

> **How to use this workbook:** every incident is written **evidence-first**. You see the page, the logs, and the graphs — but not the cause. Write your timeline, hypotheses, and tests **before** you read the analysis section. The value is in the reasoning you produce, not in reading the answer.
>
> **The method** (same for every incident):
>
> 1. **Stabilize** — is it getting worse? Stop the bleeding if it is cheap and safe.
> 2. **Frame the question** — what changed, who is affected, since when?
> 3. **Collect evidence** — from the bottom of the stack up: hardware → kernel → network → service → application. Save everything.
> 4. **Hypothesize** — two or three causes that fit the evidence, ranked.
5. **Test** — the cheapest test that discriminates between hypotheses.
> 6. **Fix and verify** — smallest safe change; verify the symptom is gone and the cause is gone.
> 7. **Prevent** — the guard, the alert, the runbook, the test that would have caught it.
>
> **The interview connection:** each incident ends with a 5-minute walkthrough script. Practice telling the story aloud: context in one sentence, symptom, evidence, hypothesis, test, root cause, fix, prevention. That story is worth more in an interview than a list of tools.

## The evidence-first contract

For every incident, do **not** read past the marker until you have written:

- [ ] A one-paragraph timeline (what happened, when, who noticed).
- [ ] Three hypotheses, ranked, each with the evidence that supports it.
- [ ] The one test that would discriminate between your top two hypotheses.
- [ ] What you would do in the first 10 minutes if this were production.

If you cannot write those four things, you are not ready to read the analysis — and that is exactly the feeling an interviewer is probing for.

## Incident index

| INC | Domain | Title | One-line lesson | File |
|---|---|---|---|---|
| INC-01 | Linux | Load average 40, CPU mostly idle | High load with low CPU means **waiting**, usually on I/O or D-state tasks | [02-linux-incidents.md](02-linux-incidents.md) |
| INC-02 | Linux | "No space left on device" with 30% free | `df` counts bytes, inodes are a separate budget | [02-linux-incidents.md](02-linux-incidents.md) |
| INC-03 | Linux | Disk at 100% but `du` finds nothing | Deleted-but-open files keep the space allocated | [02-linux-incidents.md](02-linux-incidents.md) |
| INC-04 | Linux | Service fails after every reboot | Boot-order dependencies are explicit in systemd, not implied | [02-linux-incidents.md](02-linux-incidents.md) |
| INC-05 | Network | Reachable from one subnet, not another | No reply is often a **return-path** problem, not an ingress problem | [03-network-incidents.md](03-network-incidents.md) |
| INC-06 | Network | 1% of requests time out, randomly | A PMTUD blackhole produces rare, size-dependent stalls | [03-network-incidents.md](03-network-incidents.md) |
| INC-07 | Network | TLS fails in `curl`, works in browsers | A missing intermediate breaks some clients, not all | [03-network-incidents.md](03-network-incidents.md) |
| INC-08 | Kubernetes | Pods Pending on nodes with free CPU | Scheduling obeys **requests**, not actual usage | [04-kubernetes-incidents.md](04-kubernetes-incidents.md) |
| INC-09 | Kubernetes | CrashLoopBackOff after a routine deploy | Config contracts fail at startup; make them explicit and testable | [04-kubernetes-incidents.md](04-kubernetes-incidents.md) |
| INC-10 | Kubernetes | Service has no endpoints after a deploy | A one-character label difference silently disconnects a Service | [04-kubernetes-incidents.md](04-kubernetes-incidents.md) |
| INC-11 | Kubernetes | Node NotReady, images failing to pull | DiskPressure is a node-level condition with node-level fixes | [04-kubernetes-incidents.md](04-kubernetes-incidents.md) |
| INC-12 | Observability | Prometheus memory climbs until OOM | Cardinality is a budget; one bad label can spend it all | [05-observability-incidents.md](05-observability-incidents.md) |
| INC-13 | Observability | p95 looks fine, users disagree | Quantiles cannot be averaged across instances | [05-observability-incidents.md](05-observability-incidents.md) |
| INC-14 | AWS | Workloads lost credentials after IMDSv2 hardening | Hop limits and metadata reachability are part of the security design | [06-aws-incidents.md](06-aws-incidents.md) |
| INC-15 | AWS | Nightly job slows down every few weeks | Burstable instances spend credits, then throttle | [06-aws-incidents.md](06-aws-incidents.md) |
| INC-16 | OpenStack | Large transfers stall on new instances | MTU mismatches between networks and instances cause silent fragmentation | [07-openstack-incidents.md](07-openstack-incidents.md) |
| INC-17 | OpenStack | Volume attach fails on a healthy-looking cluster | A scheduler only schedules onto **up** services in matching zones | [07-openstack-incidents.md](07-openstack-incidents.md) |
| INC-18 | AI | Distributed training hangs at collective init | Topology is part of the contract; one node missing IB changes the whole job | [08-ai-incidents.md](08-ai-incidents.md) |
| INC-19 | AI | Inference p95 spikes at peak, GPU "100% busy" | Utilization is not throughput; the KV cache and queue decide latency | [08-ai-incidents.md](08-ai-incidents.md) |
| INC-20 | AI | Accuracy drops after a library-only upgrade | The tokenizer and preprocessing are part of the model | [08-ai-incidents.md](08-ai-incidents.md) |

## How to practice

1. **Solo drill (30 min per incident).** Read the page/alert. Write the four artifacts (timeline, hypotheses, discriminating test, first 10 minutes). Then read the analysis and grade yourself honestly against it.
2. **Timed drill (10 min per incident).** Same, but with a timer. If you cannot produce a coherent timeline in 10 minutes, that domain is a weak spot — go back to the phase notes.
3. **Teach-back.** Explain the incident out loud as if the listener knows less than you. If you cannot make it simple, you do not understand it yet.
4. **Variation.** Change one fact (different OS, different cloud, IPv6 instead of IPv4) and re-derive the diagnosis. This is what real interviews do.

## What good looks like

A strong incident answer has these properties, which you can grade yourself on:

- **Timeline before theories.** "What happened, in order" precedes "why."
- **Evidence cited, not asserted.** "vmstat shows 90% wa and iostat shows the disk at 99% utilization" beats "I think it's the disk."
- **Hypotheses that can lose.** Each hypothesis names the observation that would disprove it.
- **Cheapest discriminating test first.** Not "reboot and see."
- **A prevention, not a war story.** The guard, alert, or test that stops recurrence.

## Progress tracker

| INC | First attempt (date/score) | Timed attempt | Teach-back done | Variation done |
|---|---|---|---|---|
| INC-01 | | | | |
| INC-02 | | | | |
| INC-03 | | | | |
| INC-04 | | | | |
| INC-05 | | | | |
| INC-06 | | | | |
| INC-07 | | | | |
| INC-08 | | | | |
| INC-09 | | | | |
| INC-10 | | | | |
| INC-11 | | | | |
| INC-12 | | | | |
| INC-13 | | | | |
| INC-14 | | | | |
| INC-15 | | | | |
| INC-16 | | | | |
| INC-17 | | | | |
| INC-18 | | | | |
| INC-19 | | | | |
| INC-20 | | | | |

Grade each attempt 0–4: **0** no timeline, **1** timeline but no hypotheses, **2** hypotheses but no discriminating test, **3** full method, **4** full method plus a prevention and a clean 5-minute story. Anything below 3 on an incident in your weakest domain means one more pass through that phase's notes before the next mock.
