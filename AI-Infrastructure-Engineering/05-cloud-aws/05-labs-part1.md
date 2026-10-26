# Phase 5 Labs — Part 1 (L5.1–L5.2)

> **Cost and safety warning:** these labs create billable AWS resources. Set a budget alert before starting. Use a sandbox account, one region, and a short session. Run the cleanup section even if the lab fails. A NAT Gateway and a running EC2 instance cost money every hour they exist.
>
> **Never commit credentials.** Do not put access keys in a repository, shell history, or screenshot. Use SSO or short-lived STS credentials.

---

# Lab L5.1 — Build a VPC with Public, Private, and Isolated Subnets

## Objective

Build a two-AZ VPC with an internet gateway, one NAT gateway, public/private/isolated subnets, route tables, a security group, and a default NACL. Launch two EC2 instances, prove private egress through NAT, prove SG and NACL behavior, and clean up every resource.

## Prerequisites

- AWS CLI v2 configured with an administrative role in a sandbox account.
- `jq` (optional), `curl`.
- EC2/VPC/IAM permissions; SSM permissions for Session Manager.
- A budget alert and a maximum session of 2 hours.

## Setup

```bash
export AWS_REGION=us-east-1
export AWS_DEFAULT_REGION=$AWS_REGION
aws sts get-caller-identity
aws ec2 describe-availability-zones --region "$AWS_REGION" \
  --query 'AvailabilityZones[0:2].ZoneName' --output text
```

Set the two AZs from the output:

```bash
export AZ1=us-east-1a
export AZ2=us-east-1b
export VPC_CIDR=10.50.0.0/16
export PROJECT=opslab
```

Create the VPC:

```bash
VPC_ID=$(aws ec2 create-vpc --cidr-block "$VPC_CIDR" \
  --tag-specifications "ResourceType=vpc,Tags=[{Key=Name,Value=$PROJECT-vpc},{Key=Project,Value=$PROJECT}]" \
  --query 'Vpc.VpcId' --output text)
aws ec2 modify-vpc-attribute --vpc-id "$VPC_ID" --enable-dns-support
aws ec2 modify-vpc-attribute --vpc-id "$VPC_ID" --enable-dns-hostnames
echo "$VPC_ID"
```

Create subnets:

```bash
PUB_A=$(aws ec2 create-subnet --vpc-id "$VPC_ID" --cidr-block 10.50.1.0/24 \
  --availability-zone "$AZ1" --query 'Subnet.SubnetId' --output text)
PUB_B=$(aws ec2 create-subnet --vpc-id "$VPC_ID" --cidr-block 10.50.2.0/24 \
  --availability-zone "$AZ2" --query 'Subnet.SubnetId' --output text)
PRIV_A=$(aws ec2 create-subnet --vpc-id "$VPC_ID" --cidr-block 10.50.11.0/24 \
  --availability-zone "$AZ1" --query 'Subnet.SubnetId' --output text)
PRIV_B=$(aws ec2 create-subnet --vpc-id "$VPC_ID" --cidr-block 10.50.12.0/24 \
  --availability-zone "$AZ2" --query 'Subnet.SubnetId' --output text)
ISO_A=$(aws ec2 create-subnet --vpc-id "$VPC_ID" --cidr-block 10.50.21.0/24 \
  --availability-zone "$AZ1" --query 'Subnet.SubnetId' --output text)
aws ec2 modify-subnet-attribute --subnet-id "$PUB_A" --map-public-ip-on-launch
aws ec2 modify-subnet-attribute --subnet-id "$PUB_B" --map-public-ip-on-launch
```

Create and attach the internet gateway:

```bash
IGW=$(aws ec2 create-internet-gateway --query 'InternetGateway.InternetGatewayId' --output text)
aws ec2 attach-internet-gateway --vpc-id "$VPC_ID" --internet-gateway-id "$IGW"
```

Create the NAT gateway (one, in `public-a`):

