# Phase 7 Labs — Part 1 (L7.1–L7.2)

> **Safety:** both labs are local and read-only against your own machine. L7.1 opens a listener on `127.0.0.1` for a test and uses `flock`; L7.2 reads fixture files and never calls a cloud API. Do not run the scripts against production endpoints until you have reviewed the thresholds, timeouts, and output paths.
>
> **Environment:** use a disposable Ubuntu 22.04/24.04 VM or container. `flock` and `/dev/tcp` are Linux/Bash features; macOS lacks `flock` by default. If you only have macOS, do L7.2 locally and run L7.1 in a Linux VM/container.

---

# Lab L7.1 — A Bash Health Check You Can Trust

## Objective

Build `svc-health.sh`, a Bash operations script that checks disk usage, HTTP endpoints, and TCP ports; returns monitoring-friendly exit codes; separates logs from machine output; prevents overlapping runs with `flock`; supports a truthful dry-run; writes an atomic JSON report; and is tested with ShellCheck and Bats.

The point is not the checks. The point is the **contract**: predictable arguments, exit codes, output streams, locking, timeouts, and failure behavior.

## Prerequisites

- Linux VM/container with Bash 5.x.
- `curl`, `jq`, `flock` (util-linux), `timeout` (coreutils), `shellcheck`, `bats`.
- A non-root user is fine. No sudo is needed at runtime.
- 45–60 minutes.

## Setup

```bash
sudo apt-get update
sudo apt-get install -y shellcheck bats jq curl util-linux coreutils

bash --version | head -1
shellcheck --version | head -2
bats --version
jq --version
```

If `bats` is not in your distribution's repositories:

```bash
git clone --depth 1 https://github.com/bats-core/bats-core.git "$HOME/.local/src/bats-core"
"$HOME/.local/src/bats-core/bin/bats" --version
```

Create the project:

```bash
mkdir -p ~/opslab/phase7/l7.1/{bin,tests,reports}
cd ~/opslab/phase7/l7.1
```

Expected layout:

```text
l7.1/
├── bin/
│   └── svc-health.sh
├── tests/
│   └── svc-health.bats
└── reports/
```

## Part A — Write the script

Create `bin/svc-health.sh`:

```bash
#!/usr/bin/env bash
set -Eeuo pipefail

readonly PROG=${0##*/}
readonly VERSION=0.1.0

warn_pct=80
crit_pct=95
timeout_s=3
disk_path=/
dry_run=false
json=false
output_file=""
endpoints=()
tcp_targets=()

usage() {
  cat <<EOF
Usage: $PROG [options]

Checks disk usage, HTTP endpoints, and TCP ports.
Exit codes: 0 ok, 1 warning, 2 critical, 3 usage/config/internal.

Options:
  -e URL          HTTP(S) endpoint to check (repeatable)
  -p HOST:PORT    TCP target to check (repeatable)
  -d PATH         Filesystem path to check (default: /)
  -w PERCENT      Disk warning threshold (default: 80)
  -c PERCENT      Disk critical threshold (default: 95)
  -t SECONDS      Per-check timeout (default: 3)
  -o FILE         Write the JSON report atomically to FILE
  -n              Dry run: validate arguments and print the plan
  -j              JSON output on stdout
  -h              Show this help
  -V              Show version

Examples:
  $PROG -d /var -e http://127.0.0.1:8080/healthz -p 127.0.0.1:5432
  $PROG -j -o reports/health.json -e https://example.com/health
EOF
}

die() {
  printf '%s: %s\n' "$PROG" "$*" >&2
  exit 3
}

log() {
  local level=$1
  shift
  printf '%s %-5s %s\n' "$(date -u +%FT%TZ)" "$level" "$*" >&2
}

need() {
  command -v "$1" >/dev/null 2>&1 || die "missing dependency: $1"
}

is_int() {
  [[ $1 =~ ^[0-9]+$ ]]
}

while getopts ":e:p:d:w:c:t:o:njhV" opt; do
  case $opt in
    e)
      [[ ${OPTARG:-} =~ ^https?:// ]] || die "-e requires an http(s) URL"
      endpoints+=("$OPTARG")
      ;;
    p)
      [[ ${OPTARG:-} == *:* ]] || die "-p requires HOST:PORT"
      tcp_targets+=("$OPTARG")
      ;;
    d) disk_path=${OPTARG:-} ;;
    w) warn_pct=${OPTARG:-} ;;
    c) crit_pct=${OPTARG:-} ;;
    t) timeout_s=${OPTARG:-} ;;
    o) output_file=${OPTARG:-} ;;
    n) dry_run=true ;;
    j) json=true ;;
    h) usage; exit 0 ;;
    V) printf '%s %s\n' "$PROG" "$VERSION"; exit 0 ;;
    :)
      die "option -$OPTARG requires an argument"
      ;;
    \?)
      die "unknown option -$OPTARG (try -h)"
      ;;
  esac
done
shift $((OPTIND - 1))
[[ $# -eq 0 ]] || die "unexpected argument: $1"

is_int "$warn_pct" || die "-w must be an integer"
is_int "$crit_pct" || die "-c must be an integer"
is_int "$timeout_s" || die "-t must be an integer"
(( warn_pct < crit_pct )) || die "-w must be less than -c"
(( timeout_s > 0 )) || die "-t must be greater than 0"
[[ -d $disk_path ]] || die "not a directory: $disk_path"

if [[ $dry_run == true ]]; then
  log INFO "dry-run: disk path=${disk_path} warn=${warn_pct}% crit=${crit_pct}%"
  for endpoint in "${endpoints[@]}"; do
    log INFO "dry-run: would check HTTP ${endpoint} (timeout ${timeout_s}s)"
  done
  for target in "${tcp_targets[@]}"; do
    log INFO "dry-run: would check TCP ${target} (timeout ${timeout_s}s)"
  done
  log INFO "dry-run: no checks performed"
  exit 0
fi

need jq
need curl
need flock
need timeout
need awk
need hostname

if [[ -n $output_file ]]; then
  output_dir=$(dirname "$output_file")
  [[ -d $output_dir ]] || die "output directory does not exist: $output_dir"
  tmp_output=$(mktemp "${output_dir}/.svc-health.XXXXXX") \
    || die "could not create a temporary file in ${output_dir}"
  trap 'rm -f "${tmp_output:-}"' EXIT
fi

lock_file=${LOCK_FILE:-${XDG_RUNTIME_DIR:-/tmp}/svc-health.lock}
[[ -d $(dirname "$lock_file") ]] || die "lock directory does not exist: $(dirname "$lock_file")"
exec 9>"$lock_file"
flock -n 9 || die "another run holds $lock_file"

severity=0
results=()

record() {
  local status=$1
  local name=$2
  local detail=$3
  local code

  case $status in
    OK) code=0 ;;
    WARN) code=1 ;;
    CRIT) code=2 ;;
    *) die "internal error: bad status ${status}" ;;
  esac

  if (( code > severity )); then
    severity=$code
  fi

  results+=("$(jq -nc \
    --arg status "$status" \
    --arg name "$name" \
    --arg detail "$detail" \
    '{status:$status, name:$name, detail:$detail}')")
  log "$status" "${name}: ${detail}"
}

check_disk() {
  local use
  if ! use=$(df -P "$disk_path" | awk 'NR==2 { gsub(/%/, "", $5); print $5 }'); then
    record CRIT disk "df failed for ${disk_path}"
    return
  fi
  if [[ ! $use =~ ^[0-9]+$ ]]; then
    record CRIT disk "could not parse df output for ${disk_path}"
    return
  fi

  if (( use >= crit_pct )); then
    record CRIT disk "${disk_path} at ${use}% (critical >= ${crit_pct}%)"
  elif (( use >= warn_pct )); then
    record WARN disk "${disk_path} at ${use}% (warning >= ${warn_pct}%)"
  else
    record OK disk "${disk_path} at ${use}%"
  fi
}

check_http() {
  local url=$1
  local code

  code=$(curl -sS -o /dev/null -w '%{http_code}' \
    --max-time "$timeout_s" "$url" 2>/dev/null || true)

  case $code in
    2[0-9][0-9])
      record OK "http:${url}" "HTTP ${code}"
      ;;
    3[0-9][0-9])
      record WARN "http:${url}" "HTTP ${code} (redirect; use -L only if intended)"
      ;;
    *)
      record CRIT "http:${url}" "HTTP ${code:-000} (expected 2xx)"
      ;;
  esac
}

check_tcp() {
  local target=$1
  local host=${target%:*}
  local port=${target##*:}

  if [[ ! $host =~ ^[A-Za-z0-9.-]+$ ]] || [[ ! $port =~ ^[0-9]+$ ]]; then
    record CRIT "tcp:${target}" "invalid host or port"
    return
  fi
  if (( port < 1 || port > 65535 )); then
    record CRIT "tcp:${target}" "port out of range"
    return
  fi

  # Pass host/port as positional args so they are never interpolated into code.
  if timeout "$timeout_s" bash -c 'exec 3<>"/dev/tcp/$1/$2"' _ "$host" "$port" 2>/dev/null; then
    record OK "tcp:${target}" "connected"
  else
    record CRIT "tcp:${target}" "connection failed within ${timeout_s}s"
  fi
}

json_report() {
  local checks
  checks=$(printf '%s\n' "${results[@]}" | jq -s '.')
  jq -n \
    --arg timestamp "$(date -u +%FT%TZ)" \
    --arg host "$(hostname)" \
    --argjson severity "$severity" \
    --argjson checks "$checks" \
    '{timestamp:$timestamp, host:$host, severity:$severity, checks:$checks}'
}

emit() {
  local payload
  payload=$(json_report)

  if [[ $json == true ]]; then
    printf '%s\n' "$payload"
  else
    printf 'SUMMARY severity=%d checks=%d\n' "$severity" "${#results[@]}"
  fi

  if [[ -n $output_file ]]; then
    printf '%s\n' "$payload" >"$tmp_output"
    mv "$tmp_output" "$output_file"
  fi
}

log INFO "starting checks (version ${VERSION})"
check_disk

for endpoint in "${endpoints[@]}"; do
  check_http "$endpoint"
done

for target in "${tcp_targets[@]}"; do
  check_tcp "$target"
done

emit
exit "$severity"
```

