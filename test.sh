# IPK-RDT Automated Test Script

```bash
#!/usr/bin/env bash
# This test script was created with AI assistance (ChatGPT)
# and subsequently modified by the author.

set -u
set -o pipefail

BIN="./ipk-rdt"
PORT_BASE=19000
TMP_DIR="./test_tmp"
TIMEOUT=5

PASSED=0
FAILED=0
TEST_ID=0

mkdir -p "$TMP_DIR"

cleanup() {
    pkill -f "./ipk-rdt" 2>/dev/null || true
    rm -rf "$TMP_DIR"
}

trap cleanup EXIT

print_result() {
    local name="$1"
    local result="$2"

    if [ "$result" -eq 0 ]; then
        echo "[PASS] $name"
        PASSED=$((PASSED + 1))
    else
        echo "[FAIL] $name"
        FAILED=$((FAILED + 1))
    fi
}

next_port() {
    TEST_ID=$((TEST_ID + 1))
    echo $((PORT_BASE + TEST_ID))
}

wait_server() {
    sleep 0.5
}

compare_files() {
    cmp "$1" "$2" >/dev/null 2>&1
}

run_transfer_test() {
    local name="$1"
    local input="$2"
    local mode="$3"

    local port
    port=$(next_port)

    local output="$TMP_DIR/output_${TEST_ID}.bin"
    local server_log="$TMP_DIR/server_${TEST_ID}.log"
    local client_log="$TMP_DIR/client_${TEST_ID}.log"

    if [ "$mode" = "file_to_file" ]; then
        $BIN -s -p "$port" -o "$output" \
            > /dev/null 2> "$server_log" &
        SERVER_PID=$!

        wait_server

        $BIN -c -a 127.0.0.1 -p "$port" -i "$input" \
            > /dev/null 2> "$client_log"
        CLIENT_EXIT=$?

    elif [ "$mode" = "stdin_to_file" ]; then
        $BIN -s -p "$port" -o "$output" \
            > /dev/null 2> "$server_log" &
        SERVER_PID=$!

        wait_server

        cat "$input" | \
            $BIN -c -a 127.0.0.1 -p "$port" \
            > /dev/null 2> "$client_log"
        CLIENT_EXIT=$?

    elif [ "$mode" = "file_to_stdout" ]; then
        local stdout_capture="$TMP_DIR/stdout_${TEST_ID}.bin"

        $BIN -s -p "$port" \
            > "$stdout_capture" 2> "$server_log" &
        SERVER_PID=$!

        wait_server

        $BIN -c -a 127.0.0.1 -p "$port" -i "$input" \
            > /dev/null 2> "$client_log"
        CLIENT_EXIT=$?

        output="$stdout_capture"

    elif [ "$mode" = "stdin_to_stdout" ]; then
        local stdout_capture="$TMP_DIR/stdout_${TEST_ID}.bin"

        $BIN -s -p "$port" \
            > "$stdout_capture" 2> "$server_log" &
        SERVER_PID=$!

        wait_server

        cat "$input" | \
            $BIN -c -a 127.0.0.1 -p "$port" \
            > /dev/null 2> "$client_log"
        CLIENT_EXIT=$?

        output="$stdout_capture"
    fi

    wait "$SERVER_PID"
    SERVER_EXIT=$?

    if [ "$CLIENT_EXIT" -ne 0 ]; then
        print_result "$name (client exit code)" 1
        return
    fi

    if [ "$SERVER_EXIT" -ne 0 ]; then
        print_result "$name (server exit code)" 1
        return
    fi

    compare_files "$input" "$output"
    print_result "$name" $?
}

run_timeout_test() {
    local name="$1"

    local port
    port=$(next_port)

    timeout 10 \
        $BIN -c -a 127.0.0.1 -p "$port" -i /dev/null -w 2 \
        > /dev/null 2> /dev/null

    local result=$?

    if [ "$result" -eq 0 ]; then
        print_result "$name" 1
    else
        print_result "$name" 0
    fi
}

run_signal_test() {
    local name="$1"

    local port
    port=$(next_port)

    local input="$TMP_DIR/signal_input.bin"
    local output="$TMP_DIR/signal_output.bin"

    dd if=/dev/urandom of="$input" bs=1M count=10 \
        > /dev/null 2>&1

    $BIN -s -p "$port" -o "$output" \
        > /dev/null 2> /dev/null &
    SERVER_PID=$!

    wait_server

    $BIN -c -a 127.0.0.1 -p "$port" -i "$input" \
        > /dev/null 2> /dev/null &
    CLIENT_PID=$!

    sleep 1

    kill -INT "$CLIENT_PID"

    wait "$CLIENT_PID"
    CLIENT_EXIT=$?

    wait "$SERVER_PID"
    SERVER_EXIT=$?

    if [ "$CLIENT_EXIT" -eq 0 ]; then
        print_result "$name (client should fail)" 1
        return
    fi

    if [ "$SERVER_EXIT" -eq 0 ]; then
        print_result "$name (server should fail)" 1
        return
    fi

    print_result "$name" 0
}

run_netem_test() {
    local name="$1"

    if ! command -v tc >/dev/null 2>&1; then
        echo "Skipping netem test (tc not installed)"
        return
    fi

    local port
    port=$(next_port)

    local input="$TMP_DIR/netem_input.bin"
    local output="$TMP_DIR/netem_output.bin"

    dd if=/dev/urandom of="$input" bs=1M count=5 \
        > /dev/null 2>&1

    sudo tc qdisc add dev lo root netem \
        loss 10% delay 50ms reorder 25% duplicate 5%

    $BIN -s -p "$port" -o "$output" \
        > /dev/null 2> /dev/null &
    SERVER_PID=$!

    wait_server

    $BIN -c -a 127.0.0.1 -p "$port" -i "$input" \
        > /dev/null 2> /dev/null
    CLIENT_EXIT=$?

    wait "$SERVER_PID"
    SERVER_EXIT=$?

    sudo tc qdisc del dev lo root netem

    if [ "$CLIENT_EXIT" -ne 0 ]; then
        print_result "$name (client exit)" 1
        return
    fi

    if [ "$SERVER_EXIT" -ne 0 ]; then
        print_result "$name (server exit)" 1
        return
    fi

    compare_files "$input" "$output"
    print_result "$name" $?
}

################################################################################
# Build
################################################################################

make
if [ $? -ne 0 ]; then
    echo "Build failed"
    exit 1
fi

################################################################################
# Test data generation
################################################################################

printf "Hello IPK\n" > "$TMP_DIR/small.txt"
truncate -s 0 "$TMP_DIR/empty.bin"

dd if=/dev/urandom of="$TMP_DIR/random_1m.bin" bs=1M count=1 \
    > /dev/null 2>&1

dd if=/dev/urandom of="$TMP_DIR/random_10m.bin" bs=1M count=10 \
    > /dev/null 2>&1

################################################################################
# Basic functionality tests
################################################################################

run_transfer_test \
    "Small text file transfer" \
    "$TMP_DIR/small.txt" \
    "file_to_file"

run_transfer_test \
    "Empty file transfer" \
    "$TMP_DIR/empty.bin" \
    "file_to_file"

run_transfer_test \
    "1MB binary transfer" \
    "$TMP_DIR/random_1m.bin" \
    "file_to_file"

run_transfer_test \
    "10MB binary transfer" \
    "$TMP_DIR/random_10m.bin" \
    "file_to_file"

################################################################################
# STDIN / STDOUT tests
################################################################################

run_transfer_test \
    "stdin -> file" \
    "$TMP_DIR/random_1m.bin" \
    "stdin_to_file"

run_transfer_test \
    "file -> stdout" \
    "$TMP_DIR/random_1m.bin" \
    "file_to_stdout"

run_transfer_test \
    "stdin -> stdout" \
    "$TMP_DIR/random_1m.bin" \
    "stdin_to_stdout"

################################################################################
# Timeout handling
################################################################################

run_timeout_test "Client timeout without server"

################################################################################
# Signal handling
################################################################################

run_signal_test "SIGINT handling"

################################################################################
# Network impairment tests
################################################################################

run_netem_test "Transfer under packet loss/reordering"

################################################################################
# Summary
################################################################################

echo
echo "=============================="
echo "Passed: $PASSED"
echo "Failed: $FAILED"
echo "=============================="

if [ "$FAILED" -ne 0 ]; then
    exit 1
fi

exit 0