```bash
EIP=$(aws ec2 allocate-address --domain vpc --query 'AllocationId' --output text)
NAT=$(aws ec2 create-nat-gateway --subnet-id "$PUB_A" --allocation-id "$EIP" \
  --query 'NatGateway.NatGatewayId' --output text)
aws ec2 wait nat-gateway-available --nat-gateway-ids "$NAT"
echo "$NAT"
```

Create route tables:

```bash
PUB_RT=$(aws ec2 create-route-table --vpc-id "$VPC_ID" --query 'RouteTable.RouteTableId' --output text)
aws ec2 create-route --route-table-id "$PUB_RT" --destination-cidr-block 0.0.0.0/0 --gateway-id "$IGW"
aws ec2 associate-route-table --route-table-id "$PUB_RT" --subnet-id "$PUB_A"
aws ec2 associate-route-table --route-table-id "$PUB_RT" --subnet-id "$PUB_B"

PRIV_RT=$(aws ec2 create-route-table --vpc-id "$VPC_ID" --query 'RouteTable.RouteTableId' --output text)
aws ec2 create-route --route-table-id "$PRIV_RT" --destination-cidr-block 0.0.0.0/0 --nat-gateway-id "$NAT"
aws ec2 associate-route-table --route-table-id "$PRIV_RT" --subnet-id "$PRIV_A"
aws ec2 associate-route-table --route-table-id "$PRIV_RT" --subnet-id "$PRIV_B"
```

The isolated subnet keeps the VPC's main route table (local only). No default route means no internet and no NAT.

Security group:

```bash
SG=$(aws ec2 create-security-group --group-name "$PROJECT-sg" \
  --description "opslab web and SSM" --vpc-id "$VPC_ID" --query 'GroupId' --output text)
aws ec2 authorize-security-group-ingress --group-id "$SG" \
  --protocol tcp --port 80 --cidr 10.50.0.0/16
aws ec2 authorize-security-group-egress --group-id "$SG" \
  --protocol tcp --port 443 --cidr 0.0.0.0/0
```

The default egress rule already allows all traffic; the explicit 443 rule is redundant but harmless. For a stricter lab, revoke the default egress and allow only 443 + DNS (UDP/TCP 53). Do not break SSM: it needs 443 to the SSM endpoints.

IAM role and instance profile for SSM:

```bash
cat > /tmp/ec2-trust.json <<'EOF'
{
  "Version": "2012-10-17",
  "Statement": [{
    "Effect": "Allow",
    "Principal": {"Service": "ec2.amazonaws.com"},
    "Action": "sts:AssumeRole"
  }]
}
EOF
aws iam create-role --role-name "$PROJECT-ssm-role" \
  --assume-role-policy-document file:///tmp/ec2-trust.json
aws iam attach-role-policy --role-name "$PROJECT-ssm-role" \
  --policy-arn arn:aws:iam::aws:policy/AmazonSSMManagedInstanceCore
aws iam create-instance-profile --instance-profile-name "$PROJECT-ssm-profile"
aws iam add-role-to-instance-profile --instance-profile-name "$PROJECT-ssm-profile" \
  --role-name "$PROJECT-ssm-role"
sleep 10
```

AMI:

```bash
AMI=$(aws ssm get-parameters \
  --names /aws/service/ami-amazon-linux-latest/al2023-ami-kernel-default-x86_64 \
  --query 'Parameters[0].Value' --output text)
echo "$AMI"
```

User data:

```bash
cat > /tmp/public-userdata.sh <<'EOF'
#!/bin/bash
dnf install -y nginx
systemctl enable --now nginx
echo "public $(hostname)" > /usr/share/nginx/html/index.html
EOF
cat > /tmp/private-userdata.sh <<'EOF'
#!/bin/bash
dnf install -y nginx
systemctl enable --now nginx
echo "private $(hostname)" > /usr/share/nginx/html/index.html
EOF
```

Launch the public and private instances:

