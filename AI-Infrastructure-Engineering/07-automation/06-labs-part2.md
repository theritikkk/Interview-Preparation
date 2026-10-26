# Phase 7 Labs — Part 2 (L7.3–L7.4)

> **Safety:** L7.3 changes a disposable Linux VM's nginx configuration and creates a system user. Snapshot the VM first, keep a console session open, and do not run it against your laptop or a production host. L7.4 uses GitHub Actions and an environment approval; it does not create cloud resources or store cloud keys. The optional GitOps stretch creates a local kind cluster and must be cleaned up.
>
> **Cost:** L7.3 is free on a local VM. L7.4 is free on GitHub's public-repo tier; private-repo minutes and environment protection rules depend on your plan.

---

# Lab L7.3 — Converge a VM with Ansible

## Objective

Build a small Ansible project that installs nginx, creates a service account, deploys a validated configuration and a secret, starts/reloads the service, and verifies the health endpoint. Prove idempotence by running the playbook twice, preview changes with `--check --diff`, and recover from a bad configuration without taking the service down.

## Prerequisites

- A disposable Ubuntu 22.04/24.04 VM reachable over SSH.
- SSH key access and passwordless `sudo` for the login user (or a sudo password you can supply interactively).
- `ansible-core`, `ansible-lint`, and `git` on your workstation (macOS or Linux).
- 60–90 minutes.

## Setup

On your workstation:

```bash
mkdir -p ~/opslab/phase7/l7.3
cd ~/opslab/phase7/l7.3

python3 -m venv .venv
source .venv/bin/activate
python -m pip install -U pip
pip install ansible-core ansible-lint

ansible --version
ansible-lint --version
```

Create the layout:

```text
l7.3/
├── ansible.cfg
├── inventory.ini
├── site.yml
├── teardown.yml
├── group_vars/
│   └── all/
│       ├── vars.yml
│       └── vault.yml        # encrypted
└── roles/
    └── baseline/
        ├── tasks/
        │   └── main.yml
        ├── handlers/
        │   └── main.yml
        ├── defaults/
        │   └── main.yml
        └── templates/
            ├── nginx.conf.j2
            └── index.html.j2
```

```bash
mkdir -p group_vars/all roles/baseline/{tasks,handlers,defaults,templates}
```

Snapshot the VM now. If the playbook locks you out, you will restore the snapshot or use the console.

## Part A — Inventory and config

`ansible.cfg`:

```ini
[defaults]
inventory = inventory.ini
host_key_checking = True
interpreter_python = auto_silent
stdout_callback = yaml
retry_files_enabled = False

[privilege_escalation]
become = True
become_method = sudo
become_ask_pass = False
```

`inventory.ini` (replace the address, user, and key):

```ini
[webservers]
opslab-vm ansible_host=203.0.113.10 ansible_user=ubuntu ansible_ssh_private_key_file=~/.ssh/id_ed25519

[all:vars]
ansible_python_interpreter=/usr/bin/python3
```

Test connectivity before writing any task:

```bash
ansible-inventory --graph
ansible all -m ansible.builtin.ping
ansible all -m ansible.builtin.setup -a 'filter=ansible_distribution*'
```

Expected:

```text
@all:
  |--@ungrouped:
  |--@webservers:
  |  |--opslab-vm

opslab-vm | SUCCESS => {
    "changed": false,
    "ping": "pong"
}

opslab-vm | SUCCESS => {
    "ansible_facts": {
        "ansible_distribution": "Ubuntu",
        "ansible_distribution_major_version": "24",
        ...
    },
    "changed": false
}
```

If `ping` fails, fix SSH, the key path (`0600`), the security group/firewall, or the host key before continuing. Do not disable host-key checking globally.

`group_vars/all/vars.yml`:

```yaml
---
app_user: opslab
app_root: /srv/opslab
http_port: 8080
server_name: opslab.local
```

Create the encrypted vault file:

```bash
export EDITOR=nano
ansible-vault create group_vars/all/vault.yml
```

Enter a vault password you will remember. In the editor, put:

```yaml
---
vault_app_secret: "lab-7f3c9a1e5b2d4c6f"
```

Save and exit. Verify it is encrypted:

```bash
head -1 group_vars/all/vault.yml
# $ANSIBLE_VAULT;1.1;AES256
ansible-vault view group_vars/all/vault.yml
```

Do not commit the vault password or paste the secret into your write-up. For convenience in this lab, create a password file outside the repo:

```bash
printf '%s\n' 'replace-with-your-vault-password' > ~/.ansible-vault-pass
chmod 600 ~/.ansible-vault-pass
```

Use `--vault-password-file ~/.ansible-vault-pass` in the commands below (or use `--ask-vault-pass`). A password file with a weak password is not production-grade; a secret manager or an interactive prompt is better.

## Part B — Role defaults, handlers, and tasks

`roles/baseline/defaults/main.yml`:

```yaml
---
app_user: opslab
app_root: /srv/opslab
http_port: 8080
server_name: opslab.local
```

Defaults are intentionally overridable; `group_vars/all/vars.yml` currently wins.

`roles/baseline/handlers/main.yml`:

```yaml
---
- name: Reload nginx
  ansible.builtin.service:
    name: nginx
    state: reloaded
```

A handler runs once at the end of the play, no matter how many tasks notify it. It runs only if at least one notifying task reported `changed`.

`roles/baseline/tasks/main.yml`:

