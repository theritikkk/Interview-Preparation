# Shared helpers for the demo scripts (sourced, not run).
T=./build/txnctl
R=ring.conf
FAILS=0
check() { # check "description" "got" "want"
    if [ "$2" == "$3" ]; then printf '  ok    %s  (%s)\n' "$1" "$2"
    else printf '  FAIL  %s  got [%s] want [%s]\n' "$1" "$2" "$3"; FAILS=$((FAILS + 1)); fi
}
check_contains() { # check_contains "description" "haystack" "needle"
    if [[ "$2" == *"$3"* ]]; then printf '  ok    %s\n' "$1"
    else printf '  FAIL  %s  [%s] does not contain [%s]\n' "$1" "$2" "$3"; FAILS=$((FAILS + 1)); fi
}
val() { $T $R GET "$1" | sed -n 's/^OK value="\(.*\)" (shard.*/\1/p'; }
pending_count() { $T $R PENDING | tail -1 | grep -oE '^[0-9]+'; }
shard_of() { $T $R WHERE "$1" | grep -oE '[0-9]+$'; }
start_cluster() { # [ENV=... in the caller's environment is inherited by the nodes]
    ./scripts/stop_shards.sh 1 2 3 >/dev/null 2>&1
    ./scripts/run_shards.sh 1 2 3 || { echo "cluster failed to start" >&2; exit 1; }
}
source "$(dirname "${BASH_SOURCE[0]}")/node_ctl.sh"
