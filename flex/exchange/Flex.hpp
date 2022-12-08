/** \file
 *  \brief Flex root contract interfaces and data-structs.
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) EverFlex Inc
 */

#pragma once

#include <tvm/schema/message.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/replay_attack_protection/timestamp.hpp>
#include "PriceCommon.hpp"
#include "FlexWallet.hpp"
#include "EversConfig.hpp"
#include "FlexSalt.hpp"
#include "PairCloneEvers.hpp"
#include "XchgPair.hpp"
#include "immutable_ids.hpp"

namespace tvm {

/** \interface IFlexNotify
 *  \brief Notifications to AMM about orders.
 */
__interface IFlexNotify {
  /// Notification about completed exchange deal tip3/tip3
  [[internal]]
  void onXchgDealCompleted(
    bool       seller_is_taker, ///< Seller is a taker in deal
    address    pair,            ///< Address of XchgPair contract
    address    tip3root_major,  ///< Address of RootTokenContract for the major tip3 token
    address    tip3root_minor,  ///< Address of RootTokenContract for the minor tip3 token
    Tip3Config major_tip3cfg,   ///< Major tip3 configuration
    Tip3Config minor_tip3cfg,   ///< Minor tip3 configuration
    uint128    price_num,       ///< Token price numerator
    uint128    price_denum,     ///< Token price denominator
    uint128    amount           ///< Amount of major tip3 tokens in the deal
  ) = 10;
  /// Notification about added exchange order (tip3/tip3)
  [[internal]]
  void onXchgOrderAdded(
    bool    sell,           ///< Is it a sell order
    address tip3root_major, ///< Address of RootTokenContract for the major tip3 token
    address tip3root_minor, ///< Address of RootTokenContract for the minor tip3 token
    uint128 price_num,      ///< Token price numerator
    uint128 price_denum,    ///< Token price denominator
    uint128 amount,         ///< Amount of major tip3 tokens added in the order
    uint128 sum_amount      ///< Summarized amount of major tokens rest in all orders for this price (sell or buy only)
  ) = 11;
  /// Notification about canceled exchange order (tip3/tip3)
  [[internal]]
  void onXchgOrderCanceled(
    bool    sell,           ///< Is it a sell order
    address tip3root_major, ///< Address of RootTokenContract for the major tip3 token
    address tip3root_minor, ///< Address of RootTokenContract for the minor tip3 token
    uint128 price_num,      ///< Token price numerator
    uint128 price_denum,    ///< Token price denominator
    uint128 amount,         ///< Amount of major tip3 tokens canceled
    uint128 sum_amount      ///< Summarized amount of major tokens rest in all orders for this price (sell or buy only)
  ) = 12;
};
using IFlexNotifyPtr = handle<IFlexNotify>;

/// Ownership info for Flex root
struct FlexOwnershipInfo {
  uint256     deployer_pubkey;       ///< Deployer's public key
  string      ownership_description; ///< Ownership description
  address_opt owner; ///< If Flex is managed by other contract (deployer will not be able to execute non-deploy methods)
};

/// Flex root details (for getter)
struct FlexDetails {
  cell        xchg_pair_code;           ///< Exchange pair code (XchgPair)
  uint256     unsalted_price_code_hash; ///< PriceXchg code hash (unsalted)
  address_opt first_pair;               ///< First XchgPair in linked list
  address_opt last_pair;                ///< Last XchgPair in linked list
  uint32      pairs_count;              ///< Count of XchgPair contracts
};

/// Flex pairs list
struct PairsRange {
  address_opt first_pair;               ///< First XchgPair in linked list
  address_opt last_pair;                ///< Last XchgPair in linked list
};

/** \interface IFlex
 *  \brief Flex root contract interface.
 *  Flex is a root contract for exchange system.
 */
__interface IFlex {

  /// Constructor of Flex
  [[internal, deploy]]
  resumable<void> onDeploy(
    uint128        flex_keep_evers,   ///< Evers to keep in Flex
    PairCloneEvers evers,             ///< Evers configuration for each pair cloning procedure
    address_opt    old_flex           ///< Old Flex to clone pairs from
  ) = immutable_ids::flex_on_deploy_id;

  /// Register tip3/tip3 xchg pair (returns pre-calculated address of future xchg pair)
  [[internal, answer_id]]
  address addXchgPair(
    PairCloneEvers evers,       ///< Evers configuration for cloning procedure
    Tip3Config major_tip3cfg,   ///< Major tip3 configuration
    Tip3Config minor_tip3cfg,   ///< Minor tip3 configuration
    uint128    min_amount,      ///< Minimum amount of major tokens for a deal or an order
    uint128    minmove,         ///< Minimum move for price
    uint128    price_denum,     ///< Price denominator for the pair
    address    notify_addr      ///< Notification address (AMM)
    ) = immutable_ids::flex_register_xchg_pair_id;

  /// Unlist tip3/tip3 xchg pair
  [[internal]]
  void unlistXchgPair(
    address pair ///< XchgPair address
  );

  /// Request first/last pair in list
  [[internal, answer_id]]
  PairsRange requestPairs() = immutable_ids::flex_request_pairs_id;
  // ========== getters ==========

  /// Get Flex configuration from code salt
  [[getter]]
  FlexSalt getConfig() = 20;

  /// Get contract state details.
  [[getter]]
  FlexDetails getDetails() = 21;

  /// Get address of tip3/tip3 exchange pair
  [[getter]]
  address getXchgTradingPair(address tip3_major_root, address tip3_minor_root) = 22;

  /// Calculate necessary lend tokens for order
  [[getter]]
  uint128 calcLendTokensForOrder(bool sell, uint128 major_tokens, price_t price) = 23;
};
using IFlexPtr = handle<IFlex>;

/// Flex persistent data struct
struct DFlex {
  uint32      exchange_version_; ///< Exchange update group version
  // ^^^ Fields defined at deploy (used for address calculation) ^^^
  int8          workchain_id_;    ///< Workchain id
  optcell       xchg_pair_code_;  ///< XchgPair code (with salt added)
  address_opt   first_pair_;      ///< First XchgPair in linked list
  address_opt   last_pair_;       ///< Last XchgPair in linked list
  uint32        pairs_count_;     ///< Count of XchgPair contracts
  uint128       flex_keep_evers_; ///< Evers to keep in the contract
  address_opt   it_;              ///< Old pair iterator (for cloning)
  address_opt   prev_clone_;      ///< Previous pair clone (for cloning)
  address_opt   next_;            ///< Next XchgPair address (for cloning)
  address_opt   notify_addr_;     ///< Notification address (AMM) (for cloning)
  uint128       min_amount_;      ///< Minimum amount of major tokens for a deal or an order
  uint128       minmove_;         ///< Minimum move for price
  uint128       price_denum_;     ///< Price denominator for the pair
  opt<Tip3Config> major_tip3cfg_; ///< Configuration of the major tip3 token for current pair (for cloning)
  opt<Tip3Config> minor_tip3cfg_; ///< Configuration of the minor tip3 token for current pair (for cloning)
};

/// \interface EFlex
/// \brief Flex events interface
__interface EFlex {
};

/// Prepare Flex StateInit structure and expected contract address (hash from StateInit)
template<>
struct preparer<IFlex, DFlex> {
  __always_inline
  static std::pair<StateInit, uint256> execute(DFlex data, cell code) {
    cell data_cl = prepare_persistent_data<IFlex, void>({}, data);
    StateInit init { {}, {}, code, data_cl, {} };
    cell init_cl = build(init).make_cell();
    return { init, uint256(tvm_hash(init_cl)) };
  }
};

} // namespace tvm
