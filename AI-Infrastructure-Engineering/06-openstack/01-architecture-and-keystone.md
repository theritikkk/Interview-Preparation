# Architecture and Keystone

> **The one-sentence model:** OpenStack is a collection of REST APIs that share an identity service; Keystone answers "who are you, which project are you acting in, and where is the service?" Every later failure — scheduling, networking, storage — is preceded by a successful authentication and a service-catalog lookup.

---

## 1. What OpenStack is, and is not

OpenStack is not a single daemon. It is a set of projects, each with its own API, database, configuration, and agents. They collaborate through REST calls, message queues, and a shared identity service.

```text
                       ┌──────────────┐
                       │   Keystone   │  identity + catalog
                       └──────┬───────┘
                              │ token / endpoint
   ┌───────────┬──────────────┼──────────────┬───────────┐
   ▼           ▼              ▼              ▼           ▼
 Glance      Nova         Neutron         Cinder      Swift
 images      compute      networking      block       object
   │           │              │              │
   │           ├── Placement  ├── agents     ├── volume backend
   │           ├── conductor  ├── OVS/OVN    └── iSCSI/RBD/NFS
   │           └── libvirt    └── namespaces
   └── image backend (file/Swift/Ceph)
```

Two planes:

- **Control plane:** APIs, databases, message queue, schedulers, conductors. It accepts requests, stores state, and chooses resources.
- **Data plane:** the actual VM, virtual switch, router namespace, volume attachment, and storage traffic. It can fail while the control plane is healthy.

A useful rule: **the API returning `202 Accepted` or `200 OK` means the request was accepted, not that the data plane works.** `openstack server create` can return an ID while the VM later goes to `ERROR`; `openstack volume create` can return an ID while the volume stays `creating`.

### The shared infrastructure

| Component | Usually | Why it matters |
|---|---|---|
| **RabbitMQ** | Message bus | APIs and agents exchange RPC; a broken bus makes services look "up" but unable to act |
| **MariaDB/MySQL** | Per-service databases | State and quota; a DB outage breaks API writes |
| **Memcached** | Token/cache | Keystone and services cache tokens; stale cache can cause odd auth behavior |
| **HAProxy** | API VIP | A single endpoint may front multiple API processes |
| **etcd** | Some deployments (e.g., OVN) | Distributed state for networking |

You do not need to operate these to debug a VM, but you need to know they exist. "Neutron is down" may mean `neutron-server` is up but RabbitMQ is unreachable.

---

## 2. Keystone: identity and scope

### The object model

| Object | What it is | AWS analogy (imperfect) |
|---|---|---|
| **Domain** | A namespace for users, groups, and projects | An account boundary or identity provider realm |
| **Project** | A tenant/workspace that owns resources | An account/subscription, but resources are scoped to it |
| **User** | A human or service identity | IAM user/role session |
| **Group** | A collection of users | IAM group |
| **Role** | A named permission set assigned to a user/group on a scope | IAM policy attachment, but coarser |
| **Role assignment** | `(actor, role, scope)` | Policy binding |
| **Service** | A registered OpenStack API (nova, neutron, ...) | AWS service |
| **Endpoint** | A URL for a service/interface/region | Regional service endpoint |
| **Token** | A short-lived bearer credential with scope and catalog | STS session token |

**Domain vs project:** a domain contains projects and users; a project is where resources live. A user can have roles in multiple projects. "Tenant" is the older name for project; both appear in commands and docs.

**Role vs policy:** the `admin` role is just a string. What it can do is defined by each service's policy file. A cloud can rename roles or require `system` scope for some APIs. Do not assume `admin` means "everything" without checking.

### Scopes

A token can be scoped to:

- **Project:** the normal case for creating VMs, networks, and volumes. The token contains the project ID and the user's roles on it.
- **Domain:** for domain-level administration (users, projects, groups).
- **System:** for cloud-wide operations (hypervisors, aggregates, some admin APIs). System scope is not the same as project `admin`.

```bash
# Project-scoped token (normal)
source openrc demo demo

# Inspect the token's scope, roles, and catalog
openstack token issue -f yaml
```

Example (abridged):

```yaml
expires_at: '2026-10-27T12:00:00.000000Z'
project_id: 3f2b...
user_id: 9a1c...
roles:
- name: member
catalog:
- name: nova
  type: compute
  endpoints:
  - interface: public
    region: RegionOne
    url: https://api.example.com/compute/v2.1
```

