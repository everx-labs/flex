/** \file
 *  \brief Flex lend payload args structure (for onTip3LendOwnership payload)
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#pragma once

#include "Tip3Creds.hpp"

namespace tvm {

/// Order arguments in payload
/// \note Change of this structure requires FlexWallet (TONTokenWallet) update (!!!)
/// That's why this file belongs to tokens/fungible
/// Receiving wallet credentials will be { pubkey: user_id, owner: client_addr }.
struct FlexLendPayloadArgs {
  bool      sell;               ///< Sell order if true, buy order if false.
  bool      immediate_client;   ///< Should this order try to be executed as a client order first
                                ///<  (find existing corresponding orders).
  bool      post_order;         ///< Should this order be enqueued if it doesn't already have corresponding orders.
  uint128   amount;             ///< Amount of major tokens to buy or sell.
  address   client_addr;        ///< Client contract address. PriceXchg will execute cancels from this address,
                                ///<  send notifications and return the remaining native funds (evers) to this address.
  uint256   user_id;            ///< User id. It is trader wallet's pubkey. Receiving wallet credentials will be { pubkey: user_id, owner: client_addr }.
  uint256   order_id;           ///< Order id for client purposes.
};

} // namespace tvm
