/** \file
 *  \brief AuthIndex contract interfaces and data-structs.
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#pragma once

#include <tvm/schema/message.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/replay_attack_protection/timestamp.hpp>

namespace tvm {

/// AuthIndex configuration data (code salt)
struct AuthIndexSalt {
  address flex; ///< Flex root
};

/** \interface IAuthIndex
 *  \brief AuthIndex contract interface.
 *  Auth index is a contract that helps dApp find user_id and FlexClient address by `dApp_pubkey`.
 *  Auth index contains `dApp_pubkey` as static data and UserIDIndex address as data.
 *  UserIDIndex contract is the owner of Auth Index.
 */
__interface IAuthIndex {

  /// Deploy the contract
  [[deploy, internal, noaccept]]
  void onDeploy() = 10;

  /// Remove the contract and send the remaining evers to \p dst
  [[internal, noaccept]]
  void remove(address dst) = 11;
};
using IAuthIndexPtr = handle<IAuthIndex>;

/// AuthIndex persistent data struct
struct DAuthIndex {
  uint256      pubkey_; ///< dApp public key
  opt<address> owner_;  ///< UserIDIndex address
};

/// \interface EAuthIndex
/// \brief AuthIndex events interface
__interface EAuthIndex {
};

/// Prepare StateInit struct and std address to deploy AuthIndex contract
template<>
struct preparer<IAuthIndex, DAuthIndex> {
  __always_inline
  static std::pair<StateInit, uint256> execute(DAuthIndex data, cell code) {
    cell data_cl = prepare_persistent_data<IAuthIndex, void>({}, data);
    StateInit init { {}, {}, code, data_cl, {} };
    cell init_cl = build(init).make_cell();
    return { init, uint256(tvm_hash(init_cl)) };
  }
};

} // namespace tvm
