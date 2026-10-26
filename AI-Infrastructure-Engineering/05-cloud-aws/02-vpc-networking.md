# AWS VPC Networking

> **The one-sentence model:** a VPC is a private IP space; each subnet is tied to one AZ and one route table; an internet gateway gives public subnets a path to the internet; a NAT gateway gives private subnets an outbound path; security groups are stateful per-ENI firewalls; network ACLs are stateless per-subnet firewalls. Most "cloud network" tickets are one of: no route, no NAT, SG vs NACL, DNS, or an overlapping CIDR.

---

## 1. VPC and subnets

```text
VPC 10.0.0.0/16
├── public-a   10.0.1.0/24   AZ a   route 0.0.0.0/0 → IGW
├── public-b   10.0.2.0/24   AZ b   route 0.0.0.0/0 → IGW
├── private-a  10.0.11.0/24  AZ a   route 0.0.0.0/0 → NAT GW in public-a
├── private-b  10.0.12.0/24  AZ b   route 0.0.0.0/0 → NAT GW in public-b
└── isolated-a 10.0.21.0/24  AZ a   no default route; VPC endpoints only
```

Rules:

- A VPC CIDR is `/16` to `/28` (IPv4). Plan for growth; changing it later is painful.
- A subnet is **AZ-scoped** and cannot span AZs. One route table association per subnet.
- AWS reserves **5 IP addresses** in every subnet: network address, VPC router, DNS, future use, broadcast. A `/24` has 251 usable addresses, not 256.
- A **public subnet** is public because its route table has `0.0.0.0/0 → igw-*`, not because of its name. An instance also needs a public IPv4/EIP to be reachable.
- A **private subnet** has no direct IGW route. It uses NAT for outbound, or VPC endpoints for AWS services.
- An **isolated subnet** has no default route at all; use endpoints or a proxy.

```bash
aws ec2 describe-vpcs --query 'Vpcs[].{ID:VpcId,CIDR:CidrBlock,Default:IsDefault}' --output table
aws ec2 describe-subnets --filters Name=vpc-id,Values=vpc-123 \
  --query 'Subnets[].{ID:SubnetId,AZ:AvailabilityZone,CIDR:CidrBlock,PublicIP:MapPublicIpOnLaunch}' --output table
```

### Subnet sizing for EKS

The VPC CNI assigns **VPC IPs** to Pods. Each node consumes IPs for its ENIs and each Pod consumes one (or a prefix). A `/24` with 251 usable IPs may hold only a few nodes plus Pods. Check the instance type's ENI/IP limits and plan for:

```text
usable IPs per subnet ≥ nodes × IPs-per-node + Pods + headroom
```

Prefix delegation assigns `/28` prefixes to ENIs, reducing IP churn but consuming more addresses per ENI. Size subnets before you need them; expanding a subnet is not possible in place (you add a new subnet/CIDR).

---

## 2. Route tables

Every VPC has a **main route table**; subnets can use it or an explicit association.

| Destination | Target | Meaning |
|---|---|---|
| `10.0.0.0/16` | `local` | Intra-VPC routing; always present |
| `0.0.0.0/0` | `igw-*` | Public internet path |
| `0.0.0.0/0` | `nat-*` | Outbound-only path via NAT |
| `10.1.0.0/16` | `pcx-*` | VPC peering |
| `10.2.0.0/16` | `tgw-*` | Transit Gateway |
| `pl-*` | `vpce-*` | Gateway endpoint (S3/DynamoDB) |
| `0.0.0.0/0` | `blackhole` | Target deleted; traffic dropped |

Longest prefix wins. A route to `10.1.0.0/16` beats `0.0.0.0/0` for `10.1.2.3`, even if the default route points to a NAT.

```bash
aws ec2 describe-route-tables --filters Name=vpc-id,Values=vpc-123 \
  --query 'RouteTables[].{ID:RouteTableId,Assoc:Associations[].SubnetId,Routes:Routes[].{Dest:DestinationCidrBlock,Target:GatewayId,State:State}}' \
  --output json
```

**Common mistake:** associating a private subnet with the main route table that has an IGW route (making it public), or creating a NAT gateway but forgetting to add the route.

