/** \file
 *  \brief WrapperEver contract implementation
 *  \author Andrew Zhogin, Vasily Selivanov
 *  \copyright 2019-2022 (c) EverFlex Inc
 */

#include "WrapperEver.hpp"
#include "WIC.hpp"
#include "TONTokenWallet.hpp"
#include "calc_wrapper_reserve_wallet.hpp"

#include <tvm/contract.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/default_support_functions.hpp>
#include <tvm/suffixes.hpp>

#ifndef TIP3_WALLET_CODE_HASH
#error "Macros TIP3_WALLET_CODE_HASH must be defined (code hash of FlexWallet)"
#endif

#ifndef TIP3_WALLET_CODE_DEPTH
#error "Macros TIP3_WALLET_CODE_DEPTH must be defined (code depth of FlexWallet)"
#endif

using namespace tvm;

class WrapperEver final : public smart_interface<IWrapperEver>, public DWrapper {
public:
  DEFAULT_SUPPORT_FUNCTIONS(IWrapperEver, void)

  using data = DWrapper;
  static constexpr bool _checked_deploy = true; /// Deploy is only allowed with [[deploy]] function call
  static constexpr unsigned internal_wallet_hash       = TIP3_WALLET_CODE_HASH;
  static constexpr unsigned internal_wallet_code_depth = TIP3_WALLET_CODE_DEPTH;

  struct error_code : tvm::error_code {
    static constexpr unsigned message_sender_is_not_my_owner    = 100; ///< Authorization error
    static constexpr unsigned wrong_bounced_header              = 101; ///< Wrong header of bounced message
    static constexpr unsigned wrong_bounced_args                = 102; ///< Wrong arguments in bounced message
    static constexpr unsigned wrong_wallet_code_hash            = 103; ///< Wallet code hash is differ from TIP3_WALLET_CODE_HASH macros
    static constexpr unsigned cant_override_wallet_code         = 104; ///< Wallet code is already set and can't be overriden
    static constexpr unsigned burn_unallocated                  = 105; ///< Burn more tokens that was allocated
    static constexpr unsigned message_sender_is_not_good_wallet = 106; ///< Message sender is not a good wallet
    static constexpr unsigned null_address                      = 107; ///< Address mustn't be empty
    static constexpr unsigned funds_mismatched                  = 108; ///< The funds does not correspond to the stated
    static constexpr unsigned uninitialized                     = 109; ///< Uninitialized
    static constexpr unsigned wrong_wic_sender                  = 110; ///< Sender is not recognized as a correct WIC
  };

  bool init(uint128 reserve_wallet_evers, cell internal_wallet_code) {
    require(!internal_wallet_code_, error_code::cant_override_wallet_code);
    require(__builtin_tvm_hashcu(internal_wallet_code) == internal_wallet_hash,
            error_code::wrong_wallet_code_hash);

    super_root_ = parse<WrapperSalt>(parser(tvm_code_salt())).super_root;
    internal_wallet_code_ = internal_wallet_code;

    tvm_rawreserve(start_balance_.get(), rawreserve_flag::up_to);

    // Deploying reserve wallet
    auto [wallet_init, dest] = calc_internal_wallet_init(0u256, tvm_myaddr());
    ITONTokenWalletPtr dest_handle(dest);
    dest_handle.deploy_noop(wallet_init, Evers(reserve_wallet_evers.get()));

    set_int_return_flag(SEND_ALL_GAS);
    return true;
  }

  address deployEmptyWallet(
    uint256     pubkey,
    address_opt internal_owner,
    uint128     evers
  ) {
    // This protects from spending root balance to deploy message
    auto value = int_value();
    tvm_rawreserve(tvm_balance() - value(), rawreserve_flag::up_to);

    auto [wallet_init, dest] = calc_internal_wallet_init(pubkey, internal_owner);
    ITONTokenWalletPtr dest_handle(dest);
    dest_handle.deploy_noop(wallet_init, Evers(evers.get()));

    // sending all rest gas except reserved old balance, processing and deployment costs
    set_int_return_flag(SEND_ALL_GAS);
    return dest;
  }

  // Notification about incoming evers from user wallet
  void onEverTransfer(
    uint128              tokens, // nanotokens
    FlexDeployWalletArgs args
  ) {
    auto value = int_value();
    require(value() >= tokens + args.evers, error_code::funds_mismatched);

    tvm_rawreserve(start_balance_.get() + total_granted_.get() + tokens.get(), rawreserve_flag::up_to);

    auto [wallet_init, dest] = calc_internal_wallet_init(args.pubkey, args.owner);
    ITONTokenWalletPtr dest_handle(dest);
    dest_handle.deploy(wallet_init, 0_ev, SEND_ALL_GAS).acceptMint(tokens, int_sender(), args.keep_evers, builder().endc());
    total_granted_ += tokens;
  }

  void burn(
    uint128     tokens, // nanotokens
    address     answer_addr,
    uint256     sender_pubkey,
    address_opt sender_owner,
    [[maybe_unused]] uint256 out_pubkey,
    address_opt out_owner,
    opt<cell>   notify
  ) {
    require(out_owner.has_value(), error_code::null_address);
    require(total_granted_ >= tokens, error_code::burn_unallocated);
    require(tvm_balance() > tokens, error_code::burn_unallocated);
    auto [sender, value] = int_sender_and_value();
    require(
      sender == expected_internal_address(sender_pubkey, sender_owner),
      error_code::message_sender_is_not_good_wallet
    );
    tvm_rawreserve(start_balance_.get() + total_granted_.get() - tokens.get(), rawreserve_flag::up_to);
    // non-empty notify for WrapperEver is a special case - used for transfer funds to an upgraded WrapperEver
    // out_owner in such case must be the new WrapperEver
    if (notify) {
      auto args = parse_chain_static<EverReTransferArgs>(parser(notify->ctos()));
      IWrapperEverPtr(*out_owner)(0_ev, SEND_ALL_GAS).
        onEverTransfer(tokens, FlexDeployWalletArgs{sender_pubkey, sender_owner, args.wallet_deploy_evers, args.wallet_keep_evers});
    } else {
      tvm_transfer(*out_owner, tokens.get(), true, SEND_ALL_GAS);
    }
    total_granted_ -= tokens;
  }

