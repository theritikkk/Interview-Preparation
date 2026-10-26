# Infrastructure Security

> **The one-sentence model:** security is the set of controls that decide **who can reach what, what they can do when they get there, and how you would know** — and every control has a failure mode that is usually a convenience someone enabled.

---

## 1. Think in assets, boundaries, and blast radius

### The three questions

For every component, answer:

1. **What is the asset?** Data, compute, credentials, availability, reputation.
2. **Who can reach it?** Network path, identity, supply chain, physical/hypervisor.
3. **What happens if it is compromised?** Blast radius: one container, one node, one account, one region, the whole company.

```text
        Internet
           │
     ┌─────▼──────┐        trust boundary 1: edge
     │  WAF / LB  │
     └─────┬──────┘
           │
     ┌─────▼──────┐        trust boundary 2: application network
     │  Service   │
     └──┬──┬──┬───┘
        │  │  └──────────► metadata/instance credentials  (trust boundary 3)
        │  └─────────────► database (credentials, network policy)
        └────────────────► CI/CD, registry, secrets store (supply chain)
```

### Threat modeling with STRIDE

| Threat | Question | Example | Control |
|---|---|---|---|
| **Spoofing** | Can someone pretend to be another identity? | Stolen SSH key, forged token | MFA, short-lived credentials, mTLS |
| **Tampering** | Can data or code be modified? | Writable image tag, unsigned artifact | Signatures, checksums, immutable tags |
| **Repudiation** | Can an action be denied? | No audit log of a deleted bucket | Central audit logs, time sync |
| **Information disclosure** | Can data leak? | Public bucket, verbose error, secret in env | Least privilege, encryption, redaction |
| **Denial of service** | Can availability be destroyed? | Unbounded request, disk fill, fork bomb | Limits, quotas, rate limits, timeouts |
| **Elevation of privilege** | Can a limited identity become admin? | Privileged pod, `sudo` misconfiguration | Least privilege, admission control, hardening |

### Blast radius

A compromised workload inherits everything it can reach:

- its service account token and cloud role;
- the node's kubelet credentials and instance profile (if not isolated);
- the database credentials in its environment;
- the internal network, including the control plane and other tenants;
- the image registry and CI token, if mounted.

The goal is not "prevent every compromise." The goal is to make one compromise **small, detectable, and recoverable**. A pod with a read-only filesystem, no token, a deny-all network policy, and a scoped database user is a much smaller incident than a privileged pod on a node with an admin instance profile.

---

## 2. Identity and least privilege

### Authentication vs authorization

| Question | Mechanism | Failure mode |
|---|---|---|
| Who are you? | Password, key, certificate, OIDC token, instance identity | Shared accounts, long-lived keys, missing MFA |
| What may you do? | IAM policy, RBAC, sudoers, file mode, capability | Wildcards, default roles, inherited admin |
| Can you prove it later? | Audit log, signed request, session recording | No logs, logs in the same blast radius, clock skew |

### Linux users, sudo, and capabilities

```bash
id
sudo -l
sudo -l -U deploy
getent passwd | awk -F: '$3 >= 1000 {print $1, $3, $6, $7}'
```

Representative `sudo -l`:

```text
User deploy may run the following commands on web-01:
    (root) NOPASSWD: /usr/bin/systemctl restart nginx
    (root) NOPASSWD: /usr/bin/journalctl -u nginx
```

Interpretation:

- `NOPASSWD` on a narrow command is often acceptable for automation; `NOPASSWD: ALL` is effectively root.
- A sudo rule that allows `systemctl` without a unit restriction can often be abused (for example, a user-writable unit file or `systemctl edit`). Allowlist the exact unit and verb.
- `sudo -l` shows the invoking user's rights; `-U` checks another account. Both require privilege to inspect fully.
- Service accounts should have `/usr/sbin/nologin` and no password hash.

Dangerous patterns:

```text
(ALL) NOPASSWD: ALL
(ALL) NOPASSWD: /bin/bash
(ALL) NOPASSWD: /usr/bin/find
(ALL) NOPASSWD: /usr/bin/vi
```

`find`, `vi`, `less`, `awk`, `python`, `tar`, and `systemctl` can execute commands or write files if not tightly constrained. A sudo rule is a code-execution primitive unless it is scoped.

### File permissions and special bits

```bash
find / -xdev -type f -perm -4000 2>/dev/null
find / -xdev -type f -perm -0002 2>/dev/null
find /etc -type f -perm /o+w 2>/dev/null
```

- SUID binaries run with the file owner's privileges. Every SUID binary is a potential privilege-escalation path; minimize the set and monitor changes.
- World-writable files in `/etc`, cron directories, or `PATH` directories are serious.
- `umask 027` or stricter for service accounts; `0640` for config with secrets.
- Check home directory and SSH permissions: `~/.ssh` 700, private keys 600, `authorized_keys` 600.

### SSH hardening

`/etc/ssh/sshd_config.d/99-hardening.conf`:

```text
PermitRootLogin no
PasswordAuthentication no
KbdInteractiveAuthentication no
PubkeyAuthentication yes
AllowUsers deploy
MaxAuthTries 3
LoginGraceTime 20
ClientAliveInterval 300
ClientAliveCountMax 2
X11Forwarding no
AllowAgentForwarding no
PermitTunnel no
```

Validate before restarting:

```bash
sudo sshd -t
sudo systemctl reload sshd
```

Keep an existing session open while testing a new one. A bad SSH config plus a closed console is a locked-out VM.

Key hygiene:

```bash
ssh-keygen -t ed25519 -a 100 -C "deploy@opslab"
ssh-keygen -lf ~/.ssh/id_ed25519.pub
ssh-keygen -R old-host.example.com
```

- Prefer Ed25519; use a passphrase; never copy private keys between people.
- `ssh-keygen -R` removes a stale host key after a rebuild; verify the new fingerprint out of band, do not blindly accept it.
- Use `ssh -o BatchMode=yes -o ConnectTimeout=5` for automation so it fails instead of prompting.
- Prefer short-lived certificates or an access broker over static `authorized_keys` at scale.

### Cloud IAM

The core model:

```text
principal (user/role/service) ──► assumes ──► role ──► policy ──► resource
                                   │
                                   └── trust policy: who may assume this role, under what conditions
```

Rules:

