/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "CryptoAcceleratorManagerImpl.hpp"
#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/impl/codegen/async_unary_call.h>

namespace telux {
namespace sec {

std::atomic<bool> CryptoAcceleratorManagerImpl::exitNow_;
std::mutex CryptoAcceleratorManagerImpl::destructorGuard_;
std::unique_ptr<::securityStub::CryptoAcceleratorManagerService::Stub>
    CryptoAcceleratorManagerImpl::stub_;

CryptoAcceleratorManagerImpl::CryptoAcceleratorManagerImpl()
    : clientEventMgr_(ClientEventManager::getInstance()) {
    exitNow_ = false;
}

CryptoAcceleratorManagerImpl::~CryptoAcceleratorManagerImpl() {
    LOG(DEBUG, __FUNCTION__);

    if (!connectionInitialized_) {
        return;
    }

    std::lock_guard<std::mutex> ssrLock(CryptoAcceleratorManagerImpl::destructorGuard_);

    exitNow_ = true;
    deinit();
}

// Helper to convert Telux ECCCurve to gRPC EccCurve.
securityStub::EccCurve
CryptoAcceleratorManagerImpl::convertCurveTeluxToGrpc(telux::sec::ECCCurve teluxCurve) {

    securityStub::EccCurve grpcCurve;

    switch (teluxCurve) {
        case telux::sec::ECCCurve::CURVE_SM2:
            grpcCurve = securityStub::EccCurve::CURVE_SM2;
            break;
        case telux::sec::ECCCurve::CURVE_NISTP256:
            grpcCurve = securityStub::EccCurve::CURVE_NISTP256;
            break;
        case telux::sec::ECCCurve::CURVE_NISTP384:
            grpcCurve = securityStub::EccCurve::CURVE_NISTP384;
            break;
        case telux::sec::ECCCurve::CURVE_BRAINPOOLP256R1:
            grpcCurve = securityStub::EccCurve::CURVE_BRAINPOOLP256R1;
            break;
        case telux::sec::ECCCurve::CURVE_BRAINPOOLP384R1:
            grpcCurve = securityStub::EccCurve::CURVE_BRAINPOOLP384R1;
            break;
        default:
            grpcCurve = securityStub::EccCurve::CURVE_NISTP256; // Default or error
            break;
    }
    return grpcCurve;
}

// Helper to convert Telux RequestPriority to gRPC RequestPriority.
securityStub::RequestPriority
CryptoAcceleratorManagerImpl::convertPriorityTeluxToGrpc(telux::sec::RequestPriority teluxPriority) {

    securityStub::RequestPriority grpcPriority;

    switch (teluxPriority) {
        case telux::sec::RequestPriority::REQ_PRIORITY_HIGH:
            grpcPriority = securityStub::RequestPriority::REQ_PRIORITY_HIGH;
            break;
        case telux::sec::RequestPriority::REQ_PRIORITY_NORMAL:
            grpcPriority = securityStub::RequestPriority::REQ_PRIORITY_NORMAL;
            break;
        default:
            grpcPriority = securityStub::RequestPriority::REQ_PRIORITY_NORMAL; // Default
            break;
    }
    return grpcPriority;
}

/*
 * Allocates and initializes resources when an application gets
 * an ICryptoAcceleratorManager instance using SecurityFactory.
 *  *
 * ------------------------------------------------------------------------------------------------
 *|        Mode         |             Request             |              Result |
 * ------------------------------------------------------------------------------------------------
 *| MODE_SYNC           | eccVerifyDigest()               | eccVerifyDigest() | | |
 *ecqvPointMultiplyAndAdd()       | ecqvPointMultiplyAndAdd()                         | |
 *MODE_ASYNC_POLL     | eccPostDigestForVerification()  | getAsyncResults() | | |
 *ecqvPostDataForMultiplyAndAdd() | getAsyncResults()                                 | |
 *MODE_ASYNC_LISTENER | eccPostDigestForVerification()  |
 *ICryptoAcceleratorListener::onVerificationResult()| |                     |
 *ecqvPostDataForMultiplyAndAdd() | ICryptoAcceleratorListener::onCalculationResult() |
 * ------------------------------------------------------------------------------------------------
*/
telux::common::ErrorCode CryptoAcceleratorManagerImpl::init(
    Mode mode, std::weak_ptr<ICryptoAcceleratorListener> cryptoAccelListener) {

    telux::common::ErrorCode ec = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::google::protobuf::Empty request{};
    ::commonStub::ErrorCodeMsg response{};
    std::shared_ptr<ICryptoAcceleratorListener> resultAndSsrListener;

    stub_ = CommonUtils::getGrpcStub<securityStub::CryptoAcceleratorManagerService>();

    status = clientEventMgr_.registerListener(shared_from_this(), CRYPTOACC_FILTER);
    if ((status != telux::common::Status::SUCCESS) &&
        (status != telux::common::Status::ALREADY)) {
        LOG(ERROR, __FUNCTION__, " can't register with ClientEventManager");
        return telux::common::CommonUtils::toErrorCode(status);
    }

    resultDeliveryMode_ = mode;

    resultAndSsrListener = cryptoAccelListener.lock();
    if ((mode == Mode::MODE_ASYNC_LISTENER) && (!resultAndSsrListener)) {
        LOG(ERROR, __FUNCTION__, " ICryptoAcceleratorListener not given");
        return telux::common::ErrorCode::INVALID_ARGUMENTS;
    }

    if (resultAndSsrListener) {
        try {
            caListenerMgr_ =
                std::make_shared<telux::common::ListenerManager<ICryptoAcceleratorListener>>();
        } catch (const std::exception &e) {
            LOG(ERROR, __FUNCTION__, " can't create ICryptoAcceleratorListener manager");
            return telux::common::ErrorCode::NO_MEMORY;
        }

        status = caListenerMgr_->registerListener(cryptoAccelListener);
        if (status != telux::common::Status::SUCCESS) {
            return telux::common::CommonUtils::toErrorCode(status);
        }

        try {
            asyncResultAndSsrDispatcher_ = std::make_shared<telux::common::TaskDispatcher>();
        } catch (const std::exception &e) {
            LOG(ERROR, __FUNCTION__, " can't create TaskDispatcher");
            return telux::common::ErrorCode::NO_MEMORY;
        }
    }

    reqStatus = stub_->Init(&clientCtx, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " communication error");
        return telux::common::ErrorCode::TRANSPORT_ERROR;
    }

