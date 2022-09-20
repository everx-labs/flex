/** \file
 *  \brief SuperRoot contract interfaces and data-structs.
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#pragma once

#include <tvm/schema/message.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/replay_attack_protection/timestamp.hpp>
#include "FlexVersion.hpp"
#include "EversConfig.hpp"
#include "WICCloneEvers.hpp"
#include "PairCloneEvers.hpp"
#include "Tip3Config.hpp"
#include "bind_info.hpp"

namespace tvm {

/// SuperRoot details for getter
struct SuperRootDetails {
  uint256          pubkey;             ///< Deployer public key
  bool             stop_trade_;        ///< Recommendation to stop trading (in case if problem found in trading algorithms)
  bool             abandon_ship_;      ///< Recommendation to cancel all orders and withdraw funds (in case if critical problem found)
  bool             update_started_;    ///< Update procedures started (Wrappers/Flex/UI cloning started)
  address          owner;              ///< Owner contract address
  address_opt      update_team;        ///< Update team address. Update team is allowed to deploy beta versions and related contracts.
  cell             global_config_code; ///< Code of GlobalConfig (salted)
  uint256          global_config_hash; ///< Code hash of GlobalConfig (of salted code)
  int8             workchain_id;       ///< Workchain id
  opt<FlexVersion> version;            ///< Latest Flex version
  opt<FlexVersion> beta_version;       ///< Beta Flex version
  address_opt      deploying_cfg;      ///< Deploying GlobalConfig contract
  address_opt      cur_cfg;            ///< Address of the current GlobalConfig (for current version, if not null)
  address_opt      beta_cfg;           ///< Address of the beta GlobalConfig (for beta version, if not null)
  address_opt      prev_super_root;    ///< Will point to the previous super root
  address_opt      next_super_root;    ///< Will be set up if new super root is deployed and should be used instead
  uint32           revision;           ///< SuperRoot revision
};

/** \interface ISuperRoot
 *  \brief SuperRoot contract interface.
 *  SuperRoot is an immutable flex super root contract, persistent through updates.
 */
