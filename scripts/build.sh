#!/usr/bin/env bash
# ==============================================================================
# Host Build Driver for Legato AF Raspberry Pi 5 Port
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Ensure submodule is initialized
if [ ! -f "${REPO_ROOT}/submodules/legato-af/Makefile" ]; then
    echo "==> Submodule not detected. Running setup_submodule.sh..."
    bash "${SCRIPT_DIR}/setup_submodule.sh"
fi

echo "==> Launching containerized build via Docker..."
exec bash "${REPO_ROOT}/docker/run-docker-build.sh" /workspace/scripts/internal-build.sh "$@"
