/** \file
 *  \brief WrapperDeployerTip3 contract implementation for TON Labs tip3 tokens
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#include "WrapperDeployerTip3.hpp"
#include "Wrapper.hpp"
#include "WrapperDeployerTip3Args.hpp"

#include <tvm/contract.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/default_support_functions.hpp>
#include <tvm/suffixes.hpp>

using namespace tvm;

class WrapperDeployerTip3 final : public smart_interface<IWrapperDeployerTip3>, public DWrapperDeployerTip3 {
public:
  using data = DWrapperDeployerTip3;

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
    uint128 ext_wallet_value,
    uint128 wrapper_deploy_value,
    uint128 wrapper_keep_balance,
    uint128 reserve_wallet_value
  ) {
    tvm_accept();
    pubkey_         = pubkey;
    wrapper_pubkey_ = wrapper_pubkey;
    super_root_     = super_root;
    ext_wallet_value_     = ext_wallet_value;
    wrapper_deploy_value_ = wrapper_deploy_value;
    wrapper_keep_balance_ = wrapper_keep_balance;
    reserve_wallet_value_ = reserve_wallet_value;
  }

  void setWrapperCode(cell code) {
    require(msg_pubkey() == pubkey_, error_code::message_sender_is_not_my_owner);
    require(!wrapper_code_, error_code::code_rewrite_not_allowed);
    tvm_accept();
    wrapper_code_ = tvm_add_code_salt<WrapperSalt>({super_root_}, code);
  }

  void setExtWalletCode(cell code) {
    require(msg_pubkey() == pubkey_, error_code::message_sender_is_not_my_owner);
    require(!ext_wallet_code_, error_code::code_rewrite_not_allowed);
    tvm_accept();
    ext_wallet_code_ = code;
  }

  void setFlexWalletCode(cell code) {
    require(msg_pubkey() == pubkey_, error_code::message_sender_is_not_my_owner);
    require(!flex_wallet_code_, error_code::code_rewrite_not_allowed);
    tvm_accept();
    flex_wallet_code_ = code;
  }

  std::pair<address, uint256> deploy(cell cl, cell wic_unsalted_code) {
    require(wrapper_code_ && ext_wallet_code_ && flex_wallet_code_, error_code::uninitialized);

    auto args = parse_chain_static<WrapperDeployerTip3Args>(parser(cl.ctos()));
    auto tip3cfg = args.tip3cfg;

    require(int_value().get() > ext_wallet_value_ + wrapper_deploy_value_, error_code::not_enough_evers);
    auto workchain_id = std::get<addr_std>(tvm_myaddr().val()).workchain_id;

    DWrapper wrapper_data {
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
      .wallet_ = {}
    };
    auto [wrapper_init, wrapper_hash_addr] = prepare<IWrapper>(wrapper_data, wrapper_code_.get());
    IWrapperPtr wrapper_addr(address::make_std(workchain_id, wrapper_hash_addr));

    tvm_rawreserve(tvm_balance() - int_value().get(), rawreserve_flag::up_to);

    // ============= Deploying external wallet for Flex wrapper ============ //
    auto [wallet_init, wallet_hash_addr] = prepare_external_wallet_state_init_and_addr(
      tip3cfg.name, tip3cfg.symbol, tip3cfg.decimals,
      tip3cfg.root_pubkey, tip3cfg.root_address,
      wrapper_pubkey_, wrapper_addr.get(),
      uint256(tvm_hash(ext_wallet_code_.get())), uint16(ext_wallet_code_.get().cdepth()),
      workchain_id, ext_wallet_code_.get());
    ITONTokenWalletPtr wallet_addr(address::make_std(workchain_id, wallet_hash_addr));
    wallet_addr.deploy_noop(wallet_init, Evers(ext_wallet_value_.get()));

    // ================== Deploying Flex wrapper ================== //
    wrapper_addr.deploy(wrapper_init, Evers(wrapper_deploy_value_.get()))
      .init(wallet_addr.get(), reserve_wallet_value_, flex_wallet_code_.get());

    set_int_return_flag(SEND_ALL_GAS);
    return { wrapper_addr.get(), wrapper_pubkey_ };
  }

  cell getArgs(Tip3Config tip3cfg) {
    return build_chain_static(tip3cfg);
  }

  // default processing of unknown messages
  static int _fallback([[maybe_unused]] cell msg, [[maybe_unused]] slice msg_body) {
    return 0;
  }

  // =============== Support functions ==================
  DEFAULT_SUPPORT_FUNCTIONS(IWrapperDeployerTip3, replay_protection_t)
};

DEFINE_JSON_ABI(IWrapperDeployerTip3, DWrapperDeployerTip3, EWrapperDeployerTip3);

// ----------------------------- Main entry functions ---------------------- //
DEFAULT_MAIN_ENTRY_FUNCTIONS(WrapperDeployerTip3, IWrapperDeployerTip3, DWrapperDeployerTip3, WrapperDeployerTip3::TIMESTAMP_DELAY)

