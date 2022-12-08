/** \file
 *  \brief Price common interfaces and data-structs
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) EverFlex Inc
 */

#pragma once

#include <tvm/suffixes.hpp>

#include "RationalValue.hpp"

namespace tvm {

/// Notification struct about order to a client
struct OrderRet {
  uint32  err_code;       ///< Error code (zero if success)
  uint128 processed;      ///< Processed major tokens amount
  uint128 enqueued;       ///< Enqueued major tokens amount
  uint128 price_num;      ///< Price numerator
  uint128 price_denum;    ///< Price denominator
  uint256 user_id;        ///< User id
  uint256 order_id;       ///< Order id
  address pair;           ///< XchgPair address
  uint8   major_decimals; ///< Decimals for the major token
  uint8   minor_decimals; ///< Decimals for the minor token
  bool    sell;           ///< Is it a sell order
};

/** \interface IPriceCallback
 *  \brief Notifications to a client about orders.
 */
__interface IPriceCallback {
  [[internal, noaccept]]
  void onOrderFinished(
    OrderRet ret ///< Notification details
  ) = 300;
};
using IPriceCallbackPtr = handle<IPriceCallback>;

static constexpr price_t taker_fee = { 15u128, 10000u128 }; ///< Taker fee: 15/10000 = 0.15%
static constexpr price_t maker_vig = {  3u128, 10000u128 }; ///< Maker vig:  3/10000 = 0.03%

static_assert(taker_fee.denum == maker_vig.denum, "Inconsistent taker fee / maker vig denominators");
static_assert(taker_fee.num > maker_vig.num, "Inconsistent taker fee / maker vig numerators");

/// Calculate lend tokens for order (including fees)
__always_inline
uint128 calc_lend_tokens_for_order(bool sell, uint128 major_tokens, price_t price) {
  auto tokens = sell ? major_tokens : (major_tokens * price);
  return tokens + tokens * taker_fee;
}

} // namespace tvm

