# EC2, EBS, and Load Balancing

> **The one-sentence model:** an EC2 instance is a VM with a launch template, an IAM role, and one or more EBS volumes in a single AZ; an ALB is an L7 reverse proxy that only sends traffic to targets it considers healthy; an ASG replaces instances but does not fix a bad health check.

---

## 1. EC2 essentials

| Concept | What it means | Operational consequence |
|---|---|---|
| **Instance type** | Family + size (`m6i.large`) | Determines vCPU, memory, network, EBS baseline |
| **vCPU** | A hyperthread, not a physical core | 4 vCPU may be 2 cores with SMT |
| **AMI** | Boot image (OS + preinstalled software) | Region-specific; AMI IDs differ across regions |
| **Launch template** | Versioned instance configuration | Use for ASGs; never edit a running instance's template and expect it to apply |
| **User data** | Boot-time script | Runs as root; failures are visible in cloud-init logs/console |
| **Instance profile** | IAM role credentials via IMDS | Prefer roles over access keys on instances |
| **Key pair** | SSH public key injected at launch | Losing it does not lose the instance; use SSM/Instance Connect |
| **Placement group** | Cluster/spread/partition placement | Affects latency and failure domains |

### Families

| Family | Optimized for | Examples |
|---|---|---|
| `t` | Burstable general purpose | `t3`, `t4g` |
| `m` | Balanced general purpose | `m6i`, `m7g` |
| `c` | Compute | `c6i`, `c7g` |
| `r`/`x` | Memory | `r6i`, `x2idn` |
| `i`/`d` | Local NVMe/HDD storage | `i4i`, `d3` |
| `p`/`g` | GPU | `p4d`, `g5` |
| `hpc` | HPC | `hpc6a` |

Burstable instances earn CPU credits; if credits run out, performance is throttled unless unlimited mode is enabled (which can cost more). Check `CPUCreditBalance` before blaming the app.

### Status checks

- **System status check:** AWS hypervisor/network path to the instance.
- **Instance status check:** the guest OS is responsive.

```bash
aws ec2 describe-instance-status --instance-ids i-123 \
  --query 'InstanceStatuses[0].{System:SystemStatus.Status,Instance:InstanceStatus.Status}'
```

An instance can be `running` with both checks failing. `running` is a billing/VM state, not a health signal.

### Metadata and IMDSv2

```bash
TOKEN=$(curl -sX PUT http://169.254.169.254/latest/api/token \
  -H 'X-aws-ec2-metadata-token-ttl-seconds: 60')
curl -s -H "X-aws-ec2-metadata-token: $TOKEN" \
  http://169.254.169.254/latest/meta-data/instance-id
```

If the app gets `401`, it is using IMDSv1 against a v2-required instance. Fix the SDK/agent, do not disable IMDSv2.

---

## 2. EBS

EBS is network-attached block storage, **AZ-scoped**. A volume can only attach to an instance in the same AZ.

| Type | Use | Notes |
|---|---|---|
| `gp3` | General purpose SSD | Baseline 3,000 IOPS/125 MB/s; provision IOPS/throughput independently |
| `gp2` | Older general purpose | IOPS tied to volume size; migrate to gp3 |
| `io2`/`io1` | High IOPS, low latency | Multi-Attach supported (same AZ, cluster-aware FS) |
| `st1` | Throughput HDD | Sequential reads, big data |
| `sc1` | Cold HDD | Lowest cost, infrequent access |
| Instance store | Ephemeral NVMe/HDD | Data lost on stop/terminate; no snapshot |

### Snapshots

- Incremental, stored in S3 (managed by AWS), region-scoped; copy to another region for DR.
- Crash-consistent by default; quiesce the application or use a filesystem freeze for app-consistent snapshots.
- Snapshot restore creates a new volume in the AZ you choose (or a new AZ).
- Snapshots cost per GB-month; old snapshots are a common hidden cost.

### Resize a volume

```bash
# 1. Increase the EBS volume size/IOPS
aws ec2 modify-volume --volume-id vol-123 --size 200 --volume-type gp3

# 2. On the instance, grow the partition and filesystem
lsblk
sudo growpart /dev/nvme0n1 1
sudo resize2fs /dev/nvme0n1p1     # ext4
# or: sudo xfs_growfs /data        # XFS
df -h
```

`modify-volume` can take time and may require a filesystem resize; it does not automatically grow the partition or filesystem. For XFS, grow the mounted filesystem. For ext4, grow online.

### Multi-Attach

