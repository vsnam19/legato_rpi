/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <telux/sec/CryptoManager.hpp>

#include "libs/common/Logger.hpp"
#include "libs/common/CommonUtils.hpp"
#include "libs/common/JsonParser.hpp"

#include "SecurityCryptoAcceleratorServerImpl.hpp"

SecurityCryptoAcceleratorServerImpl::SecurityCryptoAcceleratorServerImpl()
    : serverEvent_(ServerEventManager::getInstance())
    , clientEvent_(EventService::getInstance()) {
    LOG(DEBUG, __FUNCTION__);
}

SecurityCryptoAcceleratorServerImpl::~SecurityCryptoAcceleratorServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

// Helper function to convert a hexadecimal string to a byte string.
std::string SecurityCryptoAcceleratorServerImpl::hex_to_bytes(const std::string& hex) {

    std::string bytes;
    char* end = nullptr;
    long value = 0;

    // Hex string must have an even length.
    if (hex.length() % 2 != 0) {
        LOG(ERROR, __FUNCTION__, " invalid hex string length");
        return std::string();
    }

    bytes.reserve(hex.length() / 2);
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        value = strtol(byteString.c_str(), &end, 16);
        // Check for conversion errors or out-of-range values.
        if (*end != '\0' || value < 0 || value > 255) {
            LOG(ERROR, __FUNCTION__, " invalid hex character in string");
            return std::string();
        }
        bytes.push_back(static_cast<char>(value));
    }
    return bytes;
}

// Initializes the simulation server by reading API configuration and database files.
grpc::Status SecurityCryptoAcceleratorServerImpl::Init(::grpc::ServerContext* context,
    const ::google::protobuf::Empty* request,
    ::commonStub::ErrorCodeMsg* response) {

    telux::common::ErrorCode ec = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status;

    // Read API configuration JSON.
    if (!apiConfigJsonRoot_) {
        ec = JsonParser::readFromJsonFile(apiConfigJsonRoot_, cryptoAccMgr_API_JSON_FILE);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " can't read ", cryptoAccMgr_API_JSON_FILE);
            response->set_ec(static_cast<commonStub::ErrorCode>(
                telux::common::ErrorCode::SYSTEM_ERR));
            return grpc::Status::OK;
        }
    }

    // Read database JSON.
    if(!databaseJsonRoot_) {
        ec = JsonParser::readFromJsonFile(databaseJsonRoot_, cryptoAccMgr_DATABASE_FILE);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " can't read ", cryptoAccMgr_DATABASE_FILE);
            response->set_ec(static_cast<commonStub::ErrorCode>(
                telux::common::ErrorCode::SYSTEM_ERR));
            return grpc::Status::OK;
        }
    }

    status = serverEvent_.registerListener(shared_from_this(), CRYPTOACC_FILTER);
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " can't register with ServerEventManager");
        response->set_ec(commonStub::ErrorCode::SYSTEM_ERR);
        return grpc::Status::OK;
    }

    // If already initialized, return success.
    if (isServiceInitialized_) {
        response->set_ec(static_cast<commonStub::ErrorCode>(
            telux::common::ErrorCode::SUCCESS));
        return grpc::Status::OK;
    }

    response->set_ec(static_cast<commonStub::ErrorCode>(
        telux::common::ErrorCode::SUCCESS));

    clientsCount_++;
    isServiceInitialized_ = true;
    return grpc::Status::OK;
}

// Deinitializes the simulation server.
grpc::Status SecurityCryptoAcceleratorServerImpl::DeInit(::grpc::ServerContext* context,
    const ::google::protobuf::Empty* request,
    ::commonStub::ErrorCodeMsg* response) {

    clientsCount_--;
    if (!clientsCount_) {
         serverEvent_.deregisterListener(shared_from_this(), CRYPTOACC_FILTER);
        isServiceInitialized_ = false;
    }

    response->set_ec(static_cast<commonStub::ErrorCode>(
        telux::common::ErrorCode::SUCCESS));

    return grpc::Status::OK;
}

