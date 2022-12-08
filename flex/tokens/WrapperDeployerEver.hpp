/** \file
 *  \brief WrapperDeployerEver interfaces and data-structs for WrapperEver deployment
 *  WrapperDeployerEver deploys WrapperEver contracts for native Evers wrapping into tip3 tokens
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) EverFlex Inc
 */

#pragma once

#include <tvm/schema/basics.hpp>
#include <tvm/schema/message.hpp>
#include <tvm/contract_handle.hpp>
#include "immutable_ids.hpp"

namespace tvm {

/** \interface IWrapperDeployerEver
 *  \brief WrapperDeployerEver implementation contract interface.
 */
__interface IWrapperDeployerEver {

  /// Additional initialization on deploy.
  [[external]]
  void constructor(
    uint256 pubkey,               ///< Public key
    uint256 wrapper_pubkey,       ///< Public key for all Wrappers (used only for address calculation)
    address super_root,           ///< SuperRoot address
    uint128 wrapper_deploy_value, ///< Evers for deploy wrapper
    uint128 wrapper_keep_balance, ///< Keep evers balance in the Wrapper contract
    uint128 reserve_wallet_value  ///< Evers for reserve token wallet
  ) = 10;

  /// Set WrapperEver code
  [[external]]
  void setWrapperEverCode(cell code) = 11;

  /// Set Flex wallet code
  [[external]]
  void setFlexWalletCode(cell code) = 12;

  /// Deploy WrapperEver
  [[internal, answer_id]]
  std::pair<address, uint256> deploy(cell init_args, cell wic_unsalted_code) = immutable_ids::wrapper_deployer_deploy_id;
};
using IWrapperDeployerEverPtr = handle<IWrapperDeployerEver>;

/// WrapperDeployerEver persistent data
struct DWrapperDeployerEver {
  uint256 pubkey_;               ///< Public key for external access
  uint256 wrapper_pubkey_;       ///< Public key for all Wrappers (used only for address calculation)
  uint128 ext_wallet_value_;     ///< Evers to send in (Wrapper owned) external wallet deploy
  uint128 wrapper_deploy_value_; ///< Evers for deploy wrapper
  uint128 wrapper_keep_balance_; ///< Keep evers balance in the Wrapper contract
  uint128 reserve_wallet_value_; ///< Evers for reserve token wallet
  address super_root_;           ///< SuperRoot address
  optcell wrapper_code_;         ///< Wrapper code
  optcell flex_wallet_code_;     ///< Internal (Flex) Tip3 wallet code
};

/// WrapperDeployerTip3 events
struct EWrapperDeployerEver {
};

} // namespace tvm
