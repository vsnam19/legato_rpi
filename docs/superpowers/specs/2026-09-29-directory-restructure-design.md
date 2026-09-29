# Directory Restructure & Source Simplification Specification

## 1. Context & Motivation

The repository previously accumulated files at the root level across multiple development phases:
1. The initial Raspberry Pi 5 Legato framework port (`targets/`, `target-root/`, `patches/`, `toolchain/`, `docker/`).
2. The `CfgManager` service components scattered at the root level (`core/`, `crypto/`, `security/`, `storage/`, `events/`).
3. HTTP 429 rate limit workaround scripts (`clone_telaf_repos.py`, `test_telaf_cloner.py`, `setup_simulation.sh`, `branch_mapping.conf`, `patch_me.json`), which are no longer needed as the environment is already prepared and only runtime/build/deploy sources are needed.
4. An unorganized `scripts/` directory mixing target Raspberry Pi 5 scripts and TelAF simulation scripts.

This refactoring reorganizes the repository into a modular Legato/TelAF architecture and strips out non-essential rate-limiting scripts.

---

## 2. Directory Layout: Target State

```text
legato_rpi/
├── components/                       # Reusable C++ business logic components
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
├── apps/                             # Legato application packages
│   └── cfgManager/                   # CfgManager daemon service
│       ├── cfgManager.adef
│       └── server/                   # Server component (Component.cdef, cfgManagerServer.cpp)
│
├── samples/                          # Sample & demonstration applications
│   ├── helloWorld/                   # Minimal Legato verification app
│   └── cfgClient/                    # CfgManager client demo (.adef, clientComponent)
│
├── interfaces/                       # IPC API definitions (.api)
│   └── cfgManager.api
│
├── platform/                         # Target hardware platform ports
│   └── rpi5/                         # Raspberry Pi 5 platform definitions
│       ├── targets/                  # rpi5.sdef, rpi5.sinc, platformAdaptor/
│       ├── target-root/              # systemd units, startup scripts, installer
│       ├── toolchain/                # toolchain.rpi5.cmake, env.sh
│       ├── docker/                   # Dockerfile, run-docker-build.sh
│       └── patches/                  # 0001-add-rpi5-target-support.patch
│
├── scripts/                          # Orchestration & automation scripts
│   ├── rpi5/                         # Raspberry Pi 5 build, deploy, samples
│   │   ├── build.sh
│   │   ├── internal-build.sh
│   │   ├── deploy.sh
│   │   ├── build-sample.sh
│   │   ├── build-cfgmanager.sh
│   │   └── build-sample-client.sh
│   ├── telaf/                        # Qualcomm TelAF simulation runtime & deploy
│   │   ├── build-cfgmanager-sim.sh
│   │   ├── deploy-cfgmanager-sim.sh
│   │   └── run-simulation.sh
│   ├── run-unit-tests.sh             # Google Test runner with C2 coverage
│   └── setup_submodule.sh            # Common submodule initialization
│
├── tests/                            # Google Test suites (pure C++23)
│   ├── CMakeLists.txt
│   ├── test_main.cpp
│   ├── core/
│   ├── crypto/
│   ├── security/
│   ├── storage/
│   ├── events/
│   └── service/
│
├── docs/                             # Documentation & specifications
├── submodules/                       # Upstream submodules (legato-af)
└── README.md                         # Main repository guide
```

---

## 3. Scope of Changes

### 3.1 Files to Remove (Rate-Limit & Non-Essential Utilities)
- `scripts/telaf_simulation/clone_telaf_repos.py`
- `scripts/telaf_simulation/setup_simulation.sh`
- `scripts/telaf_simulation/branch_mapping.conf`
- `scripts/telaf_simulation/patch_me.json`
- `scripts/setup-telaf-simulation.sh`
- `tests/python/test_telaf_cloner.py`
- Remove `tests/python/` and `scripts/telaf_simulation/`

