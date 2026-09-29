/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "CryptoAcceleratorUtils.hpp"

namespace telux {
namespace sec {

/*
 * Translates libmvm specific error code to telsdk specific error code.
 */
telux::common::ErrorCode CryptoAcceleratorUtils::caToTeluxErrorCode(
    MVM_RETURN acceleratorErrorNum) {

    return telux::common::ErrorCode::GENERIC_FAILURE;
}

/*
 * Translates PKE specific error code to telsdk specific error code.
 */
telux::common::ErrorCode CryptoAcceleratorUtils::pkeToTeluxErrorCode(MVM_ERROR_STATUS pkeErrorNum) {

    return telux::common::ErrorCode::GENERIC_FAILURE;
}

}  // namespace sec
}  // namespace telux