    ec = static_cast<telux::common::ErrorCode>(response.ec());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, "can't register with server, error code: ",
            static_cast<int>(ec));
        return ec;
    }

    connectionInitialized_ = true;
    currentServiceStatus_ = telux::common::ServiceStatus::SERVICE_AVAILABLE;

    return telux::common::ErrorCode::SUCCESS;
}

/**
 * This function performs a DeInit RPC call to the server to tear down the
 * connection and release any allocated resources.
 */
telux::common::ErrorCode CryptoAcceleratorManagerImpl::deinit() {

    telux::common::ErrorCode ec = telux::common::ErrorCode::SUCCESS;
    grpc::Status reqStatus{};
    grpc::ClientContext clientCtx{};
    ::google::protobuf::Empty request{};
    ::commonStub::ErrorCodeMsg response{};

    reqStatus = stub_->DeInit(&clientCtx, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " communication error");
        return telux::common::ErrorCode::TRANSPORT_ERROR;
    }

    ec = static_cast<telux::common::ErrorCode>(response.ec());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, "can't deregister with server, error code: ",
            static_cast<int>(ec));
        return ec;
    }

    return telux::common::ErrorCode::SUCCESS;
}

/*
 * Receives events via simulation event manager framework.
 */
void CryptoAcceleratorManagerImpl::onEventUpdate(google::protobuf::Any event) {

    ::securityStub::CryptoAccServiceStatus newServiceState;
    std::vector<std::weak_ptr<ICryptoAcceleratorListener>> ssrListener;
    telux::common::ServiceStatus serviceStatus{};

    if (event.Is<::securityStub::CryptoAccServiceStatus>()) {
        event.UnpackTo(&newServiceState);
        serviceStatus = static_cast<telux::common::ServiceStatus>(newServiceState.service_status());
        caListenerMgr_->getAvailableListeners(ssrListener);
        if (!ssrListener.size()) {
            LOG(ERROR, __FUNCTION__, " can't find listener, dropped new state ",
                static_cast<int>(serviceStatus));
            return;
        }
        /* Task queue is lock protected internally. Therefore, no locking is required
         * here to synchronize with deliverResultAsync/Sync(). Application will get
         * result and ssr event serially */
        if (auto sp = std::dynamic_pointer_cast<ICryptoAcceleratorListener>(ssrListener[0].lock())) {
            asyncResultAndSsrDispatcher_->submitTask([=] {
                sp->onServiceStatusChange(serviceStatus);
                LOG(DEBUG, __FUNCTION__, " new status ", static_cast<int>(serviceStatus));
            });
        } else {
            LOG(ERROR, __FUNCTION__, " missing ssr listener, dropped new state ",
                static_cast<int>(serviceStatus));
        }

        currentServiceStatus_ = serviceStatus;
    }
}

