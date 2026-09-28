# Legato AF Port for Raspberry Pi 5 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a containerized cross-compilation system and target overlay for running Legato Application Framework core services on Raspberry Pi 5 with Raspberry Pi OS (64-bit Debian Bookworm).

**Architecture:** The project maintains upstream Legato AF as an unmodified git submodule (`submodules/legato-af`). Target definitions (`targets/rpi5.sdef`), toolchain configuration, and systemd files reside out-of-tree in this repository. All compilation occurs inside a reproducible Docker container based on `debian:bookworm` matching the target glibc version.

**Tech Stack:** C, Bash, Make, CMake, Ninja, Docker (`debian:bookworm`), GNU Toolchain (`aarch64-linux-gnu-gcc`), Systemd.

**Spec:** `docs/superpowers/specs/2026-09-29-legato-rpi5-design.md`

## Global Constraints

- Target CPU: Broadcom BCM2712 (Cortex-A76, AArch64, `-march=armv8-a+crc+crypto -mtune=cortex-a76`).
- Target OS: Raspberry Pi OS 64-bit Debian Bookworm (glibc 2.36).
- Build Environment: Containerized via Docker using `debian:bookworm` base image.
- Scope: Minimal Core Framework only (`serviceDirectory`, `configTree`, `logDaemon`, `supervisor`, `updateDaemon`, `tools`, `devMode`), excluding cellular/modem services.
- Target Install Root: `/opt/legato` with runtime state in `/var/run/legato` and `/var/config`.

## Review Focus

- Host vs Container Path Mapping: File paths passed to Legato build tools must be relative or properly resolved inside the container mount point (`/workspace`).
- Cross-Compiler Compatibility: Target binaries must be verified as ELF 64-bit LSB pie executable, ARM aarch64.
- Submodule Initialization: Build scripts must handle uninitialized or shallow submodules cleanly.
- Error Handling in Build Script: Any compiler or ninja failure must exit with a non-zero exit code immediately (`set -e`).
- Systemd Service Environment: `legato.service` must define `LEGATO_ROOT=/opt/legato/current` and appropriate runtime limits (`LimitNOFILE=65536`).

---

### Task 1: Submodule Initialization and Repository Infrastructure

**Files:**
- Create: `scripts/setup_submodule.sh`
- Create: `.gitmodules`
- Create: `.gitignore`

**Interfaces:**
- Produces: Checked-out `submodules/legato-af` directory containing upstream build system.

- [ ] **Step 1: Create `.gitignore`**
  Ignore build outputs, temp files, and docker intermediate artifacts:
  ```gitignore
  build/
  *.update
  *.tar.bz2
  *.log
  .config.*
  ```

- [ ] **Step 2: Create `scripts/setup_submodule.sh`**
  Script to clone or initialize `submodules/legato-af` pointing to `https://github.com/legatoproject/legato-af.git` with depth 1 or specified release tag (`master`).

- [ ] **Step 3: Run `scripts/setup_submodule.sh` to initialize submodule**
  Run: `bash scripts/setup_submodule.sh`
  Expected: `submodules/legato-af/Makefile` and `submodules/legato-af/CMakeLists.txt` exist.

- [ ] **Step 4: Commit**
  ```bash
  git add .gitignore .gitmodules scripts/setup_submodule.sh submodules/legato-af
  git commit -m "feat: initialize legato-af submodule and setup script"
  ```

---

### Task 2: Docker Debian Bookworm Cross-Compilation Environment

**Files:**
- Create: `docker/Dockerfile`
- Create: `docker/run-docker-build.sh`

**Interfaces:**
- Consumes: Host Docker daemon
- Produces: Docker image `legato-rpi5-builder:latest` with complete AArch64 cross-toolchain and host build utilities matching Debian Bookworm.

- [ ] **Step 1: Write `docker/Dockerfile`**
  Base on `debian:bookworm-slim`. Install:
  - `build-essential`, `gcc`, `g++`, `clang`
  - `gcc-aarch64-linux-gnu`, `g++-aarch64-linux-gnu`, `binutils-aarch64-linux-gnu`
  - `cmake`, `ninja-build`, `git`, `python3`, `pkg-config`, `bison`, `flex`
  - `libssl-dev`, `libcurl4-openssl-dev`, `libjansson-dev`
  - Set working directory to `/workspace`.

- [ ] **Step 2: Write `docker/run-docker-build.sh`**
  Builds the Docker image if not present or if Dockerfile changed, then executes the specified build command inside the container with the current repository mounted at `/workspace` and running as current UID:GID.

