/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef RANDOMNUMBERMANAGERIMPL_HPP
#define RANDOMNUMBERMANAGERIMPL_HPP

#include "protos/proto-src/security_simulation.grpc.pb.h"
#include <grpcpp/grpcpp.h>

#include <telux/sec/RandomNumberManager.hpp>

namespace telux {
namespace sec {

class RandomNumberManagerImpl : public IRandomNumberManager {

 public:
    RandomNumberManagerImpl();
    ~RandomNumberManagerImpl();

    telux::common::ErrorCode init(RNGSource generatorSource);

    telux::common::ErrorCode getRandomNumber(uint32_t &generatedNumber) override;

    telux::common::ErrorCode getRandomNumber(uint64_t &generatedNumber) override;

    telux::common::ErrorCode getRandomData(std::vector<uint8_t> &generatedData,
        size_t &dataLength) override;

    RandomNumberManagerImpl(const RandomNumberManagerImpl &) = delete;
    RandomNumberManagerImpl &operator=(const RandomNumberManagerImpl &) = delete;

 private:
    const int32_t UNINITIALIZED = -1;
    int rngFd_ = UNINITIALIZED;
    std::unique_ptr<::securityStub::RandomNumberGeneratorService::Stub> stub_;
};

}  // End of namespace sec
}  // End of namespace telux

#endif  // RANDOMNUMBERMANAGERIMPL_HPP
