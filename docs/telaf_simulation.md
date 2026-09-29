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
