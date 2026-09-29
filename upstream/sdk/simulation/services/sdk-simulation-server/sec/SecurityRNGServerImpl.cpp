/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <telux/sec/RandomNumberManager.hpp>

#include "libs/common/Logger.hpp"
#include "libs/common/CommonUtils.hpp"

#include "SecurityRNGServerImpl.hpp"

#define SEC_RNG_DDBG 0

SecurityRNGServerImpl::SecurityRNGServerImpl() {
}

SecurityRNGServerImpl::~SecurityRNGServerImpl() {
    LOG(DEBUG, __FUNCTION__);

    std::lock_guard<std::mutex> lock(operationGuard_);

    if (hwRngFd_ != UNINITIALIZED) {
        close(hwRngFd_);
    }
    if (devRandFd_ != UNINITIALIZED) {
        close(devRandFd_);
    }
}

grpc::Status SecurityRNGServerImpl::Init(::grpc::ServerContext* context,
    const ::securityStub::RNGSource* request, ::securityStub::RNGInitInfo* response) {

    int ret = -1;
    char *rngDeviceNode = NULL;
    telux::common::ErrorCode ec{};
    telux::sec::RNGSource rngSource{};

    std::lock_guard<std::mutex> lock(operationGuard_);

    if (!apiConfigJsonRoot_) {
        ec = JsonParser::readFromJsonFile(apiConfigJsonRoot_, std::string(RNGMgr_API_JSON_FILE));
        if (ec != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " can't read ", RNGMgr_API_JSON_FILE);
            response->set_error_code(commonStub::ErrorCode::SYSTEM_ERR);
            return grpc::Status::OK;
        }
    }

    rngSource = static_cast<telux::sec::RNGSource>(request->source());

    switch (rngSource) {
        case telux::sec::RNGSource::QTI_HW_TRNG:
            if (hwRngFd_ != UNINITIALIZED) {
                ret = hwRngFd_;
                ++hwrngUsersCount_;
                break;
            }
            rngDeviceNode = const_cast<char *>(HWRNG_DEV_NODE);
            ret = open(rngDeviceNode, O_RDONLY);
            if (ret < 0) {
                LOG(ERROR, __FUNCTION__, " can't open ", rngDeviceNode, " lnx err ", errno);
                response->set_error_code(commonStub::ErrorCode::SYSTEM_ERR);
                return grpc::Status::OK;
            }
            hwRngFd_ = ret;
            ++hwrngUsersCount_;
            break;
        case telux::sec::RNGSource::DEV_RANDOM:
            if (devRandFd_ != UNINITIALIZED) {
                ret = devRandFd_;
                ++devrandomUsersCount_;
                break;
            }
            rngDeviceNode = const_cast<char *>(DEV_RANDOM_DEV_NODE);
            ret = open(rngDeviceNode, O_RDONLY);
            if (ret < 0) {
                LOG(ERROR, __FUNCTION__, " can't open ", rngDeviceNode, " lnx err ", errno);
                response->set_error_code(commonStub::ErrorCode::SYSTEM_ERR);
                return grpc::Status::OK;
            }
            devRandFd_ = ret;
            ++devrandomUsersCount_;
            break;
        default:
            LOG(ERROR, __FUNCTION__, " invalid src ", static_cast<int>(rngSource));
            response->set_error_code(commonStub::ErrorCode::INVALID_ARGUMENTS);
            return grpc::Status::OK;
    }

    response->set_rng_fd(ret);
    response->set_error_code(commonStub::ErrorCode::ERROR_CODE_SUCCESS);

    return grpc::Status::OK;
}

