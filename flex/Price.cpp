#include "Price.hpp"
#include <tvm/contract.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/default_support_functions.hpp>

using namespace tvm;
using namespace schema;

static constexpr unsigned ok = 0;
struct ec : tvm::error_code {
  static constexpr unsigned out_of_tons                    = 100; // partially processed because out of tons
  static constexpr unsigned deals_limit                    = 101; // partially processed because deals limit
  static constexpr unsigned not_enough_tons_to_process     = 102;
  static constexpr unsigned not_enough_tokens_amount       = 103;
  static constexpr unsigned too_big_tokens_amount          = 104;
  static constexpr unsigned different_workchain_id         = 105;
  static constexpr unsigned unverified_tip3_wallet         = 106;
  static constexpr unsigned canceled                       = 107;
  static constexpr unsigned expired                        = 108;
};

// For tip3 we have lend_finish_time, when ownership will return back to its original owner.
// And we need some safe period to not process orders with soon expiring tip3 ownership
static constexpr unsigned safe_delay_period = 5 * 60;

__always_inline
bool is_active_time(uint32 order_finish_time) {
  return tvm_now() + safe_delay_period < order_finish_time.get();
}

__always_inline
std::optional<uint128> calc_cost(uint128 amount, uint128 price) {
  unsigned tons_cost = amount.get() * price.get();
  if (tons_cost >> 128)
    return {};
  return uint128(tons_cost);
}

class dealer {
public:
  // returns (sell_out_of_tons, buy_out_of_tons, deal_amount)
  __always_inline
  std::tuple<bool, bool, uint128> make_deal(OrderInfo& sell, OrderInfo& buy) {
    auto deal_amount = std::min(sell.amount, buy.amount);
    bool_t last_tip3_sell{sell.amount == deal_amount};
    sell.amount -= deal_amount;
    buy.amount -= deal_amount;
    auto cost = calc_cost(deal_amount, price_);

    // Smaller pays for tip3 transfer
    //  (if seller provides big sell order, he will not pay for each small transfer)
    uint128 sell_costs{0};
    uint128 buy_costs = *cost;
    if (last_tip3_sell)
      sell_costs += (tons_cfg_.transfer_tip3 + tons_cfg_.send_notify);
    else
      buy_costs += (tons_cfg_.transfer_tip3 + tons_cfg_.send_notify);

    bool sell_out_of_tons = (sell.account < sell_costs);
    bool buy_out_of_tons = (buy.account < buy_costs);
    if (sell_out_of_tons || buy_out_of_tons)
      return {sell_out_of_tons, buy_out_of_tons, uint128(0)};
    sell.account -= sell_costs;
    buy.account -= buy_costs;

    ITONTokenWalletPtr(sell.tip3_wallet)(Grams(tons_cfg_.transfer_tip3.get())).
      transfer(sell.tip3_wallet, buy.tip3_wallet, deal_amount, uint128(0), bool_t{false});
    tvm_transfer(sell.client_addr, cost->get(), /*bounce*/true, SENDER_WANTS_TO_PAY_FEES_SEPARATELY);

    notify_addr_(Grams(tons_cfg_.send_notify.get())).
      onDealCompleted(tip3root_, price_, deal_amount);

    return {false, false, deal_amount};
  }

  __attribute__((noinline))
  static std::tuple<std::optional<OrderInfoWithIdx>, queue<OrderInfo>, uint128>
  extract_active_order(std::optional<OrderInfoWithIdx> cur_order,
                       queue<OrderInfo> orders, uint128 all_amount, bool_t sell) {
    if (cur_order)
      return {cur_order, orders, all_amount};

    while (!orders.empty()) {
      cur_order = orders.front_with_idx_opt();
      auto ord = cur_order->second;
      if (!is_active_time(ord.order_finish_time)) {
        all_amount -= ord.amount;
        OrderRet ret { uint32(ec::expired), ord.original_amount - ord.amount, uint128{0} };
        IPriceCallbackPtr(ord.client_addr)(Grams(ord.account.get())).
          onOrderFinished(ret, sell);
        orders.pop();
        cur_order.reset();
        continue;
      }
      break;
    }
    return {cur_order, orders, all_amount};
  }