### Internet gateway vs NAT gateway

| | Internet Gateway (IGW) | NAT Gateway |
|---|---|---|
| Direction | Bidirectional | Outbound-initiated only |
| Placement | Attached to the VPC | In a public subnet, AZ-scoped |
| Public IP | Instance needs one/EIP | NAT has an EIP |
| Inbound | Allowed if SG/NACL allow | Not allowed (no port forwarding) |
| Cost | No hourly charge | Hourly + data processing |
| HA | Horizontally scaled | One per AZ for AZ resilience |

A NAT Gateway in AZ `a` is not used by a private subnet in AZ `b` unless you route across AZs (extra cost and a cross-AZ dependency). Production designs usually put one NAT per AZ.

### VPC endpoints

| Type | Services | How it works |
|---|---|---|
| **Gateway endpoint** | S3, DynamoDB | A route table entry to `vpce-*`; no ENI, no SG |
| **Interface endpoint** | Most AWS APIs (ECR, STS, KMS, CloudWatch) | An ENI in your subnet with an SG; PrivateLink |
| **Gateway Load Balancer endpoint** | Appliances | For inspection/firewall |

Endpoints keep traffic off the internet and can reduce NAT costs. They are not free (interface endpoints have hourly + data charges), and they need correct DNS and SGs.

```bash
aws ec2 describe-vpc-endpoints --filters Name=vpc-id,Values=vpc-123 \
  --query 'VpcEndpoints[].{ID:VpcEndpointId,Service:ServiceName,Type:VpcEndpointType,State:State}'
```

---

## 3. Security groups vs network ACLs

| Property | Security Group | Network ACL |
|---|---|---|
| Level | ENI/instance | Subnet |
| Stateful | Yes; return traffic automatically allowed | No; you must allow both directions |
| Rules | Allow only | Allow and deny |
| Evaluation | All rules, union | Numbered order; first match wins |
| Default | Deny inbound, allow outbound | Allow all (default NACL) |
| References | Can reference other SGs | CIDR only |
| Use | Primary workload firewall | Coarse subnet guardrail |

### Stateful vs stateless

A security group allowing inbound TCP 443 automatically allows the response outbound, regardless of outbound rules. A NACL does not: if a client connects to a server, the NACL must allow:

- outbound from the client subnet to the server port (e.g., 443),
- inbound to the server subnet on 443,
- outbound from the server subnet to the client's **ephemeral port** (1024–65535),
- inbound to the client subnet from the server port to the ephemeral port.

Forgetting the ephemeral-port rule is the classic "ping works but TCP hangs" or "SYN reaches the server but the reply is dropped" failure.

```bash
aws ec2 describe-security-groups --filters Name=vpc-id,Values=vpc-123 \
  --query 'SecurityGroups[].{Name:GroupName,ID:GroupId,Ingress:IpPermissions,Egress:IpPermissionsEgress}'
aws ec2 describe-network-acls --filters Name=vpc-id,Values=vpc-123 \
  --query 'NetworkAcls[].{ID:NetworkAclId,Entries:Entries[].{Num:RuleNumber,Action:RuleAction,CIDR:CidrBlock,Port:PortRange}}'
```

**Debug rule:** if the SG allows the port but the connection still times out, check the NACL return path and the route table. If the connection is refused, the packet reached the host but nothing was listening (or an SG rejected with REJECT, which AWS SGs do not do; SGs drop).

---

## 4. DNS and Route 53

- `enableDnsSupport` and `enableDnsHostnames` must be true for the VPC's DNS to work and for instances to get DNS names.
- The VPC resolver is at `VPC CIDR + 2` (e.g., `10.0.0.2`). It resolves public names and private hosted zones.
- Route 53 private hosted zones associate with VPCs; cross-account association is possible.
- Interface endpoints create private DNS names; if `privateDnsEnabled` is false, the public endpoint is used.
- DHCP option sets can change the domain name and DNS servers; changing them affects new leases, not existing ones.

