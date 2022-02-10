/** \file
 *  \brief Flex lend payload args structure (for onTip3LendOwnership payload)
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#pragma once

namespace tvm {

/// Order arguments in payload
struct FlexLendPayloadArgs {
  bool    sell;                       ///< Sell order if true, buy order if false.
  bool    immediate_client;           ///< Should this order try to be executed as a client order first
                                      ///<  (find existing corresponding orders).
  bool    post_order;                 ///< Should this order be enqueued if it doesn't already have corresponding orders.
  uint128 amount;                     ///< Amount of major tokens to buy or sell.
  addr_std_fixed receive_tip3_wallet; ///< Client tip3 wallet to receive tokens (minor for sell or major for buy)
  addr_std_fixed client_addr;         ///< Client contract address. PriceXchg will execute cancels from this address,
                                      ///<  send notifications and return the remaining native funds (evers) to this address.
  uint256 user_id;                    ///< User id for client purposes.
  uint256 order_id;                   ///< Order id for client purposes.
};

} // namespace tvm
