/** \file
 *  \brief Wrapper contract implementation
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#include "Wrapper.hpp"
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
  using data = DWrapper;
  static constexpr bool _checked_deploy = true; /// Deploy is only allowed with [[deploy]] function call
  static constexpr unsigned internal_wallet_hash       = TIP3_WALLET_CODE_HASH;
  static constexpr unsigned internal_wallet_code_depth = TIP3_WALLET_CODE_DEPTH;

  struct error_code : tvm::error_code {
    static constexpr unsigned message_sender_is_not_my_owner    = 100; ///< Authorization error
    static constexpr unsigned wrong_bounced_header              = 101; ///< Wrong header of bounced message
    static constexpr unsigned wrong_bounced_args                = 102; ///< Wrong arguments in bounced message
    static constexpr unsigned wrong_wallet_code_hash            = 105; ///< Wallet code hash is differ from TIP3_WALLET_CODE_HASH macros
    static constexpr unsigned cant_override_wallet_code         = 106; ///< Wallet code is already set and can't be overriden
    static constexpr unsigned not_my_wallet_notifies            = 107; ///< Wallet notification received from wrong address
    static constexpr unsigned burn_unallocated                  = 108; ///< Burn more tokens that was allocated
    static constexpr unsigned message_sender_is_not_good_wallet = 109; ///< Message sender is not a good wallet
    static constexpr unsigned cant_override_external_wallet     = 110; ///< Can't override external wallet
    static constexpr unsigned only_flex_may_deploy_me           = 111; ///< Wrapper may only be deployed by Flex contract
    static constexpr unsigned unexpected_refs_count_in_code     = 112; ///< Unexpected references count in code
  };

  __always_inline
  StateInit getStateInit(const message<anyval> &msg) {
    if (msg.init->isa<ref<StateInit>>()) {
      return msg.init->get<ref<StateInit>>()();
    } else {
      return msg.init->get<StateInit>();
    }
  }

  __always_inline
  bool init(address external_wallet, uint128 reserve_wallet_evers, cell internal_wallet_code) {
    require(!wallet_, error_code::cant_override_external_wallet);
    require(!internal_wallet_code_, error_code::cant_override_wallet_code);
    require(__builtin_tvm_hashcu(internal_wallet_code) == internal_wallet_hash,
            error_code::wrong_wallet_code_hash);

    auto parsed_msg = parse<message<anyval>>(parser(msg_slice()), error_code::bad_incoming_msg);
    require(!!parsed_msg.init, error_code::bad_incoming_msg);
    auto init = getStateInit(parsed_msg);
    require(!!init.code, error_code::bad_incoming_msg);
    auto mycode = *init.code;
    require(mycode.ctos().srefs() == 3, error_code::unexpected_refs_count_in_code);
    parser mycode_parser(mycode.ctos());
    mycode_parser.ldref();
    mycode_parser.ldref();
    auto mycode_salt = mycode_parser.ldrefrtos();
    auto flex_addr = parse<address>(mycode_salt);
    require(flex_addr == int_sender(), error_code::only_flex_may_deploy_me);

    flex_ = flex_addr;
    wallet_ = external_wallet;
    internal_wallet_code_ = internal_wallet_code;

    tvm_rawreserve(start_balance_.get(), rawreserve_flag::up_to);

    // Deploying reserve wallet
    auto [wallet_init, dest] = calc_internal_wallet_init(0u256, address{tvm_myaddr()});
    ITONTokenWalletPtr dest_handle(dest);
    dest_handle.deploy_noop(wallet_init, Evers(reserve_wallet_evers.get()));

    set_int_return_flag(SEND_ALL_GAS);
    return true;
  }

  __always_inline
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
  __always_inline
  WrapperRet onTip3Transfer(
    uint128     balance,
    uint128     new_tokens,
    uint256     sender_pubkey,
    address_opt sender_owner,
    cell        payload,
    address     answer_addr
  ) {
    require(int_sender() == wallet_->get(), error_code::not_my_wallet_notifies);

    // to send answer to the original caller (caller->tip3wallet->wrapper->caller)
    set_int_sender(answer_addr);
    set_int_return_value(0);
    set_int_return_flag(SEND_ALL_GAS);

    auto args = parse<FlexDeployWalletArgs>(payload.ctos());

    auto value = int_value();
    tvm_rawreserve(tvm_balance() - value(), rawreserve_flag::up_to);

    auto [wallet_init, dest] = calc_internal_wallet_init(args.pubkey, args.owner);
    ITONTokenWalletPtr dest_handle(dest);
    dest_handle.deploy(wallet_init, Evers(args.evers.get())).acceptMint(new_tokens, int_sender(), args.evers);
    total_granted_ += new_tokens;

    return { uint32(0), dest_handle.get() };
  }

  __always_inline
  void burn(
    uint128     tokens,
    address     answer_addr,
    uint256     sender_pubkey,
    address_opt sender_owner,
    uint256     out_pubkey,
    address_opt out_owner
  ) {
    require(total_granted_ >= tokens, error_code::burn_unallocated);
    auto [sender, value_gr] = int_sender_and_value();
    require(sender == expected_internal_address(sender_pubkey, sender_owner),
            error_code::message_sender_is_not_good_wallet);
    tvm_rawreserve(tvm_balance() - value_gr(), rawreserve_flag::up_to);
    (*wallet_)(0_ev, SEND_ALL_GAS).
      transferToRecipient(answer_addr, {out_pubkey, out_owner}, tokens, 0u128, true, 0u128, {});
    total_granted_ -= tokens;
  }

  __always_inline
  void transferFromReserveWallet(
    address_opt answer_addr, ///< Answer address (where to return unspent native evers).
    address     to,          ///< Destination tip3 wallet address.
    uint128     tokens       ///< Amount of tokens (balance of flex wallet on burn).
  ) {
    check_owner();
    auto answer_addr_v = answer_addr ? *answer_addr : int_sender();
    ITONTokenWalletPtr reserve_wallet(getReserveWallet());
    tvm_rawreserve(tvm_balance() - int_value().get(), rawreserve_flag::up_to);
    reserve_wallet(0_ev, SEND_ALL_GAS).
      transfer(answer_addr_v, to, tokens, 0u128, 0u128, {});
  }

  __always_inline
  uint128 requestTotalGranted() {
    auto value = int_value();
    tvm_rawreserve(tvm_balance() - value(), rawreserve_flag::up_to);
    set_int_return_flag(SEND_ALL_GAS);
    return total_granted_;
  }

  // getters
  __always_inline
  wrapper_details_info getDetails() {
    return { getName(), getSymbol(), getDecimals(),
             getRootKey(), getTotalGranted(), getInternalWalletCode(),
             getExternalWallet(), getReserveWallet(), getFlex() };
  }

  __always_inline string getName() {
    return name_;
  }

  __always_inline string getSymbol() {
    return symbol_;
  }

  __always_inline uint8 getDecimals() {
    return decimals_;
  }

  __always_inline uint256 getRootKey() {
    return root_pubkey_;
  }

  __always_inline uint128 getTotalGranted() {
    return total_granted_;
  }

  __always_inline bool hasInternalWalletCode() {
    return internal_wallet_code_;
  }

  __always_inline cell getInternalWalletCode() {
    return internal_wallet_code_.get();
  }

  __always_inline address getExternalWallet() {
    return wallet_->get();
  }

  __always_inline address getFlex() {
    return *flex_;
  }

  __always_inline
  address getWalletAddress(uint256 pubkey, address_opt owner) {
    return expected_internal_address(pubkey, owner);
  }

  __always_inline
  address getReserveWallet() {
    return calc_wrapper_reserve_wallet({name_, symbol_, decimals_, root_pubkey_, address{tvm_myaddr()}});
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
    auto bounced_val = args.tokens;

    auto [hdr, persist] = load_persistent_data<IWrapper, void, DWrapper>();
    require(bounced_val <= persist.total_granted_, error_code::wrong_bounced_args);
    persist.total_granted_ -= bounced_val;
    save_persistent_data<IWrapper, void>(hdr, persist);
    return 0;
  }

  __always_inline
  uint256 getInternalWalletCodeHash() {
    return uint256{__builtin_tvm_hashcu(internal_wallet_code_.get())};
  }

  // default processing of unknown messages
  __always_inline static int _fallback(cell /*msg*/, slice /*msg_body*/) {
    return 0;
  }

  // =============== Support functions ==================
  DEFAULT_SUPPORT_FUNCTIONS(IWrapper, void)
private:
  __always_inline
  address expected_internal_address(uint256 sender_pubkey, address_opt sender_owner) {
    auto hash = calc_int_wallet_init_hash(
      name_, symbol_, decimals_,
      root_pubkey_, address{tvm_myaddr()},
      sender_pubkey, sender_owner,
      uint256(internal_wallet_hash), uint16(internal_wallet_code_depth),
      workchain_id_
    );
    return address::make_std(workchain_id_, hash);
  }

  __always_inline
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

  __always_inline
  void check_owner() {
    require(flex_ == int_sender(), error_code::message_sender_is_not_my_owner);
  }
};

DEFINE_JSON_ABI(IWrapper, DWrapper, EWrapper);

// ----------------------------- Main entry functions ---------------------- //
MAIN_ENTRY_FUNCTIONS_NO_REPLAY(Wrapper, IWrapper, DWrapper)

