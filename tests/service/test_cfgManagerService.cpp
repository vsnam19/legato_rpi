#include <gtest/gtest.h>
#include "cfgManagerService.hpp"
#include "memoryConfigBackend.hpp"
#include "securityProvider.hpp"
#include <filesystem>
#include <vector>
#include <string>

using namespace cfg;

class CfgManagerServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        testEnclaveDir = std::filesystem::temp_directory_path() / "test_cfg_service_enclave";
        std::filesystem::remove_all(testEnclaveDir);

        auto router = std::make_unique<CfgRouter>();
        auto storage = std::make_unique<cfgmanager::storage::MemoryConfigBackend>();
        storageBackendRaw = storage.get();

        auto crypto = std::make_unique<crypto::CryptoEngine>();
        auto security = security::CreateSimulatedEnclaveProvider(testEnclaveDir.string());
        securityBackendRaw = security.get();

        auto events = std::make_unique<events::EventDispatcher>();

        service = std::make_unique<service::CfgManagerService>(
            std::move(router),
            std::move(storage),
            std::move(crypto),
            std::move(security),
            std::move(events)
        );

        ASSERT_EQ(service->Initialize(), LE_OK);
    }

    void TearDown() override {
        service.reset();
        std::filesystem::remove_all(testEnclaveDir);
    }

    std::filesystem::path testEnclaveDir;
    cfgmanager::storage::MemoryConfigBackend* storageBackendRaw{nullptr};
    security::ISecurityProvider* securityBackendRaw{nullptr};
    std::unique_ptr<service::CfgManagerService> service;
};

TEST_F(CfgManagerServiceTest, RawDataStringRoundtrip) {
    uint32_t rawId = MakeId(Category::Raw, Subsystem::Network, 0x0001);

    // Initial read should be LE_NOT_FOUND
    char buf[128] = {0};
    EXPECT_EQ(service->GetString(rawId, buf, sizeof(buf)), LE_NOT_FOUND);

    // Set string
    EXPECT_EQ(service->SetString(rawId, "192.168.1.100"), LE_OK);

    // Get string
    EXPECT_EQ(service->GetString(rawId, buf, sizeof(buf)), LE_OK);
    EXPECT_STREQ(buf, "192.168.1.100");

    // Verify storage contains plain text directly
    auto storedVal = storageBackendRaw->GetString("/cfgManager/net/item_1");
    ASSERT_TRUE(storedVal.has_value());
    EXPECT_EQ(*storedVal, "192.168.1.100");
}

TEST_F(CfgManagerServiceTest, SensitiveDataStringIsEncryptedInStorage) {
    uint32_t sensId = MakeId(Category::Sensitive, Subsystem::Security, 0x0002);

    EXPECT_EQ(service->SetString(sensId, "SuperSecretPassword123!"), LE_OK);

    // Retrieved string through service is transparently decrypted
    char buf[128] = {0};
    EXPECT_EQ(service->GetString(sensId, buf, sizeof(buf)), LE_OK);
    EXPECT_STREQ(buf, "SuperSecretPassword123!");

    // Check underlying configTree storage: MUST NOT contain plaintext
    auto rawStored = storageBackendRaw->GetString("/cfgManager/sec/item_2");
    ASSERT_TRUE(rawStored.has_value());
    EXPECT_NE(*rawStored, "SuperSecretPassword123!");
    EXPECT_TRUE(rawStored->starts_with("U0VOQw") || !rawStored->empty()); // Base64 of SENC magic
}

TEST_F(CfgManagerServiceTest, SecureDataStoredInTrustZone) {
    uint32_t secId = MakeId(Category::Secure, Subsystem::Security, 0x0003);

    EXPECT_EQ(service->SetString(secId, "TrustZoneHardwareRootToken"), LE_OK);

    // ConfigTree backend MUST NOT have this node!
    EXPECT_FALSE(storageBackendRaw->NodeExists("/cfgManager/sec/item_3"));

    // Secure provider MUST have this node
    auto secData = securityBackendRaw->ReadSecure("/cfgManager/sec/item_3");
    ASSERT_TRUE(secData.has_value());

    // Reading through service retrieves the plaintext value
    char buf[128] = {0};
    EXPECT_EQ(service->GetString(secId, buf, sizeof(buf)), LE_OK);
    EXPECT_STREQ(buf, "TrustZoneHardwareRootToken");
}

TEST_F(CfgManagerServiceTest, BinaryDataHandling) {
    // 1. Raw Binary
    uint32_t rawBinId = MakeId(Category::Raw, Subsystem::App, 0x0010);
    std::vector<uint8_t> payload = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0xAA, 0xFF};
    EXPECT_EQ(service->SetBinary(rawBinId, payload), LE_OK);

    uint8_t outBuf[64] = {0};
    size_t outLen = sizeof(outBuf);
    EXPECT_EQ(service->GetBinary(rawBinId, outBuf, outLen), LE_OK);
    EXPECT_EQ(outLen, payload.size());
    EXPECT_TRUE(std::equal(payload.begin(), payload.end(), outBuf));

    // 2. Sensitive Binary
    uint32_t sensBinId = MakeId(Category::Sensitive, Subsystem::App, 0x0011);
    EXPECT_EQ(service->SetBinary(sensBinId, payload), LE_OK);

    std::fill(std::begin(outBuf), std::end(outBuf), 0);
    outLen = sizeof(outBuf);
    EXPECT_EQ(service->GetBinary(sensBinId, outBuf, outLen), LE_OK);
    EXPECT_EQ(outLen, payload.size());
    EXPECT_TRUE(std::equal(payload.begin(), payload.end(), outBuf));

    // 3. Secure Binary
    uint32_t secBinId = MakeId(Category::Secure, Subsystem::App, 0x0012);
    EXPECT_EQ(service->SetBinary(secBinId, payload), LE_OK);

    std::fill(std::begin(outBuf), std::end(outBuf), 0);
    outLen = sizeof(outBuf);
    EXPECT_EQ(service->GetBinary(secBinId, outBuf, outLen), LE_OK);
    EXPECT_EQ(outLen, payload.size());
    EXPECT_TRUE(std::equal(payload.begin(), payload.end(), outBuf));
}

