# Phase 2 Labs — Part 1 (L2.1–L2.4)

> **How to use these labs:** do them in order on a disposable Linux VM. Read the objective, predict the result, run the commands, then compare. If the result surprises you, stop and explain it before moving on. Every lab ends with a deliberate failure you must diagnose from evidence.

**Environment:** Ubuntu 22.04/24.04 VM (or Rocky 9) with `iproute2`, `tcpdump`, `dnsutils`, `curl`, `openssl`, `python3`, `nftables`/`iptables`, `conntrack`, and `dnsmasq`. Root/sudo required. Snapshot the VM before L2.2 and L2.5; network changes can lock you out.

**Safety:** never run these labs on a machine you reach only over SSH without console access. Prefer a local VM with a host-only adapter.

---

# Lab L2.1 — Subnetting and VLSM Design

## Objective

Design a complete IPv4 addressing plan for a small AI lab from a single `/22`, without a calculator, and prove it with `ipcalc`. Detect overlap and misalignment in a broken plan.

**Skills:** binary/prefix arithmetic, VLSM, alignment, overlap detection, documentation.

## Prerequisites

- [IP addressing and subnetting](01-ip-addressing-and-subnetting.md), sections 1–6.
- Paper, pencil, and no subnet calculator for the first pass.

## Setup

On the Linux VM:

```bash
sudo apt update && sudo apt install -y ipcalc
ipcalc --version
```

```text
ipcalc 1.0.2
```

`ipcalc` is only for verification. Do the design by hand first.

## Scenario

You are given `10.20.0.0/22` for a lab. Requirements:

| Segment | Hosts needed | Notes |
|---|---:|---|
| `users` | 400 | Workstations, DHCP |
| `servers` | 100 | Static, no DHCP |
| `gpu` | 50 | Static, jumbo frames later |
| `mgmt` | 20 | Out-of-band, restricted |
| `p2p` | 2 | Router-to-router link |
| `guest` | 25 | Captive portal, isolated |

Allocate largest-first, keep every subnet aligned, and leave the remaining space contiguous for future use.

## Steps

1. Write the `/22` range: network, broadcast, usable.
2. Choose a prefix for each requirement. Remember usable = addresses − 2.
3. Allocate in descending size order. For each, write network, broadcast, usable range, and the next free address.
4. Fill the table:

| Segment | Prefix | Network | Broadcast | Usable range |
|---|---|---|---|---|
| users | | | | |
| servers | | | | |
| gpu | | | | |
| mgmt | | | | |
| p2p | | | | |
| guest | | | | |
| free | | | | |

5. Verify each allocation:

```bash
ipcalc 10.20.0.0/23
ipcalc 10.20.2.0/25
ipcalc 10.20.2.128/26
ipcalc 10.20.2.192/27
ipcalc 10.20.2.224/30
```

6. Prove no overlap with a small script:

```bash
python3 - <<'PY'
import ipaddress
nets = [
    "10.20.0.0/23",
    "10.20.2.0/25",
    "10.20.2.128/26",
    "10.20.2.192/27",
    "10.20.2.224/30",
]
parsed = [ipaddress.ip_network(n) for n in nets]
for i, a in enumerate(parsed):
    for b in parsed[i+1:]:
        if a.overlaps(b):
            print("OVERLAP", a, b)
print("checked", len(parsed), "subnets")
PY
```

```text
checked 5 subnets
```

No `OVERLAP` lines means the plan is valid.

7. Check alignment. A subnet is valid only if its network address is a multiple of its block size. For `10.20.2.224/30`, block size 4, `224 % 4 == 0` → valid.

## Expected observations

A correct plan for the six requirements:

| Segment | Prefix | Network | Broadcast | Usable range |
|---|---|---|---|---|
| users | /23 | 10.20.0.0 | 10.20.1.255 | 10.20.0.1–10.20.1.254 |
| servers | /25 | 10.20.2.0 | 10.20.2.127 | 10.20.2.1–10.20.2.126 |
| gpu | /26 | 10.20.2.128 | 10.20.2.191 | 10.20.2.129–10.20.2.190 |
| mgmt | /27 | 10.20.2.192 | 10.20.2.223 | 10.20.2.193–10.20.2.222 |
| p2p | /30 | 10.20.2.224 | 10.20.2.227 | 10.20.2.225–10.20.2.226 |

The next free address is `.228`, but `10.20.2.228/27` is **invalid**: a `/27` boundary must be a multiple of 32, and 228 is not. The next aligned `/27` boundary is `10.20.3.0` (`.192` and `.224` are /27 boundaries, but `.224/27` would overlap the `/30`). This is the alignment trap the lab is designed to expose. `ipcalc 10.20.2.228/27` silently normalizes to `10.20.2.224/27`, which overlaps the p2p link — a calculator can hide the mistake if you do not read the network address it prints.

Correct final table:

