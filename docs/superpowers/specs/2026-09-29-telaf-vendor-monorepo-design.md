# Qualcomm TelAF & Vendor Monorepo Architecture Specification

## 1. Context & Motivation

To transform the repository into an enterprise automotive-grade monorepo:
1. **Upstream TelAF Simulation**: Track Qualcomm TelAF simulation repositories directly within the repository in `upstream/`, keeping them **100% pristine** (no direct in-tree modifications).
2. **Patch Management**: Any fixes or compatibility adaptations needed for TelAF (e.g. Jinja2 `ifgen` syntax compatibility) are tracked in `patches/` as standard patch files and applied automatically via `scripts/patch.sh`.
3. **Vendor Isolation**: All custom services, components, interfaces, tests, and samples are isolated in `vendor/custom/`.
4. **Dual Build Modes**:
   - **Standalone Mode (`--standalone`)**: Compiles `cfgManager` and `cfgClient` using `mkapp -t simulation` into `.update` packages for hot runtime deployment via `scripts/deploy.sh`.
   - **Integrated System Mode (`--integrated`)**: Ingests `vendor/custom/vendor.sinc` into TelAF simulation system definitions, baking `cfgManager` into the core simulation system.

---

## 2. Directory Layout Specification

```text
legato_rpi/  (Branch: feat/telaf-simulation)
├── upstream/                         # Pristine Qualcomm TelAF Simulation Sources
│   ├── telaf/                        # TelAF Core Framework
│   ├── legato/                       # Legato Application Framework (legato-af)
│   ├── sdk/                          # Snaptel SDK
│   ├── telaf-pa/                     # Platform Adaptor
│   └── telaf-pa-default/             # Platform Adaptor Default
│
├── patches/                          # Upstream Patches (Version-controlled)
│   └── legato/
│       └── 0001-ifgen-jinja2-compatibility.patch
│
├── vendor/                           # Custom Development Code (C++20)
│   └── custom/                       # Custom vendor namespace
│       ├── apps/
│       │   └── cfgManager/           # CfgManager service daemon
│       ├── components/
│       │   └── cfgManager/           # 5 core subsystems (core, crypto, security, storage, events)
│       ├── samples/
│       │   └── cfgClient/            # Client demo app
│       ├── interfaces/
│       │   └── cfgManager.api        # RPC IDL definition
│       ├── tests/                    # Google Test C++20 suites
│       │   ├── CMakeLists.txt
│       │   └── ...
│       └── vendor.sinc               # System include for TelAF integrated build
│
├── scripts/                          # Build & Deployment Tooling
│   ├── patch.sh                      # Apply, revert, inspect upstream patches
│   ├── build.sh                      # Dual-mode builder (--standalone | --integrated)
│   ├── deploy.sh                     # Runtime package deployer & live verifier
│   ├── run-simulation.sh             # Container lifecycle manager (start|stop|status|shell)
│   └── run-unit-tests.sh             # Google Test C++20 test runner
│
├── Makefile                          # Unified developer CLI interface
├── docs/                             # Architecture & Integration Guides
└── README.md                         # Project documentation
```

---

## 3. Detailed Component Responsibilities

### 3.1 `upstream/`
- Contains clean, build-artifact-free checkouts of the TelAF simulation stack:
  - `upstream/telaf/`
  - `upstream/legato/` (contains `legato-af` and `3rdParty/`)
  - `upstream/sdk/`
  - `upstream/telaf-pa/`
  - `upstream/telaf-pa-default/`
- Build artifacts (`*/build/`, `*/rootfs/`, `*.tar.gz`, `*.o`, `*.so`) are strictly excluded in `.gitignore`.

### 3.2 `patches/`
- `patches/legato/0001-ifgen-jinja2-compatibility.patch`: Patches `framework/tools/ifgen` to support modern Jinja2 (`pass_context`, `pass_environment`, removal of `with_` extension).
- `scripts/patch.sh`:
  - `scripts/patch.sh apply`: Applies all patches in order.
  - `scripts/patch.sh revert`: Reverts patches to restore pristine state.
  - `scripts/patch.sh status`: Checks patch status.

### 3.3 `vendor/custom/`
- `vendor/custom/vendor.sinc`:
  ```text
  apps:
  {
      $PROJECT_ROOT/vendor/custom/apps/cfgManager/cfgManager.adef
  }
  interfaceSearch:
  {
      $PROJECT_ROOT/vendor/custom/interfaces
  }
  componentSearch:
  {
      $PROJECT_ROOT/vendor/custom/components
  }
  ```
- All include flags updated to `-I${PROJECT_ROOT}/vendor/custom/components/cfgManager/...`.

---

## 4. Acceptance Criteria

1. `scripts/patch.sh apply` applies cleanly to `upstream/`.
2. `scripts/run-unit-tests.sh` executes 45/45 Google Tests with C2 coverage and reports PASS.
3. `scripts/build.sh --standalone` builds `vendor/custom/apps/cfgManager/cfgManager.simulation.update` and `vendor/custom/samples/cfgClient/cfgClient.simulation.update`.
4. `scripts/deploy.sh` installs updates into container `telaf_simulation_runtime_2204_m`, restarts client, and prints live syslog verification with `ALL CHECKS PASSED!`.
5. Working tree clean, all files tracked properly without exceeding GitHub size limits.
