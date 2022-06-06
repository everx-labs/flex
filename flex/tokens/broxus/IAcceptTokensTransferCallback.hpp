/** \file
 *  \brief Callbacks from TokenWallet (Broxus standart).
 *  Adapted to c++ from https://github.com/broxus/ton-eth-bridge-token-contracts/blob/master/contracts/interfaces/IAcceptTokensTransferCallback.sol
 **/

#pragma once

namespace tvm { namespace broxus {

/** \interface IAcceptTokensTransferCallback
 *  \brief Callback from TokenWallet on receive tokens transfer (Broxus standart). Adapted to c++.
 */
__interface IAcceptTokensTransferCallback {

  /**
      Callback from TokenWallet on receive tokens transfer
  **/
  [[internal]]
  void onAcceptTokensTransfer(
      address tokenRoot,      ///< TokenRoot of received tokens
      uint128 amount,         ///< Received tokens amount
      address sender,         ///< Sender TokenWallet owner address
      address senderWallet,   ///< Sender TokenWallet address
      address remainingGasTo, ///< Address specified for receive remaining gas
      cell    payload         ///< Additional data attached to transfer by sender
  );
}

}} // namespace tvm::broxus