| Segment | Prefix | Network | Broadcast | Usable range |
|---|---|---|---|---|
| users | /23 | 10.20.0.0 | 10.20.1.255 | 10.20.0.1–10.20.1.254 |
| servers | /25 | 10.20.2.0 | 10.20.2.127 | 10.20.2.1–10.20.2.126 |
| gpu | /26 | 10.20.2.128 | 10.20.2.191 | 10.20.2.129–10.20.2.190 |
| mgmt | /27 | 10.20.2.192 | 10.20.2.223 | 10.20.2.193–10.20.2.222 |
| p2p | /30 | 10.20.2.224 | 10.20.2.227 | 10.20.2.225–10.20.2.226 |
| guest | /27 | 10.20.3.0 | 10.20.3.31 | 10.20.3.1–10.20.3.30 |
| free | — | 10.20.3.32 | 10.20.3.255 | 10.20.3.33–10.20.3.254 |

The `free` block is not a single power-of-two subnet; document it as reserved space, not as a subnet.

## Why it works

- `/22` = 4 × `/24` = 1024 addresses, from `10.20.0.0` to `10.20.3.255`.
- 400 hosts needs 9 host bits (`2^9 − 2 = 510`), so `/23` (32 − 9 = 23).
- 100 hosts needs 7 bits (`126`), `/25`.
- 50 hosts needs 6 bits (`62`), `/26`.
- 20 hosts needs 5 bits (`30`), `/27`.
- 2 hosts needs 2 bits (`2`), `/30`.
- 25 hosts needs 5 bits (`30`), `/27`.
- Allocating largest-first avoids fragmentation. Every network address must be a multiple of its block size.

## Troubleshooting task

A colleague proposes:

```text
users   10.20.0.0/24
servers 10.20.1.0/24
gpu     10.20.2.0/24
mgmt    10.20.2.128/25
guest   10.20.3.0/24
p2p     10.20.3.252/30
```

Without a calculator, find every problem:

1. Which subnets overlap?
2. Which allocation is invalid due to alignment?
3. Which segment is under-sized?
4. What is the smallest set of changes that fixes it while keeping `users` at `10.20.0.0/24`?

Write the answers before checking.

### Expected findings

- `gpu 10.20.2.0/24` overlaps `mgmt 10.20.2.128/25`.
- `p2p 10.20.3.252/30` is aligned (252 % 4 == 0), but it sits inside `guest 10.20.3.0/24` → overlap.
- `users /24` gives 254 usable, less than 400 → under-sized.
- `mgmt /25` gives 126, more than needed but not a correctness error; it wastes space.
- Fix: keep users at `10.20.0.0/23` (needs the second /24), move mgmt to a smaller aligned block, and put p2p outside guest.

The point is not the exact redesign; it is that overlap and alignment are arithmetic facts you can prove.

## Verification

```bash
python3 - <<'PY'
import ipaddress
plan = {
    "users":   "10.20.0.0/23",
    "servers": "10.20.2.0/25",
    "gpu":     "10.20.2.128/26",
    "mgmt":    "10.20.2.192/27",
    "p2p":     "10.20.2.224/30",
    "guest":   "10.20.3.0/27",
}
nets = {k: ipaddress.ip_network(v) for k, v in plan.items()}
for k, n in nets.items():
    print(f"{k:8} {n} hosts={n.num_addresses-2}")
for a in nets:
    for b in nets:
        if a < b and nets[a].overlaps(nets[b]):
            print("OVERLAP", a, b)
print("supernet contains all:", ipaddress.ip_network("10.20.0.0/22").supernet_of(
    ipaddress.ip_network("10.20.3.0/27")))
PY
```

Expected: six lines, no `OVERLAP`, and `supernet contains all: True`. (The `supernet_of` check only verifies the last subnet; extend it to all if you want a stronger check.)

## Cleanup

No system changes. Keep the table and the overlap script in your notes.

## Write-up template

```markdown
## L2.1 — Subnetting and VLSM

**Given:** 10.20.0.0/22
**Requirements:** users 400, servers 100, gpu 50, mgmt 20, p2p 2, guest 25

| Segment | Prefix | Network | Broadcast | Usable | Why this size |
|---|---|---|---|---|---|
| ... | ... | ... | ... | ... | ... |

**Alignment check:** ...
**Overlap check:** command + result
**Mistake I made:** ...
**Rule I will remember:** largest-first, align to block size, verify with overlap script.
```

---

# Lab L2.2 — Ethernet, VLANs, and Inter-VLAN Routing

## Objective

Build a four-host, two-VLAN topology with a router-on-a-stick using Linux network namespaces and a VLAN-filtering bridge. Prove:

- same-VLAN hosts communicate directly at L2;
- different-VLAN hosts require the router;
- the trunk carries 802.1Q tags;
- the bridge MAC table learns and moves entries;
- removing a VLAN from the trunk breaks only that VLAN.