```yaml
---
- name: Update apt cache
  ansible.builtin.apt:
    update_cache: true
    cache_valid_time: 3600
  when: ansible_facts['os_family'] == 'Debian'
  tags: [packages]

- name: Install nginx
  ansible.builtin.package:
    name: nginx
    state: present
  tags: [packages]

- name: Create app group
  ansible.builtin.group:
    name: "{{ app_user }}"
    system: true
  tags: [users]

- name: Create app user
  ansible.builtin.user:
    name: "{{ app_user }}"
    group: "{{ app_user }}"
    system: true
    shell: /usr/sbin/nologin
    create_home: false
  tags: [users]

- name: Create app root
  ansible.builtin.file:
    path: "{{ app_root }}"
    state: directory
    owner: root
    group: "{{ app_user }}"
    mode: "0755"
  tags: [config]

- name: Deploy site content
  ansible.builtin.template:
    src: index.html.j2
    dest: "{{ app_root }}/index.html"
    owner: root
    group: "{{ app_user }}"
    mode: "0644"
  notify: Reload nginx
  tags: [config]

- name: Deploy app environment file
  ansible.builtin.copy:
    content: |
      APP_SECRET={{ vault_app_secret }}
      APP_ENV=lab
    dest: "{{ app_root }}/app.env"
    owner: root
    group: "{{ app_user }}"
    mode: "0600"
  no_log: true
  notify: Reload nginx
  tags: [config]

- name: Back up the original nginx configuration
  ansible.builtin.copy:
    src: /etc/nginx/nginx.conf
    dest: /etc/nginx/nginx.conf.opslab-orig
    remote_src: true
    force: false
    mode: "0644"
  tags: [config]

- name: Deploy nginx configuration
  ansible.builtin.template:
    src: nginx.conf.j2
    dest: /etc/nginx/nginx.conf
    owner: root
    group: root
    mode: "0644"
    validate: "nginx -t -c %s"
  notify: Reload nginx
  tags: [config]

- name: Ensure nginx is running
  ansible.builtin.service:
    name: nginx
    state: started
    enabled: true
  tags: [service]

- name: Flush handlers before verification
  ansible.builtin.meta: flush_handlers
  tags: [service]

- name: Verify the health endpoint
  ansible.builtin.uri:
    url: "http://127.0.0.1:{{ http_port }}/health"
    status_code: 200
    return_content: true
  register: health
  until: health.status == 200
  retries: 5
  delay: 2
  changed_when: false
  tags: [verify]

- name: Check the secret file metadata
  ansible.builtin.stat:
    path: "{{ app_root }}/app.env"
  register: secret_file
  changed_when: false
  tags: [verify]

- name: Assert the secret file is private
  ansible.builtin.assert:
    that:
      - secret_file.stat.exists
      - secret_file.stat.mode == '0600'
    fail_msg: "{{ app_root }}/app.env is missing or not mode 0600"
  tags: [verify]
```

Why deploy the **full** `nginx.conf` instead of a snippet? The `validate` option runs against the temporary file that Ansible is about to install. If you deploy a snippet to `conf.d/`, `nginx -t -c %s` treats the snippet as a full config and fails (a `server` block is not allowed at the top level). A full config can be validated exactly as it will be installed. On a disposable lab VM, replacing the distro config is acceptable; the playbook backs up the original first.

`roles/baseline/templates/nginx.conf.j2`:

```nginx
user www-data;
worker_processes auto;
pid /run/nginx.pid;

events {
    worker_connections 768;
}

http {
    sendfile on;
    tcp_nopush on;
    types_hash_max_size 2048;

    include /etc/nginx/mime.types;
    default_type application/octet-stream;

    access_log /var/log/nginx/access.log;
    error_log /var/log/nginx/error.log;

    server {
        listen {{ http_port }};
        server_name {{ server_name }};

        location = /health {
            access_log off;
            default_type text/plain;
            return 200 "ok";
        }

        location / {
            root {{ app_root }};
            index index.html;
        }
    }
}
```

`roles/baseline/templates/index.html.j2`:

```html
<!doctype html>
<html lang="en">
  <head>
    <meta charset="utf-8">
    <title>{{ inventory_hostname }}</title>
  </head>
  <body>
    <h1>OpsLab</h1>
    <p>Host: {{ inventory_hostname }}</p>
    <p>Managed by Ansible. Manual edits will be overwritten.</p>
  </body>
</html>
```

No timestamp or random value. A template that changes on every render is not idempotent.

## Part C — Playbook and teardown

`site.yml`:

```yaml
---
- name: Baseline web host
  hosts: webservers
  become: true
  serial: 1
  any_errors_fatal: true

  pre_tasks:
    - name: Require a Debian-family host for this lab
      ansible.builtin.assert:
        that:
          - ansible_facts['os_family'] == 'Debian'
        fail_msg: "This lab's paths and package names assume Debian/Ubuntu."

  roles:
    - baseline
```

`serial: 1` updates one host at a time. With a single lab VM it does not change much, but it is the habit you want when the inventory grows. `any_errors_fatal` stops the play instead of continuing to other hosts after a failure.

`teardown.yml`:

```yaml
---
- name: Remove the lab web configuration
  hosts: webservers
  become: true

  tasks:
    - name: Check for the original nginx config backup
      ansible.builtin.stat:
        path: /etc/nginx/nginx.conf.opslab-orig
      register: nginx_backup

    - name: Restore the original nginx configuration
      ansible.builtin.copy:
        src: /etc/nginx/nginx.conf.opslab-orig
        dest: /etc/nginx/nginx.conf
        remote_src: true
        mode: "0644"
      when: nginx_backup.stat.exists
      notify: Reload nginx

    - name: Remove the app root
      ansible.builtin.file:
        path: "{{ app_root }}"
        state: absent

    - name: Remove the app user
      ansible.builtin.user:
        name: "{{ app_user }}"
        state: absent
        remove: true

    - name: Remove the app group
      ansible.builtin.group:
        name: "{{ app_user }}"
        state: absent

  handlers:
    - name: Reload nginx
      ansible.builtin.service:
        name: nginx
        state: reloaded
```

Teardown restores the original nginx config and removes the lab user/content. It leaves nginx installed. If nginx was already serving something on this VM, do not run this lab on it.

