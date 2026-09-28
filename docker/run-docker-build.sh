#!/usr/bin/env bash
# ==============================================================================
# Run Build Command inside Legato RPi 5 Docker Container
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
IMAGE_NAME="legato-rpi5-builder:latest"
DOCKERFILE="${SCRIPT_DIR}/Dockerfile"

# Check docker availability
if ! command -v docker >/dev/null 2>&1; then
    echo "ERROR: 'docker' is not installed or not in PATH." >&2
    exit 1
fi

# Build image if it doesn't exist or Dockerfile is newer
if ! docker image inspect "${IMAGE_NAME}" >/dev/null 2>&1; then
    echo "==> Docker image '${IMAGE_NAME}' not found. Building..."
    docker build -t "${IMAGE_NAME}" -f "${DOCKERFILE}" "${SCRIPT_DIR}"
fi

CURRENT_UID=$(id -u)
CURRENT_GID=$(id -g)

# If no arguments provided, run interactive bash
if [ $# -eq 0 ]; then
    set -- /bin/bash
fi

exec docker run --rm -i \
    -u "${CURRENT_UID}:${CURRENT_GID}" \
    -v "${REPO_ROOT}:/workspace" \
    -w /workspace \
    -e HOME=/tmp \
    -e USER="${USER:-user}" \
    "${IMAGE_NAME}" \
    "$@"
