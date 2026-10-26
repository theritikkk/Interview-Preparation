# AWS Incidents (INC-14 – INC-15)

> **Evidence first.** Same method. AWS incidents add a layer: the **control plane configuration** (security groups, IAM, IMDS, instance metadata) is part of the runtime environment. Always check what changed in the account — CloudTrail is the "what changed" witness.

---

## INC-14 — Workloads lost AWS credentials after IMDSv2 hardening

### The page

> **11:30 SEV1** — After a security hardening change enforced IMDSv2 (with a token hop limit of 1) on all instances, every application running **inside Docker containers** on the EC2 hosts began failing S3 calls with `AccessDenied` / `ExpiredToken`. Applications running directly on the host (systemd services) work fine. Nothing in the application code changed.

### The evidence (raw, as collected)

```bash
# On a host, as a systemd service (works):
curl -s -X PUT "http://169.254.169.254/latest/api/token" \
  -H "X-aws-ec2-metadata-token-ttl-seconds: 21600" | head -c 60; echo

# Inside a container on the same host (fails):
docker exec -it app-container bash -c '
  curl -s -m 2 -X PUT "http://169.254.169.254/latest/api/token" \
    -H "X-aws-ec2-metadata-token-ttl-seconds: 21600" -o /dev/null -w "%{http_code}\n"'

# Route view from inside the container:
docker exec -it app-container ip route | head -3
docker exec -it app-container bash -c 'curl -s -m 2 http://169.254.169.254/latest/meta-data/ -w "%{http_code}\n" -o /dev/null'

# The hardening change (from the change ticket / SSM document):
aws ec2 modify-instance-metadata-options \
  --instance-id i-0abc... \
  --http-tokens required \
  --http-put-response-hop-limit 1 \
  --http-endpoint enabled
```

```text
From the host (systemd):          200
From inside the container:        403

$ docker exec ... ip route
default via 172.17.0.1 dev eth0     <-- one extra hop: container -> docker bridge -> host -> IMDS

$ docker exec ... curl http://169.254.169.254/latest/meta-data/ -o /dev/null -w "%{http_code}\n"
403
```

CloudTrail (from the account, filtered for the change window) shows the `ModifyInstanceMetadataOptions` calls; no IAM policy or role changes in the window.

### Your turn (before the line)

- [ ] Timeline.
- [ ] Three hypotheses ranked with evidence.
- [ ] The one discriminating test.
- [ ] First 10 minutes.

---
---

# ANALYSIS — INC-14

### Timeline

The hardening change set `http-put-response-hop-limit 1`, meaning the IMDSv2 token request must originate from **no more than one network hop** away from the instance. A systemd service is directly on the instance (0 hops → 1 hop allowed, OK). A container on the Docker bridge network is one hop from the host (container → bridge → host), so its token PUT crosses **2 hops** from the instance's perspective and is rejected with 403. The IAM role is unchanged; the credential fetch path broke, not the permissions.

### Hypotheses ranked

1. **IMDSv2 hop limit rejects container-originated token requests.** Host curl 200 vs container curl 403 for the *same* endpoint, and `ip route` shows the container is behind the bridge (one extra hop). The change ticket shows `hop-limit 1`. Matches.
2. **IAM role/policy change.** Rejected: CloudTrail shows no IAM changes, and the host's own role calls work.
3. **Docker bridge blocking 169.254.169.254.** Considered: some setups block the link-local range. Rejected as primary because the same container reached IMDS *before* the change (it had credentials), and the 403 (an HTTP response) proves packets are reaching IMDS and being refused, not dropped silently.

### The discriminating test

The same token request from the host vs from the container, plus `ip route` inside the container to count hops. A 403 (HTTP response) vs a timeout distinguishes "IMDS is refusing" from "the network is blocking" — the response code is the discriminator.

### Root cause

`http-put-response-hop-limit 1` + a container network namespace one bridge hop from the instance = the IMDS token request exceeded the hop limit. The hardening change was written for instances running workloads directly, and did not account for containerized workloads that reach IMDS through a bridge.

### Fix (smallest safe first)

