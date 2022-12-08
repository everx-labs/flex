/** \file
 *  \brief Exchange pair (tip3/tip3) contract interfaces and data-structs.
 *  XchgPair - contract defining tip3/tip3 exchange pair. May only be deployed by Flex root contract.
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) EverFlex Inc
 */

#pragma once

#include "FlexWallet.hpp"
#include "XchgPairSalt.hpp"

#include <tvm/schema/message.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>

namespace tvm {

/// XchgPair details (for getter)
struct XchgPairDetails {
  address     tip3_major_root;      ///< Address of Wrapper for major tip3 token
  address     tip3_minor_root;      ///< Address of Wrapper for minor tip3 token
  uint128     min_amount;           ///< Minimum amount of major tokens for a deal or an order
  uint128     minmove;              ///< Minimum move for price
  uint128     price_denum;          ///< Price denominator for the pair
  address     notify_addr;          ///< Notification address (AMM)
  address     major_reserve_wallet; ///< Major reserve wallet
  address     minor_reserve_wallet; ///< Minor reserve wallet
  Tip3Config  major_tip3cfg;        ///< Configuration of the major tip3 token
  Tip3Config  minor_tip3cfg;        ///< Configuration of the minor tip3 token
  address_opt next;                 ///< Next XchgPair address
  bool        unlisted;             ///< If pair is unlisted
};

/** \interface IXchgPair
 *  \brief XchgPair contract interface.
 */
__interface IXchgPair {

  /// Initialization method, may only be called by Flex root or previous XchgPair
  [[internal, deploy]]
  void onDeploy(
    uint128    min_amount,    ///< Minimum amount of major tokens for a deal or an order
    uint128    minmove,       ///< Minimum move for price
    uint128    price_denum,   ///< Price denominator for the pair
    uint128    deploy_value,  ///< Crystals to be kept in the contract
    address    notify_addr,   ///< Notification address (AMM)
    Tip3Config major_tip3cfg, ///< Major tip3 configuration
    Tip3Config minor_tip3cfg  ///< Minor tip3 configuration
  ) = 10;

  /// Request contract details (by internal message)
  [[internal, answer_id]]
  XchgPairDetails requestDetails() = 11;

  /// Set `next` pair address
  [[internal]]
  void setNext(address next) = 12;

  /// Set `unlisted` flag
  [[internal]]
  void unlist() = 13;

  // ========== getters ==========
  /// Get contract details
  [[getter]]
  XchgPairDetails getDetails() = immutable_ids::pair_get_details_id;

  /// Get XchgPair configuration from code salt (common for all pairs of one flex)
  [[getter]]
  XchgPairSalt getConfig() = 14;

  /// Get PriceXchg contract code with configuration salt added
  [[getter]]
  cell getPriceXchgCode(bool salted) = immutable_ids::pair_get_price_xchg_code_id;

  /// Get PriceXchg salt (configuration) for this pair
  [[getter]]
  cell getPriceXchgSalt() = 15;
};
using IXchgPairPtr = handle<IXchgPair>;

/// XchgPair persistent data struct
struct DXchgPair {
  address tip3_major_root_;       ///< Address of Wrapper for major tip3 token. Used for address calculation.
  address tip3_minor_root_;       ///< Address of Wrapper for minor tip3 token. Used for address calculation.
  // ^^^ Fields defined at deploy (used for address calculation) ^^^
  uint128 min_amount_;            ///< Minimum amount of major tokens for a deal or an order
  uint128 minmove_;               ///< Minimum move for price
  uint128 price_denum_;           ///< Price denominator for the pair
  address notify_addr_;           ///< Notification address (AMM)
  address major_reserve_wallet_;  ///< Major reserve wallet
  address minor_reserve_wallet_;  ///< Minor reserve wallet
  opt<Tip3Config> major_tip3cfg_; ///< Configuration of the major tip3 token.
  opt<Tip3Config> minor_tip3cfg_; ///< Configuration of the minor tip3 token.
  opt<address>    next_;          ///< Next XchgPair address
  bool_t          unlisted_;      ///< If pair is unlisted
};

/// \interface EXchgPair
/// \brief XchgPair events interface
__interface EXchgPair {
};

/// Prepare Exchange Pair StateInit structure and expected contract address (hash from StateInit)
template<>
struct preparer<IXchgPair, DXchgPair> {
  __always_inline
  static std::pair<StateInit, uint256> execute(DXchgPair data, cell code) {
    cell data_cl = prepare_persistent_data<IXchgPair, void, DXchgPair>({}, data);
    StateInit init { {}, {}, code, data_cl, {} };
    cell init_cl = build(init).make_cell();
    return { init, uint256(tvm_hash(init_cl)) };
  }
};

} // namespace tvm
