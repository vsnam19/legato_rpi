#!/usr/bin/env bash
# ==============================================================================
# Qualcomm TelAF Simulation Runner
# ==============================================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

# Target simulation workstation directory
SIMULATION_DIR="${WORKSPACE_ROOT}/telaf_simulation_env"
WORKSTATION_DIR="${SIMULATION_DIR}/telaf/simulation/workstation"

# Fallback to pre-existing workstation if telaf_simulation_env has not been set up
if [ ! -f "${WORKSTATION_DIR}/telaf_simulation.tar.gz" ]; then
    FALLBACK_WORKSTATION="/home/namvs/Workspaces/projects/automotive/telaf_stu/simulation_env/telaf/simulation/workstation"
    if [ -f "${FALLBACK_WORKSTATION}/telaf_simulation.tar.gz" ]; then
        WORKSTATION_DIR="${FALLBACK_WORKSTATION}"
    fi
fi

CONTAINER_NAME="telaf_simulation_runtime_2204_m"
IMG_NAME="telaf_simulation_runtime_2204"
IMG_VERSION="1.0.0"
IPV6_NETWORK_NAME="${CONTAINER_NAME%??}_ipv6net"
IPV6_DEFAULT_SUBNET="2001:0DB8::/112"
SIMULATION_TARBALL_NAME="telaf_simulation.tar.gz"

# Detect cgroup version
if [ -d "/sys/fs/cgroup/freezer" ]; then
    CG_OPTIONS="--cgroupns=private"
elif [ -e "/sys/fs/cgroup/cgroup.controllers" ]; then
    CG_OPTIONS="--cgroupns=private --cgroup-parent=user.slice"
else
    CG_OPTIONS=""
fi

# Volumes
APP_VOL="${CONTAINER_NAME}_sml_app"
DATA_VOL="${CONTAINER_NAME}_sml_data"
PERSIST_VOL="${CONTAINER_NAME}_sml_persist"
MNT_LEGATO_VOL="${CONTAINER_NAME}_sml_mnt_legato"
SSH_INFO_VOL="telaf_simulation_common_sml_ssh_info"

ensure_infrastructure() {
    # Ensure volumes exist
    for vol in "$APP_VOL" "$DATA_VOL" "$PERSIST_VOL" "$MNT_LEGATO_VOL" "$SSH_INFO_VOL"; do
        if ! docker volume inspect "$vol" &>/dev/null; then
            docker volume create "$vol" >/dev/null
        fi
    done

    # Ensure ipv6 bridge network exists
    if ! docker network inspect "$IPV6_NETWORK_NAME" &>/dev/null; then
        docker network create --driver bridge --ipv6 --subnet "$IPV6_DEFAULT_SUBNET" "$IPV6_NETWORK_NAME" >/dev/null
    fi
}

is_container_running() {
    [ "$(docker inspect -f '{{.State.Running}}' "${CONTAINER_NAME}" 2>/dev/null)" = "true" ]
}

start_daemon() {
    ensure_infrastructure

    if is_container_running; then
        echo "Container '${CONTAINER_NAME}' is already running."
        return 0
    fi

    # Clean up stale stopped container
    docker rm -f "${CONTAINER_NAME}" &>/dev/null || true

    echo "Starting TelAF Simulation Container (${CONTAINER_NAME})..."
    docker run -d \
        --name "${CONTAINER_NAME}" \
        -i -t \
        --privileged=true \
        --net="${IPV6_NETWORK_NAME}" \
        -p 9022:22 \
        ${CG_OPTIONS} \
        -e TELAF_IN_CONTAINER=yes \
        -e CONTAINER_WHO_AM_I=master \
        -e CONTAINER_NAME="${CONTAINER_NAME}" \
        -e SIMULATION_TARBALL_NAME="${SIMULATION_TARBALL_NAME}" \
        -v "${WORKSTATION_DIR}:/root/simulation:rw" \
        -v "${APP_VOL}:/app:rw" \
        -v "${DATA_VOL}:/data:rw" \
        -v "${PERSIST_VOL}:/persist:rw" \
        -v "${MNT_LEGATO_VOL}:/mnt/legato:rw" \
        -v "${SSH_INFO_VOL}:/root/simulation/.ssh:rw" \
        "${IMG_NAME}:${IMG_VERSION}" \
        /bin/bash -c "source /root/simulation/up_simulation.sh master; /legato/systems/current/bin/telaf start; sleep 2; tail -f /dev/null" >/dev/null

    echo "Waiting for TelAF framework to initialize..."
    sleep 5

    echo "Checking TelAF version & status:"
    docker exec "${CONTAINER_NAME}" /legato/systems/current/bin/telaf version
    docker exec "${CONTAINER_NAME}" /legato/systems/current/bin/telaf status
}

check_status() {
    if ! is_container_running; then
        echo "TelAF Simulation Container '${CONTAINER_NAME}' is NOT running."
        exit 1
    fi
    echo "=== Container Status ==="
    docker inspect -f 'Name: {{.Name}} | Status: {{.State.Status}} | IP: {{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' "${CONTAINER_NAME}"
    echo
    echo "=== TelAF Framework Version ==="
    docker exec "${CONTAINER_NAME}" /legato/systems/current/bin/telaf version
    echo
    echo "=== TelAF Framework Status ==="
    docker exec "${CONTAINER_NAME}" /legato/systems/current/bin/telaf status
    echo
    echo "=== Legato Installed Apps ==="
    docker exec "${CONTAINER_NAME}" /legato/systems/current/bin/app list 2>/dev/null || true
}

get_version() {
    if ! is_container_running; then
        echo "TelAF Simulation Container is not running. Starting temporarily..."
        start_daemon
    fi
    docker exec "${CONTAINER_NAME}" /legato/systems/current/bin/telaf version
}

stop_daemon() {
    if is_container_running; then
        echo "Stopping TelAF Simulation Container (${CONTAINER_NAME})..."
        docker stop -t 3 "${CONTAINER_NAME}" >/dev/null
        docker rm "${CONTAINER_NAME}" >/dev/null
        echo "Simulation container stopped."
    else
        echo "Container '${CONTAINER_NAME}' is not running."
    fi
}

enter_shell() {
    if ! is_container_running; then
        echo "Container is not running. Starting it now..."
        start_daemon
    fi
    echo "Attaching interactive bash session to TelAF Simulation container..."
    docker exec -it "${CONTAINER_NAME}" /bin/bash
}

get_logs() {
    if ! is_container_running; then
        echo "Container is not running."
        exit 1
    fi
    docker exec "${CONTAINER_NAME}" /sbin/logread -f
}

cmd="${1:-start}"
case "$cmd" in
    start|up|run)
        start_daemon
        ;;
    status)
        check_status
        ;;
    version)
        get_version
        ;;
    stop|down)
        stop_daemon
        ;;
    shell|sh|bash)
        enter_shell
        ;;
    logs)
        get_logs
        ;;
    *)
        echo "Usage: $0 [start|status|version|stop|shell|logs]"
        exit 1
        ;;
esac