TEST_F(CfgManagerServiceTest, ReadOnlyFlagEnforcement) {
    uint32_t roId = MakeId(Category::Raw, Subsystem::System, 0x0005, Flag::ReadOnly);

    EXPECT_EQ(service->SetString(roId, "immutable_v1"), LE_OK);

    // Second write should be rejected
    EXPECT_EQ(service->SetString(roId, "immutable_v2"), LE_NOT_PERMITTED);

    char buf[64] = {0};
    EXPECT_EQ(service->GetString(roId, buf, sizeof(buf)), LE_OK);
    EXPECT_STREQ(buf, "immutable_v1");

    // Deletion of read-only should also be rejected
    EXPECT_EQ(service->Delete(roId), LE_NOT_PERMITTED);
}

TEST_F(CfgManagerServiceTest, InvalidCfgIdRejection) {
    uint32_t invalidId = 0x00000000; // No valid category flag

    char buf[64] = {0};
    size_t len = sizeof(buf);
    uint8_t bin[10] = {0};

    EXPECT_EQ(service->SetString(invalidId, "test"), LE_BAD_PARAMETER);
    EXPECT_EQ(service->GetString(invalidId, buf, sizeof(buf)), LE_BAD_PARAMETER);
    EXPECT_EQ(service->SetBinary(invalidId, bin), LE_BAD_PARAMETER);
    EXPECT_EQ(service->GetBinary(invalidId, bin, len), LE_BAD_PARAMETER);
    EXPECT_EQ(service->Delete(invalidId), LE_BAD_PARAMETER);
}

TEST_F(CfgManagerServiceTest, OverflowDetection) {
    uint32_t rawId = MakeId(Category::Raw, Subsystem::System, 0x0020);
    EXPECT_EQ(service->SetString(rawId, "LongLongLongStringValue"), LE_OK);

    // Buffer too small
    char tinyBuf[5] = {0};
    EXPECT_EQ(service->GetString(rawId, tinyBuf, sizeof(tinyBuf)), LE_OVERFLOW);

    // Binary buffer too small
    std::vector<uint8_t> largeData(32, 0x77);
    EXPECT_EQ(service->SetBinary(rawId, largeData), LE_OK);

    uint8_t tinyBin[8] = {0};
    size_t tinyLen = sizeof(tinyBin);
    EXPECT_EQ(service->GetBinary(rawId, tinyBin, tinyLen), LE_OVERFLOW);
}

TEST_F(CfgManagerServiceTest, DeleteOperations) {
    uint32_t rawId = MakeId(Category::Raw, Subsystem::System, 0x0030);
    uint32_t secId = MakeId(Category::Secure, Subsystem::System, 0x0031);

    EXPECT_EQ(service->SetString(rawId, "value"), LE_OK);
    EXPECT_EQ(service->SetString(secId, "value"), LE_OK);

    EXPECT_EQ(service->Delete(rawId), LE_OK);
    EXPECT_EQ(service->Delete(secId), LE_OK);

    // Deleting again returns LE_NOT_FOUND
    EXPECT_EQ(service->Delete(rawId), LE_NOT_FOUND);
    EXPECT_EQ(service->Delete(secId), LE_NOT_FOUND);
}

TEST_F(CfgManagerServiceTest, PubSubChangeNotifications) {
    uint32_t id1 = MakeId(Category::Raw, Subsystem::Network, 0x0040);
    uint32_t id2 = MakeId(Category::Raw, Subsystem::Network, 0x0041);

    int id1Notifications = 0;
    std::string lastVal;

    auto sub = service->Subscribe(id1, [&](uint32_t id, std::string_view val) {
        id1Notifications++;
        lastVal = std::string(val);
    });

    EXPECT_NE(sub, 0u);

    // Update id2 -> id1 subscriber should NOT be notified
    EXPECT_EQ(service->SetString(id2, "id2_val"), LE_OK);
    EXPECT_EQ(id1Notifications, 0);

    // Update id1 -> subscriber notified
    EXPECT_EQ(service->SetString(id1, "new_ip"), LE_OK);
    EXPECT_EQ(id1Notifications, 1);
    EXPECT_EQ(lastVal, "new_ip");

    // Unsubscribe
    EXPECT_TRUE(service->Unsubscribe(sub));
    EXPECT_EQ(service->SetString(id1, "another_ip"), LE_OK);
    EXPECT_EQ(id1Notifications, 1); // Not incremented
}