```bash
aws ec2 describe-vpc-attribute --vpc-id vpc-123 --attribute enableDnsSupport
aws ec2 describe-vpc-attribute --vpc-id vpc-123 --attribute enableDnsHostnames
aws route53 list-hosted-zones
```

**Common mistake:** assuming a private subnet can resolve/route to S3 through a gateway endpoint without a route table entry, or that an interface endpoint is reachable when its SG does not allow 443 from the client.

---

## 5. VPC connectivity

| Option | Scope | Transitive? | Overlapping CIDRs? | Use |
|---|---|---|---|---|
| VPC peering | 1:1 | No | No | Simple, low-latency, few VPCs |
| Transit Gateway | Hub | Yes | No (unless appliance/NAT) | Many VPCs/accounts, centralized routing |
| PrivateLink | Service | N/A | Yes (service side) | Expose a service without full network peering |
| VPN | Site-to-site | Depends | Possible with NAT | On-prem/hybrid |
| Direct Connect | Dedicated | Depends | Possible | High-bandwidth hybrid |

Peering is not transitive: A↔B and B↔C does not give A↔C. You need a route in both directions and SG rules on both sides. Overlapping CIDRs cannot be peered; re-IP or use PrivateLink/NAT.

---

## 6. Flow logs and Reachability Analyzer

Flow logs record IP traffic metadata (source/destination, ports, protocol, packets, bytes, action) at the VPC/subnet/ENI level. They do **not** record packet payloads or the process/identity.

```bash
aws ec2 create-flow-logs \
  --resource-type VPC --resource-ids vpc-123 \
  --traffic-type ALL \
  --log-destination-type cloud-watch-logs \
  --log-group-name /aws/vpc/opslab
```

Fields to read: `srcaddr`, `dstaddr`, `srcport`, `dstport`, `protocol`, `action` (`ACCEPT`/`REJECT`), `log-status`. `REJECT` is a NACL/SG drop (or no route in some cases); `ACCEPT` does not prove the application responded.

```bash
aws ec2 create-network-insights-path \
  --source <instance-id> --destination <instance-id> --protocol tcp --destination-port 443
aws ec2 start-network-insights-analysis --network-insights-path-id nip-123
```

Reachability Analyzer evaluates SG, NACL, route tables, and endpoints for a path. It is faster than guessing, but it does not test the application or DNS.

---

## 7. EKS networking specifics

- **VPC CNI** assigns each Pod a VPC IP from the node's subnet. Pods are first-class VPC addresses; SGs/NACLs/flow logs see them.
- **Node ENI limits:** each instance type has a max ENIs and IPs per ENI. The CNI pre-allocates IPs; a node can run out of Pod IPs before CPU/memory.
- **Subnet exhaustion:** a `/24` with many nodes and Pods runs out; monitor `AvailableIpAddressCount` and CNI metrics.
- **Prefix delegation:** assigns `/28` prefixes per ENI, improving Pod density and reducing IP churn; requires planning.
- **Security groups for Pods:** can apply SGs directly to Pod ENIs; otherwise the node SG applies.
- **Service CIDR** is separate from the VPC CIDR; do not overlap.
- **Private nodes** need NAT or VPC endpoints for ECR, S3, STS, CloudWatch, and the EKS API.
- **Load balancers:** the AWS Load Balancer Controller creates ALB/NLB from Ingress/Service; it needs IAM permissions via IRSA/Pod Identity and subnet tags (`kubernetes.io/role/elb`, `internal-elb`).

```bash
kubectl get nodes -o wide
kubectl get pods -A -o wide
aws ec2 describe-subnets --subnet-ids <ids> \
  --query 'Subnets[].{ID:SubnetId,Free:AvailableIpAddressCount,CIDR:CidrBlock}'
```

---

## 8. Failure scenarios

### A — Private instance has no internet

```text
$ curl -m 5 https://example.com
curl: (28) Connection timed out
```

Fault domain: route/NAT/DNS/SG. Check, in order:

1. Subnet route table has `0.0.0.0/0 → nat-*` (not `local`/blackhole).
2. NAT Gateway is `available`, in a public subnet, with an EIP.
3. Public subnet route has `0.0.0.0/0 → igw-*`.
4. NAT's SG/NACL allows outbound and return traffic.
5. Instance SG allows outbound 443; NACL allows both directions.
6. DNS resolves (`getent hosts example.com`); if not, check `enableDnsSupport` and the resolver.

