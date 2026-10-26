# Phase 6 Labs — Part 1 (L6.1–L6.2)

> **Safety:** L6.1 is read-only. L6.2 creates a network, router, security group, keypair, VM, and floating IP. Use a sandbox project. Do not run this in a production tenant. Delete every resource at the end and prove it with CLI output.
>
> **If you have no OpenStack cloud:** do L6.1 against a public DevStack VM or a read-only API your instructor provides. For L6.2, use a disposable DevStack VM. Do not install DevStack on your daily laptop.

---

# Lab L6.1 — Map the Cloud Without Changing It

## Objective

Build an evidence-based map of an OpenStack cloud: identity scope, service catalog, endpoints, compute/network/volume agents, hypervisors, Placement providers, images, flavors, external networks, and volume types. Distinguish a control-plane failure from a data-plane failure using read-only commands.

## Prerequisites

- Access to an OpenStack project (sandbox) or a DevStack VM.
- `openstack` CLI installed (`python3-openstackclient`).
- An `openrc` or `clouds.yaml`; project-scoped `member` is enough for most commands.
- 30–40 minutes.

## Setup

```bash
openstack --version
source /path/to/openrc        # or: export OS_CLOUD=opslab
openstack token issue
```

If `token issue` fails, stop and fix authentication before continuing. Record the exact error and the values of `OS_AUTH_URL`, `OS_PROJECT_NAME`, `OS_USER_DOMAIN_NAME`, `OS_PROJECT_DOMAIN_NAME`, `OS_REGION_NAME`, and `OS_INTERFACE` (redact secrets).

Create a scratch directory for your evidence:

```bash
mkdir -p ~/opslab/phase6/l6.1
cd ~/opslab/phase6/l6.1
```

## Part A — Identity and catalog

```bash
openstack token issue -f yaml | tee token.yaml
openstack catalog list | tee catalog.txt
openstack endpoint list --interface public | tee endpoints-public.txt
openstack project list | tee projects.txt
openstack user list --domain Default | tee users.txt
openstack role assignment list --names | tee role-assignments.txt
openstack service list | tee services.txt
```

Expected observations:

- `token.yaml` has a `project_id`, `user_id`, `roles`, `expires_at`, and a `catalog`.
- `catalog.txt` lists at least `keystone`, `glance`, `nova`, `neutron`, `cinder`, and `placement`; a minimal cloud may omit some.
- `endpoints-public.txt` has a public URL per service/region.
- `role-assignments.txt` shows your user/group and role on the project. You should see `member` (or similar), not `admin`, unless you are an operator.

> **Do not paste `token.yaml` into your write-up.** It contains a bearer token. Redact `id` and any secret fields.

## Part B — Control-plane services

```bash
openstack compute service list | tee compute-services.txt
openstack network agent list | tee network-agents.txt
openstack volume service list | tee volume-services.txt
openstack hypervisor list | tee hypervisors.txt
openstack resource provider list | tee providers.txt
```

Expected observations:

- `nova-scheduler`, `nova-conductor`, and at least one `nova-compute` are `up`/`enabled`.
- `network agent list` shows DHCP, L3, OVS/OVN, metadata, or other agents; note which are `alive`/`UP`.
- `volume service list` shows `cinder-scheduler` and at least one `cinder-volume` backend.
- `hypervisor list` shows compute hosts; this may require admin. If you get 403, record that as an authorization boundary, not a cloud failure.
- `resource provider list` shows a provider per compute host (again, may require admin).

If a service list is empty, check whether the command needs admin scope before concluding the service is down. A 403 is evidence about your role, not about the service.

## Part C — Resources you can see

```bash
openstack image list --long | tee images.txt
openstack flavor list | tee flavors.txt
openstack network list | tee networks.txt
openstack network list --external | tee external-networks.txt
openstack subnet list | tee subnets.txt
openstack volume type list | tee volume-types.txt
openstack server list | tee servers.txt
openstack volume list | tee volumes.txt
openstack floating ip list | tee floating-ips.txt
```

Expected observations:

- At least one `active` image and one flavor small enough for your quota.
- At least one external network if the cloud offers floating IPs; record its name/ID.
- Volume types show the available backends; note whether any are encrypted or multi-attach capable.
- Existing servers/volumes belong to your project; do not touch them.

## Part D — Build the map

Fill this table from the command output (IDs can be truncated):