  __always_inline
  void process_queue(unsigned sell_idx, unsigned buy_idx) {
    std::optional<OrderInfoWithIdx> sell_opt;
    std::optional<OrderInfoWithIdx> buy_opt;
    unsigned deals_count = 0;
    while (true) {
      std::tie(sell_opt, sells_, sells_amount_) =
        extract_active_order(sell_opt, sells_, sells_amount_, bool_t{true});
      std::tie(buy_opt, buys_, buys_amount_) =
        extract_active_order(buy_opt, buys_, buys_amount_, bool_t{false});
      if (!sell_opt || !buy_opt)
        break;

      auto [sell_idx_cur, sell] = *sell_opt;
      auto [buy_idx_cur, buy] = *buy_opt;

      bool sell_out_of_tons = false;
      bool buy_out_of_tons = false;
      uint128 deal_amount{0};
      // ==== if we hit deals limit ====
      if (++deals_count > deals_limit_) {
        auto half_process_queue = tons_cfg_.process_queue / 2;
        auto safe_extra = tons_cfg_.return_ownership + tons_cfg_.order_answer;
        if (sell.account < half_process_queue + safe_extra) {
          sell_out_of_tons = true;
        }
        if (buy.account < half_process_queue + safe_extra) {
          buy_out_of_tons = true;
        }
        if (!sell_out_of_tons && !buy_out_of_tons) {
          sell.account -= half_process_queue;
          buy.account -= half_process_queue;
          IPricePtr(address{tvm_myaddr()})(Grams(tons_cfg_.process_queue.get())).
            processQueue();
          if (sell_idx == sell_idx_cur)
            ret_ = { uint32(ec::deals_limit), sell.original_amount - sell.amount, sell.amount };
          if (buy_idx == buy_idx_cur)
            ret_ = { uint32(ec::deals_limit), buy.original_amount - buy.amount, buy.amount };
          break;
        }
      }

      // ==== make deal ====
      if (!sell_out_of_tons && !buy_out_of_tons) {
        std::tie(sell_out_of_tons, buy_out_of_tons, deal_amount) = make_deal(sell, buy);
      }

      // ==== if one of sides is out of tons ====
      if (sell_out_of_tons) {
        sells_.pop();
        OrderRet ret { uint32(ec::out_of_tons), sell.original_amount - sell.amount, uint128{0} };
        if (sell_idx == sell_idx_cur)
          ret_ = ret;
        if (sell.account > tons_cfg_.return_ownership) {
          sell.account -= tons_cfg_.return_ownership;
          ITONTokenWalletPtr(sell.tip3_wallet)(Grams(tons_cfg_.return_ownership.get())).
            returnOwnership(sell.amount);
          IPriceCallbackPtr(sell.client_addr)(Grams(sell.account.get())).
            onOrderFinished(ret, bool_t{true});
        }
        sell_opt.reset();
      }
      if (buy_out_of_tons) {
        buys_.pop();
        OrderRet ret { uint32(ec::out_of_tons), buy.original_amount - buy.amount, uint128{0} };
        if (buy_idx == buy_idx_cur)
          ret_ = ret;
        IPriceCallbackPtr(buy.client_addr)(Grams(buy.account.get())).
          onOrderFinished(ret, bool_t{false});
        buy_opt.reset();
      }
      if (sell_out_of_tons || buy_out_of_tons)
        continue;

      sell_opt->second = sell;
      buy_opt->second = buy;

      sells_amount_ -= deal_amount;
      buys_amount_ -= deal_amount;
      // ==== if one of sides is out of tokens amount ====
      if (!sell.amount) {
        sells_.pop();
        OrderRet ret { uint32(ok), sell.original_amount, uint128{0} };
        if (sell_idx == sell_idx_cur)
          ret_ = ret;
        IPriceCallbackPtr(sell.client_addr)(Grams(sell.account.get())).
          onOrderFinished(ret, bool_t{true});
        sell_opt.reset();
      }
      if (!buy.amount) {
        buys_.pop();
        OrderRet ret { uint32(ok), buy.original_amount, uint128{0} };
        if (buy_idx == buy_idx_cur)
          ret_ = ret;
        IPriceCallbackPtr(buy.client_addr)(Grams(buy.account.get())).
          onOrderFinished(ret, bool_t{false});
        buy_opt.reset();
      }
    }
    if (sell_opt && sell_opt->second.amount) {
      // If the last sell order is not fully processed, modify its amount
      auto sell = sell_opt->second;
      sells_.change_front(sell);
      if (sell_idx == sell_opt->first)
        ret_ = OrderRet { uint32(ok), sell.original_amount - sell.amount, sell.amount };
    }
    if (buy_opt && buy_opt->second.amount) {
      // If the last buy order is not fully processed, modify its amount
      auto buy = buy_opt->second;
      buys_.change_front(buy);
      if (buy_idx == buy_opt->first)
        ret_ = OrderRet { uint32(ok), buy.original_amount - buy.amount, buy.amount };
    }
  }

