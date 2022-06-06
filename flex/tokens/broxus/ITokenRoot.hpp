/** \file
 *  \brief Subpart of ITokenRoot interface (Broxus standart).
 *  Adapted to c++ from https://github.com/broxus/ton-eth-bridge-token-contracts/blob/master/contracts/interfaces/ITokenRoot.sol
 **/

#pragma once

namespace tvm { namespace broxus {

/** \interface ITokenRoot
 *  \brief Subpart of ITokenRoot interface (Broxus standart). Adapted to c++.
 **/
__interface ITokenRoot {
  /// Deploy new TokenWallet. Can be called by anyone.
  [[internal, answer_id]]
  address deployWallet(
    address owner,            ///< Token wallet owner address
    uint128 deployWalletValue ///< Gas value to
  );
};

using ITokenRootPtr = handle<ITokenRoot>;

}} // namespace tvm::broxus
