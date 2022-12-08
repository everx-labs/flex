/** \file
 *  \brief GlobalConfig contract interfaces and data-structs.
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) EverFlex Inc
 */

#pragma once

#include <tvm/schema/message.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/replay_attack_protection/timestamp.hpp>
#include "FlexVersion.hpp"

namespace tvm {

/// GlobalConfig configuration in code salt
struct GlobalConfigSalt {
  address super_root; ///< Flex SuperRoot address
};

/// GlobalConfig details for getter
struct GlobalConfigDetails {
  FlexVersion    version;      ///< Flex version
  address        wrappers_cfg; ///< WrappersConfig address
  address        flex;         ///< Flex root for this version
  address        user_cfg;     ///< User contracts and debots configuration
  string         description;  ///< Update description
};

/** \interface IGlobalConfig
 *  \brief GlobalConfig contract interface.
 *  GlobalConfig is an immutable "current version update config" contract
 */
__interface IGlobalConfig {
  [[deploy, internal, noaccept]]
  void onDeploy(
    uint128        keep_evers,   ///< Keep evers in this contract
    address        wrappers_cfg, ///< WrappersConfig address
    address        flex,         ///< Flex root address
    address        user_cfg,     ///< User contracts configuration address
    string         description   ///< Update description
  );

  /// Get info about contract state details
  [[getter]]
  GlobalConfigDetails getDetails();

  /// Get config from code salt
  [[getter]]
  GlobalConfigSalt getConfig();
};
using IGlobalConfigPtr = handle<IGlobalConfig>;

/// GlobalConfig persistent data struct
struct DGlobalConfig {
  FlexVersion         version_;           ///< Flex version
  address_opt         wrappers_cfg_;      ///< WrappersConfig address
  address_opt         flex_;              ///< Flex root for this version
  address_opt         user_cfg_;          ///< User contracts and debots configuration
  opt<string>         description_;       ///< Update description
};

/// \interface EGlobalConfig
/// \brief GlobalConfig events interface
__interface EGlobalConfig {
};

/// Prepare StateInit struct and std address to deploy GlobalConfig contract
template<>
struct preparer<IGlobalConfig, DGlobalConfig> {
  __always_inline
  static std::pair<StateInit, uint256> execute(DGlobalConfig data, cell code) {
    cell data_cl = prepare_persistent_data<IGlobalConfig, void>({}, data);
    StateInit init { {}, {}, code, data_cl, {} };
    cell init_cl = build(init).make_cell();
    return { init, uint256(tvm_hash(init_cl)) };
  }
};

} // namespace tvm
