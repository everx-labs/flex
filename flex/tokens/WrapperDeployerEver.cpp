/** \file
 *  \brief WrapperDeployerEver contract implementation
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#include "WrapperDeployerEver.hpp"
#include "WrapperEver.hpp"
#include "WrapperDeployerEverArgs.hpp"

#include <tvm/contract.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/default_support_functions.hpp>
#include <tvm/suffixes.hpp>

using namespace tvm;

class WrapperDeployerEver final : public smart_interface<IWrapperDeployerEver>, public DWrapperDeployerEver {
public:
  using data = DWrapperDeployerEver;

  static constexpr unsigned TIMESTAMP_DELAY = 1800;
  using replay_protection_t = replay_attack_protection::timestamp<TIMESTAMP_DELAY>;

  struct error_code : tvm::error_code {
    static constexpr unsigned message_sender_is_not_my_owner = 100; ///< Authorization error
    static constexpr unsigned not_enough_evers               = 101; ///< Not enough evers to process
    static constexpr unsigned code_rewrite_not_allowed       = 102; ///< Code rewrite not allowed
    static constexpr unsigned uninitialized                  = 103; ///< Uninitialized
  };

  void constructor(
    uint256 pubkey,
    uint256 wrapper_pubkey,
    address super_root,
    uint128 wrapper_deploy_value,
    uint128 wrapper_keep_balance,
    uint128 reserve_wallet_value
  ) {
    tvm_accept();
    pubkey_         = pubkey;
    wrapper_pubkey_ = wrapper_pubkey;
    super_root_     = super_root;
    wrapper_deploy_value_ = wrapper_deploy_value;
    wrapper_keep_balance_ = wrapper_keep_balance;
    reserve_wallet_value_ = reserve_wallet_value;
  }

  void setWrapperEverCode(cell code) {
    require(msg_pubkey() == pubkey_, error_code::message_sender_is_not_my_owner);
    require(!wrapper_code_, error_code::code_rewrite_not_allowed);
    tvm_accept();
    wrapper_code_ = tvm_add_code_salt<WrapperSalt>({super_root_}, code);
  }

  void setFlexWalletCode(cell code) {
    require(msg_pubkey() == pubkey_, error_code::message_sender_is_not_my_owner);
    require(!flex_wallet_code_, error_code::code_rewrite_not_allowed);
    tvm_accept();
    flex_wallet_code_ = code;
  }

  std::pair<address, uint256> deploy(cell cl, cell wic_unsalted_code) {
    require(wrapper_code_ && flex_wallet_code_, error_code::uninitialized);

    require(int_value().get() > wrapper_deploy_value_, error_code::not_enough_evers);
    auto workchain_id = std::get<addr_std>(tvm_myaddr().val()).workchain_id;

    string ev_name = { 'E', 'V', 'E', 'R' };

    DWrapper wrapper_data {
      .wic_unsalted_code_ = wic_unsalted_code,
      .name_     = ev_name,
      .symbol_   = ev_name,
      .decimals_ = 9u8,
      .workchain_id_ = workchain_id,
      .root_pubkey_  = wrapper_pubkey_,
      .total_granted_ = {},
      .internal_wallet_code_ = {},
      .start_balance_ = Evers(wrapper_keep_balance_.get()),
      .super_root_ = super_root_,
      .wallet_ = {}
    };
    auto [init, hash] = prepare<IWrapperEver>(wrapper_data, wrapper_code_.get());
    IWrapperEverPtr wrapper(address::make_std(workchain_id, hash));

    tvm_rawreserve(tvm_balance() - int_value().get(), rawreserve_flag::up_to);

    // ================== Deploying Flex wrapper ================== //
    wrapper.deploy(init, Evers(wrapper_deploy_value_.get()))
      .init(reserve_wallet_value_, flex_wallet_code_.get());

    set_int_return_flag(SEND_ALL_GAS);
    return { wrapper.get(), wrapper_pubkey_ };
  }

  // default processing of unknown messages
  static int _fallback([[maybe_unused]] cell msg, [[maybe_unused]] slice msg_body) {
    return 0;
  }

  // =============== Support functions ==================
  DEFAULT_SUPPORT_FUNCTIONS(IWrapperDeployerEver, replay_protection_t)
};

DEFINE_JSON_ABI(IWrapperDeployerEver, DWrapperDeployerEver, EWrapperDeployerEver);

// ----------------------------- Main entry functions ---------------------- //
DEFAULT_MAIN_ENTRY_FUNCTIONS(WrapperDeployerEver, IWrapperDeployerEver, DWrapperDeployerEver, WrapperDeployerEver::TIMESTAMP_DELAY)