```bash
PUB_INST=$(aws ec2 run-instances --image-id "$AMI" --instance-type t3.micro \
  --subnet-id "$PUB_A" --security-group-ids "$SG" \
  --iam-instance-profile Name="$PROJECT-ssm-profile" \
  --associate-public-ip-address \
  --metadata-options HttpTokens=required,HttpPutResponseHopLimit=1 \
  --user-data file:///tmp/public-userdata.sh \
  --tag-specifications "ResourceType=instance,Tags=[{Key=Name,Value=$PROJECT-public}]" \
  --query 'Instances[0].InstanceId' --output text)

PRIV_INST=$(aws ec2 run-instances --image-id "$AMI" --instance-type t3.micro \
  --subnet-id "$PRIV_A" --security-group-ids "$SG" \
  --iam-instance-profile Name="$PROJECT-ssm-profile" \
  --metadata-options HttpTokens=required,HttpPutResponseHopLimit=1 \
  --user-data file:///tmp/private-userdata.sh \
  --tag-specifications "ResourceType=instance,Tags=[{Key=Name,Value=$PROJECT-private}]" \
  --query 'Instances[0].InstanceId' --output text)

aws ec2 wait instance-running --instance-ids "$PUB_INST" "$PRIV_INST"
aws ec2 describe-instances --instance-ids "$PUB_INST" "$PRIV_INST" \
  --query 'Reservations[].Instances[].{ID:InstanceId,Subnet:SubnetId,Private:PrivateIpAddress,Public:PublicIpAddress,State:State.Name}'
```

Wait for SSM to register:

```bash
for i in $(seq 1 30); do
  aws ssm describe-instance-information \
    --filters "Key=InstanceIds,Values=$PUB_INST,$PRIV_INST" \
    --query 'InstanceInformationList[].PingStatus' --output text
  sleep 10
done
```

Expected: `Online Online` (may take 1–3 minutes after boot).

## Part A — Prove private egress through NAT

Start a session on the private instance:

```bash
aws ssm start-session --target "$PRIV_INST"
```

Inside:

```bash
curl -sS --max-time 10 https://example.com | head -2
curl -sS --max-time 10 http://<PUBLIC_PRIVATE_IP>/ 
getent hosts example.com
ip route
```

Expected: the HTTPS request succeeds through the NAT gateway; the public instance returns `public <hostname>`; DNS resolves; the default route points to the VPC router (`10.50.0.1` or similar), which forwards to the NAT.

On the isolated subnet, if you launch an instance there, the same curl times out (no route). You can prove this conceptually with a route table lookup:

```bash
aws ec2 describe-route-tables --filters Name=association.subnet-id,Values="$ISO_A" \
  --query 'RouteTables[].Routes[].{Dest:DestinationCidrBlock,Target:GatewayId,State:State}'
```

## Part B — Security group behavior

From the private session, the public instance's port 80 works because the SG allows `10.50.0.0/16` on 80. Remove the rule:

```bash
aws ec2 revoke-security-group-ingress --group-id "$SG" \
  --protocol tcp --port 80 --cidr 10.50.0.0/16
```

Retry from the private session:

```bash
curl -v --max-time 5 http://<PUBLIC_PRIVATE_IP>/
```

Expected: timeout (the SG drops silently). Re-add:

```bash
aws ec2 authorize-security-group-ingress --group-id "$SG" \
  --protocol tcp --port 80 --cidr 10.50.0.0/16
```

## Part C — NACL behavior

Find the default NACL for the VPC:

```bash
NACL=$(aws ec2 describe-network-acls --filters Name=vpc-id,Values="$VPC_ID" \
  Name=default,Values=true --query 'NetworkAcls[0].NetworkAclId' --output text)
echo "$NACL"
```

Add an egress deny for the public instance's private IP on port 80, with a lower rule number than the default allow:

```bash
aws ec2 create-network-acl-entry --network-acl-id "$NACL" \
  --rule-number 90 --protocol tcp --port-range From=80,To=80 \
  --cidr-block <PUBLIC_PRIVATE_IP>/32 --egress --rule-action deny
```

Retry from the private session:

```bash
curl -v --max-time 5 http://<PUBLIC_PRIVATE_IP>/
```

Expected: timeout, even though the SG allows 80. The NACL is stateless; you denied only the outbound request, and the return path may also need rules. Remove the deny:

```bash
aws ec2 delete-network-acl-entry --network-acl-id "$NACL" \
  --rule-number 90 --egress
```

**Do not change the default NACL's allow-all rule or SSH/SSM ports.** You can lock yourself out of the instances.

## Part D — Route table evidence

```bash
aws ec2 describe-route-tables --filters Name=vpc-id,Values="$VPC_ID" \
  --query 'RouteTables[].{ID:RouteTableId,Assoc:Associations[].SubnetId,Routes:Routes[].{Dest:DestinationCidrBlock,IGW:GatewayId,NAT:NatGatewayId,State:State}}' \
  --output json
```

Write down which subnet uses which table. Confirm:

- public subnets → IGW,
- private subnets → NAT,
- isolated → local only.

## Expected observations

| Check | Expected |
|---|---|
| Public instance | Has public IP; reachable from internet if SG allows |
| Private instance | No public IP; SSM works via NAT/endpoints |
| Private `curl https://example.com` | Works via NAT |
| Private `curl http://public-private-ip` | Works when SG allows 80 |
| SG rule removed | Timeout (drop) |
| NACL deny 80 | Timeout even with SG allow |
| Isolated subnet | No default route; no internet |
| Route tables | Public → IGW; private → NAT; isolated → local |

## Internal explanation

- The internet gateway is the VPC's bidirectional door; the NAT gateway is an outbound proxy for private subnets.
- A subnet's route table, not its name, determines whether it is public.
- SGs are stateful per-ENI; NACLs are stateless per-subnet and require both directions.
- SSM works from a private instance because the agent connects outbound to the SSM endpoints through NAT (or interface endpoints), not because of inbound access.

## Troubleshooting task

A teammate reports: "The private instance can reach `example.com` but not `s3.amazonaws.com`; the public instance can reach both." You have no packet capture. Write the fault domain and the first four checks, covering:

1. Route table for the private subnet.
2. NAT gateway state/subnet/EIP.
3. SG/NACL egress and DNS.
4. VPC endpoint/private DNS for S3.

Do not change anything until you have listed what each check would prove.

## Verification

- [ ] VPC has public, private, and isolated subnets in two AZs.
- [ ] Public subnet routes to IGW; private routes to NAT; isolated has no default.
- [ ] SSM session works to both instances.
- [ ] Private instance reaches the internet through NAT.
- [ ] SG removal causes a timeout; restoring it fixes the path.
- [ ] NACL deny causes a timeout even with SG allow; removing it fixes the path.
- [ ] You can explain the 5 reserved IPs and why a `/24` is 251 usable.

## Cleanup