/*
 * Handle events injected externally by the user.
 */
void SecurityCryptoAcceleratorServerImpl::onEventUpdate(::eventService::UnsolicitedEvent usrEvent) {
    LOG(DEBUG,__FUNCTION__);

    std::string event;
    std::string token;

    if (usrEvent.filter() != CRYPTOACC_FILTER) {
        return;
    }

    event = usrEvent.event();

    token = EventParserUtil::getNextToken(event, " ");
    if (token == "ssr") {
        handleSSREvent(event);
    }
}

/*
 * Receive report injected by the user. Translate it and dispatch for application.
 *
 * telsdk_event_injector -f ccs -e ssr SERVICE_UNAVAILABLE
 */
void SecurityCryptoAcceleratorServerImpl::handleSSREvent(std::string eventParams) {

    ::securityStub::CryptoAccServiceStatus newServiceState{};
    ::eventService::EventResponse anyResponse{};

    if (eventParams == "SERVICE_AVAILABLE") {
        newServiceState.set_service_status(commonStub::ServiceStatus::SERVICE_AVAILABLE);
    } else if (eventParams == "SERVICE_UNAVAILABLE" || eventParams == "SERVICE_FAILED") {
        newServiceState.set_service_status(commonStub::ServiceStatus::SERVICE_UNAVAILABLE);
    } else {
        LOG(ERROR, __FUNCTION__, " invalid parameters: ", eventParams);
        return;
    }

    anyResponse.set_filter(CRYPTOACC_FILTER);
    anyResponse.mutable_any()->PackFrom(newServiceState);
    clientEvent_.updateEventQueue(anyResponse);
}


/*
 * Sync ECC verify.
 * If the JSON response indicates a successful operation, proceed to extract the 'r' component from
 * the digital signature. This component is then returned as the result.
 */
grpc::Status SecurityCryptoAcceleratorServerImpl::EccVerifyDigest(::grpc::ServerContext* context,
    const ::securityStub::EccVerificationRequest* request,
    ::securityStub::EccVerificationResponse* response) {

    std::string ecStr;
    std::string signature_r_from_request;
    const size_t EXPECTED_RESULT_LENGTH = 128; // 256 hex characters = 128 bytes
    std::string padded_result_data;

    // Handle common error simulation based on API config.
    ecStr = apiConfigJsonRoot_["ICryptoAcceleratorManager"]
                                         ["eccVerifyDigest"]["error"].asString();
    if (handleAndSetCommonErrorCode(ecStr, __FUNCTION__, " failed to verify ECC digest",
                                    response)) {
        return grpc::Status::OK;
    }

    // Get the signature_r from the request.
    signature_r_from_request = request->signature_r();
    if (signature_r_from_request.empty()) {
        LOG(WARNING, __FUNCTION__,
            " signature_r field is empty in the request, returning empty resultData.");
    }

    // Simulate result by padding or truncating signature_r to an expected length.
    padded_result_data = signature_r_from_request;

    if (padded_result_data.length() < EXPECTED_RESULT_LENGTH) {
        padded_result_data.resize(EXPECTED_RESULT_LENGTH, '\0'); // Pad with null bytes
    } else if (padded_result_data.length() > EXPECTED_RESULT_LENGTH) {
        LOG(WARNING, __FUNCTION__, " signature_r from request (",
            signature_r_from_request.length(), " bytes) is longer than expected (",
            EXPECTED_RESULT_LENGTH, " bytes), truncating.");
        padded_result_data.resize(EXPECTED_RESULT_LENGTH);
    }

    response->set_resultdata(padded_result_data);
    response->set_error_code(static_cast<commonStub::ErrorCode>(
        telux::common::ErrorCode::SUCCESS));
    return grpc::Status::OK;
}


