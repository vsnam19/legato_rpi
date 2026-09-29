#!/usr/bin/env bash
# ==============================================================================
# build.sh - Build CfgManager & CfgClient (C++23) for Qualcomm TelAF Simulation
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Search for TelAF simulation environment with built host tools
SIM_ENV="${PROJECT_ROOT}/telaf_simulation_env"
if [ ! -f "${SIM_ENV}/legato/legato-af/bin/mkapp" ]; then
    FALLBACK_ENV="/home/namvs/Workspaces/projects/automotive/telaf_stu/simulation_env"
    if [ -f "${FALLBACK_ENV}/legato/legato-af/bin/mkapp" ]; then
        SIM_ENV="${FALLBACK_ENV}"
    elif [ ! -d "${SIM_ENV}/telaf" ]; then
        echo "ERROR: TelAF simulation environment not found at ${SIM_ENV} or fallback." >&2
        exit 1
    fi
fi

echo "====================================================================="
echo " Building CfgManager & CfgClient (C++23) for TelAF Simulation"
echo "====================================================================="
echo "Project Root   : ${PROJECT_ROOT}"
echo "Simulation Env : ${SIM_ENV}"
echo

# Compile inside Ubuntu 22.04 develop container to ensure binary compatibility
docker run --rm \
    --entrypoint /bin/bash \
    -u "$(id -u):$(id -g)" \
    -v "${PROJECT_ROOT}:/workspace" \
    -v "${SIM_ENV}:/simulation_env" \
    telaf_simulation_develop_2204:1.0.0 -c '
    set +e
    set +u
    export PROJECT_ROOT=/workspace
    export TELAF_ROOT=/simulation_env/telaf

    cd "$TELAF_ROOT"
    source set_af_env.sh simulation
    cd ../legato/legato-af
    source framework/tools/scripts/configlegatoenv
    set -e
    set -u

    echo "==> Building CfgManager service (cfgManager.simulation.update)..."
    cd "$PROJECT_ROOT/apps/cfgManager"
    rm -rf _build_cfgManager
    mkapp -v -t simulation -i "$PROJECT_ROOT/interfaces" -i "$TELAF_ROOT/interfaces" cfgManager.adef

    echo "==> Building CfgClient sample (cfgClient.simulation.update)..."
    cd "$PROJECT_ROOT/samples/cfgClient"
    rm -rf _build_cfgClient
    mkapp -v -t simulation -i "$PROJECT_ROOT/interfaces" -i "$TELAF_ROOT/interfaces" cfgClient.adef
'

echo
echo "====================================================================="
echo " Packages Built Successfully for TelAF Simulation:"
echo " - apps/cfgManager/cfgManager.simulation.update"
echo " - samples/cfgClient/cfgClient.simulation.update"
echo "====================================================================="