**Skills:** veth, bridge, 802.1Q, access/trunk ports, ARP, `bridge fdb`, `tcpdump -e`, IP forwarding.

## Prerequisites

- [Ethernet, ARP, switching, VLANs](02-ethernet-arp-switching-vlans.md).
- [Routing and Linux networking](03-routing-and-linux-networking.md), sections 1–4.
- Root access; `tcpdump`, `iproute2`, `bridge` (part of iproute2).

## Setup

All in one VM. Use a scratch directory for scripts.

```bash
sudo ip netns add h10a
sudo ip netns add h10b
sudo ip netns add h20a
sudo ip netns add router

sudo ip link add br0 type bridge vlan_filtering 1
sudo ip link set br0 up
```

Create veth pairs and attach them:

```bash
# VLAN 10 access ports
sudo ip link add v10a type veth peer name v10a-br
sudo ip link set v10a netns h10a
sudo ip link set v10a-br master br0
sudo ip link set v10a-br up

sudo ip link add v10b type veth peer name v10b-br
sudo ip link set v10b netns h10b
sudo ip link set v10b-br master br0
sudo ip link set v10b-br up

# VLAN 20 access port
sudo ip link add v20a type veth peer name v20a-br
sudo ip link set v20a netns h20a
sudo ip link set v20a-br master br0
sudo ip link set v20a-br up

# Trunk to the router
sudo ip link add vtrunk type veth peer name vtrunk-br
sudo ip link set vtrunk netns router
sudo ip link set vtrunk-br master br0
sudo ip link set vtrunk-br up
```

Configure bridge VLAN membership:

```bash
sudo bridge vlan add dev v10a-br vid 10 pvid untagged
sudo bridge vlan add dev v10b-br vid 10 pvid untagged
sudo bridge vlan add dev v20a-br vid 20 pvid untagged

# Trunk: tagged VLANs 10 and 20
sudo bridge vlan add dev vtrunk-br vid 10
sudo bridge vlan add dev vtrunk-br vid 20
```

Check:

```bash
sudo bridge vlan show
```

Expected:

```text
port    vlan-id
br0     1 PVID Egress Untagged
v10a-br 10 PVID Egress Untagged
v10b-br 10 PVID Egress Untagged
v20a-br 20 PVID Egress Untagged
vtrunk-br 1 PVID Egress Untagged
vtrunk-br 10
vtrunk-br 20
```

Configure hosts:

```bash
sudo ip -n h10a link set lo up
sudo ip -n h10a addr add 10.10.10.11/24 dev v10a
sudo ip -n h10a link set v10a up
sudo ip -n h10a route add default via 10.10.10.1

sudo ip -n h10b link set lo up
sudo ip -n h10b addr add 10.10.10.12/24 dev v10b
sudo ip -n h10b link set v10b up
sudo ip -n h10b route add default via 10.10.10.1

sudo ip -n h20a link set lo up
sudo ip -n h20a addr add 10.20.20.11/24 dev v20a
sudo ip -n h20a link set v20a up
sudo ip -n h20a route add default via 10.20.20.1
```

Configure the router (router-on-a-stick):

```bash
sudo ip -n router link set lo up
sudo ip -n router link set vtrunk up

sudo ip -n router link add link vtrunk name vtrunk.10 type vlan id 10
sudo ip -n router link add link vtrunk name vtrunk.20 type vlan id 20
sudo ip -n router link set vtrunk.10 up
sudo ip -n router link set vtrunk.20 up

sudo ip -n router addr add 10.10.10.1/24 dev vtrunk.10
sudo ip -n router addr add 10.20.20.1/24 dev vtrunk.20
sudo ip netns exec router sysctl -w net.ipv4.ip_forward=1
```

Expected:

```text
net.ipv4.ip_forward = 1
```

## Steps and expected observations

### 1. Same VLAN, direct L2

```bash
sudo ip netns exec h10a ping -c 3 10.10.10.12
```

Expected:

```text
3 packets transmitted, 3 received, 0% packet loss
```

Check ARP:

```bash
sudo ip netns exec h10a ip neigh show
```

Expected: `10.10.10.12 ... REACHABLE` or `STALE`.

Check the bridge MAC table:

```bash
sudo bridge fdb show | grep -E 'v10a|v10b'
```

Expected: the h10a MAC on `v10a-br` with `vlan 10`, the h10b MAC on `v10b-br` with `vlan 10`.

### 2. Different VLANs, routed

```bash
sudo ip netns exec h10a ping -c 3 10.20.20.11
sudo ip netns exec h10a traceroute -n 10.20.20.11
```

Expected: ping succeeds; traceroute shows one hop, `10.10.10.1`.

Check the router's neighbour table:

```bash
sudo ip netns exec router ip neigh show
```

Expected entries for both `10.10.10.11` and `10.20.20.11`.

### 3. See the tags