1. **No long-lived access keys for workloads.** Use instance profiles, workload identity, or OIDC federation.
2. **Scope actions and resources.** `s3:GetObject` on `arn:aws:s3:::bucket/prefix/*`, not `s3:*` on `*`.
3. **Use conditions.** Restrict by account, region, VPC endpoint, MFA, source IP, or tag where supported.
4. **Separate roles by job.** CI deploy role ≠ application runtime role ≠ human admin role.
5. **Permission boundaries and SCPs** limit the maximum, but do not grant anything themselves.
6. **Break-glass is explicit.** A monitored, time-bound admin role with an alert is better than permanent admin for everyone.
7. **Review unused permissions.** Access Analyzer, last-accessed data, and periodic reviews.

A dangerous policy:

```json
{
  "Effect": "Allow",
  "Action": "*",
  "Resource": "*"
}
```

A scoped policy:

```json
{
  "Effect": "Allow",
  "Action": ["s3:GetObject", "s3:PutObject"],
  "Resource": "arn:aws:s3:::opslab-artifacts/builds/*"
}
```

The scoped policy still needs a bucket policy and a role trust policy that prevent cross-account or anonymous access.

### Kubernetes RBAC

```text
ServiceAccount ──bound by──► RoleBinding ──► Role (namespaced)
      │                         │
      │                         └──► ClusterRole (cluster-wide, if ClusterRoleBinding)
      └── token mounted into Pod (unless automountServiceAccountToken: false)
```

```bash
kubectl auth can-i --list
kubectl auth can-i get secrets -n checkout
kubectl auth can-i --list --as=system:serviceaccount:checkout:api
kubectl get rolebindings,clusterrolebindings -A -o wide
```

Representative output:

```text
Resources                                       Non-Resource URLs   Resource Names   Verbs
pods                                            []                  []               [get list watch]
pods/log                                        []                  []               [get]
secrets                                         []                  []               [get]
```

Interpretation:

- `get secrets` in a namespace is effectively read access to every credential in that namespace. Treat it as privileged.
- `create pods` can become node-level access if the pod can mount host paths or run privileged.
- `impersonate` and `bind` on roles/clusterroles are privilege-escalation paths.
- Wildcards (`*`) in verbs/resources are a finding until proven necessary.
- The default service account token is mounted automatically unless disabled; a compromised pod can use it against the API server.

Minimal pod security context:

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: api
spec:
  automountServiceAccountToken: false
  securityContext:
    runAsNonRoot: true
    runAsUser: 10001
    fsGroup: 10001
    seccompProfile:
      type: RuntimeDefault
  containers:
    - name: api
      image: registry.example.com/api@sha256:...
      securityContext:
        allowPrivilegeEscalation: false
        readOnlyRootFilesystem: true
        capabilities:
          drop: ["ALL"]
      resources:
        requests: {cpu: 100m, memory: 128Mi}
        limits:   {cpu: 500m, memory: 256Mi}
```

`runAsNonRoot` without a numeric `runAsUser` can fail if the image's user is named but not numeric; set both. `readOnlyRootFilesystem` requires emptyDir mounts for temp/cache paths. A pod that needs no Kubernetes API access should not have a token.

### Credential lifecycle

| Credential | Lifetime | Storage | Rotation |
|---|---|---|---|
| Human password | Session | Password manager/IdP | On compromise, periodically |
| MFA device | Persistent | Hardware key/authenticator | On loss |
| SSH key | Months | Agent/secure store | Quarterly or on offboarding |
| Cloud role session | Minutes–hours | Memory | Automatic |
| CI OIDC token | Minutes | Workflow runtime | Automatic |
| Service account token | Configurable | Projected volume | Short-lived |
| Database password | Weeks–months | Secrets manager | Automated |
| API key | Depends | Secrets manager | On exposure, scheduled |

If a credential cannot be rotated without a deploy, it is a design defect.

---

## 3. Network security

### Default deny, then allow

```text
Internet ──► edge (443 only) ──► app subnet (app port only) ──► data subnet (DB port only)
                                       │
                                       └──► egress: package registry, object storage, DNS
```

Controls:

- **Security groups / firewall:** stateful allowlists attached to workloads; default deny inbound, explicit egress.
- **Network ACLs:** stateless subnet-level allow/deny; useful for coarse blocking, easy to misconfigure because return traffic must be allowed.
- **Private subnets:** no public IP; egress via NAT or VPC endpoints.
- **Bastion / session manager:** no direct SSH from the internet; audited, short-lived access.
- **Service mesh / mTLS:** identity-based service-to-service auth, not just IP allowlists.
- **Kubernetes NetworkPolicy:** namespaced L3/L4 allowlists; requires a CNI that enforces them (kindnet often does not).

Check listening services:

```bash
ss -tulpn
```

Representative output:

```text
Netid State  Local Address:Port   Peer Address:Port  Process
udp   UNCONN 127.0.0.53:53         0.0.0.0:*          systemd-resolve
tcp   LISTEN 0.0.0.0:22            0.0.0.0:*          sshd
tcp   LISTEN 127.0.0.1:9090        0.0.0.0:*          prometheus
tcp   LISTEN 0.0.0.0:3000          0.0.0.0:*          node
```

- `0.0.0.0` is all IPv4 interfaces; `[::]` is all IPv6. A service bound to `0.0.0.0` is reachable wherever the firewall allows it.
- `127.0.0.1` is local only; that is the right default for Prometheus, Grafana admin, and debug endpoints.
- A listener is not the same as reachability: the firewall/security group may block it. Check both.
- `ss -tulpn` needs root to see other users' process names; `sudo ss -tulpn` for the full picture.

Firewall inspection:

```bash
sudo nft list ruleset
sudo ufw status verbose
sudo iptables -S
```

Do not run `iptables -F` on a remote host to "see if the firewall is the problem." You may lose access and remove the control that was protecting you. Change one rule, keep a console, and have a rollback.

### Egress is part of the attack surface

Inbound controls get the attention; egress is how data leaves and how a compromised host reaches the internet.

- Restrict egress to required destinations (package mirrors, object storage, APIs) where the platform supports it.
- Log DNS and blocked egress; a sudden spike to an unknown domain is a detection signal.
- Block the cloud metadata endpoint from workloads that do not need it, and require IMDSv2.
- A compromised container with unrestricted egress can download tools, exfiltrate data, or join a botnet.

### The metadata service

Cloud instance metadata (for example, `169.254.169.254`) can return temporary credentials to anything that can reach it. Server-side request forgery (SSRF) in a web app can therefore become cloud account access.

Controls:

- Require IMDSv2 (session token) where available; block IMDSv1.
- Set a hop limit of 1 so containers cannot reach it through the host.
- Use IRSA/workload identity or instance profiles scoped to the workload.
- Block `169.254.169.254` from pods that do not need it via network policy.
- Treat any SSRF as a credential incident until proven otherwise.

### TLS

```text
Client                                          Server
  │──── ClientHello (versions, ciphers, SNI) ────►│
  │◄─── ServerHello, Certificate, KeyExchange ───│
  │──── verify chain + hostname + validity ──────│
  │──── Finished ────────────────────────────────►│
  │◄─── Finished ─────────────────────────────────│
  │════ encrypted application data ══════════════│