/*
 * Sync ECQV calculation.
 * Check if the operation completed successfully by verifying the error code.
 * If the error code indicates success, proceed to validate the result by
 * comparing the input parameters (e.g., expected values or computed results)
 * against the corresponding values retrieved from the JSON response.
 */
grpc::Status SecurityCryptoAcceleratorServerImpl::EcqvPointMultiplyAndAdd(
    ::grpc::ServerContext* context, const ::securityStub::EcqvRequest* request,
    ::securityStub::EcqvResponse* response) {

    std::string ecStr;
    std::string curveStr = "";
    const Json::Value& curveData = Json::nullValue; // Initialize with null value
    bool foundMatch = false;
    std::string result_x_hex;
    std::string result_y_hex;
    std::string db_scalar;
    std::string db_mp_x;
    std::string db_mp_y;
    std::string db_ap_x;
    std::string db_ap_y;
    std::string req_scalar_bytes;
    std::string req_mp_x_bytes;
    std::string req_mp_y_bytes;
    std::string req_ap_x_bytes;
    std::string req_ap_y_bytes;
    std::string result_x_bytes;
    std::string result_y_bytes;

    // Handle common error simulation based on API config.
    ecStr = apiConfigJsonRoot_["ICryptoAcceleratorManager"]
                                         ["ecqvPointMultiplyAndAdd"]["error"].asString();
    if (handleAndSetCommonErrorCode(ecStr, __FUNCTION__, " failed to perform ECQV operation",
                                    response)) {
        return grpc::Status::OK;
    }

    curveStr = mapCurveToString(request->curve());
    if (curveStr.empty()) {
        LOG(ERROR, __FUNCTION__, " Unknown or unsupported ECC curve type in request: ",
            request->curve());
        response->set_error_code(static_cast<commonStub::ErrorCode>(
            telux::common::ErrorCode::INVALID_ARGUMENTS));
        return grpc::Status::OK;
    }

    // Check if the base 'ecqvCalculation' exists in the database.
    if (!databaseJsonRoot_["ICryptoAcceleratorManager"].isMember("ecqvCalculation")) {
        LOG(ERROR, __FUNCTION__, " 'ecqvCalculation' data not found in database");
        response->set_error_code(static_cast<commonStub::ErrorCode>(
            telux::common::ErrorCode::INVALID_ARGUMENTS));
        return grpc::Status::OK;
    }

    // Check if the specific curve data exists under 'ecqvCalculation'.
    if (!databaseJsonRoot_["ICryptoAcceleratorManager"]["ecqvCalculation"].isMember(curveStr)) {
        LOG(ERROR, __FUNCTION__, " Curve data not found for ", curveStr,
            " under 'ecqvCalculation' in database");
        response->set_error_code(static_cast<commonStub::ErrorCode>(
            telux::common::ErrorCode::INVALID_ARGUMENTS));
        return grpc::Status::OK;
    }

    // Re-declare and initialize 'curveData' if it was already initialized with Json::nullValue
    const Json::Value& _curveData =
        databaseJsonRoot_["ICryptoAcceleratorManager"]["ecqvCalculation"][curveStr];

    // Attempt to find a matching entry in the database based on request parameters.
    if (_curveData.isMember("scalar") && _curveData.isMember("multiplicandPoint") &&
        _curveData.isMember("addendPoint") && _curveData.isMember("result")) {

        db_scalar = _curveData["scalar"].asString();
        db_mp_x = _curveData["multiplicandPoint"]["x"].asString();
        db_mp_y = _curveData["multiplicandPoint"]["y"].asString();
        db_ap_x = _curveData["addendPoint"]["x"].asString();
        db_ap_y = _curveData["addendPoint"]["y"].asString();

        // Convert request bytes to hex for comparison (or convert DB hex to bytes, both work).
        // Here, converting DB hex to bytes and comparing bytes for consistency.
        req_scalar_bytes = request->scalar();
        req_mp_x_bytes = request->multiplicand_point_x();
        req_mp_y_bytes = request->multiplicand_point_y();
        req_ap_x_bytes = request->addend_point_x();
        req_ap_y_bytes = request->addend_point_y();

        if (hex_to_bytes(db_scalar) == req_scalar_bytes &&
            hex_to_bytes(db_mp_x) == req_mp_x_bytes &&
            hex_to_bytes(db_mp_y) == req_mp_y_bytes &&
            hex_to_bytes(db_ap_x) == req_ap_x_bytes &&
            hex_to_bytes(db_ap_y) == req_ap_y_bytes) {

            result_x_hex = _curveData["result"]["x"].asString();
            result_y_hex = _curveData["result"]["y"].asString();
            foundMatch = true;
        }
    }

    if (!foundMatch) {
        LOG(ERROR, __FUNCTION__, " No matching ECQV input found for curve ", curveStr,
            " in database.");
        response->set_error_code(static_cast<commonStub::ErrorCode>(
            telux::common::ErrorCode::INVALID_ARGUMENTS));
        return grpc::Status::OK;
    }

    if (result_x_hex.empty() || result_y_hex.empty()) {
        LOG(ERROR, __FUNCTION__, " ECQV result (x or y) is empty for curve ", curveStr);
        response->set_error_code(static_cast<commonStub::ErrorCode>(
            telux::common::ErrorCode::INVALID_ARGUMENTS));
        return grpc::Status::OK;
    }

    result_x_bytes = hex_to_bytes(result_x_hex);
    result_y_bytes = hex_to_bytes(result_y_hex);

    if (result_x_bytes.empty() || result_y_bytes.empty()) {
        LOG(ERROR, __FUNCTION__,
            " Failed to convert ECQV result (x or y) hex string to bytes for curve ", curveStr);
        response->set_error_code(static_cast<commonStub::ErrorCode>(
            telux::common::ErrorCode::SYSTEM_ERR));
        return grpc::Status::OK;
    }

    // Concatenate the byte representations of x and y for the final result.
    response->set_resultdata(result_x_bytes + result_y_bytes);
    response->set_error_code(static_cast<commonStub::ErrorCode>(
        telux::common::ErrorCode::SUCCESS));
    return grpc::Status::OK;
}