## Part D — Lint and first convergence

```bash
ansible-playbook site.yml --syntax-check
ansible-lint site.yml
```

Expected:

```text
playbook: site.yml
```

`ansible-lint` may print warnings about task names, file modes, or variable naming. Fix real issues; do not disable a rule without a written reason.

First convergence:

```bash
ansible-playbook site.yml --ask-vault-pass
```

Representative output (task order and counts vary slightly):

```text
PLAY [Baseline web host] *******************************************************

TASK [Gathering Facts] *********************************************************
ok: [opslab-vm]

TASK [Require a Debian-family host for this lab] *******************************
ok: [opslab-vm]

TASK [baseline : Update apt cache] *********************************************
changed: [opslab-vm]

TASK [baseline : Install nginx] ************************************************
changed: [opslab-vm]

TASK [baseline : Create app group] *********************************************
changed: [opslab-vm]

TASK [baseline : Create app user] **********************************************
changed: [opslab-vm]

TASK [baseline : Create app root] **********************************************
changed: [opslab-vm]

TASK [baseline : Deploy site content] ******************************************
changed: [opslab-vm]

TASK [baseline : Deploy app environment file] **********************************
changed: [opslab-vm]

TASK [baseline : Back up the original nginx configuration] *********************
changed: [opslab-vm]

TASK [baseline : Deploy nginx configuration] ***********************************
changed: [opslab-vm]

TASK [baseline : Ensure nginx is running] **************************************
changed: [opslab-vm]

TASK [baseline : Flush handlers before verification] ***************************
RUNNING HANDLER [baseline : Reload nginx] **************************************
changed: [opslab-vm]

TASK [baseline : Verify the health endpoint] ***********************************
ok: [opslab-vm]

TASK [baseline : Check the secret file metadata] *******************************
ok: [opslab-vm]

TASK [baseline : Assert the secret file is private] ****************************
ok: [opslab-vm]

PLAY RECAP *********************************************************************
opslab-vm                  : ok=16   changed=10   unreachable=0    failed=0    skipped=0    rescued=0    ignored=0
```

Verify from your workstation:

```bash
curl -sS http://<vm-address>:8080/health
# ok

curl -sS http://<vm-address>:8080/ | head -5
# <!doctype html>
# <html lang="en">
#   <head>
# ...

ssh ubuntu@<vm-address> 'sudo stat -c "%a %U:%G %n" /srv/opslab/app.env'
# 600 root:opslab /srv/opslab/app.env

ssh ubuntu@<vm-address> 'sudo nginx -t'
# nginx: configuration file /etc/nginx/nginx.conf test is successful
```

Do **not** `cat` `app.env` into a terminal or a write-up. Check the mode and owner; that is enough.

## Part E — Prove idempotence

Run the same playbook again:

```bash
ansible-playbook site.yml --ask-vault-pass
```

Expected recap:

```text
opslab-vm                  : ok=16   changed=0    unreachable=0    failed=0    skipped=0    rescued=0    ignored=0
```

Now preview a no-op change:

```bash
ansible-playbook site.yml --check --diff --ask-vault-pass
```

Expected: `changed=0`, no diff. The health check and stat tasks are read-only (`changed_when: false`); the handlers do not run because nothing notified them.

If `--check` fails on a fresh host because nginx is not installed and the template's `validate` cannot find the binary, that is expected. Converge normally first on a disposable host, then use check mode for subsequent changes.

## Internal explanation

### Facts

`gather_facts` runs the setup module before tasks. `ansible_facts['os_family']` lets the same role branch between Debian and RHEL families. The lab asserts Debian because the package name, user, and paths differ on RHEL; a real role would include `vars/Debian.yml` and `vars/RedHat.yml`.

### Modules vs `command`

`apt`, `package`, `user`, `group`, `file`, `template`, `copy`, and `service` are declarative. Ansible compares the current state to the desired state and reports `changed` only when it acts. `command`/`shell` always run unless you add `creates`, `removes`, or `changed_when`. That is why the role avoids them for configuration.

### Handlers

The template and copy tasks notify `Reload nginx`. Ansible records the notification and runs the handler once at the end of the play. `meta: flush_handlers` forces it to run before the health check, so the check tests the new config rather than the old process. If the handler name does not match exactly, or the task does not report `changed`, the handler does not run.

### `validate`

`validate: "nginx -t -c %s"` runs nginx's config test against the temporary file **before** replacing the live file. If the test fails, Ansible aborts the task and leaves the old config in place. This is the difference between "the playbook failed" and "the service is down."

### `no_log`

`no_log: true` suppresses the task's output, including the rendered content and diffs. It does not encrypt the file on disk, and it can hide useful errors. Use it for tasks that handle secrets, and verify the file mode separately. A secret in a template is still plaintext on the target.

### Vault

`ansible-vault` encrypts the file at rest. The play decrypts it in memory when it runs. The vault password is the weak point: a password file with `chmod 600` is better than a password in shell history, but a secret manager is better. Vault does not protect the rendered file on the target, logs from a task without `no_log`, or a secret copied into a backup.

### `serial` and blast radius

`serial: 1` runs the play against one host at a time. With a load balancer, you would also drain the host, wait for connections to finish, upgrade, health-check, and re-add it. Ansible's `serial` alone does not drain traffic. `any_errors_fatal` stops the rollout on the first failure instead of continuing to the rest of the fleet.

### Check mode

`--check` asks modules to predict changes without applying them. Not every module supports it perfectly: a fresh host may lack the binaries or files that the check needs, and `command` tasks may still run unless guarded. Use check mode as a review tool, not as a guarantee.

## Troubleshooting task

Do these in order. After each, collect evidence before fixing.

### 1. A bad nginx config must not replace the good one

