/** \file
 *  \brief UserDataConfig contract interfaces and data-structs.
 *
 *  UserDataConfig keeps FlexClient/debots/UI configuration for specific `user` version
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) EverFlex Inc
 */

#pragma once

#include "FlexVersion.hpp"
#include "bind_info.hpp"

#include <tvm/schema/message.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>

namespace tvm {

/// UserDataConfig configuration in code salt
struct UserDataConfigSalt {
  address super_root; ///< Flex SuperRoot address
};

/// UserDataConfig details for getter
struct UserDataConfigDetails {
  FlexVersion triplet;     ///< Flex version triplet
  bind_info   binding;     ///< Exchange binding info (Flex address and PriceXchg code hash (unsalted)).
  cell flex_client_stub;   ///< FlexClientStub code. TODO: We don't need it in requestDetails, maybe optimize?
  cell flex_client_code;   ///< FlexClient code (no salt)
  cell auth_index_code;    ///< AuthIndex code (no salt)
  cell user_id_index_code; ///< UserIdIndex code (unsalted)
};

/** \interface IUserDataConfig
 *  \brief UserDataConfig contract interface.
 *
 *  UserDataConfig keeps FlexClient/debots/UI configuration for specific `user` version
 */
__interface IUserDataConfig {
  /// Deploy processing
  [[deploy, internal]]
  void onDeploy(
    bind_info binding,      ///< Exchange binding info (Flex address and PriceXchg code hash (unsalted))
    cell flex_client_stub,  ///< FlexClientStub code
    cell flex_client_code,  ///< FlexClient code
    cell auth_index_code,   ///< AuthIndex code
    cell user_id_index_code ///< UserIdIndex code
  );

  /// Deploy FlexClient, \p signature - FlexClientStub address.val (without workchain id), signed by owner of the \p pubkey
  [[internal, answer_id]]
  address deployFlexClient(uint256 pubkey, uint128 deploy_evers, bytes signature);

  /// Request info about contract state details (internal call)
  [[internal, answer_id]]
  UserDataConfigDetails requestDetails();

  /// Get FlexClient address
  [[getter]]
  address getFlexClientAddr(uint256 pubkey);

  /// Get info about contract state details
  [[getter]]
  UserDataConfigDetails getDetails();

  /// Get config from code salt
  [[getter]]
  UserDataConfigSalt getConfig();
};
using IUserDataConfigPtr = handle<IUserDataConfig>;

/// UserDataConfig persistent data struct
struct DUserDataConfig {
  FlexVersion    triplet_;            ///< Flex version triplet. Used for address calculation.
  opt<bind_info> binding_;            ///< Exchange binding info (Flex address and PriceXchg code hash (unsalted)).
  optcell        flex_client_stub_;   ///< FlexClientStub code (salted).
  optcell        flex_client_code_;   ///< FlexClient code (no salt).
  optcell        auth_index_code_;    ///< AuthIndex code (no salt).
  optcell        user_id_index_code_; ///< UserIdIndex code (unsalted).
};

/// \interface EUserDataConfig
/// \brief UserDataConfig events interface
__interface EUserDataConfig {
};

/// Prepare StateInit struct and std address to deploy UserDataConfig contract
template<>
struct preparer<IUserDataConfig, DUserDataConfig> {
  __always_inline
  static std::pair<StateInit, uint256> execute(DUserDataConfig data, cell code) {
    cell data_cl = prepare_persistent_data<IUserDataConfig, void>({}, data);
    StateInit init { {}, {}, code, data_cl, {} };
    cell init_cl = build(init).make_cell();
    return { init, uint256(tvm_hash(init_cl)) };
  }
};

} // namespace tvm