/**
 * Delivers the result of an asynchronous operation to the registered listener.
 *
 * This function is typically called by a TaskDispatcher. It copies the result data,
 * checks for available listeners, and dispatches the appropriate callback
 * (`onVerificationResult` or `onCalculationResult`) based on the operation type.
 * A simulated delay can be introduced before delivery.
 */
void CryptoAcceleratorManagerImpl::deliverResultAsync(securityStub::OperationResult result,
                                                      int cbDelay) {

    uint32_t uniqueId = result.id();
    telux::common::ErrorCode ec = static_cast<telux::common::ErrorCode>(result.error_code());
    std::vector<uint8_t> resultData(result.data().begin(), result.data().end()); // Copy bytes
    std::vector<std::weak_ptr<ICryptoAcceleratorListener>> eccListener;

    caListenerMgr_->getAvailableListeners(eccListener);

    if (eccListener.empty()) {
        LOG(ERROR, __FUNCTION__, " Can't find listener, result dropped, unique id ", uniqueId);
        return;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));

    if (auto sp = std::dynamic_pointer_cast<ICryptoAcceleratorListener>(eccListener[0].lock())) {
        if (result.operationtype() == securityStub::OperationType::OP_TYPE_VERIFY) {
            sp->onVerificationResult(uniqueId, ec, resultData);
        } else if (result.operationtype() == securityStub::OperationType::OP_TYPE_CALCULATE) {
            sp->onCalculationResult(uniqueId, ec, resultData);
        } else {
            LOG(ERROR, __FUNCTION__, " Unknown operation type for unique id ", uniqueId);
        }
        return;
    }

    LOG(ERROR, __FUNCTION__, " Listener type mismatch or expired, result dropped, unique id ",
        uniqueId);
}

/**
 * Initiates an asynchronous ECC digest verification operation.
 *
 * This function sends an ECC verification request to the server. If the
 * result delivery mode is `MODE_ASYNC_LISTENER`, it dispatches a task to
 * an internal dispatcher to deliver the result asynchronously via a callback.
 * Otherwise, it assumes the result will be polled later.
 */
