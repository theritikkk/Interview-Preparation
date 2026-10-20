# Per-node control for the durability tests (source it from the project root:
#   source scripts/node_ctl.sh ). Unlike run_shards.sh these act on ONE node.
BIN=${BIN:-./build/raft_node}
PROBE=${PROBE:-./build/direct_probe}
DATA=${RAFT_DATA_DIR:-data}

node_port() { awk -v id="$2" '$1==id {print $3}' "shards/shard$1.conf"; }
node_ids()  { awk '!/^#/ && NF {print $1}' "shards/shard$1.conf"; }
node_pid()  { cat "logs/shard$1-node$2.pid" 2>/dev/null; }
# pid_running PID — alive AND not a zombie. `kill -0` alone is wrong here: a killed node whose
# parent has exited is re-parented to PID 1, which in this sandbox does not reap promptly, so the
# dead process lingers as a zombie and kill -0 keeps succeeding on it.
pid_running(){ local st; st=$(ps -o stat= -p "$1" 2>/dev/null); [ -n "$st" ] && [[ "$st" != Z* ]]; }
node_alive(){ local p; p=$(node_pid "$1" "$2"); [ -n "$p" ] && pid_running "$p"; }

# start_node S ID  — (re)start one node on its existing data; appends to its log.
# Environment variables set by the caller (RAFT_*) reach the node.
start_node() {
    mkdir -p logs
    rm -f "logs/shard$1-node$2.pid"
    setsid nohup bash -c 'echo $$ > "$0"; exec "$@"' "logs/shard$1-node$2.pid" \
        "$BIN" "shards/shard$1.conf" "$2" >> "logs/shard$1-node$2.log" 2>&1 < /dev/null &
    disown 2>/dev/null
    local i; for i in $(seq 1 50); do [ -s "logs/shard$1-node$2.pid" ] && break; sleep 0.1; done
}

# kill_node S ID [signal]  — default SIGKILL; waits until the process is gone.
kill_node() {
    local p; p=$(node_pid "$1" "$2"); [ -n "$p" ] || return 0
    kill -"${3:-KILL}" "$p" 2>/dev/null
    local i; for i in $(seq 1 100); do pid_running "$p" || break; sleep 0.1; done
}
kill_shard() { local id; for id in $(node_ids "$1"); do kill -KILL "$(node_pid "$1" "$id")" 2>/dev/null; done; sleep 0.3; }
start_shard() { local id; for id in $(node_ids "$1"); do start_node "$1" "$id"; done; }

# node_info S ID — the node's own INFO line (empty if unreachable)
node_info() { $PROBE 127.0.0.1 "$(node_port "$1" "$2")" INFO 2>/dev/null | sed -n 's/.*value="\(.*\)"$/\1/p'; }
# info_field S ID key
info_field() { node_info "$1" "$2" | tr ' ' '\n' | sed -n "s/^$3=//p"; }

# leader_node S — id of the node that says it is leader (highest term), or empty
leader_node() {
    local id best="" bt=-1 line t
    for id in $(node_ids "$1"); do
        line=$(node_info "$1" "$id"); [ -n "$line" ] || continue
        [[ "$line" == *"role=leader"* ]] || continue
        t=$(echo "$line" | tr ' ' '\n' | sed -n 's/^term=//p')
        if [ "$t" -gt "$bt" ]; then bt=$t; best=$id; fi
    done
    echo "$best"
}
# wait_leader S [seconds] — returns 0 and echoes the leader once one answers INFO as leader
wait_leader() {
    local i l; for i in $(seq 1 $(( ${2:-30} * 2 ))); do l=$(leader_node "$1"); [ -n "$l" ] && { echo "$l"; return 0; }; sleep 0.5; done; return 1
}
# line_field "<INFO line>" key
line_field() { echo "$1" | tr ' ' '\n' | sed -n "s/^$2=//p"; }
# wait_converged S [seconds] — until a leader exists and every node of the shard reports
# applied == lastIndex (it has applied its whole log) with the same applied index and the
# same state digest. Echoes "<applied> <digest>". (Agreeing is not enough on its own: right
# after a whole-shard restart every node can agree on the snapshot's state before the new
# leader has committed anything, which is not "caught up".)
wait_converged() {
    local i id line a li d first_a first_d ok leaders
    for i in $(seq 1 $(( ${2:-30} * 2 ))); do
        ok=1; first_a=""; first_d=""; leaders=0
        for id in $(node_ids "$1"); do
            line=$(node_info "$1" "$id"); [ -n "$line" ] || { ok=0; break; }
            a=$(line_field "$line" applied); li=$(line_field "$line" lastIndex); d=$(line_field "$line" digest)
            [ "$a" = "$li" ] || { ok=0; break; }
            [[ "$line" == *"role=leader"* ]] && leaders=$((leaders + 1))
            if [ -z "$first_a" ]; then first_a=$a; first_d=$d; elif [ "$a" != "$first_a" ] || [ "$d" != "$first_d" ]; then ok=0; break; fi
        done
        [ $ok = 1 ] && [ $leaders -ge 1 ] && { echo "$first_a $first_d"; return 0; }
        sleep 0.5
    done; return 1
}