If `project_id` is missing, you have an unscoped token; most resource APIs will reject it. If the roles are not what you expect, check `openstack role assignment list`.

### Authentication flow

```text
openstack CLI / SDK
  │  POST /v3/auth/tokens  (user, password, project scope)
  ▼
Keystone
  │  validates domain/user/password, resolves roles, builds catalog
  ▼
token (Fernet/JWT) with expiry, project, roles, catalog
  │
  ▼
client calls Nova/Neutron/Cinder with X-Auth-Token: <token>
  │
  ▼
service validates token (locally or via Keystone), checks policy
```

A **Fernet** token is encrypted and self-contained; Keystone does not need to store every token, but it does keep revocation events. A **JWT** token is signed. Either way, the token is a bearer credential: anyone who has it can act as you until it expires.

### `openrc` and `clouds.yaml`

The classic `openrc` exports environment variables:

```bash
export OS_AUTH_URL=https://api.example.com/identity/v3
export OS_IDENTITY_API_VERSION=3
export OS_PROJECT_NAME=demo
export OS_USERNAME=alice
export OS_PASSWORD='...'
export OS_USER_DOMAIN_NAME=Default
export OS_PROJECT_DOMAIN_NAME=Default
export OS_REGION_NAME=RegionOne
export OS_INTERFACE=public
```

A safer, modern approach is `clouds.yaml`:

```yaml
clouds:
  opslab:
    auth:
      auth_url: https://api.example.com/identity/v3
      username: alice
      password: !secret
      project_name: demo
      user_domain_name: Default
      project_domain_name: Default
    region_name: RegionOne
    interface: public
    identity_api_version: 3
```

```bash
chmod 600 ~/.config/openstack/clouds.yaml
export OS_CLOUD=opslab
openstack token issue
```

`OS_CLOUD` selects a cloud; `--os-cloud` overrides it for one command. Prefer application credentials or a scoped service account for automation; do not put a human password in CI.

### Application credentials

```bash
openstack application credential create opslab-ci \
  --role member \
  --expiration 2026-12-31T23:59:59
```

The secret is shown once. Use it with:

```bash
export OS_AUTH_TYPE=v3applicationcredential
export OS_APPLICATION_CREDENTIAL_ID=...
export OS_APPLICATION_CREDENTIAL_SECRET=...
```

Application credentials are scoped to a project and cannot be used to create other application credentials unless explicitly allowed. They are still bearer secrets; rotate and revoke them.

### Service catalog and endpoints

```bash
openstack service list
openstack endpoint list --service compute
openstack catalog list
```

Example:

```text
+----------------------------------+-----------+-----------+
| ID                               | Name      | Type      |
+----------------------------------+-----------+-----------+
| 4d3...                           | nova      | compute   |
| 5e4...                           | neutron   | network   |
| 6f5...                           | cinder    | volumev3  |
+----------------------------------+-----------+-----------+

+----------------------------------+-----------+---------+---------+----------------------+
| ID                               | Region    | Service | Interface | URL                  |
+----------------------------------+-----------+---------+---------+----------------------+
| 7a6...                           | RegionOne | nova    | public  | https://.../v2.1     |
| 7a6...                           | RegionOne | nova    | internal| http://.../v2.1      |
| 7a6...                           | RegionOne | nova    | admin   | http://.../v2.1      |
+----------------------------------+-----------+---------+---------+----------------------+
```

Three interfaces:

- **public:** reachable by tenants, usually TLS and a public DNS name.
- **internal:** reachable inside the cloud network, used by services.
- **admin:** restricted to operators, often on a management network.

A common failure is a service URL that resolves from the control plane but not from the client, or an endpoint in the wrong region. `openstack endpoint list` is evidence; guessing from `/etc/hosts` is not.

### Role assignments

```bash
openstack role assignment list --names --project demo
openstack role assignment list --user alice --project demo --names
openstack role add --project demo --user alice member
openstack role remove --project demo --user alice member
```

A role assignment is a triple: **who** (user/group), **what** (role), **where** (project/domain/system). If a user can see a VM but cannot create one, check the role assignment on the project, the service policy, and the quota — in that order.

Groups simplify administration:

```bash
openstack group create developers
openstack group add user developers alice
openstack role add --project demo --group developers member
```