Make it executable and parse-check it:

```bash
chmod +x bin/svc-health.sh
bash -n bin/svc-health.sh
shellcheck -x bin/svc-health.sh
```

Expected: `bash -n` and `shellcheck` produce no output. If ShellCheck reports `SC2086`, `SC2317`, or `SC2015`, fix the code rather than disabling the rule.

## Part B — Run it

Start a local HTTP server for a positive test:

```bash
python3 -m http.server 18080 --bind 127.0.0.1 &
SERVER_PID=$!
sleep 0.5
curl -sS -o /dev/null -w '%{http_code}\n' http://127.0.0.1:18080/
```

Expected `200`. Run the health check:

```bash
./bin/svc-health.sh \
  -d / \
  -e http://127.0.0.1:18080/ \
  -p 127.0.0.1:18080
echo "exit=$?"
```

Representative output (timestamps and percentages differ):

```text
2026-10-27T09:12:01Z INFO  starting checks (version 0.1.0)
2026-10-27T09:12:01Z OK    disk: / at 43%
2026-10-27T09:12:01Z OK    http:http://127.0.0.1:18080/: HTTP 200
2026-10-27T09:12:01Z OK    tcp:127.0.0.1:18080: connected
2026-10-27T09:12:02Z INFO  summary severity=0 checks=3
SUMMARY severity=0 checks=3
exit=0
```

Now a failure case:

```bash
./bin/svc-health.sh -p 127.0.0.1:9; echo "exit=$?"
```

Expected:

```text
... CRIT  tcp:127.0.0.1:9: connection failed within 3s
... INFO  summary severity=2 checks=1
SUMMARY severity=2 checks=1
exit=2
```

JSON output, with logs on stderr:

```bash
./bin/svc-health.sh -j -e http://127.0.0.1:18080/ -p 127.0.0.1:9 \
  > /tmp/health.json 2>/tmp/health.log
echo "exit=$?"
jq . /tmp/health.json
```

Expected JSON shape:

```json
{
  "timestamp": "2026-10-27T09:14:22Z",
  "host": "opslab-vm",
  "severity": 2,
  "checks": [
    {
      "status": "OK",
      "name": "http:http://127.0.0.1:18080/",
      "detail": "HTTP 200"
    },
    {
      "status": "CRIT",
      "name": "tcp:127.0.0.1:9",
      "detail": "connection failed within 3s"
    }
  ]
}
```

Test the atomic file output and dry-run:

```bash
./bin/svc-health.sh -o reports/health.json -p 127.0.0.1:9 >/dev/null 2>&1
echo "exit=$?"
ls -l reports/
jq -e '.severity == 2' reports/health.json

./bin/svc-health.sh -n -e http://127.0.0.1:18080/ -p 127.0.0.1:9
echo "exit=$?"
```

Expected: `reports/health.json` exists and validates; dry-run exits 0 without connecting to anything.

Test lock contention in two terminals:

```bash
# Terminal 1: hold the lock for 10 seconds
flock /tmp/svc-health.lock sleep 10

# Terminal 2: run the check
./bin/svc-health.sh -p 127.0.0.1:9; echo "exit=$?"
```

Expected in terminal 2:

```text
svc-health.sh: another run holds /tmp/svc-health.lock
exit=3
```

Stop the HTTP server when done:

```bash
kill "$SERVER_PID"
wait "$SERVER_PID" 2>/dev/null || true
```

## Part C — Bats tests

Create `tests/svc-health.bats`:

```bash
#!/usr/bin/env bats

setup() {
  export LOCK_FILE="$BATS_TEST_TMPDIR/svc-health.lock"
  SCRIPT="$BATS_TEST_DIRNAME/../bin/svc-health.sh"
}

@test "help exits 0 and prints usage" {
  run "$SCRIPT" -h
  [ "$status" -eq 0 ]
  [[ $output == *"Usage:"* ]]
}

@test "version exits 0" {
  run "$SCRIPT" -V
  [ "$status" -eq 0 ]
  [[ $output == *"0.1.0"* ]]
}

@test "unknown option exits 3" {
  run "$SCRIPT" -z
  [ "$status" -eq 3 ]
  [[ $output == *"unknown option"* ]]
}

@test "missing option argument exits 3" {
  run "$SCRIPT" -e
  [ "$status" -eq 3 ]
  [[ $output == *"requires an argument"* ]]
}

@test "inverted thresholds exit 3" {
  run "$SCRIPT" -w 90 -c 80
  [ "$status" -eq 3 ]
  [[ $output == *"less than"* ]]
}

@test "closed TCP port is critical" {
  run "$SCRIPT" -p 127.0.0.1:9
  [ "$status" -eq 2 ]
}

@test "JSON report parses and has severity 2" {
  # Logs go to stderr; keep stdout clean for jq.
  run bash -c '"$1" -j -p 127.0.0.1:9 2>/dev/null' _ "$SCRIPT"
  [ "$status" -eq 2 ]
  echo "$output" | jq -e '.severity == 2 and (.checks | length) == 1'
}

@test "dry-run does not perform checks" {
  run "$SCRIPT" -n -p 127.0.0.1:9
  [ "$status" -eq 0 ]
  [[ $output == *"dry-run"* ]]
  [[ $output == *"no checks performed"* ]]
}

@test "lock contention exits 3" {
  flock "$LOCK_FILE" sleep 5 &
  holder=$!
  sleep 0.3

  run "$SCRIPT" -p 127.0.0.1:9
  [ "$status" -eq 3 ]
  [[ $output == *"another run holds"* ]]

  kill "$holder" 2>/dev/null || true
  wait "$holder" 2>/dev/null || true
}
```

Run:

```bash
bats tests/svc-health.bats
```

Expected:

```text
1..9
ok 1 help exits 0 and prints usage
ok 2 version exits 0
ok 3 unknown option exits 3
ok 4 missing option argument exits 3
ok 5 inverted thresholds exit 3
ok 6 closed TCP port is critical
ok 7 JSON report parses and has severity 2
ok 8 dry-run does not perform checks
ok 9 lock contention exits 3
```

If test 9 is flaky, the holder may not have acquired the lock before the script runs. Increase the `sleep 0.3` or use a readiness check (`flock -n "$LOCK_FILE" true; echo $?`) before running. Do not remove the test because it is inconvenient; the lock is the feature.

## Internal explanation

### Why these exit codes

`0/1/2/3` follows the common monitoring convention:

| Code | Meaning | Caller action |
|---|---|---|
| 0 | OK | Do nothing |
| 1 | WARNING | Open a ticket; watch the trend |
| 2 | CRITICAL | Page/alert; investigate now |
| 3 | UNKNOWN/usage | Fix the check itself; do not page the service owner |

A CI job or monitoring agent can branch on the code without parsing text. If you return 0 for a failed check because "the script ran," you have made the monitoring useless.

### Why logs go to stderr and data to stdout

`-j` writes one JSON document to stdout; logs, timestamps, and warnings go to stderr. This lets a caller do:

```bash
svc-health.sh -j ... | jq .
```

without stripping log lines. If you print logs to stdout, you must parse around them, and every parser eventually breaks.

### Why `set -e` is not enough

`set -e` does not exit in every context. It is disabled inside conditions (`if`, `while`, `&&`, `||`, `!`) and can be bypassed by command substitutions, subshells, and functions called in a conditional. The script uses explicit checks for `df`, `curl`, and `flock`, and avoids relying on `-e` for control flow.

Example of the trap:

```bash
# This does NOT exit if grep fails; the if consumes the failure.
if grep -q pattern file; then
  ...
fi

# This DOES exit if the assignment fails, but only because it is a simple command.
value=$(command_that_can_fail)
```

`pipefail` makes a pipeline fail if any stage fails, not just the last one. Without it, `df | awk` could report success even if `df` failed and `awk` printed nothing.

### Why `curl` uses `-w` and not `-f`

`curl -f` returns a non-zero exit code for HTTP 4xx/5xx and suppresses the body. That is useful for downloads, but here we want the status code in the report. `-w '%{http_code}'` prints `000` when the connection fails, which we map to CRITICAL. `--max-time` bounds DNS, connect, and transfer time.

### Why `/dev/tcp` with positional arguments

`/dev/tcp/host/port` is a Bash feature, not a file. The script runs it in a child Bash and passes the host and port as `$1`/`$2`, so a malicious value cannot become part of the script text. The `timeout` command bounds the connection attempt. `nc` would also work, but its flags and exit codes vary across implementations; `/dev/tcp` is available wherever Bash is.

### Why `flock`

Cron jobs and alert retries overlap. Two checks writing the same report or hammering the same endpoint can produce confusing results. `flock -n 9` takes an exclusive lock on file descriptor 9 without waiting; if another process holds it, the script exits 3 instead of queueing. The kernel releases the lock when the process exits, so there is no stale PID file to clean up.

