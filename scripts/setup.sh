#!/usr/bin/env bash
# Setup and dependency installation script for Ubuntu/Debian

set -e

COLOR_GREEN="\033[32m"
COLOR_RESET="\033[0m"

echo -e "${COLOR_GREEN}=== Installing Syscall Orchestra Dependencies ===${COLOR_RESET}"

if [ "$EUID" -ne 0 ]; then
    echo "This setup script installs system packages and must be run with sudo/root privileges."
    echo "Running: sudo apt-get update && sudo apt-get install -y build-essential libasound2-dev alsa-utils pkg-config"
    sudo apt-get update
    sudo apt-get install -y build-essential libasound2-dev alsa-utils pkg-config
else
    apt-get update
    apt-get install -y build-essential libasound2-dev alsa-utils pkg-config
fi

echo -e "${COLOR_GREEN}Setup and package installation completed!${COLOR_RESET}"
echo "You can now run 'make' to build the project."
exit 0
