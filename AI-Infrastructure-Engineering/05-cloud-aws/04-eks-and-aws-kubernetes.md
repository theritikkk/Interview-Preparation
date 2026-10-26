# EKS and AWS Kubernetes Networking

> **The one-sentence model:** EKS runs the control plane for you; your VPC runs the data plane. The VPC CNI gives Pods VPC IPs, IRSA or Pod Identity gives Pods AWS permissions, the EBS/EFS CSI drivers give Pods storage, and the AWS Load Balancer Controller gives Ingress/Services AWS load balancers. Every one of those is an integration point that can fail independently of Kubernetes.

---

## 1. What EKS manages

| Layer | EKS manages | You manage |
|---|---|---|
| Control plane | API server, etcd, scheduler, controller-manager, upgrades | Access, logging, audit, version timing |
| Nodes | Nothing by default | AMI/node group, IAM role, scaling, patching, disk |
| Networking | Nothing | VPC/subnets, CNI, SGs, NAT/endpoints, LB |
| Identity | OIDC issuer, cluster auth | IAM roles, RBAC, IRSA/Pod Identity, access entries |
| Storage | Nothing | CSI drivers, StorageClasses, EBS/EFS, backups |
| Addons | Managed addon lifecycle | Which addons, versions, configuration |

EKS is not "Kubernetes on autopilot" unless you choose Fargate/Autopilot-style options. The control plane is highly available; the data plane is your responsibility.

```bash
aws eks list-clusters
aws eks describe-cluster --name opslab --query 'cluster.{Status:status,Version:version,Endpoint:endpoint,Role:roleArn}'
aws eks update-kubeconfig --name opslab --region us-east-1
kubectl get nodes -o wide
kubectl -n kube-system get pods
```

### Control plane endpoint

- **Public:** accessible from the internet (optionally CIDR-restricted). Convenient, larger attack surface.
- **Public + private:** both; private access from the VPC.
- **Private:** only from the VPC/connected networks. Requires a bastion, SSM, VPN, or Direct Connect.

The cluster security group controls control-plane-to-node and control-plane-to-API traffic. Do not confuse it with node/pod security groups.

### Authentication and authorization

```text
aws eks update-kubeconfig
  → kubeconfig uses an exec plugin: aws eks get-token
  → STS token identifies the IAM principal
  → EKS authenticates the IAM principal
  → Kubernetes RBAC/access entries authorize the request
```

Two models:

- **`aws-auth` ConfigMap** (legacy): maps IAM roles/users to Kubernetes users/groups. Easy to lock yourself out.
- **EKS access entries** (newer): AWS API-managed mappings with access policies (`AmazonEKSClusterAdminPolicy`, `AmazonEKSViewPolicy`, etc.). Prefer access entries for new clusters.

```bash
aws eks list-access-entries --cluster-name opslab
aws eks describe-access-entry --cluster-name opslab --principal-arn <arn>
```

**Lockout recovery:** the cluster creator has admin by default (depending on auth mode); an access entry for an admin role is the recovery path. Do not delete the `aws-auth` ConfigMap or the last admin entry while debugging.

---

## 2. Nodes and node groups

| Type | What it is | Use |
|---|---|---|
| **Managed node group** | EKS-managed ASG with EKS-optimized AMI | Default; easiest patching/scaling |
| **Self-managed** | Your ASG/instances, your bootstrap | Custom AMIs, special networking |
| **Fargate** | Serverless Pods, no nodes to manage | Small/irregular workloads; no DaemonSets/privileged |
| **Karpenter** | Just-in-time node provisioning | Cost/efficiency at scale; not a native EKS node group |

Node IAM role needs at least:

- `AmazonEKSWorkerNodePolicy`
- `AmazonEKS_CNI_Policy` (or a custom CNI policy)
- `AmazonEC2ContainerRegistryReadOnly`
- `AmazonSSMManagedInstanceCore` (for SSM access)
- `eks:DescribeCluster` if using the bootstrap script

The node role is **not** the Pod role. If Pods can reach IMDS, they may inherit the node role; IRSA/Pod Identity prevents that.