  address tip3root_;
  IFlexNotifyPtr notify_addr_;
  uint128 price_;
  unsigned deals_limit_;
  TonsConfig tons_cfg_;
  uint128 sells_amount_;
  queue<OrderInfo> sells_;
  uint128 buys_amount_;
  queue<OrderInfo> buys_;
  std::optional<OrderRet> ret_;
};

struct process_ret {
  uint128 sells_amount;
  queue<OrderInfo> sells_;
  uint128 buys_amount;
  queue<OrderInfo> buys_;
  std::optional<OrderRet> ret_;
};

__attribute__((noinline))
process_ret process_queue_impl(address tip3root, IFlexNotifyPtr notify_addr,
                               uint128 price, uint8 deals_limit, TonsConfig tons_cfg,
                               unsigned sell_idx, unsigned buy_idx,
                               uint128 sells_amount, queue<OrderInfo> sells,
                               uint128 buys_amount, queue<OrderInfo> buys) {
  dealer d{tip3root, notify_addr, price, deals_limit.get(), tons_cfg,
           sells_amount, sells, buys_amount, buys, {}};
  d.process_queue(sell_idx, buy_idx);
  return { d.sells_amount_, d.sells_, d.buys_amount_, d.buys_, d.ret_ };
}

__attribute__((noinline))
std::pair<queue<OrderInfo>, uint128> cancel_order_impl(
    queue<OrderInfo> orders, addr_std_fixed client_addr, uint128 all_amount, bool_t sell,
    Grams return_ownership, Grams process_queue, Grams incoming_val) {
  bool is_first = true;
  for (auto it = orders.begin(); it != orders.end();) {
    auto next_it = std::next(it);
    auto ord = *it;
    if (ord.client_addr == client_addr) {
      unsigned minus_val = is_first ? process_queue.get() : 0;
      if (sell) {
        ITONTokenWalletPtr(ord.tip3_wallet)(return_ownership).
          returnOwnership(ord.amount);
        minus_val += return_ownership.get();
      }
      unsigned plus_val = ord.account.get() + (is_first ? incoming_val.get() : 0);
      is_first = false;
      if (plus_val > minus_val) {
        unsigned ret_val = plus_val - minus_val;
        OrderRet ret { uint32(ec::canceled), ord.original_amount - ord.amount, uint128{0} };
        IPriceCallbackPtr(ord.client_addr)(Grams(ret_val)).
          onOrderFinished(ret, sell);
      }

      all_amount -= ord.amount;

      orders.erase(it);
    }
    it = next_it;
  }
  return { orders, all_amount };
}

class Price final : public smart_interface<IPrice>, public DPrice {
public:
  __always_inline
  OrderRet onTip3LendOwnership(
    address answer_addr,
    uint128 balance,
    uint32  lend_finish_time,
    uint256 pubkey,
    address internal_owner,
    cell    payload
  ) {
    auto [tip3_wallet, value] = int_sender_and_value();
    ITONTokenWalletPtr wallet_in(tip3_wallet);
    Grams ret_owner_gr(tons_cfg_.return_ownership.get());

    // to send answer to the original caller (caller->tip3wallet->price->caller)
    set_int_sender(answer_addr);
    set_int_return_value(tons_cfg_.order_answer.get());

    auto min_value = onTip3LendOwnershipMinValue();

    auto args = parse<SellArgs>(payload.ctos());
    auto amount = args.amount;
    unsigned err = 0;
    if (value.get() < min_value)
      err = ec::not_enough_tons_to_process;
    else if (!verify_tip3_addr(tip3_wallet, pubkey, std::get<addr_std>(internal_owner.val()).address))
      err = ec::unverified_tip3_wallet;
    else if (amount < min_amount_)
      err = ec::not_enough_tokens_amount;
    else if (balance < amount)
      err = ec::too_big_tokens_amount;
    else if (!calc_cost(amount, price_))
      err = ec::too_big_tokens_amount;
    else if (!is_active_time(lend_finish_time))
      err = ec::expired;

    if (err)
      return on_sell_fail(err, wallet_in, amount);

    uint128 account = uint128(value.get()) - tons_cfg_.process_queue - tons_cfg_.order_answer;

    OrderInfo sell{amount, amount, account, tip3_wallet, args.receive_wallet, lend_finish_time};
    sells_.push(sell);
    sells_amount_ += sell.amount;

    notify_addr_(Grams(tons_cfg_.send_notify.get())).
      onOrderAdded(bool_t{true}, tip3cfg_.root_address, price_, sell.amount, sells_amount_);

    auto [sells_amount, sells, buys_amount, buys, ret] =
      process_queue_impl(tip3cfg_.root_address, notify_addr_, price_, deals_limit_, tons_cfg_,
                         sells_.back_with_idx().first, 0,
                         sells_amount_, sells_, buys_amount_, buys_);
    sells_ = sells;
    buys_ = buys;
    sells_amount_ = sells_amount;
    buys_amount_ = buys_amount;

    if (sells_.empty() && buys_.empty())
      suicide(flex_);
    if (ret) return *ret;
    return { uint32(ok), uint128(0), sell.amount };
  }

