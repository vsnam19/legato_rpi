# Qualcomm TelAF Simulation Guide & CfgManager Service Integration

## 1. Overview

This document describes the Qualcomm Telematics Application Framework (**TelAF**) simulation environment, the architecture of the monorepo layout, and instructions for building, deploying, and verifying services.

---

## 2. Monorepo Structure

The repository is organized following an automotive monorepo pattern:

- **`upstream/`**: Contains clean, pristine checkouts of the Qualcomm TelAF simulation components:
  - `telaf/`: Core framework
  - `legato/`: Legato Application Framework (legato-af)
  - `sdk/`: Snaptel SDK
  - `telaf-pa/` and `telaf-pa-default/`: Platform adaptors
  - *Note:* Upstream code is kept 100% untouched in git history.
- **`patches/`**: Contains version-controlled `.patch` files applied to upstream components.
  - `patches/legato/0001-ifgen-jinja2-compatibility.patch`: Modern Jinja2 compatibility for Legato `ifgen`.
- **`vendor/custom/`**: All custom components, apps, samples, interfaces, and unit tests:
  - `apps/cfgManager/`: CfgManager service daemon (`cfgManager.adef`, `server/Component.cdef`)
  - `components/cfgManager/`: C++20 core business logic (core, crypto, security, storage, events)
  - `samples/cfgClient/`: Sample client application (`cfgClient.adef`, `clientComponent/Component.cdef`)
  - `interfaces/cfgManager.api`: RPC IPC interface specification
  - `tests/`: Pure C++20 Google Test suite (45 tests)
  - `vendor.sinc`: Vendor system include for TelAF integration
  - `system_simulation.sdef`: Integrated system definition
- **`scripts/`**: Build, test, patch, deploy, and container lifecycle scripts.
- **`Makefile`**: Unified developer command-line interface.

---

## 3. Tooling and CLI Reference

| Command | Script Equivalent | Purpose |
|---|---|---|
| `make test` | `./scripts/run-unit-tests.sh` | Executes 45 Google Test C++20 unit tests. |
| `make coverage` | `./scripts/run-unit-tests.sh --coverage` | Generates C2 branch coverage report. |
| `make patch-status` | `./scripts/patch.sh status` | Checks whether upstream patches are applied. |
| `make patch-apply` | `./scripts/patch.sh apply` | Applies all patches in `patches/` to `upstream/`. |
| `make patch-revert` | `./scripts/patch.sh revert` | Reverts all patches, restoring pristine `upstream/`. |
| `make build-app` | `./scripts/build.sh --standalone` | Compiles standalone `.update` packages (`cfgManager`, `cfgClient`). |
| `make build-system` | `./scripts/build.sh --integrated` | Compiles full TelAF system image with vendor apps baked in. |
| `make deploy` | `./scripts/deploy.sh` | Deploys update packages to runtime container and executes live demo. |
| `make run-sim` | `./scripts/run-simulation.sh start` | Starts the TelAF simulation Docker container. |
| `make stop-sim` | `./scripts/run-simulation.sh stop` | Stops the TelAF simulation Docker container. |
| `make sim-status` | `./scripts/run-simulation.sh status` | Displays container and daemon status. |
| `make sim-shell` | `./scripts/run-simulation.sh shell` | Opens bash shell in runtime container. |

---

## 4. Simulation Container Lifecycle Management

The simulation runs in a dedicated Docker runtime container (`telaf_simulation_runtime_2204_m`).

### 4.1 Start Simulation
```bash
make run-sim
```

### 4.2 Check Status & Version
```bash
make sim-status
```

Output:
```text
=== Container Status ===
Name: /telaf_simulation_runtime_2204_m | Status: running | IP: 172.18.0.2

=== TelAF Framework Version ===
telaf.lnx.1.1-260100_f701d881562e9b2927a7ec217c8d8c85_modified

=== TelAF Framework Status ===
TelAF Systems were installed
TelAF framework is running
```

---

## 5. Developing & Deploying CfgManager (C++20)

### 5.1 Architecture Highlights
1. **C++20 Modern Service**:
   - Implemented with `-std=c++20`, using modern features, structured bindings, `std::span`, and smart pointers.
   - Built with `-static-libstdc++` and `-static-libgcc` in `Component.cdef` to guarantee runtime symbol compatibility inside Ubuntu 22.04.
