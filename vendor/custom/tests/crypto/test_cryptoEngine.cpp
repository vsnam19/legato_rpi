#include <gtest/gtest.h>
#include "cryptoEngine.hpp"

using namespace cfg::crypto;

class CryptoEngineTest : public ::testing::Test {
protected:
    std::vector<uint8_t> testKey_ = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
    };
};

TEST_F(CryptoEngineTest, StringEncryptionDecryptionRoundtrip) {
    CryptoEngine engine(testKey_);
    std::string plaintext = "SuperSecretWiFiPassword@2026";

    auto encOpt = engine.EncryptString(plaintext);
    ASSERT_TRUE(encOpt.has_value());
    EXPECT_FALSE(encOpt->empty());
    EXPECT_NE(*encOpt, plaintext);

    auto decOpt = engine.DecryptString(*encOpt);
    ASSERT_TRUE(decOpt.has_value());
    EXPECT_EQ(*decOpt, plaintext);
}

TEST_F(CryptoEngineTest, BinaryEncryptionDecryptionRoundtrip) {
    CryptoEngine engine(testKey_);
    std::vector<uint8_t> raw = { 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0xFF, 0x42 };

    auto encOpt = engine.Encrypt(raw);
    ASSERT_TRUE(encOpt.has_value());
    EXPECT_GT(encOpt->size(), raw.size() + 32);

    auto decOpt = engine.Decrypt(*encOpt);
    ASSERT_TRUE(decOpt.has_value());
    EXPECT_EQ(*decOpt, raw);
}

TEST_F(CryptoEngineTest, UniqueIvPerEncryption) {
    CryptoEngine engine(testKey_);
    std::string text = "SameContentDifferentIv";

    auto enc1 = engine.EncryptString(text);
    auto enc2 = engine.EncryptString(text);

    ASSERT_TRUE(enc1.has_value() && enc2.has_value());
    EXPECT_NE(*enc1, *enc2); // Different IVs must result in completely different ciphertext

    EXPECT_EQ(engine.DecryptString(*enc1), text);
    EXPECT_EQ(engine.DecryptString(*enc2), text);
}

TEST_F(CryptoEngineTest, EmptyAndBoundaryData) {
    CryptoEngine engine(testKey_);

    // Empty string
    auto encEmpty = engine.EncryptString("");
    ASSERT_TRUE(encEmpty.has_value());
    auto decEmpty = engine.DecryptString(*encEmpty);
    ASSERT_TRUE(decEmpty.has_value());
    EXPECT_TRUE(decEmpty->empty());

    // 4096-byte boundary data
    std::vector<uint8_t> largeData(4096, 0x5A);
    auto encLarge = engine.Encrypt(largeData);
    ASSERT_TRUE(encLarge.has_value());
    auto decLarge = engine.Decrypt(*encLarge);
    ASSERT_TRUE(decLarge.has_value());
    EXPECT_EQ(*decLarge, largeData);
}

TEST_F(CryptoEngineTest, TamperDetectionFailsDecryption) {
    CryptoEngine engine(testKey_);
    std::vector<uint8_t> original = { 1, 2, 3, 4, 5, 6, 7, 8 };

    auto encOpt = engine.Encrypt(original);
    ASSERT_TRUE(encOpt.has_value());
    auto envelope = *encOpt;

    // Header structure:
    // Magic: [0..3]
    // IV:    [4..15]
    // Tag:   [16..31]
    // Len:   [32..35]
    // Data:  [36..]

    // 1. Corrupt Magic
    auto badMagic = envelope;
    badMagic[0] ^= 0xFF;
    EXPECT_FALSE(engine.Decrypt(badMagic).has_value());

    // 2. Corrupt IV
    auto badIv = envelope;
    badIv[5] ^= 0xFF;
    EXPECT_FALSE(engine.Decrypt(badIv).has_value());

    // 3. Corrupt Auth Tag
    auto badTag = envelope;
    badTag[20] ^= 0xFF;
    EXPECT_FALSE(engine.Decrypt(badTag).has_value());

    // 4. Corrupt Ciphertext payload
    auto badCipher = envelope;
    badCipher[36] ^= 0xFF;
    EXPECT_FALSE(engine.Decrypt(badCipher).has_value());

    // 5. Truncated envelope
    std::vector<uint8_t> truncated(envelope.begin(), envelope.begin() + 20);
    EXPECT_FALSE(engine.Decrypt(truncated).has_value());
}

TEST_F(CryptoEngineTest, InvalidKeyRejection) {
    std::vector<uint8_t> badKey = { 1, 2, 3 }; // Not 32 bytes
    CryptoEngine engine(badKey);

    EXPECT_FALSE(engine.HasValidKey());
    EXPECT_FALSE(engine.EncryptString("test").has_value());
    EXPECT_FALSE(engine.DecryptString("test").has_value());
    EXPECT_FALSE(engine.SetKey(badKey));
}

TEST_F(CryptoEngineTest, Base64Utilities) {
    // Empty
    EXPECT_EQ(CryptoEngine::Base64Encode({}), "");
    auto decEmpty = CryptoEngine::Base64Decode("");
    ASSERT_TRUE(decEmpty.has_value());
    EXPECT_TRUE(decEmpty->empty());

    // Single padding '='
    std::vector<uint8_t> data1 = { 'a', 'b' };
    std::string b64_1 = CryptoEngine::Base64Encode(data1);
    EXPECT_TRUE(b64_1.ends_with("="));
    EXPECT_FALSE(b64_1.ends_with("=="));
    auto dec1 = CryptoEngine::Base64Decode(b64_1);
    ASSERT_TRUE(dec1.has_value());
    EXPECT_EQ(*dec1, data1);

    // Double padding '=='
    std::vector<uint8_t> data2 = { 'a' };
    std::string b64_2 = CryptoEngine::Base64Encode(data2);
    EXPECT_TRUE(b64_2.ends_with("=="));
    auto dec2 = CryptoEngine::Base64Decode(b64_2);
    ASSERT_TRUE(dec2.has_value());
    EXPECT_EQ(*dec2, data2);

    // No padding
    std::vector<uint8_t> data3 = { 'a', 'b', 'c' };
    std::string b64_3 = CryptoEngine::Base64Encode(data3);
    EXPECT_FALSE(b64_3.ends_with("="));
    auto dec3 = CryptoEngine::Base64Decode(b64_3);
    ASSERT_TRUE(dec3.has_value());
    EXPECT_EQ(*dec3, data3);
}