- [ ] **Step 3: Verify Docker image build and cross-compiler invocation**
  Run: `bash docker/run-docker-build.sh aarch64-linux-gnu-gcc --version`
  Expected: Outputs `aarch64-linux-gnu-gcc (Debian 12.2.0-...)` and exits with code 0.

- [ ] **Step 4: Commit**
  ```bash
  git add docker/Dockerfile docker/run-docker-build.sh
  git commit -m "feat: add debian bookworm aarch64 cross-build docker environment"
  ```

---

### Task 3: Raspberry Pi 5 Target System Definition and Include Files

**Files:**
- Create: `targets/rpi5.sdef`
- Create: `targets/rpi5.sinc`
- Create: `toolchain/env.sh`
- Create: `toolchain/toolchain.rpi5.cmake`

**Interfaces:**
- Produces: Legato target definitions for `rpi5` specifying core services (`tools`, `devMode`), platform adapters, and compiler optimization flags.

- [ ] **Step 1: Write `targets/rpi5.sinc`**
  Define `buildVars` for target `rpi5`, specifying Linux platform adapters (`le_pa_dcs`, `le_pa_clockSync`).

- [ ] **Step 2: Write `targets/rpi5.sdef`**
  Include `legatoTargetConfig.sinc`, `rpi5.sinc`, tools app (`$LEGATO_ROOT/apps/tools/tools`), devMode app (`$LEGATO_ROOT/apps/tools/devMode`), and CLI command bindings (`app`, `sdir`, `config`, `log`).

- [ ] **Step 3: Write `toolchain/toolchain.rpi5.cmake` and `toolchain/env.sh`**
  Set `CMAKE_SYSTEM_NAME Linux`, `CMAKE_SYSTEM_PROCESSOR aarch64`, target flags `-march=armv8-a+crc+crypto -mtune=cortex-a76`, and export `LEGATO_TARGET=rpi5`, `CC=aarch64-linux-gnu-gcc`, `CXX=aarch64-linux-gnu-g++`.

- [ ] **Step 4: Verify syntax and environment script**
  Run: `bash -c "source toolchain/env.sh && [ \"\$LEGATO_TARGET\" = \"rpi5\" ]"`
  Expected: Exits 0.

- [ ] **Step 5: Commit**
  ```bash
  git add targets/ toolchain/
  git commit -m "feat: add rpi5 target definition, platform config, and toolchain scripts"
  ```

---

### Task 4: Build Orchestration and Host Tools Compilation

**Files:**
- Create: `scripts/build.sh`
- Create: `scripts/internal-build.sh`

**Interfaces:**
- Consumes: `docker/run-docker-build.sh`, `submodules/legato-af`, `targets/rpi5.sdef`, `toolchain/env.sh`
- Produces: `build/rpi5/bin/serviceDirectory`, `build/rpi5/system.rpi5.update`

- [ ] **Step 1: Write `scripts/internal-build.sh`**
  Executed inside the container:
  1. Set up paths and export environment from `toolchain/env.sh`.
  2. Build native host tools (`make tools` inside `$LEGATO_ROOT` or CMake build for `bin/mksys`, `bin/mkapp`, `bin/ifgen`).
  3. Symlink / register `rpi5` target into Legato target directories.
  4. Invoke `mksys -t rpi5 targets/rpi5.sdef` to compile framework libraries, daemons, and apps.
  5. Package into `build/rpi5/system.rpi5.update`.

- [ ] **Step 2: Write `scripts/build.sh`**
  Host wrapper that invokes `docker/run-docker-build.sh /workspace/scripts/internal-build.sh "$@"`.

- [ ] **Step 3: Run build and verify output binaries**
  Run: `bash scripts/build.sh`
  Expected:
  - Build completes with exit code 0.
  - `build/rpi5/system.rpi5.update` exists.
  - `readelf -h build/rpi5/framework/bin/serviceDirectory` shows `AArch64`.

- [ ] **Step 4: Commit**
  ```bash
  git add scripts/build.sh scripts/internal-build.sh
  git commit -m "feat: add build orchestration scripts and containerized compilation"
  ```

---

### Task 5: Target Runtime Configuration, Systemd Service, and Installer

**Files:**
- Create: `target-root/systemd/legato.service`
- Create: `target-root/install-rpi5.sh`

**Interfaces:**
- Produces: Filesystem bootstrap script and systemd unit ready to install onto Raspberry Pi OS.

- [ ] **Step 1: Write `target-root/systemd/legato.service`**
  Systemd unit configured with `Environment="LEGATO_ROOT=/opt/legato/current"`, `ExecStart=/opt/legato/current/bin/startLegato`, `ExecStop=/opt/legato/current/bin/stopLegato`, `Restart=always`.

