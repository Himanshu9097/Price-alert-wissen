#!/usr/bin/env bash
set -euo pipefail

PASS=0
FAIL=0

assert_exit_code() {
    local desc="$1"
    local expected="$2"
    shift 2
    set +e
    "$@" > /dev/null 2>&1
    local actual=$?
    set -e
    if [[ $actual -eq $expected ]]; then
        echo "  [PASS] CLI: $desc (exit code $actual)"
        PASS=$((PASS + 1))
    else
        echo "  [FAIL] CLI: $desc (expected $expected, got $actual)"
        FAIL=$((FAIL + 1))
    fi
}

echo "=================================================="
echo "Running CLI End-to-End Contract Tests..."
echo "=================================================="

# Test 1: Successful execution
assert_exit_code "Normal run with valid files" 0 ./price_alert sample/prices.csv sample/rules.json

# Test 2: Argument count validation
assert_exit_code "No arguments should exit 1" 1 ./price_alert
assert_exit_code "1 argument should exit 1" 1 ./price_alert sample/prices.csv
assert_exit_code "3 arguments should exit 1" 1 ./price_alert sample/prices.csv sample/rules.json extra_arg

# Test 3: Nonexistent file
assert_exit_code "Nonexistent prices file should exit 1" 1 ./price_alert sample/missing.csv sample/rules.json
assert_exit_code "Nonexistent rules file should exit 1" 1 ./price_alert sample/prices.csv sample/missing.json

# Test 4: Stdout vs Stderr separation on error
ERR_OUT=$(./price_alert sample/missing.csv sample/rules.json 2>&1 1>/dev/null || true)
if [[ -n "$ERR_OUT" ]]; then
    echo "  [PASS] CLI: Diagnostics written to stderr on failure"
    PASS=$((PASS + 1))
else
    echo "  [FAIL] CLI: Stderr was empty on failure"
    FAIL=$((FAIL + 1))
fi

STDOUT_ON_ERR=$(./price_alert sample/missing.csv sample/rules.json 2>/dev/null || true)
if [[ -z "$STDOUT_ON_ERR" ]]; then
    echo "  [PASS] CLI: Stdout is empty when an error occurs"
    PASS=$((PASS + 1))
else
    echo "  [FAIL] CLI: Stdout was not empty on error: $STDOUT_ON_ERR"
    FAIL=$((FAIL + 1))
fi

# Test 5: Verify exact sample stdout matches expected output
ACTUAL=$(./price_alert sample/prices.csv sample/rules.json)
EXPECTED=$(cat sample/expected_output.txt)
if [[ "$ACTUAL" == "$EXPECTED" ]]; then
    echo "  [PASS] CLI: Output matches expected sample output verbatim"
    PASS=$((PASS + 1))
else
    echo "  [FAIL] CLI: Output mismatch"
    echo "Expected:"
    echo "$EXPECTED"
    echo "Actual:"
    echo "$ACTUAL"
    FAIL=$((FAIL + 1))
fi

echo "=================================================="
echo "CLI Test Summary: $PASS passed, $FAIL failed"
echo "=================================================="

if [[ $FAIL -ne 0 ]]; then
    exit 1
fi
