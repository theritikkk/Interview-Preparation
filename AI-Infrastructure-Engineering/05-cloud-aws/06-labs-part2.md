# Phase 5 Labs — Part 2 (L5.3–L5.4)

> **Cost warning:** L5.3 creates an ALB and EC2 instances; L5.4 creates an EKS control plane, nodes, NAT gateways, a load balancer, and EBS volumes. These are among the most expensive labs in the repository. Set a budget alert, use the smallest sizes, and delete everything in the same session. `eksctl delete cluster` is not optional.

---

# Lab L5.3 — EC2, EBS, ALB, and an Auto Scaling Group

## Objective

Launch a two-instance web fleet with a launch template and ASG, put it behind an ALB with a health check, prove target health and 503 behavior, attach and resize an EBS volume, and clean up the fleet.

## Prerequisites

- AWS CLI v2, administrative sandbox role.
- Default VPC with at least two public subnets in different AZs, or L5.1's public subnets.
- SSM permissions and an instance profile (L5.1 creates one; this lab creates its own).
- 45–60 minutes.

## Setup

```bash
export AWS_REGION=us-east-1
export AWS_DEFAULT_REGION=$AWS_REGION
export PROJECT=opslab
aws sts get-caller-identity
```

Find the default VPC and two public subnets:

```bash
VPC=$(aws ec2 describe-vpcs --filters Name=isDefault,Values=true \
  --query 'Vpcs[0].VpcId' --output text)
aws ec2 describe-subnets --filters Name=vpc-id,Values="$VPC" \
  --query 'Subnets[?MapPublicIpOnLaunch==`true`].[SubnetId,AvailabilityZone]' --output text
```

Choose two subnets in different AZs:

```bash
export SUBNET1=subnet-aaaa
export SUBNET2=subnet-bbbb
export AZ1=us-east-1a
export AZ2=us-east-1b
```

Create the ALB security group:

```bash
SG=$(aws ec2 create-security-group --group-name "$PROJECT-alb-sg" \
  --description "opslab ALB HTTP" --vpc-id "$VPC" --query 'GroupId' --output text)
aws ec2 authorize-security-group-ingress --group-id "$SG" \
  --protocol tcp --port 80 --cidr 0.0.0.0/0
```

Create an instance role/profile for SSM:

```bash
cat > /tmp/ec2-trust.json <<'EOF'
{"Version":"2012-10-17","Statement":[{"Effect":"Allow","Principal":{"Service":"ec2.amazonaws.com"},"Action":"sts:AssumeRole"}]}
EOF
aws iam create-role --role-name "$PROJECT-ec2-role" \
  --assume-role-policy-document file:///tmp/ec2-trust.json
aws iam attach-role-policy --role-name "$PROJECT-ec2-role" \
  --policy-arn arn:aws:iam::aws:policy/AmazonSSMManagedInstanceCore
aws iam create-instance-profile --instance-profile-name "$PROJECT-ec2-profile"
aws iam add-role-to-instance-profile --instance-profile-name "$PROJECT-ec2-profile" \
  --role-name "$PROJECT-ec2-role"
sleep 10
```

User data:

```bash
cat > /tmp/web-userdata.sh <<'EOF'
#!/bin/bash
dnf install -y nginx
systemctl enable --now nginx
echo "ok" > /usr/share/nginx/html/health.html
echo "hello from $(hostname)" > /usr/share/nginx/html/index.html
EOF
UD=$(base64 < /tmp/web-userdata.sh | tr -d '\n')
AMI=$(aws ssm get-parameters \
  --names /aws/service/ami-amazon-linux-latest/al2023-ami-kernel-default-x86_64 \
  --query 'Parameters[0].Value' --output text)
cat > /tmp/lt.json <<EOF
{
  "ImageId": "$AMI",
  "InstanceType": "t3.micro",
  "SecurityGroupIds": ["$SG"],
  "IamInstanceProfile": {"Name": "$PROJECT-ec2-profile"},
  "MetadataOptions": {"HttpTokens": "required", "HttpPutResponseHopLimit": 1},
  "UserData": "$UD"
}
EOF
```