| Item | Value | Evidence command |
|---|---|---|
| Region | | `openstack endpoint list` |
| Project / project ID | | `openstack token issue` |
| Role(s) | | `openstack token issue` |
| Nova endpoint | | `openstack endpoint list --service nova` |
| Neutron endpoint | | `openstack endpoint list --service neutron` |
| Cinder endpoint | | `openstack endpoint list --service cinder` |
| Compute hosts | | `openstack hypervisor list` |
| Network agents up/down | | `openstack network agent list` |
| Volume backends | | `openstack volume service list` |
| External network | | `openstack network list --external` |
| Default image | | `openstack image list` |
| Smallest usable flavor | | `openstack flavor list` |
| Volume types | | `openstack volume type list` |

## Internal explanation

Keystone issues a project-scoped token with a catalog. Each service's endpoint is a URL; the CLI does not discover services by DNS. A service can be registered but have no healthy API process, and an agent can be down while the API is up. This lab separates three questions:

1. **Can I authenticate and see the service?** (Keystone/catalog)
2. **Is the control-plane service alive?** (API/agent heartbeat)
3. **Is there capacity/resource for my request?** (hypervisors/Placement/images/flavors)

Most "OpenStack is down" reports fail at one of these layers, not all three.

## Troubleshooting task

Do this in a **subshell** so you do not damage your main environment.

1. Reproduce an authentication failure with a deliberately wrong project:

```bash
(
  export OS_PROJECT_NAME=no-such-project
  openstack server list
)
```

Record the HTTP status and message. Is this 401 or 404? Why?

2. Reproduce an authorization failure by asking for an admin-only view as a non-admin:

```bash
openstack hypervisor list
```

If it succeeds, you are admin; use a non-admin project or skip and explain what a 403 would mean.

3. Pick one agent that is `down`/`disabled` (or explain what you would check if all are up). Name the data-plane symptom a tenant would see and the first two commands you would run.

**Do not fix the cloud.** The task is to classify the failure: authentication, authorization, endpoint, control-plane service, agent, or capacity.

## Verification

- [ ] `token issue` succeeds and shows the expected project and roles.
- [ ] You can name the public endpoint for Nova, Neutron, and Cinder.
- [ ] You can name at least one compute host and one network agent.
- [ ] You can name one external network, one image, one flavor, and one volume type.
- [ ] You can explain the difference between a 401 and a 403 you observed.
- [ ] You did not create or modify any resource.

## Cleanup

No cloud resources were created. Remove local evidence containing tokens:

```bash
shred -u token.yaml 2>/dev/null || rm -f token.yaml
unset OS_PROJECT_NAME   # if you exported it in the current shell
```

Keep the redacted map and command outputs for your write-up.

## Write-up template

```markdown
# L6.1 — Cloud map

- Date/cloud/region:
- Project/role:
- Auth method (openrc/clouds.yaml/application credential):

## Service map
| Service | Endpoint (redacted) | Interface | Region |
|---|---|---|---|

## Control plane
| Component | Host | State | Evidence |
|---|---|---|---|

## Resources
- External network:
- Image/flavor:
- Volume types:

## Failure classification
- Wrong project -> HTTP ___ because ...
- Admin-only command -> HTTP ___ because ...
- Agent down -> tenant symptom -> first two checks ...

## One thing I would ask the operator
```

---

# Lab L6.2 — Create a Network and a Reachable VM

## Objective

Create a tenant network, subnet, router with an external gateway, security group, keypair, and VM; attach a floating IP; prove reachability with ping/SSH and metadata; inspect the port binding and console log. Then break SSH with a security-group change and diagnose it from evidence.

## Prerequisites

- L6.1 completed; you know the external network, image, flavor, and project quota.
- A disposable project and a keypair public key on your machine.
- 45–60 minutes.
- If using Cirros, SSH key injection may vary; use the console as a fallback. If using Ubuntu, know the default user (`ubuntu`).

## Setup

```bash
source /path/to/openrc
openstack token issue
openstack quota show
openstack network list --external
openstack image list
openstack flavor list
openstack keypair list
```

Choose values and export them:

```bash
export EXTERNAL=$(openstack network list --external -f value -c Name | head -n1)
export IMAGE=$(openstack image list -f value -c Name | grep -Ei 'ubuntu|cirros' | head -n1)
export FLAVOR=$(openstack flavor list -f value -c Name | grep -E 'm1.small|m1.tiny' | head -n1)
echo "$EXTERNAL | $IMAGE | $FLAVOR"
```

