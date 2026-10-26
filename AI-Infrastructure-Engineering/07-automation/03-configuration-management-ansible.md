# Configuration Management with Ansible

> **The one-sentence model:** Ansible connects over SSH (or a local connection), reads facts, compares the current state to a playbook, and applies only the differences. A good playbook is idempotent, reviewable, and safe to run twice; a playbook that shells out to `command` for everything is just a remote Bash script with extra YAML.

---

## 1. The model

```text
inventory (hosts/groups)
  │
  ▼
playbook (plays → roles/tasks)
  │  modules (declarative actions)
  ▼
target host over SSH
  │  gather facts, compare state, apply diff
  ▼
handlers (reload/restart after change)
```

| Term | Meaning |
|---|---|
| **Inventory** | Hosts and groups, with connection variables |
| **Playbook** | One or more plays mapping hosts to tasks/roles |
| **Play** | A host group + variables + tasks/roles |
| **Task** | A call to a module with arguments |
| **Module** | A declarative action (`apt`, `template`, `service`, ...) |
| **Role** | Reusable bundle of tasks/defaults/vars/handlers/templates/files |
| **Handler** | A task triggered by `notify`, usually reload/restart |
| **Fact** | Discovered host data (`ansible_facts`) |
| **Inventory variables** | `group_vars/`, `host_vars/`, inline vars |
| **Collection** | A distributable bundle of modules/roles/plugins |

Ansible is **agentless**: it uses SSH and Python on the target. It is **push-based** by default: you run `ansible-playbook`. AWX/AAP adds a controller/UI, but the execution model is the same.

---

## 2. Inventory

INI:

```ini
[webservers]
web1 ansible_host=203.0.113.10 ansible_user=ubuntu ansible_ssh_private_key_file=~/.ssh/id_ed25519
web2 ansible_host=203.0.113.11 ansible_user=ubuntu ansible_ssh_private_key_file=~/.ssh/id_ed25519

[dbservers]
db1 ansible_host=203.0.113.20 ansible_user=ubuntu

[all:vars]
ansible_python_interpreter=/usr/bin/python3
```

YAML:

```yaml
all:
  children:
    webservers:
      hosts:
        web1:
          ansible_host: 203.0.113.10
          ansible_user: ubuntu
        web2:
          ansible_host: 203.0.113.11
          ansible_user: ubuntu
    dbservers:
      hosts:
        db1:
          ansible_host: 203.0.113.20
          ansible_user: ubuntu
```

```bash
ansible-inventory -i inventory.ini --graph
ansible-inventory -i inventory.ini --host web1
ansible -i inventory.ini all -m ansible.builtin.ping
ansible -i inventory.ini webservers --list-hosts
```

**Inventory is not a secret store.** Do not put passwords in it. Use SSH keys, `become`, and Vault/secret managers. A static inventory is fine for a lab; a cloud/CMDB dynamic inventory plugin is better at scale.

### `ansible.cfg`

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

Keep `host_key_checking = True` for real environments; use a known_hosts entry or `accept-new` for a lab. Disabling it globally trains you to ignore MITM risk.

---

## 3. Playbook anatomy

```yaml
---
- name: Configure the web tier
  hosts: webservers
  become: true
  serial: 1
  any_errors_fatal: true
  vars:
    app_user: opslab
  tasks:
    - name: Install packages
      ansible.builtin.package:
        name:
          - nginx
          - curl
        state: present

    - name: Deploy nginx config
      ansible.builtin.template:
        src: nginx.conf.j2
        dest: /etc/nginx/nginx.conf
        owner: root
        group: root
        mode: "0644"
        validate: "nginx -t -c %s"
      notify: Reload nginx

    - name: Ensure nginx is running
      ansible.builtin.service:
        name: nginx
        state: started
        enabled: true

  handlers:
    - name: Reload nginx
      ansible.builtin.service:
        name: nginx
        state: reloaded
```

Run:

```bash
ansible-playbook -i inventory.ini site.yml --syntax-check
ansible-playbook -i inventory.ini site.yml --list-tasks
ansible-playbook -i inventory.ini site.yml --list-hosts
ansible-playbook -i inventory.ini site.yml --check --diff
ansible-playbook -i inventory.ini site.yml
```

`--check` runs modules in check mode where supported; not every module/task supports it (e.g., some `command` tasks). A check run can fail because a prerequisite is missing; read the error rather than assuming the playbook is broken.

### `serial` and failure control

- `serial: 1` updates one host at a time.
- `serial: "25%"` updates a quarter of the batch at a time.
- `max_fail_percentage: 10` stops the play if too many hosts fail.
- `any_errors_fatal: true` stops the whole play on the first failed host (useful for a tightly coupled service).
- `strategy: free` lets fast hosts proceed; `linear` (default) waits for the batch.

For a web tier behind a load balancer, `serial` plus health checks and connection draining is the minimum safe rollout. Ansible does not drain the LB for you; use a `pre_task`/`post_task` with your LB API or a `uri`/`wait_for` check.

---

## 4. Modules and idempotence

Prefer a purpose-built module:

| Need | Module |
|---|---|
| Package | `ansible.builtin.package`, `apt`, `dnf` |
| Service | `ansible.builtin.service`, `systemd` |
| File | `copy`, `template`, `file`, `lineinfile`, `blockinfile` |
| User/group | `user`, `group` |
| Archive | `unarchive` |
| Download | `get_url` |
| Command | `command` (no shell), `shell` (last resort) |
| HTTP check | `uri` |
| Wait | `wait_for`, `wait_for_connection` |
| Reboot | `reboot` |
| Firewall | `community.general.ufw`, `ansible.posix.firewalld` |

```yaml
- name: Create the app user
  ansible.builtin.user:
    name: "{{ app_user }}"
    system: true
    shell: /usr/sbin/nologin
    create_home: false
    state: present

- name: Ensure the directory exists
  ansible.builtin.file:
    path: /srv/app
    state: directory
    owner: "{{ app_user }}"
    group: "{{ app_user }}"
    mode: "0755"

- name: Deploy the config
  ansible.builtin.template:
    src: app.conf.j2
    dest: /etc/app/app.conf
    mode: "0640"
  notify: Restart app
```

### `command`/`shell` are not idempotent by default

```yaml
- name: Initialize the database (once)
  ansible.builtin.command:
    cmd: /usr/local/bin/app-init
    creates: /var/lib/app/initialized
```

`creates`/`removes` make the task skip when the file exists/does not exist. For read-only commands, set `changed_when: false`:

```yaml
- name: Read the app version
  ansible.builtin.command:
    cmd: /usr/local/bin/app --version
  register: app_version
  changed_when: false
  failed_when: app_version.rc != 0
```

For a command that sometimes changes state:

```yaml
- name: Run migration
  ansible.builtin.command:
    cmd: /usr/local/bin/app migrate
  register: migrate
  changed_when: "'applied' in migrate.stdout"
  failed_when: migrate.rc != 0
```

`shell` is only for pipes/redirection that a module cannot express. Never interpolate untrusted input into a shell string; use `command` with an argv list or a module.

### Handlers

A handler runs once at the end of the play if notified, even if multiple tasks notify it. Use `meta: flush_handlers` when a later task needs the new config immediately:

```yaml
- name: Deploy config
  ansible.builtin.template:
    src: nginx.conf.j2
    dest: /etc/nginx/nginx.conf
  notify: Reload nginx

- name: Flush handlers before health check
  ansible.builtin.meta: flush_handlers

- name: Check health
  ansible.builtin.uri:
    url: http://127.0.0.1/health
    status_code: 200
  retries: 5
  delay: 2
```

---

## 5. Variables and precedence

Ansible has many variable sources. A practical precedence (low to high):