### 3.2 Relocations via Git Move
1. **CfgManager Subsystems -> `components/cfgManager/`**:
   - `git mv core components/cfgManager/core`
   - `git mv crypto components/cfgManager/crypto`
   - `git mv security components/cfgManager/security`
   - `git mv storage components/cfgManager/storage`
   - `git mv events components/cfgManager/events`

2. **RPi5 Platform Files -> `platform/rpi5/`**:
   - `git mv targets platform/rpi5/targets`
   - `git mv target-root platform/rpi5/target-root`
   - `git mv toolchain platform/rpi5/toolchain`
   - `git mv patches platform/rpi5/patches`
   - `git mv docker platform/rpi5/docker`

3. **Scripts Organization**:
   - `scripts/rpi5/`: `build.sh`, `internal-build.sh`, `deploy.sh`, `build-sample.sh`, `build-cfgmanager.sh`, `build-sample-client.sh`
   - `scripts/telaf/`: `build-cfgmanager-sim.sh`, `deploy-cfgmanager-sim.sh`, `run-simulation.sh` (renamed from `run-telaf-simulation.sh`)

### 3.3 Configuration and Code Updates

1. **`apps/cfgManager/server/Component.cdef`**:
   Update paths:
   - Sources: `${PROJECT_ROOT}/components/cfgManager/<module>/src/...`
   - Include flags: `-I${PROJECT_ROOT}/components/cfgManager/<module>/include`

2. **`samples/cfgClient/clientComponent/Component.cdef`**:
   Update include:
   - `-I${PROJECT_ROOT}/components/cfgManager/core/include`

3. **`tests/CMakeLists.txt`**:
   Update paths:
   - Include directories: `${CMAKE_CURRENT_SOURCE_DIR}/../components/cfgManager/<module>/include`
   - Core sources: `${CMAKE_CURRENT_SOURCE_DIR}/../components/cfgManager/<module>/src/*.cpp`

4. **`scripts/run-unit-tests.sh`**:
   Update gcov coverage filters:
   - `--filter '/workspace/components/cfgManager/core/'`
   - `--filter '/workspace/components/cfgManager/crypto/'`
   - `--filter '/workspace/components/cfgManager/security/'`
   - `--filter '/workspace/components/cfgManager/storage/'`
   - `--filter '/workspace/components/cfgManager/events/'`

5. **`platform/rpi5/docker/run-docker-build.sh` & `scripts/rpi5/build.sh`**:
   - Update script and workspace paths pointing to `platform/rpi5/...`.

6. **`scripts/rpi5/internal-build.sh`**:
   - Update symlink paths:
     - `targets/rpi5.sdef` -> `platform/rpi5/targets/rpi5.sdef`
     - `targets/rpi5.sinc` -> `platform/rpi5/targets/rpi5.sinc`
     - `platformAdaptor/wdog` -> `platform/rpi5/targets/platformAdaptor/wdog`
     - Patch path -> `platform/rpi5/patches/0001-add-rpi5-target-support.patch`

7. **`scripts/telaf/build-cfgmanager-sim.sh` & `scripts/telaf/deploy-cfgmanager-sim.sh`**:
   - Update any internal path references to ensure relative resolution from `scripts/telaf/` works accurately.

8. **`docs/telaf_simulation.md` & `README.md`**:
   - Remove rate-limit cloning instructions.
   - Streamline simulation guide to: start simulation -> build packages -> deploy & verify.
   - Update tree structure diagrams.

---

## 4. Acceptance Verification

1. **Google Test Unit Tests**:
   - Run `./scripts/run-unit-tests.sh`
   - Must pass all 45 C++ unit tests.
   - Code coverage must remain >= 80% on core components.
2. **TelAF Simulation Build & Execution**:
   - Run `./scripts/telaf/build-cfgmanager-sim.sh` -> packages built without error.
   - Run `./scripts/telaf/deploy-cfgmanager-sim.sh` -> installed into container, live demo execution passes with clean syslog.
