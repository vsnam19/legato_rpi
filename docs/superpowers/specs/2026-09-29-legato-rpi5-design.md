# Design Specification: Legato Application Framework Port for Raspberry Pi 5

## 1. Executive Summary & Goals

This project provides a native port and build infrastructure for the **Legato Application Framework (AF)** targeting the **Raspberry Pi 5** (Broadcom BCM2712, Quad-core ARM Cortex-A76, 64-bit `aarch64`).

The primary objective is to enable fast, modular IoT application development and prototyping on standard **Raspberry Pi OS (64-bit Debian Bookworm)** without requiring a full Yocto/BitBake operating system rebuild. The framework is built directly from upstream Legato AF source via cross-compilation on an x86_64 Linux host, outputting standard Legato system update bundles (`system.rpi5.update`) deployable over the network via Legato's `instsys` tool and managed by `systemd`.

---

## 2. Requirements & Constraints

### 2.1 Hardware & Operating System
- **Target Hardware**: Raspberry Pi 5 (BCM2712, 4GB/8GB/16GB RAM, AArch64).
- **Target Operating System**: Official Raspberry Pi OS 64-bit (Debian Bookworm, kernel 6.6+, glibc 2.36+).
- **Host Build Platform & Containerized Environment**:
  - Host OS: Linux x86_64 with Docker installed (`docker`).
  - Container Base Image: `debian:bookworm` (matching Raspberry Pi OS glibc 2.36 runtime exactly).
  - Container Toolchain:
    - Host compiler: `gcc`, `g++`, `ninja-build`, `cmake` (>= 3.25)
    - Cross compiler: `gcc-aarch64-linux-gnu`, `g++-aarch64-linux-gnu`, `binutils-aarch64-linux-gnu`
    - Dependencies: `git`, `python3`, `pkg-config`, `libssl-dev`, `libcurl4-openssl-dev`, `libjansson-dev`

### 2.2 Framework Scope
- **Included Core Framework**:
  - `serviceDirectory`: Central IPC discovery and binding daemon.
  - `configTree`: Hierarchical transactional configuration database.
  - `logDaemon`: Centralized structured logging and control (`logread`, `logctrl`).
  - `supervisor`: Process watchdog, cgroup resource constraints, auto-recovery.
  - `updateDaemon`: Package installation, atomic system slot switching, and rollback.
  - `tools`: Standard CLI utilities (`app`, `sdir`, `config`, `legato`, `sbtrace`).
  - `devMode`: Interactive debugging, memory inspection, and tracing utilities.
- **Excluded Features**:
  - Cellular modem services (`modemService`, `cellNetService`, `smsInboxService`, `voiceCallService`), QMI/MBIM, and Sierra-specific baseband drivers.

---

## 3. Architecture & Repository Structure

The project adopts an **Out-of-Tree Platform Overlay & Submodule** architecture. The upstream Legato AF repository is maintained as a clean, unmodified git submodule. All Raspberry Pi 5 definitions, toolchains, build scripts, systemd configurations, and sample apps reside in the top-level repository.

### 3.1 Directory Layout
```
legato_rpi/
├── submodules/
│   └── legato-af/              # Git submodule: upstream Legato AF repository
├── docker/
│   ├── Dockerfile              # Debian Bookworm AArch64 cross-build container definition
│   └── run-docker-build.sh     # Script to build container image and run compilation
├── targets/
│   ├── rpi5.sdef               # System definition for Raspberry Pi 5
│   └── rpi5.sinc               # Target-specific build variables and PA mappings
├── toolchain/
│   ├── toolchain.rpi5.cmake    # CMake cross-compilation flags and target triple
│   └── env.sh                  # Host environment variables (CC, CXX, SYSROOT, PATH)
├── scripts/
│   ├── setup_submodule.sh      # Submodule checkout and dependency initialization
│   ├── build.sh                # Main build driver: host tools + rpi5 target cross-build
│   └── deploy.sh               # Network/SSH deployment tool (bootstrap + instsys)
├── target-root/
│   ├── systemd/
│   │   └── legato.service      # Systemd service unit for Raspberry Pi OS
│   └── install-rpi5.sh         # Target-side bootstrap installer script
├── samples/
│   └── helloWorld/             # Verification app for validating IPC & logging
│       ├── helloWorld.adef
│       └── helloComp/
│           ├── Component.cdef
│           └── hello.c
└── docs/
    └── superpowers/specs/      # Architectural and design specifications
```

---

## 4. Build System & Target Definition

### 4.1 Target System Definition (`targets/rpi5.sdef`)
The `rpi5.sdef` defines the root system bundle:
```ini
// Raspberry Pi 5 Core System Definition
#include "$LEGATO_ROOT/legatoTargetConfig.sinc"
#include "$LEGATO_RPI_ROOT/targets/rpi5.sinc"

apps:
{
    // Command-line tools and utilities
    $LEGATO_ROOT/apps/tools/tools

    // Development utilities (sbtrace, inspect, gdbserver hooks)
    $LEGATO_ROOT/apps/tools/devMode
}

commands:
{
    app = tools:/bin/app
    sdir = tools:/bin/sdir
    config = tools:/bin/config
    log = tools:/bin/log
}
```

### 4.2 Target Include (`targets/rpi5.sinc`)
```ini
buildVars:
{
    LEGATO_TARGET = rpi5
    LEGATO_DCS_PA = ${PA_DIR}/dcs/linux/components/le_pa_dcs
    LEGATO_CLOCKSYNC_PA = ${PA_DIR}/clock/linux/components/le_pa_clockSync
}
```

