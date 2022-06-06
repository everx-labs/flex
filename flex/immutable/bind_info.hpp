#pragma once

#include <tvm/schema/message.hpp>

namespace tvm {

/// Binding to allow orders only to specific flex root and with specific unsalted PriceXchg code hash
struct bind_info {
  address flex;                     ///< Flex root address
  uint256 unsalted_price_code_hash; ///< PriceXchg code hash (unsalted)
};

} // namespace tvm
