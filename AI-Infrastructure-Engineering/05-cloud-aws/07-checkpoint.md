# Phase 5 Checkpoint — Cloud and AWS

> **Rules:** closed book, 90 minutes for Parts A–D. No console, no CLI, no notes, no search. Then do the practical sign-off in a sandbox account. Target: **80% overall**, and at least **32/40** on the diagnosis scenarios.
>
> **Assumptions:** region `us-east-1`, AWS CLI v2, EKS 1.29+, a modern VPC CNI, and prices from the period when this was written. Prices are illustrative; check the current pricing page before you rely on a number. You have completed L5.1–L5.4 or the documented read-only alternative. If you did not run a lab, say so; do not claim operational evidence you do not have.

> **Safety:** use a sandbox account, never production. Set a budget alert before the practical. Delete NAT gateways, load balancers, EIPs, EBS volumes, and the EKS cluster when finished. A checkpoint is not a reason to leave billable resources running.

---

## Part A — Rapid facts (25 × 2 = 50 points)

1. What is the difference between a Region and an Availability Zone?
2. Why is a single-AZ architecture a single failure domain even if it has three instances?
3. What can the root user do that an IAM administrator cannot?
4. What is the difference between an IAM user and an IAM role?
5. What is an SCP, and can it grant permissions?
6. What is a permissions boundary?
7. Put these in evaluation order: explicit deny, SCP, identity allow, implicit deny.
8. What is the difference between a role's trust policy and its permissions policy?
9. What does `iam:PassRole` allow, and why is `Resource: "*"` dangerous?
10. What is a confused deputy, and how does `sts:ExternalId` reduce the risk?
11. What does `aws sts get-caller-identity` prove, and what does it not prove?
12. What is IMDSv2, and what class of attack does it mitigate?
13. Explain IRSA in one sentence.
14. What does CloudTrail record that VPC Flow Logs do not?
15. What makes a subnet public in AWS?
16. How many usable IPv4 addresses are in a `/24` subnet, and why?
17. What is the difference between an internet gateway and a NAT gateway?
18. What is the difference between a security group and a network ACL?
19. Why does a NACL need ephemeral-port rules when a security group does not?
20. Which route wins for `10.1.2.3`: `10.1.0.0/16` via peering or `0.0.0.0/0` via NAT? Why?
21. What is the difference between a gateway endpoint and an interface endpoint?
22. Why is an EBS volume tied to one Availability Zone?
23. What causes an ALB 502, a 503, and a 504?
24. Why is an EC2 instance `running` not proof that it is healthy?
25. How does the VPC CNI give a Pod an IP, and does it enforce Kubernetes NetworkPolicy?

---

## Part B — Calculations (6 × 5 = 30 points)

### B1 — EKS subnet and Pod IP capacity

An EKS cluster has **6 worker nodes** across two private `/24` subnets, 3 nodes in each subnet.

Assumptions:

- Each node type supports **3 ENIs** and **10 IPv4 addresses per ENI**.
- The VPC CNI can use at most `(ENIs × IPs per ENI) − 1` Pod IPs per node; the missing one is the node's primary IP.
- No prefix delegation.
- Each node and each Pod consumes one address from its subnet.
- AWS reserves 5 addresses per subnet.

Answer:

1. What is the maximum number of Pods per node?
2. How many Pod IPs do the 6 nodes consume in total?
3. How many addresses are consumed in each `/24` by 3 nodes at full Pod density, including the node IPs?
4. How many nodes can one `/24` support at full Pod density before it runs out?
5. What happens to a new Pod when the subnet has no free IPs, and what is the first command you run?

### B2 — IAM decision table

A role `ReadRole` has:

- an identity policy allowing `s3:GetObject` on `arn:aws:s3:::app-data/*`;
- a permissions boundary allowing `s3:GetObject` on `arn:aws:s3:::app-data/public/*`;
- an SCP allowing `s3:*`;
- a bucket policy allowing `ReadRole` to `s3:GetObject` on `arn:aws:s3:::app-data/*`;
- an explicit deny in the identity policy on `arn:aws:s3:::app-data/secret/*`.

For each request, state **allow** or **deny**, and name the controlling policy:

1. `s3:GetObject` on `arn:aws:s3:::app-data/public/report.csv`
2. `s3:GetObject` on `arn:aws:s3:::app-data/secret/keys.txt`
3. `s3:GetObject` on `arn:aws:s3:::other-data/file.txt`
4. `s3:PutObject` on `arn:aws:s3:::app-data/public/new.txt`
5. Which single change would make request 3 succeed without widening access to `app-data/secret/*`?

### B3 — Cost of a short-lived lab

Use these **illustrative** on-demand prices for `us-east-1`:

| Resource | Price |
|---|---|
| NAT gateway | $0.045/hour + $0.045/GB processed |
| EKS control plane | $0.10/hour |
| `m6i.large` EC2 | $0.096/hour |
| gp3 EBS | $0.08/GB-month |
| ALB | $0.0225/hour + $0.008/LCU-hour |
| Data transfer | ignore |

The lab runs for **100 hours** and creates:

- 2 NAT gateways, 50 GB processed total;
- 1 EKS control plane;
- 3 `m6i.large` instances;
- 100 GB gp3;
- 1 ALB with 2 LCUs average.

Answer:

1. What is the approximate total cost?
2. Which resource category is the largest cost?
3. What remains billable if you delete only the node group but leave everything else running for a full 730-hour month?
4. Name three cleanup checks that a console resource list can miss.

### B4 — ALB and ASG capacity

