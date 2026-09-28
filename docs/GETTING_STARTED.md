# Getting Started with Legato AF on Raspberry Pi 5

This guide provides step-by-step instructions to compile, deploy, and run the [Legato Application Framework](https://github.com/legatoproject/legato-af) on the **Raspberry Pi 5** (ARM64 / AArch64) using a reproducible Docker containerized build environment.

---

## 1. Prerequisites

### Host System Requirements
- **Operating System:** Linux (Ubuntu 20.04+, Debian 11+, Fedora, etc.)
- **Docker:** Docker CE 20.10+ installed and runnable without sudo (`docker ps` works)
- **Git:** Git 2.25+

### Target Hardware Requirements
- **Device:** Raspberry Pi 5 (4GB / 8GB / 16GB)
- **Target OS:** Raspberry Pi OS 64-bit (Debian 12 Bookworm, kernel 6.6+, glibc 2.36)
- **Network:** Raspberry Pi 5 connected to local network with SSH enabled (`sudo raspi-config` -> Interface Options -> SSH)

---

## 2. Repository Setup

Clone this repository and initialize the upstream Legato AF submodule:

```bash
git clone --recurse-submodules <repo_url> legato_rpi
cd legato_rpi

# Alternatively, initialize existing clone:
bash scripts/setup_submodule.sh
```

The setup script checks the submodule status and fetches external build dependencies (such as `Kconfiglib`).

---

## 3. Building Legato for Raspberry Pi 5

All builds are fully containerized using Docker to eliminate host dependency conflicts (such as Python 2.7 / Jinja2 / multiarch glibc toolchains).

To build the complete Legato system update package and framework daemons:

```bash
bash scripts/build.sh
```

### What Happens During the Build:
1. `docker/run-docker-build.sh` automatically builds or uses the cached `legato-rpi5-builder` Docker image (based on `debian:bookworm-slim`).
2. Applies necessary target patches (`patches/0001-add-rpi5-target-support.patch`) to the upstream framework.
3. Compiles the Legato host tools (`mksys`, `mkapp`, `mkcomp`, `ifgen`, `setconfig`).
4. Cross-compiles the target binaries for AArch64 (`-march=armv8-a+crc+crypto -mtune=cortex-a76`):
   - `serviceDirectory` (IPC routing)
   - `logCtrlDaemon` (logging daemon)
   - `configTree` (configuration tree database)
   - `updateDaemon` (over-the-air package update manager)
   - `watchdog` (heartbeat monitor with `/dev/watchdog` support)
   - `supervisor` (process and sandbox manager)
   - Command-line utilities: `app`, `config`, `sdir`, `log`, `legato`, `update`, `inspect`, `xattr`.
5. Outputs the final deliverables into `build/rpi5/`:
   - `build/rpi5/system.rpi5.update` (Self-contained Legato system image)
   - `build/rpi5/framework/bin/` (Target executables)
   - `build/rpi5/framework/lib/` (Target shared libraries)

---

## 4. Deploying to Raspberry Pi 5

### Method A: Automated Network Deployment (Recommended)

Ensure you know your Raspberry Pi 5's IP address (e.g., `192.168.1.100`) and username (default is `pi`):

```bash
# Bootstrap target over SSH:
bash scripts/deploy.sh --bootstrap 192.168.1.100 pi
```

The bootstrap script will:
1. Test SSH connectivity to the Raspberry Pi.
2. Transfer `system.rpi5.update` and target installation scripts.
3. Run `target-root/install-rpi5.sh` as root on the target.
4. Set up `/opt/legato` and `/legato` filesystem layout.
5. Configure systemd unit `legato.service` and enable autostart on boot.
6. Start the Legato framework and display initial service status.

### Method B: Manual Target Installation

If you prefer to install manually or copy via USB drive:

1. Copy `build/rpi5/system.rpi5.update` and the `target-root/` directory to your Raspberry Pi:
   ```bash
   scp build/rpi5/system.rpi5.update pi@192.168.1.100:/tmp/
   scp -r target-root pi@192.168.1.100:/tmp/
   ```

2. SSH into your Raspberry Pi 5:
   ```bash
   ssh pi@192.168.1.100
   ```

3. Run the installer:
   ```bash
   sudo /tmp/target-root/install-rpi5.sh /tmp/system.rpi5.update
   ```

4. Start Legato:
   ```bash
   sudo systemctl start legato
   ```

---

## 5. Controlling Legato on the Target

Log in to the Raspberry Pi 5 via SSH or serial console:

### Check Framework Status
```bash
legato status
```
Output shows whether the supervisor is running and lists installed systems.

### Inspect Registered Services
```bash
sdir list
```
Displays all registered Legato IPC service endpoints (such as `logClient`, `configTree`, `updateDaemon`, etc.).

### Systemd Integration
```bash
# Check service status
sudo systemctl status legato

# Stop Legato framework
sudo systemctl stop legato

# Restart Legato framework
sudo systemctl restart legato
```

### Viewing Logs
```bash
# Live framework daemon logs via systemd
journalctl -u legato -f

# Legato log command
log read
```

---

## 6. Developing and Running Legato Applications

A sample application (`samples/helloWorld`) is provided to verify application building, packaging, and execution.

### Building the Sample App
Run the sample build script:

```bash
bash scripts/build-sample.sh helloWorld
```
This generates `build/rpi5/apps/helloWorld.rpi5.update`.

### Deploying the App to Raspberry Pi 5
Use the deployment tool to stream the update to the running Legato instance:

```bash
bash scripts/deploy.sh --update 192.168.1.100 build/rpi5/apps/helloWorld.rpi5.update
```

### Monitoring the App
On the Raspberry Pi:
```bash
# Check app status
app status

# View log messages output by helloWorld
log read | grep helloWorld
```
Expected log output:
```text
INFO | helloWorld[1234]/helloComp_exe << Hello, Raspberry Pi 5! >>
INFO | helloWorld[1234]/helloComp_exe << Legato Application Framework is running smoothly on AArch64. >>
```

---

## 7. Troubleshooting

- **Architecture Mismatch:** If `install-rpi5.sh` warns about architecture, verify that you are running 64-bit Raspberry Pi OS (`uname -m` must return `aarch64`). Legato for RPi 5 is built specifically for 64-bit ARM.
- **IPC Socket Permissions:** Legato IPC creates Unix domain sockets in `/var/run/legato` and `/tmp/legato`. These directories must have permission `1777` (handled automatically by `install-rpi5.sh` and `startLegato`).
- **Sysctl Warnings:** Legato requires larger socket buffers for IPC transfers. `startLegato` configures:
  ```bash
  sysctl -w net.unix.max_dgram_qlen=32
  sysctl -w net.core.wmem_max=266240
  sysctl -w net.core.rmem_max=266240
  ```