Capture on the trunk inside the router namespace:

```bash
sudo ip netns exec router timeout 10 tcpdump -eni vtrunk vlan
```

In another terminal:

```bash
sudo ip netns exec h10a ping -c 2 10.20.20.11
```

Expected: frames with `vlan 10` and `vlan 20`, and ARP requests tagged accordingly.

### 4. ARP is link-local

Flush the cache and ping across VLANs:

```bash
sudo ip netns exec h10a ip neigh flush dev v10a
sudo ip netns exec h10a ping -c 1 10.20.20.11
sudo ip netns exec h10a ip neigh show
```

Expected: the only neighbour is `10.10.10.1` (the gateway). h10a never ARPs for `10.20.20.11`; it ARPs for its gateway. This is the single most important L2/L3 distinction to explain in interviews.

### 5. MAC learning and flooding

Ping an unused address in the same VLAN:

```bash
sudo ip netns exec h10a ping -c 1 -W 1 10.10.10.99
```

Capture on h10b:

```bash
sudo ip netns exec h10b timeout 5 tcpdump -eni v10b arp
```

Expected: h10b sees the ARP request for `10.10.10.99` because it is broadcast/flooded, but no reply. The switch does not know where `.99` is, so it floods.

## Why it works

- Each veth pair is a virtual cable. The bridge forwards between ports like a switch.
- `vlan_filtering 1` makes the bridge VLAN-aware. An access port has a PVID; untagged ingress is classified into that VLAN, and egress is untagged.
- A trunk port carries tagged frames for the allowed VLANs. The router's `vtrunk.10`/`vtrunk.20` interfaces add/remove tags.
- Same-VLAN traffic is switched at L2; different-VLAN traffic must go to the router, which strips the tag, routes, and re-tags on egress.
- ARP is broadcast within a VLAN and never crosses a router.

## Troubleshooting task

Break it in three ways, one at a time. Diagnose from evidence before fixing.

### Break A — remove VLAN 20 from the trunk

```bash
sudo bridge vlan del dev vtrunk-br vid 20
sudo ip netns exec h10a ping -c 2 10.20.20.11
```

Expected: 100% loss. Same-VLAN ping still works. Capture on the trunk: VLAN 20 frames leave the bridge? No — the bridge drops them because the port is not a member. Capture on `v20a-br`: the ARP request from h10a never arrives.

Restore:

```bash
sudo bridge vlan add dev vtrunk-br vid 20
```

### Break B — put h20a in VLAN 10

```bash
sudo bridge vlan add dev v20a-br vid 10 pvid untagged
sudo bridge vlan del dev v20a-br vid 20
sudo ip netns exec h20a ip addr flush dev v20a
sudo ip netns exec h20a ip addr add 10.20.20.11/24 dev v20a
sudo ip netns exec h20a ip neigh flush dev v20a
```

Now h20a is in VLAN 10 but still has a 10.20.20.11/24 address. It ARPs for its gateway, but the gateway is in VLAN 20; the ARP goes to VLAN 10 hosts and nobody answers. Capture on `v20a-br` shows untagged ARP, but the router never sees it. This is the classic "IP is right, VLAN is wrong" outage.

Restore VLAN 20:

```bash
sudo bridge vlan add dev v20a-br vid 20 pvid untagged
sudo bridge vlan del dev v20a-br vid 10
sudo ip netns exec h20a ip neigh flush dev v20a
```

### Break C — disable forwarding

```bash
sudo ip netns exec router sysctl -w net.ipv4.ip_forward=0
sudo ip netns exec h10a ping -c 2 10.20.20.11
```

Expected: same-VLAN works, cross-VLAN fails. The router receives the packet, but the kernel drops it because forwarding is off. `tcpdump` on `vtrunk.10` shows the request arriving; `vtrunk.20` shows nothing leaving.

Restore:

```bash
sudo ip netns exec router sysctl -w net.ipv4.ip_forward=1
```

## Verification

```bash
sudo ip netns exec h10a ping -c 2 10.10.10.12
sudo ip netns exec h10a ping -c 2 10.20.20.11
sudo ip netns exec h20a ping -c 2 10.10.10.11
sudo bridge vlan show
sudo bridge fdb show | grep -E 'v10|v20|vtrunk'
```

Expected: all pings 0% loss; VLAN table shows 10 on `v10a-br`/`v10b-br`, 20 on `v20a-br`, both tagged on `vtrunk-br`; FDB entries show the correct port and VLAN.

## Cleanup

```bash
sudo ip netns del h10a
sudo ip netns del h10b
sudo ip netns del h20a
sudo ip netns del router
sudo ip link del br0
```

Deleting a namespace deletes its veth endpoints; deleting the bridge removes the remaining ports. Verify:

```bash
ip netns list
ip -br link show type bridge
```

## Write-up template

