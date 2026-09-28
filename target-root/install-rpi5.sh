#!/usr/bin/env bash
# ==============================================================================
# install-rpi5.sh - Legato AF Target Runtime Installer for Raspberry Pi 5
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FORCE=0
PACKAGE_PATH=""

# Parse arguments
while [ $# -gt 0 ]; do
    case "$1" in
        --force)
            FORCE=1
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [--force] [system.rpi5.update | system.tar.gz]"
            echo ""
            echo "Installs Legato Application Framework on Raspberry Pi 5 (Raspberry Pi OS 64-bit)."
            echo "Options:"
            echo "  --force    Bypass AArch64 architecture validation check"
            echo "  --help     Display this help message"
            exit 0
            ;;
        *)
            PACKAGE_PATH="$1"
            shift
            ;;
    esac
done

echo "====================================================================="
echo " Installing Legato Application Framework on Raspberry Pi 5"
echo "====================================================================="

# 1. Privilege validation
if [ "$(id -u)" -ne 0 ]; then
    echo "ERROR: This script must be run as root. Run with sudo: sudo $0" >&2
    exit 1
fi

# 2. Architecture validation
ARCH="$(uname -m)"
if [ "$ARCH" != "aarch64" ] && [ "$FORCE" -eq 0 ]; then
    echo "ERROR: Detected architecture '$ARCH'. Raspberry Pi 5 requires 'aarch64'." >&2
    echo "Use --force to install anyway." >&2
    exit 1
fi

# 3. Create required system directories
echo "==> Creating filesystem layout..."
mkdir -p /opt/legato/bin
mkdir -p /opt/legato/systems/0
mkdir -p /opt/legato/apps
mkdir -p /var/run/legato
chmod 1777 /var/run/legato
mkdir -p /tmp/legato
chmod 1777 /tmp/legato
mkdir -p /var/config
chmod 755 /var/config

# Backwards compatibility directories for Legato internal daemons
mkdir -p /mnt/flash/legato
mkdir -p /mnt/flash/home
mkdir -p /mnt/legato

if [ ! -e /legato ]; then
    ln -sf /opt/legato /legato
fi

# 4. Extract update package / system if provided
if [ -z "$PACKAGE_PATH" ]; then
    if [ -f "${SCRIPT_DIR}/system.rpi5.update" ]; then
        PACKAGE_PATH="${SCRIPT_DIR}/system.rpi5.update"
    elif [ -f "/tmp/system.rpi5.update" ]; then
        PACKAGE_PATH="/tmp/system.rpi5.update"
    fi
fi

if [ -n "$PACKAGE_PATH" ] && [ -f "$PACKAGE_PATH" ]; then
    echo "==> Installing system package: ${PACKAGE_PATH}..."
    if grep -q -a "BZh" "$PACKAGE_PATH" 2>/dev/null; then
        OFFSET=$(grep -b -a -o "BZh" "$PACKAGE_PATH" | head -n 1 | cut -d: -f1)
        tail -c +"$((OFFSET + 1))" "$PACKAGE_PATH" | tar -xjf - -C /opt/legato/systems/0/
    elif file "$PACKAGE_PATH" | grep -q "gzip compressed"; then
        tar -xzf "$PACKAGE_PATH" -C /opt/legato/systems/0/
    elif file "$PACKAGE_PATH" | grep -q "bzip2 compressed"; then
        tar -xjf "$PACKAGE_PATH" -C /opt/legato/systems/0/
    else
        echo "WARNING: Unrecognized package format. Skipping auto-extraction." >&2
    fi
fi

# Initialize index and status if system files are present
if [ -d /opt/legato/systems/0/bin ]; then
    echo "0" > /opt/legato/systems/0/index
    echo "good" > /opt/legato/systems/0/status
    ln -sfn 0 /opt/legato/systems/current
    ln -sfn systems/current /opt/legato/current
fi

# 5. Install runtime startup / shutdown scripts
echo "==> Installing Legato runtime scripts..."
if [ -f "${SCRIPT_DIR}/bin/startLegato" ]; then
    cp -f "${SCRIPT_DIR}/bin/startLegato" /opt/legato/bin/startLegato
    chmod +x /opt/legato/bin/startLegato
fi

if [ -f "${SCRIPT_DIR}/bin/stopLegato" ]; then
    cp -f "${SCRIPT_DIR}/bin/stopLegato" /opt/legato/bin/stopLegato
    chmod +x /opt/legato/bin/stopLegato
fi

if [ -d /opt/legato/current/bin ]; then
    ln -sf /opt/legato/bin/startLegato /opt/legato/current/bin/startLegato 2>/dev/null || true
    ln -sf /opt/legato/bin/stopLegato /opt/legato/current/bin/stopLegato 2>/dev/null || true
fi

# 6. Configure dynamic linker (ld.so)
echo "==> Configuring shared libraries..."
cat << 'EOF' > /etc/ld.so.conf.d/legato.conf
/opt/legato/current/lib
/legato/systems/current/lib
EOF
ldconfig 2>/dev/null || true

# 7. Configure environment PATH
echo "==> Configuring system PATH and environment..."
cat << 'EOF' > /etc/profile.d/legato.sh
export LEGATO_ROOT=/opt/legato/current
export PATH=/opt/legato/current/bin:/legato/systems/current/bin:$PATH
EOF
chmod 644 /etc/profile.d/legato.sh

# Symlink standard CLI tools into /usr/local/bin
TOOLS="app config sdir log legato update inspect xattr"
for tool in $TOOLS; do
    if [ -x "/opt/legato/current/bin/${tool}" ]; then
        ln -sf "/opt/legato/current/bin/${tool}" "/usr/local/bin/${tool}"
    fi
done

# 8. Install and enable systemd unit
echo "==> Configuring systemd service..."
SERVICE_FILE="${SCRIPT_DIR}/systemd/legato.service"
if [ -f "$SERVICE_FILE" ]; then
    cp -f "$SERVICE_FILE" /etc/systemd/system/legato.service
    chmod 644 /etc/systemd/system/legato.service
    systemctl daemon-reload
    systemctl enable legato.service || true
    echo "==> Legato service enabled successfully."
fi

echo "====================================================================="
echo " Installation Complete!"
echo " Start Legato with: sudo systemctl start legato"
echo " Check status with: legato status  (or: systemctl status legato)"
echo "====================================================================="