### Why atomic output

`printf > file` truncates the file before writing. A reader can see a partial JSON document. `mktemp` + `mv` writes the full content to a temporary file in the same directory, then renames it atomically. The `trap` removes the temporary file if the script exits before the rename. Same-filesystem rename is atomic on POSIX.

### Why the dry-run performs no checks

A dry-run that contacts endpoints is not a dry-run; it can trigger rate limits, wake services, or leak a request. This script's checks are read-only, but the contract is still useful: `-n` validates arguments and prints the plan, so you can review a long command before running it in a change window.

## Troubleshooting task

Work in a scratch copy. After each change, record the symptom, evidence, and fix.

1. **Connection refused vs DNS failure.** Run:

```bash
./bin/svc-health.sh -e http://127.0.0.1:9/; echo "exit=$?"
./bin/svc-health.sh -e http://no-such-host.invalid/; echo "exit=$?"
```

Both produce HTTP `000` and exit 2. Explain why the report cannot distinguish them without more detail, and add a `detail` that includes `curl`'s exit code (for example, 7 = connection refused, 6 = DNS). Do not print the response body; it may contain sensitive data.

2. **Threshold boundary.** Run:

```bash
./bin/svc-health.sh -d / -w 1 -c 100; echo "exit=$?"
```

Expected: WARNING (exit 1) if disk usage is at least 1% and below 100%. Explain why `-w 1 -c 100` is a useful test and why `-w 100 -c 100` is rejected.

3. **Inverted thresholds.** Run:

```bash
./bin/svc-health.sh -w 95 -c 80; echo "exit=$?"
```

Expected: usage error, exit 3, no checks. A critical threshold below the warning threshold is a configuration bug; fail before checking.

4. **Lock contention.** Hold the lock in one terminal and run the script in another. Record the exact message and exit code. Then remove the holder and confirm the script runs. Do not delete the lock file manually; `flock` uses the file descriptor, and deleting the path can create a new inode with a different lock.

5. **Read-only output directory.** Run:

```bash
mkdir -p /tmp/health-ro
chmod 0555 /tmp/health-ro
./bin/svc-health.sh -o /tmp/health-ro/report.json -p 127.0.0.1:9; echo "exit=$?"
chmod 0755 /tmp/health-ro
```

Expected: exit 3 with a clear `mktemp` or permission error, not a partial report. If you run as root, the permission test may succeed; use a non-root user or skip with an explanation.

6. **ShellCheck regression.** Temporarily change `"$disk_path"` to `$disk_path` in the `df` call and run `shellcheck`. Observe `SC2086`, then revert. Explain why quoting matters even when the current path has no spaces.

## Verification

- [ ] `bash -n bin/svc-health.sh` passes.
- [ ] `shellcheck -x bin/svc-health.sh` passes.
- [ ] `bats tests/svc-health.bats` passes all tests.
- [ ] A healthy endpoint + open port exits 0.
- [ ] A closed port exits 2 and names the target.
- [ ] A warning threshold exits 1; a critical threshold exits 2.
- [ ] `-j` output parses with `jq` and logs do not contaminate stdout.
- [ ] `-o` writes a complete JSON file atomically; no `.svc-health.*` temp file remains.
- [ ] `-n` performs no network checks and exits 0.
- [ ] A second concurrent run exits 3 with a lock message.
- [ ] You can explain every `set` option and every explicit error check.

## Cleanup

```bash
# Stop the test server if it is still running.
kill "$SERVER_PID" 2>/dev/null || true
wait "$SERVER_PID" 2>/dev/null || true

# Remove scratch output and any held lock holder.
rm -rf reports /tmp/health-ro /tmp/health.json /tmp/health.log
rm -f /tmp/svc-health.lock

# Optional: remove the whole lab directory after the write-up.
# rm -rf ~/opslab/phase7/l7.1
```

Do not remove `bin/svc-health.sh` if you plan to reuse it in L7.4; copy it into the pipeline repository instead.

## Write-up template

```markdown
# L7.1 — Bash health check

## Contract
- Arguments:
- Exit codes:
- Stdout format:
- Stderr format:
- Lock file:
- Output file:

## Evidence
| Check | Command | Exit | Observation |
|---|---|---|---|
| Healthy HTTP + TCP | | | |
| Closed TCP | | | |
| Warning threshold | | | |
| Critical threshold | | | |
| Inverted thresholds | | | |
| JSON output | | | |
| Atomic file | | | |
| Dry-run | | | |
| Lock contention | | | |
| Bats suite | | | |

## Failure behavior
- DNS failure vs refused connection:
- Missing output directory:
- Partial report prevention:
- Concurrent run:

## ShellCheck
- Warnings found:
- Fixes:
- Suppressions (with justification):

## What I would change for production
- Thresholds:
- Timeout:
- Authentication/headers:
- Alert routing:
- Log redaction:
- Packaging/scheduling:
```

---

# Lab L7.2 — A Python Capacity Report

## Objective

Build `capacity-report`, a typed Python CLI that joins server inventory with flavor definitions, aggregates requested vCPU/RAM/disk per project and in total, compares usage against per-project capacity limits, emits a table or JSON, and exits with a distinct code when utilization exceeds a threshold. Add unit tests that run without a cloud account.

## Prerequisites

- Python 3.11+ and `venv`.
- `git`, `jq` (for inspecting JSON), and a terminal.
- No OpenStack/AWS account. Fixtures are enough.
- 60–90 minutes.

## Setup

```bash
mkdir -p ~/opslab/phase7/l7.2
cd ~/opslab/phase7/l7.2

python3 -m venv .venv
source .venv/bin/activate
python -m pip install -U pip
mkdir -p src/capacity_report tests/fixtures
```

Create `pyproject.toml`:

```toml
[build-system]
requires = ["setuptools>=68"]
build-backend = "setuptools.build_meta"

[project]
name = "capacity-report"
version = "0.1.0"
requires-python = ">=3.11"
dependencies = []

[project.optional-dependencies]
dev = ["pytest>=8", "ruff>=0.6", "mypy>=1.10"]

[project.scripts]
capacity-report = "capacity_report.cli:main"

[tool.setuptools.packages.find]
where = ["src"]

[tool.ruff]
line-length = 100
target-version = "py311"

[tool.mypy]
strict = true
```

Create `src/capacity_report/__init__.py` (empty) and install:

