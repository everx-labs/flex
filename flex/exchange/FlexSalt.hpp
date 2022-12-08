/** \file
 *  \brief Flex contract configuration structure (stored in code salt)
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) EverFlex Inc
 */

#pragma once

#include <tvm/schema/message.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>

namespace tvm {

/// Flex configuration structure (stored in code salt)
struct FlexSalt {
  address     super_root;      ///< SuperRoot address
  EversConfig ev_cfg;          ///< Processing costs configuration of Flex in native funds (evers).
  uint8       deals_limit;     ///< Limit for processed deals in one request.
  cell        xchg_pair_code;  ///< Code of XchgPair contract (unsalted).
  cell        xchg_price_code; ///< Code of PriceXchg contract (unsalted).
};

} // namespace tvm

