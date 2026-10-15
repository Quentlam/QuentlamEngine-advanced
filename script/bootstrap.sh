#!/usr/bin/env bash
#
# bootstrap.sh - Linux entry point for the QuentlamEngine dependency bootstrap.
# Thin wrapper around BootstrapProject.py so a single implementation serves
# every platform (Windows uses script/BootstrapProject.bat instead).
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd)"

if command -v python3 >/dev/null 2>&1; then
    PYTHON_BIN="python3"
elif command -v python >/dev/null 2>&1; then
    PYTHON_BIN="python"
else
    echo "ERROR: python3 was not found on PATH." >&2
    exit 1
fi

exec "${PYTHON_BIN}" "${SCRIPT_DIR}/BootstrapProject.py" --repo-root "${REPO_ROOT}" "$@"