### Node bootstrap

The node must join the cluster with the right cluster name, API endpoint, CA, and kubelet flags. Managed node groups handle this; self-managed nodes use `bootstrap.sh` in user data. A node that is `NotReady` often has a bootstrap/IAM/networking problem, not a Kubernetes problem.

```bash
aws eks describe-nodegroup --cluster-name opslab --nodegroup-name workers \
  --query 'nodegroup.{Status:status,Desired:scalingConfig.desiredSize,Min:scalingConfig.minSize,Max:scalingConfig.maxSize,Instance:instanceTypes}'
kubectl describe node <node>
```

---

## 3. VPC CNI

The VPC CNI is the default EKS networking plugin. It assigns **VPC IP addresses** to Pods, so Pods are routable in the VPC and visible to SGs/NACLs/flow logs.

```text
Node ENI (primary) ── secondary ENIs ── Pod IPs
  │
  └── aws-node DaemonSet manages ENIs/IPs via the EC2 API
```

Implications:

- **Subnet sizing:** each node/Pod consumes VPC IPs. A `/24` may be too small for a dense cluster.
- **ENI limits:** each instance type has max ENIs and IPs per ENI. The CNI pre-allocates; a node can hit its IP limit while CPU/memory are free.
- **Prefix delegation:** assign `/28` prefixes to ENIs, increasing Pod density; requires planning and supported instance types.
- **Security groups for Pods:** attach SGs directly to Pod ENIs (requires the VPC CNI/security group policy); otherwise the node SG applies.
- **NetworkPolicy:** the VPC CNI does **not** enforce Kubernetes NetworkPolicy by itself. Use Calico/Cilium or another policy engine if you need it.
- **Custom networking:** Pods can use a different subnet/CIDR than the node; useful for IP exhaustion or security separation.

```bash
kubectl -n kube-system get ds aws-node -o wide
kubectl -n kube-system logs ds/aws-node --tail=50
kubectl get pods -A -o wide
aws ec2 describe-subnets --subnet-ids <ids> \
  --query 'Subnets[].{ID:SubnetId,Free:AvailableIpAddressCount,CIDR:CidrBlock}'
```

If Pods are `Pending` with `failed to assign an IP`, inspect subnet free IPs and `aws-node` logs before restarting anything.

---

## 4. IAM for Pods: IRSA and Pod Identity

### IRSA

```text
ServiceAccount annotated with role ARN
  → projected token (aud=sts.amazonaws.com, sub=system:serviceaccount:ns:sa)
  → AWS SDK calls sts:AssumeRoleWithWebIdentity
  → IAM trust policy checks OIDC provider, sub, aud
  → temporary credentials
```

Trust policy:

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Principal": {
        "Federated": "arn:aws:iam::111122223333:oidc-provider/oidc.eks.us-east-1.amazonaws.com/id/EXAMPLED539D4633E53DE1B71EXAMPLE"
      },
      "Action": "sts:AssumeRoleWithWebIdentity",
      "Condition": {
        "StringEquals": {
          "oidc.eks.us-east-1.amazonaws.com/id/EXAMPLED539D4633E53DE1B71EXAMPLE:sub": "system:serviceaccount:app:s3-reader",
          "oidc.eks.us-east-1.amazonaws.com/id/EXAMPLED539D4633E53DE1B71EXAMPLE:aud": "sts.amazonaws.com"
        }
      }
    }
  ]
}
```

ServiceAccount:

```yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  name: s3-reader
  namespace: app
  annotations:
    eks.amazonaws.com/role-arn: arn:aws:iam::111122223333:role/S3Reader
```

```bash
eksctl utils associate-iam-oidc-provider --cluster opslab --approve
eksctl create iamserviceaccount \
  --cluster opslab --namespace app --name s3-reader \
  --attach-policy-arn arn:aws:iam::aws:policy/AmazonS3ReadOnlyAccess \
  --approve
