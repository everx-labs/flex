/** \file
 *  \brief Error codes for PriceXchg.
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#pragma once

namespace tvm { namespace xchg {

static constexpr unsigned ok = 0;
/// Error codes for PriceXchg.
struct ec : tvm::error_code {
  static constexpr unsigned out_of_tons                    = 100; ///< Partially processed because out of tons
  static constexpr unsigned deals_limit                    = 101; ///< Partially processed because deals limit
  static constexpr unsigned not_enough_tons_to_process     = 102; ///< Not enough native funds to process (crystals)
  static constexpr unsigned not_enough_tokens_amount       = 103; ///< Not enough tokens amount
  static constexpr unsigned too_big_tokens_amount          = 104; ///< Too big calculated tokens amount
  static constexpr unsigned unverified_tip3_wallet         = 105; ///< Unverified tip3 token wallet
  static constexpr unsigned canceled                       = 106; ///< Order is canceled
  static constexpr unsigned expired                        = 107; ///< Order is expired
  static constexpr unsigned no_post_order_partially_done   = 108; ///< Order without post-order flag is partially done
  static constexpr unsigned incorrect_price                = 109; ///< Incorrect price

  /// \brief When an order without 'immediate_client' flag comes to a PriceXchg with enqueued orders of other side.
  /** New sell order comes to a PriceXchg with enqueued buy orders.
      Or new buy order comes to a PriceXchg with enqueued sell orders. **/
  static constexpr unsigned have_other_side_with_non_immediate_client = 110;
  /// \brief When an order without 'post_order' flag comes to a PriceXchg with enqueued orders of this side.
  /** New sell order comes to a PriceXchg with enqueued sell orders.
      Or new buy order comes to a PriceXchg with enqueued buy orders. **/
  static constexpr unsigned have_this_side_with_non_post_order = 111;
};

}} // namespace tvm::xchg