```markdown
## L2.2 — VLANs and inter-VLAN routing

**Topology:** (ASCII diagram with VLANs, IPs, trunk)
**Access ports:** ...
**Trunk VLANs:** ...
**Evidence same-VLAN:** ping + `bridge fdb`
**Evidence cross-VLAN:** ping + traceroute + router neigh
**Evidence tags:** tcpdump line showing `vlan 10`
**Break A root cause:** ...
**Break B root cause:** ...
**Break C root cause:** ...
**One sentence I can say in an interview:** "Same subnet = ARP for the host; different subnet = ARP for the gateway; VLANs decide whether that ARP is visible."
```

---

# Lab L2.3 — TCP Sockets, States, Backlog, and TIME_WAIT

## Objective

Observe a TCP connection from listener to teardown, fill an accept queue, create `CLOSE_WAIT`, generate `TIME_WAIT`, and distinguish refused from timeout. Read each state in `ss` and explain what the kernel is waiting for.

**Skills:** `ss`, `tcpdump`, sockets, backlog, state machine, ephemeral ports, MTU.

## Prerequisites

- [TCP, UDP, sockets](04-tcp-udp-sockets.md), sections 1–8.
- `nc`, `ss`, `tcpdump`, `python3`.

## Setup

```bash
sudo apt update && sudo apt install -y netcat-openbsd tcpdump iproute2 python3 traceroute
```

Create a working directory:

```bash
mkdir -p ~/labs/l2.3 && cd ~/labs/l2.3
```

## Steps and expected observations

### 1. Start a listener and inspect it

```bash
nc -l 9000
```

In another terminal:

```bash
ss -lntp | grep 9000
```

Expected:

```text
LISTEN 0 1 0.0.0.0:9000 0.0.0.0:* users:(("nc",pid=...,fd=3))
```

`Send-Q` for a listening socket is the backlog (netcat uses 1). `Recv-Q` is 0 because no connection is waiting to be accepted.

### 2. Capture the handshake

```bash
sudo tcpdump -ni lo -c 3 'tcp port 9000'
```

Connect from another terminal:

```bash
nc 127.0.0.1 9000
```

Expected: SYN, SYN-ACK, ACK. Type a line; it appears in the listener.

Close the client with Ctrl-D. Capture the FIN/ACK exchange. `ss -tan | grep 9000` may briefly show `TIME-WAIT` on the client side.

### 3. Inspect internals

With a connection open:

```bash
ss -tin dst 127.0.0.1
```

Expected fields: `cubic`, `rtt`, `cwnd`, `mss:65483` (loopback MTU 65536), `retrans:0/0`.

```bash
ss -tan state established '( sport = :9000 or dport = :9000 )'
```

### 4. Fill the accept queue

Create a server that listens with `backlog=1` and never calls `accept()`:

```bash
cat > backlog_server.py <<'PY'
import socket, time
s = socket.socket()
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(("127.0.0.1", 9001))
s.listen(1)
print("listening with backlog=1; not accepting", flush=True)
time.sleep(300)
PY
python3 backlog_server.py &
```

Open several clients in the background:

```bash
for i in 1 2 3 4 5; do nc -vz 127.0.0.1 9001 & done
wait
```

Expected: some connections succeed (the kernel completes the handshake and queues them), then later ones hang or are dropped. Inspect:

```bash
ss -lnt | grep 9001
```

Expected:

```text
LISTEN 2 1 127.0.0.1:9001 0.0.0.0:*
```

`Recv-Q=2` means two completed connections are waiting to be accepted; `Send-Q=1` is the backlog. Linux may allow backlog+1, so 2 is normal.

Check the drop counters:

```bash
nstat -az | grep -E 'ListenOverflows|ListenDrops'
```

Expected: values increase when the queue overflows. If the counter does not move, the clients are still in the SYN queue rather than the accept queue.

Kill the server:

```bash
kill %1
```

The queued clients receive RST because the listening socket closed with unaccepted connections.

### 5. Create CLOSE_WAIT

Create a server that accepts, reads, then sleeps without closing:

```bash
cat > closewait_server.py <<'PY'
import socket, time
s = socket.socket()
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(("127.0.0.1", 9002))
s.listen(5)
conn, addr = s.accept()
print("accepted; reading one byte", flush=True)
conn.recv(1)
print("peer closed; sleeping without close()", flush=True)
time.sleep(300)
PY
python3 closewait_server.py &
```

Connect and close from the client:

```bash
printf 'x' | nc 127.0.0.1 9002
```

Inspect:

```bash
ss -tan state close-wait '( sport = :9002 or dport = :9002 )'
```

Expected:

```text
CLOSE-WAIT 0 0 127.0.0.1:9002 127.0.0.1:... 
```

The peer sent FIN; the server's process still holds the socket open. This is an application bug, not a kernel bug.

Kill the server:

```bash
kill %1
```

The socket moves to `LAST_ACK`/`CLOSED` as the kernel closes it.

