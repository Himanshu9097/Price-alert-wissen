#!/usr/bin/env bash
set -euo pipefail

# Determine repository root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"

# Build executable if not present or outdated
if [[ ! -f "price_alert" ]]; then
    echo "Building price_alert..." >&2
    make -s price_alert
fi

# Execute CLI tool with provided arguments
exec ./price_alert "$@"