1. For hosts that run containers needing IMDS: raise the hop limit to 2 (the documented value for containerized workloads):

```bash
aws ec2 modify-instance-metadata-options \
  --instance-id i-0abc... \
  --http-put-response-hop-limit 2
```

2. Verify inside the container: token PUT returns 200, S3 call succeeds.
3. Roll out to the affected instance group with a canary host first.
4. Longer term, prefer **explicit credentials** for containerized workloads rather than relying on IMDS through a bridge: ECS task roles, EKS IRSA/Workload Identity, or a scoped credentials broker — each gives short-lived, narrowly scoped credentials without the hop-limit coupling.

### Prevention

- IMDS hardening rollouts must include a test from **every workload runtime** on the instance (host, Docker, K8s, ECS/EKS agents).
- An instance-metadata option change is a code change: canary one instance, test the workloads, then roll.
- Prefer workload identity over IMDS-through-a-bridge for containers; document the hop-limit implication in the hardening runbook.
- Alert on credential-fetch failures (`ExpiredToken`/`AccessDenied` on AssumeRole/IMDS paths) as a distinct class from authorization failures.

### Lessons

- **IMDSv2's hop limit is a security control that has runtime consequences.** Hardening that is not tested against every workload runtime is a latent outage.
- **A 403 from IMDS means "refused at IMDS," not "denied by IAM."** The response code distinguishes the layer.
- **Containers change the hop count.** Any link-local service (IMDS, metrics, DNS on some setups) is affected by network-namespace topology.

### 5-minute walkthrough script

> "After IMDSv2 enforcement with a hop limit of 1, containerized workloads on every host started failing S3 calls while systemd services on the same hosts worked. The discriminator was making the same token request from both places: the host got 200, the container got 403 — a response, not a timeout, meaning IMDS was refusing, not blocking. The container sits one bridge hop from the host, so its token PUT crossed two hops and exceeded the limit of 1. CloudTrail confirmed no IAM changes. The fix was raising the hop limit to 2 for container hosts, canary-first; the durable fix is workload identity so containers don't depend on IMDS through a bridge. The lesson: a hardening change is a runtime change — test it from every workload runtime on the instance."

---

## INC-15 — Nightly job slows down every few weeks

### The page

> **Recurring SEV2** — A nightly data-processing job on a `t3.2xlarge` slows to ~4× its normal duration for a few days, then recovers, in a cycle of roughly 2–3 weeks. It has happened five times. Each time, someone "fixed" it by rebooting, which coincided with recovery.

### The evidence (raw, as collected)

```bash
aws cloudwatch get-metric-statistics \
  --namespace AWS/EC2 --metric-name CPUCreditBalance \
  --dimensions Name=InstanceId,Value=i-0nightly \
  --start-time 2026-10-01T00:00:00Z --end-time 2026-10-20T00:00:00Z \
  --period 3600 --statistics Average \
  --query 'sort_by(Datapoints, &Timestamp)[*].[Timestamp,Average]' \
  --output text | tail -15

aws cloudwatch get-metric-statistics \
  --namespace AWS/EC2 --metric-name CPUSurplusCreditBalance \
  ... --query '...'

# On the instance during a slow period:
cat /sys/kernel/debug/... 2>/dev/null || true
top -bn1 | head -5
iostat -x 2 3
aws ec2 describe-instances --instance-ids i-0nightly \
  --query 'Reservations[0].Instances[0].[InstanceType,CpuOptions.CoreCount]'
```

```text
CPUCreditBalance (hourly average, abridged):
Oct 01 00:00  288
Oct 05 00:00  120
Oct 09 00:00  18     <-- slow period starts
Oct 12 00:00  4      <-- deepest slowdown, job takes ~4x
Oct 15 00:00  96
Oct 18 00:00  140

CPUSurplusCreditBalance during the slow period: rises to ~64 (spending surplus, being billed)

$ top -bn1 (during slow period, abridged)
%Cpu(s):  98.7 us   <-- the job IS running; it's just slower per-operation
```

### Your turn (before the line)