```bash
touch src/capacity_report/__init__.py
pip install -e '.[dev]'
```

## Part A — Fixtures

`tests/fixtures/flavors.json`:

```json
[
  {"Name": "m1.small", "VCPUs": 2, "RAM": 2048, "Disk": 20},
  {"Name": "m1.medium", "VCPUs": 4, "RAM": 4096, "Disk": 40}
]
```

`tests/fixtures/servers.json`:

```json
[
  {"Name": "web-1", "Project": "demo", "Flavor": "m1.small", "Status": "ACTIVE"},
  {"Name": "web-2", "Project": "demo", "Flavor": "m1.small", "Status": "ACTIVE"},
  {"Name": "db-1", "Project": "platform", "Flavor": "m1.medium", "Status": "ACTIVE"},
  {"Name": "old-1", "Project": "demo", "Flavor": "m1.small", "Status": "DELETED"}
]
```

`tests/fixtures/limits.json`:

```json
{
  "projects": {
    "demo": {"vcpus": 4, "ram_mb": 8192, "disk_gb": 100},
    "platform": {"vcpus": 16, "ram_mb": 32768, "disk_gb": 200}
  }
}
```

`tests/fixtures/servers-unknown.json`:

```json
[
  {"Name": "web-1", "Project": "demo", "Flavor": "m1.small", "Status": "ACTIVE"},
  {"Name": "bad-1", "Project": "demo", "Flavor": "m1.large", "Status": "ACTIVE"}
]
```

Expected aggregation:

| Project | Servers | vCPU | RAM MB | Disk GB |
|---|---|---|---|---|
| demo | 2 | 4 | 4096 | 40 |
| platform | 1 | 4 | 4096 | 40 |
| TOTAL | 3 | 8 | 8192 | 80 |

`demo` is at 100% of its 4-vCPU limit, so it should be flagged at the default 85% threshold.

## Part B — Models

`src/capacity_report/models.py`:

```python
from dataclasses import dataclass


@dataclass(frozen=True)
class Flavor:
    name: str
    vcpus: int
    ram_mb: int
    disk_gb: int


@dataclass(frozen=True)
class Server:
    name: str
    project: str
    flavor: str
    status: str


@dataclass
class Usage:
    project: str
    servers: int = 0
    vcpus: int = 0
    ram_mb: int = 0
    disk_gb: int = 0


@dataclass(frozen=True)
class Limit:
    vcpus: int
    ram_mb: int
    disk_gb: int


@dataclass(frozen=True)
class Finding:
    project: str
    resource: str
    used: int
    limit: int
    ratio: float | None  # None means the limit is zero and usage is non-zero


@dataclass
class Report:
    projects: list[Usage]
    total: Usage
    limits: dict[str, Limit]
    total_limit: Limit
    findings: list[Finding]
```

Why frozen for `Flavor`, `Server`, `Limit`, and `Finding`? They are value objects; immutability makes them safe to share and catches accidental mutation. `Usage` is mutable because the aggregation loop updates counters.

## Part C — Join and validate

`src/capacity_report/report.py`:

```python
import json
from dataclasses import asdict
from pathlib import Path

from .models import Finding, Flavor, Limit, Report, Server, Usage


def _load_array(path: Path) -> list[dict]:
    try:
        raw = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        raise ValueError(f"{path}: invalid JSON: {exc}") from exc
    if not isinstance(raw, list):
        raise ValueError(f"{path}: expected a JSON array")
    for index, item in enumerate(raw):
        if not isinstance(item, dict):
            raise ValueError(f"{path}: item {index} is not an object")
    return raw


def _int_field(raw: dict, key: str, context: str) -> int:
    try:
        value = int(raw[key])
    except (KeyError, TypeError, ValueError) as exc:
        raise ValueError(f"{context}: missing or invalid {key!r}") from exc
    if value < 0:
        raise ValueError(f"{context}: {key} must be >= 0")
    return value


def parse_flavors(path: Path) -> dict[str, Flavor]:
    flavors: dict[str, Flavor] = {}
    for index, raw in enumerate(_load_array(path)):
        context = f"{path}: flavor {index}"
        name = str(raw.get("Name", "")).strip()
        if not name:
            raise ValueError(f"{context}: empty Name")
        if name in flavors:
            raise ValueError(f"{context}: duplicate flavor {name!r}")
        flavors[name] = Flavor(
            name=name,
            vcpus=_int_field(raw, "VCPUs", context),
            ram_mb=_int_field(raw, "RAM", context),
            disk_gb=_int_field(raw, "Disk", context),
        )
    if not flavors:
        raise ValueError(f"{path}: no flavors")
    return flavors


def parse_servers(path: Path) -> list[Server]:
    servers: list[Server] = []
    for index, raw in enumerate(_load_array(path)):
        context = f"{path}: server {index}"
        name = str(raw.get("Name", "")).strip()
        flavor = str(raw.get("Flavor", "")).strip()
        if not name or not flavor:
            raise ValueError(f"{context}: empty Name or Flavor")
        servers.append(
            Server(
                name=name,
                project=str(raw.get("Project") or "unknown").strip(),
                flavor=flavor,
                status=str(raw.get("Status") or "UNKNOWN").upper(),
            )
        )
    return servers


def parse_limits(path: Path) -> dict[str, Limit]:
    try:
        raw = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        raise ValueError(f"{path}: invalid JSON: {exc}") from exc
    if not isinstance(raw, dict):
        raise ValueError(f"{path}: expected a JSON object")

    projects = raw.get("projects")
    if not isinstance(projects, dict) or not projects:
        raise ValueError(f"{path}: expected a non-empty 'projects' object")

    limits: dict[str, Limit] = {}
    for project, values in projects.items():
        context = f"{path}: project {project!r}"
        if not isinstance(values, dict):
            raise ValueError(f"{context}: limit must be an object")
        limits[str(project)] = Limit(
            vcpus=_int_field(values, "vcpus", context),
            ram_mb=_int_field(values, "ram_mb", context),
            disk_gb=_int_field(values, "disk_gb", context),
        )
    return limits


def _collect_findings(
    findings: list[Finding],
    project: str,
    usage: Usage,
    limit: Limit,
    max_utilization: float,
) -> None:
    for resource, used, capacity in (
        ("vcpus", usage.vcpus, limit.vcpus),
        ("ram_mb", usage.ram_mb, limit.ram_mb),
        ("disk_gb", usage.disk_gb, limit.disk_gb),
    ):
        if capacity == 0:
            if used > 0:
                findings.append(
                    Finding(project=project, resource=resource, used=used, limit=0, ratio=None)
                )
            continue
        ratio = used / capacity
        if ratio > max_utilization:
            findings.append(
                Finding(
                    project=project,
                    resource=resource,
                    used=used,
                    limit=capacity,
                    ratio=ratio,
                )
            )


def build_report(
    servers_path: Path,
    flavors_path: Path,
    limits_path: Path,
    max_utilization: float,
) -> Report:
    flavors = parse_flavors(flavors_path)
    limits = parse_limits(limits_path)
    by_project: dict[str, Usage] = {}
    total = Usage(project="TOTAL")

    for server in parse_servers(servers_path):
        if server.status == "DELETED":
            continue
        try:
            flavor = flavors[server.flavor]
        except KeyError as exc:
            raise ValueError(
                f"unknown flavor {server.flavor!r} for server {server.name!r}"
            ) from exc
        if server.project not in limits:
            raise ValueError(
                f"no capacity limit for project {server.project!r} "
                f"(server {server.name!r})"
            )

        usage = by_project.setdefault(server.project, Usage(project=server.project))
        for item in (usage, total):
            item.servers += 1
            item.vcpus += flavor.vcpus
            item.ram_mb += flavor.ram_mb
            item.disk_gb += flavor.disk_gb

    if not by_project:
        raise ValueError(f"{servers_path}: no non-deleted servers")

    total_limit = Limit(
        vcpus=sum(limit.vcpus for limit in limits.values()),
        ram_mb=sum(limit.ram_mb for limit in limits.values()),
        disk_gb=sum(limit.disk_gb for limit in limits.values()),
    )

    findings: list[Finding] = []
    for project, usage in sorted(by_project.items()):
        _collect_findings(findings, project, usage, limits[project], max_utilization)
    _collect_findings(findings, "TOTAL", total, total_limit, max_utilization)

    return Report(
        projects=sorted(by_project.values(), key=lambda item: item.project),
        total=total,
        limits=limits,
        total_limit=total_limit,
        findings=findings,
    )


def _pct(used: int, capacity: int) -> float | None:
    if capacity == 0:
        return None
    return used / capacity * 100.0


def _fmt_pct(value: float | None) -> str:
    return "n/a" if value is None else f"{value:.1f}"


def render(report: Report, fmt: str, max_utilization: float) -> str:
    if fmt == "json":
        payload = {
            "max_utilization": max_utilization,
            "status": "over_threshold" if report.findings else "ok",
            "projects": [
                {
                    **asdict(usage),
                    "limit": asdict(report.limits[usage.project]),
                    "utilization_pct": {
                        "vcpus": _pct(usage.vcpus, report.limits[usage.project].vcpus),
                        "ram_mb": _pct(usage.ram_mb, report.limits[usage.project].ram_mb),
                        "disk_gb": _pct(usage.disk_gb, report.limits[usage.project].disk_gb),
                    },
                }
                for usage in report.projects
            ],
            "total": {
                **asdict(report.total),
                "limit": asdict(report.total_limit),
                "utilization_pct": {
                    "vcpus": _pct(report.total.vcpus, report.total_limit.vcpus),
                    "ram_mb": _pct(report.total.ram_mb, report.total_limit.ram_mb),
                    "disk_gb": _pct(report.total.disk_gb, report.total_limit.disk_gb),
                },
            },
            "findings": [asdict(finding) for finding in report.findings],
        }
        return json.dumps(payload, indent=2)

    headers = ("PROJECT", "SERVERS", "VCPU", "RAM_MB", "DISK_GB", "VCPU%", "RAM%", "DISK%")
    rows = [headers]
    for usage in [*report.projects, report.total]:
        limit = report.total_limit if usage.project == "TOTAL" else report.limits[usage.project]
        rows.append(
            (
                usage.project,
                str(usage.servers),
                str(usage.vcpus),
                str(usage.ram_mb),
                str(usage.disk_gb),
                _fmt_pct(_pct(usage.vcpus, limit.vcpus)),
                _fmt_pct(_pct(usage.ram_mb, limit.ram_mb)),
                _fmt_pct(_pct(usage.disk_gb, limit.disk_gb)),
            )
        )

    widths = [max(len(row[index]) for row in rows) for index in range(len(headers))]
    table = "\n".join(
        "  ".join(cell.ljust(widths[index]) for index, cell in enumerate(row)) for row in rows
    )

    if not report.findings:
        return f"{table}\n\nAll projects within {max_utilization * 100:.1f}% of capacity."

    lines = [table, "", f"OVER THRESHOLD (max {max_utilization * 100:.1f}%):"]
    for finding in report.findings:
        ratio = "n/a" if finding.ratio is None else f"{finding.ratio * 100:.1f}%"
        lines.append(
            f"  {finding.project} {finding.resource} "
            f"{finding.used}/{finding.limit} ({ratio})"
        )
    return "\n".join(lines)
```

## Part D — CLI

`src/capacity_report/cli.py`:

```python
import argparse
import logging
import sys
from pathlib import Path

from .report import build_report, render


def _ratio(value: str) -> float:
    try:
        ratio = float(value)
    except ValueError as exc:
        raise argparse.ArgumentTypeError("must be a number") from exc
    if not 0 < ratio <= 1:
        raise argparse.ArgumentTypeError("must be in (0, 1]")
    return ratio


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="capacity-report")
    parser.add_argument(
        "--servers",
        type=Path,
        required=True,
        help="JSON array from: openstack server list --long -f json",
    )
    parser.add_argument(
        "--flavors",
        type=Path,
        required=True,
        help="JSON array from: openstack flavor list -f json",
    )
    parser.add_argument(
        "--limits",
        type=Path,
        required=True,
        help="JSON object with per-project capacity limits",
    )
    parser.add_argument("--format", choices=("table", "json"), default="table")
    parser.add_argument(
        "--max-utilization",
        type=_ratio,
        default=0.85,
        help="flag usage above this fraction of capacity (default: 0.85)",
    )
    parser.add_argument("-v", "--verbose", action="store_true")
    args = parser.parse_args(argv)

    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.WARNING,
        format="%(asctime)s %(levelname)s %(message)s",
        stream=sys.stderr,
    )

    try:
        report = build_report(
            args.servers,
            args.flavors,
            args.limits,
            args.max_utilization,
        )
    except (OSError, ValueError) as exc:
        logging.error("%s", exc)
        return 1

    print(render(report, args.format, args.max_utilization))
    return 3 if report.findings else 0


if __name__ == "__main__":
    raise SystemExit(main())
```

