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
echo " Running CfgManager Google Test Suite (C++23) in Docker"
echo "====================================================================="

bash "${PROJECT_ROOT}/docker/run-docker-build.sh" bash -c "
    set -euo pipefail
    BUILD_DIR=\"/workspace/build/tests\"
    mkdir -p \"\${BUILD_DIR}\"
    cd \"\${BUILD_DIR}\"

    echo '==> Configuring CMake for GTest...'
    cmake /workspace/tests -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON

    echo '==> Building unit test binary...'
    make -j\$(nproc)

    echo '==> Executing test runner...'
    ./cfgmanager_tests ${TEST_ARGS[*]:-}

    if [ \"${COVERAGE_FLAG}\" -eq 1 ]; then
        echo '====================================================================='
        echo ' C2 Branch Coverage Report (gcovr)'
        echo '====================================================================='
        gcovr --root /workspace \
              --filter '/workspace/core/' \
              --filter '/workspace/crypto/' \
              --filter '/workspace/security/' \
              --filter '/workspace/storage/' \
              --filter '/workspace/events/' \
              --branches \
              --print-summary || true
    fi
"
