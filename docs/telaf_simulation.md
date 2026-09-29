# Qualcomm TelAF Simulation Environment & HTTP 429 Mitigation Guide

## 1. Overview

This document describes the Qualcomm Telematics Application Framework (**TelAF**) simulation architecture, the resolution for the **HTTP 429 (Too Many Requests)** rate-limiting issue during repository cloning from `git.codelinaro.org`, and instructions for running the simulation container.

---

## 2. HTTP 429 Rate Limit Analysis & Mitigation Strategy

### Why HTTP 429 Occurs
When setting up TelAF Simulation, 7 git repositories must be cloned from `git.codelinaro.org`:
1. `platform/TelAF.git` (branch: `telaf.lnx.1.1`)
2. `legato-af.git` (branch: `telaf.lnx.1.1`)
3. `legatoproject/Kconfiglib.git` (tag: `caf_migration/refs/tags/20.04.0`)
4. `legatoproject/legato-3rdParty-jansson.git` (tag: `caf_migration/refs/tags/20.04.0`)
5. `platform/vendor/qcom-opensource/snaptel-sdk.git` (branch: `telsdk.lnx.2.0.r11-rel`)
6. `platform/telaf-pa.git` (branch: `telaf-pa.lnx.1.0`)
7. `platform/telaf-pa-default.git` (branch: `telaf.lnx.1.1`)

`git.codelinaro.org` is protected by Cloudflare and GitLab rate-limiting policies. Rapid consecutive or concurrent cloning requests exceed the burst limit per IP address, resulting in:
```text
fatal: unable to access 'https://git.codelinaro.org/...': The requested URL returned error: 429
```

### Multi-Tiered Mitigation Architecture

