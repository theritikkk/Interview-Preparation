# Bash for Operations

> **The one-sentence model:** a production Bash script is a small program with an explicit contract: inputs, outputs, exit codes, cleanup, locking, and failure behavior. If a script only works when a human watches it, it is not automation yet.

---

## 1. When to use Bash

Use Bash for:

- gluing existing commands together;
- small, single-purpose checks and wrappers;
- bootstrapping a host before a config-management tool is available;
- CI steps that are naturally a sequence of CLI calls.

Do not use Bash for:

- complex data structures or parsing (use Python/jq);
- long-lived services (use a real service/manager);
- concurrent workflows with retries and state (use Python/Ansible);
- anything that needs unit tests with rich assertions (Bash tests exist, but Python is usually clearer).

A useful rule: **if the script needs a data structure more complex than an array, switch languages.**

---

## 2. Strict mode and its limits

The standard opening:

```bash
#!/usr/bin/env bash
set -Eeuo pipefail
IFS=$'\n\t'
```

| Option | Effect | Caveat |
|---|---|---|
| `-e` | Exit on an unhandled non-zero command | Ignored in `if`, `while`, `until`, `&&`, `||`, `!`, and some function/subshell contexts |
| `-E` | Inherit the `ERR` trap in functions/subshells | Does not make `-e` apply where it is ignored |
| `-u` | Error on unset variables/parameters | Empty arrays/`$@` can behave differently across Bash versions; use `${var:-}` deliberately |
| `-o pipefail` | Pipeline fails if any command fails (rightmost non-zero) | Without it, only the last command's status counts |
| `IFS=$'\n\t'` | Word splitting on newline/tab, not spaces | Can break code that relies on space-separated splitting; be explicit |

`set -e` is not a substitute for error handling. A command can fail inside a pipeline, a command substitution, a subshell, or a condition and not exit. Check critical operations explicitly:

```bash
if ! output=$(curl --fail --silent --show-error --max-time 10 "$url"); then
  printf 'request failed: %s\n' "$url" >&2
  exit 1
fi
```

### `ERR` trap

```bash
trap 'rc=$?; printf "ERROR %s:%s: %s (rc=%s)\n" \
  "${BASH_SOURCE[0]}" "$LINENO" "$BASH_COMMAND" "$rc" >&2; exit "$rc"' ERR
```

The trap is for diagnostics, not for swallowing errors. Do not `exit 0` in an `ERR` trap.

### `EXIT` trap and cleanup

```bash
tmpdir=$(mktemp -d)
cleanup() {
  local rc=$?
  rm -rf -- "$tmpdir"
  exit "$rc"
}
trap cleanup EXIT
```

`EXIT` runs on normal exit and on `exit`, but not on `SIGKILL`. For `INT`/`TERM`, either let the default terminate and rely on `EXIT`, or add explicit traps that exit non-zero.

---

## 3. Arguments and usage

Use `getopts` for short options and a clear `usage()`:

```bash
usage() {
  cat <<'EOF'
Usage: ops-health.sh [-n] [-w percent] [-c percent] [-h]

  -n  dry run: print planned checks, change nothing
  -w  warning disk percent (default: 85)
  -c  critical disk percent (default: 95)
  -h  help
EOF
}

dry_run=0
warn=85
crit=95

while getopts ":nw:c:h" opt; do
  case "$opt" in
    n) dry_run=1 ;;
    w) warn=$OPTARG ;;
    c) crit=$OPTARG ;;
    h) usage; exit 0 ;;
    :) printf 'missing argument for -%s\n' "$OPTARG" >&2; usage >&2; exit 2 ;;
    \?) printf 'unknown option: -%s\n' "$OPTARG" >&2; usage >&2; exit 2 ;;
  esac
done

shift $((OPTIND - 1))
if (($# != 0)); then
  printf 'unexpected arguments: %s\n' "$*" >&2
  usage >&2
  exit 2
fi
```

Validate numeric inputs before arithmetic:

