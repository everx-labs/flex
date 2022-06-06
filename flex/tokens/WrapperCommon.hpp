/** \file
 *  \brief WrapperCommon data-structs.
 *  ...
 *  \author Andrew Zhogin, Vasily Selivanov
 *  \copyright 2019-2022 (c) TON LABS
 */

#pragma once

#include "TONTokenWallet.hpp"

namespace tvm {

/// Wrapper onTip3Transfer answer
struct WrapperRet {
  uint32  err_code;    ///< Error code (0 if success).
  address flex_wallet; ///< Deployed flex wallet.
};

/// Arguments to deploy wallet. Should be attached as a payload cell in transfer notification.
struct FlexDeployWalletArgs {
  uint256     pubkey;     ///< New wallet's public key.
  address_opt owner;      ///< New wallet's internal owner address.
  uint128     evers;      ///< Evers to be sent to the deployable wallet.
  uint128     keep_evers; ///< Evers to be kept in the deployable wallet.
};

/// Wrapper configuration salt
struct WrapperSalt {
  address super_root; ///< SuperRoot address
};

/// Wrapper details info (for getter).
struct wrapper_details_info {
  string      name;            ///< Token name.
  string      symbol;          ///< Token short symbol.
  uint8       decimals;        ///< Decimals for ui purposes. ex: balance 100 with decimals 2 will be printed as 1.00.
  uint256     root_pubkey;     ///< Public key of the Wrapper.
  uint128     total_granted;   ///< Total allocated and granted tokens.
  cell        wallet_code;     ///< Flex wallet code.
  address_opt external_wallet; ///< External wallet address. Null for Evers Wrapper.
  address     reserve_wallet;  ///< Reserve wallet address (to gather fees).
  address     super_root;      ///< Flex SuperRoot address.
  address_opt cloned;          ///< Address of last cloned Wrapper of this
  uint8       type_id;         ///< Wrapper type id (0 - tip3, 1 - evers, 2 - broxus tip3)
};

/// Wrapper persistent data struct
struct DWrapper {
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
  uint8       type_id_;              ///< Wrapper type id (0 - tip3, 1 - evers, 2 - broxus tip3)
};

/// \interface EWrapper
/// \brief Wrapper events interface
struct EWrapper {
};

} // namespace tvm