2. **TelAF RPC Interface Specification (`vendor/custom/interfaces/cfgManager.api`)**:
   - Explicit RPC message identifiers:
     - `SetString(...) = 0;`
     - `GetString(...) = 1;`
     - `SetBinary(...) = 2;`
     - `GetBinary(...) = 3;`
     - `Delete(...) = 4;`
     - `EVENT Change(...) = (5, 6);`
3. **Security Provider HAL with OP-TEE Fallback**:
   - Evaluates `/dev/tee0` device availability and tests active key derivation via `ISecurityProvider::GetMasterKey()`.
   - In container simulation environments where OP-TEE hardware supplicants are not running, the factory gracefully falls back to `SimulatedEnclaveProvider`.
4. **Path Abstraction via Composite `cfgId` (32-bit bitfield)**:
   - Consumer applications interact only via 32-bit `cfgId` (`[Subsystem 8b][Category 8b][Flags 8b][KeyId 8b]`).
   - Consumer never learns internal filesystem or configTree tree hierarchy paths.
5. **Data Classification**:
   - **RawData**: Stored directly in `configTree`.
   - **SensitiveData**: Encrypted with AES-256-GCM with per-entry random 96-bit IV and 128-bit authentication tag before writing to `configTree`.
   - **SecureData**: Stored directly in secure hardware enclave / TrustZone storage.
6. **Pub/Sub Notifications**:
   - Subscribers register for change callbacks on specific `cfgId`s or subsystem masks using Legato event loop.

---

## 6. End-to-End Verification Evidence

```text
simulation user.info TelAF:  INFO | cfgClient[152988]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 19 | =================================================================
simulation user.info TelAF:  INFO | cfgClient[152988]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 20 |  CfgManager Sample Client Application Started (C++20)
simulation user.info TelAF:  INFO | cfgClient[152988]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 21 | =================================================================
simulation user.info TelAF:  INFO | cfgClient[152988]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 29 | Target cfgIds: Raw=0x01020001, Sensitive=0x02030002, Secure=0x04030003, Bin=0x02040004
simulation user.info TelAF:  INFO | cfgClient[152988]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 33 | ==> Subscribing to change events for Raw cfgId 0x01020001...
simulation user.info TelAF:  INFO | cfgClient[152988]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 36 | Successfully registered change handler ref 0xf5
simulation user.info TelAF:  INFO | cfgClient[152988]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 42 | ==> Writing RawData string...
simulation user.info TelAF:  INFO | cfgClient[152988]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 49 | [RawData Result] Successfully read: 'rpi5-edge-gateway.local'
simulation user.info TelAF:  INFO | cfgClient[152988]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 52 | ==> Writing SensitiveData (AES-256-GCM)...
simulation user.info TelAF:  INFO | cfgClient[152988]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 59 | [SensitiveData Result] Decrypted transparently: 'M4st3r_P@ssw0rd_K3y#2026!'
simulation user.info TelAF:  INFO | cfgClient[152988]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 62 | ==> Writing SecureData (TrustZone Secure Storage)...
simulation user.info TelAF:  INFO | cfgClient[152988]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 69 | [SecureData Result] Retrieved from TrustZone: 'HW-ROOT-OF-TRUST-TOKEN-994821'
simulation user.info TelAF:  INFO | cfgClient[152988]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 72 | ==> Writing Binary SensitiveData blob...
simulation user.info TelAF:  INFO | cfgClient[152988]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 88 | [Binary Result] Retrieved 9 bytes: [ 0x10 0x20 0x30 0x40 0x50 0xDE 0xAD 0xBE 0xEF ]
simulation user.info TelAF:  INFO | cfgClient[152988]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 91 | ==> Updating RawData to trigger Pub/Sub change notification...
simulation user.info TelAF:  INFO | cfgClient[152988]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 95 | =================================================================
simulation user.info TelAF:  INFO | cfgClient[152988]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 96 |  CfgManager Sample Client Demo Complete - ALL CHECKS PASSED!
simulation user.info TelAF:  INFO | cfgClient[152988]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 97 | =================================================================
simulation user.info TelAF:  INFO | cfgClient[152988]/clientComponent T=main | client.cpp OnConfigChanged() 13 | >>> [CLIENT EVENT] Configuration changed: cfgId=0x01020001, Value='rpi5-edge-gateway.local'
simulation user.info TelAF:  INFO | cfgClient[152988]/clientComponent T=main | client.cpp OnConfigChanged() 13 | >>> [CLIENT EVENT] Configuration changed: cfgId=0x01020001, Value='rpi5-edge-gateway-RECONFIGURED.local'
```