- [ ] Timeline.
- [ ] Three hypotheses ranked with evidence.
- [ ] The one discriminating test.
- [ ] First 10 minutes.

---
---

# ANALYSIS — INC-15

### Timeline

`t3.2xlarge` is a **burstable** instance: it has a baseline CPU level and earns credits when idle, spending them when bursting above baseline. The nightly job bursts hard for ~2 hours, draining credits. Idle time between runs earns credits back, but not fast enough to cover the burst. Every ~2–3 weeks, the balance hits zero; from then on, the instance runs at baseline CPU and bills **surplus credits**. The job's per-operation speed drops (fewer effective vCPUs), stretching runtime ~4×, which drains the balance further — a self-reinforcing slow period. A reboot does nothing for credits, but coincidentally the slow periods were ending as someone rebooted; correlation looked like causation.

### Hypotheses ranked

1. **CPU credit exhaustion with surplus billing.** `CPUCreditBalance` falls to ~4 at the exact times the job slows; `CPUSurplusCreditBalance` rises during those windows; CPU `us` is high (the job is running, just slower). Matches.
2. **Disk/IO contention.** Rejected: `iostat` shows low await/utilization; the symptom is CPU throughput, not I/O wait.
3. **A noisy neighbor / steal.** Rejected: `st` time is ~0; and the cycle correlates with the job cadence, not with other workloads.

### The discriminating test

Overlay `CPUCreditBalance` and `CPUSurplusCreditBalance` with job runtime. Credit balance falling below ~10 at the same time runtime inflates is the signature. If runtime inflated while credits were high, this hypothesis would be dead and the next test would be EBS burst balance (`EBSByteBalance%`/`EBSIOBalance%`) or network throughput.

### Root cause

A sustained multi-hour burst on a burstable instance type outpaces credit accrual. The "reboot fix" was a coincidence; the actual recovery was credits re-accruing during the following idle period.

### Fix (smallest safe first)

1. For a job that bursts for hours every night, a burstable instance is the wrong shape. Right-size by measured need: the job needs ~98% of 8 vCPUs for 2 hours — that is a **fixed** load. Move to a non-burstable family (`m5.2xlarge`/`c5.2xlarge` as measured), or an ARM/Graviton equivalent for cost, and keep the burstable instance only for genuinely spiky small workloads.
2. Interim: keep the t3 but schedule nothing else on it and accept surplus-credit billing (measure whether the surplus cost already exceeds the m5 price difference — it usually does after a few slow periods).
3. If the job is parallelizable, shortening its burst by using more, smaller instances (or spot) can change the economics too; measure before changing.

### Prevention

- Alarm on `CPUCreditBalance < 20` for burstable instances that run scheduled work.
- Instance selection rule: **scheduled multi-hour sustained work → fixed performance; spiky interactive work → burstable.** Write it down; every team rediscovers this incident otherwise.
- Review the billing line item for surplus credits monthly; it is the bill-shaped symptom of this incident.
- Capacity reviews per job: runtime × CPU utilization → instance class, not instance class by habit.

### Lessons

- **Burstable instances have a performance budget.** Multi-hour sustained bursts exhaust it, then throttle, then bill surplus.
- **A reboot is not a fix when recovery is credit accrual.** Correlated recovery is why this incident survives five rounds without a root cause.
- **The metric that names the cause exists before the symptom.** `CPUCreditBalance` falling below the job's need predicts the slowdown; alarm on it.

### 5-minute walkthrough script

> "A nightly job on a t3.2xlarge slowed ~4× every two to three weeks, and each time a reboot seemed to fix it. The discriminator was CloudWatch: `CPUCreditBalance` fell to nearly zero exactly when the job slowed, and `CPUSurplusCreditBalance` rose — the instance had exhausted its burst credits and was running at baseline, billing surplus credits. Rebooting doesn't restore credits; the 'fix' was coincidence with credit accrual. The job needs ~98% of 8 vCPUs for two hours every night — that's sustained, fixed load, which is the wrong shape for a burstable instance. We moved it to a fixed-performance instance class and set an alarm on credit balance. The lesson: burstable instances have a performance budget; scheduled sustained work doesn't fit it."