```

**Common failures:** missing OIDC provider, wrong `sub` namespace/name, missing `aud`, annotation on the wrong ServiceAccount, pod not using the ServiceAccount, or the SDK not using web identity (env/credential chain).

### EKS Pod Identity

Newer, simpler for many cases:

```bash
aws eks create-addon --cluster-name opslab --addon-name eks-pod-identity-agent
aws eks create-pod-identity-association \
  --cluster-name opslab \
  --namespace app \
  --service-account s3-reader \
  --role-arn arn:aws:iam::111122223333:role/S3Reader
```

The role trust policy uses `pods.eks.amazonaws.com` and `sts:TagSession`. Pod Identity does not require OIDC annotations. Check the agent DaemonSet and the association status.

### Why not the node role?

If a Pod can reach IMDS, it can use the node's instance profile, which usually has broader permissions and is shared by every Pod on the node. IRSA/Pod Identity scopes credentials to a ServiceAccount.

---

## 5. Storage on EKS

| Driver | Access | Backend | Notes |
|---|---|---|---|
| **EBS CSI** | RWO | EBS volume, one AZ | Most common for databases/stateful sets |
| **EFS CSI** | RWX | EFS file system | Shared files; higher latency |
| **FSx CSI** | RWX/RWO | FSx for Lustre/NetApp/OpenZFS | HPC/AI workloads |
| **Instance store** | RWO | Local NVMe | Ephemeral; no snapshot |

EBS CSI requirements:

- The addon (`aws-ebs-csi-driver`) and its controller/node pods.
- An IAM role for the CSI service account (`AmazonEBSCSIDriverPolicy`) via IRSA/Pod Identity.
- A `StorageClass` with the right `provisioner` (`ebs.csi.aws.com`), `volumeBindingMode: WaitForFirstConsumer`, and `allowVolumeExpansion`.
- The PV is AZ-scoped; scheduling must match.

```bash
aws eks list-addons --cluster-name opslab
kubectl get csidrivers
kubectl get storageclass
kubectl -n kube-system get pods -l app.kubernetes.io/name=aws-ebs-csi-driver
```

**Multi-Attach** on EBS is the same trap as Phase 3: RWO volumes are per node, not per Pod. A Deployment with replicas and one PVC can attach to only one node; use a StatefulSet with per-replica PVCs.

---

## 6. Load balancing on EKS

| Need | Object | Controller |
|---|---|---|
| Internal/external L4 | Service `type: LoadBalancer` | AWS Load Balancer Controller (NLB) or in-tree |
| HTTP host/path/TLS | Ingress | AWS Load Balancer Controller (ALB) |
| Gateway API | Gateway/HTTPRoute | AWS Load Balancer Controller (newer) |
| Static IP | NLB + EIP | Controller annotations |

The AWS Load Balancer Controller needs:

- IAM permissions via IRSA/Pod Identity (the controller's service account).
- Subnet tags: `kubernetes.io/role/elb=1` for public, `kubernetes.io/role/internal-elb=1` for private.
- Cluster security group rules allowing the LB to reach node/Pod ports.
- For ALB, the target type (`instance` or `ip`); `ip` targets Pod IPs directly.

```bash
kubectl -n kube-system get deploy aws-load-balancer-controller
kubectl get ingress
kubectl describe ingress web
kubectl get svc -A | grep LoadBalancer
aws elbv2 describe-load-balancers --query 'LoadBalancers[].{Name:LoadBalancerName,DNS:DNSName,State:State.Code}'
```

A `LoadBalancer` Service stuck in `<pending>` usually means the controller is not running, lacks IAM/subnet tags, or cannot create the LB in the VPC.

---

## 7. Observability and upgrades

- Enable control plane logs: `api`, `audit`, `authenticator`, `controllerManager`, `scheduler` → CloudWatch Logs.
- Use Container Insights/ADOT/Managed Prometheus for metrics; CloudWatch for logs; Grafana for dashboards.
- EKS upgrades: control plane first, then addons, then nodes. Kubernetes skew policy: kubelet may be a few minor versions behind the API server, not ahead.
- Node draining: `kubectl drain` respects PDBs; managed node group rolling update replaces nodes. Blue/green node groups reduce risk.
- Deprecated APIs: check `kubectl api-resources` and `kubectl deprecations` (plugins) before upgrading.

```bash
aws eks update-cluster-version --name opslab --kubernetes-version 1.31
aws eks describe-update --name opslab --update-id <id>
kubectl get nodes -o wide
```

Do not upgrade the control plane and every node group at once in production. Have a rollback plan and a maintenance window.

---

## 8. Failure scenarios

### A — Nodes NotReady

```bash
kubectl get nodes
kubectl describe node <node>
aws eks describe-nodegroup --cluster-name opslab --nodegroup-name workers
```

Check:

- Node group status/health issues (`AsgInstanceLaunchFailures`, `InstanceLimitExceeded`, `InsufficientCapacity`).
- Node IAM role/instance profile.
- Subnet routing/NAT/endpoints (nodes need ECR, EKS, STS, CloudWatch).
- CNI/kubelet logs via SSM or `kubectl debug node`.
- Disk pressure, certificate expiry, clock skew.

### B — Pods Pending with IP assignment errors

Fault domain: VPC CNI/subnet. Check `AvailableIpAddressCount`, instance ENI limits, `aws-node` logs, prefix delegation. Add subnets/IPs; do not just restart the CNI.

### C — IRSA `AccessDenied`

Check:

1. The Pod's ServiceAccount (`kubectl get pod -o yaml | grep serviceAccountName`).
2. The annotation `eks.amazonaws.com/role-arn`.
3. The OIDC provider exists in IAM and matches the cluster.
4. Trust policy `sub` and `aud` exactly match the namespace/ServiceAccount.
5. `aws sts get-caller-identity` inside the Pod shows the role, not the node role.

### D — LoadBalancer pending

Check the controller logs, IAM role, subnet tags, public/private scheme, and service annotations. A private LB in public subnets (or vice versa) can fail.

### E — EBS PVC Pending / Multi-Attach

Check the EBS CSI addon, its IAM role, StorageClass provisioner/binding mode, AZ capacity, and whether the PVC is RWO shared by multiple nodes. `kubectl describe pvc` and the CSI controller logs are the evidence.

### F — Locked out of the cluster

The IAM principal is valid but has no Kubernetes authorization. Use the cluster creator/admin access entry, or an access entry with `AmazonEKSClusterAdminPolicy`. Do not delete the `aws-auth` ConfigMap; recover via the AWS API/console.

### G — CoreDNS pending/CrashLoop

Check node capacity, the CoreDNS addon version, tolerations (it may need to run on control plane/Fargate), and logs. If CoreDNS is down, cluster DNS fails; do not scale it to zero.

### H — CNI or kube-proxy addon degraded

Check the addon version compatibility with the control plane, IAM permissions, and pod logs. An addon update can break networking cluster-wide; stage it.

---

## 9. Command field manual

### `aws eks update-kubeconfig`

```bash
aws eks update-kubeconfig --name opslab --region us-east-1 --alias opslab
kubectl config current-context
```

**Why:** writes the cluster endpoint/CA and an `exec` credential plugin. `--alias` avoids context-name collisions.

**Mistake:** using the wrong region/account; the kubeconfig points at a cluster you did not intend.

### `aws eks describe-cluster`

```bash
aws eks describe-cluster --name opslab \
  --query 'cluster.{Status:status,Version:version,Endpoint:endpoint,Auth:accessConfig}'
