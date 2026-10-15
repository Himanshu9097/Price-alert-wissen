#!/usr/bin/env bash
set -euo pipefail

# Determine repository root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"

# Rebuild only if source files have changed (Make handles timestamp checks)
make -s price_alert

# Execute CLI tool with provided arguments
exec ./price_alert "$@"
