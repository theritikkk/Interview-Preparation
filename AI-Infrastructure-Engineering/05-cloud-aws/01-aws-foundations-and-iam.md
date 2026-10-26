# AWS Foundations and IAM

> **The one-sentence model:** AWS is a set of regional APIs; every API call is authenticated as a principal, authorized by policies that are evaluated with explicit deny winning, and recorded in CloudTrail. IAM is not a list of users; it is a policy evaluation engine.

---

## 1. Global infrastructure

| Concept | What it is | Why it matters |
|---|---|---|
| **Region** | A geographic area with multiple isolated AZs | Data residency, latency, service availability, cost |
| **Availability Zone (AZ)** | One or more discrete data centers with independent power/network | The basic failure domain; spread workloads across ≥ 2 AZs |
| **Edge location** | CloudFront/Route 53 edge site | Caching and DNS; not where your EC2 runs |
| **Local Zone** | A smaller extension of a region near a metro | Low latency for specific users; limited services |
| **Outpost** | AWS infrastructure in your data center | Hybrid workloads; not the default |

An AZ name (`us-east-1a`) is mapped differently across accounts. Use AZ IDs (`use1-az1`) when coordinating across accounts.

```bash
aws ec2 describe-availability-zones --region us-east-1
aws ec2 describe-regions --all-regions --query 'Regions[].RegionName' --output table
```

**Design rule:** a single-AZ architecture is a single failure domain, even if it has multiple instances. Multi-AZ costs more (cross-AZ data transfer, duplicate NAT/LB) but is the baseline for production.

---

## 2. Accounts and the root user

- The **root user** is the account owner; it cannot be restricted by IAM policies. Use it only for the few tasks that require it (account closure, some billing changes).
- Enable MFA on root, remove root access keys, and do not use root for daily work.
- Use **AWS Organizations** to group accounts into OUs and apply **Service Control Policies (SCPs)**.
- Use **IAM Identity Center** (SSO) for human access across accounts; it issues short-lived credentials.
- Use **IAM roles** for workloads and cross-account access; avoid long-lived IAM user access keys.

```text
Organization
├── OU: Security        → log archive, security tooling accounts
├── OU: Infrastructure  → shared network, DNS
├── OU: Workloads
│   ├── dev
│   ├── staging
│   └── prod
└── OU: Sandbox         → time-limited experiments, budget alarms
```

SCPs are guardrails, not grants. An SCP that allows `s3:*` does not give any principal permission; it only avoids denying it. An SCP `Deny` overrides an IAM `Allow`.

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Sid": "DenyLeaveOrganization",
      "Effect": "Deny",
      "Action": "organizations:LeaveOrganization",
      "Resource": "*"
    },
    {
      "Sid": "DenyRootAccessKeys",
      "Effect": "Deny",
      "Action": "iam:CreateAccessKey",
      "Resource": "*",
      "Condition": {
        "StringEquals": {"aws:PrincipalType": "Account"}
      }
    }
  ]
}
```

---

## 3. IAM principals and policy types

| Principal | What it is | Use |
|---|---|---|
| **Root user** | Account owner | Break-glass only |
| **IAM user** | Long-lived identity with credentials | Legacy; avoid for humans |
| **IAM role** | Identity assumed temporarily via STS | Workloads, cross-account, federation |
| **Federated identity** | External IdP (SAML/OIDC) | Workforce SSO |
| **AWS service** | Service principal (e.g., `ec2.amazonaws.com`) | Service-linked roles |

Policy types:

| Policy | Attached to | Grants or limits? |
|---|---|---|
| **Identity-based** | User, group, role | Grants (allow) or denies |
| **Resource-based** | S3 bucket, SQS queue, KMS key, role trust | Grants cross-account/same-account access; trust policy defines who can assume a role |
| **SCP** | Organization/OU/account | Limits maximum permissions; never grants |
| **Permissions boundary** | User/role | Limits maximum permissions; never grants |
| **Session policy** | Temporary session | Limits the session; never grants |
| **ACL** | S3 bucket/object (legacy) | Coarse grants; avoid in favor of policies |

### Evaluation logic

```text
1. Is there an explicit Deny in any applicable policy?       → DENY
2. Is the principal within an SCP that does not allow it?    → DENY
3. Is it outside the permissions boundary/session policy?    → DENY
4. Is there an Allow in an identity policy or resource policy? → ALLOW
5. Otherwise                                                 → DENY (implicit)
```

For cross-account access, both sides usually need to allow: the resource policy/trust policy and the caller's identity policy. For same-account access, a resource-based policy can grant access directly for many services, but the principal still needs a valid identity/session.

**Explicit deny always wins**, including a deny in an SCP, a permissions boundary, a bucket policy, or a session policy.

### Policy anatomy

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Sid": "ReadAppBucket",
      "Effect": "Allow",
      "Action": ["s3:GetObject", "s3:ListBucket"],
      "Resource": [
        "arn:aws:s3:::app-artifacts-prod",
        "arn:aws:s3:::app-artifacts-prod/*"
      ],
      "Condition": {
        "StringEquals": {"aws:PrincipalTag/team": "platform"},
        "Bool": {"aws:SecureTransport": "true"}
      }
    }
  ]
}
```