If any variable is empty, choose explicit names from the lists. Do not guess an external network name; a tenant network is not an external network.

Create a local SSH key if needed:

```bash
test -f ~/.ssh/id_ed25519.pub || ssh-keygen -t ed25519 -N '' -f ~/.ssh/id_ed25519
```

## Part A — Network, subnet, router

```bash
openstack network create opslab-net
openstack subnet create opslab-subnet \
  --network opslab-net \
  --subnet-range 10.0.100.0/24 \
  --gateway 10.0.100.1 \
  --dns-nameserver 1.1.1.1

openstack router create opslab-router
openstack router add subnet opslab-router opslab-subnet
openstack router set opslab-router --external-gateway "$EXTERNAL"

openstack network show opslab-net -f yaml
openstack subnet show opslab-subnet -f yaml
openstack router show opslab-router -f yaml
```

Expected:

- Network `opslab-net` has a tenant network type (often `vxlan` or `geneve`) and an MTU.
- Subnet has `10.0.100.0/24`, gateway `10.0.100.1`, and an allocation pool.
- Router has an interface on `opslab-subnet` and `external_gateway_info` with the external network and an external IP.

If `router set --external-gateway` fails, you likely chose a tenant network or lack permission. Stop and fix that before creating the VM.

## Part B — Security group and keypair

```bash
openstack keypair create --public-key ~/.ssh/id_ed25519.pub opslab-key

openstack security group create opslab-sg
openstack security group rule create --proto icmp opslab-sg
openstack security group rule create --proto tcp --dst-port 22 \
  --remote-ip "$(curl -s https://ifconfig.me)/32" opslab-sg

openstack security group rule list opslab-sg
```

The `curl` gets your current public IP; if it fails, substitute your known IP. For a disposable lab you may use `0.0.0.0/0` on port 22, but understand the exposure and remove it during cleanup. Do not open 22 to the world on a shared/production cloud.

## Part C — Create the VM

```bash
export NET_ID=$(openstack network show opslab-net -f value -c id)
openstack server create \
  --flavor "$FLAVOR" \
  --image "$IMAGE" \
  --nic net-id="$NET_ID" \
  --key-name opslab-key \
  --security-group opslab-sg \
  --wait \
  opslab-vm
```

If `--wait` returns `ERROR`, stop and collect evidence before deleting:

```bash
openstack server show opslab-vm -f yaml
openstack server event list opslab-vm
openstack console log show opslab-vm
openstack port list --server opslab-vm
```

If it returns `ACTIVE`, add a floating IP:

```bash
export FIP=$(openstack floating ip create "$EXTERNAL" -f value -c floating_ip_address)
openstack server add floating ip opslab-vm "$FIP"
openstack floating ip list
echo "$FIP"
```

Expected:

- Server `status: ACTIVE`, `addresses: opslab-net=10.0.100.x`.
- Port `status: ACTIVE`, `binding:vif_type: ovs` or `ovn`, `binding:host_id` set.
- Floating IP associated with the server's fixed IP/port.
- `openstack console log show` shows a booting guest (kernel/cloud-init).

## Part D — Prove reachability

```bash
ping -c 3 "$FIP"
ssh -i ~/.ssh/id_ed25519 -o StrictHostKeyChecking=no ubuntu@"$FIP" 'hostname; ip -br addr; ip route'
```

For Cirros:

```bash
ssh cirros@"$FIP"     # password: gocubsgo (if your image/default allows it)
# or:
openstack console url show opslab-vm
```

Inside the guest:

```bash
ip addr
ip route
curl -s http://169.254.169.254/latest/meta-data/instance-id
curl -s http://169.254.169.254/latest/meta-data/local-ipv4
```

Expected: the guest sees its fixed IP (not the floating IP), a default route via `10.0.100.1`, and metadata for the instance. If metadata is empty, check the metadata agent/config drive; do not assume the VM is broken.

## Part E — Inspect the data path

On the client:

```bash
openstack port list --server opslab-vm
openstack port show <port-id> -f yaml
openstack floating ip show "$FIP" -f yaml
openstack router show opslab-router -f yaml
```

On a DevStack/controller host (if you have access):

```bash
ip netns list | grep -E 'qrouter|qdhcp'
sudo ip netns exec qrouter-<id> ip addr
sudo ip netns exec qrouter-<id> ip route
sudo ip netns exec qrouter-<id> iptables -t nat -L -n -v
sudo ovs-vsctl show
```