Edit `roles/baseline/templates/nginx.conf.j2` and remove the semicolon after `return 200 "ok"`:

```nginx
return 200 "ok"
```

Run:

```bash
ansible-playbook site.yml --ask-vault-pass
```

Expected: the `Deploy nginx configuration` task fails at the `validate` step. The live `/etc/nginx/nginx.conf` is unchanged, nginx is still running, and the health endpoint still returns `ok`.

Evidence:

```bash
curl -sS http://<vm-address>:8080/health
ssh ubuntu@<vm-address> 'sudo nginx -t'
ssh ubuntu@<vm-address> 'sudo grep -n "return 200" /etc/nginx/nginx.conf'
```

Fix the template, rerun, and confirm the handler reloads nginx. Explain why validation must happen before the file replacement, not after the reload.

### 2. A missing `notify` leaves the old process running

Remove `notify: Reload nginx` from the `Deploy nginx configuration` task. Change the health response to `return 200 "ok-v2";`. Run the playbook.

Expected: the task reports `changed`, but the handler never runs. `curl` still returns `ok`, not `ok-v2`. `systemctl show nginx -p ExecMainStartTimestamp` shows the old start time.

Restore `notify`, rerun, and confirm `ok-v2`. Explain the difference between "the file changed" and "the running process uses the file."

### 3. A timestamp task is not idempotent

Add this task temporarily:

```yaml
- name: Write a timestamp
  ansible.builtin.copy:
    content: "{{ ansible_date_time.iso8601 }}\n"
    dest: "{{ app_root }}/timestamp"
    mode: "0644"
```

Run the playbook twice. Expected: `changed=1` both times. The file genuinely changes, so `changed_when: false` would be a lie. Remove the task or redesign it (for example, write a static build ID from a variable). Explain why masking a real change is worse than reporting it.

### 4. Wrong vault password

Create a temporary wrong password file:

```bash
printf '%s\n' 'wrong-password' > /tmp/wrong-vault-pass
ansible-playbook site.yml --vault-password-file /tmp/wrong-vault-pass
rm -f /tmp/wrong-vault-pass
```

Expected: a decryption failure before any task runs. No partial configuration is applied. Fix the password; do not delete the vault file.

### 5. Target the wrong host

Run with `--limit` on a host that does not exist:

```bash
ansible-playbook site.yml --limit no-such-host --ask-vault-pass
```

Expected: `skipping: no hosts matched` and no changes. Inventory/limit mistakes are a common cause of "Ansible did nothing." Use `ansible-inventory --graph` and `--list-hosts` before a production run.

## Verification

- [ ] `ansible-inventory --graph` shows exactly the intended host.
- [ ] `ansible all -m ping` succeeds.
- [ ] `ansible-playbook site.yml --syntax-check` passes.
- [ ] `ansible-lint site.yml` passes (or every warning is explained).
- [ ] First run reports changes and ends with `failed=0`.
- [ ] Second run reports `changed=0`.
- [ ] `--check --diff` on the converged host reports no changes.
- [ ] `/health` returns `ok` on port 8080.
- [ ] `app.env` is mode `0600`, owned by `root:opslab`.
- [ ] The vault secret is not printed in the play output or your write-up.
- [ ] A bad nginx template fails validation without replacing the live config.
- [ ] Removing `notify` reproduces the "file changed, process unchanged" symptom.
- [ ] Teardown restores the original config and removes the lab user.

## Cleanup

Preferred: restore the VM snapshot.

Or run teardown:

```bash
ansible-playbook teardown.yml --ask-vault-pass
```

Verify:

```bash
curl -sS --max-time 3 http://<vm-address>:8080/health; echo "exit=$?"
# curl: (7) Failed to connect ... (expected: no lab listener on 8080)

ssh ubuntu@<vm-address> 'id opslab'
# id: 'opslab': no such user

ssh ubuntu@<vm-address> 'sudo nginx -t && sudo systemctl is-active nginx'
# nginx: configuration file /etc/nginx/nginx.conf test is successful
# active
```

Remove local artifacts:

```bash
deactivate
rm -f ~/.ansible-vault-pass
rm -rf ~/opslab/phase7/l7.3/.venv
```

Do not delete the VM if you are using it for L7.4's optional self-hosted runner. Do remove the vault password file from the workstation when you are done.

## Write-up template

```markdown
# L7.3 — Ansible convergence

## Inventory
- Host/group:
- Connection user/key:
- Python interpreter:
- OS family:

## Role
- Packages:
- Users/groups:
- Files/templates:
- Handlers:
- Validation:
- Secret handling:

## Evidence
| Check | Command | Result |
|---|---|---|
| Ping | | |
| Syntax check | | |
| Lint | | |
| First run recap | | |
| Second run recap | | |
| Check mode recap | | |
| Health endpoint | | |
| Secret mode | | |
| Bad config validation | | |
| Missing notify | | |
| Teardown | | |

## Failure analysis
- Bad nginx config: symptom ___; evidence ___; why the old config survived ___
- Missing notify: symptom ___; evidence ___; fix ___
- Non-idempotent timestamp: why `changed_when: false` is wrong ___
- Wrong vault password: failure point ___; partial changes ___
- Wrong host/limit: evidence ___; fix ___

## Design notes
- Why a full nginx.conf instead of a conf.d snippet:
- Why `serial: 1` is not the same as draining:
- What `no_log` does and does not protect:
- What I would add for RHEL support:
- What I would add for a real rollout (health checks, rollback, canary):
```

---

# Lab L7.4 — A Gated CI/CD Pipeline

## Objective

Put the L7.2 Python tool under version control, build a GitHub Actions CI pipeline (lint, test matrix, build, checksum, artifact, smoke test), then add a manually triggered deploy workflow that:

- requires an environment approval;
- verifies the source CI run succeeded on `main`;
- downloads the exact artifact from that run;
- verifies its checksum;
- installs it in a clean virtual environment and runs a smoke test;
- requests a GitHub OIDC token and prints only its claims, so you can see the `environment` claim that cloud trust policies use;
- writes a deployment record artifact.

This lab proves the **promotion and gating mechanics** without cloud credentials. The optional stretch adds a real GitOps loop on kind.

## Prerequisites

- The completed L7.2 project (`capacity-report`) with a passing `pytest` suite.
- A GitHub account and `gh` CLI authenticated (`gh auth status`).
- `git`.
- 60–90 minutes.
- Optional stretch: Docker/kind, `kubectl`, and enough RAM for a local cluster.

## Setup

```bash
cd ~/opslab/phase7/l7.2
source .venv/bin/activate

git init
git add pyproject.toml src tests
git commit -m "capacity report with tests"

gh auth status
gh repo create opslab-pipeline --private --source=. --remote=origin --push
```

If the name is taken, choose another. If your plan does not support environment protection rules on private repos, make the repo public. This lab uses **no secrets**, so a public repo is safe. Do not add cloud keys, tokens, or private data.

Confirm the remote and the first push:

```bash
git remote -v
git log --oneline -1
gh repo view --web
```

Create the workflow directory:

```bash
mkdir -p .github/workflows
```

## Part A — CI workflow

`.github/workflows/ci.yml`:

```yaml
name: ci

on:
  pull_request:
  push:
    branches: [main]

permissions:
  contents: read

concurrency:
  group: ci-${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: true

jobs:
  lint:
    runs-on: ubuntu-latest
    timeout-minutes: 10
    steps:
      - uses: actions/checkout@v4
        with:
          persist-credentials: false

      - uses: actions/setup-python@v5
        with:
          python-version: "3.12"
          cache: pip

      - name: Install
        run: |
          python -m pip install --upgrade pip
          pip install -e '.[dev]'

      - name: Ruff
        run: ruff check .

      - name: Mypy
        run: mypy src

  test:
    runs-on: ubuntu-latest
    timeout-minutes: 15
    strategy:
      fail-fast: false
      matrix:
        python-version: ["3.11", "3.12"]
    steps:
      - uses: actions/checkout@v4
        with:
          persist-credentials: false

      - uses: actions/setup-python@v5
        with:
          python-version: ${{ matrix.python-version }}
          cache: pip

      - name: Install
        run: |
          python -m pip install --upgrade pip
          pip install -e '.[dev]'

      - name: Test
        env:
          TZ: UTC
          PYTHONHASHSEED: "0"
        run: pytest -q

  build:
    needs: [lint, test]
    runs-on: ubuntu-latest
    timeout-minutes: 10
    steps:
      - uses: actions/checkout@v4
        with:
          persist-credentials: false

      - uses: actions/setup-python@v5
        with:
          python-version: "3.12"

      - name: Build
        run: |
          python -m pip install --upgrade build
          python -m build

      - name: Checksums
        run: |
          cd dist
          sha256sum ./* > SHA256SUMS
          cat SHA256SUMS

      - uses: actions/upload-artifact@v4
        with:
          name: capacity-report
          path: dist/
          if-no-files-found: error
          retention-days: 14

  smoke:
    needs: build
    runs-on: ubuntu-latest
    timeout-minutes: 10
    steps:
      - uses: actions/checkout@v4
        with:
          persist-credentials: false

      - uses: actions/download-artifact@v4
        with:
          name: capacity-report
          path: dist

      - name: Verify checksum
        run: |
          cd dist
          sha256sum -c SHA256SUMS

      - name: Install the wheel in a clean venv
        run: |
          python3 -m venv .venv-smoke
          .venv-smoke/bin/pip install --no-deps dist/*.whl
          .venv-smoke/bin/capacity-report --help

      - name: Run against fixtures
        run: |
          .venv-smoke/bin/capacity-report \
            --servers tests/fixtures/servers.json \
            --flavors tests/fixtures/flavors.json \
            --limits tests/fixtures/limits.json \
            --format json > /tmp/report.json
          echo "exit=$?"
          jq -e '.status == "over_threshold"' /tmp/report.json
```

Commit and push:

```bash
git add .github/workflows/ci.yml
git commit -m "ci: lint, test, build, smoke"
git push
```

Watch the run:

```bash
gh run list --limit 5
```

Representative output:

```text
STATUS  TITLE                    WORKFLOW  BRANCH  EVENT  ID          ELAPSED  AGE
*       ci: lint, test, build    ci        main    push   4821993311  1m02s    1m
```

```bash
gh run watch 4821993311 --exit-status
echo "exit=$?"
```

Expected: exit 0, all jobs green. If a job fails:

```bash
gh run view 4821993311 --log-failed
```

Read the first failing command, not the final `Process completed with exit code 1`.

### What each job proves

| Job | Proves | Does not prove |
|---|---|---|
| `lint` | Code parses, types are consistent | Runtime behavior |
| `test` | Logic works on Python 3.11/3.12 | Works on a real cloud |
| `build` | Wheel/sdist can be produced | Wheel installs cleanly |
| `smoke` | Wheel installs in a clean venv and runs against fixtures | Production rollout |

The smoke job is the check most pipelines omit. Building a wheel does not prove it installs; installing it does not prove it runs.

## Part B — Download and inspect the artifact

```bash
gh run download 4821993311 -n capacity-report -D /tmp/ci-artifact
ls -l /tmp/ci-artifact
```

Expected:

```text
-rw-r--r--  ... capacity_report-0.1.0-py3-none-any.whl
-rw-r--r--  ... capacity_report-0.1.0.tar.gz
-rw-r--r--  ... SHA256SUMS
```

Verify and install locally:

```bash
cd /tmp/ci-artifact
sha256sum -c SHA256SUMS

python3 -m venv /tmp/ci-venv
/tmp/ci-venv/bin/pip install --no-deps ./*.whl
/tmp/ci-venv/bin/capacity-report --help
```

Expected: `OK` for each file; the help text includes `--servers`, `--flavors`, `--limits`, and `--max-utilization`.

## Part C — Protect the staging environment

In GitHub:

1. Repository → **Settings** → **Environments** → **New environment**.
2. Name it `staging` → **Configure environment**.
3. Enable **Required reviewers** → add your account.
4. Under **Deployment branches**, choose **Selected branches** → add `main`.
5. Save.

Create a second environment `prod` with the same reviewer and, optionally, a wait timer.

If your plan does not offer protection rules for private repos, you will not see the reviewer option. Options:

- make the repo public (it contains no secrets);
- proceed without approval and document that the gate is missing;
- use a manual `workflow_dispatch` as the only gate (weaker).

Do not pretend the approval exists if it does not.

## Part D — Deploy workflow with OIDC claims

`.github/workflows/deploy.yml`:

```yaml
name: deploy

on:
  workflow_dispatch:
    inputs:
      environment:
        description: Target environment
        type: choice
        options: [staging, prod]
        required: true
        default: staging
      run_id:
        description: CI run ID that produced the artifact
        type: string
        required: true

permissions:
  contents: read

concurrency:
  group: deploy-${{ inputs.environment }}
  cancel-in-progress: false

jobs:
  deploy:
    runs-on: ubuntu-latest
    timeout-minutes: 15
    environment:
      name: ${{ inputs.environment }}
      url: https://${{ inputs.environment }}.example.com
    permissions:
      contents: read
      actions: read
      id-token: write

    steps:
      - name: Validate inputs
        env:
          TARGET: ${{ inputs.environment }}
          RUN_ID: ${{ inputs.run_id }}
        run: |
          case "$TARGET" in
            staging|prod) ;;
            *) echo "invalid environment: $TARGET" >&2; exit 2 ;;
          esac
          [[ "$RUN_ID" =~ ^[0-9]+$ ]] || { echo "run_id must be numeric" >&2; exit 2; }

      - name: Verify the source CI run
        id: source
        env:
          GH_TOKEN: ${{ github.token }}
          RUN_ID: ${{ inputs.run_id }}
        run: |
          run=$(gh api "repos/${{ github.repository }}/actions/runs/$RUN_ID")
          echo "$run" | jq '{head_branch, head_sha, conclusion, event}'
          test "$(echo "$run" | jq -r .conclusion)" = "success"
          test "$(echo "$run" | jq -r .head_branch)" = "main"
          echo "sha=$(echo "$run" | jq -r .head_sha)" >> "$GITHUB_OUTPUT"

      - name: Show OIDC claims (no cloud role required)
        env:
          AUDIENCE: sts.amazonaws.com
        run: |
          TOKEN=$(curl -sS \
            -H "Authorization: bearer $ACTIONS_ID_TOKEN_REQUEST_TOKEN" \
            "$ACTIONS_ID_TOKEN_REQUEST_URL&audience=$AUDIENCE" \
            | jq -r .value)
          # Print claims only. Never print the raw token.
          echo "$TOKEN" | cut -d. -f2 | base64 -d 2>/dev/null \
            | jq '{sub, aud, ref, environment, repository, workflow_ref}'

      - uses: actions/download-artifact@v4
        with:
          name: capacity-report
          run-id: ${{ inputs.run_id }}
          github-token: ${{ github.token }}
          path: dist

      - name: Verify checksum
        run: |
          cd dist
          sha256sum -c SHA256SUMS

      - name: Install and smoke test
        run: |
          python3 -m venv .venv-deploy
          .venv-deploy/bin/pip install --no-deps dist/*.whl
          .venv-deploy/bin/capacity-report --help
          .venv-deploy/bin/python -c 'import capacity_report; print(capacity_report.__name__)'

      - name: Write deployment record
        env:
          TARGET: ${{ inputs.environment }}
          RUN_ID: ${{ inputs.run_id }}
          SOURCE_SHA: ${{ steps.source.outputs.sha }}
        run: |
          jq -n \
            --arg environment "$TARGET" \
            --arg ci_run_id "$RUN_ID" \
            --arg source_sha "$SOURCE_SHA" \
            --arg deploy_run "${{ github.run_id }}" \
            '{environment:$environment, ci_run_id:$ci_run_id,
              source_sha:$source_sha, deploy_run_id:$deploy_run,
              status:"verified"}' \
            > deployment.json
          cat deployment.json

      - uses: actions/upload-artifact@v4
        with:
          name: deployment-record
          path: deployment.json
          retention-days: 30
```

Commit and push:

```bash
git add .github/workflows/deploy.yml
git commit -m "ci: gated deploy with artifact verification and OIDC claims"
git push
```

### Why the workflow is shaped this way

- **`workflow_dispatch` only:** no fork, no PR, no accidental deploy on every commit.
- **`environment:`** gates the job and changes the OIDC `sub` claim to include `environment:<name>`.
- **`permissions`:** `actions: read` to download another run's artifact and query the run; `id-token: write` to request the OIDC token. No `contents: write`, no cloud keys.
- **Source-run verification:** prevents deploying an artifact from a random branch or a failed run. The API check is a real control, not a comment.
- **Checksum verification:** the deploy job does not trust the artifact it downloaded.
- **Clean venv smoke test:** proves the artifact installs and runs with no development dependencies.
- **OIDC claims step:** shows the exact `sub`/`aud`/`environment` values a cloud trust policy would evaluate, without creating a cloud role.
- **Deployment record:** a small audit artifact linking environment, CI run, source SHA, and deploy run.

## Part E — Run the deploy

Get the CI run ID:

```bash
gh run list --workflow=ci.yml --limit 3
```