Launch template and target group:

```bash
LT=$(aws ec2 create-launch-template --launch-template-name "$PROJECT-lt" \
  --version-description v1 --launch-template-data file:///tmp/lt.json \
  --query 'LaunchTemplate.LaunchTemplateId' --output text)
TG=$(aws elbv2 create-target-group --name "$PROJECT-tg" \
  --protocol HTTP --port 80 --vpc-id "$VPC" \
  --health-check-path /health.html --health-check-interval-seconds 10 \
  --health-check-timeout-seconds 5 --healthy-threshold-count 2 \
  --unhealthy-threshold-count 2 --matcher HttpCode=200 \
  --query 'TargetGroups[0].TargetGroupArn' --output text)
```

ASG:

```bash
aws autoscaling create-auto-scaling-group \
  --auto-scaling-group-name "$PROJECT-asg" \
  --launch-template "LaunchTemplateId=$LT,Version=\$Latest" \
  --min-size 2 --max-size 2 --desired-capacity 2 \
  --vpc-zone-identifier "$SUBNET1,$SUBNET2" \
  --target-group-arns "$TG" \
  --health-check-type ELB --health-check-grace-period 120
```

ALB and listener:

```bash
ALB=$(aws elbv2 create-load-balancer --name "$PROJECT-alb" \
  --subnets "$SUBNET1" "$SUBNET2" --security-groups "$SG" \
  --scheme internet-facing --type application \
  --query 'LoadBalancers[0].LoadBalancerArn' --output text)
aws elbv2 create-listener --load-balancer-arn "$ALB" \
  --protocol HTTP --port 80 \
  --default-actions Type=forward,TargetGroupArn="$TG"
```

Wait for targets:

```bash
for i in $(seq 1 30); do
  aws elbv2 describe-target-health --target-group-arn "$TG" \
    --query 'TargetHealthDescriptions[].TargetHealth.State' --output text
  sleep 10
done
```

Expected: `healthy healthy`. Get the ALB DNS:

```bash
DNS=$(aws elbv2 describe-load-balancers --load-balancer-arns "$ALB" \
  --query 'LoadBalancers[0].DNSName' --output text)
echo "$DNS"
curl -sS "http://$DNS/"
curl -sS "http://$DNS/health.html"
```

## Part A — Target health and 503

Get an instance ID:

```bash
INST=$(aws autoscaling describe-auto-scaling-groups \
  --auto-scaling-group-names "$PROJECT-asg" \
  --query 'AutoScalingGroups[0].Instances[0].InstanceId' --output text)
aws ssm start-session --target "$INST"
```

Inside, stop nginx:

```bash
sudo systemctl stop nginx
```

Wait 20–30 seconds and check:

```bash
aws elbv2 describe-target-health --target-group-arn "$TG" \
  --query 'TargetHealthDescriptions[].{Target:Target.Id,State:TargetHealth.State,Reason:TargetHealth.Reason}'
```

Expected: one target `unhealthy` with `Target.FailedHealthChecks` or `Target.Timeout`; the ALB still serves from the other target. Stop nginx on both instances (or set the health check path to `/missing`) to produce a 503:

```bash
curl -i "http://$DNS/"
```

Expected: `HTTP/1.1 503 Service Unavailable` if all targets are unhealthy. Restart nginx on both:

```bash
sudo systemctl start nginx
```

Check target health recovers; the ASG does not replace the instance because the instance status check is still passing (unless you set the ASG health check to ELB, which it is; the ELB health check may mark the instance unhealthy and the ASG may replace it — observe and record which happened).

## Part B — 502 behavior

