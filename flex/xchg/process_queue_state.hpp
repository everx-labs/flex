/** \file
 *  \brief Class for keeping iteration state for PriceXchg.
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#pragma once

#include "../PriceXchg.hpp"

#include "error_code.hpp"
#include <tvm/suffixes.hpp>

namespace tvm { namespace xchg {

/// Processing orders queue state for PriceXchg
class process_queue_state {
public:
  static constexpr unsigned msgs_per_deal = 4;               ///< Messages per deal completed
  static constexpr unsigned msgs_per_expired = 1;            ///< Messages per order expired
  static constexpr unsigned msgs_per_out_of_evers = 2;       ///< Messages per order out-of-evers
  static constexpr unsigned msgs_per_no_post_order_done = 2; ///< Messages per non-post-order partially done

  __always_inline
  process_queue_state(
    price_t        price,          ///< Price (rational value)
    address        pair,           ///< Address of XchgPair
    Tip3Config     major_tip3cfg,  ///< Major tip3 configuration
    Tip3Config     minor_tip3cfg,  ///< Minor tip3 configuration
    EversConfig    ev_cfg,         ///< Processing costs configuration
    uint128        min_amount,     ///< Minimum amount of major tokens for a deal or an order
    unsigned       deals_limit,    ///< Deals limit
    unsigned       msgs_limit,     ///< Messages limit
    IFlexNotifyPtr notify_addr,    ///< Notification address for AMM (IFlexNotify)
    unsigned       sell_idx,       ///< If we are processing onTip3LendOwnership with sell,
                                   ///<  this index we can use for return value
    unsigned       buy_idx         ///< If we are processing onTip3LendOwnership with buy,
                                   ///<  this index we can use for return value
  ) : price_(price), pair_(pair), major_tip3cfg_(major_tip3cfg), minor_tip3cfg_(minor_tip3cfg),
      ev_cfg_(ev_cfg), min_amount_(min_amount), deals_limit_(deals_limit), msgs_limit_(msgs_limit),
      tip3root_major_(major_tip3cfg.root_address), tip3root_minor_(minor_tip3cfg.root_address), notify_addr_(notify_addr),
      sell_idx_(sell_idx), buy_idx_(buy_idx) {}

  /// When order is expired, sending IPriceCallback::onOrderFinished() notification with the remaining balance.
  /// We don't need to call returnOwnership.
  __always_inline
  void on_expired(OrderInfoXchgWithIdx ord_idx, bool sell) {
    auto ord = ord_idx.second;
    ++expired_;
    // For the first expired order we also registering one message for amm notification
    if (sell) {
      msgs_outs_ += (sell_cancels_amount_ ? msgs_per_expired : msgs_per_expired + 1);
      sell_cancels_amount_ += ord.amount;
    } else {
      msgs_outs_ += (buy_cancels_amount_ ? msgs_per_expired : msgs_per_expired + 1);
      buy_cancels_amount_ += ord.amount;
    }
    OrderRet ret { uint32(ec::expired), ord.original_amount - ord.amount, 0u128, price_.num, price_.denum,
                   ord.user_id, ord.order_id };
    IPriceCallbackPtr(ord.client_addr)(Evers(ord.account.get())).
      onOrderFinished(ret, sell);
  }

  /// When order has not enough evers to process deals,
  ///  sending IPriceCallback::onOrderFinished() notification and
  ///  calling ITONTokenWallet::returnOwnership() to unlock tip3 tokens in the client wallet.
  __always_inline
  void on_out_of_evers(OrderInfoXchgWithIdx ord_idx, bool sell) {
    auto ord = ord_idx.second;
    ++out_of_evers_;
    // For the first expired order we also registering one message for amm notification
    if (sell) {
      msgs_outs_ += (sell_cancels_amount_ ? msgs_per_out_of_evers : msgs_per_out_of_evers + 1);
      sell_cancels_amount_ += ord.amount;
    } else {
      msgs_outs_ += (buy_cancels_amount_ ? msgs_per_out_of_evers : msgs_per_out_of_evers + 1);
      buy_cancels_amount_ += ord.amount;
    }

    OrderRet ret { uint32(ec::out_of_tons), ord.original_amount - ord.amount, 0u128, price_.num, price_.denum,
                   ord.user_id, ord.order_id };
    check_ret(sell, ord_idx.first, ret);
    if (ord.account > ev_cfg_.return_ownership) {
      ord.account -= ev_cfg_.return_ownership;
      ITONTokenWalletPtr(ord.tip3_wallet_provide)(Evers(ev_cfg_.return_ownership.get())).
        returnOwnership(ord.lend_amount);
      IPriceCallbackPtr(ord.client_addr)(Evers(ord.account.get())).
        onOrderFinished(ret, sell);
    }
  }

  /// When order is done
  __always_inline
  void on_order_done(OrderInfoXchgWithIdx ord_idx, bool sell) {
    auto ord = ord_idx.second;
    OrderRet ret { uint32(ok), ord.original_amount - ord.amount, 0u128, price_.num, price_.denum,
                   ord.user_id, ord.order_id };
    check_ret(sell, ord_idx.first, ret);
    IPriceCallbackPtr(ord.client_addr)(Evers(ord.account.get())).
      onOrderFinished(ret, sell);
  }

  /// When we have order without post-order flag and other side queue is empty
  __always_inline
  void on_no_post_order_done(OrderInfoXchgWithIdx ord_idx, bool sell) {
    auto ord = ord_idx.second;
    ++no_post_order_dones_;
    // For the first partially done no-post order we also registering one message for amm notification
    if (sell) {
      msgs_outs_ += (sell_cancels_amount_ ? msgs_per_no_post_order_done : msgs_per_no_post_order_done + 1);
      sell_cancels_amount_ += ord.amount;
    } else {
      msgs_outs_ += (buy_cancels_amount_ ? msgs_per_no_post_order_done : msgs_per_no_post_order_done + 1);
      buy_cancels_amount_ += ord.amount;
    }

    OrderRet ret { uint32(ec::no_post_order_partially_done), ord.original_amount - ord.amount, 0u128, price_.num, price_.denum,
                   ord.user_id, ord.order_id };
    check_ret(sell, ord_idx.first, ret);
    if (ord.account > ev_cfg_.return_ownership) {
      ord.account -= ev_cfg_.return_ownership;
      ITONTokenWalletPtr(ord.tip3_wallet_provide)(Evers(ev_cfg_.return_ownership.get())).
        returnOwnership(ord.lend_amount);
      IPriceCallbackPtr(ord.client_addr)(Evers(ord.account.get())).
        onOrderFinished(ret, sell);
    }
  }

  /// Check that order is the current requested order and we need to save return value
  __always_inline
  void check_ret(bool sell, unsigned idx, OrderRet ret) {
    if (sell && sell_idx_ == idx)
      ret_ = ret;
    if (!sell && buy_idx_ == idx)
      ret_ = ret;
  }

  /// When a deal is processed
  __always_inline
  void on_deal(bool seller_taker, uint128 deal_amount) {
    ++deals_processed_;
    sum_deals_amount_ += deal_amount;
    if (seller_taker)
      sum_taker_sells_amount_ += deal_amount;
    else
      sum_taker_buys_amount_ += deal_amount;
  }

  /// Is the order done? Means the remaining amount is less than min_amount or minor_cost can't be calculated
  __always_inline
  bool is_order_done(OrderInfoXchg ord) const {
    return (ord.amount == 0) || (ord.amount < min_amount_) || !minor_cost(ord.amount, price_);
  }

  /// If we hit deals or messages limit
  __always_inline
  bool overlimit() const {
    return deals_processed_ >= deals_limit_ || msgs_outs_ >= msgs_limit_;
  }

  /// Finalize state - send AMM notifications about processed deals and canceled orders
  __always_inline
  void finalize(IFlexNotifyPtr notify_addr, uint128 rest_sell_amount, uint128 rest_buy_amount) {
    if (sum_deals_amount_) {
      bool seller_taker = sum_taker_sells_amount_ > sum_taker_buys_amount_;
      notify_addr_(Evers(ev_cfg_.send_notify.get())).
        onXchgDealCompleted(seller_taker, pair_,
                            tip3root_major_, tip3root_minor_,
                            major_tip3cfg_, minor_tip3cfg_,
                            price_.numerator(), price_.denominator(), sum_deals_amount_);
    }
    if (sell_cancels_amount_) {
      notify_addr_(Evers(ev_cfg_.send_notify.get())).
        onXchgOrderCanceled(true, tip3root_major_, tip3root_minor_,
                            price_.numerator(), price_.denominator(), sell_cancels_amount_, rest_sell_amount);
    }
    if (buy_cancels_amount_) {
      notify_addr_(Evers(ev_cfg_.send_notify.get())).
        onXchgOrderCanceled(false, tip3root_major_, tip3root_minor_,
                            price_.numerator(), price_.denominator(), buy_cancels_amount_, rest_buy_amount);
    }
  }

  price_t     price_;                  ///< Price (rational value)
  address     pair_;                   ///< Address of XchgPair contract
  Tip3Config  major_tip3cfg_;          ///< Major tip3 configuration
  Tip3Config  minor_tip3cfg_;          ///< Minor tip3 configuration
  EversConfig ev_cfg_;                 ///< Evers processing costs configuration
  uint128     expired_;                ///< Expired orders met during current processing
  uint128     out_of_evers_;           ///< Out-of-evers orders met during current processing
  uint128     no_post_order_dones_;    ///< Partially done orders without post-order flag
  uint128     deals_processed_;        ///< Deals processed
  uint128     msgs_outs_;              ///< Out messages to sent
  uint128     sum_deals_amount_;       ///< Summarized amount of major tokens in all deals
  uint128     sum_taker_sells_amount_; ///< Summarized amount of major tokens in all taker-sell deals
  uint128     sum_taker_buys_amount_;  ///< Summarized amount of major tokens in all taker-buy deals
  uint128     sell_cancels_amount_;    ///< Canceled sell orders (ooe/expired)
  uint128     buy_cancels_amount_;     ///< Canceled buy orders (ooe/expired)
  uint128     min_amount_;             ///< Minimum amount of major tokens for a deal or an order
  unsigned    deals_limit_;            ///< Deals limit
  unsigned    msgs_limit_;             ///< Messages limit
  address     tip3root_major_;         ///< Address of RootTokenContract for major tip3 token
  address     tip3root_minor_;         ///< Address of RootTokenContract for minor tip3 token
  IFlexNotifyPtr notify_addr_;         ///< Notification address for AMM (IFlexNotify).
  unsigned    sell_idx_;               ///< If we are processing onTip3LendOwnership with sell, this index we can use for return value
  unsigned    buy_idx_;                ///< If we are processing onTip3LendOwnership with buy, this index we can use for return value
  opt<OrderRet> ret_;                  ///< Return value
};

}} // namespace tvm::xchg

