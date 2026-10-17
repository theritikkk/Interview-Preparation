#!/bin/bash
# procchaos.sh — real process-level chaos: pause (SIGSTOP/SIGCONT, what
# Chaos Mesh's PodChaos "pod-failure" and most process-chaos tools do
# under the hood is conceptually the same signal-based interruption, just
# applied to a container's PID 1 instead of a bare process) and kill
# (SIGKILL, what Chaos Mesh's PodChaos "pod-kill" action does — literally
# `kill -9` against the target's PID, same mechanism used here).
#
# Usage:
#   procchaos.sh pause <pid> <duration_s>   # SIGSTOP, wait, SIGCONT
#   procchaos.sh kill <pid>                 # SIGKILL
set -euo pipefail

cmd=$1
pid=$2

case "$cmd" in
  pause)
    duration=${3:-5}
    echo "[procchaos] SIGSTOP -> pid $pid (real process pause)"
    kill -STOP "$pid"
    ps -o pid,stat,cmd -p "$pid" || true
    echo "[procchaos] holding for ${duration}s..."
    sleep "$duration"
    echo "[procchaos] SIGCONT -> pid $pid (recovery)"
    kill -CONT "$pid"
    ps -o pid,stat,cmd -p "$pid" || true
    ;;
  kill)
    echo "[procchaos] SIGKILL -> pid $pid (real process kill)"
    kill -KILL "$pid"
    sleep 0.3
    if kill -0 "$pid" 2>/dev/null; then
      echo "[procchaos] pid $pid still alive?!"
      exit 1
    else
      echo "[procchaos] pid $pid is gone. Restart it yourself to demonstrate recovery."
    fi
    ;;
  *)
    echo "usage: procchaos.sh pause <pid> <duration_s> | kill <pid>" >&2
    exit 2
    ;;
esac
