/** \file
 *  \brief RootTokenContract contract implementation
 *  Compiles into two contract versions: RootTokenContract.tvc (for external wallets) and FlexTokenRoot.tvc (for internal wallets).
 *  With different macroses.
 *  Also, Wrapper contract may be internal wallets root and perform conversion external->internal and back.
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#include "RootTokenContract.hpp"
#include "TONTokenWallet.hpp"

#include <tvm/contract.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/default_support_functions.hpp>
#include <tvm/suffixes.hpp>

#ifndef TIP3_WALLET_CODE_HASH
#error "Macros TIP3_WALLET_CODE_HASH must be defined (code hash of TONTokenWallet)"
#endif

#ifndef TIP3_WALLET_CODE_DEPTH
#error "Macros TIP3_WALLET_CODE_DEPTH must be defined (code depth of TONTokenWallet)"
#endif

using namespace tvm;
using namespace schema;

template<bool Internal>
class RootTokenContract final : public smart_interface<IRootTokenContract>, public DRootTokenContract {
public:
  using data = DRootTokenContract;
  static constexpr unsigned wallet_hash       = TIP3_WALLET_CODE_HASH;
  static constexpr unsigned wallet_code_depth = TIP3_WALLET_CODE_DEPTH;

  struct error_code : tvm::error_code {
    static constexpr unsigned message_sender_is_not_my_owner  = 100;
    static constexpr unsigned not_enough_balance              = 101;
    static constexpr unsigned wrong_bounced_header            = 102;
    static constexpr unsigned wrong_bounced_args              = 103;
    static constexpr unsigned internal_owner_enabled          = 104;
    static constexpr unsigned internal_owner_disabled         = 105;
    static constexpr unsigned define_pubkey_or_internal_owner = 106;
    static constexpr unsigned wrong_wallet_code_hash          = 107;
    static constexpr unsigned cant_override_wallet_code       = 108;
  };

  __always_inline
  void constructor(
    string  name,
    string  symbol,
    uint8   decimals,
    uint256 root_pubkey,
    address_opt root_owner,
    uint128 total_supply
  ) {
    require((root_pubkey != 0) or root_owner, error_code::define_pubkey_or_internal_owner);
    name_ = name;
    symbol_ = symbol;
    decimals_ = decimals;
    root_pubkey_ = root_pubkey;
    root_owner_ = root_owner;
    total_supply_ = total_supply;
    total_granted_ = uint128(0);
  }

  __always_inline
  bool setWalletCode(cell wallet_code) {
    check_owner(true);
    tvm_accept();
    require(!wallet_code_, error_code::cant_override_wallet_code);
    require(tvm_hash(wallet_code) == wallet_hash, error_code::wrong_wallet_code_hash);
    require(wallet_code.cdepth() == wallet_code_depth, error_code::wrong_wallet_code_hash);
    wallet_code_ = wallet_code;

    if constexpr (Internal) {
      tvm_rawreserve(tvm_balance() - int_value().get(), rawreserve_flag::up_to);
      set_int_return_flag(SEND_ALL_GAS);
    }
    return true;
  }

  __always_inline
  address deployWallet(
    uint256     pubkey,
    address_opt owner,
    uint128     tokens,
    uint128     evers
  ) {
    check_owner();
    tvm_accept();
    require(total_granted_ + tokens <= total_supply_, error_code::not_enough_balance);
    require(pubkey != 0 || owner, error_code::define_pubkey_or_internal_owner);

    address answer_addr;
    if constexpr (Internal) {
      tvm_rawreserve(tvm_balance() - int_value().get(), rawreserve_flag::up_to);
      answer_addr = int_sender();
    } else {
      answer_addr = address{tvm_myaddr()};
    }

    auto [wallet_init, dest] = calc_wallet_init(pubkey, owner);

    // performing `tail call` - requesting dest to answer to our caller
    temporary_data::setglob(global_id::answer_id, return_func_id()->get());
    ITONTokenWalletPtr dest_handle(dest);
    dest_handle.deploy(wallet_init, Evers(evers.get())).
      acceptMint(tokens, answer_addr, evers);

    total_granted_ += tokens;

    set_int_return_flag(SEND_ALL_GAS);
    return dest;
  }

  __always_inline
  address deployEmptyWallet(
    uint256     pubkey,
    address_opt owner,
    uint128     evers
  ) {
    require(pubkey != 0 || owner, error_code::define_pubkey_or_internal_owner);

    // This protects from spending root balance to deploy message
    tvm_rawreserve(tvm_balance() - int_value().get(), rawreserve_flag::up_to);

    auto [wallet_init, dest] = calc_wallet_init(pubkey, owner);
    ITONTokenWalletPtr dest_handle(dest);
    dest_handle.deploy_noop(wallet_init, Evers(evers.get()));

    // sending all rest gas except reserved old balance, processing and deployment costs
    set_int_return_flag(SEND_ALL_GAS);
    return dest;
  }

  __always_inline
  void grant(
    address dest,
    uint128 tokens,
    uint128 evers
  ) {
    check_owner();
    require(total_granted_ + tokens <= total_supply_, error_code::not_enough_balance);

    tvm_accept();

    address answer_addr;
    unsigned msg_flags = 0;
    if constexpr (Internal) {
      tvm_rawreserve(tvm_balance() - int_value().get(), rawreserve_flag::up_to);
      msg_flags = SEND_ALL_GAS;
      evers = 0;
      answer_addr = int_sender();
    } else {
      answer_addr = address{tvm_myaddr()};
    }

    ITONTokenWalletPtr dest_handle(dest);
    dest_handle(Evers(evers.get()), msg_flags).acceptMint(tokens, answer_addr, 0u128);

    total_granted_ += tokens;
  }

  __always_inline
  bool mint(uint128 tokens) {
    check_owner();

    tvm_accept();

    if constexpr (Internal) {
      tvm_rawreserve(tvm_balance() - int_value().get(), rawreserve_flag::up_to);
    }

    total_supply_ += tokens;

    set_int_return_flag(SEND_ALL_GAS);
    return true;
  }

  __always_inline
  uint128 requestTotalGranted() {
    tvm_rawreserve(tvm_balance() - int_value().get(), rawreserve_flag::up_to);
    set_int_return_flag(SEND_ALL_GAS);
    return total_granted_;
  }

  // getters
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

  __always_inline address_opt getRootOwner() {
    return root_owner_;
  }

  __always_inline uint128 getTotalSupply() {
    return total_supply_;
  }

  __always_inline uint128 getTotalGranted() {
    return total_granted_;
  }

  __always_inline bool hasWalletCode() {
    return wallet_code_;
  }

  __always_inline cell getWalletCode() {
    return wallet_code_.get();
  }

  __always_inline
  address getWalletAddress(uint256 pubkey, address_opt owner) {
    return calc_wallet_init(pubkey, owner).second;
  }

  // received bounced message back
  __always_inline static int _on_bounced(cell /*msg*/, slice msg_body) {
    tvm_accept();

    using Args = args_struct_t<&ITONTokenWallet::acceptMint>;
    parser p(msg_body);
    require(p.ldi(32) == -1, error_code::wrong_bounced_header);
    auto [opt_hdr, =p] = parse_continue<abiv2::internal_msg_header_with_answer_id>(p);
    require(opt_hdr && opt_hdr->function_id == id_v<&ITONTokenWallet::acceptMint>,
            error_code::wrong_bounced_header);
    auto args = parse<Args>(p, error_code::wrong_bounced_args);
    auto bounced_val = args.tokens;

    auto [hdr, persist] = load_persistent_data<IRootTokenContract, root_replay_protection_t, DRootTokenContract>();
    require(bounced_val <= persist.total_granted_, error_code::wrong_bounced_args);
    persist.total_granted_ -= bounced_val;
    save_persistent_data<IRootTokenContract, root_replay_protection_t>(hdr, persist);
    return 0;
  }

  __always_inline
  uint256 getWalletCodeHash() {
    return uint256{tvm_hash(wallet_code_.get())};
  }

  // default processing of unknown messages
  __always_inline static int _fallback(cell /*msg*/, slice /*msg_body*/) {
    return 0;
  }

  // =============== Support functions ==================
  DEFAULT_SUPPORT_FUNCTIONS(IRootTokenContract, root_replay_protection_t)
