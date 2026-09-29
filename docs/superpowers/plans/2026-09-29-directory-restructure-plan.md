# Directory Restructure & Source Simplification Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Clean up the repository for branch `feat/telaf-simulation` by removing obsolete Raspberry Pi 5 platform files and HTTP 429 rate-limit cloner scripts, consolidating CfgManager modules into `components/cfgManager/`, and streamlining `scripts/` to focus purely on TelAF Simulation execution.

**Architecture:** Relocate business logic into `components/cfgManager/{core,crypto,security,storage,events}` via `git mv` to preserve commit history. Remove obsolete `targets/`, `target-root/`, `toolchain/`, `patches/`, `docker/`, and `tests/python/`. Re-implement `scripts/build.sh` and `scripts/deploy.sh` to target TelAF simulation directly.

**Tech Stack:** C++23, CMake, Legato Application Framework (TelAF), Google Test, Docker, Bash.

**Spec:** [`docs/superpowers/specs/2026-09-29-directory-restructure-design.md`](file:///home/namvs/Workspaces/projects/linux/legato_rpi/docs/superpowers/specs/2026-09-29-directory-restructure-design.md)

## Global Constraints

- Execution target is strictly **Qualcomm TelAF Simulation** (`mkapp -t simulation`).
- All code must continue to compile with `-std=c++23`.
- All 45 Google Test unit tests must pass.
- Full client-server demo on TelAF runtime container (`telaf_simulation_runtime_2204_m`) must pass without regression.

## Review Focus

1. `Component.cdef` include and source paths broken after moving to `components/cfgManager/`.
2. `tests/CMakeLists.txt` include directories and glob patterns broken after moving components.
3. `scripts/run-unit-tests.sh` coverage filter paths broken after moving components.
4. Redundant or dead files left behind in `scripts/` or root directory.
5. `scripts/build.sh` and `scripts/deploy.sh` not having executable permissions (+x).

---

### Task 1: Remove Obsolete RPi5 Files and Rate-Limit Scripts

**Files:**
- Remove: `targets/`
- Remove: `target-root/`
- Remove: `toolchain/`
- Remove: `patches/`
- Remove: `docker/`
- Remove: `samples/helloWorld/`
- Remove: `scripts/internal-build.sh`
- Remove: `scripts/build-sample.sh`
- Remove: `scripts/build-cfgmanager.sh`
- Remove: `scripts/build-sample-client.sh`
- Remove: `scripts/setup_submodule.sh`
- Remove: `scripts/setup-telaf-simulation.sh`
- Remove: `scripts/telaf_simulation/`
- Remove: `tests/python/`

- [ ] **Step 1: Execute git rm on obsolete files and directories**
```bash
git rm -rf targets target-root toolchain patches docker samples/helloWorld
git rm -rf scripts/internal-build.sh scripts/build-sample.sh scripts/build-cfgmanager.sh scripts/build-sample-client.sh scripts/setup_submodule.sh scripts/setup-telaf-simulation.sh scripts/telaf_simulation
git rm -rf tests/python
```

- [ ] **Step 2: Verify removals in git status**
```bash
git status --short
```

- [ ] **Step 3: Commit removals**
```bash
git commit -m "chore: remove obsolete RPi5 platform files and rate-limit cloner scripts"
```

---

### Task 2: Relocate CfgManager Modules to `components/cfgManager/`

**Files:**
- Move: `core/` -> `components/cfgManager/core/`
- Move: `crypto/` -> `components/cfgManager/crypto/`
- Move: `security/` -> `components/cfgManager/security/`
- Move: `storage/` -> `components/cfgManager/storage/`
- Move: `events/` -> `components/cfgManager/events/`

- [ ] **Step 1: Create `components/cfgManager` directory and move subsystems via git mv**
```bash
mkdir -p components/cfgManager
git mv core components/cfgManager/core
git mv crypto components/cfgManager/crypto
git mv security components/cfgManager/security
git mv storage components/cfgManager/storage
git mv events components/cfgManager/events
```

- [ ] **Step 2: Verify git status shows renames (R)**
```bash
git status --short
```

- [ ] **Step 3: Commit renames**
```bash
git commit -m "refactor: move CfgManager subsystems to components/cfgManager/"
```

---

### Task 3: Update Build Configurations & Include Paths

**Files:**
- Modify: `apps/cfgManager/server/Component.cdef`
- Modify: `samples/cfgClient/clientComponent/Component.cdef`
- Modify: `tests/CMakeLists.txt`
- Modify: `scripts/run-unit-tests.sh`

- [ ] **Step 1: Update `apps/cfgManager/server/Component.cdef`**
Update sources and include flags:
```text
sources:
{
    cfgManagerServer.cpp
    ${PROJECT_ROOT}/components/cfgManager/core/src/cfgRouter.cpp
    ${PROJECT_ROOT}/components/cfgManager/core/src/cfgManagerService.cpp
    ${PROJECT_ROOT}/components/cfgManager/crypto/src/cryptoEngine.cpp
    ${PROJECT_ROOT}/components/cfgManager/security/src/securityProviderFactory.cpp
    ${PROJECT_ROOT}/components/cfgManager/security/src/simulatedEnclaveProvider.cpp
    ${PROJECT_ROOT}/components/cfgManager/security/src/opteeProvider.cpp
    ${PROJECT_ROOT}/components/cfgManager/storage/src/memoryConfigBackend.cpp
    ${PROJECT_ROOT}/components/cfgManager/storage/src/configTreeBackend.cpp
    ${PROJECT_ROOT}/components/cfgManager/events/src/eventDispatcher.cpp
}

cxxflags:
{
    -std=c++23
    -I${PROJECT_ROOT}/components/cfgManager/core/include
    -I${PROJECT_ROOT}/components/cfgManager/crypto/include
    -I${PROJECT_ROOT}/components/cfgManager/security/include
    -I${PROJECT_ROOT}/components/cfgManager/storage/include
    -I${PROJECT_ROOT}/components/cfgManager/events/include
}
```

- [ ] **Step 2: Update `samples/cfgClient/clientComponent/Component.cdef`**
Update include flag:
```text
cxxflags:
{
    -std=c++23
    -I${PROJECT_ROOT}/components/cfgManager/core/include
}
```

- [ ] **Step 3: Update `tests/CMakeLists.txt`**
Update include directories and source file globs:
```cmake
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/cfgManager/core/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/cfgManager/crypto/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/cfgManager/security/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/cfgManager/storage/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/cfgManager/events/include
    ${OPENSSL_INCLUDE_DIR}
)

file(GLOB_RECURSE CORE_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/cfgManager/core/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/cfgManager/crypto/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/cfgManager/security/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/cfgManager/storage/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/cfgManager/events/src/*.cpp
)
```

- [ ] **Step 4: Update `scripts/run-unit-tests.sh`**
Update gcov filters:
```bash
--filter '/workspace/components/cfgManager/core/' \
--filter '/workspace/components/cfgManager/crypto/' \
--filter '/workspace/components/cfgManager/security/' \
--filter '/workspace/components/cfgManager/storage/' \
--filter '/workspace/components/cfgManager/events/'
```

- [ ] **Step 5: Run `./scripts/run-unit-tests.sh` to verify tests pass with new paths**
Expected: 45/45 tests PASS.

- [ ] **Step 6: Commit changes**
```bash
git add apps/cfgManager/server/Component.cdef samples/cfgClient/clientComponent/Component.cdef tests/CMakeLists.txt scripts/run-unit-tests.sh
git commit -m "refactor: update include paths and CMake config for components/cfgManager"
```

---

### Task 4: Streamline Scripts for TelAF Simulation

**Files:**
- Create/Overwrite: `scripts/build.sh` (TelAF simulation build)
- Create/Overwrite: `scripts/deploy.sh` (TelAF simulation deploy)
- Create/Overwrite: `scripts/run-simulation.sh` (TelAF simulation container manager)
- Remove: `scripts/build-cfgmanager-sim.sh`
- Remove: `scripts/deploy-cfgmanager-sim.sh`

- [ ] **Step 1: Write `scripts/build.sh`**
Implement build logic using develop container `telaf_simulation_develop_2204:1.0.0` to build both `apps/cfgManager` and `samples/cfgClient`.

- [ ] **Step 2: Write `scripts/deploy.sh`**
Implement deploy logic to copy packages to `telaf_simulation_runtime_2204_m`, run update, restart `cfgClient`, and verify output in syslog.

- [ ] **Step 3: Write `scripts/run-simulation.sh`**
Implement clean lifecycle management (`start`, `stop`, `status`, `shell`) for `telaf_simulation_runtime_2204_m`.

- [ ] **Step 4: Remove redundant temporary scripts**
```bash
git rm scripts/build-cfgmanager-sim.sh scripts/deploy-cfgmanager-sim.sh
```

- [ ] **Step 5: Chmod +x and verify scripts**
```bash
chmod +x scripts/build.sh scripts/deploy.sh scripts/run-simulation.sh scripts/run-unit-tests.sh
```

- [ ] **Step 6: Commit new scripts**
```bash
git add scripts/
git commit -m "feat(scripts): streamline TelAF simulation build, deploy and management scripts"
```

---

### Task 5: Update Documentation

**Files:**
- Modify: `README.md`
- Modify: `docs/telaf_simulation.md`

- [ ] **Step 1: Update `README.md`**
Reframe README to reflect TelAF Simulation target and clean directory structure.

- [ ] **Step 2: Update `docs/telaf_simulation.md`**
Remove outdated rate-limit cloner instructions and update build & deploy sections to use `scripts/build.sh` and `scripts/deploy.sh`.

- [ ] **Step 3: Commit documentation**
```bash
git add README.md docs/telaf_simulation.md
git commit -m "docs: update README and TelAF simulation guide with restructured paths"
```

---

### Task 6: End-to-End Verification & Validation

**Verification Steps:**
- [ ] **Step 1: Run Google Test suite**
```bash
./scripts/run-unit-tests.sh
```
Expected: 45/45 PASS.

- [ ] **Step 2: Run simulation build**
```bash
./scripts/build.sh
```
Expected: `cfgManager.simulation.update` and `cfgClient.simulation.update` built without error.

- [ ] **Step 3: Run simulation deploy**
```bash
./scripts/deploy.sh
```
Expected: Both packages applied, both apps running, client logread shows `ALL CHECKS PASSED!`

- [ ] **Step 4: Push branch to remote**
```bash
git push origin feat/telaf-simulation
```
