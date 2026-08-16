#!/usr/bin/env bash
# Automated test runner for Syscall Orchestra

set -e

COLOR_GREEN="\033[32m"
COLOR_YELLOW="\033[33m"
COLOR_RED="\033[31m"
COLOR_RESET="\033[0m"

echo -e "${COLOR_GREEN}=== Starting Automated Tests ===${COLOR_RESET}"

# 1. Verify compilation
if [ ! -f "./syscall_orchestra" ]; then
    echo -e "${COLOR_RED}FAIL: Main binary './syscall_orchestra' not found. Please build first.${COLOR_RESET}"
    exit 1
fi

for target in file_activity process_activity; do
    if [ ! -f "./targets/$target" ]; then
        echo -e "${COLOR_RED}FAIL: Target binary './targets/$target' not found.${COLOR_RESET}"
        exit 1
    fi
done
echo -e "${COLOR_GREEN}PASS: Compilation and target files verified.${COLOR_RESET}"

# Temporary output file
TEST_OUT="test_output.tmp"

# Helper function to run a test and check for a string
run_test_and_check() {
    local test_name="$1"
    local cmd="$2"
    local expected_pattern="$3"
    
    echo -n "Running Test '$test_name': "
    
    # Run command and capture output
    eval "$cmd" > "$TEST_OUT" 2>&1 || true
    
    # Check for pattern
    if grep -E "$expected_pattern" "$TEST_OUT" > /dev/null; then
        echo -e "${COLOR_GREEN}PASS${COLOR_RESET}"
    else
        echo -e "${COLOR_RED}FAIL${COLOR_RESET}"
        echo "Command run: $cmd"
        echo "Expected pattern to match: $expected_pattern"
        echo "--- First 10 lines of actual output ---"
        head -n 15 "$TEST_OUT"
        echo "---------------------------------------"
        rm -f "$TEST_OUT"
        exit 1
    fi
}

# 2. Test File Activity tracing & decoding
run_test_and_check "File Activity Syscalls" \
    "./syscall_orchestra --no-audio ./targets/file_activity" \
    "open(at)?|read|write|close"

# 3. Test Process Activity tracing & decoding
run_test_and_check "Process Activity Syscalls" \
    "./syscall_orchestra --no-audio ./targets/process_activity" \
    "clone|fork|execve|wait"

# 4. Test Debug Mode with PTRACE_PEEKTEXT
run_test_and_check "Debug PTRACE_PEEKTEXT logs" \
    "./syscall_orchestra --no-audio --debug ./targets/file_activity" \
    "DEBUG PTRACE_PEEKTEXT"

# 7. Test Stats Output
run_test_and_check "Stats Summary Table" \
    "./syscall_orchestra --no-audio ./targets/file_activity" \
    "SYSCALL ORCHESTRA SUMMARY"

# 8. Test Invalid Target Handling (missing executable)
run_test_and_check "Graceful Invalid Target handling" \
    "./syscall_orchestra --no-audio ./targets/non_existent_executable" \
    "not found or not executable|Failed to launch"

echo -e "\n${COLOR_GREEN}=== All tests passed successfully! ===${COLOR_RESET}"
rm -f "$TEST_OUT"
exit 0