Exit codes:

| Code | Meaning |
|---|---|
| 0 | Report generated; no project over threshold |
| 1 | Data/runtime error (missing file, bad JSON, unknown flavor, missing limit) |
| 2 | Usage error (argparse) |
| 3 | Report generated; at least one utilization finding |

A distinct code for "over threshold" lets CI fail the job without mistaking a capacity finding for a broken tool.

## Part E — Tests

`tests/test_report.py`:

```python
import json
from pathlib import Path

import pytest

from capacity_report.cli import main
from capacity_report.report import build_report, render

FIXTURES = Path(__file__).parent / "fixtures"


def _report(max_utilization: float = 0.85):
    return build_report(
        FIXTURES / "servers.json",
        FIXTURES / "flavors.json",
        FIXTURES / "limits.json",
        max_utilization,
    )


def test_groups_and_totals() -> None:
    report = _report()
    demo = next(project for project in report.projects if project.project == "demo")
    assert demo.servers == 2
    assert demo.vcpus == 4
    assert demo.ram_mb == 4096
    assert demo.disk_gb == 40
    assert report.total.servers == 3
    assert report.total.vcpus == 8
    assert report.total.ram_mb == 8192
    assert report.total.disk_gb == 80


def test_deleted_server_is_ignored() -> None:
    report = _report()
    names = [project.project for project in report.projects]
    assert "demo" in names
    assert report.total.servers == 3


def test_unknown_flavor_fails_with_context() -> None:
    with pytest.raises(ValueError, match="unknown flavor"):
        build_report(
            FIXTURES / "servers-unknown.json",
            FIXTURES / "flavors.json",
            FIXTURES / "limits.json",
            0.85,
        )


def test_missing_limit_fails(tmp_path: Path) -> None:
    limits = json.loads((FIXTURES / "limits.json").read_text(encoding="utf-8"))
    del limits["projects"]["demo"]
    path = tmp_path / "limits.json"
    path.write_text(json.dumps(limits), encoding="utf-8")

    with pytest.raises(ValueError, match="no capacity limit"):
        build_report(
            FIXTURES / "servers.json",
            FIXTURES / "flavors.json",
            path,
            0.85,
        )


def test_duplicate_flavor_fails(tmp_path: Path) -> None:
    flavors = json.loads((FIXTURES / "flavors.json").read_text(encoding="utf-8"))
    flavors.append({"Name": "m1.small", "VCPUs": 8, "RAM": 8192, "Disk": 80})
    path = tmp_path / "flavors.json"
    path.write_text(json.dumps(flavors), encoding="utf-8")

    with pytest.raises(ValueError, match="duplicate flavor"):
        build_report(
            FIXTURES / "servers.json",
            path,
            FIXTURES / "limits.json",
            0.85,
        )


def test_threshold_finding() -> None:
    report = _report()
    finding = next(
        item
        for item in report.findings
        if item.project == "demo" and item.resource == "vcpus"
    )
    assert finding.used == 4
    assert finding.limit == 4
    assert finding.ratio == pytest.approx(1.0)


def test_boundary_is_strictly_greater_than_threshold() -> None:
    report = _report(max_utilization=1.0)
    assert not report.findings


def test_json_status_and_shape() -> None:
    report = _report()
    payload = json.loads(render(report, "json", 0.85))
    assert payload["status"] == "over_threshold"
    assert payload["total"]["servers"] == 3
    assert payload["projects"][0]["project"] == "demo"
    assert payload["findings"]


def test_cli_returns_3_on_finding(capsys: pytest.CaptureFixture[str]) -> None:
    code = main(
        [
            "--servers",
            str(FIXTURES / "servers.json"),
            "--flavors",
            str(FIXTURES / "flavors.json"),
            "--limits",
            str(FIXTURES / "limits.json"),
        ]
    )
    captured = capsys.readouterr()
    assert code == 3
    assert "OVER THRESHOLD" in captured.out
    assert captured.err == ""


def test_cli_rejects_bad_ratio() -> None:
    with pytest.raises(SystemExit) as exc:
        main(
            [
                "--servers",
                "x",
                "--flavors",
                "y",
                "--limits",
                "z",
                "--max-utilization",
                "1.5",
            ]
        )
    assert exc.value.code == 2
```

Run:

```bash
pytest -q
ruff check .
mypy src
```

Expected:

```text
.........
9 passed in 0.18s
All checks passed!
Success: no issues found in 1 source file
```

## Part F — Run the CLI

```bash
capacity-report \
  --servers tests/fixtures/servers.json \
  --flavors tests/fixtures/flavors.json \
  --limits tests/fixtures/limits.json
echo "exit=$?"
```

Expected table:

```text
PROJECT   SERVERS  VCPU  RAM_MB  DISK_GB  VCPU%  RAM%  DISK%
demo      2        4     4096    40       100.0  50.0  40.0
platform  1        4     4096    40       25.0   12.5  20.0
TOTAL     3        8     8192    80       40.0   20.0  26.7

OVER THRESHOLD (max 85.0%):
  demo vcpus 4/4 (100.0%)
exit=3
```

JSON:

```bash
capacity-report \
  --servers tests/fixtures/servers.json \
  --flavors tests/fixtures/flavors.json \
  --limits tests/fixtures/limits.json \
  --format json > /tmp/capacity.json
echo "exit=$?"
jq '{status, total, findings}' /tmp/capacity.json
```

Expected:

```json
{
  "status": "over_threshold",
  "total": {
    "project": "TOTAL",
    "servers": 3,
    "vcpus": 8,
    "ram_mb": 8192,
    "disk_gb": 80,
    "limit": {"vcpus": 20, "ram_mb": 40960, "disk_gb": 300},
    "utilization_pct": {"vcpus": 40.0, "ram_mb": 20.0, "disk_gb": 26.666666666666668}
  },
  "findings": [
    {"project": "demo", "resource": "vcpus", "used": 4, "limit": 4, "ratio": 1.0}
  ]
}
```

Now raise the threshold to exactly 1.0:

```bash
capacity-report \
  --servers tests/fixtures/servers.json \
  --flavors tests/fixtures/flavors.json \
  --limits tests/fixtures/limits.json \
  --max-utilization 1.0 >/dev/null
echo "exit=$?"
```

