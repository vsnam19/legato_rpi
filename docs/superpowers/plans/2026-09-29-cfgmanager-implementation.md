# Legato CfgManager Service Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement a production-grade Legato configuration management service (`CfgManager`) in C++23 providing 3-tier data security (`RAW`, `SENSITIVE` via AES-256-GCM, `SECURE` via TrustZone) and pub/sub change notifications, abstracting all storage paths behind a composite 32-bit `cfgId`, verified with Google Test unit tests at C2 branch coverage.

**Architecture:** A decoupled service layer built on Legato IPC (`cfgManager.api`). `CfgRouter` decodes 32-bit `cfgId` into internal keys without leaking paths to clients. `CryptoEngine` applies AES-256-GCM envelope encryption using a TrustZone-sealed master key from `SecurityProvider` (supporting OP-TEE `/dev/tee0` and machine-bound Software Secure World fallback). `EventDispatcher` routes change notifications to subscribed clients.

**Tech Stack:** C++23 (`-std=c++23`, GCC 12), Legato Application Framework (IPC IDL / `ifgen`, `le_cfg`), OpenSSL 3.0 (`libcrypto`), Google Test (GTest) with gcov branch coverage, Docker containerized cross-compilation.

**Spec:** `docs/superpowers/specs/2026-09-29-cfgmanager-design.md`

## Global Constraints

- Implementation language must be C++23 (`-std=c++23`), compiled via GCC 12 inside Docker.
- Consumers must interact strictly with 32-bit `cfgId` and must never receive or specify internal filesystem or `configTree` paths.
- All new code must be verified with Google Test achieving C2 branch/condition coverage.
- All builds must execute containerized via Docker (`docker/run-docker-build.sh`).
- Upstream Legato AF submodule (`submodules/legato-af`) must remain untouched; service must reside in `apps/cfgManager` and `interfaces/`.

## Review Focus

1. **Tag Tampering in SensitiveData:** Any bit modification to the GCM authentication tag or ciphertext in `configTree` must trigger an immediate authentication failure (`LE_FAULT`) and zeroize output buffers rather than returning corrupted data.
2. **Path Leakage Prevention:** The IPC interface (`cfgManager.api`) must contain zero string path parameters for read, write, or subscribe operations.
3. **Invalid cfgId Rejection:** Malformed `cfgId`s (e.g. conflicting flags like `RAW | SECURE`, unmapped subsystems, or reserved item IDs) must be rejected with `LE_BAD_PARAMETER` before any storage or crypto operation is invoked.
4. **TrustZone Hardware Fallback:** If `/dev/tee0` is not present (standard Raspberry Pi OS), the service must transparently initialize the machine-bound Software Secure World simulator without crashing or dropping security.
5. **Subscription Isolation:** Subscribing to `cfgId_A` must never trigger callbacks when `cfgId_B` is modified, and unsubscribing must immediately cease all notifications.

---

### Task 1: Build & Test Infrastructure for C++23 & Google Test

**Files:**
- Modify: `docker/Dockerfile`
- Create: `tests/CMakeLists.txt`
- Create: `tests/test_main.cpp`
- Create: `scripts/run-unit-tests.sh`

**Interfaces:**
- Produces: Runnable Google Test harness executing inside Docker container with gcov branch coverage enabled.

- [ ] **Step 1: Update `docker/Dockerfile` with Google Test dependencies**
  Add `libgtest-dev` and `libgmock-dev` to `docker/Dockerfile` and rebuild image via `bash docker/run-docker-build.sh true`.

- [ ] **Step 2: Create initial test runner and CMakeLists.txt**
  Write `tests/CMakeLists.txt` and `tests/test_main.cpp` configured for C++23 with `-fprofile-arcs -ftest-coverage`.
  Write `scripts/run-unit-tests.sh` executing CMake build and ctest inside the container.

- [ ] **Step 3: Run test runner to verify harness**
  Run: `bash scripts/run-unit-tests.sh`
  Expected: GTest initializes, runs smoke test, reports 1 passed test.

- [ ] **Step 4: Commit**
  ```bash
  git add docker/Dockerfile tests/ scripts/run-unit-tests.sh
  git commit -m "feat(test): add Google Test infrastructure with C++23 and coverage support"
  ```

---

### Task 2: Composite `cfgId` Bitfield & Helper Library (`core/cfgId.hpp`)

**Files:**
- Create: `core/include/cfgId.hpp`
- Test: `tests/core/test_cfgId.cpp`

**Interfaces:**
- Produces: Header-only C++23 `cfg::MakeId`, `cfg::GetCategory`, `cfg::GetSubsystem`, `cfg::GetItemId`, `cfg::ValidateId`.