On one instance, change nginx to listen on a different port (or stop it abruptly) while the target group still points at 80. A 502 usually means the ALB reached the target but the target closed/reset. If both targets are stopped, expect 503 (no healthy targets), not 502. To force a 502, make the app return an invalid response or close the connection; a simple lab proxy is more work than it is worth. Record the difference: 502 = bad target response; 503 = no healthy targets.

## Part C — EBS attach, snapshot, and resize

Pick an instance and its AZ:

```bash
INST=$(aws autoscaling describe-auto-scaling-groups \
  --auto-scaling-group-names "$PROJECT-asg" \
  --query 'AutoScalingGroups[0].Instances[0].InstanceId' --output text)
AZ=$(aws ec2 describe-instances --instance-ids "$INST" \
  --query 'Reservations[0].Instances[0].Placement.AvailabilityZone' --output text)
```

Create and attach a 20 GiB gp3 volume:

```bash
VOL=$(aws ec2 create-volume --availability-zone "$AZ" --size 20 --volume-type gp3 \
  --tag-specifications "ResourceType=volume,Tags=[{Key=Name,Value=$PROJECT-data}]" \
  --query 'VolumeId' --output text)
aws ec2 wait volume-available --volume-ids "$VOL"
aws ec2 attach-volume --volume-id "$VOL" --instance-id "$INST" --device /dev/sdf
aws ec2 wait volume-in-use --volume-ids "$VOL"
```

In an SSM session:

```bash
lsblk -o NAME,SIZE,TYPE,MOUNTPOINT
# Identify the new disk (usually nvme1n1); confirm size 20G
sudo mkfs.ext4 /dev/nvme1n1
sudo mkdir -p /mnt/lab
sudo mount /dev/nvme1n1 /mnt/lab
echo "persistent?" | sudo tee /mnt/lab/file.txt
df -h /mnt/lab
```

Snapshot:

```bash
SNAP=$(aws ec2 create-snapshot --volume-id "$VOL" \
  --description "opslab data snapshot" --query 'SnapshotId' --output text)
aws ec2 wait snapshot-completed --snapshot-ids "$SNAP"
```

Resize the volume to 30 GiB, then grow the filesystem:

```bash
aws ec2 modify-volume --volume-id "$VOL" --size 30
aws ec2 wait volume-modification-completed --volume-ids "$VOL"
```

Inside the instance:

```bash
lsblk /dev/nvme1n1
sudo resize2fs /dev/nvme1n1
df -h /mnt/lab
```

Expected: the block device is 30G and the filesystem grows to 30G. `modify-volume` alone would leave the filesystem at 20G.

## Expected observations

| Check | Expected |
|---|---|
| ASG | 2 instances, one per AZ, `InService` |
| Target health | `healthy healthy` |
| ALB DNS | returns the app and `/health.html` |
| One nginx stopped | one `unhealthy`, ALB still serves |
| Both unhealthy | 503 |
| EBS attach | new `nvme1n1` in `lsblk` |
| Snapshot | `completed` |
| Modify volume | device 30G, filesystem still 20G until resize |
| `resize2fs` | `df` shows 30G |

## Internal explanation

- The ASG maintains instances; the target group decides health; the ALB only forwards to healthy targets.
- A 503 is an LB-level "no healthy targets" response, not necessarily an app 500.
- EBS is a block device; the filesystem is a separate layer. `modify-volume` changes the block device, `resize2fs`/`xfs_growfs` grows the filesystem.
- The snapshot is crash-consistent; for a database, quiesce or use a native backup.

## Troubleshooting task

The ALB returns 503 immediately after creation. `describe-target-health` shows `unused` for both targets, and the ASG has 2 `InService` instances. What is the most likely cause, and what two commands prove it? Write the answer before the debrief.

## Verification

- [ ] ASG has two healthy instances across two AZs.
- [ ] ALB serves the app and health page.
- [ ] One unhealthy target does not take down the service; all unhealthy produces 503.
- [ ] EBS volume is attached, formatted, mounted, snapshotted.
- [ ] Volume resize + filesystem grow are both verified.
- [ ] You can explain 502 vs 503 vs 504.

