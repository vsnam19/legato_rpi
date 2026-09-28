#include "cryptoEngine.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <cstring>
#include <memory>
#include <arpa/inet.h>

namespace cfg::crypto {

namespace {

constexpr uint8_t MAGIC[4] = { 'S', 'E', 'N', 'C' };
constexpr size_t IV_LEN = 12;
constexpr size_t TAG_LEN = 16;
constexpr size_t HEADER_LEN = sizeof(MAGIC) + IV_LEN + TAG_LEN + sizeof(uint32_t); // 4 + 12 + 16 + 4 = 36 bytes

struct EvpCipherCtxDeleter {
    void operator()(EVP_CIPHER_CTX* ctx) const {
        if (ctx) {
            EVP_CIPHER_CTX_free(ctx);
        }
    }
};

using ScopedEvpCtx = std::unique_ptr<EVP_CIPHER_CTX, EvpCipherCtxDeleter>;

} // namespace

CryptoEngine::CryptoEngine(std::span<const uint8_t> key256) {
    SetKey(key256);
}

bool CryptoEngine::SetKey(std::span<const uint8_t> key256) {
    if (key256.size() != 32) {
        key_.clear();
        return false;
    }
    key_.assign(key256.begin(), key256.end());
    return true;
}

bool CryptoEngine::HasValidKey() const noexcept {
    return key_.size() == 32;
}

std::optional<std::vector<uint8_t>> CryptoEngine::Encrypt(std::span<const uint8_t> plaintext) const {
    if (!HasValidKey()) {
        return std::nullopt;
    }

    // 1. Generate random IV (12 bytes)
    uint8_t iv[IV_LEN];
    if (RAND_bytes(iv, IV_LEN) != 1) {
        return std::nullopt;
    }

    // 2. Initialize cipher context
    ScopedEvpCtx ctx(EVP_CIPHER_CTX_new());
    if (!ctx) {
        return std::nullopt;
    }

    if (EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        return std::nullopt;
    }

    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, IV_LEN, nullptr) != 1) {
        return std::nullopt;
    }

    if (EVP_EncryptInit_ex(ctx.get(), nullptr, nullptr, key_.data(), iv) != 1) {
        return std::nullopt;
    }

    std::vector<uint8_t> ciphertext(plaintext.size());
    int outLen = 0;

    if (!plaintext.empty()) {
        if (EVP_EncryptUpdate(ctx.get(), ciphertext.data(), &outLen, plaintext.data(), static_cast<int>(plaintext.size())) != 1) {
            return std::nullopt;
        }
    }

    int finalLen = 0;
    if (EVP_EncryptFinal_ex(ctx.get(), ciphertext.data() + outLen, &finalLen) != 1) {
        return std::nullopt;
    }

    // 3. Extract GCM Authentication Tag (16 bytes)
    uint8_t tag[TAG_LEN];
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, TAG_LEN, tag) != 1) {
        return std::nullopt;
    }

    // 4. Assemble envelope: [Magic 4B] [IV 12B] [Tag 16B] [Len 4B] [Ciphertext NB]
    std::vector<uint8_t> envelope;
    envelope.reserve(HEADER_LEN + ciphertext.size());

    envelope.insert(envelope.end(), MAGIC, MAGIC + 4);
    envelope.insert(envelope.end(), iv, iv + IV_LEN);
    envelope.insert(envelope.end(), tag, tag + TAG_LEN);

    uint32_t netLen = htonl(static_cast<uint32_t>(ciphertext.size()));
    const uint8_t* netLenBytes = reinterpret_cast<const uint8_t*>(&netLen);
    envelope.insert(envelope.end(), netLenBytes, netLenBytes + 4);

    envelope.insert(envelope.end(), ciphertext.begin(), ciphertext.end());

    return envelope;
}