Only `io1`/`io2` in the same AZ, and only with a cluster-aware filesystem (e.g., GFS2, OCFS2) or an application that coordinates writes. Ext4/XFS on a multi-attached volume will corrupt data. Do not use Multi-Attach as a substitute for NFS/EFS.

---

## 3. Load balancers

| | ALB | NLB | CLB |
|---|---|---|---|
| Layer | L7 (HTTP/HTTPS/gRPC) | L4 (TCP/UDP/TLS) | Legacy L4/L7 |
| Routing | Host, path, header, method, query | Flow hash, port | Basic |
| TLS termination | Yes, SNI, ACM | Yes (TLS listener), passthrough | Yes |
| Source IP | `X-Forwarded-For` | Preserved (or private IP with TLS) | Varies |
| Static IP | No (use Global Accelerator) | Yes (EIP per AZ) | No |
| WebSocket | Yes | Yes | Yes |
| Use | HTTP services, ingress | TCP/UDP, extreme performance, static IP | Legacy only |

### Target groups and health checks

```bash
aws elbv2 describe-target-health --target-group-arn <arn>
```

```json
{
  "TargetHealth": {
    "State": "unhealthy",
    "Reason": "Target.FailedHealthChecks",
    "Description": "Health checks failed with these codes: [404]"
  }
}
```

States: `healthy`, `unhealthy`, `unused` (not registered/ASG not attached), `draining`, `unavailable`. A target can be `unhealthy` because of:

- wrong health check path/port/protocol,
- SG/NACL blocking the LB,
- app not listening on the target port,
- TLS mismatch (health check HTTP to an HTTPS port),
- response code not in the matcher (default 200),
- timeout too short for a slow app.

### ALB errors

| Error | Meaning | First checks |
|---|---|---|
| 502 Bad Gateway | Target closed the connection or sent an invalid response | App crash, port/protocol mismatch, keepalive, target SG |
| 503 Service Unavailable | No healthy targets | Target health, ASG capacity, health check path |
| 504 Gateway Timeout | Target did not respond before the idle/response timeout | App latency, DB, SG, LB idle timeout |
| 500 Internal Server Error | ALB/internal error or target 500 | Target logs, ALB logs |
| 464 | Invalid HTTP/2 request to a target | Protocol mismatch/header issue |

ALB access logs (S3) and CloudWatch metrics (`HTTPCode_Target_5XX_Count`, `UnHealthyHostCount`, `TargetResponseTime`) distinguish LB from app.

### Idle timeout and keepalive

ALB idle timeout default is 60 seconds. If the target's keepalive is shorter, the ALB may reuse a connection the target closed → 502. Set the target keepalive longer than the LB idle timeout, or enable keepalive tuning. For long requests, raise the idle timeout (max 4000 s) and the app's timeout.

### Deregistration delay

When a target is removed, the LB stops sending new requests but allows in-flight requests to finish for the deregistration delay (default 300 s). If the app needs less, lower it; if it has long requests, raise it. This is part of zero-downtime rollout.

---

## 4. Auto Scaling

```text
Launch template → Auto Scaling group → instances across AZs
                         ↑
                scaling policy (target tracking/step/scheduled)
```

- `min`/`desired`/`max` define the size.
- `health_check_type`: `EC2` (status checks) or `ELB` (target group health). Use `ELB` for web fleets; otherwise the ASG may keep an instance the LB considers unhealthy.
- `health_check_grace_period`: time for boot/user data before health checks count.
- `default_cooldown` and policy cooldowns prevent flapping.
- `termination_policies` choose which instance to remove (`Default`, `OldestInstance`, `NewestInstance`, `OldestLaunchTemplate`, `ClosestToNextInstanceHour`, `AllocationStrategy`).
- `instance_refresh` rolls out a new launch template version.
- Lifecycle hooks pause `pending:wait`/`terminating:wait` for bootstrap/drain.

ASG does not fix a bad AMI, a failing health check, or an app that cannot start. It replaces instances until it hits `max` or gives up.

---

## 5. ECR and images

```bash
aws ecr create-repository --repository-name opslab/app
aws ecr get-login-password | docker login --username AWS --password-stdin <account>.dkr.ecr.<region>.amazonaws.com
docker tag app:1.0 <account>.dkr.ecr.<region>.amazonaws.com/opslab/app:1.0
docker push <account>.dkr.ecr.<region>.amazonaws.com/opslab/app:1.0
```

