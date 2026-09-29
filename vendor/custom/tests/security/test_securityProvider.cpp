#include <gtest/gtest.h>
#include "securityProvider.hpp"
#include <filesystem>
#include <sys/stat.h>

using namespace cfg::security;

class SecurityProviderTest : public ::testing::Test {
protected:
    std::string testDir_ = "/tmp/test_cfg_sec_provider";

    void SetUp() override {
        std::filesystem::remove_all(testDir_);
    }

    void TearDown() override {
        std::filesystem::remove_all(testDir_);
    }
};

TEST_F(SecurityProviderTest, FallbackProviderInitialization) {
    auto provider = CreateSecurityProvider(testDir_);
    ASSERT_NE(provider, nullptr);
    EXPECT_TRUE(provider->IsAvailable());
    // On systems without /dev/tee0, it must fall back to SimulatedEnclave
    EXPECT_EQ(provider->GetType(), ProviderType::SimulatedEnclave);
}

TEST_F(SecurityProviderTest, MasterKeyGenerationAndPersistence) {
    auto provider1 = CreateSecurityProvider(testDir_);
    ASSERT_NE(provider1, nullptr);

    auto key1 = provider1->GetMasterKey();
    ASSERT_TRUE(key1.has_value());
    EXPECT_EQ(key1->size(), 32); // 256-bit key

    // Second call on same provider returns same key
    auto key1Repeat = provider1->GetMasterKey();
    ASSERT_TRUE(key1Repeat.has_value());
    EXPECT_EQ(*key1, *key1Repeat);

    // New provider instance on same storage path restores same key
    auto provider2 = CreateSecurityProvider(testDir_);
    ASSERT_NE(provider2, nullptr);
    auto key2 = provider2->GetMasterKey();
    ASSERT_TRUE(key2.has_value());
    EXPECT_EQ(*key1, *key2);
}

TEST_F(SecurityProviderTest, SecureStorageWriteReadDelete) {
    auto provider = CreateSecurityProvider(testDir_);
    ASSERT_NE(provider, nullptr);

    std::string key = "prov_token";
    std::vector<uint8_t> payload = { 0xDE, 0xAD, 0xBE, 0xEF, 0x42 };

    // Initial read non-existent
    EXPECT_FALSE(provider->ReadSecure(key).has_value());

    // Write
    EXPECT_TRUE(provider->WriteSecure(key, payload));

    // Read back
    auto readBack = provider->ReadSecure(key);
    ASSERT_TRUE(readBack.has_value());
    EXPECT_EQ(*readBack, payload);

    // Overwrite
    std::vector<uint8_t> newPayload = { 0x11, 0x22 };
    EXPECT_TRUE(provider->WriteSecure(key, newPayload));
    auto updated = provider->ReadSecure(key);
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(*updated, newPayload);

    // Delete
    EXPECT_TRUE(provider->DeleteSecure(key));
    EXPECT_FALSE(provider->ReadSecure(key).has_value());

    // Delete non-existent
    EXPECT_FALSE(provider->DeleteSecure(key));
}

TEST_F(SecurityProviderTest, FilePermissionsCheck) {
    auto provider = CreateSecurityProvider(testDir_);
    ASSERT_NE(provider, nullptr);

    std::vector<uint8_t> payload = { 1, 2, 3 };
    EXPECT_TRUE(provider->WriteSecure("item_test", payload));

    std::string filePath = testDir_ + "/item_test.sec";
    struct stat st{};
    ASSERT_EQ(stat(filePath.c_str(), &st), 0);

    // Must be 0600 (read/write only by owner)
    mode_t perm = st.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
    EXPECT_EQ(perm, S_IRUSR | S_IWUSR);
}

TEST_F(SecurityProviderTest, EdgeCases) {
    auto provider = CreateSecurityProvider(testDir_);
    ASSERT_NE(provider, nullptr);

    // Empty key should fail
    std::vector<uint8_t> payload = { 1 };
    EXPECT_FALSE(provider->WriteSecure("", payload));
    EXPECT_FALSE(provider->ReadSecure("").has_value());
    EXPECT_FALSE(provider->DeleteSecure(""));

    // Empty payload write should succeed
    EXPECT_TRUE(provider->WriteSecure("empty_key", {}));
    auto readEmpty = provider->ReadSecure("empty_key");
    ASSERT_TRUE(readEmpty.has_value());
    EXPECT_TRUE(readEmpty->empty());
}

TEST_F(SecurityProviderTest, OpteeProviderDirect) {
    auto optee = CreateOpteeProvider();
    ASSERT_NE(optee, nullptr);
    EXPECT_EQ(optee->GetType(), ProviderType::Optee);

    // In environment without /dev/tee0, it must be unavailable and reject ops safely
    if (!optee->IsAvailable()) {
        EXPECT_FALSE(optee->GetMasterKey().has_value());
        std::vector<uint8_t> data = {1, 2};
        EXPECT_FALSE(optee->WriteSecure("key", data));
        EXPECT_FALSE(optee->WriteSecure("", data));
        EXPECT_FALSE(optee->ReadSecure("key").has_value());
        EXPECT_FALSE(optee->ReadSecure("").has_value());
        EXPECT_FALSE(optee->DeleteSecure("key"));
        EXPECT_FALSE(optee->DeleteSecure(""));
    }
}