```text
role defaults
inventory group_vars/all
inventory group_vars/<group>
inventory host_vars/<host>
play vars / vars_files / role vars
task vars / set_fact / registered vars
extra vars (-e)
```

`-e` wins over almost everything. Do not use `-e` for secrets in shell history; use Vault or a prompt.

```yaml
# group_vars/webservers.yml
app_user: opslab
webserver_packages:
  - nginx
  - curl
nginx_worker_processes: auto
```

```yaml
- name: Use a default only when undefined
  ansible.builtin.debug:
    msg: "{{ nginx_worker_processes | default('auto') }}"

- name: Omit an optional argument
  ansible.builtin.user:
    name: "{{ app_user }}"
    comment: "{{ app_comment | default(omit) }}"
```

Facts:

```yaml
- name: Show OS family
  ansible.builtin.debug:
    var: ansible_facts['os_family']

- name: Install the right package
  ansible.builtin.package:
    name: "{{ 'nginx' if ansible_facts['os_family'] == 'Debian' else 'nginx' }}"
    state: present
```

`hostvars['web1']['ansible_host']` accesses another host's variables; use it sparingly, because it couples hosts.

### Vault

```bash
ansible-vault create group_vars/all/vault.yml
ansible-vault edit group_vars/all/vault.yml
ansible-vault encrypt_string 's3cret' --name 'db_password'
ansible-playbook -i inventory.ini site.yml --ask-vault-pass
ansible-playbook -i inventory.ini site.yml --vault-password-file ~/.ansible/vault-pass
```

- Commit encrypted vault files, never the plaintext or the password file.
- Use `no_log: true` on tasks that handle secrets; `--diff` can otherwise print sensitive content.
- Prefer a secret manager (Vault, AWS Secrets Manager, SOPS) for production; Ansible Vault is a file-encryption tool, not a full secret service.
- Restrict permissions on the vault password file (`chmod 600`) and do not put it in the repo.

---

## 6. Roles

```text
roles/webserver/
├── defaults/main.yml      # low precedence, safe overrides
├── vars/main.yml          # high precedence, role internals
├── tasks/main.yml
├── handlers/main.yml
├── templates/nginx.conf.j2
├── files/
├── meta/main.yml
└── README.md
```

```yaml
# roles/webserver/tasks/main.yml
---
- name: Install web packages
  ansible.builtin.package:
    name: "{{ webserver_packages }}"
    state: present
  tags: [webserver, packages]

- name: Create app user
  ansible.builtin.user:
    name: "{{ app_user }}"
    system: true
    shell: /usr/sbin/nologin
    create_home: false
  tags: [webserver, users]

- name: Deploy nginx config
  ansible.builtin.template:
    src: nginx.conf.j2
    dest: /etc/nginx/nginx.conf
    owner: root
    group: root
    mode: "0644"
    validate: "nginx -t -c %s"
  notify: Reload nginx
  tags: [webserver, config]

- name: Ensure nginx is running
  ansible.builtin.service:
    name: nginx
    state: started
    enabled: true
  tags: [webserver, service]
```

```yaml
# roles/webserver/handlers/main.yml
---
- name: Reload nginx
  ansible.builtin.service:
    name: nginx
    state: reloaded
```

```yaml
# roles/webserver/defaults/main.yml
---
app_user: opslab
webserver_packages:
  - nginx
  - curl
```

```yaml
# site.yml
---
- name: Configure web tier
  hosts: webservers
  become: true
  serial: 1
  any_errors_fatal: true
  roles:
    - role: webserver
      tags: [webserver]
```

```bash
ansible-galaxy init roles/webserver     # scaffold
ansible-galaxy collection install community.general
ansible-galaxy role install geerlingguy.nginx
```

Pin collection/role versions in `requirements.yml`; an unpinned Galaxy install is a supply-chain and reproducibility risk.

---

## 7. Testing and linting