- Enable image scanning (basic/enhanced) and a lifecycle policy to expire old images.
- Use immutable tags or digests; `latest` is not a version.
- ECR is regional; cross-region replication is optional.
- EKS nodes need `ecr:GetAuthorizationToken`, `ecr:BatchGetImage`, `ecr:GetDownloadUrlForLayer` (often via the node role or a pull-through cache).

---

## 6. Failure scenarios

### A — Instance unreachable

```text
$ ssh ec2-user@203.0.113.10
ssh: connect to host 203.0.113.10 port 22: Connection timed out
```

Fault domain: network/instance. Check:

1. Instance state and **both status checks**.
2. Subnet route to IGW/NAT and public IP/EIP.
3. SG inbound 22 (or 443) from your IP; NACL both directions.
4. Key pair and SSH user (`ec2-user` for AL2023, `ubuntu` for Ubuntu).
5. `aws ec2 get-console-output` for boot/kernel errors.
6. SSM Session Manager if the agent/role are configured; it avoids SSH entirely.

A timeout suggests drop/route; `Connection refused` suggests the host is reachable but sshd is not listening or an SG rejected (SGs drop, so usually timeout).

### B — ALB 503

Fault domain: target health/capacity. `describe-target-health`, target group health check settings, SG from LB to target, app listening, ASG desired capacity. If all targets are `unhealthy` with `404`, the health check path is wrong. If `unused`, the ASG is not attached to the target group.

### C — ALB 502

Fault domain: target/app. The LB reached the target but the target closed/reset or returned an invalid response. Check app logs, port/protocol (HTTP health check to HTTPS listener), keepalive/idle timeout mismatch, and whether the app crashed after accepting.

### D — ALB 504

Fault domain: latency/timeout. Check `TargetResponseTime`, app/DB latency, SG, and the LB idle timeout. A 504 from the LB is not necessarily an LB bug; it is often a slow backend or a blocked connection.

### E — EBS full but the volume is bigger

Fault domain: partition/filesystem. `lsblk`, `df -h`, `growpart`, `resize2fs`/`xfs_growfs`. `modify-volume` only changes the block device; the filesystem must be grown. Check for deleted-open files and inodes too.

### F — IMDS 401

Fault domain: IMDSv2 enforcement. Use a token; update the SDK/agent. Do not set `HttpTokens=optional` to make a legacy app work without understanding the SSRF risk.

### G — Lost key pair

Fault domain: access, not the instance. Use SSM Session Manager (if configured), EC2 Instance Connect, or attach the volume to a recovery instance. Do not terminate the instance to "reset" the key; you lose the instance store and may lose data.

### H — Instance store data gone after stop

Instance store is ephemeral. Stopping/terminating the instance loses it. Use EBS for persistence; instance store is for cache/scratch.

---

## 7. Command field manual

### `aws ec2 run-instances`

```bash
aws ec2 run-instances \
  --image-id ami-123 \
  --instance-type t3.micro \
  --subnet-id subnet-123 \
  --security-group-ids sg-123 \
  --iam-instance-profile Name=SSMRole \
  --metadata-options HttpTokens=required,HttpPutResponseHopLimit=1 \
  --tag-specifications 'ResourceType=instance,Tags=[{Key=Name,Value=opslab}]'
```

**Why:** reproducible launch; the CLI exposes the options the console hides. `--iam-instance-profile` is how the instance gets AWS permissions without keys.

**Mistake:** launching without a subnet/SG in the intended VPC (uses the default VPC), or forgetting `HttpTokens=required`.

### `aws ec2 describe-instance-status`

```bash
aws ec2 describe-instance-status --instance-ids i-123 --include-all-instances
```

**Interpretation:** `SystemStatus` vs `InstanceStatus`. `impaired` means the respective check failed; `insufficient-data` is common right after launch.

### `aws elbv2 describe-target-health`

```bash
aws elbv2 describe-target-health --target-group-arn <arn> \
  --query 'TargetHealthDescriptions[].{Target:Target.Id,Port:Target.Port,State:TargetHealth.State,Reason:TargetHealth.Reason}'
```

**Interpretation:** `State` and `Reason` are the fastest way to distinguish app, SG, and health-check problems. `Target.FailedHealthChecks` with a 404 is a path problem; `Target.Timeout` is a network/SG problem.

### `aws elbv2 describe-load-balancers`

```bash
aws elbv2 describe-load-balancers --names opslab-alb \
  --query 'LoadBalancers[0].{DNS:DNSName,Scheme:Scheme,VPC:VpcId,AZs:AvailabilityZones[].ZoneName}'
```

