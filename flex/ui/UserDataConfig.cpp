/** \file
 *  \brief UserDataConfig contract implementation
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#include "UserDataConfig.hpp"
#include "FlexClientStub.hpp"
#include <tvm/default_support_functions.hpp>

using namespace tvm;

class UserDataConfig final : public smart_interface<IUserDataConfig>, public DUserDataConfig {
  using data = DUserDataConfig;
public:
  DEFAULT_SUPPORT_FUNCTIONS(IUserDataConfig, void)

  static constexpr bool _checked_deploy = true; /// To allow deploy message only with `onDeploy` call
  struct error_code : tvm::error_code {
    static constexpr unsigned message_sender_is_not_my_owner = 100; ///< Authorization error
    static constexpr unsigned uninitialized                  = 101; ///< Uninitialized
    static constexpr unsigned double_deploy                  = 102; ///< Double deploy
  };

  void onDeploy(
    bind_info binding,
    cell flex_client_stub,
    cell flex_client_code,
    cell auth_index_code,
    cell user_id_index_code
  ) {
    require(int_sender() == getConfig().super_root, error_code::message_sender_is_not_my_owner);
    require(!binding_, error_code::double_deploy);
    binding_            = binding;
    flex_client_stub_   = flex_client_stub;
    flex_client_code_   = flex_client_code;
    auth_index_code_    = auth_index_code;
    user_id_index_code_ = user_id_index_code;
  }

  address deployFlexClient(uint256 pubkey, uint128 deploy_evers) {
    require(initialized(), error_code::uninitialized);
    auto workchain_id = std::get<addr_std>(tvm_myaddr().val()).workchain_id;
    DFlexClientStub data {.owner_ = pubkey};
    auto [init, hash] = prepare<IFlexClientStub>(data, flex_client_stub_.get());
    IFlexClientStubPtr ptr(address::make_std(workchain_id, hash));

    tvm_rawreserve(tvm_balance() - int_value().get(), rawreserve_flag::up_to);
    ptr.deploy(init, Evers(deploy_evers.get())).onDeploy(
      triplet_, *binding_, flex_client_code_.get(), auth_index_code_.get(), user_id_index_code_.get()
      );

    set_int_return_flag(SEND_ALL_GAS);
    return ptr.get();
  }

  UserDataConfigDetails requestDetails() {
    return _remaining_ev() & getDetails();
  }

  address getFlexClientAddr(uint256 pubkey) {
    require(initialized(), error_code::uninitialized);
    auto workchain_id = std::get<addr_std>(tvm_myaddr().val()).workchain_id;
    DFlexClientStub data {.owner_ = pubkey};
    [[maybe_unused]] auto [init, hash] = prepare<IFlexClientStub>(data, flex_client_stub_.get());
    return address::make_std(workchain_id, hash);
  }

  UserDataConfigDetails getDetails() {
    require(initialized(), error_code::uninitialized);
    return { triplet_, *binding_, flex_client_stub_.get(), flex_client_code_.get(), auth_index_code_.get(), user_id_index_code_.get() };
  }

  UserDataConfigSalt getConfig() {
    return parse_chain_static<UserDataConfigSalt>(parser(tvm_code_salt()));
  }

  bool initialized() const {
    return binding_ && flex_client_stub_ && flex_client_code_ && auth_index_code_ && user_id_index_code_;
  }

  // default processing of unknown messages
  static int _fallback([[maybe_unused]] cell msg, [[maybe_unused]] slice msg_body) {
    return 0;
  }
  // default processing of empty messages
  static int _receive([[maybe_unused]] cell msg, [[maybe_unused]] slice msg_body) {
    return 0;
  }
  // default processing of bounced messages
  static int _on_bounced([[maybe_unused]] cell msg, [[maybe_unused]] slice msg_body) {
    return 0;
  }
};

DEFINE_JSON_ABI(IUserDataConfig, DUserDataConfig, EUserDataConfig);

// ----------------------------- Main entry functions ---------------------- //
MAIN_ENTRY_FUNCTIONS_NO_REPLAY(UserDataConfig, IUserDataConfig, DUserDataConfig)
