# CI/CD (GitHub Actions)

⚠️ Skills-list item, not confirmed in any project bullet — prepare conceptually.

---

## Cluster 1: Pipeline Stages & Environments

### What is it?
A CI/CD pipeline typically runs as a sequence of stages — build → test → (optionally) package as an artifact → deploy — triggered automatically on events like a push or pull request. **Environments** (dev/staging/prod) often gate deployment behind different rules (e.g., prod requires manual approval or only deploys from the `main` branch).

### How does it work?
```yaml
on: push
jobs:
  test:
    steps:
      - uses: actions/checkout@v4
      - run: npm ci && npm test
  deploy:
    needs: test                     # only runs if the test job succeeds
    if: github.ref == 'refs/heads/main'
    steps:
      - run: ./deploy.sh
```

### Interview Questions
- **L1:** "A CI/CD pipeline automatically builds, tests, and deploys on events like a push — GitHub Actions defines this as jobs/steps in YAML, with `needs` controlling job ordering/dependencies and conditions gating which branches trigger deployment."
- **L2:** explain why gating the `deploy` job behind the `test` job passing (via `needs`) is the entire point of CI/CD's safety value — deploying only happens if the automated test suite (directly relevant to your own 285 automated Hermes tests, or NetSight's Unity/cppcheck/Valgrind suite) actually passed, catching regressions before they reach production rather than after.

---

## Cluster 2: Secrets & Rollback

### What is it?
Secrets (API keys, credentials) are stored encrypted in the CI platform (GitHub Actions Secrets) and injected as environment variables at runtime, never committed to source. **Rollback** means reverting to a previously-known-good deployed version quickly when a new deployment causes problems — ideally automated or at least a one-command/one-click operation, not a manual redeploy-from-scratch process under pressure.

### Interview Questions
- **L1:** "Secrets are stored encrypted by the CI platform and injected at runtime, never committed to code; rollback should be fast and low-friction, since it's specifically needed during an active incident when speed matters most."
- **L2:** explain a common rollback strategy: keeping the previous deployment artifact/image available and simply re-pointing traffic/deployment to it (fast) versus reverting the code and running the full pipeline again (slow, and itself carries deployment risk during an already-bad situation) — the fast path is generally preferred specifically *because* rollback happens under incident pressure.

### Quick Revision
- `needs` gates deployment behind tests passing — the core safety mechanism of CI/CD.
- Secrets: encrypted at rest in the CI platform, injected at runtime, never in source.
- Rollback should be fast (re-point to a known-good artifact) rather than a full slow redeploy, because it's needed exactly when speed matters most.
