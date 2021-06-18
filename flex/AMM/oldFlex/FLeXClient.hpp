#pragma once

#include <tvm/schema/message.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>

#include "Price.hpp"

namespace tvm { namespace schema {

struct FLeXSellArgsAddrs {
  address my_tip3_addr;
  address receive_wallet;
};

struct FLeXSellArgs {
  uint128 price;
  uint128 amount;
  uint32 lend_finish_time;
  uint128 min_amount;
  uint8 deals_limit;
  uint128 tons_value;
  cell price_code;
  ref<FLeXSellArgsAddrs> addrs;
  ref<Tip3Config> tip3cfg;
};

struct FLeXBuyArgs {
  uint128 price;
  uint128 amount;
  uint32 order_finish_time;
  uint128 min_amount;
  uint8 deals_limit;
  uint128 deploy_value;
  cell price_code;
  address my_tip3_addr;
  ref<Tip3Config> tip3cfg;
};

struct FLeXCancelArgs {
  uint128 price;
  uint128 min_amount;
  uint8 deals_limit;
  uint128 value;
  cell price_code;
  ref<Tip3Config> tip3cfg;
};

__interface IFLeXClient {

  [[external, dyn_chain_parse]]
  void constructor(uint256 pubkey, cell trading_pair_code);

  [[external, noaccept, dyn_chain_parse]]
  void setFLeXCfg(TonsConfig tons_cfg, address flex, address notify_addr);

  [[external, noaccept, dyn_chain_parse]]
  address deployTradingPair(address flex_addr, address tip3_root,
                            uint128 deploy_min_value, uint128 deploy_value);

  // args must be FLeXSellArgs
  [[external, noaccept]]
  address deployPriceWithSell(cell args);

  // args must be FLeXBuyArgs
  [[external, noaccept]]
  address deployPriceWithBuy(cell args);

  [[external, noaccept]]
  void cancelSellOrder(cell args);

  [[external, noaccept]]
  void cancelBuyOrder(cell args);

  [[external, noaccept, dyn_chain_parse]]
  void transfer(address dest, uint128 value, bool_t bounce);

  [[getter]]
  uint256 getOwner();

  [[getter]]
  address getFLeX();
};
using IFLeXClientPtr = handle<IFLeXClient>;

struct DFLeXClient {
  uint256 owner_;
  cell trading_pair_code_;
  int8 workchain_id_;
  TonsConfig tons_cfg_;
  address flex_;
  address notify_addr_;
};

__interface EFLeXClient {
};

}} // namespace tvm::schema

