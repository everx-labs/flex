/** \file
 *  \brief TONTokenWallet contract implementation
 *  Compiles into two contract versions: TONTokenWallet.tvc (external wallet) and FlexWallet.tvc (internal wallet).
 *  With different macroses.
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#include "TONTokenWallet.hpp"

#ifdef TIP3_ENABLE_BURN
#include "Wrapper.hpp"
#endif

#include <tvm/contract.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/default_support_functions.hpp>
#include <tvm/suffixes.hpp>
#include <tvm/schema/parse_chain_static.hpp>

#include "FlexTransferPayloadArgs.hpp"
#include "FlexLendPayloadArgs.hpp"

using namespace tvm;

/// Authentification configuration flags
struct auth_cfg {
  bool    allowed_for_original_owner_in_lend_state; ///< Allowed for original owner in lend state
  bool    allowed_lend_pubkey;                      ///< Allowed for lend pubkey (person/script external owner)
  bool    allowed_lend_owner;                       ///< Allowed for lend owner (contract internal owner)
  uint32  required_time;                            ///< Required time. Lend finish time must be greater.
  uint128 required_tokens;                          ///< Required tokens. Lend balance must be greater and will be decreased by.
  uint128 required_evers;                           ///< Required evers. Lend evers must be greater and will be decreased by.
  uint128 return_ownership;                         ///< Return ownership value
};

/// Implementation of TONTokenWallet contract
template<bool Internal>
class TONTokenWallet final : public smart_interface<ITONTokenWallet>, public DTONTokenWallet {
public:
  using data = DTONTokenWallet;
  static constexpr unsigned min_transfer_costs = 150000000; ///< Minimum transfer costs in evers
  static constexpr unsigned c_max_lend_owners  = 50;        ///< Limit of lend owners

  /// Error codes of TONTokenWallet contract
  struct error_code : tvm::error_code {
    static constexpr unsigned message_sender_is_not_my_owner       = 100; ///< Authorization error
    static constexpr unsigned not_enough_balance                   = 101; ///< Not enough token balance to proceed
    static constexpr unsigned message_sender_is_not_my_root        = 102; ///< Message sender is not RootTokenContract address
    static constexpr unsigned message_sender_is_not_good_wallet    = 103; ///< Message sender is not a good wallet
    static constexpr unsigned wrong_bounced_header                 = 104; ///< Wrong header of bounced message
    static constexpr unsigned wrong_bounced_args                   = 105; ///< Wrong arguments in bounced message
    static constexpr unsigned non_zero_remaining                   = 106; ///< Allowance is empty and remainingTokens is non zero
    static constexpr unsigned no_allowance_set                     = 107; ///< Allowance is not set up
    static constexpr unsigned wrong_spender                        = 108; ///< Wrong spender for allowance
    static constexpr unsigned not_enough_allowance                 = 109; ///< Not enough allowance to proceed
    static constexpr unsigned inconsistent_parameters              = 110; ///< Inconsistent parameters
    static constexpr unsigned destroy_non_empty_wallet             = 111; ///< Wallet with non-zero token balance can't be destroyed
    static constexpr unsigned wallet_in_lend_owneship              = 112; ///< Wallet in lend ownership state
    static constexpr unsigned finish_time_must_be_greater_than_now = 113; ///< Lend finish time must be in future
    static constexpr unsigned not_enough_evers_to_process          = 114; ///< Not enough evers to process
    static constexpr unsigned allowance_is_set                     = 115; ///< Allowance is set (wallet can't process lendOwnership)
    static constexpr unsigned transfer_to_zero_address             = 116; ///< Transfer to zero address
    static constexpr unsigned obsolete_lend_pubkey                 = 117; ///< Lend public key is obsolete
    static constexpr unsigned lend_owner_not_found                 = 118; ///< Lend owner not found
    static constexpr unsigned finish_time_is_out_of_lend_time      = 119; ///< Finish time is out of lend time
    static constexpr unsigned lend_owners_overlimit                = 120; ///< Lend owners overlimit
    static constexpr unsigned zero_lend_balance                    = 121; ///< Zero lend balance
    static constexpr unsigned wrong_user_id                        = 122; ///< Wrong user id
  };

  __always_inline
  void transfer(
    address answer_addr,
    address to,
    uint128 tokens,
    uint128 evers,
    uint128 return_ownership
  ) {
    transfer_impl(answer_addr, to, tokens, evers, return_ownership, false, builder().endc());
  }

  __always_inline
  void transferWithNotify(
    address answer_addr,
    address to,
    uint128 tokens,
    uint128 evers,
    uint128 return_ownership,
    cell    payload
  ) {
    // performing `tail call` - requesting dest to answer to our caller
    temporary_data::setglob(global_id::answer_id, return_func_id()->get());
    transfer_impl(answer_addr, to, tokens, evers, return_ownership, true, payload);
  }

#ifdef TIP3_DEPLOY_TRANSFER
  __always_inline
  void transferToRecipient(
    address     answer_addr,
    uint256     recipient_pubkey,
    address_opt recipient_owner,
    uint128     tokens,
    uint128     evers,
    bool        deploy,
    uint128     return_ownership
  ) {
    transfer_to_recipient_impl(answer_addr, recipient_pubkey, recipient_owner,
                               tokens, evers, deploy, return_ownership, false, builder().endc());
  }

  __always_inline
  void transferToRecipientWithNotify(
    address     answer_addr,
    uint256     recipient_pubkey,
    address_opt recipient_owner,
    uint128     tokens,
    uint128     evers,
    bool        deploy,
    uint128     return_ownership,
    cell        payload
  ) {
    // performing `tail call` - requesting dest to answer to our caller
    temporary_data::setglob(global_id::answer_id, return_func_id()->get());
    transfer_to_recipient_impl(answer_addr, recipient_pubkey, recipient_owner,
                               tokens, evers, deploy, return_ownership, true, payload);
  }
#endif // TIP3_DEPLOY_TRANSFER

  __always_inline
  uint128 requestBalance() {
    tvm_rawreserve(tvm_balance() - int_value().get(), rawreserve_flag::up_to);
    set_int_return_flag(SEND_ALL_GAS);
    return balance_;
  }

  __always_inline
  bool accept(
    uint128 tokens,
    address answer_addr,
    uint128 keep_evers
  ) {
    auto [sender, value] = int_sender_and_value();
    // the function must check that message sender is the RTW.
    require(root_address_ == sender, error_code::message_sender_is_not_my_root);
    tvm_accept();
    balance_ += tokens;

    tvm_rawreserve(tvm_balance() + keep_evers.get() - value(), rawreserve_flag::up_to);

    set_int_sender(answer_addr);
    set_int_return_value(0);
    set_int_return_flag(SEND_ALL_GAS | IGNORE_ACTION_ERRORS);

    return true;
  }

  __always_inline
  void internalTransfer(
    uint128     tokens,
    address     answer_addr,
    uint256     sender_pubkey,
    address_opt sender_owner,
    bool        notify_receiver,
    cell        payload
  ) {
    uint256 expected_addr = expected_address(sender_pubkey, sender_owner);
    auto [sender, value_gr] = int_sender_and_value();
    require(std::get<addr_std>(sender()).address == expected_addr,
            error_code::message_sender_is_not_good_wallet);
    balance_ += tokens;

    tvm_rawreserve(tvm_balance() - value_gr(), rawreserve_flag::up_to);
    // If notify_receiver is specified, we send notification to the internal owner
    if (notify_receiver && owner_address_) {
      // performing `tail call` - requesting dest to answer to our caller
      temporary_data::setglob(global_id::answer_id, return_func_id()->get());
      ITONTokenWalletNotifyPtr(*owner_address_)(Evers(0), SEND_ALL_GAS).
        onTip3Transfer(balance_, tokens, sender_pubkey, sender_owner, payload, answer_addr);
    } else {
      // In some cases (allowance request, for example) answer_addr may be this contract
      if (answer_addr != address{tvm_myaddr()})
        tvm_transfer(answer_addr, 0, false, SEND_ALL_GAS);
    }
  }

  __always_inline
  void destroy(address dest) {
    require(balance_ == 0, error_code::destroy_non_empty_wallet);
    check_owner({
      .allowed_for_original_owner_in_lend_state = false,
      .allowed_lend_pubkey                      = false,
      .allowed_lend_owner                       = false
    });
    tvm_accept();
    tvm_transfer(dest, 0, false,
      SEND_ALL_GAS | SENDER_WANTS_TO_PAY_FEES_SEPARATELY | DELETE_ME_IF_I_AM_EMPTY | IGNORE_ACTION_ERRORS);
  }

#ifdef TIP3_ENABLE_BURN
  __always_inline
  void burn(
    uint256     out_pubkey,
    address_opt out_owner
  ) {
    check_owner({
      .allowed_for_original_owner_in_lend_state = false,
      .allowed_lend_pubkey                      = false,
      .allowed_lend_owner                       = false
    });
    tvm_accept();
    IWrapperPtr root_ptr(root_address_);
    unsigned flags = SEND_ALL_GAS | SENDER_WANTS_TO_PAY_FEES_SEPARATELY | DELETE_ME_IF_I_AM_EMPTY |
                     IGNORE_ACTION_ERRORS;
    root_ptr(Evers(0), flags).
      burn(getBalance(), int_sender(), wallet_pubkey_, owner_address_, out_pubkey, out_owner);
  }
#endif // TIP3_ENABLE_BURN

#ifdef TIP3_ENABLE_LEND_OWNERSHIP
  __always_inline
  void lendOwnershipPubkey(
    opt<uint256> lend_pubkey,     ///< Lend owner public key. Revoke if empty.
    opt<uint32>  lend_finish_time ///< Lend ownership finish time. If empty, will be taken from the current lend pubkey record.
  ) {
    require(!lend_finish_time || *lend_finish_time > tvm_now(), error_code::finish_time_must_be_greater_than_now);
    // lend_finish_time must be set iff lend_pubkey set,
    //  or it may be empty if lend_pubkey set and the current lend_pubkey_ exists (to keep lend_finish_time from there).
    require((lend_pubkey.has_value() == lend_finish_time.has_value()) ||
            (lend_pubkey && lend_pubkey_), error_code::inconsistent_parameters);
#ifdef TIP3_ENABLE_ALLOWANCE
    require(!allowance_, error_code::allowance_is_set);
#endif // TIP3_ENABLE_ALLOWANCE
    check_owner({
      .allowed_for_original_owner_in_lend_state = true,
      .allowed_lend_pubkey                      = false,
      .allowed_lend_owner                       = false
    });
    tvm_accept();
    if (lend_pubkey) {
      lend_pubkey_ = {
        .lend_pubkey = *lend_pubkey,
        .lend_finish_time = (lend_finish_time ? *lend_finish_time : lend_pubkey_->lend_finish_time)
      };
    } else {
      lend_pubkey_.reset();
    }
  }

  __always_inline
  void lendOwnership(
    address answer_addr,
    uint128 evers,
    address dest,
    uint128 lend_balance,
    uint32  lend_finish_time,
    cell    deploy_init_cl,
    cell    payload
  ) {
    require(lend_finish_time > tvm_now(), error_code::finish_time_must_be_greater_than_now);
    require(lend_balance > 0, error_code::zero_lend_balance);

#ifdef TIP3_ENABLE_ALLOWANCE
    require(!allowance_, error_code::allowance_is_set);
#endif // TIP3_ENABLE_ALLOWANCE

    check_owner({
      .allowed_for_original_owner_in_lend_state = true,
      .allowed_lend_pubkey                      = true,
      .allowed_lend_owner                       = false,
      .required_time                            = lend_finish_time,
      .required_tokens                          = lend_balance,
      .required_evers                           = evers + min_transfer_costs
    });
    require(lend_owners_.size() < c_max_lend_owners || lend_owners_.contains({dest}), error_code::lend_owners_overlimit);

    auto user_id = wallet_pubkey_;
    auto args = parse_chain_static<FlexLendPayloadArgs>(parser(payload.ctos()));
    require(args.user_id == user_id, error_code::wrong_user_id);

    auto answer_addr_fxd = fixup_answer_addr(answer_addr);

    // repeated lend to the same address will be { sumX + sumY, max(timeX, timeY) }
    auto sum_lend_balance = lend_balance;
    auto sum_lend_finish_time = lend_finish_time;
    if (auto existing_lend = lend_owners_.lookup({dest})) {
      sum_lend_balance += existing_lend->lend_balance;
      sum_lend_finish_time = std::max(lend_finish_time, existing_lend->lend_finish_time);
    }

    lend_owners_.set_at({dest}, {sum_lend_balance, sum_lend_finish_time});

    unsigned msg_flags = prepare_transfer_message_flags(evers);

    auto deploy_init_sl = deploy_init_cl.ctos();
    StateInit deploy_init;
    if (!deploy_init_sl.empty())
      deploy_init = parse<StateInit>(deploy_init_sl);

    if (deploy_init.code && deploy_init.data) {
      // performing `tail call` - requesting dest to answer to our caller
      temporary_data::setglob(global_id::answer_id, return_func_id()->get());
      ITONTokenWalletNotifyPtr(dest).deploy(deploy_init, Evers(evers.get()), msg_flags).
        onTip3LendOwnership(lend_balance, lend_finish_time,
                            wallet_pubkey_, owner_address_, payload, answer_addr_fxd);
    } else {
      // performing `tail call` - requesting dest to answer to our caller
      temporary_data::setglob(global_id::answer_id, return_func_id()->get());
      ITONTokenWalletNotifyPtr(dest)(Evers(evers.get()), msg_flags).
        onTip3LendOwnership(lend_balance, lend_finish_time,
                            wallet_pubkey_, owner_address_, payload, answer_addr_fxd);
    }
  }

  __always_inline
  void returnOwnership(
    uint128 tokens
  ) {
    check_owner({
      .allowed_for_original_owner_in_lend_state = false,
      .allowed_lend_pubkey                      = false,
      .allowed_lend_owner                       = true
    });

    auto sender = int_sender();
    auto v = lend_owners_.extract({sender});
    require(!!v, error_code::lend_owner_not_found);
    tokens = std::min(tokens, v->lend_balance);

    if (v->lend_balance > tokens) {
      v->lend_balance -= tokens;
      lend_owners_.set_at({sender}, *v);
    }
  }
#endif // TIP3_ENABLE_LEND_OWNERSHIP

  // =============================== getters =============================== //
  __always_inline
  details_info getDetails() {
    auto [filtered_lend_pubkeys, filtered_lend_owners, lend_balance] = filter_lend_arrays();
    return { getName(), getSymbol(), getDecimals(),
             getBalance(), getRootKey(), getRootAddress(),
             getWalletKey(), getOwnerAddress(),
             filtered_lend_pubkeys, filtered_lend_owners, lend_balance,
             code_hash_, code_depth_, allowance(), workchain_id_ };
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
  __always_inline uint128 getBalance() {
    return balance_;
  }
  __always_inline uint256 getRootKey() {
    return root_pubkey_;
  }
  __always_inline uint256 getWalletKey() {
    return wallet_pubkey_;
  }
  __always_inline address getRootAddress() {
    return root_address_;
  }
  __always_inline address getOwnerAddress() {
    return owner_address_ ? *owner_address_ : address::make_std(int8(0), uint256(0));
  }
  __always_inline allowance_info allowance() {
#ifdef TIP3_ENABLE_ALLOWANCE
    if (allowance_) return *allowance_;
#endif
    return allowance_info{address::make_std(int8(0), uint256(0)), uint128(0)};
  }

  // ========================= allowance interface ========================= //
#ifdef TIP3_ENABLE_ALLOWANCE
  __always_inline
  void approve(
    address spender,
    uint128 remainingTokens,
    uint128 tokens
  ) {
    check_owner({
      .allowed_for_original_owner_in_lend_state = false,
      .allowed_lend_pubkey                      = false,
      .allowed_lend_owner                       = false
    });
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

  __always_inline
  void transferFrom(
    address answer_addr,
    address from,
    address to,
    uint128 tokens,
    uint128 evers
  ) {
    transfer_from_impl(answer_addr, from, to, tokens, evers, false, builder().endc());
  }

  __always_inline
  void transferFromWithNotify(
    address answer_addr,
    address from,
    address to,
    uint128 tokens,
    uint128 evers,
    cell    payload
  ) {
    transfer_from_impl(answer_addr, from, to, tokens, evers, true, payload);
  }

  __always_inline
  void internalTransferFrom(
    address answer_addr,
    address to,
    uint128 tokens,
    bool    notify_receiver,
    cell    payload
  ) {
    require(!!allowance_, error_code::no_allowance_set);
    require(int_sender() == allowance_->spender, error_code::wrong_spender);
    require(tokens <= allowance_->remainingTokens, error_code::not_enough_allowance);
    require(tokens <= balance_, error_code::not_enough_balance);

    ITONTokenWalletPtr dest_wallet(to);
    tvm_rawreserve(tvm_balance() - int_value().get(), rawreserve_flag::up_to);
    dest_wallet(Evers(0), SEND_ALL_GAS).
      internalTransfer(tokens, answer_addr, wallet_pubkey_, owner_address_, notify_receiver, payload);

    allowance_->remainingTokens -= tokens;
    balance_ -= tokens;
  }

  __always_inline
  void disapprove() {
    check_owner({
      .allowed_for_original_owner_in_lend_state = false,
      .allowed_lend_pubkey                      = false,
      .allowed_lend_owner                       = false
    });
    tvm_accept();
    allowance_.reset();
  }
#endif // TIP3_ENABLE_ALLOWANCE

  // received bounced message back
  __always_inline static int _on_bounced(cell msg, slice msg_body) {
    tvm_accept();

    parser p(msg_body);
    require(p.ldi(32) == -1, error_code::wrong_bounced_header);
    auto [opt_hdr, =p] = parse_continue<abiv2::internal_msg_header>(p);
    require(!!opt_hdr, error_code::wrong_bounced_header);
    // If it is bounced internalTransferFrom, do nothing
#ifdef TIP3_ENABLE_ALLOWANCE
    if (opt_hdr->function_id == id_v<&ITONTokenWallet::internalTransferFrom>)
      return 0;
#endif

    // other cases require load/store of persistent data
    auto [hdr, persist] = load_persistent_data<ITONTokenWallet, wallet_replay_protection_t, DTONTokenWallet>();

    // If it is bounced onTip3LendOwnership, then we need to reset lend ownership
#ifdef TIP3_ENABLE_LEND_OWNERSHIP
    if (opt_hdr->function_id == id_v<&ITONTokenWalletNotify::onTip3LendOwnership>) {
      auto parsed_msg = parse<int_msg_info>(parser(msg), error_code::bad_incoming_msg);
      auto sender = incoming_msg(parsed_msg).int_sender();
      auto [answer_id, =p] = parse_continue<uint32>(p);
      // Parsing `balance` variable, other arguments won't fit into bounced response
      auto [bounced_val, =p] = parse_continue<uint128>(p);
      require(!!bounced_val, error_code::wrong_bounced_args);

      auto v = persist.lend_owners_[{sender}];
      if (v.lend_balance <= *bounced_val) {
        persist.lend_owners_.erase({sender});
      } else {
        v.lend_balance -= *bounced_val;
        persist.lend_owners_.set_at({sender}, v);
      }
#else // TIP3_ENABLE_LEND_OWNERSHIP
    if (false) {
#endif // TIP3_ENABLE_LEND_OWNERSHIP
    } else {
      // Otherwise, it should be bounced internalTransfer
      require(opt_hdr->function_id == id_v<&ITONTokenWallet::internalTransfer>,
              error_code::wrong_bounced_header);
      using Args = args_struct_t<&ITONTokenWallet::internalTransfer>;
      static_assert(std::is_same_v<decltype(Args{}.tokens), uint128>);

      auto [answer_id, =p] = parse_continue<uint32>(p);
      // Parsing only first tokens variable internalTransfer, other arguments won't fit into bounced response
      auto bounced_val = parse<uint128>(p, error_code::wrong_bounced_args);
      persist.balance_ += bounced_val;
    }
    save_persistent_data<ITONTokenWallet, wallet_replay_protection_t>(hdr, persist);
    return 0;
  }

  // default processing of empty messages or func_id=0
  __always_inline static int _receive(cell /*msg*/, slice /*msg_body*/) {
    return 0;
  }

  // =============== Support functions ==================
  DEFAULT_SUPPORT_FUNCTIONS(ITONTokenWallet, wallet_replay_protection_t)