```bash
[[ $warn =~ ^[0-9]+$ ]] || { printf 'warn must be an integer\n' >&2; exit 2; }
(( warn < crit )) || { printf 'warn must be < crit\n' >&2; exit 2; }
```

Quote `"$@"` to preserve arguments; use arrays for command construction instead of `eval`:

```bash
args=(--fail --silent --show-error --max-time 10)
curl "${args[@]}" "$url"
```

---

## 4. Logging and output contracts

- **Data** goes to stdout; **logs/diagnostics** go to stderr. This lets `script > report.json` work.
- Include a timestamp, level, and message. Avoid logging secrets.
- Use exit codes consistently: `0` success, `1` runtime failure, `2` usage error, `3` lock/busy, `4` partial failure (document it).

```bash
log() {
  local level=$1; shift
  printf '%s %-5s %s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$level" "$*" >&2
}
die() { log ERROR "$*"; exit 1; }
```

For system logs, `logger -t ops-health "$message"` integrates with syslog/journald. For machine-readable output, use `jq -n` rather than hand-built JSON:

```bash
jq -n --arg host "$(hostname -f)" --arg status "$status" \
  '{host: $host, status: $status, checked_at: (now | todate)}'
```

---

## 5. Locking: refuse to run twice

A cron job or CI retry can overlap with a previous run. Use `flock` on a file descriptor:

```bash
lock_file=${LOCK_FILE:-${XDG_RUNTIME_DIR:-/tmp}/ops-health.lock}
exec 9>"$lock_file"
if ! flock -n 9; then
  printf 'another ops-health run holds %s\n' "$lock_file" >&2
  exit 3
fi
```

The lock is released when the script exits or fd 9 closes. `-n` fails immediately instead of waiting; use `-w 30` if a short wait is acceptable. Do not use a PID file as your only lock: PIDs are reused and stale files are common.

**Safety:** a world-writable lock path can be symlink-attacked. Prefer `/run/lock` (root/group), `$XDG_RUNTIME_DIR`, or a directory with restrictive permissions. Do not run the script as root just to create a lock.

---

## 6. Dry-run

A dry-run must not mutate state. Centralize mutations through a wrapper:

```bash
run() {
  if (( dry_run )); then
    log INFO "[dry-run] $*"
    return 0
  fi
  "$@"
}
```

Then:

```bash
run systemctl restart nginx
run install -m 0644 -o root -g root nginx.conf /etc/nginx/nginx.conf
```

For tools with native previews, prefer them:

```bash
ansible-playbook site.yml --check --diff
terraform plan
kubectl diff -f manifest.yaml
rsync -avn --delete src/ host:/srv/app/   # -n = dry run
```

A dry-run that prints "would restart nginx" but has already edited the file is a lie. Keep reads outside `run` and writes inside it.

---

## 7. Idempotence in shell

Shell scripts are usually imperative, but you can still converge:

```bash
# Directory
install -d -m 0755 /srv/app

# File from a repo, only if changed
if ! cmp -s nginx.conf /etc/nginx/nginx.conf; then
  install -m 0644 nginx.conf /etc/nginx/nginx.conf
  systemctl reload nginx
fi

# User/group
getent group app >/dev/null || groupadd --system app
id -u app >/dev/null 2>&1 || useradd --system --gid app --home /srv/app --shell /usr/sbin/nologin app

# Line in a file
grep -qxF 'PermitRootLogin no' /etc/ssh/sshd_config || \
  printf '%s\n' 'PermitRootLogin no' >> /etc/ssh/sshd_config
```

The last example is not fully idempotent if the file has a conflicting setting; a config-management tool with a template is safer. For shell, check the effective state, not just the presence of a line.

---

## 8. Safe file and process handling

### Filenames

```bash
find /var/log -type f -name '*.log' -print0 |
  while IFS= read -r -d '' file; do
    printf '%s\n' "$file"
  done
```

- `-print0` + `read -d ''` handles spaces/newlines in names.
- Quote every expansion: `"$file"`, `"$dir"`.
- Use `--` before paths/patterns that may start with `-`: `rm -f -- "$file"`.
- Do not use `for f in $(find ...)`; it splits on whitespace and globs.

