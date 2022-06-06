/** \file
 *  \brief WrappersConfig contract interfaces and data-structs.
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 *
 *  WrappersConfig keeps Wrappers configuration: first / last WIC addresses, wrapper deployers.
 *  \section upgrade Upgrade cloning procedure
 *  New WrappersConfig is cloned from previous version. WICs will be created for the same tokens (except marked unlisted).
 *  Wrappers with new code will be deployed for new WICs.
@dot
digraph wic_upgrade {
 graph [splines=ortho, nodesep=1 ordering="out" outputorder="nodesfirst"]
 node [shape=oval]
 ra [ shape=plaintext label="onWICsCloned" ];
 sroot [ shape=record label="SuperRoot" group="L" ]
 cfg [ shape=record label="WrappersConfig" group="L"];
 cfg2 [ shape=record label="WrappersConfig'" group="R"];
 deployer [ shape=record label="Deployer'" group="R+"];
 flx [ label="FLX" group="L"];
 kwt [ label="KWT" group="L" ];
 last [ label="Last" group="L" ];
 flx2 [ label="FLX'" group="R" ];
 kwt2 [ label="KWT'" group="R" ];
 last2 [ label="Last'" group="R" ];
 flx2_wrapper [ label="FLX' wrapper" group="R+" shape=octagon ];
 upgrade0 [ shape=plaintext label="cloneUpgrade" group="L" ];
 upgrade1 [ shape=plaintext label="cloneUpgrade" group="L" ];
 upgrade2 [ shape=plaintext label="cloneUpgrade" group="L" ];
 upgradeN [ shape=plaintext label="..." group="L" ];
 upgradeN2 [ shape=plaintext label="..." group="R" ];
 deploycfg [ shape=plaintext label="onDeploy" group="M" ];
 deploy1 [ shape=plaintext label="onDeploy" group="M" ];
 deploy2 [ shape=plaintext label="onDeploy" group="M" ];
 setnext [ shape=plaintext label="setNext" group="M" ];
 { rank=same ra cfg deploycfg cfg2 deployer }
 { rank=same flx deploy1 flx2 flx2_wrapper }
 { rank=same kwt deploy2 kwt2 }
 { rank=same upgrade2 setnext }
 { rank=same upgradeN upgradeN2 }
 { rank=same last last2 }
 sroot -> upgrade0 [ color=lightgray dir=none ];
 upgrade0 -> cfg [ color=lightgray ];
 cfg -> deploycfg [ color=lightgray dir=none ];
 deploycfg -> cfg2 [ color=lightgray ];
 cfg -> upgrade1 [ color=lightgray dir=none ];
 upgrade1 -> flx [ color=lightgray ];
 flx -> deploy1 [ color=lightgray dir=none ];
 deploy1 -> flx2 [ color=lightgray ];
 flx -> upgrade2 [ color=lightgray dir=none ];
 upgrade2 -> kwt [ color=lightgray ];
 kwt -> setnext [ color=lightgray dir=none ];
 setnext -> flx2 [ color=lightgray ];
 kwt -> deploy2 [ color=lightgray dir=none ];
 kwt -> deploy2 [ color=lightgray dir=none style=invis ];
 deploy2 -> kwt2 [ color=lightgray ];
 kwt -> upgradeN [ color=lightgray dir=none ];
 upgradeN -> last [ color=lightgray ];
 cfg2 -> flx2 [dir=none color=lightgray style=dashed];
 flx2 -> kwt2 [dir=none color=lightgray style=dashed];
 kwt2 -> upgradeN2 [dir=none color=lightgray style=dashed];
 upgradeN2 -> last2 [dir=none color=lightgray style=dashed];
 last -> ra [ dir=none color=lightgray constraint=false ]
 ra -> cfg2 [ color=lightgray ];
 flx2 -> deployer [ color=lightgray ];
 deployer -> flx2 [ color=lightgray ];
 deployer -> flx2_wrapper [ color=lightgray ];
}
@enddot
**/

#pragma once

#include <tvm/schema/message.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/replay_attack_protection/timestamp.hpp>
#include "WICCloneEvers.hpp"

