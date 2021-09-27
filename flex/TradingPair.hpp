#pragma once

#include <tvm/schema/message.hpp>

#include <tvm/replay_attack_protection/timestamp.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>

namespace tvm { inline namespace schema {

__interface ITradingPair {

  [[internal, noaccept, answer_id]]
  bool_t onDeploy(uint128 min_amount, uint128 deploy_value, address notify_addr) = 10;

  // ========== getters ==========
  [[getter]]
  address getFlexAddr() = 11;

  [[getter]]
  address getTip3Root() = 12;

  [[getter]]
  uint128 getMinAmount() = 13;

  [[getter]]
  address getNotifyAddr() = 14;
};
using ITradingPairPtr = handle<ITradingPair>;

struct DTradingPair {
  address flex_addr_;
  address tip3_root_;
  uint128 min_amount_; // minimum amount to buy/sell
  address notify_addr_; // address for deals notifications
};

__interface ETradingPair {
};

// Prepare Trading Pair StateInit structure and expected contract address (hash from StateInit)
inline
std::pair<StateInit, uint256> prepare_trading_pair_state_init_and_addr(DTradingPair pair_data, cell pair_code) {
  cell pair_data_cl = prepare_persistent_data<ITradingPair, void, DTradingPair>({}, pair_data);
  StateInit pair_init {
    /*split_depth*/{}, /*special*/{},
    pair_code, pair_data_cl, /*library*/{}
  };
  cell pair_init_cl = build(pair_init).make_cell();
  return { pair_init, uint256(tvm_hash(pair_init_cl)) };
}

}} // namespace tvm::schema

