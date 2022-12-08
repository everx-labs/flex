/** \file
 *  \brief Class for orders queue for PriceXchg.
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) EverFlex Inc
 */

#pragma once

namespace tvm { namespace xchg {

/// \brief Orders queue to keep orders and common state (tokens amount).
/** Working state of orders queue includes cached head order.
 *  This order may be modified "in memory" and stored back into queue (using change_front) only
 *   if order is partially processed at the end of process_queue.
 **/
class orders_queue {
public:
  uint128                  all_amount_; ///< Amount of tokens in all orders
  big_queue<OrderInfoXchg> orders_;     ///< Orders queue

  /// Is queue empty
  bool empty() const { return orders_.empty(); }

  /// Drop orders without post_order flag
  void drop_no_post_orders(process_queue_state& state, bool sell) {
    for (auto it = orders_.begin(); it != orders_.end();) {
      auto next_it = std::next(it);
      auto ord = *it;
      if (!ord.post_order) {
        state.on_no_post_order_done({it.idx_, (*it)}, sell);
        all_amount_ -= ord.amount;
        orders_.erase(it);
      }
      it = next_it;
    }
  }
};

/// \brief Working version of orders_queue with cached head order
class orders_queue_cached {
public:
  /// Constructor
  explicit orders_queue_cached(orders_queue& q) : q_(q) {}

  /// Is queue empty
  bool empty() const { return q_.orders_.empty(); }

  /// Get queue head (front) with caching
  OrderInfoXchgWithIdx& front_with_idx() {
    if (!head_) {
      head_ = q_.orders_.front_with_idx_opt();
      head_orig_amount_ = head_->second.amount;
    }
    require(!!head_, error_code::iterator_overflow);
    return *head_;
  }

  /// Pop front order
  void pop() {
    require(!!head_, error_code::iterator_overflow);
    q_.all_amount_ -= head_orig_amount_;
    q_.orders_.pop();
    head_.reset();
  }

  /// Destructor to store the remaining head order back to queue
  ~orders_queue_cached() {
    if (head_) {
      [[maybe_unused]] auto [idx, ord] = *head_;
      q_.all_amount_ -= (head_orig_amount_ - ord.amount);
      q_.orders_.change_front(ord);
    }
  }
  orders_queue& q_;
  opt<OrderInfoXchgWithIdx> head_;
  uint128 head_orig_amount_;
};

}} // namespace tvm::xchg

