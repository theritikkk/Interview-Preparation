# Resources — a small, high-value list

> **Rule:** this list is deliberately short. The repository's notes are the primary material; these are companions for depth or reference. Prefer finishing a phase's labs over collecting resources — collecting is procrastination with better lighting.
>
> **No links by design:** the notes avoid external citations; resources are named so you can find current editions yourself. Documentation changes; the concepts in the notes do not.

## Linux

| Resource | Type | When to use |
|---|---|---|
| *How Linux Works* (Ward) | Book | The mental model of boot, processes, layers — read alongside Phase 1 |
| *The Linux Command Line* (Shotts) | Book, free | The command-line reference that pairs with the Phase 1 field manual |
| `man` pages for `ps`, `ss`, `ip`, `systemctl`, `journald.conf` | Reference | The ground truth; learn to read them — it is the interview skill people skip |
| Brendan Gregg's systems-performance material | Articles/site | USE method, load-average analysis — pairs with INC-01 |

## Networking

| Resource | Type | When to use |
|---|---|---|
| *Computer Networking: A Top-Down Approach* (Kurose & Ross) | Book | First principles for TCP/DNS/HTTP when a phase note's summary is not enough |
| *TCP/IP Illustrated, Vol. 1* (Stevens) | Book | The packet-level depth for handshakes, states, and capture analysis |
| `tcpdump`/`wireshark` official docs | Reference | Capture filters vs display filters — a 10-minute read that pays forever |
| Cloud provider VPC/networking documentation | Reference | When mapping the theory to your cloud's actual model (Phase 5) |

## Containers, Kubernetes, and OpenStack

| Resource | Type | When to use |
|---|---|---|
| Kubernetes official documentation (concepts + tasks) | Reference | The authoritative source for API objects; read the "concepts" pages once |
| *Container Networking: From Bridge to BGP* (various deep-dives) | Articles | When Phase 4's networking section makes you want the CNI details |
| OpenStack documentation (architecture guides) | Reference | The service map in Phase 6 becomes real against these diagrams |
| Ansible official docs (best-practices + handlers + vault) | Reference | Pairs with Phase 7's playbook standards |
| kind documentation | Reference | The cluster-in-a-container model used across the labs |

## Observability and reliability

| Resource | Type | When to use |
|---|---|---|
| *Site Reliability Engineering* (Google SRE book) | Book, free online | Chapters on SLOs, error budgets, and postmortems pair with Phase 8 |
| *The SRE Workbook* (Google) | Book, free online | Implementing SLOs and alerts — the multiwindow burn design |
| Prometheus documentation (data model, functions) | Reference | PromQL semantics; the answer to "why is this query wrong?" |
| Grafana documentation (provisioning) | Reference | Dashboards-as-code, which is how the labs provision them |

## Cloud (AWS)

| Resource | Type | When to use |
|---|---|---|
| AWS documentation: VPC, IAM, EC2, EBS, S3 | Reference | The Phase 5 notes map concepts to these; read what you use |
| AWS Well-Architected Framework (pillars) | Guide | The reliability/cost lenses used in the phase design checklists |
| AWS pricing pages + the pricing calculator | Reference | Cost arithmetic in the labs and the cost dashboard panels |

## AI infrastructure

| Resource | Type | When to use |
|---|---|---|
| NVIDIA CUDA C++ Programming Guide (execution model chapters) | Reference | SMs, warps, memory hierarchy — deeper than Phase 9 note 01 |
| NVIDIA DCGM documentation | Reference | The metric list used in monitoring and the XID catalog |
| NCCL documentation + `nccl-tests` repository | Reference | The environment variables, logs, and the bandwidth tests in note 03 |
| PyTorch distributed documentation | Reference | `torchrun`, DDP, elastic — pairs with lab L9.3 |
| vLLM documentation (serving architecture notes) | Reference | Continuous batching/PagedAttention concepts in note 05 |
| Slurm documentation (quick start, GRES, fair-share, preempt) | Reference | The scheduler concepts in note 04 |
| MLflow documentation (tracking, registry) | Reference | The lifecycle mechanics in note 05 and lab L9.5 |
| *Designing Data-Intensive Applications* (Kleppmann) | Book | The systems thinking behind platform design decisions |

## Interview preparation

| Resource | Type | When to use |
|---|---|---|
| [12-interviews](../12-interviews/README.md) in this repo | Notes | The question bank and story template — your primary prep |
| Your own incident log | Notes | The six stories; nothing beats your own evidence |

## Anti-resources (deliberately excluded)

- **Certification dumps** — they teach recall of exam questions, not mechanisms; interviews will expose the difference.
- **Long video courses** as the primary medium — passive watching does not build retrieval; use labs and the question bank instead.
- **Tool-first tutorials** — "install X and click through it" teaches nothing that survives an interview's second follow-up question.
- **Job-description-driven panic reading** — the JD's skills map to phases; study the phases, not the buzzwords.
