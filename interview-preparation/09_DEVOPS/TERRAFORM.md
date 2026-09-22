# Terraform

🔴 Hermes: "18 Terraform modules."

---

## Cluster 1: State, Plan, Apply

### What is it?
Terraform's **state file** records what infrastructure it believes exists (mapping your declarative config to real resource IDs) — this is how `terraform plan` can compute a diff between desired config and actual reality without re-querying every possible resource from scratch each time. `plan` shows what *would* change without applying it; `apply` actually executes those changes.

### How does it work?
**Drift** occurs when real infrastructure changes outside Terraform (a manual console edit, another tool) — Terraform's state no longer matches reality, and the next `plan` will either try to "correct" the drift back to the declared config (potentially surprising) or, if the state itself wasn't refreshed, simply be wrong about current reality. **Remote state** (state stored in S3, Terraform Cloud, etc., rather than a local file) is essential for team use — it enables locking (preventing two people from `apply`-ing concurrently and corrupting state) and a single shared source of truth.

### Interview Questions
- **L1:** "Terraform's state file tracks what it believes is deployed; `plan` diffs desired config against that state to show pending changes; `apply` executes them. Drift happens when reality and state diverge outside Terraform's control."
- **L2:** explain why remote state with locking matters specifically once more than one person (or one CI pipeline plus a person) might run `apply` — without locking, two concurrent applies can corrupt the state file or apply conflicting changes, a very real operational risk in any team-managed infrastructure.
- **L3 — 🔴 directly relevant to Hermes's 18 modules:** explain why modules matter at that scale specifically: a module packages a reusable, parameterized chunk of infrastructure (e.g., "a DynamoDB table with standard tags/backup config" or "a Lambda + its IAM role + its log group") so that Hermes's likely-repeated patterns (multiple Lambdas each needing similar IAM roles, multiple DynamoDB tables) aren't copy-pasted 18 times with configuration drift between copies — a single module used 18 times (or across 18 distinct pieces of infrastructure) is a maintainability and consistency argument, not just a code-organization one. `NEEDS VERIFICATION`: know how your actual 18 modules are organized (by service? by environment? by resource type?) before describing this structure specifically.

### DO NOT SAY THIS
"Terraform automatically detects and fixes any manual changes made outside it." — It detects drift *the next time you run `plan`/`apply` against refreshed state*, and its response is to plan bringing reality back in line with the declared config — which can mean *reverting* a manual change, not intelligently reconciling it; this can be surprising/destructive if you're not aware of exactly what will be "corrected."

### Quick Revision
- State = Terraform's record of reality; `plan` = diff; `apply` = execute the diff.
- Remote state + locking = required for safe team/multi-runner use, prevents concurrent-apply corruption.
- Modules = reusable, parameterized infrastructure patterns — directly explains why Hermes needed 18 of them rather than one monolithic config.

---

## Cluster 2: Providers, Resources, Variables — The Basic Building Blocks

### What is it?
A **provider** is a plugin that lets Terraform talk to a specific platform's API (`aws`, `google`, `azurerm`) — it's what translates your declarative config into actual API calls. A **resource** is a single infrastructure object you're declaring (one Lambda function, one DynamoDB table). A **variable** parameterizes a config so the same code can be reused across environments/instances without duplication (exactly what makes a module reusable, per Cluster 1's modules discussion).

### How does it work?
```hcl
provider "aws" { region = var.aws_region }

variable "table_name" {
  type    = string
  default = "events"
}

resource "aws_dynamodb_table" "event_store" {
  name         = var.table_name
  billing_mode = "PAY_PER_REQUEST"
  hash_key     = "pk"
  attribute { name = "pk", type = "S" }
}

output "table_arn" { value = aws_dynamodb_table.event_store.arn }   # exposes a value for other modules/configs to use
```

### Interview Questions
- **L1:** "A provider is the plugin connecting Terraform to a platform's API; a resource declares one specific infrastructure object; variables parameterize config for reuse across environments — outputs expose values for other modules or configs to consume."
- **L2:** explain why declaring the same logical resource (e.g., a DynamoDB table) as a **module with variables** rather than **copy-pasted resource blocks per environment** matters specifically for consistency: a bug fix or a tagging-policy change made once in the module automatically applies everywhere it's used, versus needing to be manually propagated to every copy-pasted instance — directly the same "why 18 modules, not 18 hand-copied configs" argument from `13_PROJECT_DEEP_DIVES/HERMES.md`.
- **L3:** explain `output` values' role in a multi-module setup: a resource's ID/ARN generated in one module (e.g., a DynamoDB table's ARN) often needs to be referenced by another module (e.g., an IAM policy granting a Lambda access to that specific table) — `output` is how a value crosses that module boundary cleanly, rather than hardcoding IDs across modules.

### Quick Revision (Full File)
- State = Terraform's record of reality; `plan` = diff; `apply` = execute the diff.
- Remote state + locking = required for safe team/multi-runner use, prevents concurrent-apply corruption.
- Modules = reusable, parameterized infrastructure patterns — directly explains why Hermes needed 18 of them rather than one monolithic config.
- Provider = platform API plugin. Resource = one declared infrastructure object. Variable = parameterization enabling reuse. Output = how a value crosses a module boundary cleanly.