### 4.3 Toolchain Configuration (`toolchain/env.sh` & `toolchain/toolchain.rpi5.cmake`)
- **Target Triple**: `aarch64-linux-gnu`
- **CPU Architecture Flags**: `-march=armv8-a+crc+crypto -mtune=cortex-a76`
- **Optimization**: `-O2 -g` (or `-O3` for Release)
- **Environment Exports**:
  ```bash
  export LEGATO_ROOT="${REPO_ROOT}/submodules/legato-af"
  export LEGATO_TARGET="rpi5"
  export CC="aarch64-linux-gnu-gcc"
  export CXX="aarch64-linux-gnu-g++"
  export AR="aarch64-linux-gnu-ar"
  export STRIP="aarch64-linux-gnu-strip"
  export PATH="${LEGATO_ROOT}/bin:${PATH}"
  ```

### 4.4 Build Workflow (`scripts/build.sh`)
The build process operates in two clear phases:
1. **Host Build**: Builds native x86_64 host tools (`mksys`, `mkapp`, `mkexe`, `mkcomp`, `ifgen`) using host GCC and Ninja inside `${LEGATO_ROOT}/build/tools`.
2. **Target Cross-Build**: Runs `mksys` targeting `rpi5` with cross-compilation flags to build `liblegato.so`, daemons (`serviceDirectory`, `configTree`, `logDaemon`, `supervisor`, `updateDaemon`), and user apps, generating `${REPO_ROOT}/build/rpi5/system.rpi5.update`.

---

## 5. Runtime Lifecycle, Sandboxing, & System Integration

### 5.1 Target Filesystem Hierarchy
On the Raspberry Pi 5, Legato is structured under `/opt/legato`:
- `/opt/legato/systems/`: Unpacked system versions (e.g. `current`, `previous`, `system.unpack.*`).
- `/opt/legato/current`: Symlink to the running system directory.
- `/opt/legato/apps/`: Application installations.
- `/var/run/legato/`: Unix domain sockets for `le_ipc`, PID files, and runtime lock files (mounted on `tmpfs`).
- `/var/config/`: Persistent storage containing the `configTree` database.

### 5.2 Systemd Integration (`target-root/systemd/legato.service`)
```ini
[Unit]
Description=Legato Application Framework
After=network.target local-fs.target
Wants=network.target

[Service]
Type=simple
Environment="LEGATO_ROOT=/opt/legato/current"
ExecStart=/opt/legato/current/bin/startLegato
ExecStop=/opt/legato/current/bin/stopLegato
Restart=always
RestartSec=5
LimitNOFILE=65536

[Install]
WantedBy=multi-user.target
```

### 5.3 Sandboxing & Cgroups
- **Process Isolation**: Legato applications with `sandboxed: true` execute in isolated mount, PID, and network namespaces.
- **Hardware Access**: System utilities or hardware controllers can specify `sandboxed: false` to access raw `/dev/gpiochip*`, `/dev/i2c-*`, or `/dev/spidev*` devices directly.
- **Cgroups**: RPi OS 64-bit kernel natively supports cgroups v2 for memory caps and CPU quotas managed by the Legato `supervisor`.
- **Fault Recovery**: Process crashes trigger supervisor actions configured in `.adef` (`restart`, `reboot`, or `stopApp`).

---

## 6. Deployment Pipeline

### 6.1 Initial Target Bootstrap (`scripts/deploy.sh --bootstrap <RPi_IP>`)
1. Connects to the Raspberry Pi over SSH.
2. Ensures required target dependencies are installed (`libssl3`, `libcurl4`, `iptables`).
3. Creates directory skeleton (`/opt/legato`, `/var/run/legato`, `/var/config`).
4. Copies base framework binaries and registers `/etc/systemd/system/legato.service`.
5. Enables and starts the `legato` systemd service.

### 6.2 System & App Updates (`scripts/deploy.sh <RPi_IP>`)
1. Uses Legato's `instsys` command:
   ```bash
   instsys build/rpi5/system.rpi5.update <RPi_IP>
   ```
2. The on-device `updateDaemon` verifies package signature/integrity, installs files to a new version slot under `/opt/legato/systems/`, updates the `/opt/legato/current` symlink, and restarts the framework daemons.
3. If the new system fails to initialize within the watchdog timeout, `supervisor` automatically rolls back `/opt/legato/current` to the previous known good system.

---

## 7. Verification & Testing Strategy

### 7.1 Host Build Verification
- Confirm host tools build cleanly (`mksys --version`, `mkapp --version`).
- Verify target binary architecture:
  ```bash
  readelf -h build/rpi5/bin/serviceDirectory | grep -E "Class|Machine"
  # Expected: ELF64, AArch64
  ```
- Confirm output artifact `build/rpi5/system.rpi5.update` exists and is non-empty.

### 7.2 Target Runtime Verification
- System service check: `systemctl status legato` (`active (running)`).
- Framework status: `legato status` (`Legato: RUNNING`).
- Core service registry: `sdir list` (verifies `configTree`, `logDaemon`, `updateDaemon`).
- Functional test: Build and install `samples/helloWorld`:
  ```bash
  mkapp -t rpi5 samples/helloWorld/helloWorld.adef
  app install helloWorld.rpi5.update <RPi_IP>
  logread -f | grep "Hello, world"
  ```