- [ ] **Step 1: Write failing unit test in `tests/core/test_cfgId.cpp`**
  Write tests covering:
  - Valid ID construction for all categories (`Raw`, `Sensitive`, `Secure`).
  - Bit extraction for Subsystem and Item ID boundaries (`0x0000`, `0xFFFF`).
  - Validation failures: conflicting flags, invalid category, out-of-range subsystems.

- [ ] **Step 2: Run test to confirm failure**
  Run: `bash scripts/run-unit-tests.sh`
  Expected: Fails to compile because `cfgId.hpp` does not exist yet.

- [ ] **Step 3: Implement `core/include/cfgId.hpp`**
  Implement constexpr functions, enums, bitmask helpers, and validation logic in C++23.

- [ ] **Step 4: Run tests and verify C2 coverage**
  Run: `bash scripts/run-unit-tests.sh`
  Expected: All tests pass with 100% branch coverage on `cfgId.hpp`.

- [ ] **Step 5: Commit**
  ```bash
  git add core/include/cfgId.hpp tests/core/test_cfgId.cpp
  git commit -m "feat(core): implement composite cfgId bitfield and validator in C++23"
  ```

---

### Task 3: CfgRouter & Internal Storage Path Mapper (`core/cfgRouter`)

**Files:**
- Create: `core/include/cfgRouter.hpp`
- Create: `core/src/cfgRouter.cpp`
- Test: `tests/core/test_cfgRouter.cpp`

**Interfaces:**
- Produces: `CfgRouter` class mapping valid `cfgId`s to canonical internal keys (e.g. `"/cfgManager/net/item_10"`).

- [ ] **Step 1: Write failing unit test in `tests/core/test_cfgRouter.cpp`**
  Tests covering:
  - Resolution of valid `cfgId` to internal storage key.
  - Isolation between subsystems (system, network, security, app).
  - Rejection of invalid/unregistered `cfgId`.
  - Category retrieval for routing.

- [ ] **Step 2: Run test to confirm failure**
  Run: `bash scripts/run-unit-tests.sh`

- [ ] **Step 3: Implement `core/include/cfgRouter.hpp` and `core/src/cfgRouter.cpp`**
  Implement router class using C++23 `std::string_view`, `std::optional`, and `std::unordered_map`.

- [ ] **Step 4: Run tests and verify pass**
  Run: `bash scripts/run-unit-tests.sh`
  Expected: All `CfgRouter` tests pass with all branch paths covered.

- [ ] **Step 5: Commit**
  ```bash
  git add core/ tests/core/test_cfgRouter.cpp
  git commit -m "feat(core): implement CfgRouter internal path resolver in C++23"
  ```

---

### Task 4: TrustZone Security Provider HAL (`security/`)

**Files:**
- Create: `security/include/ISecurityProvider.hpp`
- Create: `security/include/securityProvider.hpp`
- Create: `security/src/simulatedEnclaveProvider.cpp`
- Create: `security/src/opteeProvider.cpp`
- Create: `security/src/securityProviderFactory.cpp`
- Test: `tests/security/test_securityProvider.cpp`

**Interfaces:**
- Produces: `ISecurityProvider` interface with `WriteSecure`, `ReadSecure`, `DeleteSecure`, and `GetMasterKey`.

- [ ] **Step 1: Write failing unit test in `tests/security/test_securityProvider.cpp`**
  Tests covering:
  - Master key generation and persistence across provider instances.
  - Writing and reading secure byte payloads.
  - Deleting secure items.
  - Reading non-existent item (returns failure).
  - Software enclave fallback behavior with file permission verification (`0600`).

- [ ] **Step 2: Run test to confirm failure**
  Run: `bash scripts/run-unit-tests.sh`

- [ ] **Step 3: Implement Security Provider classes**
  - `ISecurityProvider.hpp`: Virtual interface.
  - `simulatedEnclaveProvider.cpp`: PBKDF2 machine-bound sealing, atomic file writes with `fsync`, `0600` permissions.
  - `opteeProvider.cpp`: OP-TEE `/dev/tee0` client driver.
  - `securityProviderFactory.cpp`: Detection logic auto-selecting OP-TEE or fallback simulator.

- [ ] **Step 4: Run tests and verify pass**
  Run: `bash scripts/run-unit-tests.sh`
  Expected: All security provider unit tests pass with all branches exercised.

- [ ] **Step 5: Commit**
  ```bash
  git add security/ tests/security/test_securityProvider.cpp
  git commit -m "feat(security): implement TrustZone security provider and software enclave in C++23"
  ```

---

### Task 5: Authenticated Crypto Engine with AES-256-GCM (`crypto/`)

