#pragma once

#include <tvm/schema/message.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/replay_attack_protection/timestamp.hpp>

namespace tvm { inline namespace schema {

static constexpr unsigned FLEX_TIMESTAMP_DELAY = 1800;
using flex_replay_protection_t = replay_attack_protection::timestamp<FLEX_TIMESTAMP_DELAY>;

// Processing native funds value ...
struct TonsConfig {
  // ... for executing tip3 transfer
  uint128 transfer_tip3;
  // ... to return tip3 ownership
  uint128 return_ownership;
  // ... to deploy tip3 trading pair
  uint128 trading_pair_deploy;
  // .. to send answer message from Price
  uint128 order_answer;
  // ... to process processQueue function
  //  (also is used for buyTip3/onTip3LendOwnership/cancelSell/cancelBuy estimations)
  uint128 process_queue;
  // ... to send notification about completed deal (IFlexNotify)
  uint128 send_notify;
};

__interface IFlexNotify {
  [[internal, noaccept]]
  void onDealCompleted(address tip3root, uint128 price, uint128 amount) = 10;
  [[internal, noaccept]]
  void onXchgDealCompleted(address tip3root_sell, address tip3root_buy,
                           uint128 price_num, uint128 price_denum, uint128 amount) = 11;
  [[internal, noaccept]]
  void onOrderAdded(bool_t sell, address tip3root, uint128 price, uint128 amount, uint128 sum_amount) = 12;
  [[internal, noaccept]]
  void onOrderCanceled(bool_t sell, address tip3root, uint128 price, uint128 amount, uint128 sum_amount) = 13;
  [[internal, noaccept]]
  void onXchgOrderAdded(bool_t sell, address tip3root_major, address tip3root_minor,
                        uint128 price_num, uint128 price_denum, uint128 amount, uint128 sum_amount) = 14;
  [[internal, noaccept]]
  void onXchgOrderCanceled(bool_t sell, address tip3root_major, address tip3root_minor,
                           uint128 price_num, uint128 price_denum, uint128 amount, uint128 sum_amount) = 15;
};
using IFlexNotifyPtr = handle<IFlexNotify>;

__interface IFlex {

  [[external, dyn_chain_parse]]
  void constructor(uint256 deployer_pubkey,
    uint128 transfer_tip3, uint128 return_ownership, uint128 trading_pair_deploy,
    uint128 order_answer, uint128 process_queue, uint128 send_notify,
    uint8 deals_limit, address notify_addr) = 10;

  // To fit message size limit, setPairCode/setPriceCode in separate functions
  //  (not in constructor)
  [[external, noaccept]]
  void setPairCode(cell code) = 11;

  [[external, noaccept]]
  void setXchgPairCode(cell code) = 12;

  [[external, noaccept]]
  void setPriceCode(cell code) = 13;

  [[external, noaccept]]
  void setXchgPriceCode(cell code) = 14;

  // ========== getters ==========

  // means setPairCode/setXchgPairCode/setPriceCode/setXchgPriceCode executed
  [[getter]]
  bool_t isFullyInitialized() = 15;

  [[getter]]
  TonsConfig getTonsCfg() = 16;

  [[getter]]
  cell getTradingPairCode() = 17;

  [[getter]]
  cell getXchgPairCode() = 18;

  [[getter, dyn_chain_parse]]
  cell getSellPriceCode(address tip3_addr) = 19;

  [[getter, dyn_chain_parse]]
  cell getXchgPriceCode(address tip3_addr1, address tip3_addr2) = 20;

  [[getter, dyn_chain_parse]]
  address getSellTradingPair(address tip3_root) = 21;

  [[getter, dyn_chain_parse]]
  address getXchgTradingPair(address tip3_major_root, address tip3_minor_root) = 22;

  [[getter]]
  uint8 getDealsLimit() = 23;

  [[getter]]
  address getNotifyAddr() = 24;
};
using IFlexPtr = handle<IFlex>;

struct DFlex {
  uint256 deployer_pubkey_;
  TonsConfig tons_cfg_;
  std::optional<cell> pair_code_;
  std::optional<cell> xchg_pair_code_;
  std::optional<cell> price_code_;
  std::optional<cell> xchg_price_code_;
  uint8 deals_limit_; // deals limit in one processing in Price
  address notify_addr_;
};

__interface EFlex {
};

// Prepare Flex StateInit structure and expected contract address (hash from StateInit)
inline
std::pair<StateInit, uint256> prepare_flex_state_init_and_addr(DFlex flex_data, cell flex_code) {
  cell flex_data_cl =
    prepare_persistent_data<IFlex, flex_replay_protection_t, DFlex>(
      flex_replay_protection_t::init(), flex_data
    );
  StateInit flex_init {
    /*split_depth*/{}, /*special*/{},
    flex_code, flex_data_cl, /*library*/{}
  };
  cell flex_init_cl = build(flex_init).make_cell();
  return { flex_init, uint256(tvm_hash(flex_init_cl)) };
}

}} // namespace tvm::schema