```bash
# Terminate instances first
aws ec2 terminate-instances --instance-ids "$PUB_INST" "$PRIV_INST"
aws ec2 wait instance-terminated --instance-ids "$PUB_INST" "$PRIV_INST"

# NAT gateway and EIP
aws ec2 delete-nat-gateway --nat-gateway-id "$NAT"
aws ec2 wait nat-gateway-deleted --nat-gateway-ids "$NAT" 2>/dev/null || sleep 60
aws ec2 release-address --allocation-id "$EIP"

# Route tables (disassociate then delete)
for RT in "$PUB_RT" "$PRIV_RT"; do
  for ASSOC in $(aws ec2 describe-route-tables --route-table-ids "$RT" \
    --query 'RouteTables[].Associations[?Main==`false`].RouteTableAssociationId' --output text); do
    aws ec2 disassociate-route-table --association-id "$ASSOC" 2>/dev/null || true
  done
  aws ec2 delete-route-table --route-table-id "$RT" 2>/dev/null || true
done

# IGW, subnets, SG, VPC
aws ec2 detach-internet-gateway --vpc-id "$VPC_ID" --internet-gateway-id "$IGW" 2>/dev/null || true
aws ec2 delete-internet-gateway --internet-gateway-id "$IGW" 2>/dev/null || true
for SN in "$PUB_A" "$PUB_B" "$PRIV_A" "$PRIV_B" "$ISO_A"; do
  aws ec2 delete-subnet --subnet-id "$SN" 2>/dev/null || true
done
aws ec2 delete-security-group --group-id "$SG" 2>/dev/null || true
aws ec2 delete-vpc --vpc-id "$VPC_ID" 2>/dev/null || true

# IAM
aws iam remove-role-from-instance-profile --instance-profile-name "$PROJECT-ssm-profile" \
  --role-name "$PROJECT-ssm-role" 2>/dev/null || true
aws iam delete-instance-profile --instance-profile-name "$PROJECT-ssm-profile" 2>/dev/null || true
aws iam detach-role-policy --role-name "$PROJECT-ssm-role" \
  --policy-arn arn:aws:iam::aws:policy/AmazonSSMManagedInstanceCore 2>/dev/null || true
aws iam delete-role --role-name "$PROJECT-ssm-role" 2>/dev/null || true
```

Verify:

```bash
aws ec2 describe-nat-gateways --filter Name=state,Values=available,pending \
  --query 'NatGateways[].{ID:NatGatewayId,State:State,VPC:VpcId}'
aws ec2 describe-instances --filters Name=instance-state-name,Values=running \
  --query 'Reservations[].Instances[].{ID:InstanceId,Name:Tags[?Key==`Name`].Value|[0]}'
aws ec2 describe-vpcs --filters Name=tag:Project,Values="$PROJECT" \
  --query 'Vpcs[].VpcId'
```

There should be no lab NAT gateway, instance, or VPC left. NAT deletion can take a minute; wait before releasing the EIP.

## Write-up

```markdown
# L5.1 — VPC
## CIDR/subnet/AZ plan (table)
## Route table evidence (public/private/isolated)
## SSM and NAT proof
## SG vs NACL experiment: what changed and why
## Isolated subnet proof
## Cleanup verification
## The first check I would run for "private instance has no internet"
```

## Debrief for the troubleshooting task

- **Route table:** the private subnet may have a route to a NAT in a different AZ, or no route. Prove with `describe-route-tables`.
- **NAT:** state must be `available`, the NAT must be in a public subnet with an IGW route, and the EIP must be attached.
- **SG/NACL/DNS:** allow outbound 443 and DNS; the NACL return path must allow ephemeral ports. If `getent hosts s3.amazonaws.com` fails, DNS is the first problem.
- **Endpoint:** a gateway endpoint for S3 needs a route table entry; an interface endpoint needs an SG allowing 443 and private DNS. The public instance may use the public S3 endpoint via IGW and appear to work, masking the private path.

---

# Lab L5.2 — IAM Least Privilege and Evidence

## Objective

Create a least-privilege IAM role, assume it with STS, prove allowed and denied S3 actions, read the policy simulation and CloudTrail evidence, and clean up without leaving long-lived credentials.

## Prerequisites

- AWS CLI configured with an administrative sandbox role.
- Permission to create IAM roles/policies and S3 buckets.
- L5.1 is not required.
- 20–30 minutes.

## Setup

