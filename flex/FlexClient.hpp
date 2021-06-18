#pragma once

#include <tvm/schema/message.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>

#include "Price.hpp"
#include "PriceXchg.hpp"

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
  cell tip3_code;
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
  cell tip3_code;
  ref<Tip3Config> tip3cfg;
};

struct FLeXXchgCfgs {
  ref<Tip3Config> major_tip3cfg;
  ref<Tip3Config> minor_tip3cfg;
};

struct FLeXXchgArgs {
  bool_t  sell;
  uint128 price_num;
  uint128 price_denum;
  uint128 amount;
  uint128 lend_amount;
  uint32  lend_finish_time;
  uint128 min_amount;
  uint8   deals_limit;
  uint128 tons_value;
  cell    xchg_price_code;
  ref<FLeXSellArgsAddrs> addrs;
  cell    tip3_code;
  ref<FLeXXchgCfgs> tip3cfgs;
};

struct FLeXCancelArgs {
  uint128 price;
  uint128 min_amount;
  uint8   deals_limit;
  uint128 value;
  cell    price_code;
  cell    tip3_code;
  ref<Tip3Config> tip3cfg;
};

struct FLeXXchgCancelArgs {
  bool_t  sell;
  uint128 price_num;
  uint128 price_denum;
  uint128 min_amount;
  uint8   deals_limit;
  uint128 value;
  cell    xchg_price_code;
  cell    tip3_code;
  ref<FLeXXchgCfgs> tip3cfgs;
};

__interface IFLeXClient {

  [[external, dyn_chain_parse]]
  void constructor(uint256 pubkey, cell trading_pair_code, cell xchg_pair_code);

  [[external, noaccept, dyn_chain_parse]]
  void setFLeXCfg(TonsConfig tons_cfg, address flex, address notify_addr);

  [[external, noaccept, dyn_chain_parse]]
  address deployTradingPair(address tip3_root, uint128 deploy_min_value, uint128 deploy_value);

  [[external, noaccept, dyn_chain_parse]]
  address deployXchgPair(address tip3_major_root, address tip3_minor_root,
                         uint128 deploy_min_value, uint128 deploy_value);

  // args must be FLeXSellArgs
  [[external, noaccept]]
  address deployPriceWithSell(cell args);

  // args must be FLeXBuyArgs
  [[external, noaccept]]
  address deployPriceWithBuy(cell args);

  // args must be FLeXXchgArgs
  [[external, noaccept]]
  address deployPriceXchg(cell args_cl);

  [[external, noaccept]]
  void cancelSellOrder(cell args);

  [[external, noaccept]]
  void cancelBuyOrder(cell args);

  [[external, noaccept]]
  void cancelXchgOrder(cell args);

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
  cell xchg_pair_code_;
  int8 workchain_id_;
  TonsConfig tons_cfg_;
  address flex_;
  address notify_addr_;
};

__interface EFLeXClient {
};

}} // namespace tvm::schema

