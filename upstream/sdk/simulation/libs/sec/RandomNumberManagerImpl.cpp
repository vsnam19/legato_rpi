/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "libs/common/CommonUtils.hpp"
#include "common/Logger.hpp"

#include "RandomNumberManagerImpl.hpp"

namespace telux {
namespace sec {

RandomNumberManagerImpl::RandomNumberManagerImpl() {
}

RandomNumberManagerImpl::~RandomNumberManagerImpl() {
    LOG(DEBUG, __FUNCTION__);

    grpc::ClientContext clientCtx{};
    ::securityStub::RNGClientInfo request{};

    google::protobuf::Empty response{};

    request.set_rng_fd(rngFd_);

    stub_->RNGClientCleanup(&clientCtx, request, &response);
}

/*
 * Setup RNG source as specified by application.
 */
telux::common::ErrorCode RandomNumberManagerImpl::init(RNGSource generatorSource) {

    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::securityStub::RNGSource request{};

    telux::common::ErrorCode ec{};
    ::securityStub::RNGInitInfo response{};

    stub_ = CommonUtils::getGrpcStub<securityStub::RandomNumberGeneratorService>();

    request.set_source(static_cast<::securityStub::RNGSource_Source>(generatorSource));

    reqStatus = stub_->Init(&clientCtx, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " communication error");
        return telux::common::ErrorCode::TRANSPORT_ERROR;
    }

    ec = static_cast<telux::common::ErrorCode>(response.error_code());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " cant't init");
        return ec;
    }

    rngFd_ = response.rng_fd();
    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Generate a 32 bit random number.
 */
telux::common::ErrorCode RandomNumberManagerImpl::getRandomNumber(
        uint32_t &generatedNumber) {

    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::securityStub::RandomNumber request{};

    telux::common::ErrorCode ec{};
    ::securityStub::RandomNumber response{};

    request.set_rng_fd(rngFd_);
    request.set_number32(1);

    reqStatus = stub_->GetRandomNumber(&clientCtx, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " communication failed");
        return telux::common::ErrorCode::TRANSPORT_ERROR;
    }

    ec = static_cast<telux::common::ErrorCode>(response.error_code());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " cant't get random number");
        return ec;
    }

    generatedNumber = response.number32();
    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Generate a 64 bit random number.
 */
telux::common::ErrorCode RandomNumberManagerImpl::getRandomNumber(
        uint64_t &generatedNumber) {

    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::securityStub::RandomNumber request{};

    telux::common::ErrorCode ec{};
    ::securityStub::RandomNumber response{};

    request.set_rng_fd(rngFd_);
    request.set_number64(1);

    reqStatus = stub_->GetRandomNumber(&clientCtx, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " communication failed");
        return telux::common::ErrorCode::TRANSPORT_ERROR;
    }

    ec = static_cast<telux::common::ErrorCode>(response.error_code());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " cant't get random number");
        return ec;
    }

    generatedNumber = response.number64();
    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Generate random bytes and return to the application. Maximum number of
 * bytes is defined by platform implementation.
 */
telux::common::ErrorCode RandomNumberManagerImpl::getRandomData(
        std::vector<uint8_t> &generatedData, size_t &dataLength) {

    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::securityStub::RandomData request{};

    telux::common::ErrorCode ec{};
    ::securityStub::RandomData response{};

    request.set_rng_fd(rngFd_);
    request.set_length(generatedData.size());

    reqStatus = stub_->GetRandomData(&clientCtx, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " communication failed");
        return telux::common::ErrorCode::TRANSPORT_ERROR;
    }

    ec = static_cast<telux::common::ErrorCode>(response.error_code());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " cant't get random data");
        return ec;
    }

    dataLength = response.length();

    generatedData = std::vector<uint8_t>(&(response.data(0)[0]),
        &(response.data(0)[0]) + dataLength);

    return telux::common::ErrorCode::SUCCESS;
}

}  // End of namespace sec
}  // End of namespace telux