namespace tvm {

/// WrappersConfig configuration in code salt
struct WrappersConfigSalt {
  address super_root; ///< Flex SuperRoot address
  cell    wic_code;   ///< WIC code (unsalted)
};

/// WrappersConfig details for getter
struct WrappersConfigDetails {
  uint32              token_version;     ///< Tokens update group version
  dict_array<address> wrapper_deployers; ///< Wrapper deployers for different types of Wrappers
  address_opt         first_wic;         ///< First Wrapper Index Contract
  address_opt         last_wic;          ///< Last Wrapper Index Contract
  uint32              wic_count;         ///< WIC count
  uint256             salted_wic_hash;   ///< Salted WIC code hash
};

/// Wrapper deployer info
struct WrapperDeployer {
  address deployer;          ///< Deployer address
  uint256 wrapper_code_hash; ///< Wrapper code hash
  uint256 wallet_code_hash;  ///< Wallet code hash
};

/** \interface IWrappersConfig
 *  \brief WrappersConfig contract interface.
 *  WrappersConfig keeps wrappers configuration for specific token version
 */
__interface IWrappersConfig {

  /// Deploy processing
  [[deploy, internal]]
  void onDeploy(
    uint128             keep_evers,        ///< Keep evers, before sending the remaining to WIC->cloneUpgrade (used only for clone-deploy)
    opt<WICCloneEvers>  evers,             ///< Evers configuration for each WIC clone (empty for original deploy)
    opt<uint32>         old_token_version, ///< Old token version (empty for original deploy)
    dict_array<address> wrapper_deployers, ///< Wrapper deployers (empty for original deploy)
    address_opt         first_wic,         ///< First WIC of the previous WrappersConfig (empty for original deploy)
    address_opt         last_wic,          ///< Last WIC of the previous WrappersConfig (empty for original deploy)
    uint32              wic_count          ///< WIC count
  );

  /// Notification from last WIC (old) about completion of WICs cloning procedure to the new WrappersConfig
  [[internal]]
  void onWICsCloned(
    address_opt first_wic, ///< First cloned WIC
    address_opt last_wic,  ///< Last cloned WIC
    uint32      wic_count  ///< WIC count
  );

  /// Add Wrapper type
  [[internal]]
  void addWrapperType(
    uint128 keep_evers,      ///< Keep evers in the contract before returning the rest to SuperConfig
    uint8   type,            ///< New Wrapper type number, must be equal to wrapper_deployers_.size()
    address wrapper_deployer ///< Wrapper deployer contract
  );

  /// Add Wrapper of already registered type
  [[internal]]
  void addWrapper(
    uint128       keep_evers, ///< Keep evers in the contract before returning the rest to SuperConfig
    WICCloneEvers evers,      ///< Evers configuration (for each WIC clone)
    string        symbol,     ///< Token symbol
    uint8         type,       ///< Wrapper type number
    cell          init_args   ///< Initial wrapper args for deployer
  );

  /// Unlist Wrapper
  [[internal]]
  void unlistWrapper(
    address wic ///< WIC (WrapperIndexContract) address
  );

  /// Create clone of this contract with new wrappers and WICs.
  /// Returns address of the new WrappersConfig clone.
  /// \note Return message with clone address is sent immediately
  [[internal, answer_id]]
  address cloneUpgrade(
    address_opt         answer_addr,        ///< Answer address
    uint128             keep_evers,         ///< Keep evers in the contract before returning the rest to SuperConfig
    uint128             clone_deploy_evers, ///< Evers to send in the clone deploy
    WICCloneEvers       evers,              ///< Evers configuration (for each WIC clone)
    uint32              new_token_version,  ///< New tokens update group version
    dict_array<address> wrapper_deployers   ///< Wrapper deployers for different types of Wrappers
  );

  /// Get info about contract state details
  [[getter]]
  WrappersConfigDetails getDetails();

  /// Get config from code salt
  [[getter]]
  WrappersConfigSalt getConfig();
};
using IWrappersConfigPtr = handle<IWrappersConfig>;

/// WrappersConfig persistent data struct
struct DWrappersConfig {
  uint32              token_version_;     ///< Tokens update group version. Used for address calculation
  // ^^^ Fields defined at deploy (used for address calculation) ^^^
  bool_t              deployed_;          ///< onDeploy call processed
  uint128             keep_evers_;        ///< Keep evers in the contract before returning the rest to SuperConfig
  int8                workchain_id_;      ///< Workchain id
  dict_array<address> wrapper_deployers_; ///< Wrapper deployers for different types of Wrappers
  address_opt         first_wic_;         ///< First Wrapper Index Contract
  address_opt         last_wic_;          ///< Last Wrapper Index Contract. May be set when first_wic_ is unset -
                                          ///<  it means we are waiting onWICsCloned callback from this old last_wic_.
  uint32              wic_count_;         ///< WIC count
};

/// \interface EWrappersConfig
/// \brief WrappersConfig events interface
__interface EWrappersConfig {
};

/// Prepare StateInit struct and std address to deploy WrappersConfig contract
template<>
struct preparer<IWrappersConfig, DWrappersConfig> {
  __always_inline
  static std::pair<StateInit, uint256> execute(DWrappersConfig data, cell code) {
    cell data_cl = prepare_persistent_data<IWrappersConfig, void>({}, data);
    StateInit init { {}, {}, code, data_cl, {} };
    cell init_cl = build(init).make_cell();
    return { init, uint256(tvm_hash(init_cl)) };
  }
};

/// Calculate expected hash from StateInit (contract deploy address). Using code_hash and code_depth.
template<>
struct expecter<IWrappersConfig, DWrappersConfig> {
  __always_inline
  static uint256 execute(DWrappersConfig data, uint256 code_hash, uint16 code_depth) {
    cell data_cl = prepare_persistent_data<IWrappersConfig, void>({}, data);
    return tvm_state_init_hash(code_hash, uint256(tvm_hash(data_cl)), code_depth, uint16(data_cl.cdepth()));
  }
};

} // namespace tvm