telux::common::ErrorCode CryptoAcceleratorManagerImpl::eccPostDigestForVerification(
    const DataDigest &digest, const ECCPoint &publicKey, const Signature &signature,
    telux::sec::ECCCurve curve, uint32_t uniqueId, telux::sec::RequestPriority priority) {

    securityStub::EccVerificationRequest request;
    grpc::ClientContext clientCtx{};
    securityStub::EccPostVerificationResponse response{};
    grpc::Status reqStatus{};
    telux::common::ErrorCode ec;
    telux::common::Status status;
    int delay;
    securityStub::OperationResult opResult;

    if (resultDeliveryMode_ == Mode::MODE_SYNC) {
        LOG(ERROR, __FUNCTION__, " Mismatched mode and verification API");
        return telux::common::ErrorCode::INVALID_STATE;
    }

    request.set_digest(digest.digest, digest.digestLength);
    request.set_public_key_x(publicKey.x, publicKey.xLength);
    request.set_public_key_y(publicKey.y, publicKey.yLength);
    request.set_signature_r(signature.rSignature, signature.rsLength);
    request.set_signature_s(signature.sSignature, signature.rsLength);
    request.set_curve(convertCurveTeluxToGrpc(curve));
    request.set_uniqueid(uniqueId);
    request.set_priority(convertPriorityTeluxToGrpc(priority));
    request.set_mode(static_cast<securityStub::Mode>(resultDeliveryMode_));

    reqStatus = stub_->EccPostDigestForVerification(&clientCtx, request, &response);

    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Communication error");
        return telux::common::ErrorCode::TRANSPORT_ERROR;
    }

    ec = static_cast<telux::common::ErrorCode>(response.error_code());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Operation failed with status: ", static_cast<int>(status));
        return ec; // Return the specific error from the backend.
    }

    if (resultDeliveryMode_ == Mode::MODE_ASYNC_LISTENER) {
        opResult.set_id(uniqueId);
        opResult.set_operationtype(securityStub::OperationType::OP_TYPE_VERIFY);
        opResult.set_error_code(static_cast<commonStub::ErrorCode>(ec));
        opResult.set_data(response.resultdata());

        asyncResultAndSsrDispatcher_->submitTask(
            [this, opResult, delay]() { this->deliverResultAsync(opResult, delay); });
    }

    return telux::common::ErrorCode::SUCCESS;
}

/**
 * Performs a synchronous ECC digest verification operation.
 *
 * This function sends an ECC verification request to the server and waits for
 * the synchronous response. The result data is populated into the provided
 * `resultData` vector upon successful completion.
 */
telux::common::ErrorCode CryptoAcceleratorManagerImpl::eccVerifyDigest(const DataDigest &digest,
    const ECCPoint &publicKey, const Signature &signature, telux::sec::ECCCurve curve,
    uint32_t uniqueId, telux::sec::RequestPriority priority, std::vector<uint8_t> &resultData) {

    securityStub::EccVerificationRequest request;
    grpc::ClientContext clientCtx{};
    securityStub::EccVerificationResponse response{};
    grpc::Status reqStatus{};
    telux::common::ErrorCode ec;

    if (resultDeliveryMode_ != Mode::MODE_SYNC) {
        LOG(ERROR, __FUNCTION__, " Mismatched mode and verification API");
        return telux::common::ErrorCode::INVALID_STATE;
    }

    request.set_digest(digest.digest, digest.digestLength);
    request.set_public_key_x(publicKey.x, publicKey.xLength);
    request.set_public_key_y(publicKey.y, publicKey.yLength);
    request.set_signature_r(signature.rSignature, signature.rsLength);
    request.set_signature_s(signature.sSignature, signature.rsLength);
    request.set_curve(convertCurveTeluxToGrpc(curve));
    request.set_uniqueid(uniqueId);
    request.set_priority(convertPriorityTeluxToGrpc(priority));

    reqStatus = stub_->EccVerifyDigest(&clientCtx, request, &response);

    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Communication error");
        return telux::common::ErrorCode::TRANSPORT_ERROR;
    }

    ec = static_cast<telux::common::ErrorCode>(response.error_code());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " API returned error: ", static_cast<int>(ec));
        return ec;
    }

    resultData.assign(response.resultdata().begin(), response.resultdata().end());

    return telux::common::ErrorCode::SUCCESS;
}