```

What TLS gives you:

- **Confidentiality** in transit.
- **Integrity** against tampering.
- **Server authentication** if the client verifies the certificate chain and hostname.
- **Client authentication** only with mTLS (a client certificate), not with ordinary TLS.

What it does not give you:

- authorization (who may call which endpoint);
- protection after termination (inside the load balancer or service mesh);
- protection from a compromised endpoint;
- automatic certificate renewal.

Inspect a certificate:

```bash
openssl s_client -connect api.example.com:443 -servername api.example.com -showcerts </dev/null 2>/dev/null \
  | openssl x509 -noout -subject -issuer -dates -ext subjectAltName
```

Representative output:

```text
subject=CN = api.example.com
issuer=C = US, O = Let's Encrypt, CN = R3
notBefore=Oct  1 00:00:00 2026 GMT
notAfter=Dec 30 23:59:59 2026 GMT
X509v3 Subject Alternative Name:
    DNS:api.example.com, DNS:www.api.example.com
```

Check:

- `notAfter` — expiry, and whether renewal is automated.
- SAN — the hostname must be in `subjectAltName`; CN alone is not enough for modern clients.
- Chain — `s_client` should show the leaf and intermediates; a missing intermediate works in browsers that cache it and fails in `curl`/Java.
- Protocol and cipher:

```bash
openssl s_client -connect api.example.com:443 -servername api.example.com -tls1_2 </dev/null 2>&1 | grep -E 'Protocol|Cipher'
```

Common failures:

| Symptom | Cause | Evidence |
|---|---|---|
| `certificate has expired` | Renewal failed | `notAfter`, ACME logs, cron/timer |
| `unable to get local issuer certificate` | Missing intermediate | `s_client -showcerts`, chain order |
| `certificate is not valid for this name` | SAN mismatch | `-ext subjectAltName`, SNI |
| Works in browser, fails in `curl` | Browser cached intermediate or different trust store | `curl -v`, `openssl` chain |
| `tlsv1 alert protocol version` | Client/server version mismatch | `-tls1_2`, server config |
| Handshake slow | OCSP/CRL fetch, large chain, CPU | timing, `-status` |

mTLS adds a client certificate; the server verifies it against a CA. It authenticates the client but still needs authorization (which client may call which route). Rotating client certs is an operational burden; short-lived certs from a mesh or SPIFFE-style identity reduce it.

### DNS

- Use DNSSEC where the resolver validates it; it protects integrity, not confidentiality.
- Pin or allowlist critical egress domains; DNS tunneling can exfiltrate data through queries.
- Monitor for sudden resolution changes of your own domains (takeover risk).
- A dangling DNS record pointing to a released cloud IP is a subdomain-takeover path.

---

## 4. Secrets

### The secret lifecycle

```text
create ──► store encrypted ──► deliver to workload ──► use in memory ──► rotate ──► revoke
   │              │                    │                    │              │
   │              │                    │                    │              └── old value invalid
   │              │                    │                    └── never log, never argv, never image layer
   │              │                    └── file/volume or short-lived token, not baked image
   │              └── KMS/Vault/Sealed Secret, access-controlled, audited
   └── human or generated; no default password
```

### Where secrets leak

| Location | Example | Detection |
|---|---|---|
| Git history | `AWS_SECRET_ACCESS_KEY=...` committed then deleted | `gitleaks`, `git log -S` |
| Container image | `ENV DB_PASSWORD=...` in a layer | `trivy`, `docker history` |
| Environment dump | crash handler prints all env vars | log review, redaction |
| Command line | `mysql -psecret` visible in `ps` | process accounting, avoid |
| Logs | request body or headers logged | structured logging review |
| CI output | `echo $TOKEN` or debug mode | mask secrets, review logs |
| Backup/snapshot | unencrypted volume or etcd backup | encryption, access control |
| Chat/ticket | credentials pasted to debug | secret scanning, rotation |

### Kubernetes Secrets are not automatically secret

By default, a Kubernetes Secret is base64-encoded, stored in etcd, and readable by anyone with `get secrets` in the namespace. Base64 is encoding, not encryption.

Controls:

- Enable encryption at rest for etcd with a KMS provider.
- Restrict `get/list/watch secrets` via RBAC; audit access.
- Use external secrets (Vault, cloud secrets manager) with workload identity, or Sealed Secrets/SOPS for GitOps.
- Mount as files with restrictive modes instead of environment variables where possible; env vars leak in dumps and child processes.
- Do not commit encrypted secrets without controlling the decryption key; SOPS with a cloud KMS is stronger than a key in the repo.
- Rotate after any suspected exposure; deleting the Git commit is not enough.

Example external secret (conceptual):

```yaml
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: db
  namespace: checkout
spec:
  refreshInterval: 1h
  secretStoreRef:
    name: vault
    kind: ClusterSecretStore
  target:
    name: db-credentials
  data:
    - secretKey: password
      remoteRef:
        key: checkout/db
        property: password
