#!/usr/bin/env bash
# Exercise the official tiny Internet with real UDP sockets and student TCP/IP/Ethernet.
set -euo pipefail
build_dir=$(realpath "${1:?build directory required}")
output_dir=${2:?evidence directory required}
suffix=${3:-}
mkdir -p "$output_dir"
output_dir=$(realpath "$output_dir")
router_pid= server_pid= capture_pid=
cleanup() {
  for pid in "$server_pid" "$router_pid" "$capture_pid"; do
    if [[ -n "$pid" ]]; then kill "$pid" 2>/dev/null || true; fi
  done
}
trap cleanup EXIT
head -c 1048576 /dev/urandom > "$output_dir/ucla-source.bin"
head -c 1048576 /dev/urandom > "$output_dir/stanford-source.bin"
tcpdump -U -ni lo -w "$output_dir/ethernet-over-udp.pcap" 'udp port 2080' > "$output_dir/capture.log" 2>&1 &
capture_pid=$!
"$build_dir/apps/fun_router$suffix" \
  interface:stanford:50.0.0.1:9050:127.0.0.1:2050 \
  interface:ucla:80.0.0.1:9080:127.0.0.1:2080 \
  route:50.0.0.0:8:stanford route:80.0.0.0:8:ucla \
  > "$output_dir/router.log" 2>&1 &
router_pid=$!
timeout 120 "$build_dir/apps/tcp_eth_udp$suffix" server 2050 50.0.0.1 50.9.8.7:80 \
  < "$output_dir/stanford-source.bin" > "$output_dir/stanford-received.bin" 2> "$output_dir/stanford.log" &
server_pid=$!
sleep 1
kill -0 "$router_pid"
kill -0 "$server_pid"
timeout 120 "$build_dir/apps/tcp_eth_udp$suffix" client 2080 127.0.0.1:9080 80.0.0.1 80.6.5.4 50.9.8.7:80 \
  < "$output_dir/ucla-source.bin" > "$output_dir/ucla-received.bin" 2> "$output_dir/ucla.log"
wait "$server_pid"
server_pid=
cmp "$output_dir/ucla-source.bin" "$output_dir/stanford-received.bin"
cmp "$output_dir/stanford-source.bin" "$output_dir/ucla-received.bin"
sha256sum "$output_dir/"*.bin > "$output_dir/hashes.txt"
kill -INT "$capture_pid"
wait "$capture_pid" || true
capture_pid=
tshark -r "$output_dir/ethernet-over-udp.pcap" -d udp.port==2080,eth \
  -Y arp -T fields -e arp.opcode > "$output_dir/arp-opcodes.txt"
grep -qx 1 "$output_dir/arp-opcodes.txt"
grep -qx 2 "$output_dir/arp-opcodes.txt"
tshark -r "$output_dir/ethernet-over-udp.pcap" -d udp.port==2080,eth \
  -Y 'tcp.flags.syn==1' -T fields -e tcp.flags > "$output_dir/tcp-handshake.txt"
test -s "$output_dir/tcp-handshake.txt"
printf 'PASS: two distinct 1-MiB binary files exchanged exactly through student router/TCP/ARP\nPASS: captured ARP request/reply and TCP SYN\n' > "$output_dir/result.txt"
