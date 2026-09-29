/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "internal-temp.h"

#include "CryptoAcceleratorUtils.hpp"

#include <telux/sec/CryptoAcceleratorManager.hpp>

namespace telux {
namespace sec {

/*
 * Gives unique ID.
 */
uint32_t ResultParser::getId(const OperationResult &result) {

    return (result.id & 0xFFFU);
}

/*
 * Gives type of operation; ECC verification or ECQV point multiplication.
 */
OperationType ResultParser::getOperationType(const OperationResult &result) {

    return OperationType::OP_TYPE_CALCULATE;
}

/*
 * Indicates; verification/calculation succeeded or failed.
 */
telux::common::ErrorCode ResultParser::getErrorCode(const OperationResult &result) {

    return telux::common::ErrorCode::GENERIC_FAILURE;
}

/*
 * Gives further insight about failure cause. Specifically hardware PKE errors.
 */
telux::common::ErrorCode ResultParser::getCAErrorCode(const OperationResult &result) {

    return telux::common::ErrorCode::GENERIC_FAILURE;
}

/*
 * Gives raw result buffer obtained from accelerator as is.
 */
uint8_t *ResultParser::getData(OperationResult &result) {

    return result.data;
}

}  // End of namespace sec
}  // End of namespace telux