```bash
export AWS_REGION=us-east-1
export AWS_DEFAULT_REGION=$AWS_REGION
ACCOUNT=$(aws sts get-caller-identity --query Account --output text)
BUCKET="opslab-iam-$ACCOUNT-$RANDOM"
echo "$ACCOUNT $BUCKET"
aws s3api create-bucket --bucket "$BUCKET" --region "$AWS_REGION"
aws s3api put-public-access-block --bucket "$BUCKET" \
  --public-access-block-configuration \
  BlockPublicAcls=true,IgnorePublicAcls=true,BlockPublicPolicy=true,RestrictPublicBuckets=true
echo "hello from opslab" > /tmp/hello.txt
aws s3 cp /tmp/hello.txt "s3://$BUCKET/hello.txt"
```

## Part A — Create the role and policy

Trust policy (same account):

```bash
cat > /tmp/trust.json <<EOF
{
  "Version": "2012-10-17",
  "Statement": [{
    "Effect": "Allow",
    "Principal": {"AWS": "arn:aws:iam::$ACCOUNT:root"},
    "Action": "sts:AssumeRole"
  }]
}
EOF
aws iam create-role --role-name OpsLabS3Reader \
  --assume-role-policy-document file:///tmp/trust.json
```

Permissions policy: list the bucket, get objects, explicitly deny delete.

```bash
cat > /tmp/s3-reader.json <<EOF
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Sid": "ListAndRead",
      "Effect": "Allow",
      "Action": ["s3:ListBucket"],
      "Resource": "arn:aws:s3:::$BUCKET"
    },
    {
      "Sid": "ReadObjects",
      "Effect": "Allow",
      "Action": ["s3:GetObject"],
      "Resource": "arn:aws:s3:::$BUCKET/*"
    },
    {
      "Sid": "ExplicitDenyDelete",
      "Effect": "Deny",
      "Action": ["s3:DeleteObject", "s3:DeleteObjectVersion"],
      "Resource": "arn:aws:s3:::$BUCKET/*"
    }
  ]
}
EOF
aws iam put-role-policy --role-name OpsLabS3Reader \
  --policy-name OpsLabS3ReaderPolicy --policy-document file:///tmp/s3-reader.json
```

## Part B — Assume the role and prove behavior

```bash
CREDS=$(aws sts assume-role \
  --role-arn "arn:aws:iam::$ACCOUNT:role/OpsLabS3Reader" \
  --role-session-name opslab-lab \
  --query 'Credentials' --output json)
export AWS_ACCESS_KEY_ID=$(echo "$CREDS" | jq -r .AccessKeyId)
export AWS_SECRET_ACCESS_KEY=$(echo "$CREDS" | jq -r .SecretAccessKey)
export AWS_SESSION_TOKEN=$(echo "$CREDS" | jq -r .SessionToken)
aws sts get-caller-identity
```

Expected:

```json
{
  "UserId": "AROA...:opslab-lab",
  "Account": "111122223333",
  "Arn": "arn:aws:sts::111122223333:assumed-role/OpsLabS3Reader/opslab-lab"
}
```

Allowed:

```bash
aws s3 ls "s3://$BUCKET"
aws s3 cp "s3://$BUCKET/hello.txt" -
```

Denied:

```bash
aws s3 rm "s3://$BUCKET/hello.txt"
aws s3api delete-object --bucket "$BUCKET" --key hello.txt
```

Expected: `AccessDenied` for both deletes. The explicit deny wins even though the role has no allow for delete.

Denied by implicit deny (a different bucket):

```bash
aws s3 ls s3://some-other-bucket-you-do-not-own
```

Expected: `AccessDenied` (implicit or bucket policy). The error text does not tell you which policy; CloudTrail and the policy simulation do.

## Part C — Simulate and audit

```bash
aws iam simulate-principal-policy \
  --policy-source-arn "arn:aws:iam::$ACCOUNT:role/OpsLabS3Reader" \
  --action-names s3:GetObject s3:DeleteObject s3:ListBucket \
  --resource-arns "arn:aws:s3:::$BUCKET/hello.txt" "arn:aws:s3:::$BUCKET" \
  --query 'EvaluationResults[].{Action:EvalActionName,Decision:EvalDecision,Missing:MissingContextValues}'
```

