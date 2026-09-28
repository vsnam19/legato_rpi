#!/usr/bin/env bash
# ==============================================================================
# deploy.sh - Network Deployment Script for Legato AF on Raspberry Pi 5
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

DEFAULT_USER="pi"
SSH_PORT="22"
UPDATE_FILE="${PROJECT_ROOT}/build/rpi5/system.rpi5.update"

usage() {
    cat << EOF
Usage: $0 [MODE] <TARGET_IP> [OPTIONS]

Automated deployment tool for Legato AF on Raspberry Pi 5 over SSH.

Modes:
  --bootstrap <TARGET_IP> [USER]    Perform first-time installation and service setup on target
  --update <TARGET_IP> [FILE]       Push system or app update package to running Legato system
  --status <TARGET_IP> [USER]       Query Legato system status, supervisor, and active services
  --logs <TARGET_IP> [USER]         Follow live Legato framework logs via journalctl
  -h, --help                        Show this help message

Default target user: ${DEFAULT_USER}
Default update file: build/rpi5/system.rpi5.update

Examples:
  # First-time target deployment:
  $0 --bootstrap 192.168.1.100 pi

  # Deploy an updated system image over network:
  $0 --update 192.168.1.100

  # Deploy a specific sample app:
  $0 --update 192.168.1.100 build/rpi5/apps/helloWorld.rpi5.update

  # Check Legato status on device:
  $0 --status 192.168.1.100
EOF
    exit 0
}

if [ $# -eq 0 ]; then
    usage
fi

MODE="$1"
shift

case "${MODE}" in
    -h|--help)
        usage
        ;;
    --bootstrap)
        if [ $# -lt 1 ]; then
            echo "ERROR: Target IP required for bootstrap." >&2
            usage
        fi
        TARGET_IP="$1"
        USER="${2:-$DEFAULT_USER}"
        SSH_DEST="${USER}@${TARGET_IP}"

        echo "====================================================================="
        echo " Bootstrapping Legato AF on ${SSH_DEST}"
        echo "====================================================================="

        if [ ! -f "${UPDATE_FILE}" ]; then
            echo "ERROR: Build artifact '${UPDATE_FILE}' not found." >&2
            echo "Run 'bash scripts/build.sh' first." >&2
            exit 1
        fi

        echo "==> Testing SSH connection to ${SSH_DEST}..."
        ssh -o ConnectTimeout=5 -p "${SSH_PORT}" "${SSH_DEST}" "uname -a"

        echo "==> Creating temporary staging area on target..."
        ssh -p "${SSH_PORT}" "${SSH_DEST}" "rm -rf /tmp/legato-bootstrap && mkdir -p /tmp/legato-bootstrap"

        echo "==> Transferring system update package and installer scripts..."
        scp -P "${SSH_PORT}" "${UPDATE_FILE}" "${SSH_DEST}:/tmp/legato-bootstrap/system.rpi5.update"
        scp -r -P "${SSH_PORT}" "${PROJECT_ROOT}/target-root"/* "${SSH_DEST}:/tmp/legato-bootstrap/"

        echo "==> Running installer on remote Raspberry Pi 5..."
        ssh -t -p "${SSH_PORT}" "${SSH_DEST}" "sudo /tmp/legato-bootstrap/install-rpi5.sh /tmp/legato-bootstrap/system.rpi5.update"

        echo "==> Starting Legato service..."
        ssh -p "${SSH_PORT}" "${SSH_DEST}" "sudo systemctl start legato"

        echo "==> Cleaning up staging files..."
        ssh -p "${SSH_PORT}" "${SSH_DEST}" "rm -rf /tmp/legato-bootstrap"

        echo "==> Verifying Legato status..."
        sleep 2
        ssh -p "${SSH_PORT}" "${SSH_DEST}" "sudo /opt/legato/current/bin/legato status || sudo systemctl status legato --no-pager"

        echo "====================================================================="
        echo " Bootstrap complete! Legato is active on ${TARGET_IP}."
        echo "====================================================================="
        ;;

    --update)
        if [ $# -lt 1 ]; then
            echo "ERROR: Target IP required for update." >&2
            usage
        fi
        TARGET_IP="$1"
        shift
        if [ $# -gt 0 ] && [ -f "$1" ]; then
            UPDATE_PKG="$1"
            shift
        else
            UPDATE_PKG="${UPDATE_FILE}"
        fi
        USER="${1:-$DEFAULT_USER}"
        SSH_DEST="${USER}@${TARGET_IP}"

        echo "====================================================================="
        echo " Pushing update '${UPDATE_PKG}' to ${SSH_DEST}"
        echo "====================================================================="

        if [ ! -f "${UPDATE_PKG}" ]; then
            echo "ERROR: Update package '${UPDATE_PKG}' not found." >&2
            exit 1
        fi

        # Push directly to target update tool through SSH stream (Legato instsys mechanism)
        cat "${UPDATE_PKG}" | ssh -p "${SSH_PORT}" "${SSH_DEST}" "sudo /opt/legato/current/bin/update || sudo /legato/systems/current/bin/update"

        echo "==> Update applied successfully."
        ;;

    --status)
        if [ $# -lt 1 ]; then
            echo "ERROR: Target IP required." >&2
            usage
        fi
        TARGET_IP="$1"
        USER="${2:-$DEFAULT_USER}"
        SSH_DEST="${USER}@${TARGET_IP}"

        echo "====================================================================="
        echo " Legato Status on ${SSH_DEST}"
        echo "====================================================================="
        ssh -p "${SSH_PORT}" "${SSH_DEST}" "sudo /opt/legato/current/bin/legato status || sudo systemctl status legato --no-pager"
        echo ""
        echo "==> Service Directory (sdir list):"
        ssh -p "${SSH_PORT}" "${SSH_DEST}" "sudo /opt/legato/current/bin/sdir list 2>/dev/null || true"
        ;;

    --logs)
        if [ $# -lt 1 ]; then
            echo "ERROR: Target IP required." >&2
            usage
        fi
        TARGET_IP="$1"
        USER="${2:-$DEFAULT_USER}"
        SSH_DEST="${USER}@${TARGET_IP}"

        echo "Streaming logs from ${SSH_DEST} (Ctrl+C to exit)..."
        ssh -t -p "${SSH_PORT}" "${SSH_DEST}" "sudo journalctl -u legato -f"
        ;;

    *)
        # If first arg looks like an IP or hostname, treat as update or status
        if [[ "${MODE}" =~ ^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$ ]] || [[ "${MODE}" =~ ^[a-zA-Z0-9.-]+$ ]]; then
            TARGET_IP="${MODE}"
            USER="${1:-$DEFAULT_USER}"
            SSH_DEST="${USER}@${TARGET_IP}"
            echo "Deploying update to ${SSH_DEST}..."
            cat "${UPDATE_FILE}" | ssh -p "${SSH_PORT}" "${SSH_DEST}" "sudo /opt/legato/current/bin/update"
        else
            echo "ERROR: Unknown option '${MODE}'." >&2
            usage
        fi
        ;;
esac
