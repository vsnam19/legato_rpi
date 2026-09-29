# Qualcomm TelAF & Vendor Monorepo Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Reorganize the repository into an automotive-grade monorepo containing pristine Qualcomm TelAF Simulation source code in `upstream/`, patch files in `patches/`, our C++20 services/components in `vendor/custom/`, and a unified build & deployment toolchain supporting dual build modes (standalone `.update` apps and integrated TelAF system).

**Architecture:** Copy clean TelAF simulation source trees into `upstream/` (strictly excluding `.git`, `build`, `rootfs`, and binary blobs). Extract Jinja2 compatibility patch into `patches/legato/0001-ifgen-jinja2-compatibility.patch` and implement `scripts/patch.sh`. Move all our code into `vendor/custom/`, create `vendor.sinc`, and update build scripts & Makefile.

**Tech Stack:** C++20, Legato / TelAF Application Framework, CMake, Google Test, Docker, Bash.

**Spec:** [`docs/superpowers/specs/2026-09-29-telaf-vendor-monorepo-design.md`](file:///home/namvs/Workspaces/projects/linux/legato_rpi/docs/superpowers/specs/2026-09-29-telaf-vendor-monorepo-design.md)

## Global Constraints

- Upstream code in `upstream/` must remain pristine; all changes applied via `patches/`.
- Implementation language for vendor code remains strictly C++20 (`-std=c++20`).
- No files larger than 50MB committed to Git (strict .gitignore for build/rootfs/tarballs).
- All 45 Google Test unit tests must pass.
- Both standalone build/deploy and integrated builds must function seamlessly.

---

### Task 1: Populate `upstream/` with Pristine TelAF Simulation Sources

- [ ] **Step 1: Create `upstream/` and copy clean source trees**
Copy `telaf`, `legato`, `sdk`, `telaf-pa`, `telaf-pa-default` from local clean simulation environment, omitting `.git`, `build`, `rootfs`, `deps/source`, and tarballs.
- [ ] **Step 2: Update `.gitignore`**
Ensure `upstream/**/build/`, `upstream/**/rootfs/`, `upstream/**/deps/source/`, and binary artifacts are ignored.
- [ ] **Step 3: Verify size and git status**
- [ ] **Step 4: Commit pristine upstream sources**

---

### Task 2: Create `patches/` and `scripts/patch.sh`

- [ ] **Step 1: Create `patches/legato/0001-ifgen-jinja2-compatibility.patch`**
Extract and format the Jinja2 compatibility patch for `legato-af/framework/tools/ifgen`.
- [ ] **Step 2: Implement `scripts/patch.sh`**
Support `apply`, `revert`, and `status` subcommands using `patch -p1`.
- [ ] **Step 3: Test patch application and status**
Verify `./scripts/patch.sh apply` succeeds.
- [ ] **Step 4: Commit patches and patch.sh**

---

### Task 3: Relocate Custom Code to `vendor/custom/`

- [ ] **Step 1: Relocate folders via git mv**
Move `apps/`, `components/`, `samples/`, `interfaces/`, and `tests/` into `vendor/custom/`.
- [ ] **Step 2: Update `Component.cdef` files**
Update include flags and source paths to `${PROJECT_ROOT}/vendor/custom/...`.
- [ ] **Step 3: Update `vendor/custom/tests/CMakeLists.txt`**
Update paths to `../components/cfgManager/...`.
- [ ] **Step 4: Create `vendor/custom/vendor.sinc`**
Declare apps, interfaceSearch, and componentSearch for TelAF integration.
- [ ] **Step 5: Commit vendor restructuring**

---

### Task 4: Implement Dual-Mode Build & Tooling Scripts

- [ ] **Step 1: Update `scripts/build.sh`**
Support `--standalone` (build `cfgManager` and `cfgClient` update packages) and `--integrated` (build whole TelAF simulation system with vendor apps included).
- [ ] **Step 2: Update `scripts/deploy.sh`**
Reference `vendor/custom/apps/cfgManager/` and `vendor/custom/samples/cfgClient/`.
- [ ] **Step 3: Update `scripts/run-unit-tests.sh`**
Point to `vendor/custom/tests` and filter `vendor/custom/components/...`.
- [ ] **Step 4: Create top-level `Makefile`**
Provide convenient targets: `test`, `build-app`, `build-system`, `deploy`, `patch-apply`, `patch-revert`.
- [ ] **Step 5: Commit scripts and Makefile**

---

### Task 5: Update Documentation & Acceptance Verification

- [ ] **Step 1: Update `README.md` and `docs/telaf_simulation.md`**
Reflect the new monorepo layout, dual-build workflows, and patch commands.
- [ ] **Step 2: Run Google Test suite**
`./scripts/run-unit-tests.sh` -> 45/45 PASS.
- [ ] **Step 3: Run standalone build and deploy**
`./scripts/build.sh --standalone` and `./scripts/deploy.sh` -> live verification on TelAF container PASS.
- [ ] **Step 4: Commit and push branch to remote**
