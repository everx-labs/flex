/** \file
 *  \brief Subpart of ITokenWallet interface (Broxus standart).
 *  Adapted to c++ from https://github.com/broxus/ton-eth-bridge-token-contracts/blob/master/contracts/interfaces/ITokenWallet.sol
 **/

#pragma once

namespace tvm { namespace broxus {

/** \interface ITokenWallet
 *  \brief Subpart of ITokenWallet interface (Broxus standart). Adapted to c++.
 **/
__interface ITokenWallet {

  /** Creates new token wallet.
    * All the parameters are specified as initial data
    **/
  [[internal]]
  void constructor();

  /**
    Transfer tokens and optionally deploy TokenWallet for recipient
    \note Can be called only by TokenWallet owner
    \note If deployWalletValue !=0 deploy token wallet for recipient using that gas value
  **/
  [[internal]]
  void transfer(
    uint128 amount,            ///< How much tokens to transfer
    address recipient,         ///< Tokens recipient address
    uint128 deployWalletValue, ///< How much EVERs to attach to token wallet deploy
    address remainingGasTo,    ///< Remaining gas receiver
    bool    notify,            ///< Notify receiver on incoming transfer
    cell    payload            ///< Notification payload
  );

  /// Upgrade token wallet
  [[internal]]
  void upgrade(address remainingGasTo);
};

using ITokenWalletPtr = handle<ITokenWallet>;

}} // namespace tvm::broxus
