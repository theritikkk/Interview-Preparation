# Interviews — Question Bank, Project Deep Dives, and Stories

> **How interviews actually work for infra roles:** most of the signal comes from three things — (1) can you explain a mechanism clearly, (2) can you tell a real debugging story with evidence, and (3) do your projects demonstrate judgment, not just completion. This folder prepares all three. None of it replaces the phase notes; it converts them into interview performance.

## Structure of this folder

| Section | Purpose |
|---|---|
| [The question bank](#the-question-bank) | The questions you will actually be asked, with answer frameworks |
| [Project deep dives](#project-deep-dives) | How to present each project and survive follow-ups |
| [Story bank](#the-story-bank) | STAR-format debugging stories from your labs and incidents |
| [Phone screen playbook](#phone-screen-playbook) | The 30-minute call structure and pacing |
| [System design for juniors](#system-design-for-juniors) | What "design a small system" means at intern level |
| [Questions to ask them](#questions-to-ask-them) | Signals to collect about the team |

---

## The question bank

Organized by what interviewers probe. For each: the question, the answer framework (not a script), and the depth level an intern candidate should reach. Practice **aloud** — reading an answer and saying one are different skills.

### Linux fundamentals (expect 3–5 of these)

**"Walk me through what happens when you type `ls` and press enter."**
Framework: shell parses → fork/exec → the kernel loads the binary → dynamic linking → syscalls (openat on directories, getdents) → libc formats output → write to stdout → exit. Mention `strace` as how you'd prove it.
Depth target: be able to name fork/exec and one syscall per stage without notes.

**"A server is slow. Walk me through your diagnosis."**
Framework: 1) what changed (deploys, config, traffic), 2) which resource (USE method: utilization, saturation, errors for CPU/memory/disk/network), 3) process-level (`top`, `pidstat`, PSI), 4) the app's own metrics, 5) fix and verify. Never start with "reboot."
Depth target: USE method by name; two concrete commands per resource.

**"What's the difference between a process and a thread?"**
Framework: address space vs shared address space; scheduling unit; context-switch cost; why containers complicate the answer (PID namespaces).
Depth target: one sentence each plus one operational implication (e.g., thread leaks show in PID count).

**"Explain cgroups and namespaces — how do containers actually work?"**
Framework: namespaces give the *illusion* of a private system (pid, net, mnt, uts, ipc, user); cgroups give *limits* (cpu, memory, pids, io). A container = namespaces + cgroups + a copy-on-write rootfs (overlayfs).
Depth target: name four namespaces and two cgroup controllers; know what `/proc/self/cgroup` shows.

**"How would you find what's filling a disk?"**
Framework: `df -h` (bytes) → `df -i` (inodes) → `du -xh --max-depth=1` walking down → `lsof +L1` for deleted-but-open → the two-budget lesson. Tell the INC-02/INC-03 story here (see the story bank).

### Networking (expect 2–4)

**"What happens when you type a URL and press enter?"** (the classic)
Framework: DNS resolution (cache, then recursive) → TCP handshake → TLS handshake → HTTP request → server processing → response → rendering (or for infra: LB → backend → response). Know which steps can fail and how each failure looks (timeout vs refused vs TLS error).
Depth target: name the failure mode at each stage (e.g., no DNS = NXDOMAIN; filtered = timeout; no listener = refused).

**"How does TCP ensure reliability?"**
Framework: sequence numbers, ACKs, retransmission (RTO, fast retransmit), checksums, flow control (window), congestion control (slow start/AIMD). Know the difference between flow and congestion control.
Depth target: explain retransmission and windowing in two sentences each.

**"Difference between a switch and a router? Between TCP and UDP?"**
Framework: L2 MAC forwarding within a broadcast domain vs L3 IP routing between networks; TCP = connection, ordered, reliable; UDP = datagram, no ordering/recovery, lower overhead (QUIC/DNS/RTC use cases).
Depth target: one-line each, then one "so what" for each.

**"How would you debug a service that's unreachable?"**
Framework: from the client: DNS → route (`ip route get`) → TCP (`nc`/`curl -v`) → capture (`tcpdump`) at both ends → split the path. The INC-05 story fits perfectly here.

### Kubernetes/containers (expect 2–4)

**"What happens when a pod is scheduled?"**
Framework: API server admits → scheduler filters (resources, taints, affinity, volumes) → scores → binds → kubelet watches, pulls image, creates sandbox (CNI allocates IP, runtime creates namespaces/cgroups), starts containers, runs probes → status update.
Depth target: the five steps in order; know where it fails for each common Pending reason.

**"A pod is CrashLoopBackOff. Walk me through it."**
Framework: `kubectl describe pod` (exit code, last state) → `kubectl logs --previous` (the dying words) → the cause is usually app-level (config, dependency) or resource-level (OOM = 137). The INC-09 story applies.

**"What's the difference between requests and limits, and what happens if you get them wrong?"**
Framework: requests = scheduling guarantees (the scheduler's ledger), limits = runtime ceilings (memory → OOMKill, CPU → throttling). Wrong requests → wasted capacity or unschedulable pods; the INC-08 story applies.

**"How does a Service route to pods?"**
Framework: selector matches pod labels → endpoints populated → kube-proxy programs iptables/IPVS on nodes → traffic DNATs to a pod IP. The INC-10 story (label mismatch → no endpoints) is the perfect follow-up you can volunteer.

**"Explain resource requests vs limits for a GPU."**
Framework: extended resources are integer-only, requests must equal limits, no overcommit; the device plugin advertises and allocates; the scheduler does not see utilization. Reference the kind fake-GPU lab (L9.4) as your hands-on proof.

### Cloud (expect 1–3)

**"Difference between a security group and NACL?"** — stateful/instance vs stateless/subnet; allow-only vs allow+deny.

**"How would you secure credentials for a CI pipeline deploying to AWS?"** — OIDC federation with a trust policy restricted to repo/branch/environment, short-lived STS credentials, no static keys, environment approval gates. Reference the L7.4 lab.

**"Your EC2 instance can't reach S3. Diagnose."** — IMDS/role attached? (`curl http://169.254.169.254/latest/meta-data/iam/`) → vpc endpoint or NAT path → security group egress → bucket policy/region. The INC-14 story (IMDSv2 hop limit) is a strong one here.

### Observability/reliability (expect 1–3)

**"What would you monitor on a new service, and what would you alert on?"**
Framework: the four signals; SLIs per user journey; page on symptoms (latency/errors), ticket on causes (saturation); every alert has a runbook; multiwindow burn for SLOs. Reference L8.2/L8.3.

**"How do you know your monitoring works?"**
Framework: test the notification path end-to-end (synthetic canary alert); assert dashboards have data; drill alerts. "Firing in Prometheus ≠ human notified."

**"What's an error budget and what do you do when it's gone?"**
Framework: `1 − SLO` over the window; policy = freeze non-emergency deploys or renegotiate the SLO explicitly; the budget mediates velocity vs reliability.

### AI infrastructure (expect 0–2, and they are differentiators)

**"How do you schedule GPU workloads?"** — device plugin, extended resources (integer, requests=limits), taints/affinity, MIG vs time-slicing trade-off, gang scheduling for distributed jobs. Reference the L9.4 fake-GPU lab.

**"A training job is slow but GPUs show 100%."** — utilization is not throughput; check tensor activity, memory bandwidth, per-rank step time, collective wait (NCCL), thermal. The INC-18/INC-19 stories apply.

**"What's the cost per token/request, and how would you reduce it?"** — decompose: replicas × price × hours / tokens; utilization is the multiplier; levers in order (consolidation, batching, quantization, purchasing). Reference L9.5's measured CPU vs GPU comparison.

---

## Project deep dives

For every project on your resume, prepare:

1. **The 60-second version** — what it does, what you built, one hard problem solved, one number that proves it works.
2. **The 5-minute version** — architecture, trade-offs you made and rejected, what you'd change.
3. **The follow-up armor** — for each design decision, why that choice over the obvious alternative.

### Presenting OpsForge (the flagship)

See the full spec in [13-flagship-project](../13-flagship-project/README.md). The interview narrative:

- **What:** "A reproducible Linux/Kubernetes platform built with Ansible: one command brings up a VM with Kubernetes (kind), Prometheus/Grafana/Alertmanager, and a demo service; a second command runs failure drills."
- **Why it matters to the role:** it proves you can build reproducible infrastructure and **operate** it — the drills are the differentiator.
- **The number:** "The platform detects a killed service and pages a local webhook in under 2 minutes; the runbook recovers it in 4." Or: "Converging from bare VM to working stack in 12 minutes, idempotent on re-run."
- **The hard problem:** pick one real debugging story from the build (a kind networking issue, an alert that didn't fire, an Ansible idempotence bug) and tell it with the incident method.

### Presenting the AI platform capstone

- **What:** "A two-team GPU platform design on a fake-GPU kind cluster: quotas, priorities, checkpointing, monitoring, and a cost model; failure drills included."
- **The differentiator:** you can talk about GPU scheduling semantics (integer requests, MIG vs time-slicing, gang scheduling) with hands-on evidence — rare at intern level.
- **The number:** "A 4-GPU pod that can't be gang-scheduled wastes 4 GPU-hours while pending; my design eliminates that waste class." 

### The follow-up armor (memorize this table for each project)

| Decision you made | The obvious alternative | Why yours |
|---|---|---|
| kind over minikube for the lab cluster | minikube | kind's multi-node support matches the multi-node lessons; faster to create/destroy |
| Ansible for provisioning over shell scripts | shell scripts | Convergent state, idempotent re-runs, reviewable YAML |
| Prometheus over a SaaS APM | Datadog etc. | Learn the mechanism, not a vendor UI; pull model teaches target health |
| CPU-only labs with GPU theory | waiting for GPU access | The measurement discipline transfers; honesty about what was not run builds trust |
| Failure drills as a first-class deliverable | only building the happy path | Operating is the job; building is the prerequisite |

---

## The story bank

Build 4–6 stories from the workbook incidents. Use this shape (STAR with an evidence spine):

```text
Situation:  one sentence. What system, what was broken, who was affected.
Task:       what you were responsible for finding/fixing.
Action:     3–4 steps, each with a command and what it showed.
Result:     the fix, the verification, and one number.
Learning:   the guard/alert/test that prevents recurrence (this is what separates
            a story from a war story).
```

### Story slots to fill from your own work

| # | Story | Source |
|---|---|---|
| 1 | High load, idle CPU (I/O-bound) | INC-01 / your L1.x lab |
| 2 | Disk full with free bytes (inodes or deleted-open) | INC-02/03 |
| 3 | Unreachable service (return path, DNS, or TLS) | INC-05/06/07 |
| 4 | Kubernetes CrashLoop or no-endpoints | INC-09/10 |
| 5 | An alert that didn't fire / fired wrongly | INC-12/13 or your L8.2 fault |
| 6 | A cost or capacity surprise | INC-15, C4-style, or your L9.5 numbers |

For each, write it out in the template above, then practice aloud to a timer (90 seconds for situation+task, 2 minutes for action+result). Record yourself once; listen for the parts where you ramble — those are the parts you don't understand yet.

### What a good story sounds like (calibration)

**Weak:** "We had an outage once. I checked the logs and fixed the config."

**Strong:** "The service was unreachable from one subnet after a network change. Ping worked, TCP timed out — that mismatch was the first clue. I captured on both ends: nothing arrived at the server, so the block was upstream. The ACL had a rule for the jump-host CIDR but not our app subnet; ICMP was allowed separately, which is why ping passed. Fix was a source-constrained allow rule; verification was curl plus a SYN-ACK in tcpdump. The prevention was a synthetic probe per caller subnet and a rule that 'ping is not a TCP test' in the runbook."

The difference: **evidence at each step, a decision rule, and a prevention.** Practice until every story has all three.

---

## Phone screen playbook

The 30-minute screen usually is: 2 min intro, 20 min technical Q&A, 5 min your questions, 3 min logistics. Plan for it:

1. **Your 90-second intro:** name → what you're studying/building → the two projects (OpsForge + AI platform) → the role you want. Practice until smooth.
2. **Answer framework, not monologue:** 1–2 sentences of mechanism → one concrete example → stop. Let them probe. Volunteers get more follow-ups and they get to choose the ground.
3. **When you don't know:** say so, then reason out loud from adjacent knowledge ("I haven't used that tool, but the mechanism would be..."). Interviewers hire the reasoning, not the recall.
4. **Pace:** if they ask "any experience with X," never a bare "yes." One sentence of what + one of a lesson learned.
5. **Your questions:** see the next section; ask about operations, not perks.

## System design for juniors

Intern-level design prompts are small and concrete: "design a deployment pipeline for a small web service," "design monitoring for a 3-service app," "design a two-team GPU cluster." The expected shape:

```text
1. Restate + constraints:     traffic, data, latency, team size, budget. Ask 2–3 questions.
2. The happy-path diagram:    boxes and arrows you can draw in 60 seconds.
3. The numbers:               capacity arithmetic (Little's law, storage, cost). Show one calculation.
4. The failure modes:         3–5, each with a mitigation. This is where juniors win or lose.
5. The observability:         what you'd monitor and page on.
6. Trade-offs named:          what you'd revisit at 10× scale.
```

Depth target: a clean diagram, one real calculation, and a failure mode you can defend — beats a perfect architecture hand-wave. Practice with the Phase 9 capstone's D1/D2 prompts (70B training cluster, multi-tenant inference platform) and the Phase 8 D1/D2 (checkout observability, multi-tenant security).

---

## Questions to ask them

Ask questions that collect **signals about the work**, not about perks:

1. "What does the on-call look like for an intern here — shadowing, or real rotation?" *(tests whether ops is real)*
2. "What broke in production last quarter, and what changed because of it?" *(tests blameless culture and honesty)*
3. "How do you decide between building and buying for infra tooling?" *(tests engineering judgment)*
4. "What does success look like for this internship at the 90-day mark?" *(tests expectations clarity)*
5. "Who reviews the infrastructure code, and what does review focus on?" *(tests quality culture)*

Avoid asking things their website answers. One thoughtful question about their incident process is worth five about tech stack.

---

## Final preparation checklist (the week before)

- [ ] Six stories written in the template and rehearsed aloud with a timer.
- [ ] OpsForge and the AI capstone: 60-second and 5-minute versions ready; one hard problem per project chosen.
- [ ] The follow-up armor table memorized for each project.
- [ ] Mock 1/2/3 scores logged; weakest domain re-drilled.
- [ ] The 10-question oral exams from Phase 8 and 9 checkpoints answered aloud, pass 7/10.
- [ ] Your GitHub repo has a clean README with the diagram and one "start here" lab.
- [ ] Questions-to-ask list ready; one about their incident culture.
- [ ] Resume bullets rewritten with evidence ("built X; verified Y; detected Z in N minutes").
