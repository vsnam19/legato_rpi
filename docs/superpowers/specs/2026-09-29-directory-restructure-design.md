# Directory Restructure & Source Simplification Specification (TelAF Simulation)

## 1. Context & Goals

This branch (`feat/telaf-simulation`) is dedicated exclusively to **Qualcomm TelAF Simulation**.
All legacy Raspberry Pi 5 platform files, intermediate rate-limit cloning workarounds, and unused sample scripts are removed. The repository is organized strictly into a clean, modular TelAF architecture:

1. **Pure TelAF Simulation Target**: Build system and scripts target `simulation` (`mkapp -t simulation`) running on `telaf_simulation_runtime_2204:1.0.0`.
2. **Modular Components**: The 5 CfgManager subsystems are placed under `components/cfgManager/`.
3. **Streamlined Scripts**:
   - `scripts/build.sh`: Builds `cfgManager` and `cfgClient` packages for simulation.
   - `scripts/deploy.sh`: Deploys packages to the running simulation container and displays live verification logs.
   - `scripts/run-simulation.sh`: Manages the TelAF container lifecycle (`start`, `stop`, `status`, `shell`).
   - `scripts/run-unit-tests.sh`: Executes the Google Test C++23 test suite with coverage.
4. **Clean Test Suite**: `tests/` contains only Google Test C++23 suites; Python cloner tests are eliminated.

---

## 2. Target Directory Layout

```text
legato_rpi/  (Branch: feat/telaf-simulation)
├── components/                       # Business logic C++23 components
│   └── cfgManager/
│       ├── core/                     # cfgId, cfgRouter, cfgManagerService
│       │   ├── include/
│       │   └── src/
│       ├── crypto/                   # AES-256-GCM crypto engine
│       │   ├── include/
│       │   └── src/
│       ├── security/                 # TrustZone / OP-TEE / Simulated Enclave HAL
│       │   ├── include/
│       │   └── src/
│       ├── storage/                  # configTree & in-memory backend
│       │   ├── include/
│       │   └── src/
│       └── events/                   # Event dispatcher
│           ├── include/
│           └── src/
│
├── apps/                             # Legato/TelAF Application Services
│   └── cfgManager/                   # CfgManager daemon package
│       ├── cfgManager.adef
│       └── server/                   # Server component (Component.cdef, cfgManagerServer.cpp)
│
├── samples/                          # Sample Applications
│   └── cfgClient/                    # CfgManager client demo (.adef, clientComponent)
│
├── interfaces/                       # IPC API Definitions (.api)
│   └── cfgManager.api
│
├── scripts/                          # TelAF Simulation Tooling
│   ├── build.sh                      # Builds cfgManager & cfgClient for simulation
│   ├── deploy.sh                     # Deploys & verifies on simulation container
│   ├── run-simulation.sh             # Manages container lifecycle (start|stop|status|shell)
│   └── run-unit-tests.sh             # Google Test runner & C2 coverage
│
├── tests/                            # C++23 Google Test Suites
│   ├── CMakeLists.txt
│   ├── test_main.cpp
│   ├── core/
│   ├── crypto/
│   ├── security/
│   ├── storage/
│   ├── events/
│   └── service/
│
├── docs/                             # Architecture & Simulation Guides
│   ├── telaf_simulation.md
│   ├── CFG_MANAGER.md
│   └── superpowers/
│
└── README.md                         # Project documentation tailored for TelAF Simulation
```

---

## 3. Detailed Actions

### 3.1 Removals
- **RPi5 Platform Files**:
  - `targets/`
  - `target-root/`
  - `toolchain/`
  - `patches/`
  - `docker/`
  - `samples/helloWorld/`
- **RPi5 & Obsolete Scripts**:
  - `scripts/build.sh` (re-implemented for TelAF sim)
  - `scripts/deploy.sh` (re-implemented for TelAF sim)
  - `scripts/internal-build.sh`
  - `scripts/build-sample.sh`
  - `scripts/build-cfgmanager.sh`
  - `scripts/build-sample-client.sh`
  - `scripts/setup_submodule.sh`
- **Rate-Limit & Cloner Files**:
  - `scripts/telaf_simulation/` (and all its contents: `clone_telaf_repos.py`, `setup_simulation.sh`, etc.)
  - `scripts/setup-telaf-simulation.sh`
  - `tests/python/` (and `test_telaf_cloner.py`)

### 3.2 Relocations (Git Move)
- `core/` -> `components/cfgManager/core/`
- `crypto/` -> `components/cfgManager/crypto/`
- `security/` -> `components/cfgManager/security/`
- `storage/` -> `components/cfgManager/storage/`
- `events/` -> `components/cfgManager/events/`

### 3.3 New/Updated Scripts
- `scripts/build.sh`: Replaces old RPi build script with the TelAF simulation build logic (from `build-cfgmanager-sim.sh`).
- `scripts/deploy.sh`: Replaces old RPi deploy script with the TelAF simulation deploy logic (from `deploy-cfgmanager-sim.sh`).
- `scripts/run-simulation.sh`: Directly wraps container lifecycle (`scripts/telaf_simulation/run_simulation.sh` relocated to `scripts/run-simulation.sh`).
- Remove redundant `scripts/build-cfgmanager-sim.sh` and `scripts/deploy-cfgmanager-sim.sh`.

### 3.4 Code & Configuration Fixes
- `apps/cfgManager/server/Component.cdef`: Update paths to `${PROJECT_ROOT}/components/cfgManager/...`.
- `samples/cfgClient/clientComponent/Component.cdef`: Update include to `${PROJECT_ROOT}/components/cfgManager/core/include`.
- `tests/CMakeLists.txt`: Update include and source globs to `../components/cfgManager/...`.
- `scripts/run-unit-tests.sh`: Update gcov coverage filters to `/workspace/components/cfgManager/...`.

---

## 4. Acceptance Criteria

1. Google Test suite (`./scripts/run-unit-tests.sh`): **45/45 PASS**.
2. TelAF Build (`./scripts/build.sh`): Builds `apps/cfgManager/cfgManager.simulation.update` and `samples/cfgClient/cfgClient.simulation.update` successfully.
3. TelAF Deploy (`./scripts/deploy.sh`): Installs on `telaf_simulation_runtime_2204_m`, restarts `cfgClient`, and prints live syslog confirming all operations passed.
4. Clean Git Status: No untracked junk, no dead RPi5 or rate-limit files remaining.