private:
  __always_inline
  void transfer_impl(address answer_addr, address to, uint128 tokens, uint128 evers,
                     uint128 return_ownership, bool send_notify, cell payload) {
    check_transfer_requires(tokens, evers, return_ownership);
    // Transfer to zero address is not allowed.
    require(std::get<addr_std>(to()).address != 0, error_code::transfer_to_zero_address);
    tvm_accept();

    auto answer_addr_fxd = fixup_answer_addr(answer_addr);

    unsigned msg_flags = prepare_transfer_message_flags(evers);
    ITONTokenWalletPtr dest_wallet(to);
    dest_wallet(Evers(evers.get()), msg_flags).
      internalTransfer(tokens, answer_addr_fxd, wallet_pubkey_, owner_address_, send_notify, payload);
    update_spent_balance(tokens);
  }

#ifdef TIP3_DEPLOY_TRANSFER
  __always_inline
  void transfer_to_recipient_impl(address answer_addr,
                                  uint256 recipient_pubkey, address_opt recipient_owner,
                                  uint128 tokens, uint128 evers, bool deploy,
                                  uint128 return_ownership, bool send_notify, cell payload) {
    check_transfer_requires(tokens, evers, return_ownership);
    tvm_accept();

    auto answer_addr_fxd = fixup_answer_addr(answer_addr);

    unsigned msg_flags = prepare_transfer_message_flags(evers);
    auto [wallet_init, dest] = calc_wallet_init(recipient_pubkey, recipient_owner);
    ITONTokenWalletPtr dest_wallet(dest);
    if (deploy) {
      dest_wallet.deploy(wallet_init, Evers(evers.get()), msg_flags).
        internalTransfer(tokens, answer_addr_fxd, wallet_pubkey_, owner_address_, send_notify, payload);
    } else {
      dest_wallet(Evers(evers.get()), msg_flags).
        internalTransfer(tokens, answer_addr_fxd, wallet_pubkey_, owner_address_, send_notify, payload);
    }
    update_spent_balance(tokens);
  }
