#!/usr/bin/env bash
# ==============================================================================
# build-sample.sh - Build Legato sample applications for Raspberry Pi 5
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

APP_NAME="${1:-helloWorld}"

# Remove trailing slash or .adef extension if provided
APP_NAME="$(basename "${APP_NAME}" .adef)"

ADEF_PATH="${PROJECT_ROOT}/samples/${APP_NAME}/${APP_NAME}.adef"
if [ ! -f "${ADEF_PATH}" ]; then
    echo "ERROR: Sample application definition '${ADEF_PATH}' not found." >&2
    exit 1
fi

echo "====================================================================="
echo " Building Sample Application: ${APP_NAME} for Raspberry Pi 5"
echo "====================================================================="

# Run build inside Docker container
bash "${PROJECT_ROOT}/docker/run-docker-build.sh" bash -c "
    set -euo pipefail
    source /workspace/toolchain/env.sh
    export RPI5_TOOLCHAIN_DIR=\"/usr/bin\"
    export RPI5_TOOLCHAIN_PREFIX=\"aarch64-linux-gnu-\"

    mkdir -p /workspace/build/rpi5/apps
    rm -rf \"/workspace/build/rpi5/apps/_build_${APP_NAME}\"

    echo '==> Running mkapp for ${APP_NAME}...'
    cd \"/workspace/samples/${APP_NAME}\"
    mkapp -v -t rpi5 -o /workspace/build/rpi5/apps -w /workspace/build/rpi5/apps/_build_${APP_NAME} \"${APP_NAME}.adef\"

    echo '==> Sample build complete:'
    ls -lh \"/workspace/build/rpi5/apps/${APP_NAME}.rpi5.update\"
"

echo "====================================================================="
echo " Application update package ready: build/rpi5/apps/${APP_NAME}.rpi5.update"
echo "====================================================================="