Expected: `s3:GetObject` allowed, `s3:DeleteObject` explicitDeny, `s3:ListBucket` allowed (resource ARN must match the bucket, not the object).

CloudTrail:

```bash
aws cloudtrail lookup-events \
  --lookup-attributes AttributeKey=EventName,AttributeValue=DeleteObject \
  --max-results 5
```

Find the `errorCode: AccessDenied` event and the `userIdentity.arn` of the assumed role. This is how you prove who did what.

## Expected observations

| Action | Result | Why |
|---|---|---|
| `sts get-caller-identity` | assumed-role ARN | STS session |
| `s3 ls bucket` | allowed | `ListBucket` on bucket ARN |
| `s3 cp object -` | allowed | `GetObject` on object ARN |
| `s3 rm object` | denied | explicit deny |
| `s3 ls other-bucket` | denied | no allow (implicit deny) |
| Simulate `DeleteObject` | explicitDeny | policy simulation |
| CloudTrail | AccessDenied event with role ARN | audit evidence |

## Internal explanation

- A role has a trust policy (who can assume) and a permissions policy (what it can do).
- STS returns temporary credentials; the session ARN identifies the role session.
- Explicit deny overrides any allow; the absence of an allow is an implicit deny.
- CloudTrail records the API call and the principal; the error message alone rarely names the policy.

## Troubleshooting task

You change the `ReadObjects` resource to `arn:aws:s3:::wrong-bucket/*` and `s3 cp` now fails with `AccessDenied`, while `s3 ls` still works. Explain why, using the policy document and the ARN matching rules. Then fix it and prove with `s3 cp` and `simulate-principal-policy`.

A second ticket: `aws sts get-caller-identity` shows the admin role, not `OpsLabS3Reader`, after you "assumed" the role. What happened, and what would you check in the shell?

## Verification

- [ ] You created a role with a trust policy and a least-privilege permissions policy.
- [ ] You assumed it and proved the session ARN.
- [ ] List/get succeeded; delete was denied by explicit deny.
- [ ] A different bucket was denied by implicit deny.
- [ ] Policy simulation matched the real behavior.
- [ ] CloudTrail showed the denied call and the role ARN.
- [ ] You can explain trust vs permissions vs explicit deny.

## Cleanup

```bash
unset AWS_ACCESS_KEY_ID AWS_SECRET_ACCESS_KEY AWS_SESSION_TOKEN
aws s3 rm "s3://$BUCKET" --recursive
aws s3api delete-bucket --bucket "$BUCKET" --region "$AWS_REGION"
aws iam delete-role-policy --role-name OpsLabS3Reader --policy-name OpsLabS3ReaderPolicy
aws iam delete-role --role-name OpsLabS3Reader
rm -f /tmp/trust.json /tmp/s3-reader.json /tmp/hello.txt
```

Verify:

```bash
aws iam get-role --role-name OpsLabS3Reader 2>&1 | head -2
aws s3api head-bucket --bucket "$BUCKET" 2>&1 | head -2
```

Both should report not found. If the bucket is not empty, check versioning/delete markers.

## Write-up

```markdown
# L5.2 — IAM least privilege
## Trust policy vs permissions policy
## Allowed/denied matrix with actual errors
## Simulation vs real behavior
## CloudTrail evidence
## The mistake I made (wrong ARN, wrong principal, stale credentials)
## How I would give a CI job S3 access without keys
```

## Debrief

- **Wrong bucket ARN:** `ListBucket` uses the bucket ARN; `GetObject` uses the object ARN. If the resource does not match, the allow does not apply → implicit deny.
- **Still admin:** `export` in a subshell does not affect the parent; or the CLI profile/role wasn't switched. Check `aws sts get-caller-identity` and `env | grep AWS`.
- **CI without keys:** an IAM role with a trust policy for the CI provider's OIDC identity, short-lived `AssumeRoleWithWebIdentity`, and a least-privilege policy. No static access keys.