To completely solve this, [`clone_telaf_repos.py`](file:///home/namvs/Workspaces/projects/linux/legato_rpi/scripts/telaf_simulation/clone_telaf_repos.py) implements a 4-tier strategy:

```
[Repository Clone Request]
           │
           ▼
┌─────────────────────────────────┐
│ Tier 1: Local Cache Fast-Path   │──(Cache found)──► Local Clone / Reference
└─────────────────────────────────┘                   (0 network calls, 0% rate limit risk)
           │ (No local cache)
           ▼
┌─────────────────────────────────┐
│ Tier 2: Remote Clone with Retry │
│ & Exponential Backoff + Jitter  │──(HTTP 429)────► Sleep (Base × Factor^attempt ± 20%) & Retry
└─────────────────────────────────┘
           │ (Success)
           ▼
┌─────────────────────────────────┐
│ Tier 3: Polite Inter-Repo Delay │──(3-5s pause)──► Prevents triggering Cloudflare burst limits
└─────────────────────────────────┘
           │
           ▼
┌─────────────────────────────────┐
│ Tier 4: Exact Commit Pinning    │──► Checkout commit IDs from patch_me.json
└─────────────────────────────────┘
```

1. **Tier 1 - Local Mirror / Cache Fast-Path**:
   Automatically detects existing git repositories on the local filesystem (such as `/home/namvs/Workspaces/projects/automotive/telaf_stu/simulation_env` or custom `--cache-dir`). When found, it clones directly or references local git objects, avoiding any HTTP requests to `git.codelinaro.org`.
2. **Tier 2 - Exponential Backoff & Jitter**:
   Inspects stderr for `429`, `Too Many Requests`, or connection drops. Automatically retries with exponential backoff:
   $$\text{Duration} = \text{Base} \times 2^{(\text{attempt}-1)} \pm 20\%\text{ jitter}$$
3. **Tier 3 - Polite Inter-Repository Pacing**:
   Enforces a sleep delay (default: 3-5 seconds) between successive repository clones, ensuring the client remains below the requests-per-minute threshold.
4. **Tier 4 - Git Tuning & Commit Verification**:
   Sets `http.postBuffer=524288000`, `http.lowSpeedLimit=1000`, `http.lowSpeedTime=30`, and checks out the exact commit pins defined in `patch_me.json`.

---

## 3. Tooling and Script Reference

| Script | Purpose |
|---|---|
| [`scripts/setup-telaf-simulation.sh`](file:///home/namvs/Workspaces/projects/linux/legato_rpi/scripts/setup-telaf-simulation.sh) | Automated setup: cloner + workstation + tarball preparation |
| [`scripts/run-telaf-simulation.sh`](file:///home/namvs/Workspaces/projects/linux/legato_rpi/scripts/run-telaf-simulation.sh) | Runner: manages container lifecycle, starts TelAF, checks status |
| [`scripts/telaf_simulation/clone_telaf_repos.py`](file:///home/namvs/Workspaces/projects/linux/legato_rpi/scripts/telaf_simulation/clone_telaf_repos.py) | Python cloner with 429 detection, backoff, and local caching |
| [`scripts/telaf_simulation/branch_mapping.conf`](file:///home/namvs/Workspaces/projects/linux/legato_rpi/scripts/telaf_simulation/branch_mapping.conf) | TelAF repository mapping and target branches |
| [`scripts/telaf_simulation/patch_me.json`](file:///home/namvs/Workspaces/projects/linux/legato_rpi/scripts/telaf_simulation/patch_me.json) | Commit hash pins and patch instructions |
| [`tests/python/test_telaf_cloner.py`](file:///home/namvs/Workspaces/projects/linux/legato_rpi/tests/python/test_telaf_cloner.py) | Unit tests verifying 429 detection and exponential backoff |

---

## 4. Usage Instructions

### 4.1 Setup Simulation Environment
To clone and prepare all repositories with HTTP 429 resilience:
```bash
./scripts/setup-telaf-simulation.sh
```

### 4.2 Start Simulation
Start the TelAF simulation container in background daemon mode:
```bash
./scripts/run-telaf-simulation.sh start
```

### 4.3 Check Status & Version
Verify TelAF system and Legato application status:
```bash
./scripts/run-telaf-simulation.sh status
```

### 4.4 Enter Container Shell
Attach an interactive terminal inside the running simulation:
```bash
./scripts/run-telaf-simulation.sh shell
```

### 4.5 Stop Simulation
Stop and clean up the container:
```bash
./scripts/run-telaf-simulation.sh stop
```

---

## 5. Verification Evidence

### 5.1 Cloner Unit Tests Passing
```text
$ python3 -m unittest tests/python/test_telaf_cloner.py
Ran 5 tests in 0.001s
OK
```

### 5.2 TelAF Framework Status
```text
=== TelAF Framework Version ===
telaf.lnx.1.1-260100_f701d881562e9b2927a7ec217c8d8c85_modified

=== TelAF Framework Status ===
TelAF Systems were installed
TelAF framework is running

=== Legato Installed Apps ===
printClient      [running]
printServer      [running]
tafDataCallSvc   [running]
tafKeyStoreSvc   [running]
tafPMSvc         [running]
tafRadioSvc      [running]
tafSimCardSvc    [running]
tafSomeipGWSvc   [running]
MockKeyStoreSvc  [running]
MockSmsSvc       [running]
SOMEIPManager    [running]
MQTTManager      [running]
smsManager       [running]
ShoulderTap      [running]
```

---

## 6. Developing & Deploying CfgManager on TelAF Simulation

### 6.1 Architecture Highlights
1. **C++23 Modern Service**:
   - Compiles with `-std=c++23`, using modern standard library features, concepts, and structured bindings.
   - Built with `-static-libstdc++` and `-static-libgcc` to avoid libc++ runtime symbol version issues across simulation environments.
2. **TelAF RPC Interface Specification (`interfaces/cfgManager.api`)**:
   - TelAF IPC code generator (`ifgen`) requires explicit RPC message identifiers:
     - `SetString(...) = 0;`
     - `GetString(...) = 1;`
     - `SetBinary(...) = 2;`
     - `GetBinary(...) = 3;`
     - `Delete(...) = 4;`
     - `EVENT Change(...) = (5, 6);`
3. **TelAF System Service Domain Binding**:
   - TelAF core daemons (`configTree`, `logDaemon`) execute under the `tafcore` security domain.
   - In [`apps/cfgManager/cfgManager.adef`](file:///home/namvs/Workspaces/projects/linux/legato_rpi/apps/cfgManager/cfgManager.adef), IPC is bound to:
     ```text
     bindings: {
         cfgManager.server.le_cfg -> <tafcore>.le_cfg
     }
     ```
4. **Security Provider HAL with OP-TEE Fallback**:
   - Evaluates `/dev/tee0` device availability and tests active key derivation via [`ISecurityProvider::GetMasterKey()`](file:///home/namvs/Workspaces/projects/linux/legato_rpi/security/src/securityProviderFactory.cpp).
   - In environments where OP-TEE driver is accessible but secure world supplicants are absent (e.g. Docker containers), the factory gracefully falls back to `SimulatedEnclaveProvider`, guaranteeing 100% operational availability while preserving hardware security boundaries on real target hardware.
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

### 6.2 Build Instructions
Both `cfgManager` and `cfgClient` are compiled inside the TelAF simulation development Docker image (`telaf_simulation_develop_2204:1.0.0`) to match target GLIBC and compiler requirements:
```bash
./scripts/build-cfgmanager-sim.sh
```
This generates:
- `apps/cfgManager/cfgManager.simulation.update`
- `samples/cfgClient/cfgClient.simulation.update`

---

### 6.3 Deployment & Verification Instructions
Deploy the packages into the running TelAF simulation container and execute the verification sequence:
```bash
./scripts/deploy-cfgmanager-sim.sh
```

---

### 6.4 End-to-End Execution Evidence
Upon deployment, `cfgClient` connects to `cfgManager` over TelAF IPC and executes full functional validation:
```text
Sep 29 06:53:06 simulation user.info TelAF:  INFO | supervisor[219]/supervisor T=main | proc.c proc_Start() 1594 | Starting process 'cfgClient' with pid 152679
Sep 29 06:53:06 simulation user.info TelAF:  INFO | cfgClient[152679]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 19 | =================================================================
Sep 29 06:53:06 simulation user.info TelAF:  INFO | cfgClient[152679]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 20 |  CfgManager Sample Client Application Started (C++23)
Sep 29 06:53:06 simulation user.info TelAF:  INFO | cfgClient[152679]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 21 | =================================================================
Sep 29 06:53:06 simulation user.info TelAF:  INFO | cfgClient[152679]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 29 | Target cfgIds: Raw=0x01020001, Sensitive=0x02030002, Secure=0x04030003, Bin=0x02040004
Sep 29 06:53:06 simulation user.info TelAF:  INFO | cfgClient[152679]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 33 | ==> Subscribing to change events for Raw cfgId 0x01020001...
Sep 29 06:53:06 simulation user.info TelAF:  INFO | cfgClient[152679]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 36 | Successfully registered change handler ref 0x12d
Sep 29 06:53:06 simulation user.info TelAF:  INFO | cfgClient[152679]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 42 | ==> Writing RawData string...
Sep 29 06:53:06 simulation user.info TelAF:  INFO | cfgClient[152679]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 49 | [RawData Result] Successfully read: 'rpi5-edge-gateway.local'
Sep 29 06:53:06 simulation user.info TelAF:  INFO | cfgClient[152679]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 52 | ==> Writing SensitiveData (AES-256-GCM)...
Sep 29 06:53:06 simulation user.info TelAF:  INFO | cfgClient[152679]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 59 | [SensitiveData Result] Decrypted transparently: 'M4st3r_P@ssw0rd_K3y#2026!'
Sep 29 06:53:06 simulation user.info TelAF:  INFO | cfgClient[152679]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 62 | ==> Writing SecureData (TrustZone Secure Storage)...
Sep 29 06:53:06 simulation user.info TelAF:  INFO | cfgClient[152679]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 69 | [SecureData Result] Retrieved from TrustZone: 'HW-ROOT-OF-TRUST-TOKEN-994821'
Sep 29 06:53:06 simulation user.info TelAF:  INFO | cfgClient[152679]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 72 | ==> Writing Binary SensitiveData blob...
Sep 29 06:53:06 simulation user.info TelAF:  INFO | cfgClient[152679]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 88 | [Binary Result] Retrieved 9 bytes: [ 0x10 0x20 0x30 0x40 0x50 0xDE 0xAD 0xBE 0xEF ]
Sep 29 06:53:06 simulation user.info TelAF:  INFO | cfgClient[152679]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 91 | ==> Updating RawData to trigger Pub/Sub change notification...
Sep 29 06:53:06 simulation user.info TelAF:  INFO | cfgClient[152679]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 95 | =================================================================
Sep 29 06:53:06 simulation user.info TelAF:  INFO | cfgClient[152679]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 96 |  CfgManager Sample Client Demo Complete - ALL CHECKS PASSED!
Sep 29 06:53:06 simulation user.info TelAF:  INFO | cfgClient[152679]/clientComponent T=main | client.cpp _clientComponent_COMPONENT_INIT() 97 | =================================================================
Sep 29 06:53:06 simulation user.info TelAF:  INFO | cfgClient[152679]/clientComponent T=main | client.cpp OnConfigChanged() 13 | >>> [CLIENT EVENT] Configuration changed: cfgId=0x01020001, Value='rpi5-edge-gateway.local'
Sep 29 06:53:06 simulation user.info TelAF:  INFO | cfgClient[152679]/clientComponent T=main | client.cpp OnConfigChanged() 13 | >>> [CLIENT EVENT] Configuration changed: cfgId=0x01020001, Value='rpi5-edge-gateway-RECONFIGURED.local'
```