### 6. Generate TIME_WAIT

```bash
python3 -m http.server 9003 >/dev/null 2>&1 &
for i in $(seq 1 20); do curl -s -o /dev/null http://127.0.0.1:9003/; done
ss -tan state time-wait | grep 9003 | wc -l
```

Expected: ~20 `TIME-WAIT` entries, each lasting 60 seconds. Which side holds them depends on who closed first. Python's `http.server` speaks HTTP/1.0 and typically closes the connection server-side, so the server may own most of them; a keep-alive client that closes first will show them on the client. Check both with `ss -tan state time-wait | grep 9003`.

Check the ephemeral range:

```bash
sysctl net.ipv4.ip_local_port_range
```

### 7. Refused vs timeout

Refused:

```bash
nc -vz 127.0.0.1 9999
```

Expected: `Connection refused` and an immediate RST in `tcpdump`.

Timeout requires a drop. Use a local nftables rule:

```bash
sudo nft add table ip lab
sudo nft 'add chain ip lab output { type filter hook output priority 0; policy accept; }'
sudo nft add rule ip lab output tcp dport 9998 drop
time nc -vz -w 3 127.0.0.1 9998
```

Expected: timeout after 3 seconds, no RST. Remove the rule:

```bash
sudo nft delete table ip lab
```

### 8. MTU (disposable VM only)

This changes the VM's interface MTU. Do it only if you have console access or a second management path.

```bash
ip link show eth0 | grep mtu
sudo ip link set eth0 mtu 1400
ping -M do -s 1472 -c 1 8.8.8.8
ping -M do -s 1372 -c 1 8.8.8.8
sudo ip link set eth0 mtu 1500
```

Expected: the 1472-byte ping fails (`Message too long` or no reply), the 1372-byte ping succeeds. Restore 1500 immediately.

If you cannot risk the management NIC, use a veth pair with MTU 1400 in namespaces; the principle is the same.

## Why it works

- `listen(fd, backlog)` creates SYN and accept queues. The kernel completes handshakes even if the app never accepts, up to the queue limit.
- `Recv-Q` on a LISTEN socket is the accept queue depth; `Send-Q` is the backlog.
- `CLOSE_WAIT` means the peer closed and the local app has not. Only the app can fix it.
- `TIME_WAIT` is the closer's state for 60 s; it allows re-ACKing a lost FIN and prevents old duplicates from entering a new connection.
- RST is generated for a closed port, a closed socket with unread data, or an explicit reset. A DROP rule produces a timeout instead.

## Troubleshooting task

Given these observations, name the fault domain for each:

1. Client shows `SYN-SENT`; server capture shows SYN arriving but no SYN-ACK leaving. Server `ss -lnt` shows `Recv-Q=0 Send-Q=1` and `ListenOverflows` is rising.
2. Client shows `ESTABLISHED`; `ss -tan` on the server shows thousands of `CLOSE_WAIT`.
3. Client shows `TIME-WAIT`; new outbound connections fail with `Cannot assign requested address`.
4. `ping` works, `curl` to a large file stalls after the handshake.
5. Client gets `Connection refused`; `ss -lntp` on the server shows the port bound to `127.0.0.1`, not `0.0.0.0`.

### Expected answers

1. Accept queue overflow: the app is not accepting (or is too slow). The SYN queue completed the handshake, but the accept queue is full. Fix the accept loop/backlog.
2. Application socket leak: the app is not closing connections after the peer closes.
3. Ephemeral port exhaustion from short-lived connections; use pooling/keep-alive, not just a wider range.
4. Path MTU/PMTUD black hole; test `ping -M do` sizes and `tracepath`.
5. Bind address: loopback only. The client's SYN reaches the host but no socket matches; the kernel sends RST.

## Verification

Produce a table:

| State | How you created it | Command that proved it | What it means |
|---|---|---|---|
| LISTEN | `nc -l` | `ss -lntp` | Waiting for connections |
| SYN-SENT | drop SYN-ACK | `ss -tan state syn-sent` | No reply yet |
| ESTABLISHED | `nc` connect | `ss -tan state established` | Data can flow |
| CLOSE-WAIT | server sleeps after peer FIN | `ss -tan state close-wait` | App has not closed |
| TIME-WAIT | client closes first | `ss -tan state time-wait` | 2×MSL wait |
| RST | connect to closed port | `tcpdump` | Immediate abort |

## Cleanup

```bash
kill %1 %2 2>/dev/null
pkill -f backlog_server.py
pkill -f closewait_server.py
pkill -f 'http.server 9003'
rm -f ~/labs/l2.3/*.py
```

## Write-up template

```markdown
## L2.3 — TCP states

**Handshake capture:** (3 lines, flags/seq/ack)
**Backlog evidence:** `ss -lnt` before/after + `ListenOverflows`
**CLOSE_WAIT evidence:** ...
**TIME_WAIT count:** ...
**Refused vs timeout:** (RST vs no response)
**MTU result:** ...
**State I misread at first:** ...
```

