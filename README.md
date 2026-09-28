# Legato Application Framework for Raspberry Pi 5

[![Target](https://img.shields.io/badge/Target-Raspberry%20Pi%205%20(AArch64)-blue.svg)](#)
[![OS](https://img.shields.io/badge/OS-Raspberry%20Pi%20OS%20(Bookworm%2064--bit)-red.svg)](#)
[![Build](https://img.shields.io/badge/Build-Containerized%20(Docker)-green.svg)](#)

A containerized build system and target runtime port of the **[Legato Application Framework](https://github.com/legatoproject/legato-af)** for the **Raspberry Pi 5** (Broadcom BCM2712, Quad-core Cortex-A76, ARM64 / AArch64).

---

## Architecture Overview

```mermaid
flowchart TD
    subgraph Host["Host Machine (Docker Multiarch)"]
        subgraph DockerBuild["Docker Container (Debian Bookworm)"]
            HostTools["Legato Host Tools\n(mksys, mkapp, ifgen)"]
            AArch64GCC["AArch64 GCC 12 Cross-Compiler\n(-mtune=cortex-a76)"]
            Src["Legato AF Upstream\n+ RPi 5 Target Definitions & Patches"]
            HostTools --> SysBuild["Build System (make rpi5)"]
            AArch64GCC --> SysBuild
            Src --> SysBuild
            SysBuild --> ImgOut["build/rpi5/system.rpi5.update"]
            SysBuild --> SampleApp["build/rpi5/apps/helloWorld.rpi5.update"]
        end
    end

    subgraph Target["Target Device (Raspberry Pi 5 - 64-bit OS)"]
        Systemd["systemd (legato.service)"] --> StartScript["/opt/legato/current/bin/startLegato"]
        StartScript --> Sup["supervisor"]
        Sup --> Sdir["serviceDirectory (IPC Router)"]
        Sup --> LogD["logCtrlDaemon (Logging)"]
        Sup --> CfgTree["configTree (Configuration DB)"]
        Sup --> UpdD["updateDaemon (OTA Updates)"]
        Sup --> Wdog["watchdog (/dev/watchdog)"]
        Sup --> Apps["User Applications (helloWorld, etc.)"]
    end

    ImgOut -. Network Deploy (SSH / deploy.sh) .-> Target
    SampleApp -. App Update Stream .-> UpdD
```

---

## Features

- **Cortex-A76 Optimization:** Compiled with `-march=armv8-a+crc+crypto -mtune=cortex-a76` tailored for Raspberry Pi 5's BCM2712 SoC.
- **Minimal Core Scope:** Core framework daemons (`serviceDirectory`, `logCtrlDaemon`, `configTree`, `updateDaemon`, `watchdog`, `supervisor`) and developer CLI tools (`app`, `config`, `sdir`, `log`, `legato`, `update`, `inspect`, `xattr`). Modem/cellular modules are excluded for lightweight operation.
- **Hermetic Docker Build:** Builds entirely within an isolated `debian:bookworm` container matching Raspberry Pi OS's glibc 2.36 and library ecosystem.
- **Clean Submodule Design:** Keeps upstream Legato repository untouched in `submodules/legato-af/` with transparent target extensions and patches in `targets/` and `patches/`.
- **Production-Ready Target Integration:** Native systemd service unit (`legato.service`), kernel tuning (`startLegato`), automated installer (`install-rpi5.sh`), and network deployment tool (`deploy.sh`).
- **Validated Sample App:** Includes a standalone `helloWorld` application to verify the application build pipeline, IPC communication, and live logging.

---

## Repository Layout

```text
legato_rpi/
├── docker/
│   ├── Dockerfile                  # Debian Bookworm multiarch cross-compiler image
│   └── run-docker-build.sh         # Docker wrapper script with UID/GID forwarding
├── targets/
│   ├── rpi5.sdef                   # Legato system definition for Raspberry Pi 5
│   ├── rpi5.sinc                   # Target platform build variables and include rules
│   └── platformAdaptor/            # Linux watchdog platform adaptor for rpi5
├── toolchain/
│   ├── toolchain.rpi5.cmake        # CMake cross-compilation toolchain file
│   └── env.sh                      # Environment setup script for rpi5 toolchain
├── patches/
│   └── 0001-add-rpi5-target-support.patch # Upstream patches for AArch64 and rpi5 target
├── scripts/
│   ├── setup_submodule.sh          # Initializes submodules & Kconfiglib
│   ├── build.sh                    # Host wrapper to trigger containerized build
│   ├── internal-build.sh           # In-container compilation orchestration script
│   ├── build-sample.sh             # App build driver using mkapp
│   └── deploy.sh                   # SSH target bootstrap and network deployment tool
├── target-root/
│   ├── systemd/legato.service      # Systemd service unit for Raspberry Pi OS
│   ├── bin/startLegato             # Target startup script with kernel tuning
│   ├── bin/stopLegato              # Target shutdown script
│   └── install-rpi5.sh             # Target installer script
├── samples/
│   └── helloWorld/                 # Verification sample application
├── docs/
│   └── GETTING_STARTED.md          # Comprehensive step-by-step documentation
└── submodules/
    └── legato-af/                  # Upstream Legato Application Framework repository
```

---

## Quickstart

### 1. Build Legato Framework
Run the containerized build from the repository root:

```bash
bash scripts/build.sh
```

Upon completion, the system update archive is ready at `build/rpi5/system.rpi5.update`.

### 2. Deploy to Raspberry Pi 5
Bootstrap Legato onto a target Raspberry Pi 5 running 64-bit Raspberry Pi OS over SSH:

```bash
bash scripts/deploy.sh --bootstrap <TARGET_IP> [USER]
# Example: bash scripts/deploy.sh --bootstrap 192.168.1.100 pi
```

### 3. Check Target Status
Query the running Legato status from your host or on the target device:

```bash
# From host:
bash scripts/deploy.sh --status 192.168.1.100

# On the Raspberry Pi 5:
legato status
sdir list
```

### 4. Build and Run Applications

#### Hello World Sample:
```bash
bash scripts/build-sample.sh helloWorld
bash scripts/deploy.sh --update 192.168.1.100 build/rpi5/apps/helloWorld.rpi5.update
```

#### CfgManager Service (C++23 with TrustZone & AES-256-GCM):
```bash
# Build CfgManager service application:
bash scripts/build-cfgmanager.sh

# Deploy CfgManager to target:
bash scripts/deploy.sh --update 192.168.1.100 build/rpi5/apps/cfgManager.rpi5.update
```

#### CfgClient Sample Consumer App:
```bash
# Build CfgClient sample consumer:
bash scripts/build-sample-client.sh

# Deploy CfgClient to target:
bash scripts/deploy.sh --update 192.168.1.100 build/rpi5/apps/cfgClient.rpi5.update
```

### 5. Run C++23 Unit Tests & Coverage
Run the full Google Test suite with C2 branch coverage in Docker:

```bash
bash scripts/run-unit-tests.sh --coverage
```

For complete instructions and advanced options, see [docs/GETTING_STARTED.md](docs/GETTING_STARTED.md) and [docs/CFG_MANAGER.md](docs/CFG_MANAGER.md).

---

## Documentation Links

- [Getting Started Guide](docs/GETTING_STARTED.md)
- [CfgManager Service & Security Architecture](docs/CFG_MANAGER.md)
- [CfgManager Technical Design Specification](docs/superpowers/specs/2026-09-29-cfgmanager-design.md)
- [CfgManager Implementation Plan](docs/superpowers/plans/2026-09-29-cfgmanager-implementation.md)
- [Architecture & Design Specification (Port)](docs/superpowers/specs/2026-09-29-legato-rpi5-design.md)
- [Implementation Plan (Port)](docs/superpowers/plans/2026-09-29-legato-rpi5-implementation.md)

---

## License

The code and scripts in this repository are licensed under the Apache 2.0 / Mozilla Public License 2.0 consistent with the [Legato Application Framework](https://github.com/legatoproject/legato-af).

