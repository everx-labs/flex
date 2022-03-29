/** \file
 *  \brief UserIdIndex contract interfaces and data-structs.
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#pragma once

#include <tvm/schema/message.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/replay_attack_protection/timestamp.hpp>

namespace tvm {

/// UserIdIndex configuration data (code salt)
struct UserIdIndexSalt {
  address owner;           ///< Address that can send command messages to UserIdIndex. It is address of FlexClient.
  cell    auth_index_code; ///< Code of AuthIndex (salted)
};

/** \interface IUserIdIndex
 *  \brief UserIdIndex contract interface.
 *  There is no way to understand which wallets were created for some FlexClient. So we need UserID Index for it.
 *  This contract contains FlexClient address in code salt and we can find all such contracts for specified FlexClient by code hash.
 *  It can be deployed only by its corresponding FlexClient.
 */
__interface IUserIdIndex {

  /// Check that msg.sender is an owner (from configuration salt). Deploy AuthIndex with lend_pubkey.
  [[deploy, internal, noaccept]]
  void onDeploy(
    uint256 lend_pubkey,      ///< Lend public key
    string  name,             ///< User name (encoded)
    uint128 evers_to_auth_idx ///< Evers to AuthIndex deploy
  ) = 10;

  /// Remove AuthIndex with lend_pubkey. Deploy AuthIndex with new_lend_pubkey. Set lend_pubkey = new_lend_pubkey.
  [[internal, noaccept]]
  void reLendPubkey(
    uint256 new_lend_pubkey,  ///< New lend public key
    uint128 evers_to_remove,  ///< Evers for `AuthIndex->remove` call. The remaining will return to the caller (FlexClient).
    uint128 evers_to_auth_idx ///< Evers for new AuthIndex deploy.
  ) = 11;

  /// Remove AuthIndex with lend_pubkey. Redirects the rest of message evers to `AuthIndex->remove` call.
  /// AuthIndex will return the remaining to this method caller (FlexClient).
  [[internal, noaccept]]
  void remove() = 12;

  /// Get config from code salt
  [[getter]]
  UserIdIndexSalt getConfig() = 13;
};
using IUserIdIndexPtr = handle<IUserIdIndex>;

/// UserIdIndex persistent data struct
struct DUserIdIndex {
  int8        workchain_id_;
  uint256     user_id_;     ///< User id, it is used for address calculation.
  uint256     lend_pubkey_; ///< Public key of dApp. We need this field to realize that dApp_pubkey changed
                            ///<  and we need to change lend_pubkey into every user_id’s FlexWalllet.
  opt<string> name_;        ///< Encrypted login. We use naclbox for encryption.
};

/// \interface EUserIdIndex
/// \brief UserIdIndex events interface
__interface EUserIdIndex {
};

/// Prepare StateInit struct and std address to deploy UserIdIndex contract
template<>
struct preparer<IUserIdIndex, DUserIdIndex> {
  __always_inline
  static std::pair<StateInit, uint256> execute(DUserIdIndex data, cell code) {
    cell data_cl = prepare_persistent_data<IUserIdIndex, void>({}, data);
    StateInit init { {}, {}, code, data_cl, {} };
    cell init_cl = build(init).make_cell();
    return { init, uint256(tvm_hash(init_cl)) };
  }
};

} // namespace tvm