/**
 * Initiates an asynchronous ECQV point multiply and add calculation.
 *
 * This function sends an ECQV calculation request to the server. If the
 * result delivery mode is `MODE_ASYNC_LISTENER`, it dispatches a task to
 * an internal dispatcher to deliver the result asynchronously via a callback.
 * Otherwise, it assumes the result will be polled later.
 */
telux::common::ErrorCode CryptoAcceleratorManagerImpl::ecqvPostDataForMultiplyAndAdd(
    const ECCPoint &multiplicandPoint, const ECCPoint &addendPoint, const Scalar &scalar,
    telux::sec::ECCCurve curve, uint32_t uniqueId, telux::sec::RequestPriority priority) {


    securityStub::EcqvRequest request;
    grpc::ClientContext clientCtx{};
    securityStub::EcqvPostResponse response;
    grpc::Status reqStatus{};
    telux::common::ErrorCode ec;
    telux::common::Status status;
    int delay;
    securityStub::OperationResult opResult;

    if (resultDeliveryMode_ == Mode::MODE_SYNC) {
        LOG(ERROR, __FUNCTION__, " Mismatched mode and calculation API");
        return telux::common::ErrorCode::INVALID_STATE;
    }

    request.set_multiplicand_point_x(multiplicandPoint.x, multiplicandPoint.xLength);
    request.set_multiplicand_point_y(multiplicandPoint.y, multiplicandPoint.yLength);
    request.set_addend_point_x(addendPoint.x, addendPoint.xLength);
    request.set_addend_point_y(addendPoint.y, addendPoint.yLength);
    request.set_scalar(scalar.scalar, scalar.scalarLength);
    request.set_curve(convertCurveTeluxToGrpc(curve));
    request.set_uniqueid(uniqueId);
    request.set_priority(convertPriorityTeluxToGrpc(priority));
    request.set_mode(static_cast<securityStub::Mode>(resultDeliveryMode_));

    reqStatus = stub_->EcqvPostDataForMultiplyAndAdd(&clientCtx, request, &response);

    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Communication error");
        return telux::common::ErrorCode::TRANSPORT_ERROR;
    }

    ec = static_cast<telux::common::ErrorCode>(response.error_code());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Operation failed with status: ", static_cast<int>(status));
        return ec; // Return the specific error from the backend.
    }

    if (resultDeliveryMode_ == Mode::MODE_ASYNC_LISTENER) {
        opResult.set_id(uniqueId);
        opResult.set_operationtype(securityStub::OperationType::OP_TYPE_CALCULATE);
        opResult.set_error_code(static_cast<commonStub::ErrorCode>(ec));
        opResult.set_data(response.resultdata());

        asyncResultAndSsrDispatcher_->submitTask(
            [this, opResult, delay]() { this->deliverResultAsync(opResult, delay); });
    }

    return telux::common::ErrorCode::SUCCESS;
}

/**
 * Performs a synchronous ECQV point multiply and add calculation.
 *
 * This function sends an ECQV calculation request to the server and waits for
 * the synchronous response. The result data (concatenation of X and Y
 * coordinates) is populated into the provided `resultData` vector upon
 * successful completion.
 */