std::optional<std::vector<uint8_t>> CryptoEngine::Decrypt(std::span<const uint8_t> envelope) const {
    if (!HasValidKey() || envelope.size() < HEADER_LEN) {
        return std::nullopt;
    }

    // 1. Verify Magic
    if (std::memcmp(envelope.data(), MAGIC, 4) != 0) {
        return std::nullopt;
    }

    // 2. Extract IV, Tag, Length
    const uint8_t* iv = envelope.data() + 4;
    const uint8_t* tag = iv + IV_LEN;
    const uint8_t* lenPtr = tag + TAG_LEN;

    uint32_t netLen = 0;
    std::memcpy(&netLen, lenPtr, sizeof(uint32_t));
    uint32_t cipherLen = ntohl(netLen);

    if (envelope.size() != HEADER_LEN + cipherLen) {
        return std::nullopt;
    }

    const uint8_t* ciphertext = envelope.data() + HEADER_LEN;

    // 3. Decrypt and verify tag
    ScopedEvpCtx ctx(EVP_CIPHER_CTX_new());
    if (!ctx) {
        return std::nullopt;
    }

    if (EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        return std::nullopt;
    }

    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, IV_LEN, nullptr) != 1) {
        return std::nullopt;
    }

    if (EVP_DecryptInit_ex(ctx.get(), nullptr, nullptr, key_.data(), iv) != 1) {
        return std::nullopt;
    }

    std::vector<uint8_t> plaintext(cipherLen);
    int outLen = 0;

    if (cipherLen > 0) {
        if (EVP_DecryptUpdate(ctx.get(), plaintext.data(), &outLen, ciphertext, static_cast<int>(cipherLen)) != 1) {
            return std::nullopt;
        }
    }

    // Set expected authentication tag before EVP_DecryptFinal_ex
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, TAG_LEN, const_cast<uint8_t*>(tag)) != 1) {
        return std::nullopt;
    }

    int finalLen = 0;
    // EVP_DecryptFinal_ex returns > 0 on successful authentication!
    if (EVP_DecryptFinal_ex(ctx.get(), plaintext.data() + outLen, &finalLen) <= 0) {
        return std::nullopt; // Tag verification failed / tampered data
    }

    plaintext.resize(outLen + finalLen);
    return plaintext;
}

std::optional<std::string> CryptoEngine::EncryptString(std::string_view plaintext) const {
    std::span<const uint8_t> plainBytes(
        reinterpret_cast<const uint8_t*>(plaintext.data()),
        plaintext.size()
    );

    auto encBytes = Encrypt(plainBytes);
    if (!encBytes) {
        return std::nullopt;
    }

    return Base64Encode(*encBytes);
}

std::optional<std::string> CryptoEngine::DecryptString(std::string_view base64Envelope) const {
    auto decBytes = Base64Decode(base64Envelope);
    if (!decBytes) {
        return std::nullopt;
    }

    auto plainBytes = Decrypt(*decBytes);
    if (!plainBytes) {
        return std::nullopt;
    }

    return std::string(reinterpret_cast<const char*>(plainBytes->data()), plainBytes->size());
}

std::string CryptoEngine::Base64Encode(std::span<const uint8_t> data) {
    if (data.empty()) {
        return "";
    }

    int encodedLen = 4 * ((static_cast<int>(data.size()) + 2) / 3);
    std::string result(encodedLen + 1, '\0');

    int actualLen = EVP_EncodeBlock(
        reinterpret_cast<unsigned char*>(result.data()),
        data.data(),
        static_cast<int>(data.size())
    );

    result.resize(actualLen);
    return result;
}

std::optional<std::vector<uint8_t>> CryptoEngine::Base64Decode(std::string_view b64) {
    if (b64.empty()) {
        return std::vector<uint8_t>{};
    }

    std::vector<uint8_t> outBuffer(b64.size());
    int decodedLen = EVP_DecodeBlock(
        outBuffer.data(),
        reinterpret_cast<const unsigned char*>(b64.data()),
        static_cast<int>(b64.size())
    );

    if (decodedLen < 0) {
        return std::nullopt;
    }

    // Adjust for base64 padding '='
    if (b64.ends_with("==")) {
        decodedLen -= 2;
    } else if (b64.ends_with('=')) {
        decodedLen -= 1;
    }

    if (decodedLen < 0) {
        return std::nullopt;
    }

    outBuffer.resize(decodedLen);
    return outBuffer;
}

} // namespace cfg::crypto