Trigger the deploy with that ID:

```bash
gh workflow run deploy.yml \
  -f environment=staging \
  -f run_id=<ci-run-id>

sleep 2
gh run list --workflow=deploy.yml --limit 3
```

Expected status: `waiting` (if the environment requires a reviewer) or `in_progress` (if not).

Approve in GitHub: **Actions** → the `deploy` run → **Review deployments** → select `staging` → **Approve and deploy**.

Then watch:

```bash
gh run watch <deploy-run-id> --exit-status
```

Expected OIDC output (values differ):

```json
{
  "sub": "repo:my-org/opslab-pipeline:environment:staging",
  "aud": "sts.amazonaws.com",
  "ref": "refs/heads/main",
  "environment": "staging",
  "repository": "my-org/opslab-pipeline",
  "workflow_ref": "my-org/opslab-pipeline/.github/workflows/deploy.yml@refs/heads/main"
}
```

This is the claim a cloud trust policy must match. If the workflow did **not** use an environment, the `sub` would be `repo:my-org/opslab-pipeline:ref:refs/heads/main`. That one-word difference is the most common OIDC failure.

Download the deployment record:

```bash
gh run download <deploy-run-id> -n deployment-record -D /tmp/deploy-record
cat /tmp/deploy-record/deployment.json
```

Expected:

```json
{
  "environment": "staging",
  "ci_run_id": "4821993311",
  "source_sha": "a1b2c3d...",
  "deploy_run_id": "4822001122",
  "status": "verified"
}
```

The `source_sha` is the CI run's SHA, not the deploy branch's current HEAD. That is provenance: you know which commit produced the tested artifact.

## Part F — Failure drills

### 1. Wrong run ID

```bash
gh workflow run deploy.yml -f environment=staging -f run_id=1
```

After approval (if required), the `Verify the source CI run` step fails because run 1 is not a successful `main` run, or the artifact download fails with `Artifact not found`. Record which step failed and why. A deploy should fail before installing anything.

### 2. Tampered artifact

Simulate a corrupted download locally:

```bash
cp -r /tmp/ci-artifact /tmp/ci-artifact-tampered
printf 'tampered\n' >> /tmp/ci-artifact-tampered/*.whl
cd /tmp/ci-artifact-tampered
sha256sum -c SHA256SUMS; echo "exit=$?"
```

Expected: `FAILED`, exit 1. The workflow's `Verify checksum` step behaves the same way. Restore by re-downloading; do not "fix" the checksum file.

### 3. Missing OIDC permission

On a branch, remove `id-token: write` from the deploy job's permissions and push. Trigger the workflow. Expected: the OIDC claims step fails because `ACTIONS_ID_TOKEN_REQUEST_URL`/`ACTIONS_ID_TOKEN_REQUEST_TOKEN` are not set. Restore the permission. This is a fast, safe way to see the dependency without involving a cloud.

### 4. Failed test blocks the build

Add a failing assertion to a test, commit, push, and watch CI. Expected: `test` fails, `build` and `smoke` are skipped. `gh run view <id> --log-failed` shows the assertion. Revert the change. This proves `needs:` is doing its job.

### 5. Workflow does not appear

If `gh run list` shows no run after a push:

- confirm the file is under `.github/workflows/` and ends in `.yml`/`.yaml`;
- confirm the `on:` block matches the branch/event;
- check for YAML indentation/tabs;
- confirm Actions is enabled for the repo;
- check whether a `paths:` filter excluded your changes.

### 6. Fork PR

If you have a collaborator/fork, open a PR from the fork. Expected: CI runs with a read-only token; deploy is not triggered. This is correct. Do not switch to `pull_request_target` with a checkout of the fork's head to "make deploy work."

## Internal explanation

### CI is evidence, not a badge

A green check means the jobs that ran passed. It does not mean the artifact is deployable. The `smoke` job adds evidence: the wheel installs in a clean environment and runs against fixtures. The deploy job adds more: the artifact came from a successful `main` run, its checksum matches, and it installs in a fresh venv. Each stage narrows the uncertainty.

### Why build once

The wheel built by `build` is uploaded as an artifact and downloaded by `smoke` and `deploy`. Neither job rebuilds from source. If the deploy job ran `pip install .`, it could resolve different dependencies or build with a different compiler, and you would not be testing the same bytes. Build once, identify by checksum/digest, promote.

### Why the environment gate matters

The environment is more than a label. It controls:

- who must approve;
- which branches may deploy;
- which secrets are available;
- the OIDC `sub` claim.

A deploy job with `environment: prod` but no protection rules is just a comment. The gate is only real if reviewers/branches are configured.

### Why OIDC claims are shown without a cloud

GitHub can mint an OIDC token for any job with `id-token: write`; no cloud provider is required to see the claims. Exchanging the token for cloud credentials is the next step (AWS STS/GCP/Azure), and the trust policy checks the claims you just printed. Seeing the actual `sub` is faster than guessing why `AssumeRoleWithWebIdentity` failed.

### Why `actions: read`

Downloading an artifact from a different workflow run and querying that run's metadata requires the `actions` scope. The default `GITHUB_TOKEN` permissions may not include it. The deploy job grants only what it needs.

### Why the source-run check

`run_id` is user input. Without validation, a user with workflow-run permission could point the deploy at an artifact from an experimental branch. The API check asserts `conclusion == success` and `head_branch == main` before downloading. It is not a substitute for environment approval, but it narrows the blast radius.

### What this lab does not prove

- It does not deploy to a real server or cluster.
- It does not prove the artifact works with production data.
- It does not exercise cloud OIDC exchange or IAM policy.
- It does not test rollback.

The optional stretch and the Phase 8 reliability work address the first and last. The OIDC note in [CI/CD and GitOps](04-ci-cd-and-gitops.md) covers the cloud exchange and trust policy.

