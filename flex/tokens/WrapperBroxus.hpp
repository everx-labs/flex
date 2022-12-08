/** \file
 *  \brief WrapperBroxus contract interfaces and data-structs.
 *  WrapperBroxus contract performes conversion of external Broxus tip3 wallet tokens into flex tip3 wallet tokens.
 *  Flex tip3 wallet supports only internal ownership (by some contract),
 *   and supports lend ownership to temporary grant ownership of tokens to PriceXchg contract for trading.
 *  Macroses TIP3_WALLET_CODE_HASH / TIP3_WALLET_CODE_DEPTH
 *   must be defined in compilation (code hash and code depth of FlexWallet).
 *  Use tonos-cli to get those values :
 *      tonos-cli decode stateinit --tvc FlexWallet.tvc
 *  ...
 *  "code_hash": "daf8d5fab85a9698fc9546e1beefbe0f1373f24d7980294bf26c0d5793cf1338",
 *  "data_hash": "55a703465a160dce20481375de2e5b830c841c2787303835eb5821d62d65ca9d",
 *  "code_depth": "20",
 *  "data_depth": "1",
 *  ...
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) EverFlex Inc
 */

#pragma once

#include "TONTokenWallet.hpp"
#include "WrapperCommon.hpp"
#include "immutable_ids.hpp"

namespace tvm {

// ===== Flex Wrapper Contract ===== //
/** \interface IWrapperBroxus
 *  \brief WrapperBroxus contract interface.
 */
__interface IWrapperBroxus {

  /// Additional initialization on deploy.
  [[internal, deploy]]
  void init(
    address external_wallet,      ///< External token wallet to keep Wrapper's external tokens
    uint128 reserve_wallet_evers, ///< Evers to send in deploy message for reserve wallet
    cell    internal_wallet_code  ///< FlexWallet (internal wallet) code
  ) = 10;

  /// Deploy empty wallet.
  [[internal, answer_id]]
  address deployEmptyWallet(
    uint256     pubkey,  ///< New wallet's public key.
    address_opt owner,   ///< New wallet's internal owner address.
    uint128     evers    ///< Evers to be sent to the deployable wallet.
  ) = 11;

  /// \brief Implements IAcceptTokensTransferCallback::onAcceptTokensTransfer().
  /// Notification about incoming tokens from Wrapper owned external wallet.
  [[internal]]
  void onAcceptTokensTransfer(
    address tokenRoot,      ///< TokenRoot of received tokens
    uint128 amount,         ///< Received tokens amount
    address sender,         ///< Sender TokenWallet owner address
    address senderWallet,   ///< Sender TokenWallet address
    address remainingGasTo, ///< Address specified for receive remaining gas
    cell    payload         ///< Additional data attached to transfer by sender
  );

  /// \brief Burn internal tokens.
  /** Flex wallet will call this method when self destruct and wants to convert internal tokens to external.
      Wrapper will check that sender is a correct flex wallet
       and will transfer the same amount of tokens from its wallet to (out_owner) Broxus tip3 wallet **/
  [[internal]]
  void burn(
    uint128     tokens,        ///< Amount of tokens (balance of flex wallet on burn).
    address     answer_addr,   ///< Answer address.
    uint256     sender_pubkey, ///< Sender wallet pubkey.
    address_opt sender_owner,  ///< Sender wallet internal owner.
    uint256     out_pubkey,    ///< Pubkey of external tip3 wallet (ignored for Broxus tip3 wallet).
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

  /// Upgrade external wallet (call wallet_->upgrade())
  [[internal]]
  void upgradeExternalWallet();

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
using IWrapperBroxusPtr = handle<IWrapperBroxus>;

/// WrapperBroxus persistent data struct
struct DWrapperBroxus {
  cell        wic_unsalted_code_;    ///< WIC (Wrapper Index Contract) unsalted code (to verify `setCloned` call sender)
  string      name_;                 ///< Token name.
  string      symbol_;               ///< Token short symbol.
  uint8       decimals_;             ///< Decimals for ui purposes. ex: balance 100 with decimals 2 will be printed as 1.00.
  int8        workchain_id_;         ///< Workchain id.
  uint256     root_pubkey_;          ///< Public key of the Wrapper. Used for address calculation (no external access).
  uint128     total_granted_;        ///< Total allocated and granted tokens.
  optcell     internal_wallet_code_; ///< Internal (Flex) wallet code.
  Evers       start_balance_;        ///< Start balance of Wrapper (to keep on deploy).
  address_opt super_root_;           ///< SuperRoot address
  address_opt wallet_;               ///< External (wrapped) tip3 wallet, owned by the Wrapper.
  address_opt cloned_;               ///< Address of last cloned Wrapper of this
  uint256     cloned_pubkey_;        ///< Cloned Wrapper's pubkey
  uint128     out_deploy_value_;     ///< Deploy value for out wallet on withdraw
};

using EWrapperBroxus = EWrapper;

/// Prepare WrapperBroxus StateInit structure and expected contract address (hash from StateInit)
template<>
struct preparer<IWrapperBroxus, DWrapperBroxus> {
  __always_inline
  static std::pair<StateInit, uint256> execute(DWrapperBroxus data, cell code) {
    cell data_cl = prepare_persistent_data<IWrapperBroxus, void, DWrapperBroxus>({}, data);
    StateInit init { {}, {}, code, data_cl, {} };
    cell init_cl = build(init).make_cell();
    return { init, uint256(tvm_hash(init_cl)) };
  }
};

} // namespace tvm
