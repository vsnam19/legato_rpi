#include <gtest/gtest.h>
#include "IConfigBackend.hpp"
#include "memoryConfigBackend.hpp"
#include "configTreeBackend.hpp"
#include <vector>
#include <string>

using namespace cfgmanager::storage;

class MemoryConfigBackendTest : public ::testing::Test {
protected:
    void SetUp() override {
        backend = std::make_unique<MemoryConfigBackend>();
    }

    std::unique_ptr<MemoryConfigBackend> backend;
};

TEST_F(MemoryConfigBackendTest, SetAndGetString) {
    EXPECT_FALSE(backend->NodeExists("/system/network/hostname"));
    EXPECT_FALSE(backend->GetString("/system/network/hostname").has_value());

    EXPECT_TRUE(backend->SetString("/system/network/hostname", "rpi5-gateway"));
    EXPECT_TRUE(backend->NodeExists("/system/network/hostname"));

    auto val = backend->GetString("/system/network/hostname");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, "rpi5-gateway");
}

TEST_F(MemoryConfigBackendTest, OverwriteString) {
    EXPECT_TRUE(backend->SetString("/test/key", "val1"));
    EXPECT_EQ(backend->GetString("/test/key").value(), "val1");

    EXPECT_TRUE(backend->SetString("/test/key", "val2"));
    EXPECT_EQ(backend->GetString("/test/key").value(), "val2");
}

TEST_F(MemoryConfigBackendTest, SetAndGetBinary) {
    std::vector<uint8_t> payload = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03};

    EXPECT_FALSE(backend->NodeExists("/sec/token"));
    EXPECT_TRUE(backend->SetBinary("/sec/token", payload));
    EXPECT_TRUE(backend->NodeExists("/sec/token"));

    auto retrieved = backend->GetBinary("/sec/token");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(*retrieved, payload);
}

TEST_F(MemoryConfigBackendTest, DeleteNode) {
    EXPECT_TRUE(backend->SetString("/node/to/delete", "goodbye"));
    EXPECT_TRUE(backend->NodeExists("/node/to/delete"));

    EXPECT_TRUE(backend->DeleteNode("/node/to/delete"));
    EXPECT_FALSE(backend->NodeExists("/node/to/delete"));
    EXPECT_FALSE(backend->GetString("/node/to/delete").has_value());

    // Deleting again should return false (not found)
    EXPECT_FALSE(backend->DeleteNode("/node/to/delete"));
}

TEST_F(MemoryConfigBackendTest, DeleteBinaryNode) {
    std::vector<uint8_t> bin = {1, 2, 3};
    EXPECT_TRUE(backend->SetBinary("/bin/key", bin));
    EXPECT_TRUE(backend->NodeExists("/bin/key"));

    EXPECT_TRUE(backend->DeleteNode("/bin/key"));
    EXPECT_FALSE(backend->NodeExists("/bin/key"));
    EXPECT_FALSE(backend->GetBinary("/bin/key").has_value());
}

TEST_F(MemoryConfigBackendTest, ClearAndSize) {
    EXPECT_EQ(backend->Size(), 0);
    EXPECT_TRUE(backend->SetString("/k1", "v1"));
    EXPECT_TRUE(backend->SetBinary("/k2", std::vector<uint8_t>{4, 5}));
    EXPECT_EQ(backend->Size(), 2);

    backend->Clear();
    EXPECT_EQ(backend->Size(), 0);
    EXPECT_FALSE(backend->NodeExists("/k1"));
    EXPECT_FALSE(backend->NodeExists("/k2"));
}

TEST_F(MemoryConfigBackendTest, EmptyPathHandling) {
    EXPECT_FALSE(backend->SetString("", "val"));
    EXPECT_FALSE(backend->GetString("").has_value());
    EXPECT_FALSE(backend->SetBinary("", std::vector<uint8_t>{1}));
    EXPECT_FALSE(backend->GetBinary("").has_value());
    EXPECT_FALSE(backend->DeleteNode(""));
    EXPECT_FALSE(backend->NodeExists(""));
}

class ConfigTreeBackendTest : public ::testing::Test {
protected:
    void SetUp() override {
        backend = std::make_unique<ConfigTreeBackend>("testTree");
    }

    std::unique_ptr<ConfigTreeBackend> backend;
};

TEST_F(ConfigTreeBackendTest, TreeNameCheck) {
    EXPECT_EQ(backend->GetTreeName(), "testTree");
}

TEST_F(ConfigTreeBackendTest, BasicOperationsDelegation) {
    EXPECT_FALSE(backend->NodeExists("/device/serial"));
    EXPECT_TRUE(backend->SetString("/device/serial", "SN-987654321"));
    EXPECT_TRUE(backend->NodeExists("/device/serial"));

    auto val = backend->GetString("/device/serial");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, "SN-987654321");

    std::vector<uint8_t> blob = {0xAA, 0xBB, 0xCC};
    EXPECT_TRUE(backend->SetBinary("/device/blob", blob));
    auto retBlob = backend->GetBinary("/device/blob");
    ASSERT_TRUE(retBlob.has_value());
    EXPECT_EQ(*retBlob, blob);

    EXPECT_TRUE(backend->DeleteNode("/device/serial"));
    EXPECT_FALSE(backend->NodeExists("/device/serial"));
}

TEST_F(ConfigTreeBackendTest, EmptyPathHandling) {
    EXPECT_FALSE(backend->SetString("", "val"));
    EXPECT_FALSE(backend->GetString("").has_value());
    EXPECT_FALSE(backend->SetBinary("", std::vector<uint8_t>{1}));
    EXPECT_FALSE(backend->GetBinary("").has_value());
    EXPECT_FALSE(backend->DeleteNode(""));
    EXPECT_FALSE(backend->NodeExists(""));
}

