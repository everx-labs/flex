/** \file
 *  \brief WrapperDeployerBroxus interfaces and data-structs for Broxus tip3 tokens.
 *  WrapperDeployerBroxus deploys Wrapper contracts for Broxus Tip3 tokens
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

/** \interface IWrapperDeployerBroxus
 *  \brief WrapperDeployerBroxus implementation contract interface.
 */
__interface IWrapperDeployerBroxus {

  /// Additional initialization on deploy.
  [[external]]
  void constructor(
    uint256 pubkey,               ///< Public key
    uint256 wrapper_pubkey,       ///< Public key for all Wrappers (used only for address calculation)
    address super_root,           ///< SuperRoot address
    uint128 ext_wallet_value,     ///< Evers to send in (Wrapper owned) external wallet deploy
    uint128 wrapper_deploy_value, ///< Evers for deploy wrapper
    uint128 wrapper_keep_balance, ///< Keep evers balance in the Wrapper contract
    uint128 reserve_wallet_value, ///< Evers for reserve token wallet
    uint128 out_deploy_value      ///< Deploy value for out wallet on withdraw
  ) = 10;

  /// Set Wrapper code (unsalted). Salt will be added in the call.
  [[external]]
  void setWrapperCode(cell code) = 11;

  /// Set Flex wallet code
  [[external]]
  void setFlexWalletCode(cell code) = 13;

  /// Deploy Wrapper
  [[internal, answer_id]]
  resumable<std::pair<address, uint256>> deploy(cell init_args, cell wic_unsalted_code) = immutable_ids::wrapper_deployer_deploy_id;

  /// Prepare arguments structure in cell
  [[getter]]
  cell getArgs(Tip3Config tip3cfg);
};
using IWrapperDeployerBroxusPtr = handle<IWrapperDeployerBroxus>;

/// WrapperDeployerBroxus persistent data
struct DWrapperDeployerBroxus {
  uint256 pubkey_;               ///< Public key for external access
  uint256 wrapper_pubkey_;       ///< Public key for all Wrappers (used only for address calculation)
  uint128 ext_wallet_value_;     ///< Evers to send in (Wrapper owned) external wallet deploy
  uint128 wrapper_deploy_value_; ///< Evers for deploy wrapper
  uint128 wrapper_keep_balance_; ///< Keep evers balance in the Wrapper contract
  uint128 reserve_wallet_value_; ///< Evers for reserve token wallet
  uint128 out_deploy_value_;     ///< Deploy value for out wallet on withdraw
  address super_root_;           ///< SuperRoot address
  optcell wrapper_code_;         ///< Wrapper code
  optcell flex_wallet_code_;     ///< Internal (Flex) Tip3 wallet code
};

/// WrapperDeployerBroxus events
struct EWrapperDeployerBroxus {
};

} // namespace tvm
