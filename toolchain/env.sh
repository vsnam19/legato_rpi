#!/usr/bin/env bash
# ==============================================================================
# Environment Configuration for Legato AF Raspberry Pi 5 Target
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

export LEGATO_ROOT="${REPO_ROOT}/submodules/legato-af"
export LEGATO_TARGET="rpi5"
export TARGET="rpi5"

# Toolchain definitions (AArch64)
export ARCH="arm64"
export CROSS_COMPILE="aarch64-linux-gnu-"
export CC="aarch64-linux-gnu-gcc"
export CXX="aarch64-linux-gnu-g++"
export AR="aarch64-linux-gnu-ar"
export AS="aarch64-linux-gnu-as"
export LD="aarch64-linux-gnu-ld"
export STRIP="aarch64-linux-gnu-strip"

# Legato mkTools target-specific toolchain discovery
export RPI5_TOOLCHAIN_DIR="/usr/bin"
export RPI5_TOOLCHAIN_PREFIX="aarch64-linux-gnu-"


# Legato paths
export PATH="${LEGATO_ROOT}/bin:${PATH}"
export LEGATO_TARGET_SINC="${REPO_ROOT}/targets/rpi5.sinc"
