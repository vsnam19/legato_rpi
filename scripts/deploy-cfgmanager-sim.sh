#!/usr/bin/env bash
# ==============================================================================
# deploy-cfgmanager-sim.sh - Deploy & Verify CfgManager on TelAF Simulation
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

CONTAINER_NAME="telaf_simulation_runtime_2204_m"
CFG_MGR_PKG="${PROJECT_ROOT}/apps/cfgManager/cfgManager.simulation.update"
CFG_CLIENT_PKG="${PROJECT_ROOT}/samples/cfgClient/cfgClient.simulation.update"

echo "====================================================================="
echo " Deploying CfgManager & CfgClient to TelAF Simulation Runtime"
echo "====================================================================="
echo "Container: ${CONTAINER_NAME}"
echo

# 1. Verify container is running
if ! docker ps --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "ERROR: Container ${CONTAINER_NAME} is not running." >&2
    echo "Please start the TelAF simulation environment first (e.g. telaf start)." >&2
    exit 1
fi

# 2. Check if update packages exist; if not, build them
if [ ! -f "${CFG_MGR_PKG}" ] || [ ! -f "${CFG_CLIENT_PKG}" ]; then
    echo "Update packages not found. Invoking build script..."
    "${SCRIPT_DIR}/build-cfgmanager-sim.sh"
fi

# 3. Copy update packages into runtime container
echo "==> Copying packages to runtime container..."
docker cp "${CFG_MGR_PKG}" "${CONTAINER_NAME}:/tmp/cfgManager.simulation.update"
docker cp "${CFG_CLIENT_PKG}" "${CONTAINER_NAME}:/tmp/cfgClient.simulation.update"

# 4. Install packages via Legato update daemon
echo "==> Installing cfgManager package..."
docker exec "${CONTAINER_NAME}" /bin/bash -c "export PATH=/legato/systems/current/bin:\$PATH && update /tmp/cfgManager.simulation.update"

echo "==> Installing cfgClient package..."
docker exec "${CONTAINER_NAME}" /bin/bash -c "export PATH=/legato/systems/current/bin:\$PATH && update /tmp/cfgClient.simulation.update"

echo "==> Restarting cfgClient to run live demo tests..."
docker exec "${CONTAINER_NAME}" /bin/bash -c "export PATH=/legato/systems/current/bin:\$PATH && app restart cfgClient"

# Give a brief moment for client demo to execute
sleep 2

# 5. Check app status
echo
echo "==> Verifying application status:"
docker exec "${CONTAINER_NAME}" /bin/bash -c "export PATH=/legato/systems/current/bin:\$PATH && app status | grep -E 'cfgManager|cfgClient'"

# 6. Show execution logs from syslog
echo
echo "==> Client demo logread output:"
docker exec "${CONTAINER_NAME}" /bin/bash -c "logread | grep -E 'cfgClient|cfgManager' | tail -n 25"

echo
echo "====================================================================="
echo " Deployment and Verification Complete!"
echo "====================================================================="
