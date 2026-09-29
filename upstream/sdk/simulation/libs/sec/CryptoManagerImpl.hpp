/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CRYPTOMANAGERIMPL_HPP
#define CRYPTOMANAGERIMPL_HPP

#include <memory>
#include <vector>

#include <telux/sec/CryptoManager.hpp>
#include "CryptoParamImpl.hpp"
#include "internal-temp.h"

#include "protos/proto-src/security_simulation.grpc.pb.h"
#include <grpcpp/grpcpp.h>

namespace telux {
namespace sec {

// Represents operating system version, generally it comes from boot.img
static const uint32_t OS_VERSION = 110000;

// Represents version of system image, generally comes from system.img
static const uint32_t OS_SECURITY_PATCH = 202103;

// Represents root of trust, generally comes from UEFI of lower layer
static const uint8_t ROOT_OF_TRUST[32] = {0xa, 0x20, 0xef};

class CryptoManagerImpl : public ICryptoManager {

 public:
    CryptoManagerImpl();
    ~CryptoManagerImpl();

    telux::common::ErrorCode init();

    telux::common::ErrorCode generateKey(
        std::shared_ptr<ICryptoParam> cryptoParam, std::vector<uint8_t> &keyBlob) override;

    telux::common::ErrorCode importKey(std::shared_ptr<ICryptoParam> cryptoParam,
        telux::sec::KeyFormat keyFmt, std::vector<uint8_t> const &keyData,
        std::vector<uint8_t> &keyBlob) override;

    telux::common::ErrorCode exportKey(telux::sec::KeyFormat keyFmt,
        std::vector<uint8_t> const &keyBlob, std::vector<uint8_t> &keyData) override;

    telux::common::ErrorCode upgradeKey(std::shared_ptr<ICryptoParam> cryptoParam,
        std::vector<uint8_t> const &oldKeyBlob, std::vector<uint8_t> &newKeyBlob) override;

    telux::common::ErrorCode signData(std::shared_ptr<ICryptoParam> cryptoParam,
        std::vector<uint8_t> const &keyBlob, std::vector<uint8_t> const &plainText,
        std::vector<uint8_t> &signature) override;

    telux::common::ErrorCode verifyData(std::shared_ptr<ICryptoParam> cryptoParam,
        std::vector<uint8_t> const &keyBlob, std::vector<uint8_t> const &plainText,
        std::vector<uint8_t> const &signature) override;

    telux::common::ErrorCode encryptData(std::shared_ptr<ICryptoParam> cryptoParam,
        std::vector<uint8_t> const &keyBlob, std::vector<uint8_t> const &plainText,
        std::shared_ptr<EncryptedData> &encryptedData) override;

    telux::common::ErrorCode decryptData(std::shared_ptr<ICryptoParam> cryptoParam,
        std::vector<uint8_t> const &keyBlob, std::vector<uint8_t> const &encryptedText,
        std::vector<uint8_t> &decryptedText) override;

    CryptoManagerImpl(const CryptoManagerImpl &)            = delete;
    CryptoManagerImpl &operator=(const CryptoManagerImpl &) = delete;

 private:
    static std::unique_ptr<securityStub::SecurityCryptoManagerService::Stub> stub_;

    telux::common::ErrorCode deinit();
};

}  // End of namespace sec
}  // End of namespace telux

#endif  // CRYPTOMANAGERIMPL_HPP
