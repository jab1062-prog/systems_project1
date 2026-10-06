#!/bin/sh
# Run each terminating error case in its own process; verify stderr and status.
set -eu
test_tmp=$(mktemp -d)
trap 'rm -rf "$test_tmp"' EXIT HUP INT TERM

run_case() {
    expected=$1
    shift
    status=0
    "$@" >"$test_tmp/out" 2>"$test_tmp/err" || status=$?
    if [ "$status" -ne "$expected" ]; then
        echo "FAIL: $* returned $status, expected $expected" >&2
        cat "$test_tmp/err" >&2
        exit 1
    fi
}

run_case 0 ./test_correctness
grep -q 'All correctness tests passed.' "$test_tmp/out"
test ! -s "$test_tmp/err"

for case_name in outside interior double merged-double; do
    run_case 2 ./test_errors "$case_name"
    test ! -s "$test_tmp/out"
    grep -Eq '^free: Inappropriate pointer \(test_errors.c:[0-9]+\)$' "$test_tmp/err"
done

for case_name in oom oversize overflow zero; do
    run_case 0 ./test_errors "$case_name"
    test ! -s "$test_tmp/out"
    test "$(wc -l < "$test_tmp/err")" -eq 1
    grep -Eq '^malloc: Unable to allocate [0-9]+ bytes \(test_errors.c:[0-9]+\)$' "$test_tmp/err"
    case "$case_name" in
        oom) grep -q 'allocate 1 bytes' "$test_tmp/err" ;;
        oversize) grep -q 'allocate 4089 bytes' "$test_tmp/err" ;;
        zero) grep -q 'allocate 0 bytes' "$test_tmp/err" ;;
    esac
done

run_case 0 ./test_errors leak
test ! -s "$test_tmp/out"
test "$(cat "$test_tmp/err")" = 'mymalloc: 32 bytes leaked in 2 objects.'
for case_name in no-leak null; do
    run_case 0 ./test_errors "$case_name"
    test ! -s "$test_tmp/out"
    test ! -s "$test_tmp/err"
done

run_case 0 ./memgrind
test ! -s "$test_tmp/err"
grep -Eq '^Average time for the five-task workload \(50 runs\): [0-9]+\.[0-9]+ microseconds$' "$test_tmp/out"
echo 'All correctness, diagnostic, leak, and stress tests passed.'