**Interpretation:** `Scheme` `internet-facing` vs `internal`; subnets must be public for an internet-facing ALB. A `provisioning`/`failed` state points to IAM/subnet issues.

### `aws autoscaling describe-auto-scaling-groups`

```bash
aws autoscaling describe-auto-scaling-groups --auto-scaling-group-names opslab-asg \
  --query 'AutoScalingGroups[0].{Min:MinSize,Max:MaxSize,Desired:DesiredCapacity,Instances:Instances[].{ID:InstanceId,Health:HealthStatus,Lifecycle:LifecycleState}}'
```

**Interpretation:** `HealthStatus` can be `Healthy`/`Unhealthy`; `LifecycleState` `InService`/`Pending`/`Terminating`. A desired count of 0 explains an empty target group.

### `aws ec2 describe-volumes`

```bash
aws ec2 describe-volumes --filters Name=attachment.instance-id,Values=i-123 \
  --query 'Volumes[].{ID:VolumeId,Size:Size,Type:VolumeType,IOPS:Iops,State:State,AZ:AvailabilityZone}'
```

**Interpretation:** `in-use` vs `available`; an unattached volume still costs money. Check the AZ; an EBS volume cannot attach across AZs.

### `aws ec2 get-console-output`

Already covered; the first thing when SSH/SSM fails and the instance is running.

---

## 8. Checkpoint

1. What is the difference between an EC2 system status check and an instance status check?
2. Why is `running` not a health signal?
3. What is the difference between an AMI and a launch template?
4. Why are EBS volumes AZ-scoped?
5. What does `modify-volume` do, and what does it not do?
6. When can you use EBS Multi-Attach, and what filesystem caveat applies?
7. What is the difference between ALB and NLB?
8. What does a target group health check actually check?
9. What causes an ALB 502 vs 503 vs 504?
10. Why can a keepalive mismatch cause 502s?
11. What is the deregistration delay for?
12. What does `health_check_type=ELB` change in an ASG?
13. Why does an ASG not fix a failing application?
14. What is the risk of `latest` image tags in ECR?
15. What is the first check for an unreachable EC2 instance?
16. What does `get-console-output` show that SSH cannot?
17. What happens to instance store data when an instance stops?
18. Why is losing a key pair not a reason to terminate the instance?
19. What does an ALB `UnHealthyHostCount` alert tell you?
20. How do you prove an EBS volume is actually full vs the filesystem not grown?

### Answers

1. System = AWS hypervisor/network path; instance = guest OS responsiveness. Both must pass.
2. The VM is powered on but may have failed status checks, no route, a blocking firewall, or a dead app.
3. An AMI is a boot image; a launch template is a versioned configuration (AMI, type, SG, user data, IAM) used by ASGs.
4. EBS is attached over a network to a specific AZ; a volume cannot be attached to an instance in another AZ without snapshot/copy.
5. It changes the EBS volume size/type/IOPS; it does not grow the partition or filesystem.
6. `io1`/`io2` in the same AZ, with a cluster-aware filesystem/application; ext4/XFS will corrupt.
7. ALB is L7 HTTP-aware (host/path, TLS termination, proxy headers); NLB is L4 TCP/UDP/TLS with static IPs and source-IP preservation.
8. It opens a connection to the target on the configured port/path and expects a configured status code within the timeout/interval.
9. 502 = target closed/invalid response; 503 = no healthy targets; 504 = target timeout.
10. The target closes an idle keepalive connection just as the ALB reuses it; the ALB sees a reset and returns 502.
11. To let in-flight requests finish after a target is removed from the LB.
12. The ASG uses the ELB/target group health status, not just EC2 status checks, to replace instances.
13. It can only replace instances; if the AMI/user data/app/config is broken, new instances fail the same way.
14. `latest` is mutable; a rollout can silently deploy a different image, and rollback is ambiguous. Use immutable tags/digests.
15. Status checks, route/public IP, SG/NACL, key/SSH user, console output, SSM.
16. Boot/kernel/cloud-init messages before the OS/SSH is available; it is not an interactive shell.
17. It is lost on stop/terminate; instance store is ephemeral.
18. You can use SSM/Instance Connect or attach the volume to another instance; terminating loses data and instance-store state.
19. The LB has targets registered but they are failing health checks; users may see 503 or reduced capacity.
20. `lsblk` shows the block device size, `df -h` shows the filesystem size; if the device grew but the filesystem did not, run `growpart` + `resize2fs`/`xfs_growfs`.