---

# Lab L2.4 — DNS Resolution, Caching, and DHCP

## Objective

Run a local authoritative/forwarding DNS server, create A/CNAME/TXT/SRV records, observe NXDOMAIN and negative caching, compare `dig` with `getent`, and observe a DHCP DORA exchange on an isolated veth.

**Skills:** `dnsmasq`, `dig`, `getent`, `resolvectl`, record types, TTLs, search domains, DHCP relay concepts.

## Prerequisites

- [DNS and DHCP](05-dns-and-dhcp.md).
- `dnsmasq`, `dnsutils`, `tcpdump`, `busybox` (for `udhcpc`), `iproute2`.

## Setup

```bash
sudo apt update && sudo apt install -y dnsmasq dnsutils tcpdump busybox iproute2
mkdir -p ~/labs/l2.4 && cd ~/labs/l2.4
```

Check that system DNS is not already using port 53; we will use 5353/5354 to avoid conflicts.

## Part A — DNS records and caching

Create two configs to simulate split-horizon:

```bash
cat > dns-a.conf <<'EOF'
port=5353
listen-address=127.0.0.1
bind-interfaces
no-resolv
no-hosts
domain-needed
local=/lab.test/
address=/app.lab.test/10.10.10.50
address=/api.lab.test/10.10.10.60
cname=www.lab.test,app.lab.test
txt-record=lab.test,owner=infra
srv-host=_https._tcp.lab.test,app.lab.test,443,10,5
EOF

cat > dns-b.conf <<'EOF'
port=5354
listen-address=127.0.0.1
bind-interfaces
no-resolv
no-hosts
local=/lab.test/
address=/app.lab.test/203.0.113.50
EOF
```

Start both:

```bash
sudo dnsmasq --no-daemon --conf-file=$PWD/dns-a.conf &
sudo dnsmasq --no-daemon --conf-file=$PWD/dns-b.conf &
```

If dnsmasq complains about user privileges, add `user=root` to the config for the lab only, or run as root (it is already sudo).

### 1. Query records

```bash
dig @127.0.0.1 -p 5353 app.lab.test A
dig @127.0.0.1 -p 5353 www.lab.test A
dig @127.0.0.1 -p 5353 lab.test TXT
dig @127.0.0.1 -p 5353 _https._tcp.lab.test SRV
```

Expected: `NOERROR`, the A record `10.10.10.50`, the CNAME chain for `www`, the TXT string, and the SRV target/port.

### 2. NXDOMAIN and negative caching

```bash
dig @127.0.0.1 -p 5353 missing.lab.test A
```

Expected: `status: NXDOMAIN`, authority section with SOA. Query again and compare `Query time` (second may be 0 msec, served from cache).

### 3. Split horizon

```bash
dig @127.0.0.1 -p 5353 +short app.lab.test
dig @127.0.0.1 -p 5354 +short app.lab.test
```

Expected: `10.10.10.50` vs `203.0.113.50`. Same name, different answers depending on which resolver you ask. This is why "it resolves for me" is not evidence.

### 4. `dig` vs `getent`

Add a hosts entry:

```bash
echo '10.10.10.99 hosts-only.lab.test' | sudo tee -a /etc/hosts
getent hosts hosts-only.lab.test
dig @127.0.0.1 -p 5353 +short hosts-only.lab.test
```

Expected: `getent` returns `10.10.10.99`; `dig` returns nothing/NXDOMAIN. `getent` uses NSS and sees `/etc/hosts`; `dig` asks DNS directly.

Remove the entry afterward:

```bash
sudo sed -i '/hosts-only.lab.test/d' /etc/hosts
```

### 5. Observe the query on the wire

```bash
sudo tcpdump -ni lo -c 4 'udp port 5353'
```

In another terminal:

```bash
dig @127.0.0.1 -p 5353 app.lab.test
```

Expected: query with a transaction ID and `A?`, response with `1/0/0` and the address. Note whether recursion desired (`+`) is set.

## Part B — DHCP DORA on an isolated veth

This part uses network namespaces so it cannot affect the VM's real DHCP.

```bash
sudo ip netns add dhcp-srv
sudo ip netns add dhcp-cli

sudo ip link add veth-s type veth peer name veth-c
sudo ip link set veth-s netns dhcp-srv
sudo ip link set veth-c netns dhcp-cli

sudo ip -n dhcp-srv link set lo up
sudo ip -n dhcp-srv addr add 10.99.0.1/24 dev veth-s
sudo ip -n dhcp-srv link set veth-s up

sudo ip -n dhcp-cli link set lo up
sudo ip -n dhcp-cli link set veth-c up
```

Create a DHCP-only dnsmasq config:

```bash
cat > dhcp.conf <<'EOF'
port=0
interface=veth-s
bind-interfaces
dhcp-authoritative
dhcp-range=10.99.0.100,10.99.0.120,255.255.255.0,12h
dhcp-option=3,10.99.0.1
dhcp-option=6,10.99.0.1
log-dhcp
EOF

sudo ip netns exec dhcp-srv dnsmasq --no-daemon --conf-file=$PWD/dhcp.conf &
```

Capture DORA in the server namespace:

```bash
sudo ip netns exec dhcp-srv timeout 20 tcpdump -eni veth-s 'port 67 or port 68'
```

In another terminal, run a DHCP client in the client namespace with a custom script that configures the address (busybox `udhcpc`):

```bash
cat > /tmp/udhcpc.script <<'EOF'
#!/bin/sh
case "$1" in
  deconfig) ip addr flush dev "$interface" ;;
  bound|renew)
    ip addr add "$ip/$mask" dev "$interface"
    [ -n "$router" ] && ip route add default via "$router"
    ;;
esac
exit 0
EOF
chmod +x /tmp/udhcpc.script

sudo ip netns exec dhcp-cli busybox udhcpc -i veth-c -s /tmp/udhcpc.script -q -n -t 5
```

Expected: `udhcpc` obtains a lease in `10.99.0.100–10.99.0.120`, and the capture shows Discover → Offer → Request → ACK.

Verify:

```bash
sudo ip -n dhcp-cli addr show veth-c
sudo ip -n dhcp-cli route show
```

Expected: `inet 10.99.0.x/24`, default via `10.99.0.1`.

Test connectivity to the server:

```bash
sudo ip netns exec dhcp-cli ping -c 2 10.99.0.1
```

## Why it works

- `dnsmasq` answers from `address=`/`cname=`/`srv-host=` records and caches upstream answers. `no-resolv` prevents forwarding, making it authoritative for the lab names.
- NXDOMAIN is cached using the SOA minimum, so a newly created record can remain invisible until the negative TTL expires.
- `getent` follows `nsswitch.conf`; `dig` bypasses it. An application can therefore see a different answer than your DNS test.
- DHCP uses broadcast because the client has no address. DORA gives the address, gateway (option 3), and DNS (option 6). The client configures the interface only after ACK.

## Troubleshooting task

1. `dig @127.0.0.1 -p 5353 app.lab.test` returns `REFUSED`. What is the likely cause?
2. `dig` returns NXDOMAIN, but `getent hosts app.lab.test` returns an address. Why?
3. A new record was added, but clients get NXDOMAIN for five minutes. What is happening?
4. The DHCP client gets an IP but cannot reach the internet. Which two options do you check?
5. The DHCP capture shows Discover but no Offer. Name three causes.

### Expected answers

1. dnsmasq is not listening on that address/port, or another resolver owns the port; `REFUSED` often means you reached a server that will not answer for that zone/interface.
2. `/etc/hosts` or another NSS source has the name; `dig` queries DNS only.
3. Negative caching by a recursive resolver; wait for the SOA minimum or flush the internal cache.
4. Gateway (option 3) and DNS (option 6); also check forwarding/NAT on the gateway.
5. No DHCP server on the segment; relay missing; firewall blocking UDP 67/68; server pool exhausted; wrong interface/bind.

## Verification

```bash
dig @127.0.0.1 -p 5353 +short app.lab.test
dig @127.0.0.1 -p 5354 +short app.lab.test
dig @127.0.0.1 -p 5353 missing.lab.test | grep status
sudo ip -n dhcp-cli addr show veth-c
```

Expected: two different addresses for split-horizon, `NXDOMAIN` for the missing name, and a leased address on the client.

## Cleanup

```bash
sudo pkill -f 'dnsmasq --no-daemon'
sudo ip netns del dhcp-srv
sudo ip netns del dhcp-cli
sudo sed -i '/hosts-only.lab.test/d' /etc/hosts
rm -f ~/labs/l2.4/dns-*.conf ~/labs/l2.4/dhcp.conf /tmp/udhcpc.script
```

## Write-up template

```markdown
## L2.4 — DNS and DHCP

**Records created:** A, CNAME, TXT, SRV
**NXDOMAIN evidence:** status + SOA
**Split horizon:** resolver A vs B answers
**dig vs getent:** ...
**DORA capture:** (4 lines: Discover/Offer/Request/ACK)
**Lease:** address, mask, gateway, DNS
**Failure I diagnosed:** ...
**Rule:** always test with the same resolver/NSS path the application uses.
```

---

## Phase 2 lab log

| Lab | Date | Time | Pass? | Weakest idea | Retest date |
|---|---|---|---:|---|---|
| L2.1 | | | | | |
| L2.2 | | | | | |
| L2.3 | | | | | |
| L2.4 | | | | | |

Continue to [Labs part 2](10-labs-part2.md) for NAT/firewalls, HTTP/TLS, and the packet-capture capstone.