A private instance should not have a public IP; if it does, the route is still the issue.

### B — SG allows, but the connection hangs

Fault domain: NACL/route/return path. Check the NACL ephemeral ports, the route back to the client, and whether the client is in another VPC/on-prem. Capture with Flow Logs or `tcpdump` on the instance. A SYN arriving with no SYN-ACK can be the server's outbound NACL dropping the reply.

### C — Security group vs NACL

- SG: stateful, allow-only, evaluated as a union; a reference to another SG is allowed.
- NACL: stateless, ordered, allow/deny; the first matching rule wins; you must allow both directions.
- A NACL `DENY` for `0.0.0.0/0` on port 22 blocks SSH even if the SG allows it.

### D — VPC peering does not work

Check both route tables (both directions), both SGs, both NACLs, and overlapping CIDRs. Peering is not transitive. A route to the peer CIDR must point to `pcx-*` in each VPC; the peer must have a return route.

### E — Interface endpoint times out

Check the endpoint's SG allows 443 from the client, the endpoint policy allows the action, the subnet has free IPs, and DNS resolves to the endpoint (private DNS enabled or a custom DNS record). Gateway endpoints need a route table entry, not an SG.

### F — EKS Pods Pending with `failed to assign an IP`

Fault domain: subnet IP exhaustion/ENI limits. Check `AvailableIpAddressCount`, the instance type's ENI/IP limits, CNI logs (`aws-node`), and whether prefix delegation is enabled. Add subnets/IPs or use a larger subnet; do not just restart the CNI.

### G — DNS fails in a private subnet

Check `enableDnsSupport`, the DHCP option set, the Route 53 Resolver, and the endpoint/private hosted zone association. If only AWS service names fail, check the interface endpoint's private DNS and SG.

---

## 9. Command field manual

### `aws ec2 describe-route-tables`

**Purpose:** prove which route a subnet uses for a destination.

```bash
aws ec2 describe-route-tables \
  --filters Name=association.subnet-id,Values=subnet-123 \
  --query 'RouteTables[].Routes[].{Dest:DestinationCidrBlock,Target:GatewayId,Nat:NatGatewayId,State:State}'
```

**Interpretation:** `local` for the VPC CIDR; `igw-*` for public; `nat-*` for private egress; `blackhole` means the target was deleted. A missing default route is a private/isolated subnet.

**Mistake:** looking at the main route table instead of the subnet's associated table; a subnet can have an explicit association that overrides the main table.

### `aws ec2 describe-security-groups`

```bash
aws ec2 describe-security-groups --group-ids sg-123 \
  --query 'SecurityGroups[0].IpPermissions'
```

**Interpretation:** `IpProtocol`, `FromPort`, `ToPort`, `IpRanges`, `UserIdGroupPairs`. `-1` means all traffic. An egress rule of `0.0.0.0/0` on all ports is common and not automatically safe.

### `aws ec2 describe-network-acls`

```bash
aws ec2 describe-network-acls --filters Name=association.subnet-id,Values=subnet-123 \
  --query 'NetworkAcls[0].Entries'
```

**Interpretation:** rules are evaluated by `RuleNumber`; `Egress` false = ingress. A `*` rule at the end is the default deny/allow. Remember the ephemeral return path.

### `aws ec2 describe-flow-logs`

```bash
aws ec2 describe-flow-logs --filter Name=resource-id,Values=vpc-123
```

**Interpretation:** confirms logging is enabled and where records go. Absence of flow logs is not evidence of no traffic; it is absence of evidence.

### `aws ec2 describe-instances`

```bash
aws ec2 describe-instances --filters Name=vpc-id,Values=vpc-123 \
  --query 'Reservations[].Instances[].{ID:InstanceId,State:State.Name,Private:PrivateIpAddress,Public:PublicIpAddress,Subnet:SubnetId,SG:SecurityGroups}'
```