## Optional stretch — A real GitOps loop on kind

Do this only if you have Docker/kind and enough RAM. It is separate from the artifact pipeline above.

```bash
kind create cluster --name opslab-gitops
kubectl create namespace argocd
kubectl apply -n argocd -f \
  https://raw.githubusercontent.com/argoproj/argo-cd/stable/manifests/install.yaml
kubectl -n argocd rollout status deployment/argocd-server --timeout=180s
```

For production, pin the Argo CD manifest to a release tag, not `stable`.

Create a public manifest repo with:

```text
k8s/
├── deployment.yaml
├── service.yaml
└── kustomization.yaml
```

`deployment.yaml`:

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: demo
  namespace: demo
spec:
  replicas: 2
  selector:
    matchLabels:
      app: demo
  template:
    metadata:
      labels:
        app: demo
    spec:
      containers:
        - name: demo
          image: nginx:1.27
          ports:
            - containerPort: 80
          readinessProbe:
            httpGet:
              path: /
              port: 80
            initialDelaySeconds: 2
```

`service.yaml`:

```yaml
apiVersion: v1
kind: Service
metadata:
  name: demo
  namespace: demo
spec:
  selector:
    app: demo
  ports:
    - port: 80
      targetPort: 80
```

`kustomization.yaml`:

```yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization
namespace: demo
resources:
  - deployment.yaml
  - service.yaml
```

Install the Argo CD CLI and log in:

```bash
# macOS
brew install argocd
# Linux: download the current release binary from the Argo CD releases page,
# verify its checksum, and install it as /usr/local/bin/argocd.

kubectl -n argocd get secret argocd-initial-admin-secret \
  -o jsonpath='{.data.password}' | base64 -d; echo
kubectl port-forward -n argocd svc/argocd-server 8081:443 &
argocd login 127.0.0.1:8081 --username admin --insecure
```

Create the application:

```bash
argocd app create demo \
  --repo https://github.com/<you>/<manifest-repo>.git \
  --path k8s \
  --dest-server https://kubernetes.default.svc \
  --dest-namespace demo \
  --sync-policy automated \
  --auto-prune \
  --self-heal
```

Watch it sync:

```bash
argocd app get demo
kubectl get pods -n demo
kubectl port-forward -n demo svc/demo 8082:80 &
curl -sS http://127.0.0.1:8082/ | head -3
```

Break it via Git: change the image to `nginx:does-not-exist`, commit, push, and sync/watch. Expected: pods `ImagePullBackOff`, Argo CD health `Degraded`. Then revert the commit and sync. Expected: healthy again. This is the GitOps rollback path: revert the desired state, let the controller reconcile.

Cleanup:

```bash
kind delete cluster --name opslab-gitops
```

Do not use `selfHeal` on a production app until you have a plan for emergency manual changes. It will revert them.

## Verification

- [ ] CI runs on push/PR and all jobs pass.
- [ ] `lint`, `test` (3.11 and 3.12), `build`, and `smoke` are visible as separate jobs.
- [ ] The artifact downloads and `sha256sum -c SHA256SUMS` passes.
- [ ] The wheel installs in a fresh venv and `capacity-report --help` works.
- [ ] The deploy workflow requires `run_id` and validates it.
- [ ] The source-run check rejects a failed/non-`main` run.
- [ ] The environment gate shows `waiting` before approval (if protection rules are available).
- [ ] The OIDC claims show `sub` containing `environment:staging` and `aud` `sts.amazonaws.com`.
- [ ] The deployment record links environment, CI run, source SHA, and deploy run.
- [ ] A tampered artifact fails the checksum step.
- [ ] Fork PRs cannot deploy.
- [ ] No static cloud keys are stored in the repo.

## Cleanup

```bash
# Local artifacts and venvs
rm -rf /tmp/ci-artifact /tmp/ci-artifact-tampered /tmp/ci-venv /tmp/deploy-record
rm -rf .venv-smoke .venv-deploy

# Optional: stop watching / cancel a stuck run
gh run cancel <run-id>

# Optional: delete the lab repo (destructive; confirm the name twice)
# gh repo delete my-org/opslab-pipeline --yes
```

Workflow artifacts expire after the configured retention. The environment and its reviewers remain until you delete them in repository settings. If you made the repo public, remember that the code and workflow history are public; do not add secrets later.

## Write-up template

```markdown
# L7.4 — Gated CI/CD pipeline

## Repository
- Repo URL:
- Visibility:
- Default branch:
- Environments/reviewers:

## CI jobs
| Job | Command | Result | Duration |
|---|---|---|---|
| lint | | | |
| test 3.11 | | | |
| test 3.12 | | | |
| build | | | |
| smoke | | | |

## Artifact
- Name:
- Files:
- Checksum verification:
- Clean-venv install:
- Smoke command:

## Deploy gate
- Trigger:
- Required inputs:
- Source-run checks:
- Environment approval:
- OIDC claims (`sub`, `aud`, `environment`):
- Deployment record:

## Failure drills
- Wrong run ID: failing step ___; message ___; fix ___
- Tampered artifact: failing step ___; message ___; fix ___
- Missing id-token: failing step ___; message ___; fix ___
- Failing test: which jobs skipped ___; evidence ___
- Fork PR: what ran ___; what did not ___; why ___

## Security review
- GITHUB_TOKEN permissions:
- Untrusted input handled via:
- Actions pinned to SHA? If not, plan:
- Secrets stored:
- What a compromised test dependency could access:
- What the environment gate does not protect:

## What I would add for a real deploy
- Target (VM/Kubernetes/serverless):
- Credentials (OIDC role/provider):
- Health verification:
- Rollback:
- Database changes:
- Observability:
```

---

Next: [the Phase 7 checkpoint](07-checkpoint.md).