  __always_inline
  OrderRet buyTip3(uint128 amount, address receive_tip3_wallet, uint32 order_finish_time) {
    auto [sender, value_gr] = int_sender_and_value();
    require(amount >= min_amount_, ec::not_enough_tokens_amount);
    auto cost = calc_cost(amount, price_);
    require(!!cost, ec::too_big_tokens_amount);
    require(value_gr.get() > buyTip3MinValue(*cost),
            ec::not_enough_tons_to_process);
    require(is_active_time(order_finish_time), ec::expired);

    set_int_return_value(tons_cfg_.order_answer.get());

    uint128 account = uint128(value_gr.get()) - tons_cfg_.process_queue - tons_cfg_.order_answer;

    OrderInfo buy{amount, amount, account, receive_tip3_wallet, sender, order_finish_time};
    buys_.push(buy);
    buys_amount_ += buy.amount;

    notify_addr_(Grams(tons_cfg_.send_notify.get())).
      onOrderAdded(bool_t{false}, tip3cfg_.root_address, price_, buy.amount, buys_amount_);

    auto [sells_amount, sells, buys_amount, buys, ret] =
      process_queue_impl(tip3cfg_.root_address, notify_addr_, price_, deals_limit_, tons_cfg_,
                         0, buys_.back_with_idx().first,
                         sells_amount_, sells_, buys_amount_, buys_);
    sells_ = sells;
    buys_ = buys;
    sells_amount_ = sells_amount;
    buys_amount_ = buys_amount;
    if (sells_.empty() && buys_.empty())
      suicide(flex_);
    if (ret) return *ret;
    return { uint32(ok), uint128(0), buy.amount };
  }

  __always_inline
  void processQueue() {
    if (sells_.empty() || buys_.empty())
      return;
    auto [sells_amount, sells, buys_amount, buys, ret] =
      process_queue_impl(tip3cfg_.root_address, notify_addr_, price_, deals_limit_, tons_cfg_, 0, 0,
                         sells_amount_, sells_, buys_amount_, buys_);
    sells_ = sells;
    buys_ = buys;
    sells_amount_ = sells_amount;
    buys_amount_ = buys_amount;
    if (sells_.empty() && buys_.empty())
      suicide(flex_);
  }

  __always_inline
  void cancelSell() {
    auto canceled_amount = sells_amount_;
    addr_std_fixed client_addr = int_sender();
    auto value = int_value();
    auto [sells, sells_amount] =
      cancel_order_impl(sells_, client_addr, sells_amount_, bool_t{true},
                        Grams(tons_cfg_.return_ownership.get()),
                        Grams(tons_cfg_.process_queue.get()), value);
    sells_ = sells;
    sells_amount_ = sells_amount;
    canceled_amount -= sells_amount;

    notify_addr_(Grams(tons_cfg_.send_notify.get())).
      onOrderCanceled(bool_t{true}, tip3cfg_.root_address, price_, canceled_amount, sells_amount_);

    if (sells_.empty() && buys_.empty())
      suicide(flex_);
  }