```bash
ansible-playbook -i inventory.ini site.yml --syntax-check
ansible-lint site.yml
yamllint .
ansible-playbook -i inventory.ini site.yml --check --diff
ansible-playbook -i inventory.ini site.yml
ansible-playbook -i inventory.ini site.yml   # second run: expect changed=0
```

- `--syntax-check` parses YAML/playbook structure; it does not validate module arguments or target state.
- `ansible-lint` catches risky patterns (shell instead of module, missing names, octal modes, etc.).
- `--check --diff` previews changes; some modules/tasks do not support check mode. On a fresh host, package/template validation can fail because the prerequisite is missing.
- The second run is the idempotence test: `changed=0` for tasks that should be stable. A handler may still run if a task reports changed incorrectly.
- Molecule (`molecule init role`, `molecule test`) spins up a container/VM, converges, runs tests, and checks idempotence. Optional, but valuable for roles.

### Test matrix

| Test | Proves | Does not prove |
|---|---|---|
| `--syntax-check` | YAML/play structure | module behavior |
| `ansible-lint` | common bad patterns | target correctness |
| `--check --diff` | planned diff (where supported) | all modules support check |
| first run | convergence | idempotence |
| second run | idempotence for changed tasks | app health |
| `uri`/`wait_for` | service responds | correctness under load |
| Molecule | role in a disposable target | production topology |

---

## 8. A complete example

`inventory.ini`:

```ini
[webservers]
web1 ansible_host=203.0.113.10 ansible_user=ubuntu ansible_ssh_private_key_file=~/.ssh/id_ed25519

[all:vars]
ansible_python_interpreter=/usr/bin/python3
```

`group_vars/webservers.yml`:

```yaml
app_user: opslab
webserver_packages:
  - nginx
  - curl
```

`roles/webserver/templates/nginx.conf.j2`:

```nginx
worker_processes {{ nginx_worker_processes | default('auto') }};
pid /run/nginx.pid;

events {
    worker_connections 1024;
}

http {
    include /etc/nginx/mime.types;
    default_type application/octet-stream;
    sendfile on;

    server {
        listen 80 default_server;
        server_name _;

        location = /health {
            return 200 "ok\n";
        }

        location / {
            return 200 "hello from {{ inventory_hostname }}\n";
        }
    }
}
```

`site.yml`:

```yaml
---
- name: Configure web tier
  hosts: webservers
  become: true
  serial: 1
  any_errors_fatal: true
  vars:
    nginx_worker_processes: auto
  tasks:
    - name: Update apt cache
      ansible.builtin.apt:
        update_cache: true
        cache_valid_time: 3600
      when: ansible_facts['os_family'] == 'Debian'
      tags: [packages]

    - name: Install packages
      ansible.builtin.package:
        name: "{{ webserver_packages }}"
        state: present
      tags: [packages]

    - name: Create app user
      ansible.builtin.user:
        name: "{{ app_user }}"
        system: true
        shell: /usr/sbin/nologin
        create_home: false
      tags: [users]

    - name: Deploy nginx config
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

    - name: Flush handlers before health check
      ansible.builtin.meta: flush_handlers
      tags: [service]

    - name: Check the health endpoint
      ansible.builtin.uri:
        url: "http://127.0.0.1/health"
        status_code: 200
        return_content: false
      retries: 5
      delay: 2
      register: health
      until: health.status == 200
      changed_when: false
      tags: [verify]

  handlers:
    - name: Reload nginx
      ansible.builtin.service:
        name: nginx
        state: reloaded
```

Run:

```bash
ansible-inventory -i inventory.ini --graph
ansible -i inventory.ini webservers -m ansible.builtin.ping
ansible-playbook -i inventory.ini site.yml --syntax-check
ansible-playbook -i inventory.ini site.yml --check --diff
ansible-playbook -i inventory.ini site.yml
ansible-playbook -i inventory.ini site.yml
curl -sS http://203.0.113.10/health
```

Expected: first run reports changes; second run reports `changed=0` (except possibly the health check, which is `changed_when: false`). The handler reloads nginx only when the template changes.

