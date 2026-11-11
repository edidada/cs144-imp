#!/usr/bin/env bash
set -euo pipefail
# Run only inside the disposable, privileged GitHub Actions job container.
build_dir=${1:?build directory required}
output_dir=${2:?evidence directory required}
mkdir -p "$output_dir/server"
build_dir=$(realpath "$build_dir")
output_dir=$(realpath "$output_dir")

ip tuntap add dev tun144 mode tun
ip addr add 169.254.144.1/24 dev tun144
ip link set tun144 up
ip route change 169.254.144.0/24 dev tun144 rto_min 10ms
ip -details addr show tun144 > "$output_dir/tun.txt"

# This server uses Linux TCP. Only the test client uses the student TCP stack.
head -c 262144 /dev/urandom > "$output_dir/server/payload.bin"
python3 -m http.server 80 --bind 169.254.144.1 --directory "$output_dir/server" > "$output_dir/server.log" 2>&1 &
server_pid=$!
trap 'kill "$server_pid" 2>/dev/null || true' EXIT
curl --noproxy '*' --fail --retry 10 --retry-connrefused --retry-delay 1 \
  http://169.254.144.1/payload.bin -o "$output_dir/kernel-baseline.bin"
cmp "$output_dir/server/payload.bin" "$output_dir/kernel-baseline.bin"
timeout 90 "$build_dir/apps/webget" 169.254.144.1 /payload.bin \
  > "$output_dir/student-response.bin" 2> "$output_dir/student-tcp.log"
grep -a -q '^HTTP/1.0 200' "$output_dir/student-response.bin"
grep -q 'imp successfully connected' "$output_dir/student-tcp.log"
tail -c 262144 "$output_dir/student-response.bin" > "$output_dir/student-body.bin"
cmp "$output_dir/server/payload.bin" "$output_dir/student-body.bin"
sha256sum "$output_dir/server/payload.bin" "$output_dir/student-body.bin" > "$output_dir/hashes.txt"
printf 'PASS: student TCP fetched 262144 exact binary bytes from Linux TCP\n' > "$output_dir/result.txt"

# Apply the course's source-scoped NAT for the real public HTTP test.
iptables -t nat -A POSTROUTING -s 169.254.144.0/24 -j MASQUERADE
echo 1 > /proc/sys/net/ipv4/ip_forward
timeout 90 bash tests/webget_t.sh "$build_dir" > "$output_dir/internet.log" 2>&1
printf 'PASS: original Internet webget hash test using student TCP\n' >> "$output_dir/result.txt"
