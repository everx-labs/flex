/** \file
 *  \brief PriceXchg contract implementation.
 *  Contract for trading price for tip3/tip3 exchange.
 *  First tip3 in a pair is major and terms "sell", "buy", "amount" are related to the first tip3 in pair.
 *  Second tip3 is called "minor".
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#include "PriceXchg.hpp"
#include <tvm/contract.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/default_support_functions.hpp>
#include <tvm/schema/parse_chain_static.hpp>

#include "xchg/dealer.hpp"
#include "xchg/orders_queue.hpp"

using namespace tvm;
using namespace xchg;

#ifndef TIP3_WALLET_CODE_HASH
#error "Macros TIP3_WALLET_CODE_HASH must be defined (code hash of FlexWallet)"
#endif

#ifndef TIP3_WALLET_CODE_DEPTH
#error "Macros TIP3_WALLET_CODE_DEPTH must be defined (code depth of FlexWallet)"
#endif

static constexpr unsigned c_msgs_limit = 200; ///< Messages limit in process_queue for one transaction

__attribute__((noinline))
auto process_queue_impl(price_t price, address pair, Tip3Config major_tip3cfg, Tip3Config minor_tip3cfg, EversConfig ev_cfg,
                        uint128 sells_amount, big_queue<OrderInfoXchg> sells,
                        uint128 buys_amount, big_queue<OrderInfoXchg> buys,
                        uint128 min_amount, uint8 deals_limit, uint8 msgs_limit,
                        IFlexNotifyPtr notify_addr,
                        address major_reserve_wallet, address minor_reserve_wallet,
                        unsigned sell_idx, unsigned buy_idx
                        ) {
  dealer d(price, pair, major_tip3cfg, minor_tip3cfg, ev_cfg, orders_queue{sells_amount, sells}, orders_queue{buys_amount, buys},
           min_amount, deals_limit.get(), msgs_limit.get(),
           notify_addr, major_reserve_wallet, minor_reserve_wallet);
  return d.process(sell_idx, buy_idx);
}

__attribute__((noinline))
std::pair<big_queue<OrderInfoXchg>, uint128> cancel_order_impl(
    big_queue<OrderInfoXchg> orders, addr_std_fixed client_addr, uint128 all_amount, bool sell,
    Evers return_ownership, Evers process_queue, Evers incoming_val, price_t price
) {
  bool is_first = true;
  for (auto it = orders.begin(); it != orders.end();) {
    auto next_it = std::next(it);
    auto ord = *it;
    if (ord.client_addr == client_addr) {
      unsigned minus_val = process_queue.get();
      ITONTokenWalletPtr(ord.tip3_wallet_provide)(return_ownership).
        returnOwnership(ord.lend_amount);
      minus_val += return_ownership.get();

      unsigned plus_val = ord.account.get() + (is_first ? incoming_val.get() : 0);
      is_first = false;
      if (plus_val > minus_val) {
        unsigned ret_val = plus_val - minus_val;
        OrderRet ret { uint32(ec::canceled), ord.original_amount - ord.amount, 0u128, price.num, price.denum,
                       ord.user_id, ord.order_id };
        IPriceCallbackPtr(ord.client_addr)(Evers(ret_val)).
          onOrderFinished(ret, sell);
      }

      all_amount -= ord.amount;

      orders.erase(it);
    }
    it = next_it;
  }
  return { orders, all_amount };
}

/// Implements IPriceXchg
/// May be in 3 states:
/// 1. Only sell orders
/// 2. Only buy orders
/// 3. Have both sides. Temporary state, may occur when process_queue hit deals limit.
///    In the next processQueue() call (calls) to itself will be converted into state #1 or state #2.
class PriceXchg final : public smart_interface<IPriceXchg>, public DPriceXchg {
  using data = DPriceXchg;
public:
  __always_inline
  OrderRet onTip3LendOwnership(
    uint128     balance,
    uint32      lend_finish_time,
    uint256     pubkey,
    address_opt owner,
    cell        payload,
    address     answer_addr
  ) {
    auto cfg = getConfig();
    auto [tip3_wallet, value] = int_sender_and_value();
    ITONTokenWalletPtr wallet_in(tip3_wallet);
    Evers ret_owner_gr(cfg.ev_cfg.return_ownership.get());

    // to send answer to the original caller (caller->tip3wallet->price->caller)
    set_int_sender(answer_addr);
    set_int_return_value(cfg.ev_cfg.order_answer.get());

    auto min_value = onTip3LendOwnershipMinValue();

    auto args = parse_chain_static<FlexLendPayloadArgs>(parser(payload.ctos()));
    bool is_sell = args.sell;
    auto amount = args.amount;
    auto minor_amount = calc_lend_tokens_for_order(is_sell, amount, price_);

    unsigned err = 0;
    if (value.get() < min_value)
      err = ec::not_enough_tons_to_process;
    else if (is_sell ? !verify_tip3_addr(cfg.major_tip3cfg, tip3_wallet, pubkey, owner) :
                       !verify_tip3_addr(cfg.minor_tip3cfg, tip3_wallet, pubkey, owner))
      err = ec::unverified_tip3_wallet;
    else if (amount < cfg.min_amount)
      err = ec::not_enough_tokens_amount;
    else if (balance < (is_sell ? amount : minor_amount))
      err = ec::too_big_tokens_amount;
    else if (!is_active_time(lend_finish_time))
      err = ec::expired;
    // If an order doesn't have "immediate client" bit, and the Price has *other side* positive balance, then the order will be *failed*.
    else if (!args.immediate_client && (is_sell ? buys_amount_ != 0 : sells_amount_ != 0))
      err = ec::have_other_side_with_non_immediate_client;
    else if (!args.post_order && (is_sell ? sells_amount_ != 0 : buys_amount_ != 0))
      err = ec::have_this_side_with_non_post_order;
    if (err)
      return on_ord_fail(err, wallet_in, balance, args.user_id, args.order_id);

    uint128 account = uint128(value.get()) - cfg.ev_cfg.process_queue - cfg.ev_cfg.order_answer;

    OrderInfoXchg ord {
      args.immediate_client, args.post_order, amount, amount, account, balance, tip3_wallet,
      args.receive_tip3_wallet, args.client_addr, lend_finish_time, args.user_id, args.order_id,
      uint64{__builtin_tvm_ltime()}
      };
    unsigned sell_idx = 0;
    unsigned buy_idx = 0;
    uint128 notify_amount;
    if (is_sell) {
      sells_.push(ord);
      sells_amount_ += ord.amount;
      sell_idx = sells_.back_with_idx().first;
      notify_amount = sells_amount_;
    } else {
      buys_.push(ord);
      buys_amount_ += ord.amount;
      buy_idx = buys_.back_with_idx().first;
      notify_amount = buys_amount_;
    }

    IFlexNotifyPtr(cfg.notify_addr)(Evers(cfg.ev_cfg.send_notify.get())).
      onXchgOrderAdded(is_sell, cfg.major_tip3cfg.root_address, cfg.minor_tip3cfg.root_address,
                       price_.numerator(), price_.denominator(), ord.amount, notify_amount);

    auto [sells, buys, ret] =
      process_queue_impl(price_, cfg.pair, cfg.major_tip3cfg, cfg.minor_tip3cfg, cfg.ev_cfg,
                         sells_amount_, sells_, buys_amount_, buys_,
                         cfg.min_amount, cfg.deals_limit, uint8(c_msgs_limit),
                         cfg.notify_addr, cfg.major_reserve_wallet, cfg.minor_reserve_wallet,
                         sell_idx, buy_idx
                         );
    sells_ = sells.orders_;
    buys_ = buys.orders_;
    sells_amount_ = sells.all_amount_;
    buys_amount_ = buys.all_amount_;

    if (sells_.empty() && buys_.empty())
      suicide(cfg.flex);
    if (ret) return *ret;
    return { uint32(ok), 0u128, ord.amount, price_.num, price_.denum, ord.user_id, ord.order_id };
  }

  __always_inline
  void processQueue() {
    if (sells_.empty() || buys_.empty())
      return;

    auto cfg = getConfig();
    auto [sells, buys, ret] =
      process_queue_impl(price_, cfg.pair, cfg.major_tip3cfg, cfg.minor_tip3cfg, cfg.ev_cfg,
                         sells_amount_, sells_, buys_amount_, buys_,
                         cfg.min_amount, cfg.deals_limit, uint8(c_msgs_limit),
                         cfg.notify_addr, cfg.major_reserve_wallet, cfg.minor_reserve_wallet,
                         0, 0
                         );
    sells_ = sells.orders_;
    buys_ = buys.orders_;
    sells_amount_ = sells.all_amount_;
    buys_amount_ = buys.all_amount_;
    if (sells_.empty() && buys_.empty())
      suicide(cfg.flex);
  }

  __always_inline
  void cancelSell() {
    auto cfg = getConfig();
    auto canceled_amount = sells_amount_;
    addr_std_fixed client_addr = int_sender();
    auto value = int_value();
    auto [sells, sells_amount] =
      cancel_order_impl(sells_, client_addr, sells_amount_, true,
                        Evers(cfg.ev_cfg.return_ownership.get()),
                        Evers(cfg.ev_cfg.process_queue.get()), value, price_);
    sells_ = sells;
    sells_amount_ = sells_amount;
    canceled_amount -= sells_amount_;

    IFlexNotifyPtr(cfg.notify_addr)(Evers(cfg.ev_cfg.send_notify.get())).
      onXchgOrderCanceled(true, cfg.major_tip3cfg.root_address, cfg.minor_tip3cfg.root_address,
                          price_.numerator(), price_.denominator(), canceled_amount, sells_amount_);

    if (sells_.empty() && buys_.empty())
      suicide(cfg.flex);
  }

  __always_inline
  void cancelBuy() {
    auto cfg = getConfig();
    auto canceled_amount = buys_amount_;
    addr_std_fixed client_addr = int_sender();
    auto value = int_value();
    auto [buys, buys_amount] =
      cancel_order_impl(buys_, client_addr, buys_amount_, false,
                        Evers(cfg.ev_cfg.return_ownership.get()),
                        Evers(cfg.ev_cfg.process_queue.get()), value, price_);
    buys_ = buys;
    buys_amount_ = buys_amount;
    canceled_amount -= buys_amount_;

    IFlexNotifyPtr(cfg.notify_addr)(Evers(cfg.ev_cfg.send_notify.get())).
      onXchgOrderCanceled(false, cfg.major_tip3cfg.root_address, cfg.minor_tip3cfg.root_address,
                          price_.numerator(), price_.denominator(), canceled_amount, buys_amount_);

    if (sells_.empty() && buys_.empty())
      suicide(cfg.flex);
  }

  // ========== getters ==========

  __always_inline
  EversConfig ev_cfg() {
    return getConfig().ev_cfg;
  }

  __always_inline
  dict_array<OrderInfoXchg> getSells() {
    return dict_array<OrderInfoXchg>(sells_.begin(), sells_.end());
  }

  __always_inline
  dict_array<OrderInfoXchg> getBuys() {
    return dict_array<OrderInfoXchg>(buys_.begin(), buys_.end());
  }

  __always_inline
  PriceXchgSalt getConfig() {
    return parse_chain_static<PriceXchgSalt>(parser(tvm_code_salt()));
  }

  // default processing of unknown messages
  __always_inline static int _fallback(cell /*msg*/, slice /*msg_body*/) {
    return 0;
  }
  // =============== Support functions ==================
  DEFAULT_SUPPORT_FUNCTIONS(IPriceXchg, void)