> **Check-mode caveat:** on a host where nginx is not installed, `--check` may fail at the template `validate` step because `nginx` is missing. Run the first convergence normally on a disposable VM, then use `--check --diff` for subsequent changes.

---

## 9. Command field manual

### `ansible`

```bash
ansible -i inventory.ini all -m ansible.builtin.ping
ansible -i inventory.ini webservers -m ansible.builtin.command -a 'uptime'
ansible -i inventory.ini web1 -m ansible.builtin.setup -a 'filter=ansible_distribution*'
```

**Why:** ad-hoc checks before writing a playbook. `ping` proves SSH/Python, not service health. `command` here is read-only; do not use ad-hoc shell for configuration.

### `ansible-playbook`

```bash
ansible-playbook -i inventory.ini site.yml --syntax-check
ansible-playbook -i inventory.ini site.yml --check --diff
ansible-playbook -i inventory.ini site.yml --limit web1 --tags config
ansible-playbook -i inventory.ini site.yml --list-tasks
ansible-playbook -i inventory.ini site.yml --start-at-task 'Deploy nginx config'
```

**Interpretation:** `--limit` restricts hosts; `--tags` restricts tasks; `--check` previews; `--diff` shows file changes; `--start-at-task` resumes but skips earlier dependencies (use with care). `--list-tasks` is a safe review before running.

### `ansible-inventory`

```bash
ansible-inventory -i inventory.ini --graph
ansible-inventory -i inventory.ini --host web1 --yaml
```

**Why:** prove which hosts/groups/variables Ansible actually sees. Most "wrong host" incidents are an inventory/limit mistake.

### `ansible-vault`

```bash
ansible-vault create secrets.yml
ansible-vault encrypt secrets.yml
ansible-vault decrypt secrets.yml
ansible-vault edit secrets.yml
ansible-vault rekey secrets.yml
```

**Safety:** decrypted files are plaintext on disk; remove them and use `--ask-vault-pass`/a protected password file. Do not commit the password.

### `ansible-galaxy`

```bash
ansible-galaxy collection install -r requirements.yml
ansible-galaxy role install -r requirements.yml
ansible-galaxy collection list
```

**Why:** pin and vendor dependencies. Review third-party roles like any code; they run with your privileges.

### `ansible-doc`

```bash
ansible-doc ansible.builtin.template
ansible-doc -l | grep -i firewall
```

**Why:** the authoritative module arguments/return values for your installed version. Do not guess option names.

### `ansible-lint` / `yamllint`

```bash
ansible-lint site.yml
yamllint .
```

**Interpretation:** lint findings are not all bugs, but each needs a reason. Fix risky shell usage, missing names, and mode/quoting issues first.

---

## 10. Failure scenarios

### A — `UNREACHABLE` / SSH failure

Check inventory host/user/key, security group/route, SSH agent, host key, and `ansible -m ping`. `ansible_ssh_private_key_file` must point to a key with `0600` permissions. Do not disable host-key checking globally to "fix" it.

### B — `sudo: a password is required`

`become: true` is set but the user needs a password. Use a sudoers drop-in for the automation user, or `--ask-become-pass` interactively. Do not put the sudo password in plaintext inventory.

### C — Task always reports `changed`

A `command`/`shell` task without `changed_when`/`creates`, or a template that changes every run (timestamp/random), or a module argument that rewrites the file. Inspect `--diff`; make the task idempotent or set `changed_when` based on real state.

### D — Handler never runs

The notifying task did not report `changed`, or the handler name does not match exactly, or the play failed before the handler flush. Check the task output and handler name; use `meta: flush_handlers` if a later task depends on it.

### E — `--check` fails on a fresh host

Package/template prerequisites are missing, or the module does not support check mode. Run a normal convergence on a disposable host first; use check mode for subsequent changes. Do not disable check mode globally.

### F — Variable is undefined or overridden