A web tier runs **6 instances** across two AZs, 3 per AZ. Each instance safely handles **250 requests/second** at the target CPU utilization.

Normal traffic is **1,200 requests/second**.

Answer:

1. What is the total designed capacity, and what is the normal utilization?
2. If one AZ fails, what capacity remains, and what happens to 1,200 requests/second?
3. How many instances per AZ are needed to survive one AZ failure with **20% headroom**?
4. The health check is interval 30 s, timeout 5 s, unhealthy threshold 3, healthy threshold 2. What is the worst-case time before the ALB marks a dead target unhealthy?
5. Why does `health_check_type=ELB` matter for the ASG, and what happens if it is left at `EC2`?

### B5 — EBS gp3 sizing and cost

A database uses a **500 GiB gp3** volume. gp3 includes 3,000 IOPS and 125 MB/s. The workload needs **6,000 IOPS** and **400 MB/s**.

Illustrative prices: $0.08/GB-month, $0.005/extra IOPS-month, $0.04/extra MB/s-month.

Answer:

1. What is the monthly storage cost?
2. What is the monthly cost of the extra IOPS and throughput?
3. What is the total monthly cost?
4. After `modify-volume`, `lsblk` shows 500 GiB but `df -h` still shows the old size. What did you forget?
5. Why is EBS Multi-Attach not a substitute for a shared filesystem?

### B6 — Prefix delegation and subnet size

An EKS cluster uses VPC CNI **prefix delegation**. Each node has 3 ENIs, and each ENI is assigned 2 `/28` prefixes. Each `/28` contains 16 addresses. Ignore the node's primary IP for this estimate.

Answer:

1. How many Pod IPs does one node's prefixes reserve?
2. How many nodes fit in one `/24` with 251 usable addresses, before headroom?
3. Is one `/24` enough for 6 nodes with 20% headroom? Show the arithmetic.
4. What is the smallest single subnet that fits 6 nodes plus 20% headroom: `/24`, `/23`, or `/22`?
5. Why is "add more nodes" not the first fix when Pods are Pending with IP-allocation errors?

---

## Part C — Diagnosis scenarios (8 × 5 = 40 points)

For each scenario, name the **fault domain**, the **next two commands or evidence sources**, and the **fix**. Do not guess before naming the evidence. A command that only restarts something is not a diagnosis.

### C1 — S3 access denied

```text
$ aws s3 cp s3://app-data/public/report.csv .
upload failed: ... An error occurred (AccessDenied) when calling the GetObject operation: Access Denied

$ aws sts get-caller-identity
{
    "UserId": "AROAEXAMPLE:session-7",
    "Account": "111122223333",
    "Arn": "arn:aws:sts::111122223333:assumed-role/ReadRole/session-7"
}
```

The role's identity policy visibly allows `s3:GetObject` on `arn:aws:s3:::app-data/*`. The bucket is in the same account.

### C2 — Private instance cannot reach the internet

```text
$ curl -m 5 https://example.com
curl: (28) Connection timed out

$ aws ec2 describe-route-tables \
    --filters Name=association.subnet-id,Values=subnet-private-a \
    --query 'RouteTables[].Routes[]'
[
  {
    "DestinationCidrBlock": "10.20.0.0/16",
    "GatewayId": "local",
    "State": "active"
  },
  {
    "DestinationCidrBlock": "0.0.0.0/0",
    "NatGatewayId": "nat-0abc123",
    "State": "blackhole"
  }
]
```

### C3 — Security group allows 443, connection still times out

```text
$ curl -v --connect-timeout 5 https://10.20.12.10
*   Trying 10.20.12.10:443...
* Connection timed out

$ aws ec2 describe-security-groups --group-ids sg-app \
    --query 'SecurityGroups[0].IpPermissions'
[
  {
    "IpProtocol": "tcp",
    "FromPort": 443,
    "ToPort": 443,
    "IpRanges": [{"CidrIp": "10.20.11.0/24"}]
  }
]
```

The client is in `10.20.11.0/24`; the server is in `10.20.12.0/24`. The server's NACL has an inbound allow for 443 from `10.20.11.0/24`, and an outbound allow for 443 to `10.20.11.0/24`.

### C4 — ALB returns 503

```text
$ curl -I https://app.example.com
HTTP/2 503

$ aws elbv2 describe-target-health --target-group-arn arn:aws:elasticloadbalancing:...
{
  "TargetHealthDescriptions": [
    {
      "Target": {"Id": "i-0abc", "Port": 8080},
      "TargetHealth": {
        "State": "unhealthy",
        "Reason": "Target.FailedHealthChecks",
        "Description": "Health checks failed with these codes: [404]"
      }
    }
  ]
}
```

### C5 — EKS Pods Pending after a scale-up

```text
$ kubectl -n app get pods
NAME    READY   STATUS    RESTARTS   AGE
web-0   0/1     Pending   0          4m

$ kubectl -n app describe pod web-0
Events:
  Warning  FailedScheduling  ...  0/6 nodes are available: 6 Insufficient pods.

$ kubectl -n kube-system logs ds/aws-node --tail=20
... failed to assign an IP address to pod web-0: no free IP addresses in subnet
```

### C6 — IRSA returns the node role

```text
$ kubectl -n app exec s3-reader -- aws sts get-caller-identity
{
    "UserId": "AROAEXAMPLE:i-0abc",
    "Account": "111122223333",
    "Arn": "arn:aws:sts::111122223333:assumed-role/NodeRole/i-0abc"
}

$ kubectl -n app get sa s3-reader -o yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  name: s3-reader
  namespace: app
  annotations:
    eks.amazonaws.com/role-arn: arn:aws:iam::111122223333:role/S3Reader
```

