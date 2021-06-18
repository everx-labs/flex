#pragma once

#include <tvm/schema/message.hpp>

#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>

namespace tvm { namespace schema {

__interface IXchgPair {

  [[internal, noaccept, answer_id]]
  bool_t onDeploy();

  // ========== getters ==========
  [[getter]]
  address getFLeXAddr();

  // address of major tip3 token root
  [[getter]]
  address getTip3MajorRoot();

  // address of minor tip3 token root
  [[getter]]
  address getTip3MinorRoot();
};
using IXchgPairPtr = handle<IXchgPair>;

struct DXchgPair {
  address flex_addr_;
  address tip3_major_root_;
  address tip3_minor_root_;
  uint128 deploy_value_; // required minimum value to deploy TradingPair
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

