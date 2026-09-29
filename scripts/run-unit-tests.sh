#!/usr/bin/env bash
# ==============================================================================
# run-unit-tests.sh - Google Test runner with C2 branch coverage
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

COVERAGE_FLAG=0
TEST_ARGS=()

for arg in "$@"; do
    case "$arg" in
        --coverage)
            COVERAGE_FLAG=1
            ;;
        *)
            TEST_ARGS+=("$arg")
            ;;
    esac
done

echo "====================================================================="
echo " Running CfgManager Google Test Suite (C++20)"
echo "====================================================================="

BUILD_DIR="${PROJECT_ROOT}/build/tests"
if [ -f "${BUILD_DIR}/CMakeCache.txt" ]; then
    if ! grep -q "vendor/custom/tests" "${BUILD_DIR}/CMakeCache.txt"; then
        rm -rf "${BUILD_DIR}"
        mkdir -p "${BUILD_DIR}"
    fi
fi
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

echo "==> Configuring CMake for GTest..."
cmake "${PROJECT_ROOT}/vendor/custom/tests" -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON

echo "==> Building unit test binary..."
make -j"$(nproc)"

echo "==> Executing test runner..."
./cfgmanager_tests ${TEST_ARGS[*]:-}

if [ "${COVERAGE_FLAG}" -eq 1 ]; then
    echo "====================================================================="
    echo " C2 Branch Coverage Report (gcovr)"
    echo "====================================================================="
    gcovr --root "${PROJECT_ROOT}" \
          --filter "${PROJECT_ROOT}/vendor/custom/components/cfgManager/core/" \
          --filter "${PROJECT_ROOT}/vendor/custom/components/cfgManager/crypto/" \
          --filter "${PROJECT_ROOT}/vendor/custom/components/cfgManager/security/" \
          --filter "${PROJECT_ROOT}/vendor/custom/components/cfgManager/storage/" \
          --filter "${PROJECT_ROOT}/vendor/custom/components/cfgManager/events/" \
          --branches \
          --print-summary || true
fi