**Interpretation:** public IP may be absent in a private subnet; state `running` does not mean healthy (status checks). The subnet/SG IDs connect the instance to the network evidence.

### `aws ec2 get-console-output`

```bash
aws ec2 get-console-output --instance-id i-123 --latest
```

**Why:** sees boot/kernel messages when SSH/SSM is unavailable (cloud-init failure, filesystem corruption, kernel panic). It is not a shell.

### `aws ssm start-session`

```bash
aws ssm start-session --target i-123
```

**Why:** shell access without opening SSH or a public IP; requires the SSM agent and an instance profile with `AmazonSSMManagedInstanceCore`. It is the safer default for private instances.

---

## 10. Checkpoint

1. What makes a subnet public?
2. How many usable IPs are in a `/24` subnet, and why?
3. What is the difference between an IGW and a NAT gateway?
4. Why is a NAT gateway AZ-scoped, and what does that imply for HA?
5. What is the difference between a security group and a NACL?
6. Why does a NACL need ephemeral-port rules when a security group does not?
7. Which route wins: `10.1.0.0/16` via peering or `0.0.0.0/0` via NAT, for `10.1.2.3`?
8. What does a `blackhole` route mean?
9. What is a gateway endpoint vs an interface endpoint?
10. Why can two VPCs with overlapping CIDRs not be peered?
11. What does a VPC flow log record, and what does it not record?
12. Why does an EKS Pod consume a VPC IP?
13. What happens when a subnet runs out of IPs in EKS?
14. Why do private EKS nodes need NAT or endpoints?
15. What is the first check for a private instance that cannot reach the internet?
16. What is the first check for an SG-allowed connection that times out?
17. What does `enableDnsSupport=false` break?
18. Why is `0.0.0.0/0` in an SG egress rule not automatically safe?
19. What does Reachability Analyzer not test?
20. Why is "the instance is running" not proof that the network works?

### Answers

1. Its route table has a route to an internet gateway, and the instance has a public IP/EIP (or is behind a public LB).
2. 251; AWS reserves 5 (network, router, DNS, future, broadcast).
3. An IGW is bidirectional and attached to the VPC; a NAT gateway is outbound-only, lives in a public subnet, and uses an EIP.
4. A NAT gateway is created in one AZ; if that AZ fails, the private subnets routed to it lose egress. Use one NAT per AZ for resilience.
5. SG is stateful, allow-only, per-ENI; NACL is stateless, allow/deny, ordered, per-subnet.
6. NACLs do not track connection state; the return traffic uses an ephemeral port and must be explicitly allowed. SGs automatically allow return traffic for allowed connections.
7. The `/16` peering route, because longest prefix wins.
8. The route's target was deleted or is unavailable; traffic is dropped.
9. Gateway endpoints (S3/DynamoDB) use a route table target; interface endpoints use an ENI/SG/PrivateLink and private DNS.
10. The IP spaces are ambiguous; routing cannot tell which VPC owns an address. Re-IP or use PrivateLink/NAT.
11. It records metadata (src/dst IP/port, protocol, bytes, action); it does not record payload, process, or identity.
12. The VPC CNI assigns Pod IPs from the node's subnet so Pods are first-class VPC addresses.
13. New Pods cannot get an IP and stay Pending/ContainerCreating; the CNI logs show IP allocation failures. Add subnets/IPs or enable prefix delegation.
14. They need to pull images from ECR, reach the EKS API/STS, and reach other AWS services; NAT or VPC endpoints provide that path.
15. The subnet's route table (default route to NAT), then the NAT gateway state/subnet/EIP, then SG/NACL/DNS.
16. The NACL return path (ephemeral ports), the route back, and whether the server is listening; capture flow logs/tcpdump.
17. DNS resolution in the VPC; instances may not resolve public or private names, breaking package installs, AWS APIs, and service discovery.
18. It allows the instance to reach any internet destination, which can exfiltrate data or reach command-and-control; restrict destinations/ports.
19. The application, DNS, TLS, and whether the process is listening; it evaluates network configuration, not end-to-end service health.
20. The instance can be running but have a failed status check, no route, a blocking SG/NACL, or a dead application.