```

The workload gets a Kubernetes Secret synced from Vault; the pod's service account is authorized to read only that path. The secret still exists in etcd, so encryption at rest and RBAC still matter.

### Detect a leak

```bash
gitleaks detect --source . --redact --verbose
gitleaks protect --staged --redact
git log -p -S 'AKIA' --all -- . | head -50
```

Representative `gitleaks` output:

```text
Finding:     AWS_SECRET_ACCESS_KEY = "AKIA...redacted"
Secret:      AKIA...redacted
RuleID:      aws-access-token
Entropy:     4.02
File:        deploy/prod.env
Line:        12
Commit:      a1b2c3d
```

If a real secret is found:

1. **Revoke/rotate first.** Assume it is compromised; history rewriting does not un-leak it.
2. Identify exposure scope: public repo, CI logs, image registry, forks, caches.
3. Check audit logs for use of the credential.
4. Remove from the working tree and history where appropriate, but only after rotation.
5. Add a pre-commit hook and CI scan to prevent recurrence.

`git log -S` searches for changes in the number of occurrences of a string; it is a starting point, not a complete scanner (it misses secrets in binary files and some encodings).

---

## 5. Host hardening

### The baseline

1. **Minimal image:** fewer packages, fewer CVEs, smaller attack surface. Use a maintained base and rebuild regularly.
2. **Patch:** security updates on a schedule; reboot when the kernel/library requires it. A patched package without a restarted process is still vulnerable.
3. **Accounts:** no shared accounts; no empty passwords; service accounts nologin; remove stale users.
4. **SSH:** keys only, no root login, allowlist users, rate-limit.
5. **Sudo:** scoped commands, no wildcards, `requiretty` where appropriate, logging.
6. **Filesystem:** separate partitions for `/var`, `/tmp`, `/home` where practical; `noexec,nosuid,nodev` on `/tmp` and removable media.
7. **Services:** disable unused services; bind to localhost; run as non-root.
8. **Kernel:** enable ASLR, restrict `ptrace`, disable unneeded modules, set `fs.protected_*`.
9. **Audit:** `auditd` rules for identity, privilege, and sensitive file changes; ship logs off-host.
10. **Time:** NTP/chrony; logs and certificates depend on correct time.

### Inspect the host

```bash
sudo systemd-analyze security sshd.service
```

Representative output:

```text
→ Overall exposure level for sshd.service: 9.6 UNSAFE 😨
```

The score is a heuristic based on sandboxing directives (`ProtectSystem`, `PrivateTmp`, `NoNewPrivileges`, `CapabilityBoundingSet`, etc.). A high score is a prompt to review, not a proof of exploitability. SSH needs some privileges; the goal is to remove what it does not need.

```bash
sudo lynis audit system
```

Representative output:

```text
[+] Boot and services
  - Checking UEFI boot                                        [ N/A ]
  - Checking presence GRUB2                                   [ FOUND ]
...
[+] Kernel Hardening
  - Checking /proc/mounts                                     [ FOUND ]
  - Checking sysctl settings                                  [ SUGGESTION ]
