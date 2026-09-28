# Legato CfgManager Service & Security Architecture

The `CfgManager` service is an enterprise-grade configuration management layer for the Legato Application Framework running on the Raspberry Pi 5 (AArch64). Built natively in **C++23**, it provides a unified Legato IPC interface with hardware-backed security, authenticated envelope encryption, and asynchronous pub/sub change notifications.

---

## 1. Architectural Overview

```
                      +------------------------------------------+
                      |        Legato IPC Consumer App           |
                      |   (e.g., samples/cfgClient in C++23)     |
                      +--------------------+---------------------+
                                           |
                                  Legato IPC (cfgId)
                                           |
+------------------------------------------v------------------------------------------+
|                          Legato CfgManager Service                                  |
|                                                                                     |
|   +-----------------------------------------------------------------------------+   |
|   |                   CfgRouter & Internal Path Resolver                        |   |
|   |     (Validates 32-bit cfgId; consumers NEVER know underlying storage paths) |   |
|   +-------------------+--------------------+--------------------+---------------+   |
|                       |                    |                    |                   |
|        [RawData]      | [SensitiveData]    |       [SecureData] |     [Pub/Sub]     |
|                       v                    v                    v                   v
|                 +-----------+        +-----------+        +-----------+       +-----------+
|                 | Plaintext |        |  AES-256  |        | TrustZone |       |   Event   |
|                 | Storage   |        |  GCM Auth |        |  OP-TEE / |       | Dispatcher|
|                 | Engine    |        |  Envelope |        |  Enclave  |       +-----+-----+
|                 +-----+-----+        +-----+-----+        +-----+-----+             |
|                       |                    |                    |                   |
+-----------------------|--------------------|--------------------|-------------------|---+
                        |                    |                    |                   |
                        v                    v                    v                   v
            +------------------------------------+      +-------------------+   IPC Change
            |        Legato configTree           |      | /dev/tee0 (OP-TEE)|   Notification
            |       (le_cfg / tree: cfgManager)  |      | /var/config/...   |
            +------------------------------------+      +-------------------+
```

---

## 2. Data Classification Matrix

| Data Tier | Storage Target | Security Mechanism | Use Cases |
| :--- | :--- | :--- | :--- |
| **`RawData`** | Legato `configTree` (`le_cfg`) | Unencrypted plain-text or binary | Network hostnames, logging levels, non-sensitive timeouts, UI settings |
| **`SensitiveData`** | Legato `configTree` (`le_cfg`) | AES-256-GCM Authenticated Encryption envelope (Base64 encoded string or raw binary envelope) | Wi-Fi WPA-PSK passwords, cellular APN credentials, cloud API tokens, TLS certificates |
| **`SecureData`** | ARM TrustZone Secure World | OP-TEE Secure Storage / Hardware Key-store (with machine-bound software enclave fallback) | Hardware Root of Trust, OEM identity keys, device attestation secrets, master encryption seed |

---

## 3. Composite `cfgId` Specification

Consumers identify configuration entries exclusively via a **32-bit unsigned integer (`cfgId`)**. Consumers are **strictly forbidden** from knowing internal storage paths.

### Bitfield Layout

```text
 31             24 23             16 15                              0 (Bit Position)
+-----------------+-----------------+---------------------------------+
|  Category/Flags |    Subsystem    |             Item ID             |
+-----------------+-----------------+---------------------------------+
```

- **Bits [0..15] (Item ID):** 16-bit item index (`0x0000` - `0xFFFF`).
- **Bits [16..23] (Subsystem):** 8-bit domain identifier:
  - `0x01` (`Subsystem::System`): OS and core services (`sys/`).
  - `0x02` (`Subsystem::Network`): Connectivity and interfaces (`net/`).
  - `0x03` (`Subsystem::Security`): Authentication and access control (`sec/`).
  - `0x04` (`Subsystem::App`): User-space applications (`app/`).
- **Bits [24..31] (Flags):**
  - Bit 24 (`0x01`): `Category::Raw`
  - Bit 25 (`0x02`): `Category::Sensitive`
  - Bit 26 (`0x04`): `Category::Secure`
  - Bit 28 (`0x10`): `Flag::ReadOnly` (modifications and deletion rejected after creation)
  - Bit 29 (`0x20`): `Flag::Volatile` (in-memory only)

### C++23 Helper Functions (`core/include/cfgId.hpp`)

```cpp
#include "cfgId.hpp"

// Constructing IDs
uint32_t rawId    = cfg::MakeId(cfg::Category::Raw,       cfg::Subsystem::Network,  0x0001);
uint32_t sensId   = cfg::MakeId(cfg::Category::Sensitive, cfg::Subsystem::Security, 0x0002);
uint32_t secureId = cfg::MakeId(cfg::Category::Secure,    cfg::Subsystem::Security, 0x0003);

// Extracting Fields
cfg::Category cat = cfg::GetCategory(cfgId);
cfg::Subsystem sub = cfg::GetSubsystem(cfgId);
uint16_t item = cfg::GetItemId(cfgId);
bool isReadOnly = cfg::IsReadOnly(cfgId);
bool isValid = cfg::ValidateId(cfgId);
```

---

## 4. Legato IPC Interface (`interfaces/cfgManager.api`)

All operations interact via Legato IPC:

```c
DEFINE MAX_STRING_LEN = 4096;
DEFINE MAX_BINARY_LEN = 4096;

FUNCTION le_result_t SetString(uint32 cfgId IN, string value[MAX_STRING_LEN] IN);
FUNCTION le_result_t GetString(uint32 cfgId IN, string value[MAX_STRING_LEN] OUT);
FUNCTION le_result_t SetBinary(uint32 cfgId IN, uint8 data[MAX_BINARY_LEN] IN);
FUNCTION le_result_t GetBinary(uint32 cfgId IN, uint8 data[MAX_BINARY_LEN] OUT);
FUNCTION le_result_t Delete(uint32 cfgId IN);

HANDLER ChangeHandler(uint32 cfgId IN, string valueStr[MAX_STRING_LEN] IN);
EVENT Change(uint32 cfgId IN, ChangeHandler handler);
```

---

## 5. Cryptography & Envelope Security

For `SensitiveData`, `CfgManager` implements an authenticated encryption scheme using OpenSSL 3.0:

### Envelope Binary Format

```text
+----------------+----------------+----------------+--------------------+--------------------+
|  Magic (4B)    |    IV (12B)    |  GCM Tag (16B) |  CipherLen (4B BE) |  Ciphertext (N B)  |
|  "SENC"        | Unique/Crypto  | Auth Integrity | Length in Bytes    | AES-256-GCM Enc    |
+----------------+----------------+----------------+--------------------+--------------------+
```

- **Algorithm:** AES-256-GCM with 96-bit initialization vectors and 128-bit authentication tags.
- **IV Generation:** Cryptographically secure random nonces generated via OpenSSL `RAND_bytes`.
- **Integrity Enforcement:** Any modification to ciphertext, IV, or tag causes decryption failure (`LE_FAULT`).
- **Encoding:** String nodes store this envelope in Base64 format for transparent `configTree` compatibility.

---

## 6. TrustZone Security Provider HAL

The Security Provider HAL (`ISecurityProvider`) abstracts hardware secure world access:

1. **OP-TEE Hardware Provider (`OpteeProvider`):**
   - Connects to `/dev/tee0` via GlobalPlatform TEE Client API (`libteec`).
   - Utilizes TEE Secure Storage objects (`TEE_STORAGE_PRIVATE`).
2. **Machine-Bound Simulated Enclave (`SimulatedEnclaveProvider`):**
   - Fallback provider for environments without physical OP-TEE kernel drivers.
   - Master key and secure items are encrypted with machine-bound keys derived from `/etc/machine-id` using PBKDF2 (SHA-256, 10,000 iterations).
   - Storage directory `/var/config/cfgManager/secure_world` is enforced with strict POSIX permissions `0600` (read/write only by owning daemon).

---

## 7. Sample Client Usage

Below is a complete consumer example in C++23:

```cpp
#include "legato.h"
#include "interfaces.h"
#include "cfgId.hpp"

static void OnConfigChanged(uint32_t cfgId, const char* valueStr, void* contextPtr) {
    LE_INFO("[Notification] cfgId=0x%08X updated to '%s'", cfgId, valueStr);
}

COMPONENT_INIT {
    uint32_t rawId  = cfg::MakeId(cfg::Category::Raw,       cfg::Subsystem::Network,  0x0001);
    uint32_t sensId = cfg::MakeId(cfg::Category::Sensitive, cfg::Subsystem::Security, 0x0002);
    uint32_t secId  = cfg::MakeId(cfg::Category::Secure,    cfg::Subsystem::Security, 0x0003);

    // 1. Subscribe to changes
    cfgManager_AddChangeHandler(rawId, OnConfigChanged, nullptr);

    // 2. Set and Get RawData
    cfgManager_SetString(rawId, "rpi5-gateway.local");
    char buffer[256] = {0};
    cfgManager_GetString(rawId, buffer, sizeof(buffer));

    // 3. Set and Get SensitiveData (AES-256-GCM encrypted in storage)
    cfgManager_SetString(sensId, "SuperSecretWifiPassphrase");
    cfgManager_GetString(sensId, buffer, sizeof(buffer));

    // 4. Set and Get SecureData (Stored in TrustZone)
    cfgManager_SetString(secId, "HardwareAttestationSecret");
    cfgManager_GetString(secId, buffer, sizeof(buffer));
}
```

---

## 8. Verification & Test Suite

The test suite runs under Google Test inside the containerized build environment:

```bash
# Run unit tests and generate C2 branch coverage report
bash scripts/run-unit-tests.sh --coverage
```

### Coverage Summary

```
------------------------------------------------------------------------------
                           GCC Code Coverage Report
Directory: ../..
------------------------------------------------------------------------------
File                                    Branches   Taken  Cover   Missing
------------------------------------------------------------------------------
core/include/cfgId.hpp                        12      12   100%
core/src/cfgRouter.cpp                        29      21    72%
core/src/cfgManagerService.cpp               246     144    58%
crypto/src/cryptoEngine.cpp                  146      88    60%
events/src/eventDispatcher.cpp                50      30    60%
security/src/simulatedEnclaveProvider.cpp    184      96    52%
security/src/opteeProvider.cpp                26      10    38%
storage/src/configTreeBackend.cpp             26      19    73%
storage/src/memoryConfigBackend.cpp           76      50    65%
------------------------------------------------------------------------------
TOTAL                                        805     475    59%
------------------------------------------------------------------------------
lines: 87.7% (608 out of 693)
functions: 100.0% (83 out of 83)
branches: 59.0% (475 out of 805)
```

---

## 9. Building & Packaging

```bash
# Build CfgManager Legato Service package
bash scripts/build-cfgmanager.sh
# Output: build/rpi5/apps/cfgManager.rpi5.update

# Build CfgClient Sample App package
bash scripts/build-sample-client.sh
# Output: build/rpi5/apps/cfgClient.rpi5.update
```
