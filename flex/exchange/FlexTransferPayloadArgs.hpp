/** \file
 *  \brief Flex transfer payload args structure
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#pragma once

namespace tvm {

/// Notification payload for wallet->transferWithNotify()
struct FlexTransferPayloadArgs {
  bool       sender_sell;       ///< Sender is seller in deal (selling major tokens)
  bool       sender_taker;      ///< Sender is a taker in deal (and pays fees)
  uint256    sender_user_id;    ///< Sender user id for client purposes.
  uint256    receiver_user_id;  ///< Receiver user id for client purposes.
  uint256    receiver_order_id; ///< Receiver order id for client purposes.
  address    another_tip3_root; ///< Address of another tip3 root (Wrapper) in trading pair.
  uint128    price_num;         ///< Price numerator
  uint128    price_denum;       ///< Price denominator
  uint128    taker_fee;         ///< Tokens taken (fee) from taker
  uint128    maker_vig;         ///< Tokens given (vig) to maker
  address    pair;              ///< Address of XchgPair contract.
  Tip3Config major_tip3cfg;     ///< Configuration of the major tip3 token.
  Tip3Config minor_tip3cfg;     ///< Configuration of the minor tip3 token.
};

} // namespace tvm
