#include "TONTokenWallet.hpp"

#include <tvm/contract.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/default_support_functions.hpp>

using namespace tvm;
using namespace schema;

template<bool Internal>
class TONTokenWallet final : public smart_interface<ITONTokenWallet>, public DTONTokenWallet {
public:
  static constexpr unsigned min_transfer_costs = 150000000;

  struct error_code : tvm::error_code {
    static constexpr unsigned message_sender_is_not_my_owner       = 100;
    static constexpr unsigned not_enough_balance                   = 101;
    static constexpr unsigned message_sender_is_not_my_root        = 102;
    static constexpr unsigned message_sender_is_not_good_wallet    = 103;
    static constexpr unsigned wrong_bounced_header                 = 104;
    static constexpr unsigned wrong_bounced_args                   = 105;
    static constexpr unsigned non_zero_remaining                   = 106;
    static constexpr unsigned no_allowance_set                     = 107;
    static constexpr unsigned wrong_spender                        = 108;
    static constexpr unsigned not_enough_allowance                 = 109;
    static constexpr unsigned internal_owner_enabled               = 110;
    static constexpr unsigned internal_owner_disabled              = 111;
    static constexpr unsigned destroy_non_empty_wallet             = 112;
    static constexpr unsigned only_original_owner_allowed          = 113;
    static constexpr unsigned wallet_in_lend_owneship              = 114;
    static constexpr unsigned finish_time_must_be_greater_than_now = 115;
    static constexpr unsigned not_enough_tons_to_process           = 116;
  };

  __always_inline
  void transfer(address dest, TokensType tokens, bool_t return_ownership, address answer_addr) {
    auto active_balance = check_owner(/*original_owner_only*/false);

    require(tokens <= active_balance, error_code::not_enough_balance);
    require(int_value().get() >= min_transfer_costs, error_code::not_enough_tons_to_process);

    // Transfer to zero address is not allowed.
    require(std::get<addr_std>(dest()).address != 0, error_code::not_enough_balance);

    tvm_accept();

    auto owner_addr = owner_address_ ? std::get<addr_std>((*owner_address_)()).address : uint256(0);

    ITONTokenWalletPtr dest_wallet(dest);
    dest_wallet(Grams(0), SEND_REST_GAS_FROM_INCOMING).
      internalTransfer(tokens, wallet_public_key_, owner_addr, answer_addr);

    balance_ -= tokens;

    if (return_ownership) {
      lend_ownership_.reset();
    } else if (lend_ownership_) {
      lend_ownership_->lend_balance -= tokens;
    }
  }

  __always_inline
  void transferToRecipient(uint256 recipient_public_key, uint256 recipient_internal_owner,
      TokensType tokens, bool_t deploy, address answer_addr) {
    auto active_balance = check_owner(/*original_owner_only*/false);
    require(tokens <= active_balance, error_code::not_enough_balance);

    tvm_accept();

    auto owner_addr = owner_address_ ? std::get<addr_std>((*owner_address_)()).address : uint256(0);

    auto [wallet_init, dest] = calc_wallet_init(recipient_public_key, recipient_internal_owner);
    ITONTokenWalletPtr dest_wallet(dest);
    if (deploy) {
      dest_wallet.deploy(wallet_init, Grams(0), SEND_REST_GAS_FROM_INCOMING).
        internalTransfer(tokens, wallet_public_key_, owner_addr, answer_addr);
    } else {
      dest_wallet(Grams(0), SEND_REST_GAS_FROM_INCOMING).
        internalTransfer(tokens, wallet_public_key_, owner_addr, answer_addr);
    }
    balance_ -= tokens;
  }

  __always_inline
  TokensType getBalance_InternalOwner() {
    check_internal_owner(/*original_owner_only*/false);
    set_int_return_flag(SEND_REST_GAS_FROM_INCOMING);
    return balance_;
  }

  __always_inline
  void accept(TokensType tokens) {
    // the function must check that message sender is the RTW.
    require(root_address_ == int_sender(),
            error_code::message_sender_is_not_my_root);

    tvm_accept();

    balance_ += tokens;
  }

  __always_inline
  void internalTransfer(TokensType tokens, uint256 pubkey, uint256 my_owner_addr, address answer_addr) {
    uint256 expected_address = expected_sender_address(pubkey, my_owner_addr);
    auto sender = int_sender();

    require(std::get<addr_std>(sender()).address == expected_address,
            error_code::message_sender_is_not_good_wallet);

    tvm_accept();

    balance_ += tokens;

    tvm_transfer(answer_addr, 0, false, SEND_REST_GAS_FROM_INCOMING);
  }

  __always_inline
  void destroy(address dest) {
    check_owner(/*original_owner_only*/true);
    require(balance_ == 0, error_code::destroy_non_empty_wallet);
    tvm_accept();
    auto empty_cell = builder().endc();
    tvm_transfer(dest, 0, false,
      SEND_ALL_GAS | SENDER_WANTS_TO_PAY_FEES_SEPARATELY | DELETE_ME_IF_I_AM_EMPTY | IGNORE_ACTION_ERRORS,
      empty_cell);
  }

