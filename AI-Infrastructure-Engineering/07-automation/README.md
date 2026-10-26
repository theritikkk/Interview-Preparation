# Phase 7 — Automation

> **Why this phase matters:** an infrastructure engineer is paid to make changes repeatable, reviewable, and reversible. Manual commands are how you learn; automation is how you operate. The goal is not "write more scripts" — it is to replace fragile sequences with idempotent, observable, tested changes that a teammate can run at 03:00 without you.

## Outcomes

By the end of this phase you can:

1. Write a Bash operations script with strict mode, argument parsing, logging, locking, cleanup traps, and a dry-run mode.
2. Test shell scripts with ShellCheck and Bats, and explain what a test does and does not prove.
3. Write a Python infrastructure tool with timeouts, retries, structured logs, typed data, and no secret leakage.
4. Use `subprocess` safely (no `shell=True` with untrusted input) and handle partial failure.
5. Write an Ansible inventory, playbook, role, template, and handler; run `--check --diff`; prove idempotence by running twice.
6. Explain Ansible variable precedence, facts, `become`, `changed_when`, `failed_when`, tags, serial/rolling updates, and Vault.
7. Design a CI/CD pipeline with lint, test, build, environment gates, least-privilege credentials, and rollback.
8. Use OIDC/short-lived credentials instead of static cloud keys in CI.
9. Review automation for blast radius, secret handling, race conditions, and failure recovery.
10. Turn a manual lab procedure from Phases 1–6 into a reusable, documented automation.

## Prerequisites

- Phase 1 Linux: shell, permissions, systemd, logs.
- Phase 2 networking: SSH, DNS, HTTP/TLS.
- Phase 3/4: VMs, containers, Kubernetes basics.
- Phase 5/6: cloud/OpenStack APIs and identity.
- A disposable Linux VM (Ubuntu 22.04/24.04 or RHEL-compatible) for Ansible and Bash labs.
- `git`, `bash`, `shellcheck`, `python3`, `pip`, `ansible-core`, and a GitHub account (or GitLab equivalent) for the CI lab.

## Environment

| Lab | Needs | Cost/safety |
|---|---|---|
| L7.1 Bash health check | Linux VM or container; `shellcheck`, `bats` | Free; read-only checks, no destructive commands |
| L7.2 Python inventory report | Python 3.11+, `pytest`; sample JSON | Free; no cloud calls required |
| L7.3 Ansible hardening | Disposable Ubuntu/RHEL VM, SSH, sudo | Free; snapshot first; do not run against your laptop or production |
| L7.4 CI/CD | Git repo, GitHub Actions (or `act` with Docker), optional staging VM | Free tier; never put prod secrets in a public repo |

> **Do not learn Ansible against a machine you cannot rebuild.** Use a snapshot/VM. A playbook that changes SSH, sudoers, or the firewall can lock you out; keep a console session open and test `--check --diff` first.

## Study order

| Order | Note | Key question |
|---|---|---|
| 1 | [Bash for operations](01-bash-for-operations.md) | Can this script fail safely and tell me why? |
| 2 | [Python for infrastructure](02-python-for-infrastructure.md) | Can this tool handle timeouts, partial failure, and secrets? |
| 3 | [Ansible](03-configuration-management-ansible.md) | Is the change idempotent, reviewable, and convergent? |
| 4 | [CI/CD and GitOps](04-ci-cd-and-gitops.md) | What is tested, what is gated, and how do I roll back? |
| 5 | [Labs part 1](05-labs-part1.md) | Can I build and test a safe Bash/Python tool? |
| 6 | [Labs part 2](06-labs-part2.md) | Can I converge a VM and ship it through a pipeline? |
| 7 | [Checkpoint](07-checkpoint.md) | Can I design automation that survives failure? |

## The automation ladder

```text
Level 0  manual command in a terminal
Level 1  documented runbook (copy/paste, human checks)
Level 2  script with args, errors, logging, dry-run
Level 3  idempotent configuration management (Ansible)
Level 4  CI pipeline: lint → test → build → deploy → verify
Level 5  GitOps/controller reconciles desired state continuously
```

Most teams need Levels 2–4. Level 5 is not automatically better: a controller with a bad manifest can propagate a mistake faster. Match the level to the risk and the team's ability to review/rollback.

## Principles

1. **Idempotence:** running twice should produce the same end state, not duplicate resources or append config twice.
2. **Declarative where possible:** describe the desired state; let the tool compute the diff. Use imperative scripts for orchestration and one-off migrations.
3. **Dry-run first:** `--check --diff`, `--dry-run`, plan output, or a read-only mode. If the tool cannot preview, make the change small.
4. **Least privilege:** the automation identity gets only the permissions it needs, scoped to the environment.
5. **No secrets in code:** environment injection, secret manager, Vault, OIDC. Redact logs.
6. **Small blast radius:** one host/zone at a time, `serial`, canary, health checks, automatic pause on failure.
7. **Observable:** structured logs, exit codes, metrics, timestamps, request IDs, and a clear "what changed" summary.
8. **Tested:** syntax/lint, unit tests for logic, integration test on a disposable target, and a rollback test.
9. **Versioned:** every change is a commit/PR; the pipeline runs the same code a human would.
10. **Reversible:** know the rollback command before you deploy; back up state that cannot be recreated.

## Completion criterion

You can:

- Explain why `set -e` is not enough and how `trap`, `pipefail`, and explicit checks change behavior.
- Write a script that refuses to run twice concurrently, cleans up a temp directory, and supports `--dry-run`.
- Write a Python tool that times out, retries idempotent reads, and never logs a token.
- Run an Ansible playbook twice and show the second run has zero changes.
- Explain why `shell`/`command` modules are a last resort in Ansible.
- Design a pipeline that cannot deploy to prod from a fork or an unapproved branch.
- Explain the difference between push deployment and GitOps reconciliation.
- Roll back a failed deployment and prove the previous version is healthy.

Then take [the checkpoint](07-checkpoint.md).