## Cleanup

```bash
# Delete the ALB first (wait for deletion)
aws elbv2 delete-load-balancer --load-balancer-arn "$ALB"
aws elbv2 wait load-balancers-deleted --load-balancer-arns "$ALB"
aws elbv2 delete-target-group --target-group-arn "$TG"

# Delete the ASG and wait for instances to terminate
aws autoscaling delete-auto-scaling-group --auto-scaling-group-name "$PROJECT-asg" --force-delete
sleep 60
aws autoscaling describe-auto-scaling-groups --auto-scaling-group-names "$PROJECT-asg" 2>/dev/null || true

# Launch template, SG, volume, snapshot
aws ec2 delete-launch-template --launch-template-id "$LT" 2>/dev/null || true
aws ec2 detach-volume --volume-id "$VOL" 2>/dev/null || true
aws ec2 delete-volume --volume-id "$VOL" 2>/dev/null || true
aws ec2 delete-snapshot --snapshot-id "$SNAP" 2>/dev/null || true
aws ec2 delete-security-group --group-id "$SG" 2>/dev/null || true

# IAM
aws iam remove-role-from-instance-profile --instance-profile-name "$PROJECT-ec2-profile" \
  --role-name "$PROJECT-ec2-role" 2>/dev/null || true
aws iam delete-instance-profile --instance-profile-name "$PROJECT-ec2-profile" 2>/dev/null || true
aws iam detach-role-policy --role-name "$PROJECT-ec2-role" \
  --policy-arn arn:aws:iam::aws:policy/AmazonSSMManagedInstanceCore 2>/dev/null || true
aws iam delete-role --role-name "$PROJECT-ec2-role" 2>/dev/null || true
```

Verify:

```bash
aws elbv2 describe-load-balancers --query 'LoadBalancers[].{Name:LoadBalancerName,State:State.Code}'
aws autoscaling describe-auto-scaling-groups --query 'AutoScalingGroups[].AutoScalingGroupName'
aws ec2 describe-volumes --filters Name=status,Values=available \
  --query 'Volumes[].{ID:VolumeId,Size:Size,Name:Tags[?Key==`Name`].Value|[0]}'
aws ec2 describe-instances --filters Name=instance-state-name,Values=running \
  --query 'Reservations[].Instances[].InstanceId'
```

No lab ALB, ASG, unattached volume, or running instance should remain. Snapshots and AMIs are easy to forget.

## Write-up

```markdown
# L5.3 — EC2/EBS/ALB
## Architecture (AZs, subnets, SG, target group)
## Target health evidence and 503 experiment
## EBS attach/snapshot/resize evidence
## 502 vs 503 vs 504
## Cleanup verification
## What I would monitor in production
```

## Debrief

`unused` means the targets are registered in the target group but the ASG is not attached, or the target group has no registered targets. Check `aws autoscaling describe-auto-scaling-groups` for `TargetGroupARNs` and `aws elbv2 describe-target-health`. If the ASG is attached but `unused`, the instances may not be in the target group's VPC/subnets or the attachment failed. The fix is to attach the ASG to the target group, not to restart the ALB.

---

# Lab L5.4 — EKS Cluster, IRSA, EBS CSI, and a LoadBalancer

## Objective

Create a small EKS cluster, deploy a workload, expose it through an AWS load balancer, provision an EBS PVC through the CSI driver, and grant a Pod S3 access with IRSA. Then destroy the cluster and verify the AWS resources are gone.

## Prerequisites

- AWS CLI, `eksctl`, `kubectl`.
- Administrative sandbox role with EKS/IAM/EC2/ELB/S3 permissions.
- A budget alert. Expect several dollars for a short session; control plane, NAT, nodes, LB, and EBS all bill.
- 60–90 minutes. If the budget is tight, do the architecture/read-only version at the end instead of creating the cluster.

## Setup