### Destructive commands

```bash
: "${TARGET_DIR:?TARGET_DIR must be set}"
case "$TARGET_DIR" in
  /|/home|/var|/usr|/etc) die "refusing to delete $TARGET_DIR" ;;
esac
[[ -d $TARGET_DIR ]] || die "not a directory: $TARGET_DIR"
rm -rf -- "$TARGET_DIR"
```

`rm -rf "$VAR"` with an empty/unset variable is a classic disaster. `: "${VAR:?}"` fails before the `rm`. A whitelist/case guard is stronger.

### Timeouts and background jobs

```bash
timeout 10s curl --fail --silent --show-error "$url"
```

`timeout` returns 124 on timeout. For background jobs, record PIDs and `wait`; avoid orphan processes:

```bash
pids=()
for host in "${hosts[@]}"; do
  ssh -o BatchMode=yes -o ConnectTimeout=5 "$host" 'uptime' &
  pids+=("$!")
done
rc=0
for pid in "${pids[@]}"; do wait "$pid" || rc=1; done
exit "$rc"
```

`wait` without a PID waits for all children but cannot tell you which failed; capture each status.

### SSH

```bash
ssh -o BatchMode=yes -o ConnectTimeout=5 -o StrictHostKeyChecking=accept-new \
  user@host 'systemctl is-active --quiet nginx'
```

`BatchMode=yes` fails instead of prompting for a password, which is what you want in automation. Do not disable host-key checking globally; use a known_hosts file or `accept-new` for a lab.

---

## 9. Parsing output

Prefer structured output:

```bash
# OpenStack
openstack server list -f json | jq -r '.[] | [.Name, .Status, .Networks] | @tsv'

# Kubernetes
kubectl get pods -o json | jq -r '.items[] | [.metadata.name, .status.phase] | @tsv'

# systemd
systemctl show -p ActiveState --value nginx
```

Avoid `awk '{print $5}'` on human-formatted tables that can wrap or change columns. If you must parse text, anchor on a stable field and fail loudly when the format changes.

`jq -e` sets a non-zero exit status for `false`/`null`, which is useful in checks:

```bash
if ! jq -e '.status == "healthy"' response.json >/dev/null; then
  die "unhealthy response"
fi
```

---

## 10. A complete example

`ops-health.sh` checks disk, memory, a systemd service, and emits JSON. It supports `--dry-run`, uses a lock, and cleans up.

