/** \file
 *  \brief Flex evers configuration (processing costs)
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#pragma once

namespace tvm {

/// Processing native funds value ...
struct EversConfig {
  /// ... for executing tip3 transfer
  uint128 transfer_tip3;
  /// ... to return tip3 ownership
  uint128 return_ownership;
  /// .. to send answer message from PriceXchg
  uint128 order_answer;
  /// \brief ... to process processQueue function.
  /// Also is used for buyTip3 / onTip3LendOwnership / cancelSell / cancelBuy estimations.
  uint128 process_queue;
  /// ... to send notification about completed deal (IFlexNotify)
  uint128 send_notify;
  /// ... to keep evers in destination wallet
  uint128 dest_wallet_keep_evers;
};

} // namespace tvm