...
Hardening index : 62 [############        ]
```

Lynis produces suggestions, not a compliance certificate. Read each one, decide whether it applies, and record the exceptions. Blindly applying every suggestion can break the host.

```bash
sudo getcap -r / 2>/dev/null
sudo capsh --print
```

- File capabilities grant specific privileges without full SUID. `cap_net_raw` on `ping` is normal; `cap_setuid` on an arbitrary binary is not.
- `capsh --print` shows the current process's bounding/effective sets. A container with `CAP_SYS_ADMIN` is close to root on the host.

### Auditd

```bash
sudo apt-get install -y auditd
sudo auditctl -w /etc/passwd -p wa -k identity
sudo auditctl -w /etc/sudoers -p wa -k privilege
sudo auditctl -a always,exit -F arch=b64 -S execve -k exec
sudo auditctl -l
```

Representative rule list:

```text
-w /etc/passwd -p wa -k identity
-w /etc/sudoers -p wa -k privilege
-a always,exit -F arch=b64 -S execve -k exec
```

- `-w` watches a path; `-p wa` watches writes and attribute changes.
- `-k` assigns a key for searching.
- `-a always,exit -S execve` logs every command execution; high volume, useful in a lab, expensive at scale.
- Rules must persist in `/etc/audit/rules.d/*.rules`; `auditctl` rules vanish on reboot.
- `ausearch -k identity -ts recent` and `aureport --summary` read the log.

Audit logs in the same blast radius as the host are not trustworthy after a compromise. Ship them to a separate account/system with write-once or restricted deletion.

### Containers

| Control | Why | Check |
|---|---|---|
| Non-root user | Limits damage from app compromise | `runAsNonRoot`, image `USER` |
| Read-only root FS | Blocks droppers and persistence | `readOnlyRootFilesystem` |
| Drop capabilities | Removes mount/net-admin/etc. | `capabilities.drop: ["ALL"]` |
| No privilege escalation | Blocks setuid binaries | `allowPrivilegeEscalation: false` |
| Seccomp/AppArmor/SELinux | Restricts syscalls and access | `seccompProfile`, annotations |
| No host mounts | Prevents host filesystem access | review `hostPath` |
| Resource limits | Contains DoS | requests/limits |
| Network policy | Limits lateral movement | CNI enforcement |
| Image scanning | Finds known CVEs | `trivy image` |
| Signed images | Supply-chain integrity | cosign/notation |
| No secrets in image | Prevents registry leak | `docker history`, scanner |

Dangerous pod settings:

```yaml
securityContext:
  privileged: true          # effectively root on the node
  capabilities:
    add: ["SYS_ADMIN"]      # mount, namespace, and more
  hostPID: true             # see host processes
  hostNetwork: true         # bypass network policies
  hostPath:
    path: /                 # host filesystem
```

A pod with any of these is a node-level trust decision. Admission control should block them outside a narrow, audited system namespace.

Scan an image:

```bash
trivy image --severity HIGH,CRITICAL --ignore-unfixed nginx:1.25
```

Representative output:

```text
nginx:1.25 (debian 12.5)
========================
Total: 3 (HIGH: 2, CRITICAL: 1)

┌────────────┬────────────────┬──────────┬───────────────┬───────────────┐
│  Library   │ Vulnerability  │ Severity │ Installed Ver │ Fixed Version │
├────────────┼────────────────┼──────────┼───────────────┼───────────────┤
│ libssl3    │ CVE-2026-XXXX  │ CRITICAL │ 3.0.11-1      │ 3.0.13-1      │
│ libc6      │ CVE-2026-YYYY  │ HIGH     │ 2.36-9        │ 2.36-9+deb12u4│
└────────────┴────────────────┴──────────┴───────────────┴───────────────┘
```

Interpretation:

- A CVE in a library the process never loads may be lower risk, but "not reachable" is an analysis, not an assumption.
- `--ignore-unfixed` hides findings with no fix; use it to prioritize action, not to claim the image is clean.
- Rebuild on a new base; patching a running container is an anti-pattern.
- Scan the **digest** you deploy, not a tag that may have moved.

### Kubernetes admission and pod security

- **Pod Security Standards:** `privileged`, `baseline`, `restricted`. Enforce `restricted` in application namespaces; exempt only audited system namespaces.
- **Admission controllers:** OPA/Gatekeeper, Kyverno, or ValidatingAdmissionPolicy to block privileged pods, hostPath, `latest` tags, and missing limits.
- **Image policy:** allow only signed images from approved registries; reject mutable tags.
- **NetworkPolicy:** default deny ingress and egress, then allow required flows.
- **Secrets:** disable automount by default; use external secrets.
- **Audit logging:** enable API server audit with a policy that records metadata and sensitive resource changes.

Check a namespace:

```bash
kubectl get ns checkout -o jsonpath='{.metadata.labels}'
kubectl get pods -n checkout -o json | jq -r '
  .items[] |
  [.metadata.name,
   (.spec.securityContext.runAsNonRoot // false),
   (.spec.hostNetwork // false),
   (.spec.hostPID // false),
   ([.spec.containers[].securityContext.privileged // false] | any)]
  | @tsv'
```

Representative output:

```text
api-7d9f8c6b5-2xk4l  true   false  false  false
debug-5c4d8f7b9-9q2mv false  false  false  true
```

The `debug` pod is privileged. That is a finding: who created it, why, and is it still needed?

---

## 6. Cloud and OpenStack specifics

### AWS

| Control | Purpose | Check |
|---|---|---|
| IAM roles + OIDC | No static keys | `aws iam list-access-keys`, trust policies |
| S3 Block Public Access | Prevents accidental public buckets | `aws s3api get-public-access-block` |
| Security groups | Network allowlist | `aws ec2 describe-security-groups` |
| IMDSv2 | Blocks SSRF credential theft | instance metadata options |
| CloudTrail | API audit | multi-region trail, log integrity |
| GuardDuty | Threat detection | enabled in all regions |
| AWS Config | Configuration history/rules | required rules |
| KMS | Encryption keys and rotation | key policy, rotation status |
| VPC endpoints | Private access to S3/DynamoDB/etc. | endpoint policies |
| SCPs | Organization guardrails | deny dangerous actions |

```bash
aws s3api get-public-access-block --bucket opslab-artifacts
aws iam list-access-keys --user deploy
aws cloudtrail describe-trails
aws ec2 describe-instances --query 'Reservations[].Instances[].MetadataOptions'
```

Representative public access block:

```json
{
  "PublicAccessBlockConfiguration": {
    "BlockPublicAcls": true,
    "IgnorePublicAcls": true,
    "BlockPublicPolicy": true,
    "RestrictPublicBuckets": true
  }
}
```

All four `true` is the safe default for a private bucket. A bucket policy can still grant cross-account access; check the policy too.

Dangerous security group:

```text
0.0.0.0/0 → 22/tcp
0.0.0.0/0 → 3389/tcp
0.0.0.0/0 → 5432/tcp
```

SSH/RDP/database ports open to the internet are scanned continuously. Use a bastion/session manager, a VPN, or at minimum an allowlisted source IP.

CloudTrail check for a sensitive event:

```bash
aws cloudtrail lookup-events \
  --lookup-attributes AttributeKey=EventName,AttributeValue=CreateAccessKey \
  --max-results 10
```

Every `CreateAccessKey` should have a human or automation explanation. A new key followed by `PutBucketPolicy` or `RunInstances` in an unusual region is a classic compromise pattern.

### OpenStack

| Concept | Security meaning |
|---|---|
| **Domain** | Top-level identity boundary; often one per organization |
| **Project/tenant** | Resource and quota isolation; members can consume, not administer |
| **Role** | `member`, `reader`, `admin`; `admin` is powerful |
| **Keystone token** | Short-lived bearer credential; scope matters |
| **Security group** | Per-port allowlist; default deny inbound |
| **Keypair** | Public key injected at boot; private key never leaves you |
| **Cinder volume** | Block storage; snapshots and backups need access control |
| **Glance image** | Shared images can leak data or propagate a compromised base |
| **Neutron network** | Tenant networks; provider networks need admin care |

```bash
openstack token issue
openstack role assignment list --names
openstack security group rule list default
openstack image list --shared
```

- A token is a bearer credential; anyone who obtains it can act until expiry. Do not paste tokens into tickets or logs.
- `openstack role assignment list --names` shows who is admin where. Domain admin is broader than project admin.
- Shared images can cross tenant boundaries; verify the owner and checksum before use.
- Default security groups often allow all egress and no ingress; review them, do not assume.

---

## 7. Detection and audit

### What to log

| Source | Events | Retention |
|---|---|---|
| Authentication | SSH success/failure, sudo, console login, MFA | 90+ days hot, 1 year cold |
| Cloud API | CloudTrail/audit log, IAM changes, network changes | 1 year |
| Kubernetes | API audit, exec, secret access, pod create/delete | 90+ days |
| Host | `auditd` exec/identity/file changes, kernel messages | 30–90 days |
| Network | VPC flow logs, firewall denies, DNS | 30–90 days |
| Application | Auth events, authorization failures, admin actions | Per policy |
| Supply chain | Image build, signature, deploy digest | 1 year |

Logs need:

- **Time sync** (UTC, NTP) so correlation works.
- **Integrity** (central sink, restricted delete, object lock where required).
- **Context** (who, what, where, result, request ID, source IP).
- **Redaction** (never log passwords, tokens, full card numbers).
- **Retention** driven by policy, not disk space.

### Detection examples

| Signal | Why it matters | Query/check |
|---|---|---|
| Many failed SSH logins then a success | Brute force or credential stuffing | `ausearch`, auth log, fail2ban |
| New user or sudo rule | Persistence | `auditd` on `/etc/passwd`, `/etc/sudoers` |
| `CreateAccessKey` + unusual region | Credential compromise | CloudTrail |
| Public bucket policy | Data exposure | Config rule/S3 block |
| Container running as root/privileged | Escape path | admission logs, `kubectl` |
| Spike in egress to unknown domain | Exfiltration/C2 | flow logs, DNS |
| Crypto-mining CPU pattern | Resource abuse | CPU + process + network |
| Secret in CI log | Credential leak | log scanning |
| Unexpected image digest | Supply chain | admission policy |
| Disabled audit log | Covering tracks | control-plane config |

### A simple detection loop

1. **Collect** the events above into a central sink.
2. **Normalize** time, identity, resource, and result.
3. **Alert** on a small set of high-signal rules; everything else is a dashboard or a weekly review.
4. **Triage** with the same incident process as reliability, with a security lead.
5. **Preserve** evidence before containment changes the state.
6. **Improve** the rule or control after every true/false positive.

A SIEM you never tune is a log archive with a license fee.

---

## 8. Security incident response

### Principles

- **Safety first:** do not run destructive commands on a suspected compromised host; preserve volatile evidence (memory, connections, processes) before reboot.
- **Contain, then investigate:** isolate the network, revoke credentials, and disable the account, but keep the host alive if you need evidence.
- **Assume credential compromise:** rotate everything the workload could reach, not just the one key you found.
- **Preserve chain of custody:** record who accessed what, when, and with which tool; keep hashes of collected artifacts.
- **Communicate on a need-to-know basis:** security incidents can involve legal, privacy, and law enforcement. Do not speculate publicly.
- **Do not tip off the actor:** use out-of-band channels and avoid changes that alert them before you are ready.

### Compromised host playbook

```text
1. Confirm and scope: which host, account, data, credentials?
2. Preserve: memory/process list/network connections/logs (if policy allows).
3. Isolate: security group/quarantine VLAN; do not destroy the instance.
4. Revoke: instance role, SSH keys, tokens, database credentials.
5. Hunt: other hosts with the same image/key/behavior; audit logs.
6. Eradicate: rebuild from a trusted image, patch the entry point.
7. Recover: restore data from a known-good backup; verify integrity.
8. Learn: timeline, root cause, controls, detection gaps.
```

Volatile evidence commands (run only if authorized and the host is contained):

```bash
date -u
who
w
ss -tupn
ps auxf
lsof -p <pid>
sudo journalctl --since "2 hours ago" > /evidence/journal.txt
```

Hash evidence:

```bash
sha256sum /evidence/* > /evidence/SHA256SUMS
```

### Leaked credential playbook

1. Revoke or rotate the credential **now**; do not wait for the investigation.
2. Check usage: CloudTrail, application logs, CI logs, API access logs.
3. Identify scope: what could the credential do, and what did it touch?
4. Rotate dependent secrets if the credential could read them.
5. Remove from source/history and add scanning, but only after rotation.
6. Notify per policy if personal data or regulated data was accessed.

### Post-incident

- Add a detection rule for the behavior, not just a patch for the vulnerability.
- Add a preventive control (admission policy, IAM condition, network deny).
- Verify the control with a test that would have caught the original path.
- Update the threat model and the runbook.

---

## 9. Failure scenarios

### A — A pod can read the node's cloud credentials

**Symptoms:** a compromised web app calls the cloud API with the node's instance role; the role can read every bucket.

**Evidence:** pod service account, instance profile, IMDS reachability from the pod, CloudTrail events, and the role policy.

**Likely causes:** the node role is broad, IMDSv1 is enabled, the pod network can reach `169.254.169.254`, and no egress policy blocks it.

**Fix:** use workload identity/IRSA with a per-service role, require IMDSv2 with hop limit 1, block metadata from pods, and scope the role to the exact bucket/prefix. Rotate the node role credentials and audit usage.

### B — The CI token can deploy to production from any branch

**Symptoms:** a pull request from a fork can assume the deploy role; a malicious workflow exfiltrates the token.

**Evidence:** trust policy `sub` condition, environment protection rules, workflow triggers, and the token's permissions.

**Likely causes:** the OIDC trust policy allows `repo:org/repo:*`; the deploy job has no environment approval; `pull_request_target` checks out untrusted code with secrets.

**Fix:** restrict `sub` to the protected branch/environment, require approval, use least-privilege job permissions, never run untrusted code with secrets, and audit deployments by digest.

### C — An S3 bucket is public

**Symptoms:** a bucket with build artifacts or customer exports is readable anonymously.

**Evidence:** bucket policy, ACLs, Block Public Access settings, access logs, and object contents.

**Likely causes:** a policy created for a static website; ACLs enabled; block public access disabled "temporarily"; a cross-account role copied from a tutorial.

**Fix:** enable all four Block Public Access settings, remove public policy/ACLs, serve public content through CloudFront with an OAC, audit access logs, and rotate any secrets in the bucket.

### D — SSH is open to the internet with password auth

**Symptoms:** thousands of failed logins per day; one succeeds with a weak password.

**Evidence:** `sshd_config`, security group, auth logs, successful login source IPs, and sudo history.

**Likely causes:** a quick debug rule never removed; a default image with password auth; a shared password.

**Fix:** close the port, require keys, disable password auth, use a bastion/session manager, enable MFA for the bastion, and review the host for persistence.

### E — TLS certificate expired

**Symptoms:** all clients fail with a certificate error; the service is healthy on the host.

**Evidence:** `openssl s_client`, renewal timer/cron, ACME logs, DNS challenge records, and the load balancer certificate.

**Likely causes:** renewal failed silently; the certificate was manually uploaded and not automated; the load balancer uses a different cert than the host; the clock is wrong.

**Fix:** renew, automate renewal with monitoring (expiry < 21 days is a ticket; < 7 days is a page), test the renewal path, and monitor the certificate from outside.

### F — A secret is committed to a public repo

**Symptoms:** a scanner or a researcher reports an AWS key in Git history.

**Evidence:** commit, secret type, exposure duration, repository visibility, and usage logs.

**Likely causes:** `.env` committed; a debug config; a CI variable echoed; a fork or mirror.

**Fix:** revoke/rotate immediately, check usage, purge history after rotation, add pre-commit and CI scanning, and require short-lived credentials.

### G — A privileged pod escapes to the node

**Symptoms:** a container with `privileged: true` or `hostPath: /` can mount the host filesystem and read kubelet credentials.

**Evidence:** pod spec, admission logs, node audit, and what the pod could reach.

**Likely causes:** a debug pod left running; a Helm chart with privileged defaults; an admission policy in audit-only mode.

**Fix:** remove the pod, rotate node/kubelet credentials, enforce restricted pod security, block privileged/hostPath in application namespaces, and audit who can create pods.

### H — Audit logs are missing for the incident window

**Symptoms:** you know something happened but cannot reconstruct who did it.

**Evidence:** audit policy, agent status, log sink, retention, and time sync.

**Likely causes:** auditd not installed; Kubernetes audit policy excludes secrets; the agent crashed; logs rotated before collection; the attacker deleted local logs.

**Fix:** enable and ship logs centrally, test ingestion, set retention, alert on agent/log gaps, and use immutable storage for critical audit data.

### I — MFA fatigue leads to account takeover

**Symptoms:** a user approves a push notification repeatedly; an attacker logs in from a new location.

**Evidence:** IdP logs, MFA method, source IP/device, and session activity.

**Likely causes:** push-based MFA without number matching; no rate limit; no conditional access; a user conditioned to approve.

**Fix:** number matching or phishing-resistant keys, rate limiting, conditional access, impossible-travel alerts, and user reporting.

### J — A dependency or base image is compromised

**Symptoms:** a new image tag pulls a malicious package; the build pipeline runs untrusted code.

**Evidence:** image digest, SBOM, signature, build logs, dependency lockfile, and registry audit.

**Likely causes:** mutable tags, no signature verification, `latest`, a typosquatted package, a compromised maintainer token.

**Fix:** pin by digest, verify signatures, generate an SBOM, use a private proxy with allowlisting, rebuild from a trusted base, and rotate build credentials.

---

## 10. Command field manual

### `ss` — listening ports and connections

```bash
sudo ss -tulpn
sudo ss -tupn state established
```

| Flag | Meaning |
|---|---|
| `-t` | TCP |
| `-u` | UDP |
| `-l` | listening |
| `-p` | process (needs privilege for other users) |
| `-n` | numeric ports/addresses |
| `state established` | filter by socket state |

Use it to answer: what is exposed, on which interface, by which process, and who is connected. A listener on `0.0.0.0` with no firewall is an exposure. A connection to an unknown IP is a triage lead, not proof of compromise.

Mistake: running `ss` without `sudo`, seeing no process names, and concluding nothing is listening.

### `sudo -l` — effective sudo rights

```bash
sudo -l
sudo -l -U ci-runner
```

Use it to find `NOPASSWD`, wildcards, and command paths that can execute a shell. `sudo -l` as a non-root user may ask for the user's password; `-U` requires root.

Mistake: assuming a rule is safe because it names a specific binary. Check whether the binary can spawn a shell or write a file the rule then executes.

### `find` — SUID, world-writable, and key permissions

```bash
sudo find / -xdev -type f -perm -4000 -printf '%m %u %g %p\n' 2>/dev/null
sudo find / -xdev -type f -perm -0002 -printf '%m %u %g %p\n' 2>/dev/null
sudo find /etc /var/spool/cron -type f -perm /o+w 2>/dev/null
```

- `-xdev` stays on one filesystem, avoiding network mounts.
- `-perm -4000` means "all of these bits set" (SUID).
- `-perm -0002` means world-writable.
- `-printf` gives mode/owner/group/path for triage.

Mistake: running an unrestricted `find /` on a large network filesystem and calling it a security scan. Bound the path, exclude `/proc`, `/sys`, and mounts.

### `getcap` / `capsh` — Linux capabilities

```bash
sudo getcap -r / 2>/dev/null
capsh --print
```

`getcap -r /` finds file capabilities; `capsh --print` shows the current process's capability sets. Look for `cap_setuid`, `cap_setgid`, `cap_sys_admin`, `cap_sys_ptrace`, and `cap_dac_override` on unexpected binaries. A binary with `cap_setuid+ep` can become any user.

Mistake: treating `cap_net_raw` on `ping` as a critical finding while ignoring `cap_sys_admin` on a custom tool.

### `systemd-analyze security` — service sandbox score

```bash
systemd-analyze security sshd.service
systemd-analyze security --offline=true /etc/systemd/system/myapp.service
```

Shows which sandboxing directives are missing and an exposure score. Use it to improve a unit file, not to "fix" a score. SSH and other privileged services cannot be fully sandboxed without losing function.

Mistake: adding `ProtectSystem=strict` to a service that legitimately writes to `/var/lib` without adding `ReadWritePaths`, then wondering why it fails.

### `lynis` — host audit

```bash
sudo lynis audit system
sudo lynis show profiles
```

Produces a hardening index and suggestions across boot, kernel, auth, logging, and services. Read the suggestions; do not apply them blindly. Record accepted risks.

Mistake: running Lynis on a production host during peak hours and being surprised by the I/O; schedule it or run it on a clone.

### `trivy` — image and filesystem scanning

```bash
trivy image --severity HIGH,CRITICAL --ignore-unfixed registry.example.com/api:1.4.2
trivy fs --scanners vuln,secret,misconfig .
trivy k8s --report summary cluster
```

- `image` scans the image layers; use the digest you deploy.
- `fs` scans the working tree for vulnerabilities, secrets, and misconfigurations.
- `k8s` scans cluster resources for misconfigurations.
- `--ignore-unfixed` prioritizes actionable CVEs but hides the rest; state that in the report.

Mistake: scanning `nginx:latest` and reporting "clean" while deploying a different digest.

### `kube-bench` — CIS Kubernetes benchmark

```bash
kube-bench run --targets master,node
kube-bench run --targets node --benchmark cis-1.8
```

Checks kubelet, API server, etcd, and node configuration against the CIS benchmark. Run it on the node or as a job with the right host mounts; results include `PASS`, `FAIL`, `WARN`, and remediation text.

Mistake: running it inside a pod without host access and treating "not applicable" as "secure."

### `kubectl auth can-i` — authorization checks

```bash
kubectl auth can-i --list
kubectl auth can-i create pods --namespace checkout
kubectl auth can-i get secrets --as=system:serviceaccount:checkout:api -n checkout
kubectl auth can-i '*' '*' --all-namespaces
```

Use it to verify RBAC from the subject's perspective. `--as` requires impersonation permission. `--list` shows the effective rules, which is faster than reading every binding.

Mistake: testing as a cluster-admin and concluding the app's service account is fine.

### `openssl s_client` — TLS inspection

```bash
openssl s_client -connect api.example.com:443 -servername api.example.com -showcerts </dev/null
openssl s_client -connect api.example.com:443 -servername api.example.com -tls1_2 </dev/null 2>&1 | grep -E 'Protocol|Cipher'
```

- `-servername` sends SNI; without it you may get the wrong certificate on a shared IP.
- `-showcerts` prints the chain.
- `</dev/null` prevents the command from waiting for stdin.
- `-tls1_2` tests a specific version; the server may reject it.

Mistake: testing the IP instead of the hostname, then reporting a certificate mismatch that real clients do not see.

### `gitleaks` — secret scanning

```bash
gitleaks detect --source . --redact --verbose
gitleaks protect --staged --redact
```

`detect` scans the working tree and history; `protect` scans staged changes. `--redact` avoids printing the secret again. Add it to pre-commit and CI.

Mistake: finding a secret, deleting the file, and not rotating the credential. The secret is already leaked.

### `auditctl` / `ausearch` — host audit

```bash
sudo auditctl -w /etc/passwd -p wa -k identity
sudo auditctl -l
sudo ausearch -k identity -ts recent
sudo aureport --auth --summary
```

- `-w` path, `-p` permissions (`r` read, `w` write, `x` execute, `a` attribute), `-k` key.
- Rules must be persisted under `/etc/audit/rules.d/`.
- `ausearch` queries the audit log; `aureport` summarizes.

Mistake: logging every `execve` on a busy host without sizing the disk and the log pipeline, then losing the audit trail to rotation.

### Cloud CLI — quick audit

```bash
aws sts get-caller-identity
aws iam list-access-keys --user deploy
aws s3api get-public-access-block --bucket opslab-artifacts
aws cloudtrail lookup-events --lookup-attributes AttributeKey=EventName,AttributeValue=CreateAccessKey --max-results 10
aws ec2 describe-security-groups --query 'SecurityGroups[?IpPermissions[?IpRanges[?CidrIp==`0.0.0.0/0`]]].[GroupId,GroupName]'
```

Use `get-caller-identity` first: you need to know which principal and account you are auditing. The security group query is a starting point; it does not cover IPv6 or prefix lists.

Mistake: running audit commands with an admin role and leaving the session credentials in shell history or CI logs.

### Kubernetes pod security snapshot

```bash
kubectl get pods -A -o json | jq -r '
  .items[] |
  select(.spec.hostNetwork or .spec.hostPID or
         ([.spec.containers[].securityContext.privileged // false] | any)) |
  [.metadata.namespace, .metadata.name,
   (.spec.hostNetwork // false), (.spec.hostPID // false)] | @tsv'
```

Use it to find the obvious high-risk pods. It does not catch hostPath, added capabilities, or a root user; pair it with admission policy and a proper scanner.

Mistake: assuming no output means the cluster is secure. It means these three fields are clean.

---

## 11. Security review checklist

### Identity

- [ ] No long-lived cloud keys for workloads or CI.
- [ ] Human access uses SSO + MFA; break-glass is monitored.
- [ ] IAM policies are scoped by action and resource; no `*:*` outside audited admin.
- [ ] Kubernetes RBAC has no wildcards for application service accounts.
- [ ] Service account tokens are disabled where not needed.
- [ ] Offboarding removes accounts, keys, and role bindings.

### Network

- [ ] Default deny inbound; only required ports are exposed.
- [ ] SSH/RDP/databases are not open to the internet.
- [ ] Egress is restricted or monitored; metadata access is blocked where possible.
- [ ] IMDSv2 is required; hop limit is 1.
- [ ] NetworkPolicy is enforced by the CNI and tested.
- [ ] TLS certificates are valid, automated, and monitored for expiry.
- [ ] Internal service-to-service traffic is authenticated (mTLS or equivalent) where required.

### Secrets

- [ ] No secrets in Git, images, logs, or CI output.
- [ ] Secrets are stored in a manager with access control and audit.
- [ ] etcd encryption at rest is enabled.
- [ ] Rotation is automated and tested.
- [ ] A leak has a documented revoke-first procedure.

### Host and container

- [ ] Minimal image, patched, rebuilt regularly.
- [ ] Services run as non-root with dropped capabilities.
- [ ] Read-only root filesystem where possible.
- [ ] No privileged/hostPath pods outside an audited namespace.
- [ ] Admission control enforces the pod security standard.
- [ ] Auditd and Kubernetes audit logs are shipped off-host.
- [ ] Host firewall and listening ports are reviewed.

### Detection and response

- [ ] CloudTrail/audit logs are multi-region and integrity-protected.
- [ ] Alerts exist for identity changes, public storage, privileged pods, and anomalous egress.
- [ ] A security incident has a runbook, owner, and escalation path.
- [ ] Evidence preservation is practiced, not just documented.
- [ ] Post-incident actions add detection and prevention, not just a patch.

---

## 12. Checkpoint

1. What three questions define a security review of a component?
2. Explain STRIDE with one infrastructure example per letter.
3. Why is `sudo` on `systemctl` without a unit restriction effectively root?
4. What is the difference between an IAM trust policy and a permission policy?
5. Why is `get secrets` in a Kubernetes namespace a privileged permission?
6. What does `automountServiceAccountToken: false` prevent?
7. Why is a security group open to `0.0.0.0/0` on port 22 a finding even with key-only auth?
8. How does IMDSv2 reduce SSRF risk, and what still needs to be blocked?
9. What does TLS authenticate, and what does it not authorize?
10. Why can a missing intermediate certificate work in a browser but fail in `curl`?
11. Why are Kubernetes Secrets not secret by default?
12. What are the first two actions after finding a live cloud key in a public repo?
13. What is the difference between a SUID binary and a file capability?
14. Why is `privileged: true` a node-level trust decision?
15. What does `trivy --ignore-unfixed` hide, and how should you report it?
16. Why must audit logs be shipped off the host they describe?
17. What evidence do you preserve before rebooting a suspected compromised host?
18. Why is a CI OIDC trust policy with `repo:org/repo:*` dangerous?
19. How would you detect a crypto-mining workload from infrastructure signals?
20. What makes a security postmortem action different from a reliability action?