  __always_inline
  void cancelBuy() {
    auto canceled_amount = buys_amount_;
    addr_std_fixed client_addr = int_sender();
    auto value = int_value();
    auto [buys, buys_amount] =
      cancel_order_impl(buys_, client_addr, buys_amount_, bool_t{false},
                        Grams(tons_cfg_.return_ownership.get()),
                        Grams(tons_cfg_.process_queue.get()), value);
    buys_ = buys;
    buys_amount_ = buys_amount;
    canceled_amount -= buys_amount;

    notify_addr_(Grams(tons_cfg_.send_notify.get())).
      onOrderCanceled(bool_t{false}, tip3cfg_.root_address, price_, canceled_amount, buys_amount_);

    if (sells_.empty() && buys_.empty())
      suicide(flex_);
  }

  // ========== getters ==========
  __always_inline
  DetailsInfo getDetails() {
    return { getPrice(), getMinimumAmount(), getSellAmount(), getBuyAmount() };
  }

  __always_inline
  uint128 getPrice() {
    return price_;
  }

  __always_inline
  uint128 getMinimumAmount() {
    return min_amount_;
  }

  __always_inline
  TonsConfig getTonsCfg() {
    return tons_cfg_;
  }

  __always_inline
  dict_array<OrderInfo> getSells() {
    return dict_array<OrderInfo>(sells_.begin(), sells_.end());
  }

  __always_inline
  dict_array<OrderInfo> getBuys() {
    return dict_array<OrderInfo>(buys_.begin(), buys_.end());
  }

  __always_inline
  uint128 getSellAmount() {
    return sells_amount_;
  }

  __always_inline
  uint128 getBuyAmount() {
    return buys_amount_;
  }

  // default processing of unknown messages
  __always_inline static int _fallback(cell /*msg*/, slice /*msg_body*/) {
    return 0;
  }
  // =============== Support functions ==================
  DEFAULT_SUPPORT_FUNCTIONS(IPrice, void)
private:
  __always_inline
  uint128 onTip3LendOwnershipMinValue() const {
    // we need funds for processing:
    // * execute this function
    // * execute transfer from seller's tip3 wallet to buyer tip3 wallet
    // * execute returnOwnership of tip3 wallet to return ownership to the original owner
    // * send answer message
    return tons_cfg_.process_queue + tons_cfg_.transfer_tip3 + tons_cfg_.send_notify +
      tons_cfg_.return_ownership + tons_cfg_.order_answer;
  }
  __always_inline
  uint128 buyTip3MinValue(uint128 buy_cost) const {
    return buy_cost + tons_cfg_.process_queue + tons_cfg_.transfer_tip3 + tons_cfg_.send_notify +
      tons_cfg_.order_answer;
  }
  __always_inline
  bool verify_tip3_addr(address tip3_wallet, uint256 wallet_pubkey, uint256 internal_owner) {
    auto expected_address = expected_wallet_address(wallet_pubkey, internal_owner);
    return std::get<addr_std>(tip3_wallet()).address == expected_address;
  }
  __always_inline
  uint256 expected_wallet_address(uint256 wallet_pubkey, uint256 internal_owner) {
    std::optional<address> owner_addr;
    if (internal_owner)
      owner_addr = address::make_std(workchain_id_, internal_owner);

    return prepare_internal_wallet_state_init_and_addr(
      tip3cfg_.name, tip3cfg_.symbol, tip3cfg_.decimals,
      tip3cfg_.root_public_key, wallet_pubkey, tip3cfg_.root_address,
      owner_addr, tip3_code_, workchain_id_
      ).second;
  }

  __always_inline
  OrderRet on_sell_fail(unsigned ec, ITONTokenWalletPtr wallet_in, uint128 amount) {
    wallet_in(Grams(tons_cfg_.return_ownership.get())).returnOwnership(amount);

    if (sells_.empty() && buys_.empty()) {
      set_int_return_flag(SEND_ALL_GAS | DELETE_ME_IF_I_AM_EMPTY);
    } else {
      auto incoming_value = int_value().get();
      tvm_rawreserve(tvm_balance() - incoming_value, rawreserve_flag::up_to);
      set_int_return_flag(SEND_ALL_GAS);
    }
    return { uint32(ec), {}, {} };
  }
};

DEFINE_JSON_ABI(IPrice, DPrice, EPrice);

// ----------------------------- Main entry functions ---------------------- //
MAIN_ENTRY_FUNCTIONS_NO_REPLAY(Price, IPrice, DPrice)

