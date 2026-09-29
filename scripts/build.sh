#!/usr/bin/env bash
# ==============================================================================
# build.sh - Dual-Mode Builder for Qualcomm TelAF Simulation (C++20)
#
# Modes:
#   --standalone (default): Builds CfgManager service and CfgClient demo
#                          into .update packages.
#   --integrated          : Builds full TelAF simulation system image with
#                          vendor apps integrated.
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

BUILD_MODE="standalone"

for arg in "$@"; do
    case "$arg" in
        --standalone)
            BUILD_MODE="standalone"
            ;;
        --integrated)
            BUILD_MODE="integrated"
            ;;
        -h|--help)
            echo "Usage: $0 [--standalone|--integrated]"
            echo "  --standalone : Build CfgManager & CfgClient .update packages (default)"
            echo "  --integrated : Build complete TelAF system image with vendor apps"
            exit 0
            ;;
        *)
            echo "Unknown argument: $arg" >&2
            echo "Usage: $0 [--standalone|--integrated]" >&2
            exit 1
            ;;
    esac
done

echo "====================================================================="
echo " Qualcomm TelAF Simulation Builder (Mode: ${BUILD_MODE})"
echo "====================================================================="
echo "Project Root : ${PROJECT_ROOT}"
echo "Vendor Path  : ${PROJECT_ROOT}/vendor/custom"
echo "Upstream Path: ${PROJECT_ROOT}/upstream"
echo

# 1. Ensure upstream patches are applied
if [[ -f "${SCRIPT_DIR}/patch.sh" ]]; then
    if "${SCRIPT_DIR}/patch.sh" status | grep -q "\[UNAPPLIED\]"; then
        echo "==> Applying upstream patches..."
        "${SCRIPT_DIR}/patch.sh" apply
    fi
fi

# 2. Run compilation inside TelAF develop container
docker run --rm \
    --entrypoint /bin/bash \
    -u "$(id -u):$(id -g)" \
    -v "${PROJECT_ROOT}:/workspace" \
    telaf_simulation_develop_2204:1.0.0 -c "
    set +e
    set +u
    export PROJECT_ROOT=/workspace
    export TELAF_ROOT=/workspace/upstream/telaf
    export LEGATO_ROOT=/workspace/upstream/legato/legato-af

    cd \"\$TELAF_ROOT\"
    source set_af_env.sh simulation
    cd \"\$LEGATO_ROOT\"
    source framework/tools/scripts/configlegatoenv
    set -e
    set -u

    # Ensure host tools are built
    if [ ! -f \"\$LEGATO_ROOT/bin/mkapp\" ]; then
        echo '==> Host tools not found. Building host tools...'
        make tools -j\$(nproc)
    fi

    # Ensure simulation framework / liblegato is built
    if [ ! -f \"\$LEGATO_ROOT/build/simulation/framework/lib/liblegato.so\" ]; then
        echo '==> Building simulation framework...'
        make framework TARGET=simulation -j\$(nproc) || make framework TARGET=simulation
    fi

    if [ \"${BUILD_MODE}\" = \"standalone\" ]; then
        echo '==> Building CfgManager service (cfgManager.simulation.update)...'
        cd \"\$PROJECT_ROOT/vendor/custom/apps/cfgManager\"
        rm -rf _build_cfgManager
        mkapp -v -t simulation \
            -i \"\$PROJECT_ROOT/vendor/custom/interfaces\" \
            -i \"\$TELAF_ROOT/interfaces\" \
            cfgManager.adef

        echo '==> Building CfgClient sample (cfgClient.simulation.update)...'
        cd \"\$PROJECT_ROOT/vendor/custom/samples/cfgClient\"
        rm -rf _build_cfgClient
        mkapp -v -t simulation \
            -i \"\$PROJECT_ROOT/vendor/custom/interfaces\" \
            -i \"\$TELAF_ROOT/interfaces\" \
            cfgClient.adef

    elif [ \"${BUILD_MODE}\" = \"integrated\" ]; then
        echo '==> Building Integrated TelAF Simulation System...'
        cd \"\$PROJECT_ROOT/vendor/custom\"
        rm -rf _build_system
        mksys -v -t simulation \
            -i \"\$PROJECT_ROOT/vendor/custom/interfaces\" \
            -i \"\$TELAF_ROOT/interfaces\" \
            -w _build_system \
            -o _build_system \
            system_simulation.sdef
    fi
"

echo
echo "====================================================================="
if [ "${BUILD_MODE}" = "standalone" ]; then
    echo " Standalone Packages Built Successfully:"
    echo " - vendor/custom/apps/cfgManager/cfgManager.simulation.update"
    echo " - vendor/custom/samples/cfgClient/cfgClient.simulation.update"
else
    echo " Integrated TelAF System Built Successfully:"
    echo " - vendor/custom/_build_system/system_simulation.simulation.update"
fi
echo "====================================================================="