private:
  __always_inline
  int8 workchain_id() {
    return std::get<addr_std>(address{tvm_myaddr()}.val()).workchain_id;
  }

  __always_inline
  std::pair<StateInit, address> calc_wallet_init(uint256 pubkey, address_opt owner) {
    address root_address{tvm_myaddr()};

    DTONTokenWallet wallet_data =
      prepare_wallet_data(name_, symbol_, decimals_,
                          root_pubkey_, root_address,
                          pubkey, owner,
                          uint256(wallet_hash), uint16(wallet_code_depth),
                          workchain_id());

    auto [wallet_init, dest_addr] = prepare_wallet_state_init_and_addr(wallet_data, wallet_code_.get());
    address dest = address::make_std(workchain_id(), dest_addr);
    return { wallet_init, dest };
  }

  __always_inline bool is_internal_owner() const { return root_owner_.has_value(); }

  __always_inline
  void check_internal_owner() {
    require(is_internal_owner(), error_code::internal_owner_disabled);
    require(*root_owner_ == int_sender(), error_code::message_sender_is_not_my_owner);
  }

  __always_inline
  void check_external_owner(bool allow_pubkey_owner_in_internal_node) {
    require(allow_pubkey_owner_in_internal_node || !is_internal_owner(), error_code::internal_owner_enabled);
    require(msg_pubkey() == root_pubkey_, error_code::message_sender_is_not_my_owner);
  }

  // allow_pubkey_owner_in_internal_node - to allow setWalletCode initialization by external message,
  //  even in internal-owned mode
  __always_inline
  void check_owner(bool allow_pubkey_owner_in_internal_node = false) {
    if constexpr (Internal)
      check_internal_owner();
    else
      check_external_owner(allow_pubkey_owner_in_internal_node);
  }
};

DEFINE_JSON_ABI(IRootTokenContract, DRootTokenContract, ERootTokenContract, root_replay_protection_t);

// ----------------------------- Main entry functions ---------------------- //
DEFAULT_MAIN_ENTRY_FUNCTIONS_TMPL(RootTokenContract, IRootTokenContract, DRootTokenContract, ROOT_TIMESTAMP_DELAY)

