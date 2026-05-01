#!/bin/bash
# Tato testovaci cast byla vytvorena s pomoci AI (Claude)
# a nasledne upravena autorem.
#
# Test script for ipk-rdt
# Usage: ./test.sh [path to ipk-rdt binary]
 
BINARY=${1:-./ipk-rdt}
PORT=19999
PASS=0
FAIL=0
TIMEOUT=10
 
 
pass() { echo -e "PASS $1"; PASS=$((PASS+1)); }
fail() { echo -e "FAIL $1"; FAIL=$((FAIL+1)); }
 
# Run server in background, client, compare output
run_test() {
    local name=$1
    local input=$2
    local server_args=$3
    local client_args=$4
 
    local out=$(mktemp)
    PORT=$((PORT+1))
 
    $BINARY -s -p $PORT -o $out -w $TIMEOUT $server_args &
    local srv_pid=$!
    sleep 0.1
 
    echo "$input" | $BINARY -c -a 127.0.0.1 -p $PORT -w $TIMEOUT $client_args
    local cli_exit=$?
 
    wait $srv_pid
    local srv_exit=$?
 
    if [ $cli_exit -ne 0 ] || [ $srv_exit -ne 0 ]; then
        fail "$name (exit: client=$cli_exit server=$srv_exit)"
        rm -f $out
        return
    fi
 
    if [ "$(echo "$input")" = "$(cat $out)" ]; then
        pass "$name"
    else
        fail "$name (data mismatch)"
    fi
    rm -f $out
}
 
# Run file transfer test
run_file_test() {
    local name=$1
    local input_file=$2
    local server_extra=$3
    local client_extra=$4
 
    local out=$(mktemp)
    PORT=$((PORT+1))
 
    $BINARY -s -p $PORT -o $out -w $TIMEOUT $server_extra &
    local srv_pid=$!
    sleep 0.1
 
    $BINARY -c -a 127.0.0.1 -p $PORT -i $input_file -w $TIMEOUT $client_extra
    local cli_exit=$?
 
    wait $srv_pid
    local srv_exit=$?
 
    if [ $cli_exit -ne 0 ] || [ $srv_exit -ne 0 ]; then
        fail "$name (exit: client=$cli_exit server=$srv_exit)"
        rm -f $out
        return
    fi
 
    if diff -q $input_file $out > /dev/null 2>&1; then
        pass "$name"
    else
        fail "$name (data mismatch)"
    fi
    rm -f $out
}
 
echo "========================================="
echo " ipk-rdt test suite"
echo " Binary: $BINARY"
echo "========================================="
 
# Check binary exists
if [ ! -x "$BINARY" ]; then
    echo "Binary not found or not executable: $BINARY"
    exit 1
fi
 
echo ""
echo "--- Basic transfer tests ---"
 
# Empty input
run_test "empty input" "" "" ""
 
# Small text
run_test "small text" "hello world" "" ""
 
# Multiline text
run_test "multiline text" "$(printf 'line1\nline2\nline3')" "" ""
 
echo ""
echo "--- File transfer tests ---"
 
# Small binary file
tmp_in=$(mktemp)
dd if=/dev/urandom of=$tmp_in bs=1024 count=10 2>/dev/null
run_file_test "small binary file (10KB)" $tmp_in
 
# Medium file
dd if=/dev/urandom of=$tmp_in bs=1024 count=100 2>/dev/null
run_file_test "medium binary file (100KB)" $tmp_in
 
# Large file
dd if=/dev/urandom of=$tmp_in bs=1024 count=1000 2>/dev/null
run_file_test "large binary file (1MB)" $tmp_in
 
# All byte values
printf '%b' "$(printf '\\%o' $(seq 0 255))" > $tmp_in
run_file_test "all byte values (binary)" $tmp_in
 
# Empty file
> $tmp_in
run_file_test "empty file" $tmp_in
 
rm -f $tmp_in
 
echo ""
echo "--- IPv6 tests ---"
 
# IPv6 loopback
tmp_in=$(mktemp)
echo "ipv6 test data" > $tmp_in
out=$(mktemp)
PORT=$((PORT+1))
 
$BINARY -s -p $PORT -o $out -w $TIMEOUT &
srv_pid=$!
sleep 0.1
$BINARY -c -a ::1 -p $PORT -i $tmp_in -w $TIMEOUT
cli_exit=$?
wait $srv_pid
srv_exit=$?
 
if [ $cli_exit -eq 0 ] && [ $srv_exit -eq 0 ] && diff -q $tmp_in $out > /dev/null 2>&1; then
    pass "IPv6 loopback transfer"
else
    fail "IPv6 loopback transfer (exit: client=$cli_exit server=$srv_exit)"
fi
rm -f $tmp_in $out
 
echo ""
echo "--- Argument validation tests ---"
 
# No arguments
$BINARY > /dev/null 2>&1
[ $? -ne 0 ] && pass "no arguments → non-zero exit" || fail "no arguments → non-zero exit"
 
# Both -c and -s
$BINARY -c -s -p 9000 -a 127.0.0.1 > /dev/null 2>&1
[ $? -ne 0 ] && pass "both -c and -s → non-zero exit" || fail "both -c and -s → non-zero exit"
 
# Client without -a
$BINARY -c -p 9000 > /dev/null 2>&1
[ $? -ne 0 ] && pass "client without -a → non-zero exit" || fail "client without -a → non-zero exit"
 
# Server without -p
$BINARY -s > /dev/null 2>&1
[ $? -ne 0 ] && pass "server without -p → non-zero exit" || fail "server without -p → non-zero exit"
 
# Help flag
$BINARY -h > /dev/null 2>&1
[ $? -eq 0 ] && pass "-h → exit code 0" || fail "-h → exit code 0"
 
echo ""
echo "--- Signal handling tests ---"
 
# SIGTERM during idle server
PORT=$((PORT+1))
$BINARY -s -p $PORT -w 30 > /dev/null 2>&1 &
srv_pid=$!
sleep 0.2
kill -SIGTERM $srv_pid
wait $srv_pid
[ $? -eq 0 ] && pass "SIGTERM on idle server → clean exit" || fail "SIGTERM on idle server → clean exit"
 
# SIGINT during idle server
PORT=$((PORT+1))
$BINARY -s -p $PORT -w 30 > /dev/null 2>&1 &
srv_pid=$!
sleep 0.2
kill -SIGINT $srv_pid
wait $srv_pid
[ $? -eq 0 ] && pass "SIGINT on idle server → clean exit" || fail "SIGINT on idle server → clean exit"
 
echo ""
echo "========================================="
echo " Results: $PASS passed, $FAIL failed"
echo "========================================="
 
[ $FAIL -eq 0 ] && exit 0 || exit 1
 