/*
 * Async ECC verify.
 * If the JSON response indicates a successful operation, proceed to extract the 'r' component from
 * the digital signature. This component is then returned as the result.
 */
grpc::Status SecurityCryptoAcceleratorServerImpl::EccPostDigestForVerification(
    ::grpc::ServerContext* context, const ::securityStub::EccVerificationRequest* request,
    ::securityStub::EccPostVerificationResponse* response) {

    ApiResponse apiResp{};
    std::string ecStr;
    std::string result_data;
    const size_t EXPECTED_RESULT_LENGTH = 128;
    std::string signature_r_from_request;
    std::lock_guard<std::mutex> lock(asyncResultsMutex_); // Mutex guard for asyncResultsQueue_
    uint32_t op_id = 0;

    // Get API response simulation values (status, error, callback delay).
    CommonUtils::getValues(apiConfigJsonRoot_, "ICryptoAcceleratorManager",
                           "eccPostDigestForVerification", apiResp.status, apiResp.error,
                           apiResp.cbDelay);

    response->set_status(static_cast<commonStub::Status>(apiResp.status));
    if (telux::common::Status::SUCCESS != apiResp.status) {
        return grpc::Status::OK;
    }

    // Handle common error simulation based on API config.
    ecStr = apiConfigJsonRoot_["ICryptoAcceleratorManager"]
                                         ["ecqvPointMultiplyAndAdd"]["error"].asString();
    if (handleAndSetCommonErrorCode(ecStr, __FUNCTION__, " failed to perform ECC operation",
                                    response)) {
        return grpc::Status::OK;
    }

    // Simulate result by padding or truncating signature_r from the request.
    signature_r_from_request = request->signature_r();
    result_data = signature_r_from_request;

    if (result_data.length() < EXPECTED_RESULT_LENGTH) {
        result_data.resize(EXPECTED_RESULT_LENGTH, '\0');
    } else if (result_data.length() > EXPECTED_RESULT_LENGTH) {
        LOG(WARNING, __FUNCTION__, " signature_r from request (",
            signature_r_from_request.length(), " bytes) is longer than expected (",
            EXPECTED_RESULT_LENGTH, " bytes), truncating.");
        result_data.resize(EXPECTED_RESULT_LENGTH);
    }

    // Handle synchronous vs. asynchronous response based on request mode.
    if (request->mode() == securityStub::Mode::MODE_ASYNC_LISTENER) {
        response->set_resultdata(result_data);
        response->set_error_code(static_cast<commonStub::ErrorCode>(apiResp.error));
        response->set_delay(apiResp.cbDelay);
    } else {
        // For polling mode, store results in a queue.
        op_id = request->uniqueid(); // Use the uniqueId from the request.
        asyncResultsQueue_.push_back(
            {op_id, securityStub::OP_TYPE_VERIFY, result_data, apiResp.error});
        LOG(INFO, __FUNCTION__, " ECC Verification result (ID: ", op_id,
            ") stored for async retrieval. Error Code: ", static_cast<int>(apiResp.error));
    }

    return grpc::Status::OK;
}

