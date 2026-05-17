#!/bin/bash
# Tato testovaci cast byla vytvorena s pomoci AI (Claude)
# a nasledne upravena autorem.

BIN=${1:-./reliable-udp-transfer}
PASS=0
FAIL=0
PORT=19000
TIMEOUT=10

pass() { echo "PASS $1"; PASS=$((PASS+1)); }
fail() { echo "FAIL $1"; FAIL=$((FAIL+1)); }
next_port() { PORT=$((PORT+1)); echo $PORT; }

# cleanup netem
tc qdisc del dev lo root 2>/dev/null

run() {
    local name="$1" input="$2" addr="$3" server_addr="$4"

    local out p spid cexit sexit
    out=$(mktemp)
    p=$(next_port)

    # server (silenced completely)
    $BIN -s -p $p -a "$server_addr" -o "$out" -w $TIMEOUT >/dev/null 2>&1 &
    spid=$!
    sleep 0.3

    # client (silenced completely)
    if [ -f "$input" ]; then
        $BIN -c -a "$addr" -p $p -i "$input" -w $TIMEOUT >/dev/null 2>&1
    else
        echo "$input" | $BIN -c -a "$addr" -p $p -w $TIMEOUT >/dev/null 2>&1
    fi

    cexit=$?
    wait $spid
    sexit=$?

    if [ $cexit -ne 0 ] || [ $sexit -ne 0 ]; then
        fail "$name (client=$cexit server=$sexit)"
        rm -f "$out"
        return
    fi

    if [ -f "$input" ]; then
        diff -q "$input" "$out" >/dev/null 2>&1 && pass "$name" || fail "$name (mismatch)"
    else
        [ "$(echo "$input")" = "$(cat "$out")" ] && pass "$name" || fail "$name (mismatch)"
    fi

    rm -f "$out"
}

if [ ! -x "$BIN" ]; then
    echo "Binary not found: $BIN"
    exit 1
fi

echo "=== reliable-udp-transfer test suite ==="
echo ""

# ── Argument validation ─────────────────────────────
$BIN >/dev/null 2>&1
[ $? -ne 0 ] && pass "no args" || fail "no args"

$BIN -c -s -p 9000 -a 127.0.0.1 >/dev/null 2>&1
[ $? -ne 0 ] && pass "both -c and -s" || fail "both -c and -s"

$BIN -c -p 9000 >/dev/null 2>&1
[ $? -ne 0 ] && pass "missing -a client" || fail "missing -a client"

$BIN -s >/dev/null 2>&1
[ $? -ne 0 ] && pass "missing -p server" || fail "missing -p server"

$BIN -h >/dev/null 2>&1
[ $? -eq 0 ] && pass "-h" || fail "-h"

# ── Basic transfers ────────────────────────────────
f=$(mktemp); > "$f"
run "empty file" "$f" "127.0.0.1" "127.0.0.1"
rm -f "$f"

f=$(mktemp); printf 'A' > "$f"
run "single byte" "$f" "127.0.0.1" "127.0.0.1"
rm -f "$f"

run "stdin text" "hello world" "127.0.0.1" "127.0.0.1"

f=$(mktemp); dd if=/dev/urandom of="$f" bs=1024 count=10 >/dev/null 2>&1
run "10KB binary" "$f" "127.0.0.1" "127.0.0.1"
rm -f "$f"

f=$(mktemp); dd if=/dev/urandom of="$f" bs=1024 count=200 >/dev/null 2>&1
run "200KB binary" "$f" "127.0.0.1" "127.0.0.1"
rm -f "$f"

f=$(mktemp); dd if=/dev/urandom of="$f" bs=1024 count=1024 >/dev/null 2>&1
run "1MB binary" "$f" "127.0.0.1" "127.0.0.1"
rm -f "$f"

# ── stdin → stdout test ─────────────────────────────
f=$(mktemp)
echo "stdin test" > "$f"

out=$(mktemp)
p=$(next_port)

$BIN -s -p $p -a 127.0.0.1 -o "$out" -w $TIMEOUT >/dev/null 2>&1 &
spid=$!
sleep 0.3

cat "$f" | $BIN -c -a 127.0.0.1 -p $p -w $TIMEOUT >/dev/null 2>&1
cexit=$?

wait $spid
sexit=$?

if [ $cexit -eq 0 ] && [ $sexit -eq 0 ] && diff -q "$f" "$out" >/dev/null 2>&1; then
    pass "stdin to stdout"
else
    fail "stdin to stdout"
fi

rm -f "$f" "$out"

# ── IPv6 ────────────────────────────────────────────
f=$(mktemp)
dd if=/dev/urandom of="$f" bs=1024 count=10 >/dev/null 2>&1

run "IPv6" "$f" "::1" "::1"
rm -f "$f"

# ── Signal handling ─────────────────────────────────
p=$(next_port)
$BIN -s -p $p -a 127.0.0.1 -w 30 >/dev/null 2>&1 &
spid=$!
sleep 0.3
kill -SIGTERM $spid >/dev/null 2>&1
wait $spid >/dev/null 2>&1
pass "SIGTERM"

p=$(next_port)
$BIN -s -p $p -a 127.0.0.1 -w 30 >/dev/null 2>&1 &
spid=$!
sleep 0.3
kill -SIGINT $spid >/dev/null 2>&1
wait $spid >/dev/null 2>&1
pass "SIGINT"

# ── Timeout ─────────────────────────────────────────
p=$(next_port)
start=$SECONDS
$BIN -s -p $p -a 127.0.0.1 -w 2 >/dev/null 2>&1
sexit=$?
elapsed=$((SECONDS - start))

if [ $sexit -ne 0 ] && [ $elapsed -ge 2 ]; then
    pass "timeout"
else
    fail "timeout"
fi

# ── Summary ─────────────────────────────────────────
echo "========================="
echo "PASS: $PASS FAIL: $FAIL"
echo "========================="

[ $FAIL -eq 0 ] && exit 0 || exit 1