### Service users

Each OpenStack service has a service user (e.g., `nova`, `neutron`, `cinder`, `glance`) with roles in a service project. The service uses its own token to call other services (Nova → Glance for an image, Nova → Neutron for a port, Cinder → Nova for an attach). If a service user loses a role, tenant requests can fail with a 401/403 in the service log even though the tenant's own token is valid.

When you see an error in `nova-compute.log` like `Unauthorized: ...`, do not reset the tenant's password. Check the service user, its role assignment, and the service endpoint.

---

## 3. Command field manual

### `openstack token issue`

**Purpose:** prove who you are, which project you are in, when the token expires, and which catalog you received.

```bash
openstack token issue -f yaml
```

**Interpretation:** `project_id` must be present for project-scoped work; `roles` must include the role the policy expects; `catalog` must contain the service/endpoint you are about to call. `expires_at` tells you whether a cached shell is stale.

**Mistake:** assuming the shell is using the project you think. `OS_PROJECT_NAME` can be overridden by a `--os-project-name` flag or a stale `OS_CLOUD`.

### `openstack catalog list`

```bash
openstack catalog list
openstack catalog show compute
```

**Interpretation:** the catalog is per token and per region. A missing service means the service is not registered or the endpoint is not visible to your project; it does not necessarily mean the service is down.

### `openstack endpoint list`

```bash
openstack endpoint list --service compute --interface public
openstack endpoint list --region RegionOne
```

**Interpretation:** `public`/`internal`/`admin` URLs, region, and enabled state. An endpoint can be registered but disabled; a URL can be wrong. Test it with `curl -I` or `openstack --debug`.

### `openstack role assignment list`

```bash
openstack role assignment list --names --project demo
openstack role assignment list --user alice --names
```

**Interpretation:** who has which role where. A role on the wrong project or domain is a common cause of "I can see it in the console but not the CLI."

### `openstack project list` / `user list` / `domain list`

```bash
openstack project list
openstack user list --domain Default
openstack domain list
```

**Interpretation:** existence and enabled state. A disabled project or user produces authentication/authorization failures that look like policy problems.

### `openstack service list`

```bash
openstack service list
```

**Interpretation:** registered services and types. A service can be registered but have no healthy API process; pair this with the process/endpoint check.

### `openstack --debug`

```bash
openstack --debug server list
```

**Why:** shows the HTTP request, response status, and the endpoint used. It is the fastest way to distinguish 401 (authentication), 403 (authorization), 404 (wrong endpoint/ID), and 500 (service error).

**Safety:** debug output can include tokens, passwords, and full request bodies. Do not paste it into a ticket without redaction.

---

## 4. Failure scenarios

### A — `401 Unauthorized` before any resource call

```text
$ openstack server list
The request you have made requires authentication. (HTTP 401)
```

Fault domain: authentication. Check, in order:

1. `OS_AUTH_URL` points at the Keystone v3 endpoint, not an old v2 URL.
2. `OS_USERNAME`, `OS_PASSWORD`, `OS_USER_DOMAIN_NAME`, `OS_PROJECT_NAME`, `OS_PROJECT_DOMAIN_NAME` are set and consistent.
3. The user/project is enabled (`openstack user show`, `openstack project show`).
4. The token has not expired; re-source the `openrc` or `openstack token issue`.
5. Clock skew is small; a large skew breaks token validation.
6. If using application credentials, the ID/secret pair is correct and not revoked.

Do not start by resetting the password. A wrong domain or project scope produces the same 401.

### B — `403 Forbidden` after authentication

```text
$ openstack server create ...
You are not authorized to perform the requested action. (HTTP 403)
```

Fault domain: authorization/policy. The token is valid but lacks a role or scope. Check:

- `openstack token issue` roles and project;
- `openstack role assignment list --user <user> --project <project> --names`;
- the service policy for that action (default roles may require `admin` or `system` scope);
- whether the action is project-scoped or system-scoped (e.g., hypervisor list);
- quota is a separate error (`403` can also be quota in some services; read the message).

### C — `404 Not Found` for a service you know exists

Fault domain: endpoint/catalog/region. Check `openstack endpoint list --service <service>` and `openstack catalog show <service>`. A wrong region or a public URL that does not route from your network produces a 404/connection error, not a 403.