__interface ISuperRoot {

  /// Deploy processing
  [[internal]]
  void onDeploy(
    cell        global_config_code, ///< GlobalConfig code
    cell        flex_client_stub,   ///< FlexClientStub code
    address_opt prev_super_root     ///< Previous super root
  ) = 10;

  /// Update Flex to the new beta version.
  /// Flex will send deploy message with new GlobalConfig. The current release version will remain working.
  /// `release` method should be called also to convert beta into current release.
  /// Allowed also for the update team if set.
  [[internal]]
  void update(
    uint128        cfg_deploy_evers, ///< Evers to send in GlobalConfig deploy message
    uint128        cfg_keep_evers,   ///< Evers to keep in GlobalConfig contract
    FlexVersion    version,          ///< New Flex version
    address        wrappers_cfg,     ///< WrappersConfig
    address        flex,             ///< New Flex root address
    address        user_cfg,         ///< New user contracts configuration
    string         description       ///< Update description
  ) = 11;

  /// Callback from GlobalConfig contract about successfull deploy.
  /// SuperRoot will update version_ field when receive this notification.
  [[internal]]
  void updateConfirmed(
    FlexVersion version ///< Confirmed new Flex version
  ) = 12;

  /// Convert beta-version into release. Not allowed for the update team.
  [[internal]]
  void release() = 13;

  /// Perform proxy call from SuperRoot to an owned subpart (WrappersConfig, Flex, UserDataConfig)
  [[internal]]
  void proxy(
    cell msg,                     ///< Message to send from SuperRoot
    bool cant_work_during_update, ///< Operation must fail if update procedure is already started
    bool starting_update          ///< This operation starting update
  ) = 14;

  /// Deploy WrappersConfig contract. Allowed also for the update team if set.
  [[internal, answer_id]]
  address deployWrappersConfig(
    uint128 deploy_evers,            ///< Evers for deploy contract
    uint128 wrappers_cfg_keep_evers, ///< Evers to keep in WrappersConfig contract
    uint32  token_version,           ///< Token update group version
    cell    wrappers_cfg_code,       ///< WrappersConfig contract code
    cell    wic_code                 ///< Wrapper Index Contract code
  ) = 15;

  /// Deploy Flex contract. Allowed also for the update team if set.
  [[internal, answer_id]]
  address deployFlex(
    uint128        deploy_evers,     ///< Evers for deploy Flex contract
    uint128        keep_evers,       ///< Evers to keep in Flex contract
    PairCloneEvers evers,            ///< Evers configuration for each pair cloning procedure
    address_opt    old_flex,         ///< Old Flex to clone pairs from
    uint32         exchange_version, ///< Exchange update group version
    cell           flex_code,        ///< Code of Flex contract (unsalted)
    cell           xchg_pair_code,   ///< Code of XchgPair contract (unsalted)
    cell           xchg_price_code,  ///< Code of PriceXchg contract (unsalted)
    EversConfig    ev_cfg,           ///< Processing costs configuration of Flex in native funds (evers)
    uint8          deals_limit       ///< Limit for processed deals in one request
  ) = 16;

  /// Deploy UserDataConfig contract. Allowed also for the update team if set.
  [[internal, answer_id]]
  address deployUserDataConfig(
    uint128     deploy_evers,       ///< Evers for deploy contract
    FlexVersion triplet,            ///< Flex version
    bind_info   binding,            ///< Exchange binding info (Flex address and PriceXchg code hash (unsalted))
    cell        user_data_cfg_code, ///< UserDataConfig code
    cell        flex_client_code,   ///< FlexClient code
    cell        auth_index_code,    ///< AuthIndex code
    cell        user_id_index_code  ///< UserIdIndexCode
  ) = 17;

  /// Clone WrappersConfig to the new version. Allowed also for the update team if set.
  [[internal, answer_id]]
  void cloneWrappersConfig(
    address             wrappers_cfg,           ///< WrappersConfig address
    uint128             wrapper_cfg_keep_evers, ///< Evers to keep in WrappersConfig before returning the rest back
    uint128             clone_deploy_evers,     ///< Evers to send in WrappersConfig clone deploy
    WICCloneEvers       wic_evers,              ///< Evers configuration (for each WIC clone)
    uint32              new_token_version,      ///< New token update group version
    dict_array<address> wrapper_deployers       ///< Wrapper deployers for different types of Wrappers
  ) = 18;

  /// Transfer native evers from super root. Not allowed for the update team.
  [[internal]]
  void transfer(
    address to,   ///< Destination address
    uint128 evers ///< Evers to transfer
  ) = 19;

  /// Transfer flex tip3 tokens from a reserve wallet. Not allowed for the update team.
  [[internal]]
  void transferReserveTokens(
    address wrapper, ///< Wrapper address
    uint128 tokens,  ///< Tokens to transfer
    address to       ///< Destination address (must be flex tip3 wallet)
  ) = 20;

  /// Set flags. If optional is not set, flag will not be changed. Not allowed for the update team.
  [[internal]]
  void setFlags(
    opt<bool> stop_trade,    ///< Recommendation to stop trading (in case if problem found in trading algorithms)
    opt<bool> abandon_ship,  ///< Recommendation to cancel all orders and withdraw funds (in case if critical problem found)
    opt<bool> update_started ///< Update procedures started (Wrappers/Flex/UI cloning started)
  ) = 21;

  /// Set owner contract. Pubkey access will be unavailable after this. Not allowed for the update team.
  [[internal]]
  void setOwner(
    address owner ///< New owner contract
  ) = 22;

  /// Set update team address. Update team is allowed to deploy beta versions and related contracts. Not allowed for the update team.
  [[internal]]
  void setUpdateTeam(
    address_opt team ///< Update team address. Empty means revoke access.
  ) = 23;

  /// Set next (updated) super root. Not allowed for the update team.
  [[internal]]
  void setNextSuperRoot(
    address next_super_root ///< Next super root
  ) = 24;

  /// Get contract details
  [[getter]]
  SuperRootDetails getDetails() = 25;

  /// Get GlobalConfig address for specified version
  [[getter]]
  address getGlobalConfig(FlexVersion version) = 26;

  /// Get current GlobalConfig address
  [[getter]]
  address getCurrentGlobalConfig() = 27;
};
using ISuperRootPtr = handle<ISuperRoot>;

/// SuperRoot persistent data struct
struct DSuperRoot {
  uint256          pubkey_;                ///< Deployer public key
  bool_t           stop_trade_;            ///< Recommendation to stop trading (in case if problem found in trading algorithms)
  bool_t           abandon_ship_;          ///< Recommendation to cancel all orders and withdraw funds (in case if critical problem found)
  bool_t           update_started_;        ///< Update procedures started (Wrappers/Flex/UI cloning started).
  address          owner_;                 ///< Owner contract address
  address_opt      update_team_;           ///< Update team address. Update team is allowed to deploy beta versions and related contracts.
  optcell          global_config_code_;    ///< Code of GlobalConfig (salted)
  optcell          flex_client_stub_;      ///< Code of FlexClientStub (salted)
  int8             workchain_id_;          ///< Workchain id
  opt<FlexVersion> version_;               ///< Latest Flex version
  opt<FlexVersion> beta_version_;          ///< Beta Flex version
  address_opt      deploying_cfg_;         ///< Deploying GlobalConfig contract
  address_opt      prev_super_root_;       ///< Will point to the previous super root
  address_opt      next_super_root_;       ///< Will be set up if new super root is deployed and should be used instead
  uint32           revision_;              ///< SuperRoot revision
};

/// \interface ESuperRoot
/// \brief SuperRoot events interface
__interface ESuperRoot {
};

/// Prepare StateInit struct and std address to deploy SuperRoot contract
template<>
struct preparer<ISuperRoot, DSuperRoot> {
  __always_inline
  static std::pair<StateInit, uint256> execute(DSuperRoot data, cell code) {
    cell data_cl = prepare_persistent_data<ISuperRoot, void>({}, data);
    StateInit init { {}, {}, code, data_cl, {} };
    cell init_cl = build(init).make_cell();
    return { init, uint256(tvm_hash(init_cl)) };
  }
};

} // namespace tvm