| Element | Meaning |
|---|---|
| `Effect` | `Allow` or `Deny` |
| `Action` | API actions (`s3:GetObject`); wildcards are broad |
| `Resource` | ARN(s); `*` is broad |
| `Condition` | Context keys: principal tags, source IP, MFA, TLS, VPC endpoint, time |
| `Principal` | Only in resource/trust policies; who the statement applies to |

### Trust policies

A role has two policies: the **trust policy** (who may assume it) and the **permissions policy** (what the role can do). A common mistake is fixing the permissions policy when the trust policy is the problem.

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Principal": {"AWS": "arn:aws:iam::111122223333:root"},
      "Action": "sts:AssumeRole",
      "Condition": {
        "StringEquals": {"sts:ExternalId": "opslab-2026"},
        "Bool": {"aws:MultiFactorAuthPresent": "true"}
      }
    }
  ]
}
```

The `Principal` can be an account, a role, a user, a federated provider, or a service. `"AWS": "arn:aws:iam::111122223333:root"` means "the account", not the root user specifically; the caller still needs `sts:AssumeRole` permission in its identity policy.

**Confused deputy:** a service assumes your role on behalf of another customer. Use `sts:ExternalId` and tight `Principal`/conditions for third-party access.

**PassRole:** attaching a role to EC2/EKS/Lambda requires `iam:PassRole` on that role. Granting `iam:PassRole` with `Resource: "*"` lets a principal attach a more privileged role to a service it controls.

---

## 4. STS and temporary credentials

AWS Security Token Service issues short-lived credentials.

| API | Use |
|---|---|
| `AssumeRole` | Role chaining, cross-account, service roles |
| `AssumeRoleWithWebIdentity` | OIDC federation (EKS IRSA, GitHub Actions) |
| `AssumeRoleWithSAML` | SAML workforce federation |
| `GetSessionToken` | MFA-protected temporary credentials for an IAM user |
| `GetCallerIdentity` | Who am I? (no permission required) |

```bash
aws sts get-caller-identity
```

```json
{
    "UserId": "AROA...:session-name",
    "Account": "111122223333",
    "Arn": "arn:aws:sts::111122223333:assumed-role/AdminRole/session-name"
}
```

The `Arn` tells you whether you are a user, an assumed role, or a federated session. If it is not what you expect, stop before debugging the application.

```bash
aws sts assume-role \
  --role-arn arn:aws:iam::111122223333:role/ReadOnlyAudit \
  --role-session-name lab \
  --duration-seconds 900