- [ ] **Step 2: Write `target-root/install-rpi5.sh`**
  Target-side installer script:
  - Validates running on Raspberry Pi OS 64-bit (`uname -m` == aarch64).
  - Creates directories `/opt/legato`, `/var/run/legato`, `/var/config`.
  - Installs `legato.service` into `/etc/systemd/system/`.
  - Reloads systemd daemon (`systemctl daemon-reload`) and enables service (`systemctl enable legato`).

- [ ] **Step 3: Validate bash syntax of `install-rpi5.sh` and systemd unit**
  Run: `bash -n target-root/install-rpi5.sh`
  Expected: Exits 0 with no syntax errors.

- [ ] **Step 4: Commit**
  ```bash
  git add target-root/
  git commit -m "feat: add target runtime systemd service and installer bootstrap"
  ```

---

### Task 6: Automated Network Deployment Script (`deploy.sh`)

**Files:**
- Create: `scripts/deploy.sh`

**Interfaces:**
- Consumes: `build/rpi5/system.rpi5.update`, `target-root/`, target IP/hostname over SSH.
- Produces: Fully deployed and running Legato system on remote Raspberry Pi 5.

- [ ] **Step 1: Write `scripts/deploy.sh`**
  Options:
  - `--bootstrap <TARGET_IP> [USER]`: Copies initial system tarball and runs `install-rpi5.sh` over SSH.
  - `<TARGET_IP> [USER]`: Pushes `build/rpi5/system.rpi5.update` via `instsys` or SSH update daemon.
  - `--status <TARGET_IP>`: Queries `legato status` and `sdir list` via SSH.

- [ ] **Step 2: Validate syntax and argument parsing of `deploy.sh`**
  Run: `bash scripts/deploy.sh --help`
  Expected: Displays usage information and exits 0.

- [ ] **Step 3: Commit**
  ```bash
  git add scripts/deploy.sh
  git commit -m "feat: add network deployment script for rpi5"
  ```

---

### Task 7: Sample Verification App (`samples/helloWorld`)

**Files:**
- Create: `samples/helloWorld/helloWorld.adef`
- Create: `samples/helloWorld/helloComp/Component.cdef`
- Create: `samples/helloWorld/helloComp/hello.c`
- Create: `scripts/build-sample.sh`

**Interfaces:**
- Produces: Validated Legato application `helloWorld.rpi5.update` to verify app compilation, IPC logging, and deployment.

- [ ] **Step 1: Write `samples/helloWorld/helloComp/hello.c` and `Component.cdef`**
  Component printing `LE_INFO("Hello, Raspberry Pi 5! Legato Application Framework is running.");` using `legato.h`.

- [ ] **Step 2: Write `samples/helloWorld/helloWorld.adef`**
  Application definition declaring the executable and `sandboxed: false` (or `sandboxed: true`).

- [ ] **Step 3: Write `scripts/build-sample.sh` and compile sample inside Docker**
  Run: `bash scripts/build-sample.sh helloWorld`
  Expected: Generates `build/rpi5/apps/helloWorld.rpi5.update`.

- [ ] **Step 4: Verify ELF architecture of sample binary**
  Run: `readelf -h build/rpi5/apps/helloWorld/bin/helloWorld | grep -E "Class|Machine"`
  Expected: `Class: ELF64`, `Machine: AArch64`.

- [ ] **Step 5: Commit**
  ```bash
  git add samples/ scripts/build-sample.sh
  git commit -m "feat: add helloWorld sample application and build script"
  ```

---

### Task 8: End-to-End Build and Verification Documentation

**Files:**
- Create: `README.md`
- Create: `docs/GETTING_STARTED.md`

**Interfaces:**
- Produces: Complete end-to-end documentation for building, deploying, testing, and developing Legato apps for Raspberry Pi 5.

- [ ] **Step 1: Write `docs/GETTING_STARTED.md`**
  Step-by-step guide: prerequisites, Docker setup, building Legato AF, setting up Raspberry Pi 5 with Raspberry Pi OS, deploying the framework, and running sample apps.

- [ ] **Step 2: Write `README.md`**
  Project overview, architecture diagram, quickstart commands, and repository structure.

- [ ] **Step 3: Verify all links and instructions**
  Ensure file paths in documentation match actual repository paths.

- [ ] **Step 4: Commit**
  ```bash
  git add README.md docs/GETTING_STARTED.md
  git commit -m "docs: add comprehensive README and getting started documentation"
  ```
