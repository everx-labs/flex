/** \file
 *  \brief Flex version structure
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#pragma once

#include <tvm/schema/basics.hpp>

namespace tvm {

/// Flex version triplet
struct FlexVersion {
  uint32 wallet;   ///< Version of token wallets contracts
  uint32 exchange; ///< Version of exchange contracts (Flex root, pair, price)
  uint32 user;     ///< Version of user contracts (FlexClient)

  __always_inline
  bool is_null() const { return wallet == 0 && exchange == 0 && user == 0; }
};

} // namespace tvm
