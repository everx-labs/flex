/** \file
 *  \brief Wrapper Index Contract interfaces and data-structs.
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#pragma once

#include "WrappersConfig.hpp"

#include <tvm/schema/message.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>

namespace tvm {

/// Wrapper Index Contract configuration in code salt
struct WICSalt {
  address super_root;              ///< SuperRoot address
  address wrappers_cfg;            ///< WrappersConfig address
  uint256 wrappers_cfg_code_hash;  ///< WrappersConfig salted code hash (for recognition of the next WrappersConfig)
  uint16  wrappers_cfg_code_depth; ///< WrappersConfig salted code depth (for recognition of the next WrappersConfig)
};

/// WIC details for getter
struct WICDetails {
  string       symbol;       ///< Token symbol
  int8         workchain_id; ///< Workchain id
  opt<address> deployer;     ///< Wrapper deployer used to deploy the wrapper
  opt<address> wrapper;      ///< Wrapper address
  opt<uint8>   type;         ///< Wrapper type
  opt<cell>    init_args;    ///< Init args
  opt<address> next;         ///< Next WIC address
  bool         unlisted;     ///< Wrapper is unlisted and should not be cloned during cloneUpgrade
};

/** \interface IWIC
 *  \brief Wrapper Index Contract interface.
 *  WIC is a contract to index (enumerate) wrappers
 */
__interface IWIC {
  [[deploy, internal]]
  resumable<void> onDeploy(
    uint128     keep_evers,       ///< Keep evers in the contract
    address_opt old_wrappers_cfg, ///< Old WrappersConfig address (empty if it is original listing deploy)
    address_opt old_wrapper,       ///< Old wrapper for this clone
    bool        keep_wrapper,      ///< If deployer is not changed (at cloning), keep_wrapper will be true.
    address     deployer,         ///< Wrapper deployer address
    uint8       type,             ///< Wrapper type
    cell        init_args         ///< Init args
  ) = 10;

  /// Set next WIC address
  [[internal]]
  void setNext(
    address_opt old_wrappers_cfg, ///< Old WrappersConfig address in case of call from previous `next` WIC
    opt<string> next_symbol,      ///< Next WIC `symbol`. Must be set iff old_wrappers_cfg is set.
    address     next              ///< New `next` WIC in the linked list
  ) = 11;

  /// Clone this WIC to the new Flex version.
  /// This call will be chained to the next WIC (if exists).
  [[internal]]
  void cloneUpgrade(
    WICCloneEvers       evers,            ///< Evers configuration
    opt<address>        first_clone,      ///< First WIC clone (if it is a nested call)
    opt<address>        last_clone,       ///< Last WIC clone (if it is a nested call)
    opt<string>         prev_symbol,      ///< Previous WIC symbol (if it is a nested call). Including unlisted.
    uint32              wic_count,        ///< WIC count
    uint32              token_version,    ///< Tokens update group version
    address             new_wrappers_cfg, ///< New WrappersConfig address
    dict_array<address> wrapper_deployers ///< Wrapper deployers for different types
  ) = 12;

  /// Mark this WIC/Wrapper as unlisted. Unlisted WIC will not be cloned during cloneUpgrade.
  [[internal]]
  void unlist() = 13;

  /// Get info about contract state details
  [[getter]]
  WICDetails getDetails() = 14;
};
using IWICPtr = handle<IWIC>;

/// Wrapper Index Contract persistent data struct
struct DWIC {
  string       symbol_;       ///< Token symbol. Used for address calculation
  // ^^^ Fields defined at deploy (used for address calculation) ^^^
  int8         workchain_id_; ///< Workchain id
  opt<address> deployer_;     ///< Wrapper deployer used to deploy the wrapper
  opt<address> wrapper_;      ///< Wrapper address
  opt<uint8>   type_;         ///< Wrapper type
  opt<cell>    init_args_;    ///< Init args
  opt<address> next_;         ///< Next WIC address
  bool_t       unlisted_;     ///< Wrapper is unlisted and should not be cloned during cloneUpgrade
};

/// \interface EWIC
/// \brief Wrapper Index Contract events interface
__interface EWIC {
};

/// Prepare StateInit struct and std address to deploy WIC contract
template<>
struct preparer<IWIC, DWIC> {
  __always_inline
  static std::pair<StateInit, uint256> execute(DWIC data, cell code) {
    cell data_cl = prepare_persistent_data<IWIC, void>({}, data);
    StateInit init { {}, {}, code, data_cl, {} };
    cell init_cl = build(init).make_cell();
    return { init, uint256(tvm_hash(init_cl)) };
  }
};

} // namespace tvm
