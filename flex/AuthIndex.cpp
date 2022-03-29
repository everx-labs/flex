/** \file
 *  \brief AuthIndex contract implementation
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#include "AuthIndex.hpp"
#include <tvm/contract.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/default_support_functions.hpp>
#include <tvm/suffixes.hpp>
#include <tvm/schema/parse_chain_static.hpp>
#include <tvm/schema/build_chain_static.hpp>

using namespace tvm;

/// AuthIndex contract implementation
class AuthIndex final : public smart_interface<IAuthIndex>, public DAuthIndex {
  using data = DAuthIndex;
public:
  static constexpr bool _checked_deploy = true; /// To allow deploy message only with `onDeploy` call
  struct error_code : tvm::error_code {
    static constexpr unsigned message_sender_is_not_my_owner = 100; ///< Authorization error
    static constexpr unsigned double_deploy                  = 101; ///< Double deploy
  };

  __always_inline
  void onDeploy() {
    require(!owner_, error_code::double_deploy);
    owner_ = int_sender();
  }

  __always_inline
  void remove(address dst) {
    require(owner_ && (int_sender() == *owner_), error_code::message_sender_is_not_my_owner);
    suicide(dst);
  }

  __always_inline
  AuthIndexSalt getConfig() {
    return parse_chain_static<AuthIndexSalt>(parser(tvm_code_salt()));
  }

  // default processing of unknown messages
  __always_inline static int _fallback(cell /*msg*/, slice /*msg_body*/) {
    return 0;
  }
  // default processing of empty messages
  __always_inline static int _receive(cell /*msg*/, slice /*msg_body*/) {
    return 0;
  }
  // =============== Support functions ==================
  DEFAULT_SUPPORT_FUNCTIONS(IAuthIndex, void)
};

DEFINE_JSON_ABI(IAuthIndex, DAuthIndex, EAuthIndex);

// ----------------------------- Main entry functions ---------------------- //
MAIN_ENTRY_FUNCTIONS_NO_REPLAY(AuthIndex, IAuthIndex, DAuthIndex)