```bash
export AWS_REGION=us-east-1
export AWS_DEFAULT_REGION=$AWS_REGION
export CLUSTER=opslab
export ACCOUNT=$(aws sts get-caller-identity --query Account --output text)
aws sts get-caller-identity
eksctl version
kubectl version --client
```

Create the cluster (this takes 15–25 minutes):

```bash
eksctl create cluster \
  --name "$CLUSTER" \
  --region "$AWS_REGION" \
  --version 1.30 \
  --nodegroup-name workers \
  --node-type t3.small \
  --nodes 2 --nodes-min 1 --nodes-max 2 \
  --managed --with-oidc
```

```bash
eksctl get cluster --name "$CLUSTER" --region "$AWS_REGION"
kubectl get nodes -o wide
kubectl -n kube-system get pods
```

Expected: two `Ready` nodes, CoreDNS/kube-proxy/aws-node running. `eksctl` creates a VPC with public/private subnets and NAT gateways; note them for cleanup.

## Part A — Deploy and expose

```bash
kubectl create namespace app
kubectl -n app create deployment web --image=nginx:1.27 --replicas=2
kubectl -n app expose deployment web --type=LoadBalancer --port=80 --target-port=80
kubectl -n app get svc web -w
```

Wait for `EXTERNAL-IP` to become a hostname (not `<pending>`), then:

```bash
DNS=$(kubectl -n app get svc web -o jsonpath='{.status.loadBalancer.ingress[0].hostname}')
echo "$DNS"
curl -sS --max-time 10 "http://$DNS/" | head -3
```

This uses the in-tree AWS load balancer path (Classic LB) in many EKS versions; newer clusters may use the AWS Load Balancer Controller for NLB/ALB. Either way, the Service should get an external hostname. If it stays pending, check the controller/IAM/subnet tags.

## Part B — EBS CSI and a PVC

Create the CSI service account role and addon:

```bash
eksctl create iamserviceaccount \
  --cluster "$CLUSTER" --region "$AWS_REGION" \
  --namespace kube-system --name ebs-csi-controller-sa \
  --role-name "AmazonEKS_EBS_CSI_DriverRole_$CLUSTER" \
  --attach-policy-arn arn:aws:iam::aws:policy/service-role/AmazonEBSCSIDriverPolicy \
  --approve --role-only

eksctl create addon --cluster "$CLUSTER" --region "$AWS_REGION" \
  --name aws-ebs-csi-driver \
  --service-account-role-arn "arn:aws:iam::$ACCOUNT:role/AmazonEKS_EBS_CSI_DriverRole_$CLUSTER" \
  --force
```

```bash
kubectl -n kube-system get pods -l app.kubernetes.io/name=aws-ebs-csi-driver
kubectl get csidrivers
```

Create a StorageClass and PVC:

```bash
kubectl apply -f - <<'EOF'
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: ebs-sc
provisioner: ebs.csi.aws.com
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
parameters:
  type: gp3
---
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: data
  namespace: app
spec:
  accessModes: ["ReadWriteOnce"]
  storageClassName: ebs-sc
  resources:
    requests:
      storage: 1Gi
EOF
kubectl -n app get pvc data -w
```

The PVC stays `Pending` until a Pod schedules (WaitForFirstConsumer). Deploy a writer:

```bash
kubectl -n app apply -f - <<'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: writer
  namespace: app
spec:
  containers:
    - name: app
      image: busybox:1.36
      command: ["sh", "-c", "echo persisted > /data/file.txt; sleep 3600"]
      volumeMounts:
        - name: data
          mountPath: /data
  volumes:
    - name: data
      persistentVolumeClaim:
        claimName: data
EOF
kubectl -n app get pvc,pod
kubectl -n app exec writer -- cat /data/file.txt
```

Delete and recreate the Pod; the PVC persists. Check the EBS volume in AWS:

```bash
aws ec2 describe-volumes --filters Name=tag:kubernetes.io/created-for/pvc/name,Values=data \
  --query 'Volumes[].{ID:VolumeId,Size:Size,AZ:AvailabilityZone,State:State}'
```

## Part C — IRSA: Pod access to S3

Create a bucket and a least-privilege policy:

```bash
BUCKET="opslab-eks-$ACCOUNT-$RANDOM"
aws s3api create-bucket --bucket "$BUCKET" --region "$AWS_REGION"
aws s3api put-public-access-block --bucket "$BUCKET" \
  --public-access-block-configuration \
  BlockPublicAcls=true,IgnorePublicAcls=true,BlockPublicPolicy=true,RestrictPublicBuckets=true
echo "hello from eks" > /tmp/eks.txt
aws s3 cp /tmp/eks.txt "s3://$BUCKET/eks.txt"

cat > /tmp/eks-s3-policy.json <<EOF
{
  "Version": "2012-10-17",
  "Statement": [
    {"Effect": "Allow", "Action": ["s3:ListBucket"], "Resource": "arn:aws:s3:::$BUCKET"},
    {"Effect": "Allow", "Action": ["s3:GetObject"], "Resource": "arn:aws:s3:::$BUCKET/*"}
  ]
}
EOF
POLICY_ARN=$(aws iam create-policy --policy-name "OpsLabEKS-S3Read-$RANDOM" \
  --policy-document file:///tmp/eks-s3-policy.json --query 'Policy.Arn' --output text)
echo "$POLICY_ARN"
```

Create the IRSA ServiceAccount:

```bash
eksctl create iamserviceaccount \
  --cluster "$CLUSTER" --region "$AWS_REGION" \
  --namespace app --name s3-reader \
  --attach-policy-arn "$POLICY_ARN" --approve
```

Deploy a Pod that uses it:

```bash
kubectl -n app apply -f - <<EOF
apiVersion: v1
kind: Pod
metadata:
  name: s3-reader
  namespace: app
spec:
  serviceAccountName: s3-reader
  restartPolicy: Never
  containers:
    - name: aws
      image: amazon/aws-cli:2.17.0
      command: ["aws", "s3", "ls", "s3://$BUCKET"]
EOF
kubectl -n app logs s3-reader
```

Expected: the bucket contents (`eks.txt`), not `AccessDenied`. Prove the identity inside the Pod:

```bash
kubectl -n app exec s3-reader -- aws sts get-caller-identity
```

The ARN should be the IRSA role, not the node role.

## Expected observations

| Check | Expected |
|---|---|
| `kubectl get nodes` | 2 Ready |
| `kubectl get svc web` | external hostname, curl works |
| EBS CSI pods | controller + node pods Running |
| PVC before Pod | Pending (WaitForFirstConsumer) |
| PVC after Pod | Bound |
| EBS volume | `in-use`, 1 GiB, in the Pod's AZ |
| IRSA Pod logs | bucket listing |
| `sts get-caller-identity` | assumed role for `app/s3-reader` |
| Node role | not used by the Pod |

## Internal explanation

- EKS runs the control plane; the VPC CNI gives Pods VPC IPs; the AWS Load Balancer Controller/in-tree cloud provider creates the LB; the EBS CSI driver creates/attaches volumes.
- IRSA uses the cluster OIDC provider and a projected token; the trust policy restricts `sub`/`aud`. The Pod does not need static keys.
- EBS is AZ-scoped and RWO; a PVC cannot be shared across nodes.
- `kubectl expose --type=LoadBalancer` creates an AWS LB; it is not a Kubernetes-only object.

## Troubleshooting task

1. The `web` Service stays `<pending>` for 10 minutes. List the first four checks.
2. The `s3-reader` Pod logs `AccessDenied` for `s3:ListBucket`. The ServiceAccount annotation exists. Check the trust policy `sub`, the OIDC provider, and whether the Pod actually uses the ServiceAccount.
3. A second replica of a Deployment mounts the same RWO PVC and stays `ContainerCreating` with `Multi-Attach error`. Explain why and the correct fix.

