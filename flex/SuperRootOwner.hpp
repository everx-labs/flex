/** \file
 *  \brief SuperRootOwner (single owner) contract interfaces and data-structs.
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#pragma once

#include <tvm/schema/message.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/replay_attack_protection/timestamp.hpp>
#include "FlexVersion.hpp"
#include "WICCloneEvers.hpp"
#include "PairCloneEvers.hpp"
#include "EversConfig.hpp"
#include "Tip3Config.hpp"
#include "bind_info.hpp"

namespace tvm {

/// SuperRootOwner details for getter
struct SuperRootOwnerDetails {
  bool        initialized;        ///< All code cells initialized
  uint256     pubkey;             ///< Owner public key
  address_opt super_root;         ///< SuperRoot address
  optcell     super_root_code;    ///< Code of SuperRoot (unsalted)
  optcell     global_cfg_code;    ///< Code of GlobalConfig (unsalted)
  optcell     flex_client_stub;   ///< FlexClientStub code (unsalted)
  optcell     wrappers_cfg_code;  ///< Code of WrappersConfig (unsalted)
  optcell     wic_code;           ///< Code of WrapperIndexContract (WIC, unsalted)
  optcell     flex_code;          ///< Code of Flex (unsalted)
  optcell     pair_code;          ///< Code of XchgPair (unsalted)
  optcell     price_code;         ///< Code of PriceXchg (unsalted)
  optcell     user_data_cfg_code; ///< UserDataConfig code (unsalted)
  optcell     flex_client_code;   ///< FlexClient code (unsalted)
  optcell     auth_index_code;    ///< AuthIndex code (unsalted)
  optcell     user_id_index_code; ///< UserIdIndexCode (unsalted)
};

/// Sub-component code type for setCode
enum class sroot_code_type {
  super_root = 1,
  global_cfg = 2,
  flex_client_stub = 3,
  wrappers_cfg = 4,
  wic = 5,
  flex = 6,
  pair = 7,
  price = 8,
  user_data_cfg = 9,
  flex_client = 10,
  auth_index = 11,
  user_id_index = 12
};

/** \interface ISuperRootOwner
 *  \brief SuperRootOwner contract interface.
 *  SuperRootOwner is an owning contract for SuperRoot.
 *  First version just proxying external pubkey access into internal command messages to the SuperRoot.
 */
