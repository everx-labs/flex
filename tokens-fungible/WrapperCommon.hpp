/** \file
 *  \brief WrapperCommon data-structs.
 *  ...
 *  \author Andrew Zhogin, Vasily Selivanov
 *  \copyright 2019-2022 (c) TON LABS
 */

#pragma once

#include "TONTokenWallet.hpp"

namespace tvm
{
  /// Wrapper onTip3Transfer answer
  struct WrapperRet
  {
    uint32 err_code;     ///< Error code (0 if success).
    address flex_wallet; ///< Deployed flex wallet.
  };

  /// Arguments to deploy wallet. Should be attached as a payload cell in transfer notification.
  struct FlexDeployWalletArgs
  {
    uint256 pubkey;    ///< New wallet's public key.
    address_opt owner; ///< New wallet's internal owner address.
    uint128 evers;     ///< Evers to be sent to the deployable wallet.
  };

  /// Wrapper persistent data struct
  struct DWrapper
  {
    string name_;                    ///< Token name.
    string symbol_;                  ///< Token short symbol.
    uint8 decimals_;                 ///< Decimals for ui purposes. ex: balance 100 with decimals 2 will be printed as 1.00.
    int8 workchain_id_;              ///< Workchain id.
    uint256 root_pubkey_;            ///< Public key of the Wrapper. Used for address calculation (no external access).
    uint128 total_granted_;          ///< Total allocated and granted tokens.
    optcell internal_wallet_code_;   ///< Internal (Flex) wallet code.
    Evers start_balance_;            ///< Start balance of Wrapper (to keep on deploy).
    address_opt flex_;               ///< Flex root
    opt<ITONTokenWalletPtr> wallet_; ///< External (wrapped) tip3 wallet, owned by the Wrapper.
  };
  /// \interface EWrapper
  /// \brief Wrapper events interface
  struct EWrapper
  {
  };

} // namespace tvm
