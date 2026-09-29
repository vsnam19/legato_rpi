/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SECURITY_RNG_SERVER_IMPL_HPP
#define SECURITY_RNG_SERVER_IMPL_HPP

#include <mutex>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>

#include "protos/proto-src/security_simulation.grpc.pb.h"

#include "libs/common/JsonParser.hpp"

class SecurityRNGServerImpl : public ::securityStub::RandomNumberGeneratorService::Service {

 public:
    SecurityRNGServerImpl();
    ~SecurityRNGServerImpl();

    grpc::Status Init(::grpc::ServerContext* context,
        const ::securityStub::RNGSource* request,
        ::securityStub::RNGInitInfo* response) override;

    grpc::Status GetRandomNumber(::grpc::ServerContext* context,
        const ::securityStub::RandomNumber* request,
        ::securityStub::RandomNumber* response) override;

    grpc::Status GetRandomData(::grpc::ServerContext* context,
        const ::securityStub::RandomData* request,
        ::securityStub::RandomData* response) override;

    grpc::Status RNGClientCleanup(::grpc::ServerContext* context,
        const ::securityStub::RNGClientInfo* request,
        ::google::protobuf::Empty* response) override;

 private:
    const int32_t UNINITIALIZED = -1;
    const char * const RNGMgr_API_JSON_FILE = "api/sec/IRandomNumberManager.json";
    const char * const HWRNG_DEV_NODE = "/dev/hwrng";
    const char * const DEV_RANDOM_DEV_NODE = "/dev/random";

    int hwRngFd_ = UNINITIALIZED;
    int devRandFd_ = UNINITIALIZED;
    uint32_t hwrngUsersCount_ = 0;
    uint32_t devrandomUsersCount_ = 0;

    Json::Value apiConfigJsonRoot_;
    std::mutex operationGuard_;
};

#endif // SECURITY_RNG_SERVER_IMPL_HPP
