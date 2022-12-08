/** \file
 *  \brief PriceXchg contract implementation.
 *  Contract for trading price for tip3/tip3 exchange.
 *  First tip3 in a pair is major and terms "sell", "buy", "amount" are related to the first tip3 in pair.
 *  Second tip3 is called "minor".
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) EverFlex Inc
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
    Evers return_ownership, Evers process_queue, Evers incoming_val, price_t price, opt<uint256> user_id, opt<uint256> order_id,
    address pair, uint8 major_decimals, uint8 minor_decimals
) {
  bool is_first = true;
  for (auto it = orders.begin(); it != orders.end();) {
    auto next_it = std::next(it);
    auto ord = *it;
    if ((ord.client_addr == client_addr) && (!user_id || (*user_id == ord.user_id)) && (!order_id || (*order_id == ord.order_id))) {
      unsigned minus_val = is_first ? process_queue.get() : 0;
      ITONTokenWalletPtr(ord.tip3_wallet_provide)(return_ownership).
        returnOwnership(ord.lend_amount);
      minus_val += return_ownership.get();

      unsigned plus_val = ord.account.get() + (is_first ? incoming_val.get() : 0);
      is_first = false;
      if (plus_val > minus_val) {
        unsigned ret_val = plus_val - minus_val;
        OrderRet ret { uint32(ec::canceled), ord.original_amount - ord.amount, 0u128, price.num, price.denum,
                       ord.user_id, ord.order_id, pair, major_decimals, minor_decimals, sell };
        IPriceCallbackPtr(ord.tip3_wallet_provide)(Evers(ret_val)).
          onOrderFinished(ret);
      }

      all_amount -= ord.amount;

      orders.erase(it);
    }
    it = next_it;
  }
  return { orders, all_amount };
}

/// Is it a correct price: price.num % minmove == 0
__always_inline
bool is_correct_price(price_t price, uint128 minmove) {
  return 0 == (price.num % minmove);
}

/// Implements IPriceXchg
/// May be in 3 states:
/// 1. Only sell orders
/// 2. Only buy orders
/// 3. Have both sides. Temporary state, may occur when process_queue hit deals limit.
///    In the next processQueue() call (calls) to itself will be converted into state #1 or state #2.
class PriceXchg final : public smart_interface<IPriceXchg>, public DPriceXchg {
  using data = DPriceXchg;
  static constexpr bool _checked_deploy = true; /// Deploy is only allowed with [[deploy]] function call
public:
  OrderRet onTip3LendOwnership(
    uint128     balance,
    uint32      lend_finish_time,
    Tip3Creds   creds,
    cell        payload,
    address     answer_addr
  ) {
    auto cfg = getConfig();
    price_t price { price_num_, cfg.price_denum };
    require(is_correct_price(price, cfg.minmove), ec::incorrect_price);
    require(lend_finish_time > safe_delay_period, ec::expired);
    lend_finish_time -= safe_delay_period;
    auto [tip3_wallet, value] = int_sender_and_value();
    ITONTokenWalletPtr wallet_in(tip3_wallet);
    Evers ret_owner_gr(cfg.ev_cfg.return_ownership.get());

    auto [pubkey, owner] = creds;

    // to send answer to the original caller (caller->tip3wallet->price->caller)
    set_int_sender(answer_addr);
    set_int_return_value(cfg.ev_cfg.order_answer.get());

    auto min_value = onTip3LendOwnershipMinValue();

    auto args = parse_chain_static<FlexLendPayloadArgs>(parser(payload.ctos()));
    bool is_sell = args.sell;
    auto amount = args.amount;

    auto minor_amount = calc_lend_tokens_for_order(is_sell, amount, price);

    unsigned err = 0;
    if (value.get() < min_value)
      err = ec::not_enough_tons_to_process;
    else if (is_sell ? !verify_tip3_addr(cfg.major_tip3cfg, cfg, tip3_wallet, pubkey, owner) :
                       !verify_tip3_addr(cfg.minor_tip3cfg, cfg, tip3_wallet, pubkey, owner))
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
      return on_ord_fail(is_sell, cfg, err, wallet_in, balance, args.user_id, args.order_id, cfg.price_denum);

    uint128 account = uint128(value.get()) - cfg.ev_cfg.process_queue - cfg.ev_cfg.order_answer;

    OrderInfoXchg ord {
      args.immediate_client, args.post_order, amount, amount, account, balance, tip3_wallet,
      args.client_addr, lend_finish_time, args.user_id, args.order_id,
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
                       price.numerator(), price.denominator(), ord.amount, notify_amount);

    auto [sells, buys, ret] =
      process_queue_impl(price, cfg.pair, cfg.major_tip3cfg, cfg.minor_tip3cfg, cfg.ev_cfg,
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
    return { uint32(ok), 0u128, ord.amount, price.num, price.denum, ord.user_id, ord.order_id,
             cfg.pair, cfg.major_tip3cfg.decimals, cfg.minor_tip3cfg.decimals, is_sell };
  }

  void processQueue() {
    if (sells_.empty() || buys_.empty())
      return;

    auto cfg = getConfig();
    auto [sells, buys, ret] =
      process_queue_impl({price_num_, cfg.price_denum}, cfg.pair, cfg.major_tip3cfg, cfg.minor_tip3cfg, cfg.ev_cfg,
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

  void cancelOrder(
    bool         sell,
    opt<uint256> user_id,
    opt<uint256> order_id
  ) {
    auto cfg = getConfig();
    auto [client_addr, value] = int_sender_and_value();
    uint128 canceled_amount;
    uint128 rest_amount;
    if (sell) {
      canceled_amount = sells_amount_;
      auto [sells, sells_amount] =
        cancel_order_impl(sells_, client_addr, sells_amount_, true,
                          Evers(cfg.ev_cfg.return_ownership.get()),
                          Evers(cfg.ev_cfg.process_queue.get()), value, {price_num_, cfg.price_denum}, user_id, order_id,
                          cfg.pair, cfg.major_tip3cfg.decimals, cfg.minor_tip3cfg.decimals);
      sells_ = sells;
      sells_amount_ = sells_amount;
      canceled_amount -= sells_amount_;
    } else {
      canceled_amount = buys_amount_;
      auto [buys, buys_amount] =
        cancel_order_impl(buys_, client_addr, buys_amount_, false,
                          Evers(cfg.ev_cfg.return_ownership.get()),
                          Evers(cfg.ev_cfg.process_queue.get()), value, {price_num_, cfg.price_denum}, user_id, order_id,
                          cfg.pair, cfg.major_tip3cfg.decimals, cfg.minor_tip3cfg.decimals);
      buys_ = buys;
      buys_amount_ = buys_amount;
      canceled_amount -= buys_amount_;
    }

    IFlexNotifyPtr(cfg.notify_addr)(Evers(cfg.ev_cfg.send_notify.get())).
      onXchgOrderCanceled(sell, cfg.major_tip3cfg.root_address, cfg.minor_tip3cfg.root_address,
                          price_num_, cfg.price_denum, canceled_amount, rest_amount);

    if (sells_.empty() && buys_.empty())
      suicide(cfg.flex);
  }

  void cancelWalletOrder(
    bool         sell,
    address      owner,
    uint256      user_id,
    opt<uint256> order_id
  ) {
    auto cfg = getConfig();
    auto [tip3_wallet, value] = int_sender_and_value();
    bool good_wallet = sell ? verify_tip3_addr(cfg.major_tip3cfg, cfg, tip3_wallet, user_id, owner):
                              verify_tip3_addr(cfg.minor_tip3cfg, cfg, tip3_wallet, user_id, owner);
    require(good_wallet, ec::unverified_tip3_wallet);

    uint128 canceled_amount;
    uint128 rest_amount;
    if (sell) {
      canceled_amount = sells_amount_;
      auto [sells, sells_amount] =
        cancel_order_impl(sells_, owner, sells_amount_, true,
                          Evers(cfg.ev_cfg.return_ownership.get()),
                          Evers(cfg.ev_cfg.process_queue.get()), value, {price_num_, cfg.price_denum}, user_id, order_id,
                          cfg.pair, cfg.major_tip3cfg.decimals, cfg.minor_tip3cfg.decimals);
      sells_ = sells;
      sells_amount_ = sells_amount;
      canceled_amount -= sells_amount_;
      rest_amount = sells_amount_;
    } else {
      canceled_amount = buys_amount_;
      auto [buys, buys_amount] =
        cancel_order_impl(buys_, owner, buys_amount_, false,
                          Evers(cfg.ev_cfg.return_ownership.get()),
                          Evers(cfg.ev_cfg.process_queue.get()), value, {price_num_, cfg.price_denum}, user_id, order_id,
                          cfg.pair, cfg.major_tip3cfg.decimals, cfg.minor_tip3cfg.decimals);
      buys_ = buys;
      buys_amount_ = buys_amount;
      canceled_amount -= buys_amount_;
      rest_amount = buys_amount_;
    }

    IFlexNotifyPtr(cfg.notify_addr)(Evers(cfg.ev_cfg.send_notify.get())).
      onXchgOrderCanceled(sell, cfg.major_tip3cfg.root_address, cfg.minor_tip3cfg.root_address,
                          price_num_, cfg.price_denum, canceled_amount, rest_amount);

    if (sells_.empty() && buys_.empty())
      suicide(cfg.flex);
  }

  // ========== getters ==========

  EversConfig ev_cfg() {
    return getConfig().ev_cfg;
  }

  dict_array<OrderInfoXchg> getSells() {
    return dict_array<OrderInfoXchg>(sells_.begin(), sells_.end());
  }

  dict_array<OrderInfoXchg> getBuys() {
    return dict_array<OrderInfoXchg>(buys_.begin(), buys_.end());
  }

  PriceXchgSalt getConfig() {
    return parse_chain_static<PriceXchgSalt>(parser(tvm_code_salt()));
  }

  PriceXchgDetails getDetails() {
    return { price_num_, getSells(), getBuys(), getConfig() };
  }

  // default processing of unknown messages
  static int _fallback([[maybe_unused]] cell msg, [[maybe_unused]] slice msg_body) {
    return 0;
  }
  // =============== Support functions ==================
  DEFAULT_SUPPORT_FUNCTIONS(IPriceXchg, void)
private:
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

  __attribute__((noinline))
  static bool verify_tip3_addr(
    Tip3Config    cfg,
    PriceXchgSalt salt,
    address       tip3_wallet,
    uint256       wallet_pubkey,
    opt<address>  wallet_internal_owner
  ) {
    auto expected_address = expected_wallet_address(cfg, salt, wallet_pubkey, wallet_internal_owner);
    return std::get<addr_std>(tip3_wallet()).address == expected_address;
  }

  /// Optimized expected tip3 address calculation using code hash instead of full code cell.
  /// Macroses should be defined: TIP3_WALLET_CODE_HASH, TIP3_WALLET_CODE_DEPTH
  __always_inline
  static uint256 expected_wallet_address(
    Tip3Config    cfg,
    PriceXchgSalt salt,
    uint256       wallet_pubkey,
    address_opt   wallet_owner
  ) {
    return calc_int_wallet_init_hash(
      cfg, wallet_pubkey, wallet_owner,
      uint256(TIP3_WALLET_CODE_HASH), uint16(TIP3_WALLET_CODE_DEPTH), salt.workchain_id
      );
  }

  OrderRet on_ord_fail(bool sell, PriceXchgSalt cfg, unsigned ec, ITONTokenWalletPtr wallet_in,
                       uint128 lend_amount, uint256 user_id, uint256 order_id, uint128 price_denum) {
    wallet_in(Evers(ev_cfg().return_ownership.get())).returnOwnership(lend_amount);
    if (sells_.empty() && buys_.empty()) {
      set_int_return_flag(SEND_ALL_GAS | DELETE_ME_IF_I_AM_EMPTY);
    } else {
      auto incoming_value = int_value().get();
      tvm_rawreserve(tvm_balance() - incoming_value, rawreserve_flag::up_to);
      set_int_return_flag(SEND_ALL_GAS);
    }
    return { uint32(ec), {}, {}, price_num_, price_denum, user_id, order_id, cfg.pair,
             cfg.major_tip3cfg.decimals, cfg.minor_tip3cfg.decimals, sell };
  }
};

DEFINE_JSON_ABI(IPriceXchg, DPriceXchg, EPriceXchg);

// ----------------------------- Main entry functions ---------------------- //
MAIN_ENTRY_FUNCTIONS_NO_REPLAY(PriceXchg, IPriceXchg, DPriceXchg)