__interface ISuperRootOwner {

  /// Constructor of SuperRootOwner
  [[external]]
  void constructor(
    uint256 pubkey ///< Owner's public key.
  );

  /// Set code of specific sub-component
  [[external]]
  void setCode(
    uint8 type, ///< Must be `sroot_code_type` enum
    cell  code  ///< Sub-component code
  );

  /// Deploy SuperRoot
  [[external]]
  address deploySuperRoot(
    uint128     evers,          ///< Evers for processing
    address_opt prev_super_root ///< Previous super root (if any)
  );

  /// Update Flex to the new beta version. The current release version will remain working.
  /// Flex will send deploy message with new GlobalConfig.
  /// `release` method should be called also to convert beta into current release.
  [[external]]
  void update(
    uint128     main_evers,       ///< Main call evers
    uint128     cfg_deploy_evers, ///< Evers to send in GlobalConfig deploy message
    uint128     cfg_keep_evers,   ///< Evers to keep in GlobalConfig contract
    FlexVersion version,          ///< New Flex version
    address     wrappers_cfg,     ///< WrappersConfig
    address     flex,             ///< New Flex root address
    address     user_cfg,         ///< New user contracts configuration
    string      description       ///< Update description
  );

  /// Convert beta-version into release.
  [[external]]
  void release(
    uint128 main_evers ///< Main call evers
  );

  /// Add wrapper type. Will fail if the update procedure started.
  [[external]]
  void addWrapperType(
    uint128 main_evers,              ///< Main call evers
    uint128 wrappers_cfg_keep_evers, ///< Evers to keep in WrappersConfig before returning the rest back
    address wrappers_cfg,            ///< WrappersConfig address
    uint8   type,                    ///< New Wrapper type number, must be equal to wrapper_deployers_.size()
    address wrapper_deployer         ///< Wrapper deployer contract
  );

  /// Add wrapper to list a new token. Will fail if the update procedure started.
  [[external]]
  void addWrapper(
    uint128       main_evers,              ///< Main call evers
    uint128       wrappers_cfg_keep_evers, ///< Evers to keep in WrappersConfig before returning the rest back
    address       wrappers_cfg,            ///< WrappersConfig address
    WICCloneEvers evers,                   ///< Evers configuration
    string        symbol,                  ///< Token symbol
    uint8         type,                    ///< Wrapper type number
    cell          init_args                ///< Initial wrapper args for deployer
  );

  /// Add XchgPair to list a new trading pair. Will fail if the update procedure started.
  [[external]]
  void addXchgPair(
    uint128        main_evers,      ///< Main call evers
    address        flex,            ///< Flex address
    PairCloneEvers evers,           ///< Evers configuration for cloning procedure
    Tip3Config     major_tip3cfg,   ///< Major tip3 configuration
    Tip3Config     minor_tip3cfg,   ///< Minor tip3 configuration
    uint128        min_amount,      ///< Minimum amount of major tokens for a deal or an order
    uint128        minmove,         ///< Minimum move for price
    uint128        price_denum,     ///< Price denominator for the pair
    address        notify_addr      ///< Notification address (AMM)
  );

  /// Unlist Wrapper
  [[external]]
  void unlistWrapper(
    uint128 main_evers,   ///< Main call evers
    address wrappers_cfg, ///< WrappersConfig address
    address wic           ///< WIC (WrapperIndexContract) address
  );

  /// Unlist XchgPair
  [[external]]
  void unlistXchgPair(
    uint128 main_evers, ///< Main call evers
    address flex,       ///< Flex address
    address pair        ///< XchgPair address
  );

  /// Upgrade WrapperBroxus's wallet
  [[external]]
  void upgradeBroxusWrapperWallet(
    uint128 main_evers,   ///< Main call evers
    address wrapper       ///< WrapperBroxus address
  );

  /// Deploy WrappersConfig contract.
  [[external]]
  resumable<address> deployWrappersConfig(
    uint128 main_evers,              ///< Main call evers
    uint128 deploy_evers,            ///< Evers for deploy contract
    uint128 wrappers_cfg_keep_evers, ///< Evers to keep in WrappersConfig contract
    uint32  token_version            ///< Token update group version
  );

  /// Deploy Flex contract.
  [[external]]
  resumable<address> deployFlex(
    uint128        main_evers,       ///< Main call evers
    uint128        deploy_evers,     ///< Evers for deploy Flex contract
    uint128        keep_evers,       ///< Evers to keep in Flex contract
    PairCloneEvers evers,            ///< Evers configuration for each pair cloning procedure
    address_opt    old_flex,         ///< Old Flex to clone pairs from
    uint32         exchange_version, ///< Exchange update group version
    EversConfig    ev_cfg,           ///< Processing costs configuration of Flex in native funds (evers)
    uint8          deals_limit       ///< Limit for processed deals in one request
  );

  /// Deploy UserDataConfig contract.
  [[external]]
  resumable<address> deployUserDataConfig(
    uint128     main_evers,   ///< Main call evers
    uint128     deploy_evers, ///< Evers for deploy contract
    FlexVersion triplet,      ///< Flex version
    address     flex          ///< Flex address
  );

  /// Clone WrappersConfig to the new version
  [[external]]
  resumable<address> cloneWrappersConfig(
    uint128             main_evers,             ///< Main call evers
    address             wrappers_cfg,           ///< WrappersConfig address
    uint128             wrapper_cfg_keep_evers, ///< Evers to keep in WrappersConfig before returning the rest back
    uint128             clone_deploy_evers,     ///< Evers to send in WrappersConfig clone deploy
    WICCloneEvers       wic_evers,              ///< Evers configuration (for each WIC clone)
    uint32              new_token_version,      ///< New token update group version
    dict_array<address> wrapper_deployers       ///< Wrapper deployers for different types of Wrappers
  );

  /// Set flags. If optional is not set, flag will not be changed.
  [[external]]
  void setFlags(
    uint128   main_evers,    ///< Main call evers
    opt<bool> stop_trade,    ///< Recommendation to stop trading (in case if problem found in trading algorithms)
    opt<bool> abandon_ship,  ///< Recommendation to cancel all orders and withdraw funds (in case if critical problem found)
    opt<bool> update_started ///< Update procedures started (Wrappers/Flex/UI cloning started)
  );

  /// Transfer native evers from super root. Not allowed for the update team.
  [[external]]
  void transfer(
    uint128 main_evers, ///< Main call evers
    address to,         ///< Destination address
    uint128 evers       ///< Evers to transfer
  );

  /// Transfer flex tip3 tokens from a reserve wallet. Not allowed for the update team.
  [[external]]
  void transferReserveTokens(
    uint128 main_evers, ///< Main call evers
    address wrapper,    ///< Wrapper address
    uint128 tokens,     ///< Tokens to transfer
    address to          ///< Destination address (must be flex tip3 wallet)
  );

  /// Set new owner contract. This owner contract will loose ownership access.
  [[external]]
  void setOwner(
    uint128 main_evers, ///< Main call evers
    address owner       ///< New owner contract
  );

  /// Set update team address. Update team is allowed to deploy beta versions and related contracts.
  [[external]]
  void setUpdateTeam(
    uint128     main_evers, ///< Main call evers
    address_opt team        ///< Update team address. Empty means revoke access.
  );

  /// Set next (updated) super root.
  [[external]]
  void setNextSuperRoot(
    uint128 main_evers,     ///< Main call evers
    address next_super_root ///< Next super root
  );

  /// Get contract details
  [[getter]]
  SuperRootOwnerDetails getDetails();
};
using ISuperRootOwnerPtr = handle<ISuperRootOwner>;

/// SuperRootOwner persistent data struct
struct DSuperRootOwner {
  uint256     pubkey_;             ///< Owner public key
  address_opt super_root_;         ///< SuperRoot address
  optcell     super_root_code_;    ///< Code of SuperRoot (unsalted)
  optcell     global_cfg_code_;    ///< Code of GlobalConfig (unsalted)
  optcell     flex_client_stub_;   ///< FlexClientStub code (unsalted)
  optcell     wrappers_cfg_code_;  ///< Code of WrappersConfig (unsalted)
  optcell     wic_code_;           ///< Code of WrapperIndexContract (WIC, unsalted)
  optcell     flex_code_;          ///< Code of Flex (unsalted)
  optcell     pair_code_;          ///< Code of XchgPair (unsalted)
  optcell     price_code_;         ///< Code of PriceXchg (unsalted)
  optcell     user_data_cfg_code_; ///< UserDataConfig code (unsalted)
  optcell     flex_client_code_;   ///< FlexClient code (unsalted)
  optcell     auth_index_code_;    ///< AuthIndex code (unsalted)
  optcell     user_id_index_code_; ///< UserIdIndexCode (unsalted)
};

/// \interface ESuperRootOwner
/// \brief SuperRootOwner events interface
__interface ESuperRootOwner {
};

} // namespace tvm
