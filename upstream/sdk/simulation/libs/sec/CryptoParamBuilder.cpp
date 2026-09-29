/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <memory>
#include <vector>

#include <telux/sec/CryptoDefines.hpp>
#include <telux/sec/CryptoParamBuilder.hpp>
#include "CryptoParamImpl.hpp"

namespace telux {
namespace sec {

CryptoParamBuilder::CryptoParamBuilder() {
    cryptoParam_ = nullptr;
}

CryptoParamBuilder CryptoParamBuilder::setAlgorithm(int32_t algorithm) {
    if (cryptoParam_ == nullptr) {
        cryptoParam_ = std::make_shared<CryptoParamImpl>();
    }

    std::static_pointer_cast<CryptoParamImpl>(cryptoParam_)->setAlgorithm(algorithm);
    return *this;
}

CryptoParamBuilder CryptoParamBuilder::setCryptoOperation(int32_t operation) {
    if (cryptoParam_ == nullptr) {
        cryptoParam_ = std::make_shared<CryptoParamImpl>();
    }

    std::static_pointer_cast<CryptoParamImpl>(cryptoParam_)->setCryptoOperation(operation);
    return *this;
}

CryptoParamBuilder CryptoParamBuilder::setDigest(int32_t digest) {
    if (cryptoParam_ == nullptr) {
        cryptoParam_ = std::make_shared<CryptoParamImpl>();
    }

    std::static_pointer_cast<CryptoParamImpl>(cryptoParam_)->setDigest(digest);
    return *this;
}

CryptoParamBuilder CryptoParamBuilder::setPadding(int32_t padding) {
    if (cryptoParam_ == nullptr) {
        cryptoParam_ = std::make_shared<CryptoParamImpl>();
    }

    std::static_pointer_cast<CryptoParamImpl>(cryptoParam_)->setPadding(padding);
    return *this;
}

CryptoParamBuilder CryptoParamBuilder::setKeySize(int32_t keySize) {
    if (cryptoParam_ == nullptr) {
        cryptoParam_ = std::make_shared<CryptoParamImpl>();
    }

    std::static_pointer_cast<CryptoParamImpl>(cryptoParam_)->setKeySize(keySize);
    return *this;
}

CryptoParamBuilder CryptoParamBuilder::setMacLength(int32_t macLength) {
    if (cryptoParam_ == nullptr) {
        cryptoParam_ = std::make_shared<CryptoParamImpl>();
    }

    std::static_pointer_cast<CryptoParamImpl>(cryptoParam_)->setMacLength(macLength);
    return *this;
}

CryptoParamBuilder CryptoParamBuilder::setMinimumMacLength(int32_t minMacLength) {
    if (cryptoParam_ == nullptr) {
        cryptoParam_ = std::make_shared<CryptoParamImpl>();
    }

    std::static_pointer_cast<CryptoParamImpl>(cryptoParam_)->setMinimumMacLength(minMacLength);
    return *this;
}

CryptoParamBuilder CryptoParamBuilder::setBlockMode(int32_t blockMode) {
    if (cryptoParam_ == nullptr) {
        cryptoParam_ = std::make_shared<CryptoParamImpl>();
    }

    std::static_pointer_cast<CryptoParamImpl>(cryptoParam_)->setBlockMode(blockMode);
    return *this;
}

CryptoParamBuilder CryptoParamBuilder::setCurve(int32_t curve) {
    if (cryptoParam_ == nullptr) {
        cryptoParam_ = std::make_shared<CryptoParamImpl>();
    }

    std::static_pointer_cast<CryptoParamImpl>(cryptoParam_)->setCurve(curve);
    return *this;
}

CryptoParamBuilder CryptoParamBuilder::setCallerNonce(bool callerNonce) {
    if (cryptoParam_ == nullptr) {
        cryptoParam_ = std::make_shared<CryptoParamImpl>();
    }

    std::static_pointer_cast<CryptoParamImpl>(cryptoParam_)->setCallerNonce(callerNonce);
    return *this;
}

CryptoParamBuilder CryptoParamBuilder::setPublicExponent(uint64_t publicExponent) {
    if (cryptoParam_ == nullptr) {
        cryptoParam_ = std::make_shared<CryptoParamImpl>();
    }

    std::static_pointer_cast<CryptoParamImpl>(cryptoParam_)->setPublicExponent(publicExponent);
    return *this;
}

CryptoParamBuilder CryptoParamBuilder::setInitVector(std::vector<uint8_t> initVector) {
    if (cryptoParam_ == nullptr) {
        cryptoParam_ = std::make_shared<CryptoParamImpl>();
    }

    std::static_pointer_cast<CryptoParamImpl>(cryptoParam_)->setInitVector(initVector);
    return *this;
}

CryptoParamBuilder CryptoParamBuilder::setUniqueData(std::vector<uint8_t> uniqueData) {
    if (cryptoParam_ == nullptr) {
        cryptoParam_ = std::make_shared<CryptoParamImpl>();
    }

    std::static_pointer_cast<CryptoParamImpl>(cryptoParam_)->setUniqueData(uniqueData);
    return *this;
}

CryptoParamBuilder CryptoParamBuilder::setAssociatedData(std::vector<uint8_t> associatedData) {
    if (cryptoParam_ == nullptr) {
        cryptoParam_ = std::make_shared<CryptoParamImpl>();
    }

    std::static_pointer_cast<CryptoParamImpl>(cryptoParam_)->setAssociatedData(associatedData);
    return *this;
}

std::shared_ptr<ICryptoParam> CryptoParamBuilder::build() {
    std::shared_ptr<ICryptoParam> tempCryptoParam = cryptoParam_;
    cryptoParam_ == nullptr;
    return tempCryptoParam;
}

}  // End of namespace sec
}  // End of namespace telux