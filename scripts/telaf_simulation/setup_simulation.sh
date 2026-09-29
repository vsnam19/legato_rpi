#!/usr/bin/env bash
# ==============================================================================
# Setup Qualcomm TelAF Simulation Environment with HTTP 429 Resilience
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
SIMULATION_DIR="${WORKSPACE_ROOT}/telaf_simulation_env"

echo "=================================================================="
echo "    Qualcomm TelAF Simulation Environment Setup"
echo "=================================================================="
echo "Workspace Root : ${WORKSPACE_ROOT}"
echo "Simulation Dir : ${SIMULATION_DIR}"
echo

# 1. Check Docker environment
echo "[Step 1/3] Checking Docker prerequisites..."
if ! command -v docker &> /dev/null; then
    echo "ERROR: Docker CLI not found. Please install Docker." >&2
    exit 1
fi

if ! docker info &> /dev/null; then
    echo "ERROR: Docker daemon is not running or accessible." >&2
    exit 1
fi
echo "Docker daemon is accessible."

# Check for TelAF Docker images
if ! docker image inspect telaf_simulation_runtime_2204:1.0.0 &> /dev/null; then
    echo "WARNING: Image telaf_simulation_runtime_2204:1.0.0 not found locally."
    echo "Please ensure the Docker image is built or imported."
else
    echo "Found Docker image: telaf_simulation_runtime_2204:1.0.0"
fi
echo

# 2. Run resilient git clone (Handles 429 rate limits, cache fallback, backoff)
echo "[Step 2/3] Cloning/Updating TelAF repositories with HTTP 429 resilience..."
python3 "${SCRIPT_DIR}/clone_telaf_repos.py" \
    --config "${SCRIPT_DIR}/branch_mapping.conf" \
    --patch-config "${SCRIPT_DIR}/patch_me.json" \
    --target-dir "${SIMULATION_DIR}" \
    --pacing-delay 3.0 \
    --initial-backoff 10.0 \
    --max-retries 5

echo

# 3. Setup workstation files and simulation tarball
echo "[Step 3/3] Preparing simulation workstation artifacts..."
WORKSTATION_DIR="${SIMULATION_DIR}/telaf/simulation/workstation"
mkdir -p "${WORKSTATION_DIR}"

# Check for pre-built simulation tarball in known cache locations
CACHE_TARBALL="/home/namvs/Workspaces/projects/automotive/telaf_stu/simulation_env/telaf/simulation/workstation/telaf_simulation.tar.gz"
DEST_TARBALL="${WORKSTATION_DIR}/telaf_simulation.tar.gz"

if [ ! -f "${DEST_TARBALL}" ]; then
    if [ -f "${CACHE_TARBALL}" ]; then
        echo "Copying cached pre-built simulation tarball from ${CACHE_TARBALL}..."
        cp -af "${CACHE_TARBALL}" "${DEST_TARBALL}"
    else
        echo "WARNING: ${DEST_TARBALL} not found."
        echo "You can build it inside the developer container using 'make build-telaf' or provide a tarball."
    fi
else
    echo "Simulation tarball is present: ${DEST_TARBALL}"
fi

# Ensure simulation_configuration.json exists
if [ ! -f "${WORKSTATION_DIR}/simulation_configuration.json" ]; then
    cat << 'EOF' > "${WORKSTATION_DIR}/simulation_configuration.json"
{
    "master_tarball" : "telaf_simulation.tar.gz",
    "slavex_tarball" : "telaf_simulation.tar.gz"
}
EOF
fi

# Copy cgroup relocate script if missing
CACHE_CGROUP="/home/namvs/Workspaces/projects/automotive/telaf_stu/simulation_env/telaf/simulation/workstation/relocate_cgroup_v2_root.sh"
if [ ! -f "${WORKSTATION_DIR}/relocate_cgroup_v2_root.sh" ] && [ -f "${CACHE_CGROUP}" ]; then
    cp -af "${CACHE_CGROUP}" "${WORKSTATION_DIR}/relocate_cgroup_v2_root.sh"
fi

echo
echo "=================================================================="
echo " TelAF Simulation Environment Setup COMPLETE!"
echo " Next step: Launch simulation using:"
echo "   ${SCRIPT_DIR}/run_simulation.sh [start|status|shell|stop]"
echo "=================================================================="
