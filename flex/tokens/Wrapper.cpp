/** \file
 *  \brief Wrapper contract implementation
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#include "Wrapper.hpp"
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

class Wrapper final : public smart_interface<IWrapper>, public DWrapper {
public:
  DEFAULT_SUPPORT_FUNCTIONS(IWrapper, void)

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
    static constexpr unsigned not_my_wallet_notifies            = 105; ///< Wallet notification received from wrong address
    static constexpr unsigned burn_unallocated                  = 106; ///< Burn more tokens that was allocated
    static constexpr unsigned message_sender_is_not_good_wallet = 107; ///< Message sender is not a good wallet
    static constexpr unsigned cant_override_external_wallet     = 108; ///< Can't override external wallet
    static constexpr unsigned uninitialized                     = 109; ///< Uninitialized
    static constexpr unsigned wrong_wic_sender                  = 110; ///< Sender is not recognized as a correct WIC
  };

  void init(
    address external_wallet,
    uint128 reserve_wallet_evers,
    cell    internal_wallet_code
  ) {
    require(!wallet_, error_code::cant_override_external_wallet);
    require(!internal_wallet_code_, error_code::cant_override_wallet_code);
    require(__builtin_tvm_hashcu(internal_wallet_code) == internal_wallet_hash,
            error_code::wrong_wallet_code_hash);

    super_root_ = parse<WrapperSalt>(parser(tvm_code_salt())).super_root;
    wallet_ = external_wallet;
    internal_wallet_code_ = internal_wallet_code;

    // Deploying reserve wallet
    auto [wallet_init, dest] = calc_internal_wallet_init(0u256, tvm_myaddr());
    ITONTokenWalletPtr dest_handle(dest);
    dest_handle.deploy_noop(wallet_init, Evers(reserve_wallet_evers.get()));

    return _all_except(start_balance_).with_void();
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

  // Notification about incoming tokens from Wrapper owned external wallet
  WrapperRet onTip3Transfer(
    uint128        balance,
    uint128        new_tokens,
    uint128        evers_balance,
    Tip3Config     tip3cfg,
    opt<Tip3Creds> sender,
    Tip3Creds      receiver,
    cell           payload,
    address        answer_addr
  ) {
    require(wallet_ && int_sender() == *wallet_, error_code::not_my_wallet_notifies);

    // to send answer to the original caller (caller->tip3wallet->wrapper->caller)
    set_int_sender(answer_addr);
    set_int_return_value(0);
    set_int_return_flag(SEND_ALL_GAS);

    auto args = parse<FlexDeployWalletArgs>(payload.ctos());

    auto value = int_value();
    tvm_rawreserve(tvm_balance() - value(), rawreserve_flag::up_to);

    auto [wallet_init, dest] = calc_internal_wallet_init(args.pubkey, args.owner);
    ITONTokenWalletPtr dest_handle(dest);
    dest_handle.deploy(wallet_init, Evers(args.evers.get())).acceptMint(new_tokens, int_sender(), args.keep_evers, payload);
    total_granted_ += new_tokens;

    return { uint32(0), dest_handle.get() };
  }

  void burn(
    uint128     tokens,
    address     answer_addr,
    uint256     sender_pubkey,
    address_opt sender_owner,
    uint256     out_pubkey,
    address_opt out_owner,
    opt<cell>   notify
  ) {
    require(!!wallet_, error_code::uninitialized);
    require(total_granted_ >= tokens, error_code::burn_unallocated);
    auto [sender, value_gr] = int_sender_and_value();
    require(sender == expected_internal_address(sender_pubkey, sender_owner),
            error_code::message_sender_is_not_good_wallet);
    ITONTokenWalletPtr(*wallet_)(_remaining_ev()).
      transferToRecipient(answer_addr, {out_pubkey, out_owner}, tokens, 0u128, 0u128, true, 0u128, notify);
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
    reserve_wallet(_remaining_ev()).
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
    require(super_root_ && internal_wallet_code_ && wallet_, error_code::uninitialized);
    return { name_, symbol_, decimals_,
             root_pubkey_, total_granted_, internal_wallet_code_.get(),
             *wallet_, getReserveWallet(), *super_root_, cloned_, 0u8 };
  }

  Tip3Config getTip3Config() {
    return { name_, symbol_, decimals_, root_pubkey_, tvm_myaddr() };
  }

  bool hasInternalWalletCode() { return internal_wallet_code_; }

  address getWalletAddress(uint256 pubkey, address_opt owner) {
    return expected_internal_address(pubkey, owner);
  }

  address getReserveWallet() {
    return calc_wrapper_reserve_wallet({name_, symbol_, decimals_, root_pubkey_, tvm_myaddr()});
  }

  // received bounced message back
  __always_inline static int _on_bounced(cell /*msg*/, slice msg_body) {
    tvm_accept();

    using Args = args_struct_t<&ITONTokenWallet::acceptMint>;
    parser p(msg_body);
    require(p.ldi(32) == -1, error_code::wrong_bounced_header);
    auto [opt_hdr, =p] = parse_continue<abiv1::internal_msg_header>(p);
    require(opt_hdr && opt_hdr->function_id == id_v<&ITONTokenWallet::acceptMint>,
            error_code::wrong_bounced_header);
    auto args = parse<Args>(p, error_code::wrong_bounced_args);
    auto bounced_val = args._value;

    auto [hdr, persist] = load_persistent_data<IWrapper, void, DWrapper>();
    require(bounced_val <= persist.total_granted_, error_code::wrong_bounced_args);
    persist.total_granted_ -= bounced_val;
    save_persistent_data<IWrapper, void>(hdr, persist);
    return 0;
  }

  uint256 getInternalWalletCodeHash() {
    return uint256{__builtin_tvm_hashcu(internal_wallet_code_.get())};
  }

  // default processing of unknown messages
  static int _fallback([[maybe_unused]] cell msg, [[maybe_unused]] slice msg_body) {
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
        root_pubkey_, tvm_myaddr(),
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

DEFINE_JSON_ABI(IWrapper, DWrapper, EWrapper);

// ----------------------------- Main entry functions ---------------------- //
MAIN_ENTRY_FUNCTIONS_NO_REPLAY(Wrapper, IWrapper, DWrapper)

