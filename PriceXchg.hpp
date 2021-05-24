#pragma once

#include <tvm/schema/message.hpp>

#include <tvm/replay_attack_protection/timestamp.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/small_dict_map.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/big_queue.hpp>
#include <tvm/string.hpp>
#include <tvm/numeric_limits.hpp>

#include "Flex.hpp"
#include "PriceCommon.hpp"
#include "TONTokenWallet.hpp"

namespace tvm { namespace schema {

using ITONTokenWalletPtr = handle<ITONTokenWallet>;

struct RationalPrice {
  uint128 numerator() const { return num; }
  uint128 denominator() const { return denum; }
  uint128 num;
  uint128 denum;
};
using price_t = RationalPrice;

struct PayloadArgs {
  bool_t sell;
  uint128 amount;
  addr_std_fixed receive_tip3_wallet;
  addr_std_fixed client_addr;
};

struct OrderInfoXchg {
  uint128 original_amount;
  uint128 amount;
  uint128 account; // native funds from client to pay for processing
  addr_std_fixed tip3_wallet_provide;
  addr_std_fixed tip3_wallet_receive;
  addr_std_fixed client_addr;
  uint32 order_finish_time;
};
using OrderInfoXchgWithIdx = std::pair<unsigned, OrderInfoXchg>;

struct DetailsInfoXchg {
  uint128 price_num;
  uint128 price_denum;
  uint128 min_amount;
  uint128 sell_amount;
  uint128 buy_amount;
};

__interface IPriceXchg {

  [[internal, noaccept, answer_id]]
  OrderRet onTip3LendOwnership(
    uint128 balance, uint32 lend_finish_time, uint256 pubkey, uint256 internal_owner,
    cell payload, address answer_addr) = 201;

  [[internal, noaccept]]
  void processQueue() = 203;

  // will cancel all orders with this sender's receive_wallet
  [[internal, noaccept]]
  void cancelSell() = 204;

  // will cancel all orders with this sender's answer_addr
  [[internal, noaccept]]
  void cancelBuy() = 205;
};
using IPriceXchgPtr = handle<IPriceXchg>;

struct DPriceXchg {
  price_t price_;
  uint128 sells_amount_;
  uint128 buys_amount_;
  addr_std_fixed flex_;
  uint128 min_amount_;
  uint8 deals_limit_; // limit for processed deals in one request

  IFLeXNotifyPtr notify_addr_;

  int8 workchain_id_;

  TonsConfig tons_cfg_;
  cell tip3_code_;
  Tip3Config major_tip3cfg_;
  Tip3Config minor_tip3cfg_;

  big_queue<OrderInfoXchg> sells_;
  big_queue<OrderInfoXchg> buys_;
};

__interface EPriceXchg {
};

__always_inline
std::pair<StateInit, uint256> prepare_price_xchg_state_init_and_addr(DPriceXchg price_data, cell price_code) {
  cell price_data_cl = prepare_persistent_data<IPriceXchg, void, DPriceXchg>({}, price_data);
  StateInit price_init {
    /*split_depth*/{}, /*special*/{},
    price_code, price_data_cl, /*library*/{}
  };
  cell price_init_cl = build(price_init).make_cell();
  return { price_init, uint256(tvm_hash(price_init_cl)) };
}

}} // namespace tvm::schema

