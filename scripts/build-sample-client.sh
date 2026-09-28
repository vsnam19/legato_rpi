#!/usr/bin/env bash
# ==============================================================================
# build-sample-client.sh - Build Legato CfgClient Sample for Raspberry Pi 5
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

echo "====================================================================="
echo " Building CfgClient Sample Application for Raspberry Pi 5"
echo "====================================================================="

# Run build inside Docker container
bash "${PROJECT_ROOT}/docker/run-docker-build.sh" bash -c "
    set -euo pipefail
    source /workspace/toolchain/env.sh
    export RPI5_TOOLCHAIN_DIR=\"/usr/bin\"
    export RPI5_TOOLCHAIN_PREFIX=\"aarch64-linux-gnu-\"
    export PROJECT_ROOT=\"/workspace\"

    mkdir -p /workspace/build/rpi5/apps
    rm -rf /workspace/build/rpi5/apps/_build_cfgClient

    echo '==> Running mkapp for cfgClient...'
    cd /workspace/samples/cfgClient
    mkapp -v -t rpi5 -o /workspace/build/rpi5/apps -w /workspace/build/rpi5/apps/_build_cfgClient cfgClient.adef

    echo '==> CfgClient package build complete:'
    ls -lh /workspace/build/rpi5/apps/cfgClient.rpi5.update
"

echo "====================================================================="
echo " CfgClient package ready: build/rpi5/apps/cfgClient.rpi5.update"
echo "====================================================================="
