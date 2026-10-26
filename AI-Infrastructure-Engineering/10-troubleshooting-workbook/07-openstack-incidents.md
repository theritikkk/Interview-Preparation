# OpenStack Incidents (INC-16 – INC-17)

> **Evidence first.** Same method. OpenStack incidents add distributed-service reasoning: every operation touches several services (nova, neutron, cinder, glance, keystone), and the error often lives in the service you did not look at. Always ask: **which service owns the failing step?**

---

## INC-16 — Large transfers stall on newly created instances

### The page

> **20:40 SEV2** — Instances created this week on a new compute host transfer large files slowly and stall intermittently. Old instances on other hosts are fine. Small transfers work everywhere. The network team says nothing changed.

### The evidence (raw, as collected)

```bash
# From an old instance (fine):
ip link show eth0 | grep -i mtu
ip route get 10.0.44.20

# From a new instance (stalling):
ip link show eth0 | grep -i mtu
ping -c 3 -M do -s 8971 10.0.44.20      # jumbo probe (8971 + 28 = 8999 ~ 9000)
ping -c 3 -M do -s 1472 10.0.44.20      # standard probe

# On the OpenStack side:
openstack network show provider-vlan44 -c mtu -c provider:network_type
openstack port show <server-port-id> -c mtu -c network_id
neutron net-update ... # (not run; read-only checks only)
```

```text
Old instance:
  eth0: mtu 8950
  route get: ... mtu 8950

New instance:
  eth0: mtu 1500
  ping -s 8971: 3 packets transmitted, 0 received, 100% packet loss
  ping -s 1472: 3 packets transmitted, 3 received, 0% packet loss

$ openstack network show provider-vlan44 -c mtu
  mtu = 8950

$ openstack port show ... -c mtu
  mtu = 8950        <-- the port says 8950; the instance booted with 1500
```

The new compute host's physical NICs are configured for jumbo frames (MTU 9000+); the image used for the new instances has no MTU configuration in cloud-init and no DHCP MTU option reaching the guest.

### Your turn (before the line)

- [ ] Timeline.
- [ ] Three hypotheses ranked with evidence.
- [ ] The one discriminating test.
- [ ] First 10 minutes.

---
---

# ANALYSIS — INC-16

### Timeline

