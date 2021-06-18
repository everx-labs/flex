#pragma once

#include <tvm/schema/message.hpp>

#include <tvm/replay_attack_protection/timestamp.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/small_dict_map.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/queue.hpp>
#include <tvm/string.hpp>

#include "FLeX.hpp"
#include "TONTokenWallet.hpp"

namespace tvm { namespace schema {

using ITONTokenWalletPtr = handle<ITONTokenWallet>;

struct Tip3Config {
  string name;
  string symbol;
  uint8 decimals;
  uint256 root_public_key;
  address root_address;
  cell code;
};

struct OrderRet {
  uint32 err_code;
  uint128 processed;
  uint128 enqueued;
};

struct SellArgs {
  uint128 amount;
  addr_std_fixed receive_wallet;
};

struct OrderInfo {
  uint128 original_amount;
  uint128 amount;
  uint128 account; // native funds from client to pay for processing
  addr_std_fixed tip3_wallet;
  addr_std_fixed client_addr;
  uint32 order_finish_time;
};
using OrderInfoWithIdx = std::pair<unsigned, OrderInfo>;

struct DetailsInfo {
  uint128 price;
  uint128 min_amount;
  uint128 sell_amount;
  uint128 buy_amount;
};

__interface IPriceCallback {
  [[internal, noaccept]]
  void onOrderFinished(OrderRet ret, bool_t sell);
};
using IPriceCallbackPtr = handle<IPriceCallback>;

// usage from debot:
// co_await tip3.tail_call<OrderRet>(price_addr, gr).lendOwnership(...)

__interface IPrice {

  [[internal, noaccept, answer_id]]
  OrderRet onTip3LendOwnership(
    uint128 balance, uint32 lend_finish_time, uint256 pubkey, uint256 internal_owner,
    cell payload, address answer_addr) = 201;

  [[internal, noaccept, answer_id]]
  OrderRet buyTip3(uint128 amount, address receive_tip3, uint32 order_finish_time) = 202;

  [[internal, noaccept]]
  void processQueue() = 203;

  // will cancel all orders with this sender's receive_wallet
  [[internal, noaccept]]
  void cancelSell() = 204;

  // will cancel all orders with this sender's answer_addr
  [[internal, noaccept]]
  void cancelBuy() = 205;
};
using IPricePtr = handle<IPrice>;

struct DPrice {
  uint128 price_;
  uint128 sells_amount_;
  uint128 buys_amount_;
  addr_std_fixed flex_;
  uint128 min_amount_;
  uint8 deals_limit_; // limit for processed deals in one request

  IFLeXNotifyPtr notify_addr_;

  int8 workchain_id_;

  TonsConfig tons_cfg_;
  Tip3Config tip3cfg_;

  queue<OrderInfo> sells_;
  queue<OrderInfo> buys_;
};

__interface EPrice {
};

__always_inline
std::pair<StateInit, uint256> prepare_price_state_init_and_addr(DPrice price_data, cell price_code) {
  cell price_data_cl = prepare_persistent_data<IPrice, void, DPrice>({}, price_data);
  StateInit price_init {
    /*split_depth*/{}, /*special*/{},
    price_code, price_data_cl, /*library*/{}
  };
  cell price_init_cl = build(price_init).make_cell();
  return { price_init, uint256(tvm_hash(price_init_cl)) };
}

}} // namespace tvm::schema

