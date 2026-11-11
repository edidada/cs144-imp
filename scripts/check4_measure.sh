#!/usr/bin/env bash
set -euo pipefail
output=${1:?output directory required}
mkdir -p "$output"
date -Is > "$output/started.txt"
uname -a > "$output/environment.txt"
selected=''
# These are the public distant-host examples in the official Fall 2026 handout.
# Probe first: an unreachable host or RTT below 100ms cannot satisfy this path.
for host in www.cs.ox.ac.uk 162.105.253.58 www.canterbury.ac.nz 41.186.255.86; do
  ping -n -c 5 -i 0.2 -W 2 "$host" > "$output/probe-$host.txt" 2>&1 || true
  average=$(awk -F '/' '/^rtt / {print $5}' "$output/probe-$host.txt")
  if awk -v value="$average" 'BEGIN {exit !(value != "" && value > 100)}'; then
    selected=$host
    break
  fi
done
if [ -z "$selected" ]; then
  printf 'BLOCKED: no reachable handout host with measured RTT >100ms from this runner\n' > "$output/status.txt"
  exit 1
fi
printf '%s\n' "$selected" > "$output/target.txt"
timeout 60 traceroute -n -m 25 -w 1 "$selected" > "$output/traceroute.txt" 2>&1 || true
date -Is > "$output/sampling-start.txt"
start_seconds=$SECONDS
# 0.2 second intervals for a full hour; preserve missing replies and timestamps.
ping -n -D -O -i 0.2 -w 3600 -W 2 "$selected" > "$output/ping-hour.txt" 2>&1
elapsed=$((SECONDS - start_seconds))
printf '%s\n' "$elapsed" > "$output/sampling-duration-seconds.txt"
test "$elapsed" -ge 3600
# Each bounded higher-rate run lasts about one second, not a sustained flood.
for size in 56 512 1400; do
  ping -n -D -i 0.02 -c 50 -W 2 -s "$size" "$selected" > "$output/short-load-$size.txt" 2>&1 || true
done
date -Is > "$output/finished.txt"
printf 'COLLECTED: one distant path; analysis and private classmate path remain incomplete\n' > "$output/status.txt"
