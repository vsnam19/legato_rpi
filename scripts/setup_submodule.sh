#!/usr/bin/env bash
# ==============================================================================
# Setup Legato AF Submodule for Raspberry Pi 5 Port
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
SUBMODULE_DIR="${REPO_ROOT}/submodules/legato-af"

echo "==> Setting up Legato AF submodule in ${SUBMODULE_DIR}..."

if [ ! -d "${SUBMODULE_DIR}" ]; then
    mkdir -p "${REPO_ROOT}/submodules"
fi

if [ -f "${SUBMODULE_DIR}/Makefile" ] && [ -f "${SUBMODULE_DIR}/CMakeLists.txt" ]; then
    echo "==> Legato AF is already present in ${SUBMODULE_DIR}."
    exit 0
fi

# If a local scratch copy exists from earlier clone, bootstrap from it to save time
SCRATCH_CLONE="/home/namvs/.gemini/antigravity-cli/brain/a4df534f-0db6-493a-8f41-987fb58c17b8/scratch/legato-af-shallow"
if [ -d "${SCRATCH_CLONE}/.git" ] && [ ! -d "${SUBMODULE_DIR}/.git" ]; then
    echo "==> Bootstrapping from local cache..."
    cp -r "${SCRATCH_CLONE}" "${SUBMODULE_DIR}"
else
    echo "==> Initializing git submodule from remote..."
    git -C "${REPO_ROOT}" submodule update --init --depth 1 submodules/legato-af || {
        echo "==> Submodule update fallback: direct shallow clone..."
        git clone --depth 1 --branch master https://github.com/legatoproject/legato-af.git "${SUBMODULE_DIR}"
    }
fi

echo "==> Legato AF submodule setup complete."
ls -lh "${SUBMODULE_DIR}/Makefile" "${SUBMODULE_DIR}/CMakeLists.txt"