telux::common::ErrorCode CryptoAcceleratorManagerImpl::ecqvPointMultiplyAndAdd(
    const ECCPoint &multiplicandPoint, const ECCPoint &addendPoint, const Scalar &scalar,
    telux::sec::ECCCurve curve, uint32_t uniqueId, telux::sec::RequestPriority priority,
    std::vector<uint8_t> &resultData) {


    securityStub::EcqvRequest request;
    grpc::ClientContext clientCtx{};
    securityStub::EcqvResponse response{};
    grpc::Status reqStatus{};
    telux::common::ErrorCode ec;

    if (resultDeliveryMode_ != Mode::MODE_SYNC) {
        LOG(ERROR, __FUNCTION__, " Mismatched mode and calculation API");
        return telux::common::ErrorCode::INVALID_STATE;
    }

    request.set_multiplicand_point_x(multiplicandPoint.x, multiplicandPoint.xLength);
    request.set_multiplicand_point_y(multiplicandPoint.y, multiplicandPoint.yLength);
    request.set_addend_point_x(addendPoint.x, addendPoint.xLength);
    request.set_addend_point_y(addendPoint.y, addendPoint.yLength);
    request.set_scalar(scalar.scalar, scalar.scalarLength);
    request.set_curve(convertCurveTeluxToGrpc(curve));
    request.set_uniqueid(uniqueId);
    request.set_priority(convertPriorityTeluxToGrpc(priority));

    reqStatus = stub_->EcqvPointMultiplyAndAdd(&clientCtx, request, &response);

    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Communication error ");
        return telux::common::ErrorCode::TRANSPORT_ERROR;
    }

    ec = static_cast<telux::common::ErrorCode>(response.error_code());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " API returned error: ", static_cast<int>(ec));
        return ec;
    }

    resultData.assign(response.resultdata().begin(), response.resultdata().end());

    return telux::common::ErrorCode::SUCCESS;
}

/**
 * Retrieves asynchronous operation results from the server.
 *
 * This function is used when the result delivery mode is `MODE_ASYNC_POLL`.
 * It sends a request to the server to get a specified number of pending
 * asynchronous results and populates them into the provided `results` vector.
 * The function blocks until results are available or a timeout occurs.
 */
telux::common::ErrorCode CryptoAcceleratorManagerImpl::getAsyncResults(
    std::vector<OperationResult> &results, uint32_t numResultsToRead, int32_t timeout,
    uint32_t &numResultsRead) {


    securityStub::GetAsyncResultsRequest request;
    securityStub::GetAsyncResultsResponse response{};
    grpc::ClientContext clientCtx{};
    grpc::Status reqStatus{};
    telux::common::ErrorCode ec;
    size_t data_len;
    OperationResult telux_op_result;

    if (resultDeliveryMode_ != Mode::MODE_ASYNC_POLL) {
        LOG(ERROR, __FUNCTION__, " Invalid mode");
        return telux::common::ErrorCode::INVALID_STATE;
    }

    if (numResultsToRead > results.size()) {
        LOG(ERROR, __FUNCTION__, " insufficient memory provided");
        return telux::common::ErrorCode::NO_MEMORY;
    }

    request.set_numresultstoread(numResultsToRead);
    request.set_timeout(timeout);

    reqStatus = stub_->GetAsyncResults(&clientCtx, request, &response);

    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Communication error ");
        return telux::common::ErrorCode::TRANSPORT_ERROR;
    }

    ec = static_cast<telux::common::ErrorCode>(response.error_code());
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " API returned error: ", static_cast<int>(ec));
        return ec;
    }

    numResultsRead = response.numresultsread();
    results.clear(); // Clear any existing data in the output vector.
    results.reserve(numResultsRead); // Reserve space for efficiency.

    for (int i = 0; i < response.results_size(); ++i) {
        const auto& grpc_op_result = response.results(i);
        telux_op_result.id = grpc_op_result.id();
        telux_op_result.operationType = static_cast<uint32_t>(grpc_op_result.operationtype());
        telux_op_result.errCode = grpc_op_result.error_code();

        data_len = std::min((size_t)CA_RESULT_DATA_LENGTH, (size_t)grpc_op_result.data().length());
        memcpy(telux_op_result.data, grpc_op_result.data().data(), data_len);
        if (data_len < CA_RESULT_DATA_LENGTH) {
            memset(telux_op_result.data + data_len, 0, CA_RESULT_DATA_LENGTH - data_len);
        }

        results.push_back(telux_op_result);
    }

    return telux::common::ErrorCode::SUCCESS;
}

}  // End of namespace sec
}  // End of namespace telux