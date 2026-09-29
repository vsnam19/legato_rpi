/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CRYPTOACCELERATORMANAGERIMPL_HPP
#define CRYPTOACCELERATORMANAGERIMPL_HPP

#include "common/CommonUtils.hpp"
#include "common/TaskDispatcher.hpp"
#include "common/ListenerManager.hpp"

#include <telux/sec/CryptoAcceleratorManager.hpp>
#include "common/event-manager/ClientEventManager.hpp"

#include "internal-temp.h"

#include "protos/proto-src/security_simulation.grpc.pb.h"
#include <grpcpp/grpcpp.h>

namespace telux {
namespace sec {

class CryptoAcceleratorManagerImpl
    : public ICryptoAcceleratorManager,
      public telux::common::IEventListener,
      public std::enable_shared_from_this<CryptoAcceleratorManagerImpl> {

 public:

    CryptoAcceleratorManagerImpl();
    ~CryptoAcceleratorManagerImpl();

    CryptoAcceleratorManagerImpl(const CryptoAcceleratorManagerImpl &)            = delete;
    CryptoAcceleratorManagerImpl &operator=(const CryptoAcceleratorManagerImpl &) = delete;

    telux::common::ErrorCode init(Mode mode, std::weak_ptr<ICryptoAcceleratorListener> caListener);

    telux::common::ErrorCode deinit();
    //****** MODE_ASYNC_LISTENER/MODE_ASYNC_POLL - Asynchronous APIs ******//

    telux::common::ErrorCode eccPostDigestForVerification(const DataDigest &digest,
        const ECCPoint &publicKey, const Signature &signature, telux::sec::ECCCurve curve,
        uint32_t uniqueId, telux::sec::RequestPriority priority) override;

    telux::common::ErrorCode ecqvPostDataForMultiplyAndAdd(const ECCPoint &multiplicandPoint,
        const ECCPoint &addendPoint, const Scalar &scalar, telux::sec::ECCCurve curve,
        uint32_t uniqueId, telux::sec::RequestPriority priority) override;

    telux::common::ErrorCode getAsyncResults(std::vector<OperationResult> &results,
        uint32_t numResultsToRead, int32_t timeout, uint32_t &numResultsRead) override;

    //*********** MODE_SYNC - Synchronous APIs ***********//

    telux::common::ErrorCode eccVerifyDigest(const DataDigest &digest, const ECCPoint &publicKey,
        const Signature &signature, telux::sec::ECCCurve curve, uint32_t uniqueId,
        telux::sec::RequestPriority priority, std::vector<uint8_t> &resultData) override;

    telux::common::ErrorCode ecqvPointMultiplyAndAdd(const ECCPoint &multiplicandPoint,
        const ECCPoint &addendPoint, const Scalar &scalar, telux::sec::ECCCurve curve,
        uint32_t uniqueId, telux::sec::RequestPriority priority,
        std::vector<uint8_t> &resultData) override;

    /* Protect against concurrent SSR and CryptoAcceleratorManagerImpl destruction */
    static std::mutex destructorGuard_;

    /* Set to true to indicate - reader thread should terminate now */
    static std::atomic<bool> exitNow_;

    void onEventUpdate(google::protobuf::Any event) override;

 private:
    const char * const CRYPTOACC_FILTER = "cryptoAcc";
    ClientEventManager &clientEventMgr_;
    static std::unique_ptr<securityStub::CryptoAcceleratorManagerService::Stub> stub_;

    /* Updated with user provided mode during init */
    Mode resultDeliveryMode_ = Mode::MODE_ASYNC_POLL;

    /* Set to true if the mvm initialization succeeds */
    bool connectionInitialized_ = false;

    /* Dispatcher to deliver results to the ecc/ecqv listeners */
    std::shared_ptr<telux::common::TaskDispatcher> asyncResultAndSsrDispatcher_;

    std::shared_ptr<telux::common::ListenerManager<ICryptoAcceleratorListener>> caListenerMgr_;

    /* Used during SSR */
    telux::common::ServiceStatus currentServiceStatus_;
    securityStub::RequestPriority convertPriorityTeluxToGrpc(telux::sec::RequestPriority teluxPriority);
    securityStub::EccCurve convertCurveTeluxToGrpc(telux::sec::ECCCurve teluxCurve);
    /* Passes ECC/ECQV result to listener asynchronously */
    void deliverResultAsync(securityStub::OperationResult result, int delay);
};

}  // End of namespace sec
}  // End of namespace telux

#endif  // CRYPTOACCELERATORMANAGERIMPL_HPP