`-e` extra vars, `host_vars`, `group_vars`, role defaults, and play vars interact. Use `ansible-inventory --host`, `debug: var=...`, and `ansible-playbook -vvv` (redact secrets). Prefer role defaults for overridable values.

### G — Secrets printed in diff/logs

A template/variable containing a secret was rendered, or `no_log` is missing. Add `no_log: true` to the task, avoid `--diff` on secret files, and rotate if exposed. Vault encryption protects the file at rest, not the rendered output.

### H — Rolling update takes down the service

`serial` is too large, no health check/drain, or the handler restarts all hosts at once. Use `serial: 1`, a load-balancer drain/health check, `max_fail_percentage`, and a rollback plan. Test with one canary host.

### I — Works for one host, fails for another

Different OS family, package manager, Python interpreter, sudo policy, disk layout, or existing config. Use facts/`when`, test the matrix, and avoid assumptions from one host.

---

## 11. Checkpoint

1. What is the difference between an inventory, a playbook, a play, a role, and a task?
2. Why is Ansible called agentless, and what does it need on the target?
3. What does idempotence mean, and how do you test it?
4. Why is `command` not idempotent by default, and what fixes it?
5. What does `changed_when: false` mean?
6. What is a handler, and when does it run?
7. Why would you use `meta: flush_handlers`?
8. What is the difference between `serial: 1` and `strategy: free`?
9. What does `--check --diff` do, and what can it miss?
10. Why can `--check` fail on a fresh host?
11. What is the variable precedence order (roughly)?
12. Why is `-e` risky for secrets?
13. What does `become: true` do?
14. Why should you not disable host-key checking globally?
15. What is the difference between `ansible.builtin.package` and `apt`/`dnf`?
16. Why are `shell`/`command` a last resort?
17. How do you handle a command that should run only once?
18. What is Ansible Vault, and what does it not protect?
19. How do you do a safe rolling update?
20. What is the first thing you check when a play targets the wrong host?

### Answers

1. Inventory = hosts/groups; playbook = file of plays; play = hosts + tasks/roles; role = reusable bundle; task = one module call.
2. It uses SSH and Python on the target; no long-running agent is required (though Python and SSH must work).
3. Running twice converges to the same state; test by running the playbook again and checking `changed=0` for stable tasks.
4. It runs unconditionally and reports changed; use `creates`/`removes`, `changed_when`, or a purpose-built module.
5. The task may run but its result should not count as a change (read-only checks).
6. A task triggered by `notify` when a notifying task changes; usually reload/restart; runs once at the end of the play.
7. To apply notified changes before a later task (e.g., health check) needs the new config.
8. `serial: 1` batches hosts one at a time; `strategy: free` lets hosts proceed independently without waiting for the batch.
9. It previews changes/diffs where modules support check mode; it can miss modules that don't support check mode and runtime/health effects.
10. Prerequisites (packages/files) are absent, or a module cannot validate in check mode; run a normal convergence first.
11. Role defaults → inventory group_vars → host_vars → play vars/vars_files/role vars → task vars/set_fact/register → extra vars.
12. It is visible in shell history/process lists and overrides other variables; use Vault/secret injection.
13. It escalates privileges (usually sudo) for tasks that need root.
14. It enables MITM; use a known_hosts entry or a controlled `accept-new` for labs.
15. `package` is a generic wrapper that selects the OS package manager; `apt`/`dnf` are OS-specific and expose more options.
16. They bypass declarative state/idempotence and can inject commands; use a module unless there is no alternative.
17. `command` with `creates:` (or a flag file/state check), not a shell `if` that always runs.
18. It encrypts files at rest with a password; it does not protect rendered output, logs, memory, or a compromised controller.
19. `serial: 1`/small batch, drain/health check, `max_fail_percentage`, canary, rollback, and a maintenance window.
20. `ansible-inventory --graph`/`--host`, `--list-hosts`, `--limit`, and the inventory/group vars; most wrong-host issues are inventory or limit mistakes.