```

**Interpretation:** `status` `ACTIVE`/`UPDATING`/`FAILED`; `accessConfig` shows the authentication mode (ConfigMap, API, or both).

### `aws eks describe-nodegroup`

```bash
aws eks describe-nodegroup --cluster-name opslab --nodegroup-name workers \
  --query 'nodegroup.{Status:status,Health:health,Scaling:scalingConfig}'
```

**Interpretation:** `health.issues` names capacity/IAM/subnet problems. A node group can be `DEGRADED` while some nodes are Ready.

### `kubectl -n kube-system logs ds/aws-node`

**Why:** the CNI's IP allocation/ENI errors are here, not in the Pod's events. Look for `failed to assign an IP`, EC2 API throttling, or IAM permission errors.

### `aws sts get-caller-identity` inside a Pod

```bash
kubectl -n app exec s3-reader -- aws sts get-caller-identity
```

**Interpretation:** if it returns the node role, IRSA/Pod Identity is not wired; if it returns `AccessDenied`, the trust/permissions policy is wrong.

### `aws elbv2 describe-load-balancers`

Already covered; in EKS, correlate the LB name/tags with the Service/Ingress and controller logs.

---

## 10. Checkpoint

1. What does EKS manage, and what do you manage?
2. What is the difference between the public and private cluster endpoint?
3. How does `kubectl` authenticate to EKS?
4. What is the difference between `aws-auth` and access entries?
5. What is the node IAM role used for?
6. Why should Pods not use the node role?
7. What does the VPC CNI do?
8. Why does a Pod consume a VPC IP?
9. What happens when a subnet runs out of IPs?
10. What is prefix delegation?
11. Does the VPC CNI enforce NetworkPolicy?
12. What is IRSA?
13. What does the IRSA trust policy check?
14. What is EKS Pod Identity?
15. What IAM policy does the EBS CSI driver need?
16. Why does an EBS PVC need `WaitForFirstConsumer`?
17. What does the AWS Load Balancer Controller do?
18. Why would a `LoadBalancer` Service stay pending?
19. What is the first check for a node NotReady?
20. What is the first check for an IRSA AccessDenied?

### Answers

1. EKS manages the control plane (API/etcd/scheduler/controller-manager); you manage nodes, VPC/CNI, IAM/RBAC, storage, addons, and workloads.
2. Public is reachable from the internet (optionally CIDR-restricted); private is reachable only inside the VPC/connected networks.
3. The kubeconfig uses an `exec` plugin (`aws eks get-token`), which uses the IAM principal's credentials to get a token; EKS authenticates the IAM identity, then Kubernetes RBAC/access entries authorize.
4. `aws-auth` is a ConfigMap mapping IAM principals to Kubernetes users/groups; access entries are AWS API-managed mappings with access policies. Prefer access entries.
5. It lets the node/agent call EC2/ECR/EKS APIs (CNI, image pulls, bootstrap) and is the instance profile for the node.
6. Every Pod on the node could inherit the node's broader permissions via IMDS; it violates least privilege and blast-radius isolation.
7. It assigns VPC IPs to Pods, manages ENIs/IPs, and wires Pod networking into the VPC.
8. Pods are first-class VPC addresses; the CNI allocates from the node's subnet.
9. New Pods cannot get an IP and stay Pending/ContainerCreating; the CNI logs allocation failures.
10. Assigning `/28` prefixes to ENIs instead of individual IPs, increasing Pod density and reducing churn.
11. No; it provides networking/IPAM, not policy enforcement. Use Calico/Cilium or another policy engine.
12. IAM Roles for Service Accounts: OIDC federation of a Kubernetes ServiceAccount to an IAM role for short-lived AWS credentials.
13. The OIDC provider, `sub` (`system:serviceaccount:namespace:name`), and `aud` (`sts.amazonaws.com`).
14. An EKS-native way to associate an IAM role with a ServiceAccount via the Pod Identity agent, without OIDC annotations.
15. `AmazonEBSCSIDriverPolicy` (or a least-privilege equivalent) attached to the CSI service account role.
16. To provision the EBS volume in the same AZ as the scheduled Pod; immediate binding can create a volume in the wrong AZ.
17. It watches Ingress/Service objects and creates/configures ALB/NLB, target groups, listeners, and security groups.
18. The controller is not running, lacks IAM permissions, subnet tags are missing, or the scheme/subnets are wrong.
19. `kubectl describe node`, node group health, kubelet/CNI logs, IAM/instance profile, subnet routing/NAT/endpoints, disk.
20. Check the ServiceAccount annotation, OIDC provider, trust policy `sub`/`aud`, and `sts get-caller-identity` inside the Pod.
