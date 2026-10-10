#!/usr/bin/env bash
# run_tests.sh - Automated test runner for IITPSx Assembler and Emulator
# Name: Yashashwani | ID: 2501AI49
# "I declare this is my own work."

set -u

echo "=========================================================="
echo "Running IITPSx Assembler & Emulator Test Suite"
echo "=========================================================="

PASSED=0
TOTAL=0

run_test() {
    local test_name="$1"
    local asm_file="tests/${test_name}.asm"
    local obj_file="tests/${test_name}.o"
    local lst_file="tests/${test_name}.lst"
    local log_file="tests/${test_name}.log"
    local emu_args="${2:-}"
    local expect_asm_fail="${3:-0}"
    local expect_emu_fail="${4:-0}"

    TOTAL=$((TOTAL + 1))
    echo -n "Test ${test_name}... "
    rm -f "${log_file}"

    echo "=== [1] Assembler Execution: ./asm ${asm_file} ===" > "${log_file}"
    ./asm "${asm_file}" >> "${log_file}" 2>&1
    local asm_status=$?

    if [ ${expect_asm_fail} -eq 1 ]; then
        if [ ${asm_status} -ne 0 ]; then
            echo "PASS (Expected assembly error detected)"
            PASSED=$((PASSED + 1))
        else
            echo "FAIL (Expected assembly error, but succeeded)"
        fi
        return
    else
        if [ ${asm_status} -ne 0 ]; then
            echo "FAIL (Assembly failed unexpectedly)"
            return
        fi
    fi

    # Run emulator if object file was produced
    if [ -f "${obj_file}" ]; then
        echo -e "\n=== [2] Emulator Execution: ./emu ${obj_file} ${emu_args} ===" >> "${log_file}"
        ./emu "${obj_file}" ${emu_args} >> "${log_file}" 2>&1
        local emu_status=$?

        if [ ${expect_emu_fail} -eq 1 ]; then
            if [ ${emu_status} -ne 0 ]; then
                echo "PASS (Expected emulator runtime error detected)"
                PASSED=$((PASSED + 1))
            else
                echo "FAIL (Expected runtime error, but emulator exited 0)"
            fi
        else
            if [ ${emu_status} -eq 0 ]; then
                echo "PASS (Clean assembly and successful execution)"
                PASSED=$((PASSED + 1))
            else
                echo "FAIL (Emulator exited with error: ${emu_status})"
            fi
        fi
    else
        echo "FAIL (Object file not generated)"
    fi
}

# test01: Valid test1 specification file (infinite loop on loop: br loop, test with -max 10)
run_test "test01" "-max 10" 0 1

# test02: Error test file (all errors reported, asm fails)
run_test "test02" "" 1 0

# test03: SET pseudo-op test (forward & backward references)
run_test "test03" "-trace" 0 0

# test04: mul and div verification (leaves A = 21)
run_test "test04" "-trace" 0 0

# test05: Bubble sort of array with -dump (sorted array output)
run_test "test05" "-dump" 0 0

# test06: Sum of N natural numbers (1..10 = 55)
run_test "test06" "-trace" 0 0

# test07: Polynomial evaluation (3x^2 + 5x + 7 for x=4 => 75)
run_test "test07" "-trace" 0 0

# test08: Mean of array of 5 elements using div (mean = 35)
run_test "test08" "-trace" 0 0

# test09: Division by zero error test (emulator detects div by 0 and exits 1)
run_test "test09" "" 0 1

# test10: Factorial of 6 using loop and mul (6! = 720)
run_test "test10" "-trace" 0 0

echo "=========================================================="
echo "Test Summary: ${PASSED}/${TOTAL} tests passed."
echo "=========================================================="

if [ ${PASSED} -eq ${TOTAL} ]; then
    exit 0
else
    exit 1
fi
