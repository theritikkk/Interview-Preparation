# Phase 5 — Cloud and AWS

> **Why this phase matters:** RTDS runs infrastructure that may sit on cloud IaaS. You do not need to be an AWS solutions architect, but you must be able to explain why a private subnet has no internet, why a security group and a NACL disagree, how an IAM policy is evaluated, and how EKS Pods get VPC IPs. These are the same routing, NAT, DNS, and identity concepts from Phases 1–4 with different control planes.

![OpenStack VM flow](../images/openstack-vm-flow.svg)

> The diagram is the OpenStack flow you will study in Phase 6. Keep it in mind: AWS and OpenStack solve the same problems (API → identity → scheduler → network → storage) with different names. The comparison is the point.

## Outcomes

By the end of this phase you can:

1. Explain Regions, Availability Zones, edge locations, and why an AZ is a failure domain.
2. Design a VPC with public, private, and isolated subnets, correct route tables, and a NAT path.
3. Explain security groups vs network ACLs, including statefulness and ephemeral ports.
4. Read an IAM policy and predict allow/deny using explicit deny, SCPs, resource policies, and trust policies.
5. Use IAM roles and STS instead of long-lived access keys; explain instance profiles and IRSA.
6. Launch EC2 with a launch template, attach EBS, and put it behind an ALB with health checks.
7. Explain EKS control plane vs data plane, VPC CNI, IRSA/Pod Identity, EBS CSI, and the AWS Load Balancer Controller.
8. Diagnose the common cloud failures: no route, SG vs NACL, private subnet egress, IAM AccessDenied, unhealthy target, Pod IP exhaustion, and a pending LoadBalancer.
9. Destroy everything you created and prove it with a cleanup check.

## Prerequisites

- Phase 2 networking (CIDR, routes, NAT, DNS, TCP).
- Phase 3 storage (block vs object, IOPS, snapshots).
- Phase 4 Kubernetes (Pods, Services, Ingress, PV/PVC, RBAC).
- An AWS account you are allowed to use for labs, with billing alerts enabled.

## Environment and cost safety

| Lab | Creates | Relative cost | Cleanup |
|---|---|---|---|
| L5.1 VPC | VPC, subnets, IGW, NAT GW, 2 EC2, SG/NACL, flow logs | NAT GW is hourly + data; EC2 hourly | Delete NAT GW, EIP, instances, VPC |
| L5.2 IAM | Role, policy, S3 bucket, temporary credentials | S3 storage is tiny; no NAT if you use the CLI from your machine | Delete role/policy/bucket |
| L5.3 EC2/ALB | Launch template, ASG, ALB, EBS | ALB hourly + LCU; EC2/EBS | Delete ASG, ALB, TG, LT, SG, snapshots |
| L5.4 EKS | EKS control plane, node group, NAT, LB, EBS | Highest; control plane is hourly, nodes/NAT/LB add up | `eksctl delete cluster`, verify VPC cleanup |

> **Do not use a production account.** Use a sandbox account or a dedicated OU. Set a budget alert before creating anything. Check the current AWS pricing page for your region; prices change and free tiers change. The labs assume a few hours, not a month.

> **Root account rules:** MFA on root, no root access keys, no daily use. Create an administrative role/user with MFA for labs. Never commit credentials. Use `aws configure sso` or short-lived STS credentials.

## Study order

| Order | Note | Key question |
|---|---|---|
| 1 | [AWS foundations and IAM](01-aws-foundations-and-iam.md) | Who is allowed to do what, and how do I prove it? |
| 2 | [VPC networking](02-vpc-networking.md) | Why can this subnet not reach the internet? |
| 3 | [EC2, EBS, and load balancing](03-compute-storage-load-balancing.md) | Is the instance healthy, the target healthy, or the app healthy? |
| 4 | [EKS and AWS Kubernetes networking](04-eks-and-aws-kubernetes.md) | How do Pods get VPC IPs, and how do they get AWS permissions? |
| 5 | [Labs part 1](05-labs-part1.md) | Can I build a VPC and prove IAM least privilege? |
| 6 | [Labs part 2](06-labs-part2.md) | Can I run a load-balanced app and an EKS workload? |
| 7 | [Checkpoint](07-checkpoint.md) | Can I diagnose cloud failures from evidence? |

## The shared responsibility model

```text
AWS responsible for:            You responsible for:
- physical security             - IAM users/roles/policies
- hypervisor                   - security groups, NACLs, routing
- managed service availability  - OS patching (EC2), data encryption
- global network               - application security, backups, cost
```

"Managed" does not mean "secure by default". EKS manages the control plane; you still manage node groups, IAM, CNI, storage, and workload security. RDS manages the database engine; you still manage credentials, network access, and backups.

## The cloud translation table

| Concept | AWS | OpenStack | Kubernetes |
|---|---|---|---|
| Virtual network | VPC | Neutron network | CNI network |
| Subnet | Subnet (AZ-scoped) | Subnet | Pod CIDR / node CIDR |
| Router | Route table + IGW/NAT | Neutron router | Node routing / CNI |
| Firewall | Security group, NACL | Security group | NetworkPolicy |
| Identity | IAM | Keystone | RBAC + ServiceAccount |
| Virtual machine | EC2 | Nova instance | Pod |
| Block volume | EBS | Cinder volume | PV/PVC + CSI |
| Object store | S3 | Swift | Object store (external) |
| Load balancer | ALB/NLB | Octavia | Service/Ingress |
| Image | AMI | Glance image | Container image |

## Completion criterion

You can:

- Draw a VPC with two AZs and explain every route table entry.
- Given an SG and NACL, predict whether a connection succeeds, including the return path.
- Read an IAM policy and a trust policy and explain who can assume the role.
- Use `aws sts get-caller-identity` and CloudTrail to prove which principal made a call.
- Explain why a private subnet needs a NAT gateway (or endpoints) and what it costs.
- Diagnose an ALB 503 from target health, not from the app logs alone.
- Explain how the VPC CNI assigns Pod IPs and why subnet sizing matters.
- Explain IRSA/Pod Identity and why it is better than node-instance credentials.
- Tear down every resource and verify no NAT gateway, LB, or EBS volume remains.

Then take [the checkpoint](07-checkpoint.md).
