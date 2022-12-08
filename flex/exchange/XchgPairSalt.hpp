/** \file
 *  \brief Exchange pair (XchgPair) configuration structure (stored in code salt)
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) EverFlex Inc
 */

#pragma once

#include "FlexWallet.hpp"
#include "EversConfig.hpp"
#include "RationalValue.hpp"

#include <tvm/schema/message.hpp>

#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>

namespace tvm {

/// XchgPair configuration structure (stored in salt)
struct XchgPairSalt {
  address     flex;            ///< Flex root address
  EversConfig ev_cfg;          ///< Processing costs configuration of Flex in native funds (evers).
  uint8       deals_limit;     ///< Limit for processed deals in one request.
  cell        xchg_price_code; ///< Code of PriceXchg contract (unsalted).
};

} // namespace tvm

