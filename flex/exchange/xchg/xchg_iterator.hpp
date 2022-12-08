/** \file
 *  \brief Exchange orders iterator for PriceXchg.
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) EverFlex Inc
 */

#pragma once

#include "../PriceXchg.hpp"
#include "orders_queue.hpp"

namespace tvm { namespace xchg {

/// \brief Tip3/tip3 exchange orders iterator.
/** Skips expired and out-of-evers orders. Notifies process_queue_state about such orders. **/
class xchg_iterator {
public:
  xchg_iterator(
    process_queue_state&      state,      ///< Common iteration state with calculating message limits
    orders_queue_cached&      orders,     ///< Orders queue
    uint128                   deal_costs, ///< Deal costs in evers
    bool                      sell        ///< Is it a sell order
  ) : state_(state), orders_(orders), deal_costs_(deal_costs), sell_(sell) {
  }

  /// Move to the first active order in the queue
  bool first_active() {
    while (!orders_.empty() && !state_.overlimit()) {
      OrderInfoXchgWithIdx& ord_idx = orders_.front_with_idx();
      [[maybe_unused]] auto [idx, ord] = ord_idx;

      // If the order is expired
      if (!is_active_time(ord.order_finish_time)) {
        state_.on_expired(ord_idx, sell_);
        orders_.pop();
        continue;
      }
      // If the order has not enough evers for processing
      if (ord.account < deal_costs_) {
        state_.on_out_of_evers(ord_idx, sell_);
        orders_.pop();
        continue;
      }
      return true;
    }
    return false;
  }

  OrderInfoXchgWithIdx& cur() { return orders_.front_with_idx(); }

  OrderInfoXchg& operator*() {
    return cur().second;
  }

  /// When deal is completed
  void on_deal(uint128 deal_amount, uint128 costs, uint128 lend_spent) {
    auto& ord_idx = cur();
    [[maybe_unused]] auto& [idx, ord] = ord_idx;
    ord.amount -= deal_amount;
    ord.account -= costs;
    ord.lend_amount -= lend_spent;

    // If order is done
    if (state_.is_order_done(ord)) {
      state_.on_order_done(ord_idx, sell_);
      orders_.pop();
    // If order is out-of-evers
    } else if (ord.account < deal_costs_) {
      state_.on_out_of_evers(ord_idx, sell_);
      orders_.pop();
    }
  }

  /// When order is out-of-evers
  void drop_with_ooc() {
    state_.on_out_of_evers(cur(), sell_);
    orders_.pop();
  }

  process_queue_state&      state_;      ///< Processing orders queue state for PriceXchg
  orders_queue_cached&      orders_;     ///< Orders queue
  uint128                   deal_costs_; ///< Deal processing costs
  bool                      sell_;       ///< Is it a sell order
};

}} // namespace tvm::xchg