```

The response contains `AccessKeyId`, `SecretAccessKey`, and `SessionToken`. Export them into a profile or use `--profile` with a configured `role_arn`/`source_profile`. Temporary credentials expire; a cached shell with expired credentials produces `ExpiredToken`.

### Credential chain

The AWS CLI/SDK looks for credentials in a order roughly:

1. Command-line/environment (`AWS_ACCESS_KEY_ID`, `AWS_PROFILE`).
2. Shared credentials/config (`~/.aws/credentials`, `~/.aws/config`).
3. SSO cache.
4. Container/instance role (ECS task role, EC2 instance profile).
5. IMDS (EC2) or EKS web identity/Pod Identity.

`AWS_PROFILE`, `AWS_REGION`, and `AWS_DEFAULT_REGION` change behavior silently. Always check `aws sts get-caller-identity` and the region before diagnosing an access problem.

### Instance profiles and IMDS

An EC2 instance profile attaches a role to an instance. The SDK retrieves temporary credentials from the Instance Metadata Service.

- **IMDSv1** is a simple HTTP GET; SSRF can steal credentials.
- **IMDSv2** requires a session token (`PUT /latest/api/token`) and is the default/recommended setting.
- Set `HttpTokens=required`, `HttpPutResponseHopLimit=1` (or 2 for containers), and restrict IMDS access with network policy.

```bash
TOKEN=$(curl -sX PUT http://169.254.169.254/latest/api/token \
  -H 'X-aws-ec2-metadata-token-ttl-seconds: 60')
curl -s -H "X-aws-ec2-metadata-token: $TOKEN" \
  http://169.254.169.254/latest/meta-data/iam/security-credentials/
```

If this returns `401 Unauthorized`, IMDSv2 is required and you used v1. If it returns a role name but the app gets `AccessDenied`, the role's policy is the problem, not the metadata.

### IRSA in one paragraph

EKS IRSA (IAM Roles for Service Accounts) federates a Kubernetes ServiceAccount to an IAM role through the cluster's OIDC provider. The Pod gets a projected web identity token, the AWS SDK calls `AssumeRoleWithWebIdentity`, and STS returns temporary credentials scoped to that ServiceAccount. The trust policy restricts `sub` to `system:serviceaccount:<namespace>:<name>` and `aud` to `sts.amazonaws.com`. This avoids giving every Pod on a node the node role's permissions. EKS Pod Identity is the newer alternative with an EKS add-on and role associations; the principle is the same: per-workload identity, short-lived credentials.

---

## 5. CloudTrail, Config, and evidence

- **CloudTrail** records API calls (management events by default; data events such as S3 object reads are optional and cost extra). It answers "who called what, when, from where, and was it allowed?"
- **AWS Config** records resource configuration changes and evaluates rules.
- **IAM Access Analyzer** finds resources shared externally and can generate least-privilege policies from CloudTrail.
- **GuardDuty** detects suspicious activity; **Security Hub** aggregates findings.
- **VPC Flow Logs** answer network-level questions; CloudTrail answers API-level questions. Do not confuse them.

```bash
aws cloudtrail lookup-events \
  --lookup-attributes AttributeKey=EventName,AttributeValue=RunInstances \
  --max-results 10
```

```json
{
  "EventName": "RunInstances",
  "Username": "alice",
  "EventTime": "2026-10-27T10:14:03Z",
  "SourceIPAddress": "203.0.113.10",
  "errorCode": "AccessDenied"
}
```

`Username` may be a role session name, not a human. Correlate with `userIdentity.arn` and `sessionContext`.

---

## 6. Billing and account hygiene

1. Enable a **budget** with alerts at 50/80/100% of a small monthly amount before creating anything.
2. Enable **Cost Explorer** and tag resources (`Project`, `Owner`, `Environment`, `Expires`).
3. Use a single region for labs; avoid cross-region data transfer.
4. Prefer **NAT instances** or VPC endpoints over NAT Gateways for long-lived labs if cost matters; NAT Gateway is convenient but hourly.
5. Delete unattached EBS volumes, old snapshots, idle load balancers, and unused Elastic IPs. These are common surprise charges.
6. Use **Savings Plans/Spot** only after the basics; do not optimize a lab.
7. Check the **Billing → Bills** page and **Cost Explorer** after cleanup, not just the console resource list.

```bash
aws ce get-cost-and-usage \
  --time-period Start=2026-10-01,End=2026-10-28 \
  --granularity DAILY \
  --metrics UnblendedCost \
  --group-by Type=DIMENSION,Key=SERVICE
