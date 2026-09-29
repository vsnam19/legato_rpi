/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "CryptoParamImpl.hpp"

namespace telux {
namespace sec {

CryptoParamImpl::CryptoParamImpl() {
    algorithm_      = -1;
    operation_      = -1;
    digest_         = -1;
    padding_        = -1;
    keySize_        = -1;
    macLength_      = -1;
    minMacLength_   = -1;
    blockMode_      = -1;
    curve_          = -1;
    callerNonce_    = false;
    publicExponent_ = 0;
}

int32_t CryptoParamImpl::getAlgorithm() {
    return algorithm_;
}

int32_t CryptoParamImpl::getCryptoOperation() {
    return operation_;
}

int32_t CryptoParamImpl::getDigest() {
    return digest_;
}

int32_t CryptoParamImpl::getPadding() {
    return padding_;
}

int32_t CryptoParamImpl::getKeySize() {
    return keySize_;
}

int32_t CryptoParamImpl::getMinMacLength() {
    return minMacLength_;
}

int32_t CryptoParamImpl::getMacLength() {
    return macLength_;
}

int32_t CryptoParamImpl::getBlockMode() {
    return blockMode_;
}

int32_t CryptoParamImpl::getCurve() {
    return curve_;
}

bool CryptoParamImpl::getCallerNonce() {
    return callerNonce_;
}

uint64_t CryptoParamImpl::getPublicExponent() {
    return publicExponent_;
}

std::shared_ptr<std::vector<uint8_t>> CryptoParamImpl::getInitVector() {
    return initVector_;
}

std::shared_ptr<std::vector<uint8_t>> CryptoParamImpl::getUniqueData() {
    return uniqueData_;
}

std::shared_ptr<std::vector<uint8_t>> CryptoParamImpl::getAssociatedData() {
    return associatedData_;
}

void CryptoParamImpl::setAlgorithm(AlgorithmTypes algorithm) {
    algorithm_ = algorithm;
}

void CryptoParamImpl::setCryptoOperation(CryptoOperationTypes operation) {
    operation_ = operation;
}

void CryptoParamImpl::setDigest(DigestTypes digest) {
    digest_ = digest;
}

void CryptoParamImpl::setPadding(PaddingTypes padding) {
    padding_ = padding;
}

void CryptoParamImpl::setKeySize(int32_t keySize) {
    keySize_ = keySize;
}

void CryptoParamImpl::setMinimumMacLength(int32_t minMacLength) {
    minMacLength_ = minMacLength;
}

void CryptoParamImpl::setMacLength(int32_t macLength) {
    macLength_ = macLength;
}

void CryptoParamImpl::setBlockMode(BlockModeTypes blockMode) {
    blockMode_ = blockMode;
}

void CryptoParamImpl::setCurve(int32_t curve) {
    curve_ = curve;
}

void CryptoParamImpl::setCallerNonce(bool callerNonce) {
    callerNonce_ = callerNonce;
}

void CryptoParamImpl::setPublicExponent(uint64_t publicExponent) {
    publicExponent_ = publicExponent;
}

void CryptoParamImpl::setInitVector(std::vector<uint8_t> initVector) {
    initVector_ = std::make_shared<std::vector<uint8_t>>(initVector);
}

void CryptoParamImpl::setUniqueData(std::vector<uint8_t> uniqueData) {
    uniqueData_ = std::make_shared<std::vector<uint8_t>>(uniqueData);
}

void CryptoParamImpl::setAssociatedData(std::vector<uint8_t> associatedData) {
    associatedData_ = std::make_shared<std::vector<uint8_t>>(associatedData);
}

}  // End of namespace sec
}  // End of namespace telux