**Files:**
- Create: `crypto/include/cryptoEngine.hpp`
- Create: `crypto/src/cryptoEngine.cpp`
- Test: `tests/crypto/test_cryptoEngine.cpp`

**Interfaces:**
- Produces: `CryptoEngine` providing `Encrypt`, `Decrypt`, `EncodeEnvelope`, `DecodeEnvelope` using OpenSSL 3.0 EVP AES-256-GCM.

- [ ] **Step 1: Write failing unit test in `tests/crypto/test_cryptoEngine.cpp`**
  Tests covering:
  - Plaintext to ciphertext roundtrip (both string and binary).
  - Unique IV generation per encryption.
  - Tamper detection: flip 1 bit in IV -> decryption fails.
  - Tamper detection: flip 1 bit in Auth Tag -> decryption fails.
  - Tamper detection: flip 1 bit in Ciphertext -> decryption fails.
  - Corrupted envelope magic rejection.
  - Empty data and maximum payload (4096 bytes) handling.

- [ ] **Step 2: Run test to confirm failure**
  Run: `bash scripts/run-unit-tests.sh`

- [ ] **Step 3: Implement `crypto/include/cryptoEngine.hpp` and `crypto/src/cryptoEngine.cpp`**
  Implement AES-256-GCM encryption/decryption, random IV generation via `RAND_bytes`, `SENC` binary envelope packing, and Base64 encoding.

- [ ] **Step 4: Run tests and verify pass**
  Run: `bash scripts/run-unit-tests.sh`
  Expected: All crypto tests pass with 100% branch and error-case coverage.

- [ ] **Step 5: Commit**
  ```bash
  git add crypto/ tests/crypto/test_cryptoEngine.cpp
  git commit -m "feat(crypto): implement AES-256-GCM authenticated crypto engine in C++23"
  ```

---

### Task 6: Event Dispatcher for Pub/Sub Change Notifications (`events/`)

**Files:**
- Create: `events/include/eventDispatcher.hpp`
- Create: `events/src/eventDispatcher.cpp`
- Test: `tests/events/test_eventDispatcher.cpp`

**Interfaces:**
- Produces: `EventDispatcher` managing subscriber registration, unsubscription, and event broadcasting.

- [ ] **Step 1: Write failing unit test in `tests/events/test_eventDispatcher.cpp`**
  Tests covering:
  - Register subscriber for specific `cfgId` -> notify -> callback received.
  - Unsubscribe -> notify -> callback NOT received.
  - Multiple subscribers on same `cfgId` -> all receive notification.
  - Subscribers on different `cfgId`s -> ensure no event leakage.
  - Thread-safe concurrency test.

- [ ] **Step 2: Run test to confirm failure**
  Run: `bash scripts/run-unit-tests.sh`

- [ ] **Step 3: Implement `events/include/eventDispatcher.hpp` and `events/src/eventDispatcher.cpp`**
  Implement event dispatcher using `std::mutex`, `std::unordered_multimap`, and RAII subscriber tokens.

- [ ] **Step 4: Run tests and verify pass**
  Run: `bash scripts/run-unit-tests.sh`
  Expected: All event dispatcher tests pass.

- [ ] **Step 5: Commit**
  ```bash
  git add events/ tests/events/test_eventDispatcher.cpp
  git commit -m "feat(events): implement thread-safe pub/sub event dispatcher in C++23"
  ```

---

### Task 7: Storage Backend & Legato `configTree` Adapter (`storage/`)

**Files:**
- Create: `storage/include/IConfigBackend.hpp`
- Create: `storage/include/configTreeBackend.hpp`
- Create: `storage/src/configTreeBackend.cpp`
- Create: `storage/src/memoryConfigBackend.cpp`
- Test: `tests/storage/test_storageBackend.cpp`

**Interfaces:**
- Produces: `IConfigBackend` abstraction with `ConfigTreeBackend` (for Legato runtime) and `MemoryConfigBackend` (for host unit tests).

- [ ] **Step 1: Write failing unit test in `tests/storage/test_storageBackend.cpp`**
  Tests covering:
  - Write, read, overwrite, and delete string and binary values.
  - Non-existent key handling (returns `LE_NOT_FOUND`).
  - Tree path prefix verification.

- [ ] **Step 2: Run test to confirm failure**
  Run: `bash scripts/run-unit-tests.sh`

- [ ] **Step 3: Implement storage backends**
  - `IConfigBackend.hpp`: Interface definition.
  - `memoryConfigBackend.cpp`: Fast in-memory map implementation for unit tests.
  - `configTreeBackend.cpp`: Legato `le_cfg` implementation for target runtime.

- [ ] **Step 4: Run tests and verify pass**
  Run: `bash scripts/run-unit-tests.sh`