A new compute host was added with jumbo-frame-enabled physical NICs. Existing images on old hosts had MTU baked in (8950) via cloud-init or image config. The new instances were booted from an image **without** that configuration, and the DHCP server (neutron's dnsmasq) was not passing MTU option 26 for this network — so the guest defaulted to 1500 while the network/ports advertised 8950. Large TCP segments from the guest (up to 1460-byte MSS) are fine, but the **reverse** direction is the problem: hosts on the network send larger frames toward the guest, which drops them (or the L2 path fragments poorly), so inbound-heavy transfers (downloads, reads from shared storage) stall while small control traffic works.

### Hypotheses ranked

1. **MTU mismatch between the guest (1500) and the network (8950).** Guest `mtu 1500`, jumbo ping fails, standard ping passes, port/network MTU 8950. Matches.
2. **Host NIC/driver issue on the new compute host.** Possible but unsupported: the same host passes standard-size traffic, and old instances (same host, if any) with correct MTU work.
3. **Security groups/iptables dropping fragmented packets.** Related in mechanism but the root is the mismatch; SG rules would affect both sizes.

### The discriminating test

The `ping -M do -s <size>` sweep from the affected guest to a known-good peer: jumbo fails, standard passes → the guest's MTU is the constraint. If jumbo passed from the guest but large transfers still stalled, the next test would be in the opposite direction (from the peer to the guest), since asymmetric MTU is a common variant.

### Root cause

The image lacked MTU configuration and DHCP did not deliver option 26, so the guest ran at 1500 in an 8950 network. Transfers involving frames larger than 1500 toward the guest fail or fragment badly; TCP's behavior under silent drops (no ICMP Frag Needed from L2) looks like random stalls.

### Fix (smallest safe first)

1. Set the MTU on affected guests to match the network:

```bash
sudo ip link set dev eth0 mtu 8950
```

   (plus persist it via cloud-init/netplan or the image config).
2. Verify: jumbo ping passes in both directions; large transfers complete; `tracepath -n` shows the expected path MTU.
3. Fix at the source so new instances are correct: enable DHCP MTU option 26 on the neutron network (`neutron net-update <net> --mtu 8950` is one path; with ML2/OVS, `path_mtu`/`mtu` on the network or the `dnsmasq` config for option 26), and bake MTU into images via cloud-init (`network-config` with `mtu`).

### Prevention

- Bake MTU into every image's cloud-init network config; never rely on the guest defaulting correctly.
- Ensure DHCP delivers option 26 (MTU) on every network with non-default MTU.
- Post-boot validation: a startup script (cloud-init `runcmd`) asserts `ip link` MTU equals the port's MTU, and a jumbo ping to the default gateway; fail visibly if not.
- Instance launch checklists include a large-transfer smoke test for new images.
- Document the MTU per network in the platform runbook; OpenStack MTU touches neutron (ports/networks), the physical switches, the hypervisor bridges, and the guest image — four places, and the truth is the **minimum** of them.

### Lessons

- **The truth is the minimum MTU on the path,** and in OpenStack that minimum spans four layers: switch, hypervisor bridge, neutron network/port, guest.
- **DHCP option 26 is how guests learn MTU.** If it is not delivered, the guest guesses (usually 1500) and the mismatch is silent.
- **Jumbo `ping -M do` is the 10-second MTU test.** Small traffic passing while large traffic fails is the signature.

### 5-minute walkthrough script

> "New instances on a new compute host stalled on large transfers while small traffic worked. The old instances had MTU 8950; the new ones booted with 1500. A jumbo `ping -M do` sweep failed at 8971 bytes and passed at 1472, and the neutron network/port showed MTU 8950 — so the guest was running at 1500 inside a jumbo network. The image had no MTU in cloud-init and DHCP wasn't delivering option 26, so the guest guessed wrong. Large frames toward the guest were dropped or fragmented badly, which looks like random stalls. Fix: set MTU on the guest, persist it in the image config, and deliver option 26 via DHCP. The lesson: the effective MTU is the minimum across switch, bridge, neutron, and guest — make the guest learn it, don't let it guess."

---

## INC-17 — Volume attach fails on a healthy-looking cluster

### The page

> **13:20 SEV2** — Attaching a volume to a new instance fails with `No valid host was found`. The API shows the cluster healthy; instances launch fine without volumes. Cinder's volume creates fine; only the attach fails.

### The evidence (raw, as collected)

```bash
openstack server show app-3 -c OS-EXT-AZ:availability_zone -c hypervisor_hostname -c status
openstack volume show vol-77 -c status -c availability_zone -c attachments -c size
openstack volume service list
cinder service-list 2>/dev/null | head   # or the unified CLI equivalent
openstack availability zone list
journalctl -u cinder-scheduler --since "1 hour ago" | tail -20   # if controller access
```

```text
$ openstack server show app-3
  OS-EXT-AZ:availability_zone = zone-b
  status = ACTIVE

$ openstack volume show vol-77
  status = available
  availability_zone = zone-a        <-- MISMATCH
  attachments = []

$ openstack volume service list
+------------------+-----------------+------+---------+-------+----------------------------+
| Binary           | Host            | Zone | Status  | State | Updated At                 |
+------------------+-----------------+------+---------+-------+----------------------------+
| cinder-volume    | cinder@lvm-za   | zone-a | enabled | down  | 3 hours ago                |
| cinder-volume    | cinder@lvm-zb   | zone-b | enabled | up    | 2 seconds ago              |
| cinder-scheduler | controller-1    | zone-b | enabled | up    | 2 seconds ago              |
+------------------+-----------------+------+---------+-------+----------------------------+

$ journalctl -u cinder-scheduler | tail -5
... No valid host was found. ... (filter: availability zone)
```

### Your turn (before the line)

- [ ] Timeline.
- [ ] Three hypotheses ranked with evidence.
- [ ] The one discriminating test.
- [ ] First 10 minutes.

---
---

# ANALYSIS — INC-17

### Timeline

The volume `vol-77` was created in availability zone `zone-a` (its `cinder-volume` backend, `cinder@lvm-za`, has been down for 3 hours). The instance `app-3` lives in `zone-b`. Nova's scheduler filters cinder-volume backends by matching availability zone; with the zone-a backend down, there is no valid host to attach the volume, and the attach fails with `No valid host was found`. Instance launches without volumes are unaffected — which is why the cluster "looks healthy."

### Hypotheses ranked

1. **Zone mismatch plus a down cinder-volume service in the volume's zone.** Volume AZ `zone-a` vs server AZ `zone-b`; `cinder-volume` for zone-a is `down`; scheduler log says "No valid host (filter: availability zone)." Matches.
2. **No capacity in the backend.** Rejected: the filter named in the log is the availability-zone filter, not capacity; and volume *create* would have failed earlier for capacity.
3. **Volume state/corruption.** Rejected: the volume is `available` with no attachments; the state is fine.

### The discriminating test

`openstack volume service list` — it names the per-host service state and zones directly. Combined with the volume's AZ and the server's AZ, the two-line evidence closes the case. (If both services were up and zones matched, the next test would be the scheduler filters and capacity in `cinder.conf`/volume-type extras.)

### Root cause

The attach is scheduled by cinder (with nova coordination) onto a `cinder-volume` service **in the same availability zone as the volume**, and that service was down. The zone-a backend died (check its logs: a common cause is a full backend, a failed LVM/target driver, or a config change); because volume *creation* in zone-a fails too, the failure is only visible when someone tries to use a **pre-existing** zone-a volume with a zone-b instance.

### Fix (smallest safe first)

1. Bring the zone-a `cinder-volume` service back: check its logs for the failure (config error, backend full, driver failure), fix, and restart the service. Verify `openstack volume service list` shows `up`.
2. Verify the attach succeeds (or delete and recreate the volume in zone-b if the data is disposable and the backend recovery is slow — a legitimate operational shortcut when the volume is replaceable).
3. Confirm volume creation works in zone-a again; otherwise the zone is still degraded.

### Prevention

- Alert on cinder service state (`cinder-volume` down > 2 min) — this is a 5-line check against the API.
- Zone-aware tooling: when creating a volume for a server, pass `--availability-zone <server's zone>`; script it so the mismatch cannot happen.
- Health check that actually exercises the path: create a tiny test volume **per zone** hourly and attach it to a disposable test server; a scheduler-level failure is invisible to API-level health checks.
- Document the "No valid host" runbook: it is almost always (a) a down service, (b) a zone/filter mismatch, or (c) capacity — in that order.

### Lessons

- **"No valid host" is a scheduler message, not a volume message.** It says: no candidate backend passed the filters. Read which filter failed.
- **Zone affinity is a real constraint.** A volume lives in a zone; a compute instance lives in a zone; attach requires a backend in the volume's zone.
- **API-level health checks miss scheduler-level failures.** Exercise the full path (create + attach) with a canary, or the down service is invisible until a user hits it.

### 5-minute walkthrough script

> "Volume attaches failed with 'No valid host was found' while instance launches were fine. That message is from the scheduler, so the question is which filter failed. The volume was in zone-a; the instance in zone-b; and `openstack volume service list` showed the zone-a cinder-volume service down for three hours. Nova filters backends by availability zone, so with zone-a's backend down there was no valid host — the cluster looked healthy because nothing else touched zone-a. Fix was recovering the zone-a backend service, then verifying volume creation in that zone again; for disposable data, recreating the volume in the instance's zone is a legitimate shortcut. Prevention: service-state alerting, zone-aware tooling, and an hourly create-and-attach canary per zone. The lesson: a scheduler error names its filter — and a down backend is invisible until someone tries to use it."
