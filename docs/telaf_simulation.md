# Qualcomm TelAF Simulation Guide & CfgManager Service Integration

## 1. Overview

This document describes the Qualcomm Telematics Application Framework (**TelAF**) simulation environment, the architecture of the **CfgManager** service running inside it, and step-by-step instructions for container lifecycle management, building, deploying, and verifying services.

---

## 2. Tooling and Script Reference

All execution and developer tooling scripts are located in `scripts/`:

| Script | Purpose |
|---|---|
| [`scripts/run-simulation.sh`](file:///home/namvs/Workspaces/projects/linux/legato_rpi/scripts/run-simulation.sh) | Manages TelAF simulation container lifecycle (`start`, `stop`, `status`, `shell`, `logs`). |
| [`scripts/build.sh`](file:///home/namvs/Workspaces/projects/linux/legato_rpi/scripts/build.sh) | Compiles `cfgManager` daemon and `cfgClient` demo using development container (`telaf_simulation_develop_2204:1.0.0`). |
| [`scripts/deploy.sh`](file:///home/namvs/Workspaces/projects/linux/legato_rpi/scripts/deploy.sh) | Installs update packages into runtime container (`telaf_simulation_runtime_2204_m`), restarts client, and prints live syslog. |
| [`scripts/run-unit-tests.sh`](file:///home/namvs/Workspaces/projects/linux/legato_rpi/scripts/run-unit-tests.sh) | Executes the 45 Google Test C++23 unit tests and generates C2 branch coverage report. |

---

## 3. Simulation Container Lifecycle Management

The simulation runs in a dedicated Docker runtime container (`telaf_simulation_runtime_2204_m`).

### 3.1 Start Simulation
```bash
./scripts/run-simulation.sh start
```

### 3.2 Check Status & Version
Verify that the TelAF framework daemons are active:
```bash
./scripts/run-simulation.sh status
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

### 3.3 Enter Container Shell
```bash
./scripts/run-simulation.sh shell
```

### 3.4 View Live System Logs
```bash
./scripts/run-simulation.sh logs
```

### 3.5 Stop Simulation
```bash
./scripts/run-simulation.sh stop
```

---

## 4. Developing & Deploying CfgManager on TelAF Simulation

### 4.1 Architecture Highlights
1. **C++23 Modern Service**:
   - Implemented with `-std=c++23`, using modern features, structured bindings, `std::span`, and smart pointers.
   - Built with `-static-libstdc++` and `-static-libgcc` in `Component.cdef` to guarantee runtime symbol compatibility inside Ubuntu 22.04.
2. **TelAF RPC Interface Specification (`interfaces/cfgManager.api`)**:
   - TelAF IPC code generator (`ifgen`) requires explicit RPC message identifiers:
     - `SetString(...) = 0;`
     - `GetString(...) = 1;`
     - `SetBinary(...) = 2;`
     - `GetBinary(...) = 3;`
     - `Delete(...) = 4;`
     - `EVENT Change(...) = (5, 6);`
3. **TelAF System Service Domain Binding**:
   - Core daemons (`configTree`, `logDaemon`) execute under the `tafcore` security domain.
   - In [`apps/cfgManager/cfgManager.adef`](file:///home/namvs/Workspaces/projects/linux/legato_rpi/apps/cfgManager/cfgManager.adef), IPC is bound to:
     ```text
     bindings: {
         cfgManager.server.le_cfg -> <tafcore>.le_cfg
     }
     ```
4. **Security Provider HAL with OP-TEE Fallback**:
   - Evaluates `/dev/tee0` device availability and tests active key derivation via [`ISecurityProvider::GetMasterKey()`](file:///home/namvs/Workspaces/projects/linux/legato_rpi/components/cfgManager/security/src/securityProviderFactory.cpp).
   - In container simulation environments where OP-TEE hardware supplicants are not running, the factory gracefully falls back to `SimulatedEnclaveProvider`, guaranteeing 100% operational availability while preserving hardware security boundaries on real target hardware.
5. **Path Abstraction via Composite `cfgId` (32-bit bitfield)**:
   - Consumer applications interact only via 32-bit `cfgId` (`[Subsystem 8b][Category 8b][Flags 8b][KeyId 8b]`).
   - Consumer never learns internal filesystem or configTree tree hierarchy paths.
6. **Data Classification**:
   - **RawData**: Stored directly in `configTree`.
   - **SensitiveData**: Encrypted with AES-256-GCM with per-entry random 96-bit IV and 128-bit authentication tag before writing to `configTree`.
   - **SecureData**: Stored directly in secure hardware enclave / TrustZone storage.
7. **Pub/Sub Notifications**:
   - Subscribers register for change callbacks on specific `cfgId`s or subsystem masks using Legato event loop.

---

### 4.2 Build Instructions
Both `cfgManager` and `cfgClient` are compiled inside the TelAF simulation development Docker image (`telaf_simulation_develop_2204:1.0.0`) to match target GLIBC and compiler requirements:
```bash
./scripts/build.sh
```
This generates:
- `apps/cfgManager/cfgManager.simulation.update`
- `samples/cfgClient/cfgClient.simulation.update`

---

### 4.3 Deployment & Verification Instructions
Deploy the packages into the running TelAF simulation container and execute the verification sequence:
```bash
./scripts/deploy.sh
```

---

### 4.4 End-to-End Execution Evidence
Upon deployment, `cfgClient` connects to `cfgManager` over TelAF IPC and executes full functional validation:
```text
Sep 29 08:54:30 simulation user.info TelAF:  INFO | supervisor[219]/supervisor T=main | proc.c proc_Start() 1594 | Starting process 'cfgClient' with pid 152725
Sep 29 08:54:30 simulation user.info TelAF:  INFO | cfgClient[152725]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 19 | =================================================================
Sep 29 08:54:30 simulation user.info TelAF:  INFO | cfgClient[152725]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 20 |  CfgManager Sample Client Application Started (C++23)
Sep 29 08:54:30 simulation user.info TelAF:  INFO | cfgClient[152725]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 21 | =================================================================
Sep 29 08:54:30 simulation user.info TelAF:  INFO | cfgClient[152725]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 29 | Target cfgIds: Raw=0x01020001, Sensitive=0x02030002, Secure=0x04030003, Bin=0x02040004
Sep 29 08:54:30 simulation user.info TelAF:  INFO | cfgClient[152725]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 33 | ==> Subscribing to change events for Raw cfgId 0x01020001...
Sep 29 08:54:30 simulation user.info TelAF:  INFO | cfgClient[152725]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 36 | Successfully registered change handler ref 0xcb
Sep 29 08:54:30 simulation user.info TelAF:  INFO | cfgClient[152725]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 42 | ==> Writing RawData string...
Sep 29 08:54:30 simulation user.info TelAF:  INFO | cfgClient[152725]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 49 | [RawData Result] Successfully read: 'rpi5-edge-gateway.local'
Sep 29 08:54:30 simulation user.info TelAF:  INFO | cfgClient[152725]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 52 | ==> Writing SensitiveData (AES-256-GCM)...
Sep 29 08:54:30 simulation user.info TelAF:  INFO | cfgClient[152725]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 59 | [SensitiveData Result] Decrypted transparently: 'M4st3r_P@ssw0rd_K3y#2026!'
Sep 29 08:54:30 simulation user.info TelAF:  INFO | cfgClient[152725]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 62 | ==> Writing SecureData (TrustZone Secure Storage)...
Sep 29 08:54:30 simulation user.info TelAF:  INFO | cfgClient[152725]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 69 | [SecureData Result] Retrieved from TrustZone: 'HW-ROOT-OF-TRUST-TOKEN-994821'
Sep 29 08:54:30 simulation user.info TelAF:  INFO | cfgClient[152725]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 72 | ==> Writing Binary SensitiveData blob...
Sep 29 08:54:30 simulation user.info TelAF:  INFO | cfgClient[152725]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 88 | [Binary Result] Retrieved 9 bytes: [ 0x10 0x20 0x30 0x40 0x50 0xDE 0xAD 0xBE 0xEF ]
Sep 29 08:54:30 simulation user.info TelAF:  INFO | cfgClient[152725]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 91 | ==> Updating RawData to trigger Pub/Sub change notification...
Sep 29 08:54:30 simulation user.info TelAF:  INFO | cfgClient[152725]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 95 | =================================================================
Sep 29 08:54:30 simulation user.info TelAF:  INFO | cfgClient[152725]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 96 |  CfgManager Sample Client Demo Complete - ALL CHECKS PASSED!
Sep 29 08:54:30 simulation user.info TelAF:  INFO | cfgClient[152725]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 97 | =================================================================
Sep 29 08:54:30 simulation user.info TelAF:  INFO | cfgClient[152725]/clientComponent T=main | client.cpp OnConfigChanged() 13 | >>> [CLIENT EVENT] Configuration changed: cfgId=0x01020001, Value='rpi5-edge-gateway.local'
Sep 29 08:54:30 simulation user.info TelAF:  INFO | cfgClient[152725]/clientComponent T=main | client.cpp OnConfigChanged() 13 | >>> [CLIENT EVENT] Configuration changed: cfgId=0x01020001, Value='rpi5-edge-gateway-RECONFIGURED.local'
```
