#include "legato.h"
#include "interfaces.h"
#include "cfgId.hpp"

#include <string>
#include <vector>
#include <iomanip>
#include <sstream>

namespace {

void OnConfigChanged(uint32_t cfgId, const char* valueStr, void* contextPtr) {
    LE_INFO(">>> [CLIENT EVENT] Configuration changed: cfgId=0x%08X, Value='%s'", cfgId, valueStr ? valueStr : "");
}

} // namespace

COMPONENT_INIT {
    LE_INFO("=================================================================");
    LE_INFO(" CfgManager Sample Client Application Started (C++23)");
    LE_INFO("=================================================================");

    // 1. Compose 32-bit cfgIds strictly abstracting all internal storage paths
    uint32_t rawId     = cfg::MakeId(cfg::Category::Raw,       cfg::Subsystem::Network,  0x0001);
    uint32_t sensId    = cfg::MakeId(cfg::Category::Sensitive, cfg::Subsystem::Security, 0x0002);
    uint32_t secureId  = cfg::MakeId(cfg::Category::Secure,    cfg::Subsystem::Security, 0x0003);
    uint32_t binId     = cfg::MakeId(cfg::Category::Sensitive, cfg::Subsystem::App,      0x0004);

    LE_INFO("Target cfgIds: Raw=0x%08X, Sensitive=0x%08X, Secure=0x%08X, Bin=0x%08X",
            rawId, sensId, secureId, binId);

    // 2. Subscribe to change events for rawId
    LE_INFO("==> Subscribing to change events for Raw cfgId 0x%08X...", rawId);
    cfgManager_ChangeHandlerRef_t subRef = cfgManager_AddChangeHandler(rawId, OnConfigChanged, nullptr);
    if (subRef) {
        LE_INFO("Successfully registered change handler ref %p", subRef);
    } else {
        LE_ERROR("Failed to register change handler!");
    }

    // 3. Test RawData (Stored in plain configTree)
    LE_INFO("==> Writing RawData string...");
    le_result_t res = cfgManager_SetString(rawId, "rpi5-edge-gateway.local");
    LE_ASSERT(res == LE_OK);

    char buf[512] = {0};
    res = cfgManager_GetString(rawId, buf, sizeof(buf));
    LE_ASSERT(res == LE_OK);
    LE_INFO("[RawData Result] Successfully read: '%s'", buf);

    // 4. Test SensitiveData (Encrypted via AES-256-GCM in configTree)
    LE_INFO("==> Writing SensitiveData (AES-256-GCM)...");
    res = cfgManager_SetString(sensId, "M4st3r_P@ssw0rd_K3y#2026!");
    LE_ASSERT(res == LE_OK);

    std::fill(std::begin(buf), std::end(buf), 0);
    res = cfgManager_GetString(sensId, buf, sizeof(buf));
    LE_ASSERT(res == LE_OK);
    LE_INFO("[SensitiveData Result] Decrypted transparently: '%s'", buf);

    // 5. Test SecureData (Stored in TrustZone / Hardware Enclave)
    LE_INFO("==> Writing SecureData (TrustZone Secure Storage)...");
    res = cfgManager_SetString(secureId, "HW-ROOT-OF-TRUST-TOKEN-994821");
    LE_ASSERT(res == LE_OK);

    std::fill(std::begin(buf), std::end(buf), 0);
    res = cfgManager_GetString(secureId, buf, sizeof(buf));
    LE_ASSERT(res == LE_OK);
    LE_INFO("[SecureData Result] Retrieved from TrustZone: '%s'", buf);

    // 6. Test Binary Payload (Sensitive Binary Blob)
    LE_INFO("==> Writing Binary SensitiveData blob...");
    const uint8_t binaryBlob[] = {0x10, 0x20, 0x30, 0x40, 0x50, 0xDE, 0xAD, 0xBE, 0xEF};
    res = cfgManager_SetBinary(binId, binaryBlob, sizeof(binaryBlob));
    LE_ASSERT(res == LE_OK);

    uint8_t outBlob[128] = {0};
    size_t outSize = sizeof(outBlob);
    res = cfgManager_GetBinary(binId, outBlob, &outSize);
    LE_ASSERT(res == LE_OK);
    LE_ASSERT(outSize == sizeof(binaryBlob));

    std::ostringstream hexStream;
    for (size_t i = 0; i < outSize; ++i) {
        hexStream << "0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                  << static_cast<int>(outBlob[i]) << " ";
    }
    LE_INFO("[Binary Result] Retrieved %zu bytes: [ %s]", outSize, hexStream.str().c_str());

    // 7. Update RawData to trigger asynchronous pub/sub notification
    LE_INFO("==> Updating RawData to trigger Pub/Sub change notification...");
    res = cfgManager_SetString(rawId, "rpi5-edge-gateway-RECONFIGURED.local");
    LE_ASSERT(res == LE_OK);

    LE_INFO("=================================================================");
    LE_INFO(" CfgManager Sample Client Demo Complete - ALL CHECKS PASSED!");
    LE_INFO("=================================================================");
}
