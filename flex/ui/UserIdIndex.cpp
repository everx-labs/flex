/** \file
 *  \brief UserIdIndex contract implementation
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#include "UserIdIndex.hpp"
#include "AuthIndex.hpp"
#include <tvm/contract.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/default_support_functions.hpp>
#include <tvm/suffixes.hpp>
#include <tvm/schema/parse_chain_static.hpp>
#include <tvm/schema/build_chain_static.hpp>

using namespace tvm;

/// UserIdIndex contract implementation
class UserIdIndex final : public smart_interface<IUserIdIndex>, public DUserIdIndex {
  using data = DUserIdIndex;
public:
  DEFAULT_SUPPORT_FUNCTIONS(IUserIdIndex, userid_replay_protection_t)

  //static constexpr bool _checked_deploy = true; /// To allow deploy message only with `onDeploy` call
  struct error_code : tvm::error_code {
    static constexpr unsigned message_sender_is_not_my_owner = 100; ///< Authorization error
    static constexpr unsigned double_deploy                  = 101; ///< Double deploy
    static constexpr unsigned only_client_may_deploy_me      = 102; ///< Only FlexClient may deploy this contract
  };

  /// Check that msg.sender is an owner (from configuration salt). Deploy AuthIndex with lend_pubkey.
  void onDeploy(uint256 lend_pubkey, string name, uint128 evers_to_auth_idx, uint128 refill_wallet, uint128 min_refill) {
    auto cfg = getConfig();
    require(int_sender() == cfg.owner, error_code::only_client_may_deploy_me);
    require(!name_, error_code::double_deploy);
    lend_pubkey_ = lend_pubkey;
    name_ = name;
    refill_wallet_ = refill_wallet;
    min_refill_ = min_refill;
    workchain_id_ = std::get<addr_std>(tvm_myaddr().val()).workchain_id;

    auto [init, std_addr] = prepare<IAuthIndex>(DAuthIndex{.pubkey_ = lend_pubkey_}, cfg.auth_index_code);
    IAuthIndexPtr auth_ptr(address::make_std(workchain_id_, std_addr));
    auth_ptr.deploy(init, Evers(evers_to_auth_idx.get())).onDeploy();
  }

  /// Remove AuthIndex with lend_pubkey. Deploy AuthIndex with new_lend_pubkey. Set lend_pubkey = new_lend_pubkey.
  void reLendPubkey(uint256 new_lend_pubkey, uint128 evers_to_remove, uint128 evers_to_deploy) {
    auto cfg = getConfig();
    require(int_sender() == cfg.owner, error_code::message_sender_is_not_my_owner);
    getAuthIndex()(Evers(evers_to_remove.get())).remove(int_sender());
    lend_pubkey_ = new_lend_pubkey;

    auto [init, std_addr] = prepare<IAuthIndex>(DAuthIndex{.pubkey_ = new_lend_pubkey}, cfg.auth_index_code);
    IAuthIndexPtr auth(address::make_std(workchain_id_, std_addr));
    auth.deploy(init, Evers(evers_to_deploy.get())).onDeploy();
  }

  /// Remove AuthIndex with lend_pubkey.
  void remove() {
    require(int_sender() == getConfig().owner, error_code::message_sender_is_not_my_owner);
    tvm_rawreserve(tvm_balance() - int_value().get(), rawreserve_flag::up_to);
    getAuthIndex()(0_ev, SEND_ALL_GAS).remove(int_sender());
  }

  uint256 requestLendPubkey(uint128 evers_balance) {
    require(int_sender() == getConfig().owner, error_code::message_sender_is_not_my_owner);
    int refill_val = static_cast<int>(refill_wallet_.get()) - static_cast<int>(evers_balance.get());
    refill_val = std::max(static_cast<int>(min_refill_.get()), refill_val);
    return _fixed_ev(uint128(static_cast<unsigned>(refill_val))) & lend_pubkey_;
  }

  void transfer(address dest, uint128 value, bool bounce) {
    require(msg_pubkey() == lend_pubkey_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    tvm_transfer(dest, value.get(), bounce);
  }

  void setRefillWallet(uint128 refill_wallet, uint128 min_refill) {
    require(msg_pubkey() == lend_pubkey_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    refill_wallet_ = refill_wallet;
    min_refill_ = min_refill;
  }

  UserIdIndexSalt getConfig() {
    return parse_chain_static<UserIdIndexSalt>(parser(tvm_code_salt()));
  }

  IAuthIndexPtr getAuthIndex() {
    auto std_addr = prepare<IAuthIndex>(DAuthIndex{.pubkey_ = lend_pubkey_}, getConfig().auth_index_code).second;
    return IAuthIndexPtr(address::make_std(workchain_id_, std_addr));
  }

  // default processing of unknown messages
  static int _fallback([[maybe_unused]] cell msg, [[maybe_unused]] slice msg_body) {
    return 0;
  }
  // default processing of empty messages
  static int _receive([[maybe_unused]] cell msg, [[maybe_unused]] slice msg_body) {
    return 0;
  }
};

DEFINE_JSON_ABI(IUserIdIndex, DUserIdIndex, EUserIdIndex, userid_replay_protection_t);

// ----------------------------- Main entry functions ---------------------- //
DEFAULT_MAIN_ENTRY_FUNCTIONS(UserIdIndex, IUserIdIndex, DUserIdIndex, USERID_TIMESTAMP_DELAY)