  __always_inline
  bool_t lendOwnership(uint256 std_dest, TokensType lend_balance, uint32 lend_finish_time,
      cell deploy_init_cl, cell payload) {
    check_owner(/*original_owner_only*/true);
    //require(lend_finish_time > tvm_now(), error_code::finish_time_must_be_greater_than_now);
    require(lend_balance <= balance_, error_code::not_enough_balance);
    tvm_accept();

    auto answer_addr = int_sender();

    auto dest = address::make_std(workchain_id_, std_dest);
    lend_ownership_ = { dest, lend_balance, lend_finish_time };

    auto deploy_init = parse<StateInit>(deploy_init_cl.ctos());

    auto owner_addr = owner_address_ ? std::get<addr_std>((*owner_address_)()).address : uint256(0);
    if (deploy_init.code && deploy_init.data) {
      // performing `tail call`
      temporary_data::setglob(global_id::answer_id, return_func_id()->get());
      ITONTokenWalletNotifyPtr(dest).deploy(deploy_init, Grams(0), SEND_REST_GAS_FROM_INCOMING).
        onTip3LendOwnership(lend_balance, lend_finish_time, wallet_public_key_, owner_addr, payload, answer_addr);
    } else {
      // performing `tail call`
      temporary_data::setglob(global_id::answer_id, return_func_id()->get());
      ITONTokenWalletNotifyPtr(dest)(Grams(0), SEND_REST_GAS_FROM_INCOMING).
        onTip3LendOwnership(lend_balance, lend_finish_time, wallet_public_key_, owner_addr, payload, answer_addr);
    }
    // TODO: implement tail_call and remove bool return here
    set_int_return_value(100000000);
    return bool_t(true);
  }

  __always_inline
  void returnOwnership() {
    check_owner(/*original_owner_only*/false);
    lend_ownership_.reset();
  }

  // getters
  __always_inline
  details_info getDetails() {
    return { getName(), getSymbol(), getDecimals(),
             getBalance(), getRootKey(), getWalletKey(),
             getRootAddress(), getOwnerAddress(), getLendOwnership(),
             getCode(), allowance(), workchain_id_ };
  }

  __always_inline bytes getName() {
    return name_;
  }
  __always_inline bytes getSymbol() {
    return symbol_;
  }
  __always_inline uint8 getDecimals() {
    return decimals_;
  }
  __always_inline TokensType getBalance() {
    return balance_;
  }
  __always_inline uint256 getRootKey() {
    return root_public_key_;
  }
  __always_inline uint256 getWalletKey() {
    return wallet_public_key_;
  }
  __always_inline address getRootAddress() {
    return root_address_;
  }
  __always_inline address getOwnerAddress() {
    return owner_address_ ? *owner_address_ : address::make_std(int8(0), uint256(0));
  }
  __always_inline lend_ownership_info getLendOwnership() {
    return lend_ownership_ ? *lend_ownership_ :
      lend_ownership_info{address::make_std(int8(0), uint256(0)), TokensType(0), uint32(0)};
  }
  __always_inline cell getCode() {
    return code_;
  }
  __always_inline allowance_info allowance() {
    return allowance_ ? *allowance_ :
      allowance_info{address::make_std(int8(0), uint256(0)), TokensType(0)};
  }

  // allowance interface
  __always_inline
  void approve(address spender, TokensType remainingTokens, TokensType tokens) {
    check_owner(/*original_owner_only*/true);
    require(tokens <= balance_, error_code::not_enough_balance);
    tvm_accept();
    if (allowance_) {
      if (allowance_->remainingTokens == remainingTokens) {
        allowance_->remainingTokens = tokens;
        allowance_->spender = spender;
      }
    } else {
      require(remainingTokens == 0, error_code::non_zero_remaining);
      allowance_ = { spender, tokens };
    }
  }

  /*__always_inline
  void transferFrom(address dest, address to, TokensType tokens,
                    WalletGramsType grams) {
    check_owner(true);
    tvm_accept();

    handle<ITONTokenWallet> dest_wallet(dest);
    dest_wallet(Grams(grams.get())).
      internalTransferFrom(to, tokens);
  }*/

  __always_inline
  void internalTransferFrom(address to, TokensType tokens) {
    require(!!allowance_, error_code::no_allowance_set);
    require(int_sender() == allowance_->spender, error_code::wrong_spender);
    require(tokens <= allowance_->remainingTokens, error_code::not_enough_allowance);
    require(tokens <= balance_, error_code::not_enough_balance);

    auto owner_addr = owner_address_ ? std::get<addr_std>((*owner_address_)()).address : uint256(0);
    handle<ITONTokenWallet> dest_wallet(to);
    dest_wallet(Grams(0), SEND_REST_GAS_FROM_INCOMING).
      internalTransfer(tokens, wallet_public_key_, owner_addr);

    allowance_->remainingTokens -= tokens;
    balance_ -= tokens;
  }

