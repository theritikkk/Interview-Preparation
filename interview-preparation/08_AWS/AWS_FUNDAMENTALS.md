# AWS Fundamentals

---

## IAM & Least Privilege

### What is it?
IAM controls *who* (principal — user, role, service) can do *what* (action) to *which* resource. Least privilege means granting only the permissions actually needed, nothing broader "just in case."

### How does it work?
```json
{
  "Effect": "Allow",
  "Action": ["dynamodb:GetItem", "dynamodb:PutItem"],
  "Resource": "arn:aws:dynamodb:*:*:table/Events",
  "Condition": { "StringEquals": { "dynamodb:LeadingKeys": "${aws:PrincipalTag/tenantId}" } }
}
```
That `Condition` block is the IAM-level version of tenant isolation — restricting access to only items whose partition key matches the caller's own tenant tag, as a defense-in-depth layer *underneath* your application-level Cognito/RBAC checks.

### Interview Questions
- **L1:** "IAM grants specific principals specific actions on specific resources; least privilege means granting the minimum needed, reducing blast radius if credentials are compromised."
- **L2:** explain **roles vs. users**: a Lambda function assumes an execution role (temporary credentials, no long-lived secret to leak) rather than using a static IAM user's access keys — directly relevant to how Hermes's Lambdas would authenticate to DynamoDB/SQS/EventBridge.
- **L3 — 🔴 Hermes-relevant:** explain defense-in-depth for multi-tenancy: application-level checks (Cognito JWT claims, RBAC) *and* IAM-level conditions (like the policy above) are two independent layers — a bug in one doesn't automatically compromise tenant isolation if the other still holds. `NEEDS VERIFICATION`: confirm whether Hermes actually uses IAM-level conditions or relies solely on application-level tenant checks before claiming defense-in-depth as a feature.

### Quick Revision
- Roles (assumed, temporary credentials) > long-lived user access keys for service-to-service auth.
- Least privilege = default-deny, then add only what's needed.
- IAM conditions can enforce tenant isolation at the infrastructure layer, independent of application code.
