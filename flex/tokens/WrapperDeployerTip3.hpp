/** \file
 *  \brief WrapperDeployerTip3 interfaces and data-structs for TON Labs tip3 tokens.
 *  WrapperDeployerTip3 deploys Wrapper contracts for TON Labs Tip3 tokens
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#pragma once

#include <tvm/schema/basics.hpp>
#include <tvm/schema/message.hpp>
#include <tvm/contract_handle.hpp>

#include "Tip3Config.hpp"
#include "immutable_ids.hpp"

namespace tvm {

/** \interface IWrapperDeployerTip3
 *  \brief WrapperDeployerTip3 implementation contract interface.
 */
__interface IWrapperDeployerTip3 {

  /// Additional initialization on deploy.
  [[external]]
  void constructor(
    uint256 pubkey,               ///< Public key
    uint256 wrapper_pubkey,       ///< Public key for all Wrappers (used only for address calculation)
    address super_root,           ///< SuperRoot address
    uint128 ext_wallet_value,     ///< Evers to send in (Wrapper owned) external wallet deploy
    uint128 wrapper_deploy_value, ///< Evers for deploy wrapper
    uint128 wrapper_keep_balance, ///< Keep evers balance in the Wrapper contract
    uint128 reserve_wallet_value  ///< Evers for reserve token wallet
  ) = 10;

  /// Set Wrapper code (unsalted). Salt will be added in the call.
  [[external]]
  void setWrapperCode(cell code) = 11;

  /// Set external wallet code
  [[external]]
  void setExtWalletCode(cell code) = 12;

  /// Set Flex wallet code
  [[external]]
  void setFlexWalletCode(cell code) = 13;

  /// Deploy Wrapper
  [[internal, answer_id]]
  std::pair<address, uint256> deploy(cell init_args, cell wic_unsalted_code) = immutable_ids::wrapper_deployer_deploy_id;

  /// Prepare arguments structure in cell
  [[getter]]
  cell getArgs(Tip3Config tip3cfg);
};
using IWrapperDeployerTip3Ptr = handle<IWrapperDeployerTip3>;

/// WrapperDeployerTip3 persistent data
struct DWrapperDeployerTip3 {
  uint256 pubkey_;               ///< Public key for external access
  uint256 wrapper_pubkey_;       ///< Public key for all Wrappers (used only for address calculation)
  uint128 ext_wallet_value_;     ///< Evers to send in (Wrapper owned) external wallet deploy
  uint128 wrapper_deploy_value_; ///< Evers for deploy wrapper
  uint128 wrapper_keep_balance_; ///< Keep evers balance in the Wrapper contract
  uint128 reserve_wallet_value_; ///< Evers for reserve token wallet
  address super_root_;           ///< SuperRoot address
  optcell wrapper_code_;         ///< Wrapper code
  optcell ext_wallet_code_;      ///< External Tip3 wallet code
  optcell flex_wallet_code_;     ///< Internal (Flex) Tip3 wallet code
};

/// WrapperDeployerTip3 events
struct EWrapperDeployerTip3 {
};

} // namespace tvm
