/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CRYPTOPARAMIMPL_HPP
#define CRYPTOPARAMIMPL_HPP

#include <memory>
#include <vector>

#include <telux/sec/CryptoDefines.hpp>

namespace telux {
namespace sec {

class CryptoParamImpl : public ICryptoParam {

 public:
    CryptoParamImpl();

    int32_t getAlgorithm();
    int32_t getCryptoOperation();
    int32_t getDigest();
    int32_t getPadding();
    int32_t getKeySize();
    int32_t getMinMacLength();
    int32_t getMacLength();
    int32_t getBlockMode();
    int32_t getCurve();
    bool getCallerNonce();
    uint64_t getPublicExponent();
    std::shared_ptr<std::vector<uint8_t>> getInitVector();
    std::shared_ptr<std::vector<uint8_t>> getUniqueData();
    std::shared_ptr<std::vector<uint8_t>> getAssociatedData();

    void setAlgorithm(AlgorithmTypes algorithm);
    void setCryptoOperation(CryptoOperationTypes operation);
    void setDigest(DigestTypes digest);
    void setPadding(PaddingTypes padding);
    void setKeySize(int32_t keySize);
    void setMinimumMacLength(int32_t minMacLength);
    void setMacLength(int32_t macLength);
    void setBlockMode(BlockModeTypes blockMode);
    void setCurve(int32_t curve);
    void setCallerNonce(bool callerNonce);
    void setPublicExponent(uint64_t publicExponent);
    void setInitVector(std::vector<uint8_t> initVector);
    void setUniqueData(std::vector<uint8_t> uniqueData);
    void setAssociatedData(std::vector<uint8_t> associatedData);

 private:
    int32_t algorithm_;
    int32_t operation_;
    int32_t digest_;
    int32_t padding_;
    int32_t keySize_;
    int32_t minMacLength_;
    int32_t macLength_;
    int32_t blockMode_;
    int32_t curve_;
    bool callerNonce_;
    uint64_t publicExponent_;
    std::shared_ptr<std::vector<uint8_t>> initVector_;
    std::shared_ptr<std::vector<uint8_t>> uniqueData_;
    std::shared_ptr<std::vector<uint8_t>> associatedData_;
};

}  // End of namespace sec
}  // End of namespace telux

#endif  // CRYPTOPARAMIMPL_HPP