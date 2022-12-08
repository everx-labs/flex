/** \file
 *  \brief Class for iterating sell & buy queues and processing tip3/tip3 deals for PriceXchg.
 *
 *  dealer works with two xchg_iterator 's and common process_queue_state.
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) EverFlex Inc
 */

#pragma once

#include "../PriceXchg.hpp"
#include "../FlexTransferPayloadArgs.hpp"
#include "process_queue_state.hpp"
#include "xchg_iterator.hpp"

#include <tvm/suffixes.hpp>
#include <tvm/schema/build_chain_static.hpp>

namespace tvm { namespace xchg {

/// Class for iterating queue and processing deals
class dealer {
public:
  dealer(
    price_t        price,                ///< Price (rational value)
    address        pair,                 ///< Address of XchgPair contract
    Tip3Config     major_tip3cfg,        ///< Major tip3 configuration
    Tip3Config     minor_tip3cfg,        ///< Minor tip3 configuration
    EversConfig    ev_cfg,               ///< Processing costs configuration (evers)
    orders_queue   sells,                ///< Sell orders queue
    orders_queue   buys,                 ///< Buy orders queue
    uint128        min_amount,           ///< Minimum amount of major tokens for a deal or an order
    unsigned       deals_limit,          ///< Deals limit
    unsigned       msgs_limit,           ///< Messages limit
    IFlexNotifyPtr notify_addr,          ///< Notification address for AMM
    address        major_reserve_wallet, ///< Major reserve wallet
    address        minor_reserve_wallet  ///< Minor reserve wallet
  ) : price_(price), pair_(pair), major_tip3cfg_(major_tip3cfg), minor_tip3cfg_(minor_tip3cfg),
      ev_cfg_(ev_cfg), sells_(sells), buys_(buys),
      min_amount_(min_amount), deals_limit_(deals_limit), msgs_limit_(msgs_limit),
      deal_costs_(ev_cfg.transfer_tip3 * 3 + ev_cfg.send_notify),
      tip3root_major_(major_tip3cfg.root_address), tip3root_minor_(minor_tip3cfg.root_address), notify_addr_(notify_addr),
      major_reserve_wallet_(major_reserve_wallet), minor_reserve_wallet_(minor_reserve_wallet) {
  }

  /// Result of process() call
  struct process_result {
    orders_queue sells; ///< Sell orders queue
    orders_queue buys;  ///< Buy orders queue
    opt<OrderRet> ret;  ///< Return value for the called function
  };

  /// Process order queues and make deals
  process_result process(unsigned sell_idx, unsigned buy_idx) {
    process_queue_state state(price_, pair_, major_tip3cfg_, minor_tip3cfg_, ev_cfg_, min_amount_, deals_limit_, msgs_limit_,
                              notify_addr_, sell_idx, buy_idx);

    {
      orders_queue_cached sells(sells_);
      xchg_iterator sells_iter(state, sells, deal_costs_, true);
      orders_queue_cached buys(buys_);
      xchg_iterator buys_iter(state, buys, deal_costs_, false);

      while (sells_iter.first_active() && buys_iter.first_active() && !state.overlimit()) {
        auto res = make_deal(*sells_iter, *buys_iter);
        if (res.sell_out_of_evers)
          sells_iter.drop_with_ooc();
        if (res.buy_out_of_evers)
          buys_iter.drop_with_ooc();
        if (!res.sell_out_of_evers && !res.buy_out_of_evers) {
          sells_iter.on_deal(res.deal_amount, res.seller_costs, res.seller_lend_spent);
          buys_iter.on_deal(res.deal_amount, res.buyer_costs, res.buyer_lend_spent);
          state.on_deal(res.seller_taker, res.deal_amount);
        }
      }
      // In case of overlimit, we need to send IPriceXchg::processQueue() to self
      if (sells_iter.first_active() && buys_iter.first_active()) {
        IPriceXchgPtr(address{tvm_myaddr()})(Evers(ev_cfg_.process_queue.get())).
          processQueue();
      }
    }

    // We need to find orders without post_order flag and finish them
    if (!state.overlimit()) {
      const bool sells_empty = sells_.empty();
      const bool buys_empty = buys_.empty();
      if (sells_empty && !buys_empty) {
        buys_.drop_no_post_orders(state, false);
      } else if (!sells_empty && buys_empty) {
        sells_.drop_no_post_orders(state, true);
      }
    }
    state.finalize(sells_.all_amount_, buys_.all_amount_); // finalize state and send AMM notifications

    return {
      sells_,
      buys_,
      state.ret_
      };
  }