The Pod uses `serviceAccountName: s3-reader`. The IAM role's trust policy condition is:

```json
"Condition": {
  "StringEquals": {
    "oidc.eks.us-east-1.amazonaws.com/id/EXAMPLE:sub": "system:serviceaccount:app:s3-read",
    "oidc.eks.us-east-1.amazonaws.com/id/EXAMPLE:aud": "sts.amazonaws.com"
  }
}
```

### C7 — LoadBalancer Service stays pending

```text
$ kubectl get svc web
NAME   TYPE           CLUSTER-IP     EXTERNAL-IP   PORT(S)        AGE
web    LoadBalancer   10.100.12.44   <pending>     80:31234/TCP   6m

$ kubectl -n kube-system logs deploy/aws-load-balancer-controller --tail=30
... failed to build load balancer: subnet subnet-0abc is not tagged with
    "kubernetes.io/role/elb"
```

### C8 — EBS PVC Pending after a node replacement

```text
$ kubectl -n app get pvc data
NAME   STATUS    VOLUME   CAPACITY   ACCESS MODES   STORAGECLASS
data   Pending                                      ebs-gp3

$ kubectl -n app get pod db-0 -o wide
NAME   READY   STATUS    RESTARTS   NODE
db-0   0/1     Pending   0          <none>

$ kubectl -n app describe pvc data
Events:
  Warning  ProvisioningFailed  ...  failed to provision volume with StorageClass "ebs-gp3":
  ... volume node affinity conflict
```

---

## Part D — Design and explain (2 × 20 = 40 points)

### D1 — Three-AZ VPC and EKS platform (20 points)

Design the network and platform for a small product team running a public HTTP API and two internal services on EKS.

Requirements:

- Three AZs; survive one AZ failure.
- Public subnets only for load balancers and NAT; no public IPs on nodes or Pods.
- Private nodes must pull images from ECR and call S3, STS, and CloudWatch.
- The EKS API endpoint must not be open to the entire internet.
- Human access uses SSO and short-lived credentials; Pods get AWS permissions per workload, not the node role.
- One service reads S3; the EBS CSI driver provisions database volumes; the AWS Load Balancer Controller creates an internet-facing ALB.
- Encrypt volumes; retain database snapshots; centralize control-plane, VPC flow, and CloudTrail logs.
- Keep the dev environment affordable; production can cost more.

Answer:

1. VPC CIDR and subnet plan, with sizes and AZ placement.
2. Route tables and egress design, including NAT and VPC endpoints. Explain the cost/HA trade-off.
3. Security groups and NACLs: what you allow, and why you do not rely on NACLs for per-workload policy.
4. IAM roles and trust: node role, IRSA roles for the app/EBS CSI/LB controller, and human access. Include the specific trust conditions.
5. EKS add-ons, node groups, storage class, and load-balancer tags.
6. Observability and audit: which logs/metrics, where they go, and the three alerts you would page on.
7. The top three failure modes of this design and the evidence you would use for each.

### D2 — Identity for humans, CI, and Pods (20 points)

Design access for:

- 20 engineers across `dev`, `staging`, and `prod`;
- GitHub Actions deploying to each environment;
- application Pods that need read-only access to one S3 prefix;
- a break-glass path for an on-call engineer;
- an auditor who needs read-only configuration history but no data access.

Requirements:

- No long-lived IAM user access keys.
- Least privilege, with explicit environment separation.
- A compromised CI job must not be able to deploy to `prod` without an approval.
- Every access decision must be attributable to a human or a workload.

Answer:

1. The human access model: Identity Center, permission sets, groups, MFA, and session duration.
2. The CI/CD trust model: OIDC provider, trust policy conditions, one role per environment, and how to prevent a `dev` job from assuming the `prod` role.
3. The Pod identity model: IRSA or Pod Identity, trust policy conditions, and why the node role is not acceptable.
4. Break-glass: who can use it, how it is protected, and how you detect misuse.
5. Auditor access: which AWS services and logs, and why it is not the same as `AdministratorAccess` with read-only intent.
6. The controls that make this auditable: CloudTrail, Access Analyzer, SCPs, permission boundaries, tagging, and access reviews.
7. Two ways this design can fail operationally, and the signal for each.

---

## Answer key

### Part A

