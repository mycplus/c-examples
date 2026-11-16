#!/bin/bash
# Runs udp-receiver and three senders on this machine (IPv4, IPv6, and one
# hand-made datagram that skips sequence numbers) and checks what arrived.
# Linux and macOS.  Usage: bash tests/sender_receiver_test.sh BUILD_DIR
bin=${1:?usage: sender_receiver_test.sh BUILD_DIR}
port=$((40000 + $$ % 20000))
tmp=$(mktemp -d)

"$bin/udp-receiver" "$port" 4 3000 > "$tmp/rx.out" 2>&1 &
rx=$!
sleep 0.5
"$bin/udp-sender" 127.0.0.1 "$port" "boiler temp=71.3C" 2 50 > "$tmp/tx4.out"
"$bin/udp-sender" ::1 "$port" "over IPv6" 1 > "$tmp/tx6.out" 2>&1 || echo "(no IPv6 loopback)" >> "$tmp/tx6.out"
printf '5 jumped ahead' > "/dev/udp/127.0.0.1/$port"
wait $rx

echo "--- sender (IPv4)"; cat "$tmp/tx4.out"
echo "--- sender (IPv6)"; cat "$tmp/tx6.out"
echo "--- receiver"; cat "$tmp/rx.out"

status=0
grep -q "from .*127.0.0.1\]*:[0-9]*, 19 bytes: 1 boiler temp=71.3C" "$tmp/rx.out" || { echo "FAIL: first datagram"; status=1; }
expected="received 4 datagram(s), 2 missing"
grep -q "no IPv6 loopback" "$tmp/tx6.out" && expected="received 3 datagram(s), 2 missing"
grep -q "$expected" "$tmp/rx.out" || { echo "FAIL: expected \"$expected\""; status=1; }
rm -rf "$tmp"
exit $status