Write your answers before the debrief.

## Verification

- [ ] EKS cluster is `ACTIVE`, nodes Ready.
- [ ] Service has an external hostname and serves nginx.
- [ ] EBS CSI addon is running; PVC binds and persists.
- [ ] IRSA Pod lists the bucket and `sts` shows the role.
- [ ] You can explain VPC CNI, IRSA, EBS CSI, and LB controller responsibilities.
- [ ] You know the cost drivers (control plane, NAT, nodes, LB, EBS).

## Cleanup

Delete Kubernetes objects that create AWS resources first:

```bash
kubectl -n app delete svc web --wait=true
kubectl -n app delete pvc data --wait=true
kubectl delete namespace app --wait=true
```

Then delete the cluster (this removes the VPC/node groups/NAT by default):

```bash
eksctl delete cluster --name "$CLUSTER" --region "$AWS_REGION" --wait
```

Verify:

```bash
aws eks list-clusters
aws ec2 describe-instances --filters Name=instance-state-name,Values=running \
  --query 'Reservations[].Instances[].{ID:InstanceId,Name:Tags[?Key==`Name`].Value|[0]}'
aws ec2 describe-volumes --filters Name=status,Values=available \
  --query 'Volumes[].{ID:VolumeId,Size:Size}'
aws elbv2 describe-load-balancers --query 'LoadBalancers[].{Name:LoadBalancerName,State:State.Code}'
aws ec2 describe-nat-gateways --filter Name=state,Values=available \
  --query 'NatGateways[].{ID:NatGatewayId,VPC:VpcId}'
```

If `eksctl delete cluster` fails, inspect the CloudFormation stacks it created (`eksctl-opslab-cluster`, `eksctl-opslab-nodegroup-workers`) and delete them in dependency order. Do not delete the VPC while a load balancer or EBS volume still depends on it.

Also delete the S3 bucket/policy if they remain:

```bash
aws s3 rm "s3://$BUCKET" --recursive
aws s3api delete-bucket --bucket "$BUCKET"
aws iam delete-policy --policy-arn "$POLICY_ARN"
```

## Write-up

```markdown
# L5.4 — EKS
## Cluster/VPC/node topology
## Service → LB path
## PVC → EBS path
## IRSA trust policy and Pod identity evidence
## Three failure modes and their first checks
## Cost estimate and cleanup verification
```

## Debrief

1. **LB pending:** check the AWS Load Balancer Controller (or in-tree provider) pods/logs, IAM permissions via IRSA, subnet tags, public/private scheme, and service annotations. A missing controller is more common than an AWS outage.
2. **IRSA AccessDenied:** the trust policy `sub` must exactly match `system:serviceaccount:app:s3-reader`; the OIDC provider must belong to this cluster; the Pod must use the ServiceAccount; the role must allow the action on the bucket. Check `sts get-caller-identity` to see which identity the SDK actually uses.
3. **Multi-Attach:** EBS RWO is per node, not per Pod. Two replicas on different nodes cannot attach the same volume. Use a StatefulSet with `volumeClaimTemplates` (one PVC per replica), or a RWX backend (EFS), or a single replica.

### Budget alternative (no cluster)

If you cannot create the cluster, do this read-only exercise on an existing EKS cluster (yours or a shared sandbox):

```bash
aws eks list-clusters
aws eks describe-cluster --name <name> --query 'cluster.{Version:version,Endpoint:endpoint,Auth:accessConfig}'
aws eks list-nodegroups --cluster-name <name>
aws eks list-addons --cluster-name <name>
kubectl get nodes -o wide
kubectl -n kube-system get ds,deploy
kubectl get csidrivers,storageclass
```

Draw the control plane → VPC → nodes → Pods → LB/EBS path, and write the trust policy you would use for a `s3-reader` ServiceAccount. That is the interview-level deliverable; the hands-on cluster is a bonus.