1. A Region is a geographic area containing multiple isolated AZs; an AZ is one or more discrete data centers with independent power and network, and is the basic failure domain.
2. All three instances share the same AZ's power, cooling, and network fate; an AZ failure takes them all down. Multi-AZ is required for AZ-failure tolerance.
3. The root user is the account owner and cannot be fully restricted by IAM; it can perform account-level tasks such as closing the account and changing certain account settings. It should be MFA-protected and used only for break-glass tasks.
4. An IAM user is a long-lived identity with credentials (password/access keys). An IAM role is assumed temporarily via STS, has no long-lived password/access key, and is the default for workloads and cross-account access.
5. A Service Control Policy is an AWS Organizations guardrail that limits the maximum permissions for accounts/OUs. It never grants permissions; an SCP allow only avoids an SCP deny.
6. A permissions boundary is a managed policy that sets the maximum permissions for an IAM user or role. It limits but does not grant; the effective permission is the intersection of identity allows, boundary, and SCPs, minus explicit denies.
7. Explicit deny → SCP/boundary limits → identity/resource allow → implicit deny. An explicit deny at any applicable layer wins.
8. The trust policy answers "who may assume this role?" (principal, action `sts:AssumeRole`/`AssumeRoleWithWebIdentity`, conditions). The permissions policy answers "what may the assumed role do?" A trust-policy problem cannot be fixed by editing the permissions policy.
9. `iam:PassRole` lets a principal pass a role to a service (EC2, EKS, Lambda). With `Resource: "*"`, the principal can attach a highly privileged role to a resource it controls, escalating privileges. Scope it to specific role ARNs and add `iam:PassedToService`.
10. A confused deputy is a service tricked into using its own permissions on another party's behalf. `sts:ExternalId` binds the trust to a known external identifier that an attacker cannot guess, and is common for third-party SaaS roles.
11. It proves the effective principal ARN and account for the current credentials. It does not prove authorization for a specific action, the region used by later calls, or that the principal is the human you think it is (a role session name may be shared).
12. IMDSv2 is the token-based EC2 Instance Metadata Service. A `PUT` request obtains a session token, which must be sent on subsequent metadata requests. It mitigates SSRF-based credential theft and is the recommended/default setting.
13. IRSA federates a Kubernetes ServiceAccount to an IAM role through the cluster's OIDC provider, so the Pod receives short-lived AWS credentials scoped to that ServiceAccount instead of the node role.
14. CloudTrail records AWS API calls: who called what, when, from where, and whether it was allowed or denied. Flow Logs record network flow metadata (IPs, ports, protocol, bytes, action), not API identity or payload.
15. Its route table has a route to an internet gateway (and the resource has a public IPv4/EIP or is behind a public load balancer). The name "public subnet" does not make it public.
16. 251. AWS reserves 5 addresses: network address, VPC router, DNS, future use, and broadcast. A `/24` has 256 addresses total.
17. An IGW is attached to the VPC and supports bidirectional internet traffic for resources with public addresses. A NAT gateway sits in a public subnet, has an EIP, supports outbound-initiated traffic only, and bills hourly plus per GB.
18. A security group is a stateful, allow-only firewall attached to an ENI/resource. A NACL is a stateless, allow/deny, numbered firewall attached to a subnet; it evaluates rules in order and must allow both directions.
19. A NACL does not track connection state, so the server's response to the client's ephemeral port (1024–65535) must be explicitly allowed in the server subnet's egress and the client subnet's ingress. A security group automatically allows return traffic for an allowed connection.
20. The `/16` peering route, because longest-prefix match wins. The default route is used only when no more-specific route matches.
21. A gateway endpoint (S3/DynamoDB) is a route-table target with no ENI; an interface endpoint is an ENI in your subnet with an SG, private DNS, and PrivateLink. Gateway endpoints are not billed hourly; interface endpoints are.
22. EBS is network-attached block storage in a specific AZ; the volume's replicas and network path are AZ-scoped. To move it, snapshot and restore/copy in the target AZ.
23. 502: the ALB reached the target but the target closed/reset the connection or returned an invalid response. 503: no healthy targets. 504: the target did not respond before the timeout. Each points to a different fault domain.
24. `running` only means the VM is powered on. The instance may have failed system/instance status checks, no route, a blocking SG/NACL, a dead application, or a full disk.
25. The VPC CNI allocates VPC IPs from the node's subnet to Pod ENIs, so Pods are first-class VPC addresses. The VPC CNI does **not** enforce Kubernetes NetworkPolicy by itself; use Calico, Cilium, or another policy engine.

### Part B

**B1**

1. `3 × 10 − 1 = 29` Pod IPs per node.
2. `6 × 29 = 174` Pod IPs.
3. Per subnet: `3 × (1 node IP + 29 Pod IPs) = 90` addresses. Each `/24` has 251 usable, leaving 161.
4. `floor(251 / 30) = 8` nodes per `/24` at full density (240 used, 11 free). A ninth node would need 30 addresses and cannot fully allocate.
5. The Pod stays `Pending` (or `ContainerCreating`) with an IP-allocation error. First command: `aws ec2 describe-subnets --subnet-ids <ids> --query 'Subnets[].{ID:SubnetId,Free:AvailableIpAddressCount,CIDR:CidrBlock}'`, then inspect `aws-node` logs. Fix by adding a subnet/CIDR, freeing addresses, enabling prefix delegation, or using custom networking. Restarting the CNI does not create addresses.

**B2**

1. **Allow.** Identity allow, boundary allow, SCP allow, bucket policy allow, no explicit deny.
2. **Deny.** The explicit deny on `app-data/secret/*` wins over every allow.
3. **Deny (implicit).** The identity policy does not allow `other-data/*`; the bucket policy alone is not enough here because the boundary also does not allow it. For same-account access, a resource policy can grant in some services, but the boundary still caps the session; the simplest correct answer is deny.
4. **Deny (implicit).** No policy allows `s3:PutObject`; the identity policy is read-only and the boundary is read-only.
5. Add an identity allow (and boundary allow) for the specific `other-data` prefix, or add a resource policy plus ensure the boundary permits it. Do not add `s3:GetObject` on `*` or remove the secret deny.

**B3**

