#pragma once

#include <tvm/schema/message.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>

#include "Price.hpp"
#include "PriceXchg.hpp"

namespace tvm { inline namespace schema {

__interface IFlexClient {

  [[external, internal]]
  void constructor(uint256 pubkey, cell trading_pair_code, cell xchg_pair_code) = 10;

    [[external, internal, noaccept]]
  void setFlexCfg(TonsConfig tons_cfg, address_t flex) = 11;

  // === additional configuration necessary for deploy wrapper === //
  [[external, internal, noaccept]]
  void setExtWalletCode(cell ext_wallet_code) = 12;

  [[external, internal, noaccept]]
  void setFlexWalletCode(cell flex_wallet_code) = 13;

  [[external, noaccept]]
  void setFlexWrapperCode(cell flex_wrapper_code) = 14;
  // === ===================================================== === //

  [[external, internal, noaccept]]
  address deployTradingPair(
    address_t tip3_root,
    uint128 deploy_min_value,
    uint128 deploy_value,
    uint128 min_trade_amount,
    address_t notify_addr
  ) = 15;

  [[external, internal, noaccept]]
  address deployXchgPair(
    address_t tip3_major_root,
    address_t tip3_minor_root,
    uint128 deploy_min_value,
    uint128 deploy_value,
    uint128 min_trade_amount,
    address_t notify_addr
  ) = 16;

  [[external, internal, noaccept]]
  address deployPriceWithSell(
    uint128 price,
    uint128 amount,
    uint32  lend_finish_time,
    uint128 min_amount,
    uint8   deals_limit,
    uint128 tons_value,
    cell    price_code,
    address_t my_tip3_addr,
    address_t receive_wallet,
    Tip3Config tip3cfg,
    address_t notify_addr
  ) = 17;

  [[external, internal, noaccept, dyn_chain_parse]]
  address deployPriceWithBuy(
    uint128 price,
    uint128 amount,
    uint32 order_finish_time,
    uint128 min_amount,
    uint8 deals_limit,
    uint128 deploy_value,
    cell price_code,
    address_t my_tip3_addr,
    Tip3Config tip3cfg,
    address_t notify_addr
  ) = 18;

  [[external, internal, noaccept]]
  address deployPriceXchg(
    bool_t  sell,
    uint128 price_num,
    uint128 price_denum,
    uint128 amount,
    uint128 lend_amount,
    uint32  lend_finish_time,
    uint128 min_amount,
    uint8   deals_limit,
    uint128 tons_value,
    cell    xchg_price_code,
    address_t my_tip3_addr,
    address_t receive_wallet,
    Tip3Config major_tip3cfg,
    Tip3Config minor_tip3cfg,
    address_t notify_addr
  ) = 19;

  [[external, internal, noaccept]]
  void cancelSellOrder(
    uint128 price,
    uint128 min_amount,
    uint8   deals_limit,
    uint128 value,
    cell    price_code,
    Tip3Config tip3cfg,
    address_t notify_addr
  ) = 20;

  [[external, internal, noaccept]]
  void cancelBuyOrder(
    uint128 price,
    uint128 min_amount,
    uint8   deals_limit,
    uint128 value,
    cell    price_code,
    Tip3Config tip3cfg,
    address_t notify_addr
  ) = 21;

  [[external, internal, noaccept]]
  void cancelXchgOrder(
    bool_t  sell,
    uint128 price_num,
    uint128 price_denum,
    uint128 min_amount,
    uint8   deals_limit,
    uint128 value,
    cell    xchg_price_code,
    Tip3Config major_tip3cfg,
    Tip3Config minor_tip3cfg,
    address_t notify_addr
  ) = 22;

  [[external, internal, noaccept]]
  void transfer(
    address_t dest,
    uint128 value,
    bool_t bounce
  ) = 23;

  [[external, internal, noaccept]]
  address deployWrapperWithWallet(
    uint256 wrapper_pubkey,
    uint128 wrapper_deploy_value,
    uint128 wrapper_keep_balance,
    uint128 ext_wallet_balance,
    uint128 set_internal_wallet_value,
    Tip3Config tip3cfg
  ) = 24;

  [[external, internal, noaccept]]
  address deployEmptyFlexWallet(
    uint256 pubkey,
    uint128 tons_to_wallet,
    Tip3Config tip3cfg
  ) = 25;

  [[external, internal, noaccept]]
  void burnWallet(
    uint128 tons_value,
    uint256 out_pubkey,
    address_t out_internal_owner,
    address_t my_tip3_addr
  ) = 26;

  [[getter]]
  uint256 getOwner() = 27;

  [[getter]]
  address getFlex() = 28;

  [[getter]]
  bool_t hasExtWalletCode() = 29;

  [[getter]]
  bool_t hasFlexWalletCode() = 30;

  [[getter]]
  bool_t hasFlexWrapperCode() = 31;

  // Prepare payload for transferWithNotify call from external wallet to wrapper's wallet
  // to deploy flex internal wallet
  [[getter, dyn_chain_parse]]
  cell getPayloadForDeployInternalWallet(
    uint256 owner_pubkey,
    address_t owner_addr,
    uint128 tons
  ) = 32;
};
using IFlexClientPtr = handle<IFlexClient>;

struct DFlexClient {
  uint256 owner_;
  cell trading_pair_code_;
  cell xchg_pair_code_;
  int8 workchain_id_;
  TonsConfig tons_cfg_;
  addr_std_compact flex_;
  optcell ext_wallet_code_;
  optcell flex_wallet_code_;
  optcell flex_wrapper_code_;
};

__interface EFlexClient {
};

}} // namespace tvm::schema