```

---

## 7. Failure scenarios

### A — `AccessDenied`

```text
An error occurred (AccessDenied) when calling the ListBucket operation:
Access Denied
```

Fault domain: authorization. Identify the principal (`aws sts get-caller-identity`), the action/resource, and the policy path. Check identity policy, resource policy, SCP, permissions boundary, and explicit deny. CloudTrail shows the denied event. `aws iam simulate-principal-policy` can test a policy, but it does not know every SCP/resource policy context.

### B — `ExpiredToken`

The credentials in the shell/profile are stale. Re-authenticate (`aws sso login`, assume the role again) or refresh the instance profile. Do not create a new access key as the first fix.

### C — `InvalidClientTokenId` / `SignatureDoesNotMatch`

The access key is wrong/revoked, or the secret is mismatched, or the clock is skewed. Check `aws sts get-caller-identity`, the profile, and the system clock.

### D — Works in the console, fails in CLI

The console may use a different role/session, region, or account. Check the console's account/role indicator and `aws sts get-caller-identity`; check `AWS_PROFILE` and `AWS_REGION`.

### E — `not authorized to perform: iam:PassRole`

The caller can create the service resource but cannot attach the role. Grant `iam:PassRole` on the specific role ARN, with a condition restricting `iam:PassedToService`.

### F — Role cannot be assumed

The trust policy is the likely cause, not the permissions policy. Check the `Principal`, `ExternalId`, MFA condition, and the caller's `sts:AssumeRole` permission. CloudTrail shows `AssumeRole` failures.

### G — SCP blocks an admin

An SCP deny overrides even `AdministratorAccess`. Check the organization's SCPs and the account's effective policies. The error may look like a normal `AccessDenied`.

---

## 8. Command field manual

### `aws sts get-caller-identity`

**Purpose:** prove which principal and account the CLI is using.

```bash
aws sts get-caller-identity --output json
```

**Interpretation:** `Arn` contains `user/`, `assumed-role/`, or `federated-user/`. If it is not the expected identity, stop; every subsequent permission error is suspect.

**Mistake:** assuming the profile name equals the role; `--profile` can point at a role, SSO session, or static keys.

### `aws iam get-policy-version` / `list-attached-role-policies`

**Purpose:** read the actual policy document and attachments.

```bash
aws iam list-attached-role-policies --role-name AppRole
aws iam get-policy-version --policy-arn <arn> --version-id v3
```

**Interpretation:** a role can have multiple policies; the effective permission is the union of allows minus explicit denies, bounded by SCP/boundary.

### `aws iam simulate-principal-policy`

```bash
aws iam simulate-principal-policy \
  --policy-source-arn arn:aws:iam::111122223333:role/AppRole \
  --action-names s3:GetObject \
  --resource-arns arn:aws:s3:::app-artifacts/file.txt
```

**Interpretation:** `EvalDecision` is `allowed`, `implicitDeny`, or `explicitDeny`. It simulates identity policies for the principal; it does not include every resource/SCP context. Use it as a hint, then verify with the real call and CloudTrail.

### `aws cloudtrail lookup-events`

**Purpose:** find who made the API call and whether it was denied.

```bash
aws cloudtrail lookup-events \
  --lookup-attributes AttributeKey=EventName,AttributeValue=AssumeRole \
  --max-results 20