  void transferFromReserveWallet(
    address_opt answer_addr,
    address     to,
    uint128     tokens
  ) {
    check_owner();
    auto answer_addr_v = answer_addr ? *answer_addr : int_sender();
    ITONTokenWalletPtr reserve_wallet(getReserveWallet());
    tvm_rawreserve(tvm_balance() - int_value().get(), rawreserve_flag::up_to);
    reserve_wallet(0_ev, SEND_ALL_GAS).
      transfer(answer_addr_v, to, tokens, 0u128, 0u128, {});
  }

  uint128 requestTotalGranted() {
    return _remaining_ev() & total_granted_;
  }

  std::pair<address_opt, uint256> cloned() {
    return _remaining_ev() & std::pair<address_opt, uint256>(cloned_, cloned_pubkey_);
  }

  void setCloned(
    address_opt cloned,
    uint256 cloned_pubkey,
    address wrappers_cfg,
    uint256 wrappers_cfg_code_hash,
    uint16  wrappers_cfg_code_depth
  ) {
    require(!!super_root_, error_code::uninitialized);
    cell wic_salted_code = tvm_add_code_salt<WICSalt>(
      {*super_root_, wrappers_cfg, wrappers_cfg_code_hash, wrappers_cfg_code_depth}, wic_unsalted_code_);
    auto [init, hash] = prepare<IWIC>(DWIC{.symbol_ = symbol_}, wic_salted_code);
    require(int_sender() == address::make_std(workchain_id_, hash), error_code::wrong_wic_sender);
    cloned_ = cloned;
    cloned_pubkey_ = cloned_pubkey;
    return _remaining_ev().with_void();
  }

  // getters
  wrapper_details_info getDetails() {
    require(internal_wallet_code_ && super_root_, error_code::uninitialized);
    return { name_, symbol_, decimals_,
             root_pubkey_, total_granted_, internal_wallet_code_.get(),
             {}, getReserveWallet(), *super_root_, cloned_, 1u8 };
  }

  Tip3Config getTip3Config() {
    return { name_, symbol_, decimals_, root_pubkey_, tvm_myaddr() };
  }

  bool hasInternalWalletCode() {
    return internal_wallet_code_;
  }

  address getWalletAddress(uint256 pubkey, address_opt owner) {
    return expected_internal_address(pubkey, owner);
  }

  address getReserveWallet() {
    return calc_wrapper_reserve_wallet({name_, symbol_, decimals_, root_pubkey_, address{tvm_myaddr()}});
  }

  // received bounced message back
  __attribute__((noinline))
  static int _on_bounced([[maybe_unused]] cell msg, slice msg_body) {
    tvm_accept();

    using Args = args_struct_t<&ITONTokenWallet::acceptMint>;
    parser p(msg_body);
    require(p.ldi(32) == -1, error_code::wrong_bounced_header);
    auto [opt_hdr, =p] = parse_continue<abiv1::internal_msg_header>(p);
    require(opt_hdr && opt_hdr->function_id == id_v<&ITONTokenWallet::acceptMint>,
            error_code::wrong_bounced_header);
    auto args = parse<Args>(p, error_code::wrong_bounced_args);
    auto bounced_val = args._value;

    auto [hdr, persist] = load_persistent_data<IWrapperEver, void, DWrapper>();
    require(bounced_val <= persist.total_granted_, error_code::wrong_bounced_args);
    persist.total_granted_ -= bounced_val;
    save_persistent_data<IWrapperEver, void>(hdr, persist);
    return 0;
  }

  uint256 getInternalWalletCodeHash() {
    return uint256{__builtin_tvm_hashcu(internal_wallet_code_.get())};
  }

  // default processing of unknown messages
  static int _fallback(cell /*msg*/, slice /*msg_body*/) {
    return 0;
  }
private:
  address expected_internal_address(uint256 sender_pubkey, address_opt sender_owner) {
    auto hash = calc_int_wallet_init_hash(
      { name_, symbol_, decimals_, root_pubkey_, tvm_myaddr() },
      sender_pubkey, sender_owner,
      uint256(internal_wallet_hash), uint16(internal_wallet_code_depth),
      workchain_id_
    );
    return address::make_std(workchain_id_, hash);
  }

  std::pair<StateInit, address> calc_internal_wallet_init(uint256 pubkey,
                                                          address_opt owner_addr) {
    auto [wallet_init, dest_addr] =
      prepare_internal_wallet_state_init_and_addr(
        name_, symbol_, decimals_,
        root_pubkey_, address{tvm_myaddr()},
        pubkey, owner_addr,
        uint256(internal_wallet_hash), uint16(internal_wallet_code_depth),
        workchain_id_, internal_wallet_code_.get());
    address dest = address::make_std(workchain_id_, dest_addr);
    return { wallet_init, dest };
  }

  void check_owner() {
    require(!!super_root_, error_code::uninitialized);
    require(*super_root_ == int_sender(), error_code::message_sender_is_not_my_owner);
  }
};

DEFINE_JSON_ABI(IWrapperEver, DWrapper, EWrapper);

// ----------------------------- Main entry functions ---------------------- //
MAIN_ENTRY_FUNCTIONS_NO_REPLAY(WrapperEver, IWrapperEver, DWrapper)