### D — Works in Horizon, fails in CLI

The dashboard may use a different project, region, or auth method. Check the project selector in Horizon and compare with `openstack token issue`. Check `OS_CLOUD`, `OS_PROJECT_NAME`, `OS_REGION_NAME`, and any `--os-*` flags.

### E — Service user failure

```text
nova-compute.log: Unauthorized: ... (HTTP 401)
```

The tenant request may be valid, but Nova's service user cannot call Glance/Neutron/Cinder. Check the service user's role assignment, the service project, and the endpoint. This is an operator problem; do not ask the tenant to re-login.

### F — Token works, then suddenly stops

Possible causes: token expiry, Keystone restart with a changed Fernet key (old tokens invalid), revocation, clock skew, or a load balancer routing to a node with a different key. Reissue the token; if it fails consistently, check Keystone logs and the key repository.

---

## 5. Security and operational hygiene

1. **Least privilege:** use a project-scoped `member` for normal work; reserve `admin`/system scope for operators. Do not share the admin `openrc`.
2. **No secrets in Git:** `openrc`, `clouds.yaml`, application credential secrets, and tokens are credentials. Use a secret manager or environment injection; `chmod 600` is the minimum.
3. **Short-lived tokens:** prefer application credentials with expiration for CI; rotate them. A token in a shell history or debug log is a leak.
4. **Separate projects:** dev/test/prod should be separate projects with quotas; do not rely on naming conventions for isolation.
5. **Quotas:** set them per project. A quota error is a control-plane guardrail, not a scheduler bug.
6. **Audit:** Keystone, Nova, Neutron, and Cinder logs record the request ID. Correlate the client's `X-Openstack-Request-Id` with the service log; that is faster than searching by timestamp.
7. **Do not debug with `--debug` in a shared terminal** if the output may be captured; redact tokens.

---

## 6. Checkpoint

1. What is the difference between a domain and a project?
2. What is a project-scoped token vs a system-scoped token?
3. What does the service catalog contain, and why is it per token?
4. What is the difference between the public, internal, and admin endpoint interfaces?
5. What is a role assignment? Name its three parts.
6. Why can a valid token still get a 403?
7. Why can a valid tenant token still cause a 401 in `nova-compute.log`?
8. What does `openstack token issue` prove, and what does it not prove?
9. What is the first check for a 401?
10. What is the first check for a 403?
11. Why is a token a bearer credential, and what follows for handling?
12. What is an application credential, and why is it better for CI than a password?
13. What does `openstack --debug` show, and why should you redact it?
14. Why is "the API returned 200" not proof that the data plane works?
15. What is the difference between the control plane and the data plane in OpenStack?

### Answers

1. A domain is a namespace for identity objects (users, groups, projects); a project is a resource-owning tenant inside a domain. Users can have roles on multiple projects.
2. A project-scoped token is limited to one project and carries roles there; a system-scoped token is for cloud-wide operations and is not the same as project `admin`.
3. It maps service types/names to endpoints per region/interface. It is built into the token at issuance, so changing an endpoint may require a new token.
4. Public is for tenants and usually TLS; internal is for service-to-service traffic inside the cloud; admin is restricted to operators.
5. A binding of an actor (user/group), a role, and a scope (project/domain/system).
6. The token is valid but the policy denies the action: missing role, wrong project/scope, or a policy that requires a different role.
7. Nova's service user cannot authenticate/authorize to another service (Glance, Neutron, Cinder); the tenant's token is unrelated to that service-to-service call.
8. It proves the authenticated identity, project scope, roles, expiry, and catalog. It does not prove authorization for a specific action or that the service/data plane is healthy.
9. Check the auth URL, domain, project scope, credentials, user/project enabled state, and token expiry.
10. Check the token's roles/project, role assignments, and the service policy/scope for that action.
11. Anyone who holds it can act as the identity until expiry; never log, commit, or paste it, and prefer short-lived scoped tokens.
12. A scoped, expiring credential for automation that avoids storing a human password and can be revoked independently.
13. The HTTP requests/responses, endpoint, status, and possibly tokens/bodies. Redact credentials before sharing.
14. The API accepted the request; scheduling, agent, network, or storage can still fail asynchronously.
15. The control plane accepts/stores/chooses; the data plane runs the VM, forwards packets, and attaches storage. Each has separate failure modes and evidence.