1. NAT hourly: `2 × 0.045 × 100 = $9.00`. NAT data: `50 × 0.045 = $2.25`. EKS: `0.10 × 100 = $10.00`. EC2: `3 × 0.096 × 100 = $28.80`. EBS: `100 × 0.08 × (100/730) ≈ $1.10`. ALB: `0.0225 × 100 = $2.25`; LCU: `2 × 0.008 × 100 = $1.60`. **Total ≈ $55.00.**
2. EC2 compute is the largest single category at $28.80 (about 52%). In many real labs NAT and data transfer surprise people because they are easy to leave running.
3. Remaining monthly: 2 NAT gateways `2 × 0.045 × 730 = $65.70`; EKS control plane `0.10 × 730 = $73.00`; ALB `0.0225 × 730 = $16.43`; LCU `2 × 0.008 × 730 = $11.68`; 100 GB gp3 `$8.00`; EIPs may add more if not attached. **≈ $175/month** before data processing. Deleting the node group alone is not cleanup.
4. Unattached EBS volumes, old snapshots, idle load balancers, unattached EIPs, NAT gateways, and the EKS control plane. Check Cost Explorer/Billing, not just the EC2 console.

**B4**

1. `6 × 250 = 1,500 req/s`; normal utilization is `1,200 / 1,500 = 80%`.
2. One AZ failure leaves 3 instances = `750 req/s`. Demand is 1,200, so 450 req/s cannot be served; users see 503s (or latency/timeouts depending on queueing).
3. Remaining AZ must serve 1,200 with 20% headroom: `1,200 × 1.2 = 1,440 req/s`; `1,440 / 250 = 5.76`, so **6 instances per AZ**, 12 total across two AZs. A 3-AZ design would need 4 per AZ (12 total) for the same math if one AZ fails and the other two carry the load; recompute for the actual AZ count.
4. Worst case approximately `3 × 30 = 90 s` plus timeout/processing, so about 90–95 s. During that window the LB may still send traffic to a dead target; clients can see 502/504.
5. With `ELB`, the ASG uses the target group's health status to replace instances. With `EC2`, the ASG may keep an instance whose EC2 status checks pass but whose application health check fails, so the LB has no healthy targets even though the ASG thinks the fleet is healthy.

**B5**

1. `500 × $0.08 = $40.00/month`.
2. Extra IOPS: `(6,000 − 3,000) × $0.005 = $15.00`. Extra throughput: `(400 − 125) × $0.04 = $11.00`. Total extras `$26.00`.
3. `$40 + $26 = $66.00/month`.
4. You changed the block device but did not grow the partition and filesystem. Run `lsblk`, `sudo growpart /dev/nvme0n1 1`, then `sudo resize2fs /dev/nvme0n1p1` (ext4) or `sudo xfs_growfs /mountpoint` (XFS).
5. Multi-Attach is only for `io1`/`io2` in the same AZ and requires a cluster-aware filesystem or coordinated application writes. Ext4/XFS on a multi-attached volume will corrupt data; use EFS or a shared filesystem service instead.

**B6**