```

**Interpretation:** `errorCode`/`errorMessage` show denials; `userIdentity` shows the principal; `sourceIPAddress` shows where. Management events are recorded by default; data events may not be.

### `aws configure sso`

**Purpose:** set up short-lived human access.

```bash
aws configure sso
aws sso login --profile platform
AWS_PROFILE=platform aws sts get-caller-identity
```

**Mistake:** sharing the SSO cache/credentials; each human should have their own identity.

### `aws ce get-cost-and-usage`

**Purpose:** see actual spend by service, not just the resource list.

```bash
aws ce get-cost-and-usage \
  --time-period Start=2026-10-01,End=2026-10-28 \
  --granularity DAILY --metrics UnblendedCost
```

**Interpretation:** NAT Gateway, data transfer, and idle load balancers often dominate a small lab. Deleting the obvious EC2 instance may not delete the NAT gateway.

---

## 9. Checkpoint

1. What is the difference between a Region and an AZ?
2. Why is a single-AZ architecture a single failure domain?
3. What can the root user do that an IAM administrator cannot?
4. What is the difference between an IAM user and an IAM role?
5. What is an SCP, and can it grant permissions?
6. What is a permissions boundary?
7. What is the evaluation order for an explicit deny, an SCP, and an allow?
8. What is the difference between a trust policy and a permissions policy?
9. What does `iam:PassRole` allow, and why is it sensitive?
10. What is a confused deputy, and how does `ExternalId` help?
11. What does `aws sts get-caller-identity` prove?
12. Why are long-lived access keys riskier than roles?
13. What is IMDSv2, and what problem does it solve?
14. What is IRSA, in one paragraph?
15. What does CloudTrail record that VPC Flow Logs do not?
16. What does a `403` from `aws s3 ls` tell you, and what does it not tell you?
17. Why can an administrator with `AdministratorAccess` still be denied?
18. What is the first thing you check when the CLI behaves differently from the console?
19. Why is a NAT Gateway a common source of surprise cost?
20. What is the safest way to give a CI job access to S3?

### Answers

1. A Region is a geographic area with multiple AZs; an AZ is an isolated failure domain (data center group) within a Region.
2. All resources in one AZ share power/network fate; an AZ failure takes them all down, even if there are multiple instances.
3. The root user is the account owner and cannot be fully restricted by IAM; it can close the account and change account-level settings.
4. A user is a long-lived identity with credentials; a role is assumed temporarily via STS and has no long-lived password/access key.
5. A Service Control Policy is an Organizations guardrail that limits maximum permissions; it never grants.
6. A permissions boundary is a managed policy that sets the maximum permissions for an IAM entity; it does not grant.
7. Explicit deny wins; SCPs and boundaries limit; an allow is considered only within those limits; otherwise implicit deny.
8. The trust policy says who may assume the role; the permissions policy says what the assumed role may do.
9. It lets a principal pass a role to a service (EC2/EKS/Lambda); with `Resource: "*"`, it can attach a highly privileged role to a resource the principal controls.
10. A service is tricked into using its permissions on another customer's behalf; `ExternalId` binds the trust to a known external identifier.
11. The effective principal ARN and account for the current credentials.
12. Access keys are long-lived, often leaked in code/CI, and are not automatically scoped to a workload; roles issue short-lived credentials.
13. IMDSv2 is the token-based EC2 metadata service; it mitigates SSRF credential theft by requiring a session token.
14. IRSA federates a Kubernetes ServiceAccount to an IAM role via the cluster OIDC provider, so Pods get short-lived, per-workload AWS credentials.
15. CloudTrail records API calls and identities; Flow Logs record network flows, not who called an API.
16. It tells you the principal is not authorized for that action/resource (or a policy/SCP/boundary denies it); it does not tell you which policy without further evidence.
17. An SCP, permissions boundary, explicit deny, or resource policy can override the allow.
18. `aws sts get-caller-identity`, the profile/region, and the account/role; the console may be using a different identity.
19. It bills hourly plus data processing even when idle, and it is easy to forget during cleanup.
20. An IAM role with a tight trust policy (OIDC/GitHub Actions) and least-privilege S3 permissions; no long-lived access keys.