```bash
#!/usr/bin/env bash
set -Eeuo pipefail
IFS=$'\n\t'

readonly SCRIPT_NAME=${0##*/}
dry_run=0
warn_pct=85
crit_pct=95
service_name=nginx

usage() {
  cat <<'EOF'
Usage: ops-health.sh [-n] [-w percent] [-c percent] [-s service] [-h]

  -n  dry run (print planned checks, no changes)
  -w  warning disk percent (default 85)
  -c  critical disk percent (default 95)
  -s  systemd service to check (default nginx)
  -h  help

Exit codes: 0 healthy, 1 warning, 2 critical, 3 usage/lock.
EOF
}

log() {
  local level=$1; shift
  printf '%s %-5s %s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$level" "$*" >&2
}
die() { log ERROR "$*"; exit 3; }

while getopts ":nw:c:s:h" opt; do
  case "$opt" in
    n) dry_run=1 ;;
    w) warn_pct=$OPTARG ;;
    c) crit_pct=$OPTARG ;;
    s) service_name=$OPTARG ;;
    h) usage; exit 0 ;;
    :) die "missing argument for -$OPTARG" ;;
    \?) die "unknown option: -$OPTARG" ;;
  esac
done
shift $((OPTIND - 1))
(($# == 0)) || die "unexpected arguments: $*"

[[ $warn_pct =~ ^[0-9]+$ ]] || die "warn must be an integer"
[[ $crit_pct =~ ^[0-9]+$ ]] || die "crit must be an integer"
(( warn_pct < crit_pct )) || die "warn must be less than crit"

tmpdir=$(mktemp -d)
cleanup() {
  local rc=$?
  rm -rf -- "$tmpdir"
  exit "$rc"
}
trap cleanup EXIT
trap 'rc=$?; log ERROR "failed at ${BASH_SOURCE[0]}:${LINENO}: ${BASH_COMMAND} (rc=$rc)"' ERR

lock_file=${LOCK_FILE:-${XDG_RUNTIME_DIR:-/tmp}/ops-health.lock}
exec 9>"$lock_file"
flock -n 9 || die "another run holds $lock_file"

command -v jq >/dev/null 2>&1 || die "jq is required"
command -v df >/dev/null 2>&1 || die "df is required"

status=ok
exit_code=0

# Disk: highest used percentage across real filesystems.
if (( dry_run )); then
  log INFO "[dry-run] would inspect disk usage"
  disk_pct=0
else
  disk_pct=$(df -P -x tmpfs -x devtmpfs 2>/dev/null |
    awk 'NR > 1 { gsub(/%/, "", $5); if ($5 + 0 > max) max = $5 + 0 } END { print max + 0 }')
fi

if (( disk_pct >= crit_pct )); then
  status=critical; exit_code=2
  log ERROR "disk ${disk_pct}% >= critical ${crit_pct}%"
elif (( disk_pct >= warn_pct )); then
  status=warning; (( exit_code < 1 )) && exit_code=1
  log WARN "disk ${disk_pct}% >= warning ${warn_pct}%"
else
  log INFO "disk ${disk_pct}%"
fi

# Memory: MemAvailable from /proc/meminfo.
mem_pct=0
if [[ -r /proc/meminfo ]]; then
  mem_pct=$(awk '
    /MemTotal:/      { total=$2 }
    /MemAvailable:/  { avail=$2 }
    END { if (total > 0) printf "%d", ((total - avail) * 100 / total) }
  ' /proc/meminfo)
  if (( mem_pct >= crit_pct )); then
    status=critical; exit_code=2
    log ERROR "memory ${mem_pct}% >= critical ${crit_pct}%"
  elif (( mem_pct >= warn_pct )); then
    [[ $status == ok ]] && status=warning
    (( exit_code < 1 )) && exit_code=1
    log WARN "memory ${mem_pct}% >= warning ${warn_pct}%"
  else
    log INFO "memory ${mem_pct}%"
  fi
else
  log WARN "/proc/meminfo unavailable; skipping memory"
fi

# Service: read-only check.
if command -v systemctl >/dev/null 2>&1; then
  if (( dry_run )); then
    log INFO "[dry-run] would check service ${service_name}"
    svc_state=unknown
  elif systemctl is-active --quiet "$service_name"; then
    svc_state=active
    log INFO "service ${service_name} active"
  else
    svc_state=inactive
    status=critical; exit_code=2
    log ERROR "service ${service_name} is not active"
  fi
else
  svc_state=unknown
  log WARN "systemctl unavailable; skipping service"
fi

jq -n \
  --arg host "$(hostname -f 2>/dev/null || hostname)" \
  --arg status "$status" \
  --arg service "$service_name" \
  --arg svc_state "$svc_state" \
  --argjson disk_pct "${disk_pct:-0}" \
  --argjson mem_pct "${mem_pct:-0}" \
  '{host: $host, status: $status, service: $service, service_state: $svc_state,
    disk_percent: $disk_pct, memory_percent: $mem_pct, checked_at: (now | todate)}'

exit "$exit_code"
```

Run it:

```bash
chmod +x ops-health.sh
shellcheck -x ops-health.sh
./ops-health.sh -n
./ops-health.sh; echo "exit=$?"
./ops-health.sh -w 50 -c 60; echo "exit=$?"
```

---

## 11. Testing shell scripts

### Syntax and lint

```bash
bash -n ops-health.sh          # parse only
shellcheck -x ops-health.sh    # static analysis
shfmt -d ops-health.sh         # formatting diff (optional)
```

ShellCheck catches quoting, `rm` hazards, unused variables, and common portability bugs. It is not a functional test.

