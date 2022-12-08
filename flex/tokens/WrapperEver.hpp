/** \file
 *  \brief WrapperEver contract interfaces and data-structs.
 *  WrapperEver contract performes conversion of Everscale native tokens (Evers) into flex tip3 wallet tokens.
 *  Flex tip3 wallet supports only internal ownership (by some contract),
 *   and supports lend ownership to temporary grant ownership of tokens to Price / PriceXchg contracts for trading.
 *  Macroses TIP3_WRAPPER_WALLET_CODE_HASH / TIP3_WRAPPER_WALLET_CODE_DEPTH
 *   must be defined in compilation (code hash and code depth of FlexWallet).
 *  Use tonos-cli to get those values :
 *      tonos-cli decode stateinit --tvc FlexWallet.tvc
 *  ...
 *  "code_hash": "daf8d5fab85a9698fc9546e1beefbe0f1373f24d7980294bf26c0d5793cf1338",
 *  "data_hash": "55a703465a160dce20481375de2e5b830c841c2787303835eb5821d62d65ca9d",
 *  "code_depth": "20",
 *  "data_depth": "1",
 *  ...
 *  \author Andrew Zhogin, Vasily Selivanov
 *  \copyright 2019-2022 (c) EverFlex Inc
 */

#pragma once

#include "TONTokenWallet.hpp"
#include "WrapperCommon.hpp"
#include "immutable_ids.hpp"
#include "EverReTransferArgs.hpp"

namespace tvm {

// ===== Flex Wrapper Contract ===== //
/** \interface IWrapperEver
 *  \brief WrapperEver contract interface.
 */
__interface IWrapperEver {

  /// Additional initialization on deploy.
  [[internal, answer_id, deploy]]
  bool init(
    uint128 reserve_wallet_evers, ///< Evers to send in deploy message for reserve wallet
    cell internal_wallet_code     ///< FlexWallet (internal wallet) code
  ) = 10;

  /// Deploy empty wallet.
  [[internal, answer_id]]
  address deployEmptyWallet(
    uint256     pubkey,  ///< New wallet's public key.
    address_opt owner,   ///< New wallet's internal owner address.
    uint128     evers    ///< Evers to be sent to the deployable wallet.
  ) = 11;

  /// \brief Implements ITONTokenWalletNotify::onEverTransfer().
  /// Notification about incoming tokens from user wallet.
  [[internal]]
  void onEverTransfer(
    uint128              tokens, ///< Amount of tokens received in transfer.
    FlexDeployWalletArgs args    ///< Payload. Must be FlexDeployWalletArgs.
  ) = 202;

  /// \brief Burn internal tokens.
  /** Flex wallet will call this method when self destruct and wants to convert internal tokens to native Evers.
      WrapperEver will check that sender is a correct flex wallet
       and will transfer the same amount of tokens from its wallet to (out_pubkey, out_owner) user wallet **/
  [[internal]]
  void burn(
    uint128     tokens,        ///< Amount of tokens (balance of flex wallet on burn).
    address     answer_addr,   ///< Answer address.
    uint256     sender_pubkey, ///< Sender wallet pubkey.
    address_opt sender_owner,  ///< Sender wallet internal owner.
    uint256     out_pubkey,    ///< Pubkey of external (wrapped) tip3 wallet.
                               ///< Where to return external tip3 tokens.
    address_opt out_owner,     ///< Internal owner (contract) of external (wrapped) tip3 wallet.
                               ///< Where to return external tip3 tokens.
    opt<cell>   notify         ///< Notification payload to the destination wallet's owner
  ) = 12;

  /// Transfer gathered fees from reserve wallet to other wallet. Request allowed only from Flex root.
  [[internal]]
  void transferFromReserveWallet(
    address_opt answer_addr, ///< Answer address (where to return unspent native evers). If empty, sender will be used.
    address     to,          ///< Destination tip3 wallet address.
    uint128     tokens       ///< Amount of tokens (balance of flex wallet on burn).
  ) = 13;

  /// Request total granted tokens.
  [[internal, answer_id]]
  uint128 requestTotalGranted() = 14;

  /// Request cloned wrapper address
  [[internal, answer_id]]
  std::pair<address_opt, uint256> cloned() = immutable_ids::wrapper_cloned_id;

  /// Set cloned wrapper address
  [[internal]]
  void setCloned(
    address_opt cloned,             ///< Cloned Wrapper of this
    uint256 cloned_pubkey,          ///< Cloned Wrapper's pubkey
    address wrappers_cfg,           ///< WrappersConfig address
    uint256 wrappers_cfg_code_hash, ///< WrappersConfig salted code hash (for recognition of the next WrappersConfig)
    uint16  wrappers_cfg_code_depth ///< WrappersConfig salted code depth (for recognition of the next WrappersConfig)
  ) = immutable_ids::wrapper_set_cloned_id;

  /// Get info about contract state details.
  [[getter]]
  wrapper_details_info getDetails() = immutable_ids::wrapper_get_details_id;

  /// Get Tip3 configuration
  [[getter]]
  Tip3Config getTip3Config() = immutable_ids::wrapper_get_tip3_config_id;

  /// If internal wallet code was set up
  [[getter]]
  bool hasInternalWalletCode() = 16;

  /// Calculate flex tip3 wallet address
  [[getter]]
  address getWalletAddress(
    uint256     pubkey, ///< Public key of the wallet.
    address_opt owner   ///< Internal owner address of the wallet.
  ) = immutable_ids::wrapper_get_wallet_address_id;

  /// Get reserve token wallet address for this Wrapper
  [[getter]]
  address getReserveWallet() = 18;
};
using IWrapperEverPtr = handle<IWrapperEver>;

/// Prepare WrapperEver StateInit structure and expected contract address (hash from StateInit)
template<>
struct preparer<IWrapperEver, DWrapper> {
  __always_inline
  static std::pair<StateInit, uint256> execute(DWrapper data, cell code) {
    cell data_cl = prepare_persistent_data<IWrapperEver, void, DWrapper>({}, data);
    StateInit init { {}, {}, code, data_cl, {} };
    cell init_cl = build(init).make_cell();
    return { init, uint256(tvm_hash(init_cl)) };
  }
};

} // namespace tvm