- [ ] **Step 5: Commit**
  ```bash
  git add storage/ tests/storage/test_storageBackend.cpp
  git commit -m "feat(storage): implement configTree adapter and mock storage backend in C++23"
  ```

---

### Task 8: Service Integration & Legato Application Packaging (`apps/cfgManager`)

**Files:**
- Create: `interfaces/cfgManager.api`
- Create: `apps/cfgManager/Component.cdef`
- Create: `apps/cfgManager/cfgManager.adef`
- Create: `apps/cfgManager/server/cfgManagerServer.cpp`
- Create: `scripts/build-cfgmanager.sh`

**Interfaces:**
- Consumes: All core components (`core`, `crypto`, `security`, `storage`, `events`).
- Produces: Containerized Legato application package `build/rpi5/apps/cfgManager.rpi5.update`.

- [ ] **Step 1: Define `interfaces/cfgManager.api`**
  Write pure `cfgId` API definition without path parameters.

- [ ] **Step 2: Implement `apps/cfgManager/server/cfgManagerServer.cpp`**
  Connect Legato IPC handlers to `CfgRouter`, `ConfigTreeBackend`, `CryptoEngine`, `SecurityProvider`, and `EventDispatcher`.

- [ ] **Step 3: Write Component and App definitions (`Component.cdef`, `cfgManager.adef`)**
  Configure C++23 build flags (`cxxflags: { -std=c++23 }`), link with `libcrypto.so.3`, declare IPC server interface `provides: { api: { cfgManager = cfgManager.api } }`.

- [ ] **Step 4: Build Legato app package inside Docker container**
  Write and run `scripts/build-cfgmanager.sh`.
  Expected: Generates `build/rpi5/apps/cfgManager.rpi5.update`.
  Verify ELF architecture: `Class: ELF64`, `Machine: AArch64`.

- [ ] **Step 5: Commit**
  ```bash
  git add interfaces/ apps/cfgManager/ scripts/build-cfgmanager.sh
  git commit -m "feat(app): implement CfgManager Legato service application in C++23"
  ```

---

### Task 9: Sample Client Application & Integration Verification (`samples/cfgClient`)

**Files:**
- Create: `samples/cfgClient/clientComponent/Component.cdef`
- Create: `samples/cfgClient/clientComponent/client.cpp`
- Create: `samples/cfgClient/cfgClient.adef`
- Create: `scripts/build-sample-client.sh`

**Interfaces:**
- Produces: Client app `build/rpi5/apps/cfgClient.rpi5.update` verifying IPC calls and change notification callbacks.

- [ ] **Step 1: Implement `samples/cfgClient/clientComponent/client.cpp`**
  Client code in C++23:
  - Registers change handler for sensitive WiFi password `CFG_ID_WIFI_PASS`.
  - Sets RAW (`CFG_ID_DEVICE_NAME`), SENSITIVE (`CFG_ID_WIFI_PASS`), SECURE (`CFG_ID_PROV_TOKEN`).
  - Gets and asserts all values match original plaintext.
  - Verifies change callback execution.

- [ ] **Step 2: Write definitions and build script**
  Write `Component.cdef`, `cfgClient.adef`, and `scripts/build-sample-client.sh`.

- [ ] **Step 3: Build sample client inside Docker container**
  Run: `bash scripts/build-sample-client.sh`
  Expected: Generates `build/rpi5/apps/cfgClient.rpi5.update` (AArch64 ELF).

- [ ] **Step 4: Commit**
  ```bash
  git add samples/cfgClient/ scripts/build-sample-client.sh
  git commit -m "feat(sample): implement cfgClient sample verification app in C++23"
  ```

---

### Task 10: End-to-End Verification, C2 Coverage Report & Documentation

**Files:**
- Create: `docs/CFG_MANAGER.md`
- Modify: `README.md`

**Interfaces:**
- Produces: Full documentation of `CfgManager`, verified GTest test suite with C2 coverage report, and updated system overview.

- [ ] **Step 1: Run full unit test suite with gcov branch coverage report**
  Run: `bash scripts/run-unit-tests.sh --coverage`
  Verify that all branches in `core`, `crypto`, `security`, and `events` are covered.

- [ ] **Step 2: Write `docs/CFG_MANAGER.md`**
  Comprehensive guide: architecture, IPC API reference, `cfgId` composition rules, encryption details, TrustZone setup, and client integration guide.

- [ ] **Step 3: Update `README.md`**
  Add CfgManager service and sample client to system architecture diagram and quickstart guide.

- [ ] **Step 4: Commit**
  ```bash
  git add docs/CFG_MANAGER.md README.md
  git commit -m "docs: add comprehensive CfgManager service documentation and test report"
  ```