### Bats

[Bats](https://github.com/bats-core/bats-core) runs Bash test files:

```bash
# tests/health.bats
setup() {
  export LOCK_FILE="$BATS_TEST_TMPDIR/ops-health.lock"
  export PATH="$BATS_TEST_DIRNAME/fixtures:$PATH"
}

@test "help exits zero" {
  run "$BATS_TEST_DIRNAME/../ops-health.sh" -h
  [ "$status" -eq 0 ]
  [[ $output == *"Usage:"* ]]
}

@test "bad option exits 3" {
  run "$BATS_TEST_DIRNAME/../ops-health.sh" -z
  [ "$status" -eq 3 ]
}

@test "dry-run does not require root" {
  run "$BATS_TEST_DIRNAME/../ops-health.sh" -n
  [ "$status" -eq 0 ]
}
```

```bash
bats tests/
```

For code that calls `systemctl`, `df`, or `curl`, put fake executables earlier in `PATH` that print fixtures and exit with the status you want. This tests your logic, not the real system. Always run at least one integration test on a disposable VM.

---

## 12. Command field manual

### `shellcheck`

**Purpose:** find common Bash bugs before execution.

```bash
shellcheck -x script.sh
shellcheck -s bash -S warning script.sh
```

**Interpretation:** fix errors first, then warnings; use `# shellcheck disable=SC...` only with a comment explaining why. `-x` follows sourced files.

### `bash -n`

**Purpose:** parse without executing. It catches syntax errors, not logic or quoting bugs.

### `flock`

```bash
exec 9>/run/lock/myjob.lock
flock -n 9 || exit 3
```

**Why:** serialize cron/CI runs. The lock is advisory; every writer must use it.

### `timeout`

```bash
timeout --signal=TERM --kill-after=5s 30s command
```

**Interpretation:** exit 124 means timed out; `--kill-after` sends KILL if TERM is ignored. Do not wrap interactive commands.

### `jq`

```bash
jq -r '.items[] | [.name, .status] | @tsv' input.json
jq -e '.status == "ok"' input.json
```

**Why:** structured parsing, safe quoting, boolean exit status. `-e` is useful in tests/checks.

### `find` + `read`

```bash
find /path -type f -print0 | while IFS= read -r -d '' f; do
  printf '%q\n' "$f"
done
```

**Why:** handles arbitrary filenames. If you need the loop variable after the loop, use process substitution (`while ...; done < <(find ...)`) so the loop is not in a subshell.

### `trap`

```bash
trap 'echo cleanup' EXIT
trap 'echo "failed at $LINENO"' ERR
```

**Why:** deterministic cleanup and diagnostics. Test the trap paths (SIGINT, error, normal exit).

---

## 13. Failure scenarios

### A — Script exits early with no message

`set -e` exited on an unhandled command. Add an `ERR` trap and check the command explicitly. Run with `bash -x` locally, but redact secrets.

### B — Pipeline hides a failure

```bash
curl ... | jq ...   # jq succeeds on empty input; curl failed
```

`pipefail` fixes the status, but you still need `curl --fail` and a check for empty output.

### C — Word splitting breaks filenames

`for f in $(ls)` fails on spaces/globs. Use `find -print0` + `read -d ''`, or an array built with `mapfile`/`readarray` (Bash 4+).

### D — `rm -rf` deletes the wrong directory

Unset/empty variable, a trailing slash mistake, or a glob. Use `: "${VAR:?}"`, a case whitelist, and `--`. Test with `echo`/`ls` first.

### E — Two cron runs overlap

Use `flock`. A PID file alone is not enough; stale PIDs and reused PIDs cause false positives/negatives.

### F — Dry-run mutates state

A write happened outside the `run` wrapper. Audit every redirection, `install`, `sed -i`, `systemctl`, and API call; make reads explicit and writes centralized.

### G — Secret leaked in logs

`set -x`, `curl -v`, or `echo "$TOKEN"` wrote a credential. Disable tracing around secrets, use `--silent`, and redact headers. Rotate the exposed secret.

### H — SSH works interactively, fails in CI

CI has no agent/password prompt, unknown host key, or wrong key permissions. Use `BatchMode=yes`, a known_hosts entry, and a dedicated deploy key; check `ssh -vvv` locally with secrets redacted.

---

## 14. Checkpoint

1. What does `set -e` do, and name two contexts where it does not exit.
2. What does `set -o pipefail` change?
3. Why is `set -u` not a complete defense against empty variables?
4. What is the difference between an `ERR` trap and an `EXIT` trap?
5. Why does `trap cleanup EXIT` need to preserve `$?`?
6. What is a race condition that `flock` prevents?
7. Why is a PID file alone a weak lock?
8. What makes a script idempotent? Give a shell example.
9. Why is `for f in $(find ...)` unsafe?
10. How do you handle a filename with a newline?
11. Why is `rm -rf "$DIR"` dangerous, and how do you guard it?
12. What does `timeout` return on timeout?
13. Why should logs go to stderr and data to stdout?
14. What does `shellcheck` catch that `bash -n` does not?
15. What does a Bats test prove, and what does it not prove?
16. Why is `curl | bash` risky?
17. What is the risk of `set -x` in a script that handles tokens?
18. How do you make a dry-run trustworthy?
19. Why should you avoid `eval`?
20. What is the first thing you do when a script behaves differently in CI than locally?

### Answers

1. Exit on an unhandled non-zero simple command. It is ignored in `if`/`while`/`until` conditions, `&&`/`||`/`!` lists, and some function/subshell contexts.
2. A pipeline returns the rightmost non-zero status, so a failure in `curl | jq` is not hidden by a successful `jq`.
3. An unset variable errors, but an empty one (`VAR=""`) does not; `"$@"`/empty arrays and intentional empties need `${var:-}` or explicit checks.
4. `ERR` runs when a command fails (subject to `set -e` semantics); `EXIT` runs when the shell exits, normal or via `exit`.
5. `$?` at trap entry is the status you want to return; overwriting it with a cleanup command and exiting 0 hides the failure.
6. Two overlapping cron/CI runs mutating the same state; the lock serializes them.
7. PIDs are reused and stale files remain; `flock` ties the lock to an open file descriptor and releases it on exit.
8. It checks current state before changing and converges to the same result on a second run, e.g. `cmp -s` before `install`, `grep -qxF` before appending, `mkdir -p`.
9. Command substitution splits on whitespace/globs; filenames with spaces/newlines break, and the loop may process the wrong files.
10. `find -print0` + `while IFS= read -r -d ''`, or `mapfile -d ''` in Bash 4+.
11. An empty/unset variable becomes `/` or a broad path; guard with `: "${DIR:?}"`, reject `/`, `/home`, etc., require a directory, and use `--`.
12. 124 (unless the command handles the signal differently).
13. So `script > data.json` captures only data; diagnostics remain visible and do not corrupt the output.
14. ShellCheck catches quoting, `rm` hazards, unused/undefined variables, portability, and common logic bugs; `bash -n` only parses syntax.
15. It proves your script's behavior against the fixtures/fakes you provided; it does not prove the real service/OS behaves that way.
16. You execute unreviewed remote code with your privileges; a compromised URL/DNS/MITM can run anything. Download, verify a checksum/signature, review, then run.
17. Tracing prints expanded arguments, including tokens/passwords; disable it around secrets and redact logs.
18. All mutations go through the dry-run wrapper; reads are explicit; test that no files/services/API resources change.
19. It re-parses input as code, enabling injection and quoting bugs; use arrays and direct execution.
20. Compare the interpreter/Bash version, environment variables, working directory, PATH, user/permissions, SSH agent, and whether the tool is reading a TTY; reproduce with the same non-interactive shell.

---

## 15. Lab bridge

Now do [L7.1](05-labs-part1.md). You will turn the health-check idea into a tested script with fixtures, a lock, a dry-run, and a failure path. Do not copy the example verbatim; adapt it and explain every difference.
