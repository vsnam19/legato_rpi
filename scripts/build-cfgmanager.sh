#!/usr/bin/env bash
# ==============================================================================
# build-cfgmanager.sh - Build Legato CfgManager Service for Raspberry Pi 5
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

echo "====================================================================="
echo " Building CfgManager Legato Service for Raspberry Pi 5"
echo "====================================================================="

# Run build inside Docker container
bash "${PROJECT_ROOT}/docker/run-docker-build.sh" bash -c "
    set -euo pipefail
    source /workspace/toolchain/env.sh
    export RPI5_TOOLCHAIN_DIR=\"/usr/bin\"
    export RPI5_TOOLCHAIN_PREFIX=\"aarch64-linux-gnu-\"
    export PROJECT_ROOT=\"/workspace\"

    mkdir -p /workspace/build/rpi5/apps
    rm -rf /workspace/build/rpi5/apps/_build_cfgManager

    echo '==> Running mkapp for cfgManager...'
    cd /workspace/apps/cfgManager
    mkapp -v -t rpi5 -o /workspace/build/rpi5/apps -w /workspace/build/rpi5/apps/_build_cfgManager cfgManager.adef

    echo '==> CfgManager service package build complete:'
    ls -lh /workspace/build/rpi5/apps/cfgManager.rpi5.update
"

echo "====================================================================="
echo " CfgManager package ready: build/rpi5/apps/cfgManager.rpi5.update"
echo "====================================================================="