  /// Result of make_deal
  struct deal_result {
    bool seller_taker;         ///< Seller is a taker in deal
    bool sell_out_of_evers;    ///< Is the sell order out of evers
    bool buy_out_of_evers;     ///< Is the buy order out of evers
    uint128 deal_amount;       ///< Amount of major tokens for the deal (without fees)
    uint128 seller_costs;      ///< Seller evers costs to be taken
    uint128 buyer_costs;       ///< Buyer evers costs to be taken
    uint128 seller_lend_spent; ///< Seller lend tokens spent (major tokens for seller)
    uint128 buyer_lend_spent;  ///< Buyer lend tokens spent (minor tokens for buyer)
  };

  /// Make tip3/tip exchange deal
  deal_result make_deal(OrderInfoXchg sell, OrderInfoXchg buy) {
    auto deal_amount = std::min(sell.amount, buy.amount);

    bool last_tip3_sell = (sell.amount == deal_amount) || (sell.amount < deal_amount + min_amount_);
    bool last_tip3_buy = (buy.amount == deal_amount) || (buy.amount < deal_amount + min_amount_);

    auto buy_payment = minor_cost(deal_amount, price_);
    // it is unlikely here, because (amount * price) calculation is performed before for initial order
    // so just removing ending orders from queues with 'out_of_evers' reason
    if (!buy_payment)
      return {.seller_taker = false, last_tip3_sell, last_tip3_buy, 0u128, 0u128, 0u128, 0u128};

    auto major_deal_amount = deal_amount;
    auto minor_deal_amount = *buy_payment;

    auto sell_extra_return = last_tip3_sell ? (sell.lend_amount - major_deal_amount) : 0u128;
    auto buy_extra_return = last_tip3_buy ? (buy.lend_amount - minor_deal_amount) : 0u128;

    // Smaller pays for tip3 transfer
    //  (if seller provides big sell order, he will not pay for each small transfer)
    bool seller_pays_costs = last_tip3_sell;
    bool sell_out_of_evers = seller_pays_costs && (sell.account < deal_costs_);
    bool buy_out_of_evers = !seller_pays_costs && (buy.account < deal_costs_);
    if (sell_out_of_evers || buy_out_of_evers)
      return {.seller_taker = false, sell_out_of_evers, buy_out_of_evers, 0u128, 0u128, 0u128, 0u128};

    uint128 seller_costs = seller_pays_costs ? deal_costs_ : 0u128;
    uint128 buyer_costs = !seller_pays_costs ? deal_costs_ : 0u128;

    uint128 seller_lend_spent;
    uint128 buyer_lend_spent;

    // (seller_taker & buyer_maker) || (seller_maker & buyer_taker)
    // We have values:
    // * major_deal_amount
    // * minor_deal_amount
    // * if (seller_taker) {
    //     taker_fee_val = major_deal_amount * taker_fee;
    //     maker_vig_val = major_deal_amount * maker_vig;
    //     reserve_val = taker_fee_val - maker_vig_val;
    //   }
    // * if (buyer_taker) {
    //     taker_fee_val = minor_deal_amount * taker_fee;
    //     maker_vig_val = minor_deal_amount * maker_vig;
    //     reserve_val = taker_fee_val - maker_vig_val;
    //   }
    // *
    // In `seller_taker` scheme, we need to perform token transfers:
    // * transfer of seller.(major_deal_amount + maker_vig_val) tokens
    //    to buyer major token wallet (buyer.tip3_creds_receive).
    // * transfer of buyer.minor_deal_amount tokens to seller minor token wallet (seller.tip3_creds_receive).
    // * transfer of seller.reserve_val to major reserve wallet.
    // In `buyer_taker` scheme, we need to perform token transfers:
    // * transfer of buyer.(minor_deal_amount + maker_vig_val) tokens
    //    to seller minor token wallet (seller.tip3_creds_receive).
    // * transfer of seller.major_deal_amount tokens to buyer major token wallet (buyer.tip3_creds_receive).
    // * transfer of buyer.reserve_val to minor reserve wallet.

    bool seller_taker = (sell.ltime > buy.ltime);

    if (seller_taker) {
      uint128 taker_fee_val = mul(major_deal_amount, taker_fee);
      uint128 maker_vig_val = mul(major_deal_amount, maker_vig);
      uint128 reserve_val = taker_fee_val - maker_vig_val;
      seller_lend_spent = major_deal_amount + taker_fee_val;
      buyer_lend_spent = minor_deal_amount;

      FlexTransferPayloadArgs seller_payload {
        .sender_sell = true,
        .sender_taker = true,
        .sender_user_id = sell.user_id,
        .receiver_user_id = buy.user_id,
        .receiver_order_id = buy.order_id,
        .another_tip3_root = tip3root_minor_,
        .price_num = price_.numerator(),
        .price_denum = price_.denominator(),
        .taker_fee = taker_fee_val,
        .maker_vig = maker_vig_val,
        .pair = pair_,
        .major_tip3cfg = major_tip3cfg_,
        .minor_tip3cfg = minor_tip3cfg_
      };
      // Transfer of major tokens from seller to buyer
      ITONTokenWalletPtr(sell.tip3_wallet_provide)(Evers(ev_cfg_.transfer_tip3.get())).
        transferToRecipient({}, { buy.user_id, buy.client_addr }, major_deal_amount + maker_vig_val,
                            0u128, ev_cfg_.dest_wallet_keep_evers, true, 0u128,
                            build_chain_static(seller_payload));

      FlexTransferPayloadArgs buyer_payload {
        .sender_sell = false,
        .sender_taker = false,
        .sender_user_id = buy.user_id,
        .receiver_user_id = sell.user_id,
        .receiver_order_id = sell.order_id,
        .another_tip3_root = tip3root_major_,
        .price_num = price_.numerator(),
        .price_denum = price_.denominator(),
        .taker_fee = taker_fee_val,
        .maker_vig = maker_vig_val,
        .pair = pair_,
        .major_tip3cfg = major_tip3cfg_,
        .minor_tip3cfg = minor_tip3cfg_
      };
      // Transfer of minor tokens from buyer to seller
      ITONTokenWalletPtr(buy.tip3_wallet_provide)(Evers(ev_cfg_.transfer_tip3.get())).
        transferToRecipient({}, { sell.user_id, sell.client_addr }, minor_deal_amount,
                            0u128, ev_cfg_.dest_wallet_keep_evers, true, buy_extra_return,
                            build_chain_static(buyer_payload));
      // Transfer of major tokens from seller to major reserve wallet
      if (reserve_val > 0) {
        ITONTokenWalletPtr(sell.tip3_wallet_provide)(Evers(ev_cfg_.transfer_tip3.get())).
          transfer({}, major_reserve_wallet_, reserve_val, 0u128, 0u128,
                   build_chain_static(seller_payload));
      }
    } else {
      uint128 taker_fee_val = mul(minor_deal_amount, taker_fee);
      uint128 maker_vig_val = mul(minor_deal_amount, maker_vig);
      uint128 reserve_val = taker_fee_val - maker_vig_val;
      seller_lend_spent = major_deal_amount;
      buyer_lend_spent = minor_deal_amount + taker_fee_val;

      FlexTransferPayloadArgs buyer_payload {
        .sender_sell = false,
        .sender_taker = true,
        .sender_user_id = buy.user_id,
        .receiver_user_id = sell.user_id,
        .receiver_order_id = sell.order_id,
        .another_tip3_root = tip3root_major_,
        .price_num = price_.numerator(),
        .price_denum = price_.denominator(),
        .taker_fee = taker_fee_val,
        .maker_vig = maker_vig_val,
        .pair = pair_,
        .major_tip3cfg = major_tip3cfg_,
        .minor_tip3cfg = minor_tip3cfg_
      };
      // transfer of minor tokens from buyer to seller
      ITONTokenWalletPtr(buy.tip3_wallet_provide)(Evers(ev_cfg_.transfer_tip3.get())).
        transferToRecipient({}, { sell.user_id, sell.client_addr }, minor_deal_amount + maker_vig_val,
                            0u128, ev_cfg_.dest_wallet_keep_evers, true, 0u128,
                            build_chain_static(buyer_payload));

      FlexTransferPayloadArgs seller_payload {
        .sender_sell = true,
        .sender_taker = false,
        .sender_user_id = sell.user_id,
        .receiver_user_id = buy.user_id,
        .receiver_order_id = buy.order_id,
        .another_tip3_root = tip3root_minor_,
        .price_num = price_.numerator(),
        .price_denum = price_.denominator(),
        .taker_fee = taker_fee_val,
        .maker_vig = maker_vig_val,
        .pair = pair_,
        .major_tip3cfg = major_tip3cfg_,
        .minor_tip3cfg = minor_tip3cfg_
      };
      // transfer of major tokens from seller to buyer
      ITONTokenWalletPtr(sell.tip3_wallet_provide)(Evers(ev_cfg_.transfer_tip3.get())).
        transferToRecipient({}, { buy.user_id, buy.client_addr }, major_deal_amount,
                            0u128, ev_cfg_.dest_wallet_keep_evers, true, sell_extra_return,
                            build_chain_static(seller_payload));
      // Transfer of minor tokens from buyer to minor reserve wallet
      if (reserve_val > 0) {
        ITONTokenWalletPtr(buy.tip3_wallet_provide)(Evers(ev_cfg_.transfer_tip3.get())).
          transfer({}, minor_reserve_wallet_, reserve_val, 0u128, 0u128,
                   build_chain_static(buyer_payload));
      }
    }
    return {
      .seller_taker = seller_taker, false, false, deal_amount,
      seller_costs, buyer_costs,
      seller_lend_spent, buyer_lend_spent
    };
  }

  price_t        price_;                ///< Price (rational value)
  address        pair_;                 ///< Address of XchgPair contract
  Tip3Config     major_tip3cfg_;        ///< Major tip3 configuration
  Tip3Config     minor_tip3cfg_;        ///< Minor tip3 configuration
  EversConfig    ev_cfg_;               ///< Processing costs configuration (evers)
  orders_queue   sells_;                ///< Sell orders queue
  orders_queue   buys_;                 ///< Buy orders queue
  uint128        min_amount_;           ///< Minimum amount of major tokens for a deal or an order
  unsigned       deals_limit_;          ///< Deals limit
  unsigned       msgs_limit_;           ///< Messages limit
  uint128        deal_costs_;           ///< Deal costs in evers
  address        tip3root_major_;       ///< Address of RootTokenContract for major tip3 token
  address        tip3root_minor_;       ///< Address of RootTokenContract for minor tip3 token
  IFlexNotifyPtr notify_addr_;          ///< Notification address for AMM
  address        major_reserve_wallet_; ///< Major reserve wallet
  address        minor_reserve_wallet_; ///< Minor reserve wallet
};

}} // namespace tvm::xchg