  __always_inline
  void disapprove() {
    check_owner(/*original_owner_only*/true);
    tvm_accept();
    allowance_.reset();
  }

  // received bounced message back
  __always_inline static int _on_bounced(cell msg, slice msg_body) {
    tvm_accept();

    parser p(msg_body);
    require(p.ldi(32) == -1, error_code::wrong_bounced_header);
    auto [opt_hdr, =p] = parse_continue<abiv1::internal_msg_header>(p);
    require(!!opt_hdr, error_code::wrong_bounced_header);
    // If it is bounced internalTransferFrom, do nothing
    //if (opt_hdr->function_id == id_v<&ITONTokenWallet::internalTransferFrom>)
    //  return 0;

    // Otherwise, it should be bounced internalTransfer
    require(opt_hdr->function_id == id_v<&ITONTokenWallet::internalTransfer>,
            error_code::wrong_bounced_header);
    using Args = args_struct_t<&ITONTokenWallet::internalTransfer>;
    static_assert(std::is_same_v<decltype(Args{}.tokens), TokensType>);

    // Parsing only first tokens variable internalTransfer pubkey argument won't fit into bounced response
    auto bounced_val = parse<TokensType>(p, error_code::wrong_bounced_args);

    auto [hdr, persist] = load_persistent_data<ITONTokenWallet, wallet_replay_protection_t, DTONTokenWallet>();
    persist.balance_ += bounced_val;
    save_persistent_data<ITONTokenWallet, wallet_replay_protection_t>(hdr, persist);
    return 0;
  }
  // default processing of unknown messages
  __always_inline static int _fallback(cell msg, slice msg_body) {
    return 0;
  }

  // =============== Support functions ==================
  DEFAULT_SUPPORT_FUNCTIONS(ITONTokenWallet, wallet_replay_protection_t)
private:
  __always_inline
  std::pair<StateInit, uint256> calc_wallet_init_hash(uint256 pubkey, uint256 internal_owner) {
    std::optional<address> owner_addr;
    if (internal_owner)
      owner_addr = address::make_std(workchain_id_, internal_owner);
    DTONTokenWallet wallet_data {
      name_, symbol_, decimals_,
      TokensType(0), root_public_key_, pubkey,
      root_address_, owner_addr, {}, code_, {}, workchain_id_
    };
    return prepare_wallet_state_init_and_addr(wallet_data);
  }

  __always_inline
  uint256 expected_sender_address(uint256 sender_public_key, uint256 sender_owner_addr) {
    return calc_wallet_init_hash(sender_public_key, sender_owner_addr).second;
  }

  __always_inline
  std::pair<StateInit, address> calc_wallet_init(uint256 pubkey, uint256 internal_owner) {
    auto [wallet_init, dest_addr] = calc_wallet_init_hash(pubkey, internal_owner);
    address dest = address::make_std(workchain_id_, dest_addr);
    return { wallet_init, dest };
  }

  __always_inline
  std::optional<lend_ownership_info> check_lend_ownership() {
    if (lend_ownership_) {
      if (tvm_now() >= lend_ownership_->lend_finish_time) {
        lend_ownership_.reset();
        return {};
      }
      return lend_ownership_;
    }
    return {};
  }

  __always_inline bool is_internal_owner() const { return owner_address_.has_value(); }

  __always_inline
  TokensType check_internal_owner(bool original_owner_only) {
    if (auto lend_info = check_lend_ownership()) {
      require(!original_owner_only, error_code::only_original_owner_allowed);
      require(lend_info->owner == int_sender(),
              error_code::message_sender_is_not_my_owner);
      return std::min(balance_, lend_info->lend_balance);
    } else {
      require(is_internal_owner(), error_code::internal_owner_disabled);
      require(*owner_address_ == int_sender(),
              error_code::message_sender_is_not_my_owner);
      return balance_;
    }
  }

  __always_inline
  TokensType check_external_owner(bool original_owner_only) {
    require(!is_internal_owner(), error_code::internal_owner_enabled);
    require(tvm_pubkey() == wallet_public_key_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    require(!check_lend_ownership(), error_code::wallet_in_lend_owneship);
    return balance_;
  }

  __always_inline TokensType check_owner(bool original_owner_only) {
    if constexpr (Internal)
      return check_internal_owner(original_owner_only);
    else
      return check_external_owner(original_owner_only);
  }
};

DEFINE_JSON_ABI(ITONTokenWallet, DTONTokenWallet, ETONTokenWallet);

// ----------------------------- Main entry functions ---------------------- //
DEFAULT_MAIN_ENTRY_FUNCTIONS_TMPL(TONTokenWallet, ITONTokenWallet, DTONTokenWallet, TOKEN_WALLET_TIMESTAMP_DELAY)