1. `3 ENIs × 2 prefixes × 16 addresses = 96` addresses per node (plus the node's primary IP if counted).
2. `floor(251 / 96) = 2` nodes per `/24` (192 used, 59 free). A third node needs 96 and does not fit.
3. 6 nodes need `6 × 96 = 576` addresses; with 20% headroom `576 × 1.2 = 691.2`. A `/24` has 251 usable, so **no**.
4. `/22` (1,024 total, 1,019 usable) fits 691. A `/23` has 512 total, 507 usable, which is not enough. If AZ-scoped, use one `/22` per AZ or multiple `/24`s per AZ with careful CNI configuration.
5. The CNI cannot allocate an address; adding nodes may make it worse by consuming more subnet space. First check free IPs, ENI/prefix limits, and CNI logs; then add address space/prefixes, free unused IPs, or redesign with custom networking/prefix delegation.

### Part C

**C1 — Authorization / policy evaluation**

- **Fault domain:** IAM authorization, not S3 availability or networking.
- **Next evidence:** `aws iam list-attached-role-policies --role-name ReadRole` and `aws iam get-policy-version`; `aws iam simulate-principal-policy --policy-source-arn arn:aws:iam::111122223333:role/ReadRole --action-names s3:GetObject --resource-arns arn:aws:s3:::app-data/public/report.csv`; CloudTrail `lookup-events` filtered to `AccessDenied`; check SCPs, permissions boundary, bucket policy, KMS key policy, and `aws:SecureTransport`/VPC endpoint conditions.
- **Fix:** remove the explicit deny or correct the boundary/SCP/bucket policy. If the role has a boundary that does not allow the prefix, widen only that prefix. Do not attach `AmazonS3FullAccess` as a first fix.
- **Why:** the identity policy is only one input; explicit deny, SCP, boundary, bucket policy, and KMS can each deny. `simulate-principal-policy` is a hint, not the full multi-policy truth.

**C2 — Route/NAT**

- **Fault domain:** VPC routing/NAT, not the application or DNS (though DNS may fail as a consequence).
- **Next evidence:** `aws ec2 describe-nat-gateways --nat-gateway-ids nat-0abc123` (state, subnet, EIP); the public subnet's route table (`0.0.0.0/0 → igw-*`); SG/NACL egress and return; `getent hosts example.com`; VPC Flow Logs for `REJECT`.
- **Fix:** recreate the deleted NAT gateway (or repair the route), ensure it is in a public subnet with an EIP and an IGW route, then restore the private route. If the NAT is healthy but the route is blackhole, the target was deleted; updating the route alone may not be enough.
- **Why:** `blackhole` means the route target no longer exists. The instance can be perfectly healthy and still have no egress.

**C3 — NACL return path**

- **Fault domain:** stateless NACL, not the security group.
- **Next evidence:** `aws ec2 describe-network-acls --filters Name=association.subnet-id,Values=subnet-...`; VPC Flow Logs showing `REJECT` on the return direction; `tcpdump` on client/server if you have access; confirm the server is listening with `ss -lntp`.
- **Fix:** allow the server subnet's egress to the client CIDR on ephemeral ports 1024–65535, and the client subnet's ingress from the server on the same range (and the corresponding request-direction rules). Keep the rules as narrow as the client range allows.
- **Why:** the request's SYN may arrive, but the SYN-ACK is dropped by the server subnet's outbound NACL, producing a timeout that looks like an SG problem or a dead app.

**C4 — Target health / health-check path**

- **Fault domain:** target group health check or application route, not the ALB itself.
- **Next evidence:** `aws elbv2 describe-target-groups --target-group-arns ...` (health check path/port/protocol/matcher); `aws elbv2 describe-target-health`; from the instance `curl -v http://127.0.0.1:8080/health`; app/access logs; SG from LB to target.
- **Fix:** correct the health-check path/port/protocol/matcher, or add the missing `/health` route. If the app returns 404 on `/`, do not change the matcher to `200-499` to hide the problem; fix the route or point the check at a real endpoint.
- **Why:** 503 means no healthy targets; a 404 health check is a configuration mismatch, not a crashed app. The ALB access logs and `UnHealthyHostCount` confirm the pattern.

**C5 — VPC CNI/IP exhaustion**

- **Fault domain:** VPC CNI / subnet IP capacity or ENI limits.
- **Next evidence:** `aws ec2 describe-subnets` for `AvailableIpAddressCount`; `kubectl -n kube-system logs ds/aws-node`; `kubectl describe node` for `max-pods`; `aws ec2 describe-instance-types` for ENI/IP limits; check whether prefix delegation is enabled.
- **Fix:** add a subnet/CIDR, free unused IPs, enable prefix delegation (after sizing), or use custom networking. Scale the node group only after confirming address capacity. Restarting `aws-node` is not a fix.
- **Why:** the node may have CPU/memory free but no VPC addresses. The scheduler's "Insufficient pods" and the CNI's "no free IP addresses" point to IPAM, not compute.

**C6 — IRSA trust / web identity**

- **Fault domain:** Pod identity/IRSA, not S3 permissions (yet).
- **Next evidence:** `kubectl -n app get pod <pod> -o yaml | grep serviceAccountName`; `kubectl -n app get sa s3-reader -o yaml`; `aws iam get-role --role-name S3Reader`; `aws iam list-open-id-connect-providers`; check the Pod's projected token and AWS SDK env (`AWS_ROLE_ARN`, `AWS_WEB_IDENTITY_TOKEN_FILE`); CloudTrail `AssumeRoleWithWebIdentity` failures.
- **Fix:** correct the trust policy `sub` from `system:serviceaccount:app:s3-read` to `system:serviceaccount:app:s3-reader` (exact namespace/name), ensure the OIDC provider matches the cluster, and confirm the Pod uses the annotated ServiceAccount. Restart the Pod after fixing the trust/annotation.
- **Why:** `sts get-caller-identity` returning the node role means the SDK fell back to the instance profile; the web identity token was not used or the trust rejected it. Do not broaden the trust to `*` or grant the node role S3 access.

**C7 — Load balancer controller / subnet tags / IAM**

- **Fault domain:** AWS Load Balancer Controller provisioning, not Kubernetes Service DNS.
- **Next evidence:** controller logs (as shown); `kubectl describe svc web` events; controller ServiceAccount annotation and IRSA role; subnet tags `kubernetes.io/role/elb=1` (public) or `kubernetes.io/role/internal-elb=1` (private); controller version and cluster SG rules.
- **Fix:** tag the correct public/private subnets, ensure the controller has the required IAM permissions via IRSA/Pod Identity, and use the right `scheme` annotation. For an internal LB, place it in private subnets and tag them `internal-elb`.
- **Why:** the controller creates AWS resources outside Kubernetes; a missing subnet tag or IAM permission leaves the Service `<pending>` while the Pods are healthy.

**C8 — EBS topology / node affinity**

- **Fault domain:** storage provisioning/topology, not the database process.
- **Next evidence:** `kubectl -n app describe pvc data`; `kubectl get pv <pv> -o yaml` (node affinity/AZ); `kubectl get storageclass ebs-gp3 -o yaml` (`volumeBindingMode`, provisioner, parameters); EBS CSI controller logs; node AZ labels; `aws ec2 describe-volumes` for the volume's AZ.
- **Fix:** use `volumeBindingMode: WaitForFirstConsumer` so the volume is provisioned in the Pod's AZ; if an existing PV is bound to another AZ, restore from snapshot in the correct AZ or reschedule the Pod to the PV's AZ (only if that is operationally acceptable). Do not force-attach a volume across AZs.
- **Why:** EBS is AZ-scoped; a PV's node affinity and the Pod's node selector must agree. A replacement node in a different AZ can expose a latent storage design problem.

### Part D

**D1 — Example design**

1. **CIDR/subnets:** VPC `10.20.0.0/16`. Public `/24` per AZ: `10.20.1.0/24`, `10.20.2.0/24`, `10.20.3.0/24` (ALBs/NAT only). Private `/20` per AZ: `10.20.16.0/20`, `10.20.32.0/20`, `10.20.48.0/20` for nodes/Pods. Optional isolated `/24` per AZ for databases with no default route. This leaves room for growth and avoids a single subnet spanning AZs (which is impossible).
2. **Routing/egress:** each private subnet has its own route table with `0.0.0.0/0` to a NAT gateway in the same AZ's public subnet. Three NAT gateways is the HA production choice; for dev, one NAT is cheaper but is a cross-AZ dependency and a single point of failure. Add a gateway endpoint for S3 and interface endpoints for ECR, STS, CloudWatch, KMS, and EKS if the NAT/data cost justifies them. Endpoints also keep AWS API traffic off the internet. Public route tables point to the IGW; isolated subnets have no default route.
3. **Security groups/NACLs:** SGs are the primary control: ALB SG allows 443 from the internet; node SG allows the ALB and required cluster traffic; Pod-level SGs for sensitive services if supported; EBS/EFS and database SGs allow only the app SG. NACLs are coarse subnet guardrails (allow 443/1024–65535 as needed, deny known bad ranges), not per-workload policy. Keep the default NACL allow or use carefully ordered rules; do not rely on NACLs for application identity.
4. **IAM:** node role has only `AmazonEKSWorkerNodePolicy`, `AmazonEKS_CNI_Policy`, `AmazonEC2ContainerRegistryReadOnly`, and SSM if needed; it is not used by Pods. App IRSA role trusts the cluster OIDC provider with `sub: system:serviceaccount:app:api` and `aud: sts.amazonaws.com`, and allows only the S3 prefix. EBS CSI role trusts `system:serviceaccount:kube-system:ebs-csi-controller-sa` and has `AmazonEBSCSIDriverPolicy` (or narrower). LB controller role trusts its ServiceAccount and has the AWS Load Balancer Controller policy. Humans use IAM Identity Center permission sets mapped to groups, with MFA and short sessions; EKS access entries map admin/view roles. No long-lived access keys.
5. **EKS/add-ons:** private endpoint plus a CIDR-restricted public endpoint for admin access, or private-only with SSM/bastion. Managed node groups in private subnets across 3 AZs (general + memory-optimized), cluster autoscaler/Karpenter as appropriate. Add-ons: VPC CNI, CoreDNS, kube-proxy, EBS CSI, Pod Identity agent, AWS Load Balancer Controller. StorageClass `ebs-gp3` with `ebs.csi.aws.com`, `WaitForFirstConsumer`, encryption, `allowVolumeExpansion`, and a `Retain` policy for database PVs plus snapshots/backups. Tag public subnets `kubernetes.io/role/elb=1` and private `kubernetes.io/role/internal-elb=1`; use Ingress annotations for scheme/certificate.
6. **Observability/audit:** EKS control-plane logs (`api`, `audit`, `authenticator`, `controllerManager`, `scheduler`) to CloudWatch; VPC Flow Logs to CloudWatch/S3; an organization CloudTrail trail to a log-archive account; Container Insights/ADOT metrics; ALB access logs and target health; EBS/CSI and node metrics. Page on: ALB 5xx rate/`UnHealthyHostCount`, API latency/error SLO burn, and node/Pod capacity or IP exhaustion. Alert on NAT/data cost anomalies and certificate expiry too.
7. **Failure modes:** (a) one AZ fails → NAT/ALB/nodes in that AZ are lost; evidence: AZ-scoped health checks, target health, node NotReady. (b) subnet IP exhaustion → Pods Pending with CNI IP errors; evidence: `AvailableIpAddressCount`, `aws-node` logs. (c) IRSA/OIDC misconfiguration → app `AccessDenied` while the node role works; evidence: `sts get-caller-identity` in the Pod, trust policy `sub`/`aud`, CloudTrail. Add: EBS AZ mismatch, LB subnet tags, and a bad health-check path. Each needs a runbook and a dashboard panel.

**D2 — Example identity design**

1. **Humans:** IAM Identity Center with groups `platform-admin`, `developer`, `auditor`; permission sets `Admin`, `PowerUser` (no IAM/Organizations), `ReadOnly`, and a scoped `Deploy` role. MFA required; session duration 1–8 hours depending on role. Assignments are per account/OU, not shared users. No IAM users for humans; no access keys. EKS access entries map the SSO roles to Kubernetes RBAC (view/edit/admin per environment).
2. **CI/CD:** GitHub Actions OIDC provider in each account; a role per environment with a trust policy restricted to `repo:org/repo:ref:refs/heads/main` (or `environment:prod`) and `aud: sts.amazonaws.com`. Dev/staging roles are assumable from the main branch; the prod role additionally requires a GitHub Environment with required reviewers, and its trust policy only allows the protected environment's `sub`. The job gets short-lived credentials, not stored secrets. Scope deploy permissions to the specific EKS cluster/namespace and ECR repo; `iam:PassRole` only for the needed roles. The prod role should not be assumable by the dev role (no role chaining across environments).
3. **Pods:** IRSA or EKS Pod Identity per workload. Trust policy `sub` is exactly `system:serviceaccount:<ns>:<sa>` and `aud` is `sts.amazonaws.com`; permissions are the minimum S3 prefix (`s3:GetObject` on `arn:aws:s3:::bucket/prefix/*`) and maybe `s3:ListBucket` with a prefix condition. The node role is not acceptable because every Pod on the node could reach IMDS and inherit broader permissions; it also makes attribution impossible.
4. **Break-glass:** a dedicated `BreakGlassAdmin` role in the prod account, trusted only by a small group with hardware MFA, with a short max session and a required `sts:ExternalId`/ticket condition if your tooling supports it. CloudTrail alarms on `AssumeRole` for that role, and a ticket/incident ID is required. Review its use after every event; rotate/disable if suspicious. Do not leave it permanently assigned.
5. **Auditor:** `ReadOnlyAccess` plus CloudTrail/Config/CloudWatch Logs read, Security Hub/GuardDuty findings, and S3 `GetObject` only on the audit log bucket if needed. No access to application data buckets, no `s3:GetObject` on customer data, no IAM mutation. Use an organization-level audit account with delegated admin for security services; the auditor should not need to assume a workload role.
6. **Auditability:** organization CloudTrail with immutable log archive and log-file validation; AWS Config rules; IAM Access Analyzer for external access and unused permissions; SCPs to deny root access keys, region restrictions, and disabling of CloudTrail/GuardDuty; permission boundaries on workload roles; consistent tags (`Owner`, `Environment`, `CostCenter`, `Expires`); quarterly access reviews and automated stale-access reports.
7. **Failure modes:** (a) a CI trust policy uses `repo:org/repo:*` and a fork/PR can assume the prod role → restrict `sub` to protected branches/environments and require approvals; signal: unexpected `AssumeRole` source/IP and CloudTrail alarms. (b) an IRSA `sub` wildcard or shared ServiceAccount lets one workload read another's data → per-workload SAs and exact `sub`, plus Access Analyzer and S3 access logs; signal: `GetObject` from an unexpected role/session. Add: human role sprawl and unused permissions — access reviews and Access Analyzer.

---

## Scoring

| Section | Points | Your score | Target |
|---|---:|---:|---:|
| A — facts | 50 | | ≥ 40 |
| B — calculations | 30 | | ≥ 24 |
| C — diagnosis | 40 | | ≥ 32 |
| D — design | 40 | | ≥ 32 |
| **Total** | **160** | | **≥ 128 (80%)** |

If you score below 80%, do not move on. Re-read the specific note, redo the matching lab (L5.1–L5.4) or the read-only alternative, and retake after 48 hours. A conceptual miss in VPC routing, IAM evaluation, or EKS networking is a prerequisite gap for OpenStack and AI infrastructure; fix it before adding more control planes.

---

## Practical sign-off

Do these in a sandbox account, without notes. Each is pass/fail. If you do not have an account, complete the paper alternative for the item and mark it **not executed**; do not claim operational evidence.

- [ ] Prove the active principal and region with `aws sts get-caller-identity` and `aws configure get region`; explain the ARN format.
- [ ] Create a VPC with two AZs, public/private subnets, one NAT per AZ, and route tables. Show the routes and prove a private instance can reach the internet through NAT.
- [ ] Break private egress by deleting the NAT route or the NAT gateway; diagnose it from `describe-route-tables`, `describe-nat-gateways`, and Flow Logs; fix it.
- [ ] Demonstrate SG vs NACL: allow 443 in the SG, remove the NACL ephemeral return rule, reproduce the timeout, then fix it.
- [ ] Read a role's trust policy and permissions policy; use `simulate-principal-policy` and CloudTrail to explain an `AccessDenied`.
- [ ] Launch EC2 with IMDSv2 required, no inbound SSH, and SSM Session Manager; connect with `aws ssm start-session`.
- [ ] Attach and resize an EBS volume; grow the partition and filesystem; prove the size with `lsblk` and `df -h`.
- [ ] Put two instances behind an ALB with an ASG; break the health-check path, observe 503/`unhealthy`, fix it, and observe recovery.
- [ ] Create an EKS cluster, deploy a Pod, and prove the Pod has a VPC IP from the node subnet (`kubectl get pod -o wide`, `describe-subnets`).
- [ ] Configure IRSA for a ServiceAccount; prove `sts get-caller-identity` inside the Pod returns the role; break the trust `sub`, observe the failure, fix it.
- [ ] Provision an EBS PVC with `WaitForFirstConsumer`; delete the Pod and prove the data persists; explain the reclaim policy.
- [ ] Create an Ingress/Service behind the AWS Load Balancer Controller; remove a subnet tag and diagnose the pending LB; restore it.
- [ ] Run a cost check (`aws ce get-cost-and-usage`) and a cleanup check: no NAT gateways, EIPs, ALBs, unattached EBS, old snapshots, or EKS clusters left behind.

**Cleanup proof:** paste the output of the describe commands (redact account IDs) into your lab write-up. A screenshot of the console is weaker than command output because it may hide resources in another region.

---

## Oral exam (12 minutes)

Answer out loud, without notes. Record yourself. If you use a term you cannot define, stop and define it.

1. "Walk me through what happens when an EC2 instance in a private subnet calls `https://s3.amazonaws.com`."
2. "A developer says the security group allows 443 but the connection times out. What are the first three things you check?"
3. "Explain IAM policy evaluation, including an explicit deny and an SCP."
4. "What is the difference between a trust policy and a permissions policy, and which one would you check for an `AssumeRole` failure?"
5. "How does an EKS Pod get an IP, and why can a node have free CPU but still not schedule a Pod?"
6. "How does IRSA work, and why not give every Pod the node role?"
7. "An ALB returns 503. Walk me through the evidence you collect before changing anything."
8. "A `LoadBalancer` Service is pending. What are the possible fault domains?"
9. "How would you design a VPC for three AZs without making NAT a single point of failure?"
10. "Tell me about a mistake you made in these labs, the evidence you used, and what you changed in your process."

---

## Retest log

| Date | A | B | C | D | Total | Weak areas | Remediation | Retest date |
|---|---:|---:|---:|---:|---:|---|---|---|
| | | | | | | | | |
| | | | | | | | | |

When you pass, continue to [Phase 6 — OpenStack](../06-openstack/README.md). The same VPC, identity, storage, and load-balancing questions return with different names; the point is to recognize the pattern, not memorize the console.
