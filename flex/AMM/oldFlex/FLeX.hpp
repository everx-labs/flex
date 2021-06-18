#pragma once

#include "tvm/schema/message.hpp"
#include "tvm/smart_switcher.hpp"
#include "tvm/contract_handle.hpp"

namespace tvm { namespace schema {

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
  // ... to send notification about completed deal (IFLeXNotify)
  uint128 send_notify;
};

__interface IFLeXNotify {
  [[internal, noaccept]]
  void onDealCompleted(address tip3root, uint128 price, uint128 amount);
};
using IFLeXNotifyPtr = handle<IFLeXNotify>;

__interface IFLeX {

  [[external, dyn_chain_parse]]
  void constructor(uint256 deployer_pubkey,
    uint128 transfer_tip3, uint128 return_ownership, uint128 trading_pair_deploy,
    uint128 order_answer, uint128 process_queue, uint128 send_notify,
    uint128 min_amount, uint8 deals_limit, address notify_addr);

  // To fit message size limit, setPairCode/setPriceCode in separate functions
  //  (not in constructor)
  [[external, noaccept]]
  void setPairCode(cell code);

  [[external, noaccept]]
  void setPriceCode(cell code);

  // ========== getters ==========

  // means setPairCode/setPriceCode executed
  [[getter]]
  bool_t isFullyInitialized();

  [[getter]]
  TonsConfig getTonsCfg();

  [[getter]]
  cell getTradingPairCode();

  [[getter, dyn_chain_parse]]
  cell getSellPriceCode(address tip3_addr);

  [[getter, dyn_chain_parse]]
  cell getXchgPriceCode(address tip3_addr1, address tip3_addr2);

  [[getter, dyn_chain_parse]]
  address getSellTradingPair(address tip3_root);

  [[getter]]
  uint128 getMinAmount();

  [[getter]]
  uint8 getDealsLimit();

  [[getter]]
  address getNotifyAddr();
};
using IFLeXPtr = handle<IFLeX>;

struct DFLeX {
  uint256 deployer_pubkey_;
  TonsConfig tons_cfg_;
  std::optional<cell> pair_code_;
  std::optional<cell> price_code_;
  uint128 min_amount_; // minimum amount to buy/sell
  uint8 deals_limit_; // deals limit in one processing in Price
  address notify_addr_;
};

__interface EFLeX {
};

}} // namespace tvm::schema

