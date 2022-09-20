/** \file
 *  \brief WrapperDeployerBroxus contract implementation for Broxus tip3 tokens
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#include "WrapperDeployerBroxus.hpp"
#include "WrapperBroxus.hpp"
#include "WrapperDeployerBroxusArgs.hpp"
#include "broxus/ITokenWallet.hpp"
#include "broxus/ITokenRoot.hpp"

#include <tvm/contract.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/default_support_functions.hpp>
#include <tvm/suffixes.hpp>

using namespace tvm;

class WrapperDeployerBroxus final : public smart_interface<IWrapperDeployerBroxus>, public DWrapperDeployerBroxus {
public:
  using data = DWrapperDeployerBroxus;

  static constexpr unsigned TIMESTAMP_DELAY = 1800;
  using replay_protection_t = replay_attack_protection::timestamp<TIMESTAMP_DELAY>;

  DEFAULT_SUPPORT_FUNCTIONS(IWrapperDeployerBroxus, replay_protection_t)

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
    uint128 ext_wallet_value,
    uint128 wrapper_deploy_value,
    uint128 wrapper_keep_balance,
    uint128 reserve_wallet_value,
    uint128 out_deploy_value
  ) {
    tvm_accept();
    pubkey_         = pubkey;
    wrapper_pubkey_ = wrapper_pubkey;
    super_root_     = super_root;
    ext_wallet_value_     = ext_wallet_value;
    wrapper_deploy_value_ = wrapper_deploy_value;
    wrapper_keep_balance_ = wrapper_keep_balance;
    reserve_wallet_value_ = reserve_wallet_value;
    out_deploy_value_     = out_deploy_value;
  }

  void setWrapperCode(cell code) {
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

  // TODO: fix answer_id automatic store in the execution context
  static __attribute__((noinline))
  uint32 save_answer_id(uint32 answer_id) {
    return answer_id;
  }

  resumable<std::pair<address, uint256>> deploy(cell cl, cell wic_unsalted_code) {
    require(wrapper_code_ && flex_wallet_code_, error_code::uninitialized);

    require(!!return_func_id(), error_code::iterator_overflow);
    auto answer_id = save_answer_id(*return_func_id());

    auto args = parse_chain_static<WrapperDeployerBroxusArgs>(parser(cl.ctos()));
    auto tip3cfg = args.tip3cfg;

    require(int_value().get() > ext_wallet_value_ + wrapper_deploy_value_, error_code::not_enough_evers);
    auto workchain_id = std::get<addr_std>(tvm_myaddr().val()).workchain_id;

    DWrapperBroxus wrapper_data {
      .wic_unsalted_code_ = wic_unsalted_code,
      .name_ = tip3cfg.name,
      .symbol_ = tip3cfg.symbol,
      .decimals_ = tip3cfg.decimals,
      .workchain_id_ = workchain_id,
      .root_pubkey_ = wrapper_pubkey_,
      .total_granted_ = {},
      .internal_wallet_code_ = {},
      .start_balance_ = Evers(wrapper_keep_balance_.get()),
      .super_root_ = super_root_,
      .wallet_ = {},
      .out_deploy_value_ = out_deploy_value_
    };
    auto [wrapper_init, wrapper_hash_addr] = prepare<IWrapperBroxus>(wrapper_data, wrapper_code_.get());
    IWrapperBroxusPtr wrapper_addr(address::make_std(workchain_id, wrapper_hash_addr));
    uint128 old_balance(tvm_balance() - int_value().get());

    // ============= Deploying external wallet for Flex wrapper ============ //
    address wallet_addr = co_await broxus::ITokenRootPtr(tip3cfg.root_address)(_all_except(old_balance)).
      deployWallet(wrapper_addr.get(), ext_wallet_value_);

    // ================== Deploying Flex wrapper ================== //
    wrapper_addr.deploy(wrapper_init, Evers(wrapper_deploy_value_.get()))
      .init(wallet_addr, reserve_wallet_value_, flex_wallet_code_.get());

    // TODO: we need to restore answer_id manually here
    set_return_func_id(answer_id);

    co_return _all_except(old_balance) & std::make_pair(wrapper_addr.get(), wrapper_pubkey_);
  }

  cell getArgs(Tip3Config tip3cfg) {
    return build_chain_static(tip3cfg);
  }

  // default processing of unknown messages
  static int _fallback([[maybe_unused]] cell msg, [[maybe_unused]] slice msg_body) {
    return 0;
  }
};

DEFINE_JSON_ABI(IWrapperDeployerBroxus, DWrapperDeployerBroxus, EWrapperDeployerBroxus);

// ----------------------------- Main entry functions ---------------------- //
DEFAULT_MAIN_ENTRY_FUNCTIONS(WrapperDeployerBroxus, IWrapperDeployerBroxus, DWrapperDeployerBroxus, WrapperDeployerBroxus::TIMESTAMP_DELAY)

