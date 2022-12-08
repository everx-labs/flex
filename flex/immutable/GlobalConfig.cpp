/** \file
 *  \brief GlobalConfig contract implementation
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) EverFlex Inc
 */

#include "GlobalConfig.hpp"
#include "SuperRoot.hpp"
#include <tvm/contract.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/default_support_functions.hpp>
#include <tvm/suffixes.hpp>
#include <tvm/schema/parse_chain_static.hpp>
#include <tvm/schema/build_chain_static.hpp>

using namespace tvm;

/// GlobalConfig contract implementation
class GlobalConfig final : public smart_interface<IGlobalConfig>, public DGlobalConfig {
  using data = DGlobalConfig;
public:
  static constexpr bool _checked_deploy = true; /// To allow deploy message only with `onDeploy` call
  struct error_code : tvm::error_code {
    static constexpr unsigned message_sender_is_not_my_owner = 100; ///< Authorization error
    static constexpr unsigned double_deploy                  = 101; ///< Double deploy
    static constexpr unsigned uninitialized                  = 102; ///< Contract is uninitialized
    static constexpr unsigned only_super_root_may_deploy_me  = 103; ///< Only Flex SuperRoot may deploy this contract
    static constexpr unsigned wrong_wrapper_type_num         = 104; ///< Wrong wrapper type number
  };

  void onDeploy(
    uint128        keep_evers,
    address        wrappers_cfg,
    address        flex,
    address        user_cfg,
    string         description
  ) {
    require(!flex_, error_code::double_deploy);
    require(int_sender() == getConfig().super_root, error_code::only_super_root_may_deploy_me);
    wrappers_cfg_ = wrappers_cfg;
    flex_         = flex;
    user_cfg_     = user_cfg;
    description_  = description;

    tvm_rawreserve(keep_evers.get(), rawreserve_flag::up_to);
    ISuperRootPtr(int_sender())(0_ev, SEND_ALL_GAS).updateConfirmed(version_);
  }

  GlobalConfigDetails getDetails() {
    require(wrappers_cfg_ && flex_ && user_cfg_ && description_, error_code::uninitialized);
    return { version_, *wrappers_cfg_, *flex_, *user_cfg_, *description_ };
  }

  GlobalConfigSalt getConfig() {
    return parse_chain_static<GlobalConfigSalt>(parser(tvm_code_salt()));
  }

  // default processing of unknown messages
  static int _fallback([[maybe_unused]] cell msg, [[maybe_unused]] slice msg_body) {
    return 0;
  }
  // default processing of empty messages
  static int _receive([[maybe_unused]] cell msg, [[maybe_unused]] slice msg_body) {
    return 0;
  }
  // =============== Support functions ==================
  DEFAULT_SUPPORT_FUNCTIONS(IGlobalConfig, void)
};

DEFINE_JSON_ABI(IGlobalConfig, DGlobalConfig, EGlobalConfig);

// ----------------------------- Main entry functions ---------------------- //
MAIN_ENTRY_FUNCTIONS_NO_REPLAY(GlobalConfig, IGlobalConfig, DGlobalConfig)
