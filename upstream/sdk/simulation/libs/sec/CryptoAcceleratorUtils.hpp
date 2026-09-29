/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CRYPTOACCELERATORUTILS_HPP
#define CRYPTOACCELERATORUTILS_HPP

#include "internal-temp.h"

#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace sec {

class CryptoAcceleratorUtils {

 public:
    /* Translates libmvm specific error code to telsdk specific error code */
    static telux::common::ErrorCode caToTeluxErrorCode(MVM_RETURN acceleratorErrorNum);

    /* Translates public key engine (PKE) specific error code to telsdk specific error code */
    static telux::common::ErrorCode pkeToTeluxErrorCode(MVM_ERROR_STATUS pkeErrorNum);

 private:
    CryptoAcceleratorUtils() {
    }
};

}  // End of namespace sec
}  // End of namespace telux

#endif  // CRYPTOACCELERATORUTILS_HPP