You do not need to read every flow. Identify the router namespace, the external interface, the DNAT rule for the floating IP, and the port's host binding.

## Internal explanation

The VM's fixed IP is in the tenant subnet. The router has an interface in that subnet and an external gateway on the provider network. Outbound traffic is SNATed to the router's external address; the floating IP adds a DNAT rule from the external address to the fixed IP. Security groups are enforced at the port, not in the guest. Metadata travels from the guest to `169.254.169.254`, through the router/DHCP namespace, to the metadata agent and Nova.

`ACTIVE` means libvirt started the domain and the port was bound. Reachability additionally requires the router, floating IP, security group, guest route, and guest sshd. That is why a VM can be `ACTIVE` and still time out.

## Troubleshooting task

Do this in order. After each change, collect evidence before undoing it.

1. **Remove the SSH rule** (keep ICMP):

```bash
openstack security group rule list opslab-sg
# Identify the tcp/22 rule ID, then:
openstack security group rule delete <ssh-rule-id>
ssh -i ~/.ssh/id_ed25519 ubuntu@"$FIP" true
```

Expected: SSH times out. Ping may still work. Evidence: `openstack security group rule list`, guest `ss -lntp` via console, and the fact that ICMP still works. This proves the port/host path is alive but TCP/22 is filtered.

2. **Restore the rule**:

```bash
openstack security group rule create --proto tcp --dst-port 22 \
  --remote-ip "$(curl -s https://ifconfig.me)/32" opslab-sg
ssh -i ~/.ssh/id_ed25519 ubuntu@"$FIP" 'echo restored'
```

3. **Diagnose a floating IP that is allocated but not associated** (do not delete the working one):

```bash
export FIP2=$(openstack floating ip create "$EXTERNAL" -f value -c floating_ip_address)
openstack floating ip show "$FIP2"
ping -c 2 "$FIP2"       # expected: timeout/no association
openstack server add floating ip opslab-vm "$FIP2"
ping -c 2 "$FIP2"
openstack floating ip delete "$FIP2"
```

Record why the first ping failed and what changed after association. Do not leave `FIP2` allocated.

4. **Optional:** if you have console access, stop `sshd` in the guest and observe `Connection refused` vs timeout. Explain the difference: a refused connection means the packet reached the host but no process listened; a timeout suggests filtering/routing.

## Verification

- [ ] Server is `ACTIVE`; port is bound to a host.
- [ ] Floating IP is associated with the correct fixed IP/port.
- [ ] Ping and SSH work (or console login works for Cirros).
- [ ] The guest has a default route via the subnet gateway.
- [ ] Metadata returns the instance ID (or you can explain config drive).
- [ ] Removing/restoring the SSH rule reproduces and fixes the timeout.
- [ ] You can name the DNAT/SNAT path and the router namespace.

## Cleanup

Delete in this order and wait between steps:

```bash
openstack server delete --wait opslab-vm
openstack floating ip delete "$FIP"

openstack router unset --external-gateway opslab-router
openstack router remove subnet opslab-router opslab-subnet
openstack router delete opslab-router

openstack subnet delete opslab-subnet
openstack network delete opslab-net

openstack security group delete opslab-sg
openstack keypair delete opslab-key
```

Check:

```bash
openstack server list --name opslab-vm
openstack floating ip list
openstack router list
openstack network list
openstack security group list --name opslab-sg
openstack keypair list
```

If a delete fails because a port is still attached, wait for the server delete to complete, then retry. Do not delete the external network or a shared image/flavor.

## Write-up template

```markdown
# L6.2 — Network and VM

- Cloud/region/project:
- External network:
- Image/flavor:
- Tenant CIDR:
- Fixed IP / floating IP:

## Evidence
| Check | Command | Result |
|---|---|---|
| Server status | server show | |
| Port binding | port show | |
| Router gateway | router show | |
| Floating IP | floating ip show | |
| Guest route | ip route | |
| Metadata | curl 169.254.169.254 | |

## Packet path
client -> floating IP -> router DNAT -> fixed IP -> port -> tap -> guest
guest -> router SNAT -> external network -> client

## Troubleshooting
- Removed SSH rule: symptom ___; evidence ___; fix ___
- Unassociated floating IP: symptom ___; evidence ___; fix ___
- Refused vs timeout: explanation ___

## Cleanup proof
(paste the empty list outputs)
```