#endif // TIP3_DEPLOY_TRANSFER

#ifdef TIP3_ENABLE_ALLOWANCE
  __always_inline
  void transfer_from_impl(address answer_addr, address from, address to,
                          uint128 tokens, uint128 evers, bool send_notify, cell payload) {
    check_owner({
      .allowed_for_original_owner_in_lend_state = false,
      .allowed_lend_pubkey                      = false,
      .allowed_lend_owner                       = false
    });
    tvm_accept();

    auto answer_addr_fxd = fixup_answer_addr(answer_addr);
    unsigned msg_flags = prepare_transfer_message_flags(evers);

    ITONTokenWalletPtr dest_wallet(from);
    dest_wallet(Evers(evers.get()), msg_flags).
      internalTransferFrom(answer_addr_fxd, to, tokens, send_notify, payload);
  }
#endif // TIP3_ENABLE_ALLOWANCE

  // If zero answer_addr is specified, it is corrected to incoming sender (for internal message),
  // or this contract address (for external message)
  __always_inline
  address fixup_answer_addr(address answer_addr) {
    if (std::get<addr_std>(answer_addr()).address == 0) {
      if constexpr (Internal)
        return address{int_sender()};
      else
        return address{tvm_myaddr()};
    }
    return answer_addr;
  }

  __always_inline
  void check_transfer_requires(uint128 tokens, uint128 evers, uint128 return_ownership) {
    check_owner({
      .allowed_for_original_owner_in_lend_state = true,  ///< Original owner may transfer unlocked tokens (in lend)
      .allowed_lend_pubkey                      = false, ///< Lend pubkey (person/script external owner) can't transfer tokens
      .allowed_lend_owner                       = true,  ///< Lend owner (contract) can transfer tokens
      .required_tokens                          = tokens,
      .required_evers                           = evers,
      .return_ownership                         = return_ownership
    });

    if constexpr (Internal)
      require(int_value().get() >= min_transfer_costs, error_code::not_enough_evers_to_process);
    else
      require(evers.get() >= min_transfer_costs && tvm_balance() > evers.get(),
              error_code::not_enough_evers_to_process);
  }

  __always_inline
  unsigned prepare_transfer_message_flags(uint128 &evers) {
    unsigned msg_flags = DEFAULT_MSG_FLAGS;
    if constexpr (Internal) {
      tvm_rawreserve(tvm_balance() - int_value().get(), rawreserve_flag::up_to);
      msg_flags = SEND_ALL_GAS;
      evers = 0;
    }
    return msg_flags;
  }

  // When balance is spent (transfer functions)
  __always_inline
  void update_spent_balance(uint128 tokens) {
    balance_ -= tokens;
  }

  __always_inline
  uint256 expected_address(uint256 sender_pubkey, address_opt sender_owner) {
    DTONTokenWallet wallet_data {
      name_, symbol_, decimals_,
      0u128, root_pubkey_, root_address_,
      sender_pubkey, sender_owner,
#ifdef TIP3_ENABLE_LEND_OWNERSHIP
      {}, {},
#endif
#ifdef TIP3_DEPLOY_TRANSFER
      code_,
#endif
      code_hash_, code_depth_,
#ifdef TIP3_ENABLE_ALLOWANCE
      {},
#endif
      workchain_id_
    };
    auto init_hdr = persistent_data_header<ITONTokenWallet, wallet_replay_protection_t>::init();
    cell data_cl = prepare_persistent_data<ITONTokenWallet, wallet_replay_protection_t>(init_hdr, wallet_data);
    return tvm_state_init_hash(code_hash_, uint256(tvm_hash(data_cl)), code_depth_, uint16(data_cl.cdepth()));
  }