private:
  __always_inline
  uint128 onTip3LendOwnershipMinValue() {
    // we need funds for processing:
    // * execute this function
    // * execute transfer from seller's tip3 wallet to buyer tip3 wallet
    // * execute transfer from buyer's tip3 wallet to seller tip3 wallet
    // * execute transfer from taker's tip3 wallet to reserve tip3 wallet
    // * execute returnOwnership of tip3 wallet to return ownership to the original owner
    // * send answer message
    auto cfg = getConfig();
    return cfg.ev_cfg.process_queue + 3 * cfg.ev_cfg.transfer_tip3 + cfg.ev_cfg.send_notify +
      cfg.ev_cfg.return_ownership + cfg.ev_cfg.order_answer;
  }

  __always_inline
  bool verify_tip3_addr(
    Tip3Config cfg,
    address tip3_wallet,
    uint256 wallet_pubkey,
    std::optional<address> wallet_internal_owner
  ) {
    auto expected_address = expected_wallet_address(cfg, wallet_pubkey, wallet_internal_owner);
    return std::get<addr_std>(tip3_wallet()).address == expected_address;
  }

  /// Optimized expected tip3 address calculation using code hash instead of full code cell.
  /// Macroses should be defined: TIP3_WALLET_CODE_HASH, TIP3_WALLET_CODE_DEPTH
  __always_inline
  uint256 expected_wallet_address(
    Tip3Config  cfg,
    uint256     wallet_pubkey,
    address_opt wallet_owner
  ) {
    return calc_int_wallet_init_hash(
      cfg.name, cfg.symbol, cfg.decimals, cfg.root_pubkey, cfg.root_address,
      wallet_pubkey, wallet_owner,
      uint256(TIP3_WALLET_CODE_HASH), uint16(TIP3_WALLET_CODE_DEPTH), getConfig().workchain_id
      );
  }

  __always_inline
  OrderRet on_ord_fail(unsigned ec, ITONTokenWalletPtr wallet_in, uint128 lend_amount,
                       uint256 user_id, uint256 order_id) {
    wallet_in(Evers(ev_cfg().return_ownership.get())).returnOwnership(lend_amount);
    if (sells_.empty() && buys_.empty()) {
      set_int_return_flag(SEND_ALL_GAS | DELETE_ME_IF_I_AM_EMPTY);
    } else {
      auto incoming_value = int_value().get();
      tvm_rawreserve(tvm_balance() - incoming_value, rawreserve_flag::up_to);
      set_int_return_flag(SEND_ALL_GAS);
    }
    return { uint32(ec), {}, {}, price_.num, price_.denum, user_id, order_id };
  }
};

DEFINE_JSON_ABI(IPriceXchg, DPriceXchg, EPriceXchg);

// ----------------------------- Main entry functions ---------------------- //
MAIN_ENTRY_FUNCTIONS_NO_REPLAY(PriceXchg, IPriceXchg, DPriceXchg)

