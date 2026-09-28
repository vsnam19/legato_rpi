#!/usr/bin/env bash
# ==============================================================================
# Internal Build Script executed inside Docker Container
# ==============================================================================
set -euo pipefail

WORKSPACE="/workspace"
LEGATO_ROOT="${WORKSPACE}/submodules/legato-af"
TARGET="rpi5"

echo "====================================================================="
echo " Building Legato Application Framework for Raspberry Pi 5 (${TARGET})"
echo "====================================================================="

# 1. Ensure Kconfiglib is present
if [ ! -f "${LEGATO_ROOT}/3rdParty/Kconfiglib/setconfig.py" ]; then
    echo "==> Fetching Kconfiglib..."
    git clone --depth 1 https://github.com/ulfalizer/Kconfiglib.git "${LEGATO_ROOT}/3rdParty/Kconfiglib"
fi

# 2. Ensure target definition symlinks
mkdir -p "${LEGATO_ROOT}/targets"
ln -sf "../../../targets/rpi5.sdef" "${LEGATO_ROOT}/targets/rpi5.sdef"
ln -sf "../../../targets/rpi5.sinc" "${LEGATO_ROOT}/targets/rpi5.sinc"

# 3. Ensure platformAdaptor symlink
if [ ! -d "${LEGATO_ROOT}/platformAdaptor/wdog" ]; then
    mkdir -p "${LEGATO_ROOT}/platformAdaptor"
    ln -sf "${WORKSPACE}/targets/platformAdaptor/wdog" "${LEGATO_ROOT}/platformAdaptor/wdog"
fi

# 4. Apply patch if not already applied
if [ -f "${WORKSPACE}/patches/0001-add-rpi5-target-support.patch" ]; then
    if ! git -C "${LEGATO_ROOT}" diff --quiet; then
        echo "==> Submodule working copy is ready."
    else
        echo "==> Applying Raspberry Pi 5 patches to Legato AF..."
        git -C "${LEGATO_ROOT}" apply "${WORKSPACE}/patches/0001-add-rpi5-target-support.patch" || true
    fi
fi

# 5. Build host tools first
echo "==> Building Legato host tools..."
cd "${LEGATO_ROOT}"
make tools

# 6. Build target system
echo "==> Building Legato target system for ${TARGET}..."
export PATH="${LEGATO_ROOT}/bin:${PATH}"
make "${TARGET}"

# 7. Export build artifacts to top-level build directory
mkdir -p "${WORKSPACE}/build/rpi5"
cp -f "${LEGATO_ROOT}/build/rpi5/system.rpi5.update" "${WORKSPACE}/build/rpi5/system.rpi5.update"
if [ -d "${LEGATO_ROOT}/build/rpi5/framework" ]; then
    mkdir -p "${WORKSPACE}/build/rpi5/framework"
    cp -rf "${LEGATO_ROOT}/build/rpi5/framework/bin" "${WORKSPACE}/build/rpi5/framework/"
    cp -rf "${LEGATO_ROOT}/build/rpi5/framework/lib" "${WORKSPACE}/build/rpi5/framework/"
fi

echo "====================================================================="
echo " Build Succeeded!"
echo " System update package: ${WORKSPACE}/build/rpi5/system.rpi5.update"
echo "====================================================================="
ls -lh "${WORKSPACE}/build/rpi5/system.rpi5.update"