Expected `0`. The comparison is `ratio > max_utilization`, so exactly at the limit is allowed. Decide whether that is the semantics you want; document it. Some teams use `>=` for "at capacity" alerting. The important thing is that the test encodes the choice.

## Internal explanation

### Boundary validation, not defensive `.get()`

`_int_field` fails with the record context instead of silently defaulting a missing `VCPUs` to zero. A zero would understate usage and hide a capacity problem. `.get("Project") or "unknown"` is used only for the project label; an unknown project then fails the limit check, so it cannot silently disappear.

### Why unknown flavors are fatal

If a server references a flavor that is not in the flavor list, the tool cannot compute its usage. Skipping it would produce a report that looks complete but undercounts. Failing with the server name and flavor lets the operator fix the input (wrong filter, deleted flavor, pagination truncation) before trusting the number.

### Why missing limits are fatal

A project with no limit cannot be evaluated. Reporting it as "0% used" is false. The tool fails and names the project. In a real environment, you might choose to report "unlimited" explicitly; that is a product decision, but it must be visible.

### Why stdout/stderr separation matters

The table or JSON is the product; logs are diagnostics. Piping stdout into `jq` or a dashboard must not require stripping timestamps. `logging` writes to stderr; `print` writes the report to stdout. Tests assert `captured.err == ""` for the success path so accidental logging does not creep into the data stream.

### Why exit code 3

A capacity breach is not a tool failure. If CI treats both as exit 1, it cannot distinguish "fix the pipeline" from "the cluster is full." Code 3 lets the caller route the finding to the capacity owner while still failing the job.

### Why test the boundary

`ratio > 1.0` and `ratio >= 1.0` differ at exactly 100%. Fixtures at 100% make the choice visible. A test that only checks 99% and 101% would miss an off-by-one in the comparison.

### Why no cloud call

The join and aggregation logic is deterministic and testable from files. The cloud API is an input source with pagination, auth, and transient failures; testing it requires a sandbox and belongs in an integration test. Keeping the core pure makes the unit suite fast and reliable. In production, add a separate `--from-openstack` adapter that writes the same JSON shape.

## Troubleshooting task

Work on copies so the passing suite remains intact.

1. **Malformed JSON.** Remove a comma from `flavors.json` and run the CLI:

```bash
cp tests/fixtures/flavors.json /tmp/flavors-bad.json
# edit /tmp/flavors-bad.json to remove a comma
capacity-report --servers tests/fixtures/servers.json \
  --flavors /tmp/flavors-bad.json \
  --limits tests/fixtures/limits.json; echo "exit=$?"
```

Expected: exit 1, a message containing the path and JSON error location, no traceback. If you see a traceback, the exception is not caught or a different exception type escaped. Fix the handler, not the fixture.

2. **Unknown flavor.** Add a server with `"Flavor": "m1.large"` to a copy of `servers.json`. Expected: exit 1, `unknown flavor 'm1.large' for server '<name>'`. This is the correct behavior; do not add a default flavor.

3. **Missing limit.** Remove `demo` from a copy of `limits.json`. Expected: exit 1, `no capacity limit for project 'demo'`. Explain why a warning-and-continue would be dangerous for a capacity report.

4. **Duplicate flavor.** Append a second `m1.small` with different values. Expected: exit 1, `duplicate flavor 'm1.small'`. Decide whether last-write-wins is ever acceptable; justify your answer.

5. **Zero limit with usage.** Set `demo.vcpus` to `0` in a copy of `limits.json` and run with `--format json`. Expected: a finding with `"ratio": null` and table output `n/a`. Confirm the JSON is valid with `jq`. Explain why `Infinity` would be a bad JSON value.

6. **Threshold inversion at the boundary.** Run with `--max-utilization 0.99` and `1.0`. Expected: exit 3 then exit 0. Change the comparison to `>=` and rerun the tests; observe which test fails. Revert. This is how you detect an unintended semantic change.

7. **Type error in a fixture.** Change `"VCPUs": 2` to `"VCPUs": "two"`. Expected: exit 1 with the flavor index and field name. If the message is a bare `ValueError: invalid literal for int()`, improve `_int_field` to include the context.

## Verification

- [ ] `pytest -q` passes all tests.
- [ ] `ruff check .` passes.
- [ ] `mypy src` passes in strict mode.
- [ ] Table totals match the hand calculation.
- [ ] Deleted servers are excluded.
- [ ] Unknown flavor, missing limit, duplicate flavor, and malformed JSON each exit 1 with context.
- [ ] JSON output parses with `jq`; `status` is `over_threshold` or `ok`.
- [ ] `--max-utilization 1.0` exits 0 for the 100% fixture; 0.99 exits 3.
- [ ] Logs go to stderr; stdout is parseable.
- [ ] No secret or token is accepted as an argument or printed.

## Cleanup

```bash
deactivate
rm -f /tmp/capacity.json /tmp/flavors-bad.json
# Optional: remove the lab directory after the write-up.
# rm -rf ~/opslab/phase7/l7.2
```

Keep the project if you plan to use it in L7.4. The pipeline lab expects a Python package with tests and a build step.

## Write-up template

```markdown
# L7.2 — Python capacity report

## Input contract
- servers.json fields:
- flavors.json fields:
- limits.json shape:
- Deleted statuses:
- Unknown/missing data behavior:

## Aggregation
| Project | Servers | vCPU | RAM MB | Disk GB | vCPU % | Finding? |
|---|---|---|---|---|---|---|
| demo | | | | | | |
| platform | | | | | | |
| TOTAL | | | | | | |

## Exit codes
- 0:
- 1:
- 2:
- 3:

## Tests
- Unit tests:
- Boundary test:
- Failure-path tests:
- What is not tested (and why):

## Troubleshooting
- Malformed JSON: symptom ___; evidence ___; fix ___
- Unknown flavor: symptom ___; evidence ___; fix ___
- Missing limit: symptom ___; evidence ___; fix ___
- Zero limit: behavior ___; JSON representation ___
- Threshold boundary: behavior ___; test ___

## Production notes
- Where the JSON comes from:
- Pagination/auth/timeouts (if added):
- How findings are routed:
- What would be logged/redacted:
- What would be an integration test:
```

---

Next: [Labs part 2](06-labs-part2.md) — converge a VM with Ansible and ship the tool through a pipeline.
