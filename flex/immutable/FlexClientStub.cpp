/** \file
 *  \brief FlexClientStub contract implementation
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) EverFlex Inc
 */

#include "FlexClientStub.hpp"

#include <tvm/contract.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/default_support_functions.hpp>
#include <tvm/suffixes.hpp>

using namespace tvm;

/// FlexClientStub contract. Implements IFlexClientStub.
class FlexClientStub final : public smart_interface<IFlexClientStub>, public DFlexClientStub {
  using data = DFlexClientStub;
  static constexpr bool _checked_deploy = true; /// Deploy is only allowed with [[deploy]] function call
public:
  struct error_code : tvm::error_code {
    static constexpr unsigned unsalted_flex_client_stub = 101; ///< Unsalted FlexClientStub code
    static constexpr unsigned double_initialization     = 102; ///< Double initialization
    static constexpr unsigned stub_call_upgrade         = 103; ///< onCodeUpgrade called for stub
  };

  void onDeploy(
    FlexVersion triplet,
    bind_info   binding,
    cell        flex_client_code,
    cell        auth_index_code,
    cell        user_id_index_code,
    bytes       signature
  ) {
    require(tvm_mycode().ctos().srefs() == 3, error_code::unsalted_flex_client_stub);
    require(!auth_index_code_, error_code::double_initialization);

    // self-destruct if signature is incorrect
    if (!__builtin_tvm_chksignu(std::get<addr_std>(tvm_myaddr().val()).address.get(), signature.get().ctos(), owner_.get())) {
      tvm_transfer(int_sender(), 0, false,
        SEND_ALL_GAS | DELETE_ME_IF_I_AM_EMPTY | IGNORE_ACTION_ERRORS | SENDER_WANTS_TO_PAY_FEES_SEPARATELY);
      return;
    }

    triplet_            = triplet;
    binding_            = binding;
    auth_index_code_    = auth_index_code;
    user_id_index_code_ = user_id_index_code;

    cell state = prepare_persistent_data<IFlexClientStub, flex_client_replay_protection_t, data>(header_, static_cast<data&>(*this));
    tvm_setcode(flex_client_code);
    tvm_setcurrentcode(parser(flex_client_code).skipref().ldref());
    onCodeUpgrade(state);
  }

  resumable<void> unused() {
    co_return;
  }

  __attribute__((noinline, noreturn))
  static void onCodeUpgrade([[maybe_unused]] cell state) {
    tvm_throw(error_code::stub_call_upgrade); // Must not be called
  }

  // =============== Support functions ==================
  DEFAULT_SUPPORT_FUNCTIONS(IFlexClientStub, flex_client_replay_protection_t)

  // default processing of unknown messages
  static int _fallback([[maybe_unused]] cell msg, [[maybe_unused]] slice msg_body) {
    return 0;
  }
  // default processing of empty messages
  static int _receive([[maybe_unused]] cell msg, [[maybe_unused]] slice msg_body) {
    return 0;
  }
};

DEFINE_JSON_ABI(IFlexClientStub, DFlexClientStub, EFlexClientStub, flex_client_replay_protection_t);

// ----------------------------- Main entry functions ---------------------- //
DEFAULT_MAIN_ENTRY_FUNCTIONS(FlexClientStub, IFlexClientStub, DFlexClientStub, FLEX_CLIENT_TIMESTAMP_DELAY)