grpc::Status SecurityRNGServerImpl::GetRandomNumber(::grpc::ServerContext* context,
    const ::securityStub::RandomNumber* request, ::securityStub::RandomNumber* response) {

    int ret = 0;
    int rngFd = 0;
    uint32_t number32 = 0;
    uint64_t number64 = 0;
    std::string ecStr = "";
    telux::common::ErrorCode ec{};

    std::lock_guard<std::mutex> lock(operationGuard_);

    rngFd = request->rng_fd();
    response->set_rng_fd(rngFd);

    ecStr = apiConfigJsonRoot_["IRandomNumberManager"]["getRandomNumber"]["error"].asString();
    ec = CommonUtils::mapErrorCode(ecStr);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        response->set_error_code(static_cast<commonStub::ErrorCode>(ec));
        return grpc::Status::OK;
    }

    if (request->has_number64()) {
        ret = read(rngFd, &number64, sizeof(number64));
        if (ret != sizeof(number64)) {
            LOG(ERROR, __FUNCTION__, " can't read, lnx err ", errno);
            response->set_error_code(commonStub::ErrorCode::SYSTEM_ERR);
            return grpc::Status::OK;
        }
        response->set_number64(number64);
    } else if (request->has_number32()) {
        ret = read(rngFd, &number32, sizeof(number32));
        if (ret != sizeof(number32)) {
            LOG(ERROR, __FUNCTION__, " can't read, lnx err ", errno);
            response->set_error_code(commonStub::ErrorCode::SYSTEM_ERR);
            return grpc::Status::OK;
        }
        response->set_number32(number32);
    } else {
        LOG(ERROR, __FUNCTION__, " missing number's data type");
        response->set_error_code(commonStub::ErrorCode::INVALID_ARGUMENTS);
        return grpc::Status::OK;
    }

#ifdef SEC_RNG_DDBG
    std::cout << "number32 " << number32 << ", number64 " << number64 << std::endl;
#endif

    response->set_error_code(commonStub::ErrorCode::ERROR_CODE_SUCCESS);
    return grpc::Status::OK;
}

grpc::Status SecurityRNGServerImpl::GetRandomData(::grpc::ServerContext* context,
    const ::securityStub::RandomData* request, ::securityStub::RandomData* response) {

    int ret = 0;
    int rngFd = 0;
    std::vector<uint8_t> generatedData(request->length(), 0);
    std::string ecStr = "";
    telux::common::ErrorCode ec{};

    std::lock_guard<std::mutex> lock(operationGuard_);

    rngFd = request->rng_fd();
    response->set_rng_fd(rngFd);

    ecStr = apiConfigJsonRoot_["IRandomNumberManager"]["getRandomData"]["error"].asString();
    ec = CommonUtils::mapErrorCode(ecStr);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        response->set_error_code(static_cast<commonStub::ErrorCode>(ec));
        return grpc::Status::OK;
    }

    ret = read(rngFd, generatedData.data(), request->length());
    if (ret != static_cast<int>(request->length())) {
        LOG(ERROR, __FUNCTION__, " can't read, lnx err ", errno);
        response->set_error_code(commonStub::ErrorCode::SYSTEM_ERR);
        return grpc::Status::OK;
    }

#ifdef SEC_RNG_DDBG
    for (size_t x = 0; x < request->length(); x++) {
        printf("%02x ", generatedData[x] & 0xffU);
    }
    std::cout << "\n" << std::endl;
#endif

    response->add_data(generatedData.data(), generatedData.size());
    response->set_length(generatedData.size());
    response->set_error_code(commonStub::ErrorCode::ERROR_CODE_SUCCESS);

    return grpc::Status::OK;
}

grpc::Status SecurityRNGServerImpl::RNGClientCleanup(::grpc::ServerContext* context,
    const ::securityStub::RNGClientInfo* request, ::google::protobuf::Empty* response) {

    int rngFd = 0;

    std::lock_guard<std::mutex> lock(operationGuard_);

    rngFd = request->rng_fd();

    if (rngFd == hwRngFd_) {
        hwrngUsersCount_--;
        if (!hwrngUsersCount_) {
            close(hwRngFd_);
            hwRngFd_ = UNINITIALIZED;
        }
    } else if (rngFd == devRandFd_) {
        devrandomUsersCount_--;
        if (!devrandomUsersCount_) {
            close(devRandFd_);
            devRandFd_ = UNINITIALIZED;
        }
    } else {
    }

    return grpc::Status::OK;
}