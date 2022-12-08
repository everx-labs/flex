/** \file
 *  \brief FlexClientStub contract interfaces and data-structs
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) EverFlex Inc
 */

#pragma once

#include <tvm/schema/message.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/replay_attack_protection/timestamp.hpp>

#include "FlexVersion.hpp"
#include "bind_info.hpp"

namespace tvm {

static constexpr unsigned FLEX_CLIENT_TIMESTAMP_DELAY = 1800;
using flex_client_replay_protection_t = replay_attack_protection::timestamp<FLEX_CLIENT_TIMESTAMP_DELAY>;

struct FlexClientStubSalt {
  address super_root;
};

/// \brief FlexClientStub is a placement contract to be setcoded to an actual FlexClient
__interface IFlexClientStub {
  /// Deploy notification
  [[internal, deploy]]
  void onDeploy(
    FlexVersion triplet,            ///< Flex version
    bind_info   binding,            ///< Exchange binding info (Flex address and PriceXchg code hash (unsalted))
    cell        flex_client_code,   ///< FlexClient code (actual version, no salt)
    cell        auth_index_code,    ///< AuthIndex code (no salt)
    cell        user_id_index_code, ///< UserIdIndex code (unsalted)
    bytes       signature           ///< Signature to verify user pubkey ownership
  ) = 10;

  /// We need this unused resumable method to reserve awaits map in persistent data header,
  ///  because the real FlexClient requires it.
  [[internal]]
  resumable<void> unused() = 11;
};
using IFlexClientStubPtr = handle<IFlexClientStub>;

/// FlexClientStub persistent data struct
struct DFlexClientStub {
  uint256        owner_;              ///< Owner's public key. Used for address calculation.
  FlexVersion    triplet_;            ///< Version triplet
  FlexVersion    ex_triplet_;         ///< Ex-version triplet (initialized during code upgrade)
  optcell        auth_index_code_;    ///< AuthIndex code (no salt)
  optcell        user_id_index_code_; ///< UserIdIndex code (unsalted)
  opt<bind_info> binding_;            ///< Binding info for exchange
};

/// \interface EFlexClientStub
/// \brief FlexClientStub events interface
__interface EFlexClientStub {
};

/// Prepare StateInit struct and std address to deploy FlexClientStub contract
template<>
struct preparer<IFlexClientStub, DFlexClientStub> {
  __always_inline
  static std::pair<StateInit, uint256> execute(DFlexClientStub data, cell code) {
    auto init_hdr = persistent_data_header<IFlexClientStub, flex_client_replay_protection_t>::init();
    cell data_cl = prepare_persistent_data<IFlexClientStub, flex_client_replay_protection_t>(init_hdr, data);
    StateInit init { {}, {}, code, data_cl, {} };
    cell init_cl = build(init).make_cell();
    return { init, uint256(tvm_hash(init_cl)) };
  }
};

} // namespace tvm