#ifdef TIP3_DEPLOY_TRANSFER
  __always_inline
  std::pair<StateInit, address> calc_wallet_init(uint256 pubkey, address_opt owner) {
    DTONTokenWallet wallet_data {
      name_, symbol_, decimals_,
      0u128, root_pubkey_, root_address_,
      pubkey, owner,
#ifdef TIP3_ENABLE_LEND_OWNERSHIP
      {}, {},
#endif
      code_,
      code_hash_, code_depth_,
#ifdef TIP3_ENABLE_ALLOWANCE
      {},
#endif
      workchain_id_
    };
    auto [init, hash] = prepare_wallet_state_init_and_addr(wallet_data, code_);
    return { init, address::make_std(workchain_id_, hash) };
  }
#endif // TIP3_DEPLOY_TRANSFER

  __always_inline
  std::tuple<opt<lend_pubkey>, lend_owners_array, uint128> filter_lend_arrays() {
#ifdef TIP3_ENABLE_LEND_OWNERSHIP
    if (!lend_pubkey_ && lend_owners_.empty())
      return {};
    auto now_v = tvm_now();
    auto pubkey = lend_pubkey_;
    if (pubkey && pubkey->lend_finish_time <= now_v)
      pubkey.reset();

    uint128 lend_balance;
    lend_owners_array owners;
    for (auto [addr, rec] : lend_owners_) {
      if (now_v < rec.lend_finish_time) {
        owners.push_back({addr, rec.lend_balance, rec.lend_finish_time});
        lend_balance += rec.lend_balance;
      }
    }
    return { pubkey, owners, lend_balance };
#else // TIP3_ENABLE_LEND_OWNERSHIP
    return {};
#endif // TIP3_ENABLE_LEND_OWNERSHIP
  }

  /// Filter lend ownership state to erase obsolete records
  __attribute__((noinline))
  static std::tuple<opt<lend_pubkey>, lend_owners_map, uint128> filter_lend_maps(
    opt<lend_pubkey> pubkey,
    lend_owners_map  lend_owners
  ) {
#ifdef TIP3_ENABLE_LEND_OWNERSHIP
    if (!pubkey && lend_owners.empty())
      return {};
    auto now = tvm_now();
    if (pubkey && pubkey->lend_finish_time <= now)
      pubkey.reset();

    uint128 lend_balance(0);
    {
      lend_owners_map new_owners;
      for (auto [addr, v] : lend_owners) {
        if (now < v.lend_finish_time) {
          new_owners.insert({addr, v});
          lend_balance += v.lend_balance;
        }
      }
      lend_owners = new_owners;
    }
    return { pubkey, lend_owners, lend_balance };
#else // TIP3_ENABLE_LEND_OWNERSHIP
    return {};
#endif // TIP3_ENABLE_LEND_OWNERSHIP
  }

  __always_inline bool is_internal_owner() const { return owner_address_.has_value(); }

  /// Check method authorization for internal call (when received internal message from another contract).
  __attribute__((noinline))
  static std::tuple<opt<lend_pubkey>, lend_owners_map> check_internal_owner(
    auth_cfg         cfg,
    uint128          balance,
    address          sender,
    opt<address>     owner_address,
    opt<lend_pubkey> lend_pubkey_in,
    lend_owners_map  lend_owners_in
  ) {
    [[maybe_unused]] auto [lend_pubkey, lend_owners, lend_balance] = filter_lend_maps(lend_pubkey_in, lend_owners_in);
    if ( owner_address &&
         (lend_balance == 0 || cfg.allowed_for_original_owner_in_lend_state) &&
         (*owner_address == sender) ) {
      require(cfg.required_tokens + lend_balance <= balance, error_code::not_enough_balance);
      require(cfg.required_evers <= tvm_balance(), error_code::not_enough_evers_to_process);
      // Original internal owner can use non-lend tokens
      return { lend_pubkey, lend_owners };
    }
#ifdef TIP3_ENABLE_LEND_OWNERSHIP
    // Lend owners
    if (cfg.allowed_lend_owner && !lend_owners.empty()) {
      auto rec = lend_owners.extract({sender});
      require(!!rec, error_code::message_sender_is_not_my_owner);

      auto allowed_balance = std::min(balance, rec->lend_balance);

      require(cfg.required_tokens <= allowed_balance, error_code::not_enough_balance);
      require(cfg.required_time < rec->lend_finish_time, error_code::finish_time_is_out_of_lend_time);
      if (rec->lend_balance > cfg.required_tokens + cfg.return_ownership) {
        rec->lend_balance -= cfg.required_tokens + cfg.return_ownership;
        lend_owners.set_at({sender}, *rec);
      }
      return { lend_pubkey, lend_owners };
    }
#endif // TIP3_ENABLE_LEND_OWNERSHIP
    tvm_throw(error_code::message_sender_is_not_my_owner);
    return {};
  }

  /// Check method authorization for external call.
  /// May be original owner pubkey or lend pubkey.
  __always_inline
  std::tuple<opt<lend_pubkey>, lend_owners_map> check_external_owner(
    auth_cfg         cfg,
    uint128          balance,
    bool             owner_pubkey,
    uint256          msg_pubkey,
    opt<lend_pubkey> lend_pubkey_in,
    lend_owners_map  lend_owners_in
  ) {
    if (owner_pubkey) {
      auto [lend_pubkey, lend_owners, lend_balance] = filter_lend_maps(lend_pubkey_in, lend_owners_in);
      require(cfg.allowed_for_original_owner_in_lend_state ||
              (!lend_pubkey && lend_owners.empty()),
              error_code::wallet_in_lend_owneship);
      require(cfg.required_tokens + lend_balance <= balance, error_code::not_enough_balance);
      require(cfg.required_evers <= tvm_balance(), error_code::not_enough_evers_to_process);
      tvm_accept();
      tvm_commit();
      return { lend_pubkey, lend_owners };
#ifdef TIP3_ENABLE_LEND_OWNERSHIP
    } else if (lend_pubkey_in && lend_pubkey_in->lend_pubkey == msg_pubkey) {
      require(tvm_now() < lend_pubkey_in->lend_finish_time, error_code::obsolete_lend_pubkey);
      require(cfg.required_time < lend_pubkey_in->lend_finish_time, error_code::finish_time_is_out_of_lend_time);
      require(cfg.required_evers <= tvm_balance(), error_code::not_enough_evers_to_process);
      tvm_accept();
      tvm_commit();

      auto [lend_pubkey, lend_owners, lend_balance] = filter_lend_maps(lend_pubkey_in, lend_owners_in);
      require(cfg.required_tokens + lend_balance <= balance, error_code::not_enough_balance);

      return { lend_pubkey, lend_owners };
#endif // TIP3_ENABLE_LEND_OWNERSHIP
    }
    tvm_throw(error_code::message_sender_is_not_my_owner);
    return {};
  }

  /// Check method authorization
  __always_inline
  void check_owner(auth_cfg cfg) {
    if constexpr (Internal) {
#ifdef TIP3_ENABLE_LEND_OWNERSHIP
      auto [lend_pubkey, lend_owners] =
        check_internal_owner(cfg, balance_, int_sender(), owner_address_, lend_pubkey_, lend_owners_);
      lend_pubkey_ = lend_pubkey;
      lend_owners_ = lend_owners;
#else
      [[maybe_unused]] auto [lend_pubkey, lend_owners] =
        check_internal_owner(cfg, balance_, int_sender(), owner_address_, {}, {});
#endif // TIP3_ENABLE_LEND_OWNERSHIP
      return;
    } else {
      bool owner_pubkey = (msg_pubkey() == wallet_pubkey_) && !is_internal_owner();
#ifdef TIP3_ENABLE_LEND_OWNERSHIP
      auto [lend_pubkey, lend_owners] =
        check_external_owner(cfg, balance_, owner_pubkey, uint256{msg_pubkey()}, lend_pubkey_, lend_owners_);
      lend_pubkey_ = lend_pubkey;
      lend_owners_ = lend_owners;
#else
      [[maybe_unused]] auto [lend_pubkey, lend_owners] =
        check_external_owner(cfg, balance_, owner_pubkey, uint256{msg_pubkey()}, {}, {});
#endif // TIP3_ENABLE_LEND_OWNERSHIP
    }
  }
};

DEFINE_JSON_ABI(ITONTokenWallet, DTONTokenWallet, ETONTokenWallet, wallet_replay_protection_t);

// ----------------------------- Main entry functions ---------------------- //
#ifdef TIP3_ENABLE_EXTERNAL
DEFAULT_MAIN_ENTRY_FUNCTIONS_TMPL(TONTokenWallet, ITONTokenWallet, DTONTokenWallet, TOKEN_WALLET_TIMESTAMP_DELAY)
#else
MAIN_ENTRY_FUNCTIONS_NO_REPLAY_TMPL(TONTokenWallet, ITONTokenWallet, DTONTokenWallet)
#endif

