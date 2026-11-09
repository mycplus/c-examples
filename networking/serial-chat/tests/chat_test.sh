#!/bin/sh
# Connects two chat processes through a socat pseudo-terminal pair (a "virtual
# null-modem cable") and checks that a line typed on each side reaches the other.
# Linux and macOS; needs socat.  Usage: sh tests/chat_test.sh path/to/chat
chat=${1:?usage: chat_test.sh path/to/chat}
dir=$(mktemp -d)

socat pty,raw,echo=0,link="$dir/ttyA" pty,raw,echo=0,link="$dir/ttyB" 2>/dev/null &
socat_pid=$!
tries=0
while [ ! -e "$dir/ttyA" ] || [ ! -e "$dir/ttyB" ]; do
    tries=$((tries + 1))
    [ $tries -gt 50 ] && { echo "socat did not create the ptys"; exit 1; }
    sleep 0.1
done

(sleep 1; echo "hello B, this is A"; sleep 3) | "$chat" "$dir/ttyA" 115200 > "$dir/a.out" 2>&1 &
a_pid=$!
(sleep 2; echo "hi A, got it"; sleep 2) | "$chat" "$dir/ttyB" 115200 > "$dir/b.out" 2>&1 &
b_pid=$!
wait $a_pid $b_pid
kill $socat_pid 2>/dev/null

for side in a b; do
    echo "--- terminal $(echo $side | tr ab AB)"
    sed "s|$dir/|/tmp/|" "$dir/$side.out"
done

status=0
grep -q "peer> hi A, got it" "$dir/a.out" || { echo "FAIL: A did not receive B's line"; status=1; }
grep -q "peer> hello B, this is A" "$dir/b.out" || { echo "FAIL: B did not receive A's line"; status=1; }
rm -rf "$dir"
exit $status