// Helper function to map securityStub::Curve enum to a string representation.
std::string SecurityCryptoAcceleratorServerImpl::mapCurveToString(securityStub::EccCurve curve) {
    switch (curve) {
        case securityStub::CURVE_SM2:
            return "sm2";
        case securityStub::CURVE_NISTP256:
            return "nistP256";
        case securityStub::CURVE_NISTP384:
            return "nistP384";
        case securityStub::CURVE_BRAINPOOLP256R1:
            return "brainpoolP256";
        case securityStub::CURVE_BRAINPOOLP384R1:
            return "brainpoolP384";
        default:
            return "";
    }
}

/*
 * Async ECQV calculation.
 * Check if the operation completed successfully by verifying the error code.
 * If the error code indicates success, proceed to validate the result by
 * comparing the input parameters (e.g., expected values or computed results)
 * against the corresponding values retrieved from the JSON response. If the mode is MODE_ASYNC_LISTENER then return the values to library to trigger a async callback from libs.otherwise store in the queue for results.
 */
grpc::Status SecurityCryptoAcceleratorServerImpl::EcqvPostDataForMultiplyAndAdd(
    ::grpc::ServerContext* context, const ::securityStub::EcqvRequest* request,
    ::securityStub::EcqvPostResponse* response) {

    ApiResponse apiResp{};
    std::string ecStr;
    std::string curveStr = "";
    std::string final_result_data;
    const Json::Value& curveData = Json::nullValue; // Initialize with null value
    bool foundMatch = false;
    std::string result_x_hex;
    std::string result_y_hex;
    std::string db_scalar;
    std::string db_mp_x;
    std::string db_mp_y;
    std::string db_ap_x;
    std::string db_ap_y;
    std::string req_scalar_bytes;
    std::string req_mp_x_bytes;
    std::string req_mp_y_bytes;
    std::string req_ap_x_bytes;
    std::string req_ap_y_bytes;
    std::string result_x_bytes;
    std::string result_y_bytes;
    std::lock_guard<std::mutex> lock(asyncResultsMutex_); // Mutex guard for asyncResultsQueue_
    uint32_t op_id = 0;

    // Get API response simulation values (status, error, callback delay).
    CommonUtils::getValues(apiConfigJsonRoot_, "ICryptoAcceleratorManager",
                           "ecqvPostDataForMultiplyAndAdd", apiResp.status, apiResp.error,
                           apiResp.cbDelay);

    response->set_status(static_cast<commonStub::Status>(apiResp.status));
    if (telux::common::Status::SUCCESS != apiResp.status) {
        return grpc::Status::OK;
    }

    // Handle common error simulation based on API config.
    ecStr = apiConfigJsonRoot_["ICryptoAcceleratorManager"]
                                         ["ecqvPostDataForMultiplyAndAdd"]["error"].asString();
    if (handleAndSetCommonErrorCode(ecStr, __FUNCTION__, " failed to perform ECQV operation",
                                    response)) {
        return grpc::Status::OK;
    }

    curveStr = mapCurveToString(request->curve());
    if (curveStr.empty()) {
        LOG(ERROR, __FUNCTION__, " Unknown or unsupported ECC curve type in request: ",
            request->curve());
        response->set_error_code(static_cast<commonStub::ErrorCode>(
            telux::common::ErrorCode::INVALID_ARGUMENTS));
        return grpc::Status::OK;
    }

    if (!databaseJsonRoot_.isMember("ICryptoAcceleratorManager") ||
        !databaseJsonRoot_["ICryptoAcceleratorManager"].isMember("ecqvCalculation")) {
        LOG(ERROR, __FUNCTION__, " 'ecqvCalculation' data not found in database");
        apiResp.error = telux::common::ErrorCode::INVALID_ARGUMENTS;
        response->set_error_code(static_cast<commonStub::ErrorCode>(apiResp.error));
        return grpc::Status::OK;
    } else if (!databaseJsonRoot_["ICryptoAcceleratorManager"]["ecqvCalculation"].isMember(curveStr)) {
        LOG(ERROR, __FUNCTION__, " Curve data not found for ", curveStr,
            " under 'ecqvCalculation' in database");
        apiResp.error = telux::common::ErrorCode::INVALID_ARGUMENTS;
        response->set_error_code(static_cast<commonStub::ErrorCode>(apiResp.error));
        return grpc::Status::OK;
    } else {
        // Re-declare and initialize 'curveData' if it was already initialized with Json::nullValue
        const Json::Value& _curveData =
            databaseJsonRoot_["ICryptoAcceleratorManager"]["ecqvCalculation"][curveStr];

        // Attempt to find a matching entry in the database based on request parameters.
        if (_curveData.isMember("scalar") && _curveData.isMember("multiplicandPoint") &&
            _curveData.isMember("addendPoint") && _curveData.isMember("result")) {

            db_scalar = _curveData["scalar"].asString();
            db_mp_x = _curveData["multiplicandPoint"]["x"].asString();
            db_mp_y = _curveData["multiplicandPoint"]["y"].asString();
            db_ap_x = _curveData["addendPoint"]["x"].asString();
            db_ap_y = _curveData["addendPoint"]["y"].asString();

            req_scalar_bytes = request->scalar();
            req_mp_x_bytes = request->multiplicand_point_x();
            req_mp_y_bytes = request->multiplicand_point_y();
            req_ap_x_bytes = request->addend_point_x();
            req_ap_y_bytes = request->addend_point_y();

            if (hex_to_bytes(db_scalar) == req_scalar_bytes &&
                hex_to_bytes(db_mp_x) == req_mp_x_bytes &&
                hex_to_bytes(db_mp_y) == req_mp_y_bytes &&
                hex_to_bytes(db_ap_x) == req_ap_x_bytes &&
                hex_to_bytes(db_ap_y) == req_ap_y_bytes) {

                result_x_hex = _curveData["result"]["x"].asString();
                result_y_hex = _curveData["result"]["y"].asString();
                foundMatch = true;
            }
        }

        if (!foundMatch) {
            LOG(ERROR, __FUNCTION__, " No matching ECQV input found for curve ", curveStr,
                " in database.");
            apiResp.error = telux::common::ErrorCode::INVALID_ARGUMENTS;
        } else if (result_x_hex.empty() || result_y_hex.empty()) {
            LOG(ERROR, __FUNCTION__, " ECQV result (x or y) is empty for curve ", curveStr);
            apiResp.error = telux::common::ErrorCode::INVALID_ARGUMENTS;
        } else {
            result_x_bytes = hex_to_bytes(result_x_hex);
            result_y_bytes = hex_to_bytes(result_y_hex);

            if (result_x_bytes.empty() || result_y_bytes.empty()) {
                LOG(ERROR, __FUNCTION__,
                    " Failed to convert ECQV result (x or y) hex string to bytes for curve ",
                    curveStr);
                apiResp.error = telux::common::ErrorCode::SYSTEM_ERR;
            } else {
                final_result_data = result_x_bytes + result_y_bytes;
            }
        }
    }

    // Handle synchronous vs. asynchronous response based on request mode.
    if (request->mode() == securityStub::Mode::MODE_ASYNC_LISTENER) {
        response->set_resultdata(final_result_data);
        response->set_error_code(static_cast<commonStub::ErrorCode>(apiResp.error));
        response->set_delay(apiResp.cbDelay);
    } else {
        // For polling mode, store results in a queue.
        op_id = request->uniqueid(); // Use the uniqueId from the request.
        asyncResultsQueue_.push_back(
            {op_id, securityStub::OP_TYPE_CALCULATE, final_result_data, apiResp.error});
        LOG(INFO, __FUNCTION__, " ECQV result (ID: ", op_id,
            ") stored for async retrieval. Error Code: ", static_cast<int>(apiResp.error));
        response->set_error_code(static_cast<commonStub::ErrorCode>(
            telux::common::ErrorCode::SUCCESS));
    }
    return grpc::Status::OK;
}

