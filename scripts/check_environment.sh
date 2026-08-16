#!/usr/bin/env bash
# Environment verification script for Syscall Orchestra

set -e

COLOR_GREEN="\033[32m"
COLOR_YELLOW="\033[33m"
COLOR_RED="\033[31m"
COLOR_RESET="\033[0m"

echo -e "${COLOR_GREEN}=== Syscall Orchestra Environment Check ===${COLOR_RESET}"

# 1. OS Check
OS_TYPE=$(uname -s)
echo -n "Checking OS Type: "
if [ "$OS_TYPE" != "Linux" ]; then
    echo -e "${COLOR_RED}FAILED (Found: $OS_TYPE)${COLOR_RESET}"
    echo -e "Syscall Orchestra requires a Linux environment for ptrace tracing."
    exit 1
else
    echo -e "${COLOR_GREEN}OK (Linux)${COLOR_RESET}"
fi

# 2. Arch Check (ptrace registers are architecture-specific)
ARCH_TYPE=$(uname -m)
echo -n "Checking CPU Architecture: "
if [ "$ARCH_TYPE" != "x86_64" ]; then
    echo -e "${COLOR_YELLOW}WARNING (Found: $ARCH_TYPE)${COLOR_RESET}"
    echo "Note: Syscall register decoding is optimized for x86_64 Linux."
else
    echo -e "${COLOR_GREEN}OK (x86_64)${COLOR_RESET}"
fi

# 3. Compiler Check
echo -n "Checking Compiler (gcc): "
if command -v gcc >/dev/null 2>&1; then
    GCC_VER=$(gcc --version | head -n 1)
    echo -e "${COLOR_GREEN}OK ($GCC_VER)${COLOR_RESET}"
else
    echo -e "${COLOR_RED}FAILED (gcc not found)${COLOR_RESET}"
    exit 1
fi

# 4. Make Check
echo -n "Checking Build Tool (make): "
if command -v make >/dev/null 2>&1; then
    echo -e "${COLOR_GREEN}OK${COLOR_RESET}"
else
    echo -e "${COLOR_RED}FAILED (make not found)${COLOR_RESET}"
    exit 1
fi

# 5. Ptrace Permissions Check (Yama ptrace_scope)
echo -n "Checking Yama ptrace_scope: "
if [ -f /proc/sys/kernel/yama/ptrace_scope ]; then
    PTRACE_SCOPE=$(cat /proc/sys/kernel/yama/ptrace_scope)
    echo -n "ptrace_scope is $PTRACE_SCOPE "
    if [ "$PTRACE_SCOPE" -ne 0 ] && [ "$PTRACE_SCOPE" -ne 1 ]; then
        echo -e "${COLOR_YELLOW}RESTRICTED (Value > 1)${COLOR_RESET}"
        echo "Warning: You may need to run as root or set: sudo sysctl -w kernel.yama.ptrace_scope=1"
    else
        echo -e "${COLOR_GREEN}OK${COLOR_RESET}"
    fi
else
    echo -e "${COLOR_GREEN}OK (yama ptrace_scope not present/active)${COLOR_RESET}"
fi

# 6. Audio System Check
echo -n "Checking Audio Utility (aplay): "
if command -v aplay >/dev/null 2>&1; then
    echo -e "${COLOR_GREEN}OK (aplay found)${COLOR_RESET}"
else
    echo -e "${COLOR_YELLOW}WARNING (aplay not found)${COLOR_RESET}"
    echo "Without aplay or libasound2, audio output will fall back to silent mock tracing."
fi

echo -e "\n${COLOR_GREEN}Environment check completed successfully!${COLOR_RESET}"
exit 0
