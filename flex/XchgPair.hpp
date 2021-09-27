#pragma once

#include <tvm/schema/message.hpp>

#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>

namespace tvm { inline namespace schema {

__interface IXchgPair {

  [[internal, noaccept, answer_id]]
  bool_t onDeploy(uint128 min_amount, uint128 deploy_value, address notify_addr) = 10;

  // ========== getters ==========
  [[getter]]
  address getFlexAddr() = 11;

  // address of major tip3 token root
  [[getter]]
  address getTip3MajorRoot() = 12;

  // address of minor tip3 token root
  [[getter]]
  address getTip3MinorRoot() = 13;

  [[getter]]
  uint128 getMinAmount() = 14;

  [[getter]]
  address getNotifyAddr() = 15;
};
using IXchgPairPtr = handle<IXchgPair>;

struct DXchgPair {
  address flex_addr_;
  address tip3_major_root_;
  address tip3_minor_root_;
  uint128 min_amount_; // minimum amount to buy/sell
  address notify_addr_; // address for deals notifications
};

__interface EXchgPair {
};

// Prepare Exchange Pair StateInit structure and expected contract address (hash from StateInit)
inline
std::pair<StateInit, uint256> prepare_xchg_pair_state_init_and_addr(DXchgPair pair_data, cell pair_code) {
  cell pair_data_cl = prepare_persistent_data<IXchgPair, void, DXchgPair>({}, pair_data);
  StateInit pair_init {
    /*split_depth*/{}, /*special*/{},
    pair_code, pair_data_cl, /*library*/{}
  };
  cell pair_init_cl = build(pair_init).make_cell();
  return { pair_init, uint256(tvm_hash(pair_init_cl)) };
}

}} // namespace tvm::schema