// Retrieves asynchronous operation results from the queue.
grpc::Status SecurityCryptoAcceleratorServerImpl::GetAsyncResults(
    ::grpc::ServerContext* context, const ::securityStub::GetAsyncResultsRequest* request,
    ::securityStub::GetAsyncResultsResponse* response) {

    uint32_t results_to_read = 0;
    std::lock_guard<std::mutex> lock(asyncResultsMutex_); // Mutex guard for asyncResultsQueue_

    if (asyncResultsQueue_.empty()) {
        LOG(INFO, __FUNCTION__, " No pending async results to retrieve.");
        response->set_error_code(static_cast<commonStub::ErrorCode>(
            telux::common::ErrorCode::SYSTEM_ERR));
        response->set_numresultsread(0);
        return grpc::Status::OK;
    }

    // Determine how many results to read, up to the requested number or queue size.
    results_to_read = request->numresultstoread();
    if (results_to_read == 0 || results_to_read > asyncResultsQueue_.size()) {
        results_to_read = asyncResultsQueue_.size();
    }

    // Populate the response with results from the queue.
    for (uint32_t i = 0; i < results_to_read; ++i) {
        const auto& res = asyncResultsQueue_[i];
        ::securityStub::OperationResult* opResult = response->add_results();
        opResult->set_id(res.id);
        opResult->set_operationtype(res.operation_type);
        opResult->set_data(res.result_data);
        opResult->set_error_code(static_cast<commonStub::ErrorCode>(res.error_code));
    }

    LOG(INFO, __FUNCTION__, " Retrieved ", results_to_read, " async results.");
    response->set_numresultsread(results_to_read);
    response->set_error_code(static_cast<commonStub::ErrorCode>(
        telux::common::ErrorCode::SUCCESS));

    // Remove retrieved results from the queue.
    asyncResultsQueue_.erase(asyncResultsQueue_.begin(),
                             asyncResultsQueue_.begin() + results_to_read);
    LOG(INFO, __FUNCTION__, " Remaining results in queue: ", asyncResultsQueue_.size());

    return grpc::Status::OK;
}
