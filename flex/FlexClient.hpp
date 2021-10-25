#pragma once

#include <tvm/schema/message.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>

#include "Price.hpp"
#include "PriceXchg.hpp"

namespace tvm { inline namespace schema {

__interface IFlexClient {

  [[external]]
  void constructor(uint256 pubkey, cell trading_pair_code, cell xchg_pair_code) = 10;

  [[external, noaccept]]
  void setFlexCfg(TonsConfig tons_cfg, address_t flex) = 11;

  // === additional configuration necessary for deploy wrapper === //
  [[external, noaccept]]
  void setExtWalletCode(cell ext_wallet_code) = 12;

  [[external, noaccept]]
  void setFlexWalletCode(cell flex_wallet_code) = 13;

  [[external, noaccept]]
  void setFlexWrapperCode(cell flex_wrapper_code) = 14;
  // === ===================================================== === //

  [[external, noaccept]]
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
  ) = 15;

  [[external, noaccept]]
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
  ) = 16;

  [[external, noaccept]]
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
  ) = 17;

  [[external, noaccept]]
  void cancelSellOrder(
    uint128 price,
    uint128 min_amount,
    uint8   deals_limit,
    uint128 value,
    cell    price_code,
    Tip3Config tip3cfg,
    address_t notify_addr
  ) = 18;

  [[external, noaccept]]
  void cancelBuyOrder(
    uint128 price,
    uint128 min_amount,
    uint8   deals_limit,
    uint128 value,
    cell    price_code,
    Tip3Config tip3cfg,
    address_t notify_addr
  ) = 19;

  [[external, noaccept]]
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
  ) = 20;

  [[external, noaccept]]
  void transfer(
    address_t dest,
    uint128 value,
    bool_t bounce
  ) = 21;

  // Request wrapper registration from Flex Root
  [[external, noaccept]]
  void registerWrapper(
    uint256 wrapper_pubkey,
    uint128 value,
    Tip3Config tip3cfg
  ) = 22;

  // Request trading pair registration from Flex Root
  [[external, noaccept]]
  void registerTradingPair(
    uint256 request_pubkey,
    uint128 value,
    address tip3_root,
    uint128 min_amount,
    address notify_addr
  ) = 23;

  // Request xchg pair registration from Flex Root
  [[external, noaccept]]
  void registerXchgPair(
    uint256 request_pubkey,
    uint128 value,
    address tip3_major_root,
    address tip3_minor_root,
    uint128 min_amount,
    address notify_addr
  ) = 24;

  [[external, noaccept]]
  address deployEmptyFlexWallet(
    uint256 pubkey,
    uint128 tons_to_wallet,
    Tip3Config tip3cfg
  ) = 25;

  [[external, noaccept]]
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
  [[getter]]
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
  address flex_;
  optcell ext_wallet_code_;
  optcell flex_wallet_code_;
  optcell flex_wrapper_code_;
};

__interface EFlexClient {
};

}} // namespace tvm::schema


