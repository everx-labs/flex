/** \file
 *  \brief stTONs contract implementation
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#include "stTONs.hpp"
#include <tvm/contract.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/default_support_functions.hpp>
#include <tvm/suffixes.hpp>

#include "DePool.hpp"

using namespace tvm;

enum class DePoolError {
  STATUS_SUCCESS                           = 0,
  STATUS_DEPOOL_CLOSED                     = 3,
  STATUS_NO_PARTICIPANT                    = 6,
  STATUS_REMAINING_STAKE_LESS_THAN_MINIMAL = 16,
  STATUS_TRANSFER_AMOUNT_IS_TOO_BIG        = 18,
  STATUS_TRANSFER_SELF                     = 19,
  STATUS_TRANSFER_TO_OR_FROM_VALIDATOR     = 20,
  STATUS_INVALID_ADDRESS                   = 22,
  STATUS_TRANSFER_WHILE_COMPLETING_STEP    = 26,
  STATUS_NOT_REPORTED_YET                  = 0xFF
};

template<bool Internal>
class stTONs final : public smart_interface<IstTONs>, public DstTONs {
  using data = DstTONs;
public:
  struct error_code : tvm::error_code {
    static constexpr unsigned sender_is_not_my_owner               = 100; ///< Authorization error
    static constexpr unsigned transferring_stake_back              = 101; ///< Contract is in "transferring stake back" state
    static constexpr unsigned not_transferred_stake_back           = 102; ///< IstTONs::returnStake() should be called first
    static constexpr unsigned not_enough_balance                   = 103; ///< Not enough token balance to proceed
    static constexpr unsigned finish_time_must_be_greater_than_now = 104; ///< Lend finish time must be in some future
    static constexpr unsigned internal_owner_enabled               = 105; ///< Internal ownership is enabled
    static constexpr unsigned internal_owner_disabled              = 106; ///< Internal ownership is disabled
    static constexpr unsigned wallet_in_lend_owneship              = 107; ///< The wallet is in lend ownership state
    static constexpr unsigned incorrect_depool_address             = 108; ///< Received DePool notification from incorrect address
    static constexpr unsigned has_depool_error                     = 109; ///< Contract received an error notification from DePool
    static constexpr unsigned only_original_owner_allowed          = 110; ///< This method may be called only by original owner (not by lend owner)
    static constexpr unsigned wrong_bounced_header                 = 111; ///< Incorrect header in bounced message
    static constexpr unsigned wrong_bounced_args                   = 112; ///< Incorrect arguments in bounced message
  };

  /// Implements tvm::IstTONs::onDeploy()
  __always_inline
  void onDeploy() {
  }

  /// Implements tvm::IstTONs::lendOwnership()
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
    require(!transferring_stake_back_, error_code::transferring_stake_back);
    auto allowed_balance = check_owner(/*original_owner_only*/true, /*allowed_for_original_owner_in_lend_state*/true);
    // Current allocated lend balance plus new lend balance LEQ all wallet balance
    require(lend_balance > 0 && lend_balance <= allowed_balance, error_code::not_enough_balance);
    require(lend_finish_time > tvm_now(), error_code::finish_time_must_be_greater_than_now);
    tvm_accept();

    auto answer_addr_fxd = fixup_answer_addr(answer_addr);

    // repeated lend to the same address will be { sumX + sumY, max(timeX, timeY) }
    auto sum_lend_balance = lend_balance;
    auto sum_lend_finish_time = lend_finish_time;
    if (auto existing_lend = lend_ownership_.lookup(dest)) {
      sum_lend_balance += existing_lend->lend_balance;
      sum_lend_finish_time = std::max(lend_finish_time, existing_lend->lend_finish_time);
    }

    lend_ownership_.set_at(dest, {sum_lend_balance, sum_lend_finish_time});

    unsigned msg_flags = prepare_transfer_message_flags(evers);
    auto deploy_init_sl = deploy_init_cl.ctos();
    StateInit deploy_init;
    if (!deploy_init_sl.empty())
      deploy_init = parse<StateInit>(deploy_init_sl);
    if (deploy_init.code && deploy_init.data) {
      // performing `tail call` - requesting dest to answer to our caller
      temporary_data::setglob(global_id::answer_id, return_func_id()->get());
      IstTONsNotifyPtr(dest).deploy(deploy_init, Evers(evers.get()), msg_flags).
        onLendOwnership(lend_balance, lend_finish_time,
                        owner_pubkey_, owner_address_, depool_.get(), depool_pubkey_, payload,
                        answer_addr_fxd);
    } else {
      // performing `tail call` - requesting dest to answer to our caller
      temporary_data::setglob(global_id::answer_id, return_func_id()->get());
      IstTONsNotifyPtr(dest)(Evers(evers.get()), msg_flags).
        onLendOwnership(lend_balance, lend_finish_time,
                        owner_pubkey_, owner_address_, depool_.get(), depool_pubkey_, payload,
                        answer_addr_fxd);
    }
  }

  /// Implements tvm::IstTONs::returnOwnership()
  __always_inline
  void returnOwnership(uint128 tokens) {
    check_owner(/*original_owner_only*/false, /*allowed_for_original_owner_in_lend_state*/false);
    auto sender = int_sender();
    auto v = lend_ownership_[sender];
    if (v.lend_balance <= tokens) {
      lend_ownership_.erase(sender);
    } else {
      v.lend_balance -= tokens;
      lend_ownership_.set_at(sender, v);
    }
  }

  /// Implements tvm::IstTONs::returnStake()
  __always_inline
  void returnStake(address dst, uint128 processing_evers) {
    check_owner(/*original_owner_only*/true, /*allowed_for_original_owner_in_lend_state*/false);
    depool_(Evers(processing_evers.get())).transferStake(dst, 0u64);
    transferring_stake_back_ = true;
    last_depool_error_ = static_cast<unsigned>(DePoolError::STATUS_NOT_REPORTED_YET);
  }

  /// Implements tvm::IstTONs::finalize()
  __always_inline
  void finalize(address dst, bool ignore_errors) {
    require(transferring_stake_back_, error_code::not_transferred_stake_back);
    check_owner(/*original_owner_only*/true, /*allowed_for_original_owner_in_lend_state*/false);
    require(ignore_errors ||
      (last_depool_error_ == static_cast<unsigned>(DePoolError::STATUS_SUCCESS)) ||
      (last_depool_error_ == static_cast<unsigned>(DePoolError::STATUS_DEPOOL_CLOSED)) ||
      (last_depool_error_ == static_cast<unsigned>(DePoolError::STATUS_NO_PARTICIPANT)),
      error_code::has_depool_error);
    tvm_accept();
    suicide(dst);
  }

  /// Implements tvm::IParticipant::onTransfer()
  __always_inline
  void receiveStakeTransfer(address source, uint128 amount) {
    require(int_sender() == depool_.get(), error_code::incorrect_depool_address);
    tvm_accept();

    balance_ += amount;
  }

  /// Implements tvm::IParticipant::receiveAnswer()
  __always_inline
  void receiveAnswer(uint32 errcode, uint64 comment) {
    require(int_sender() == depool_.get(), error_code::incorrect_depool_address);
    tvm_accept();

    last_depool_error_ = uint8(errcode.get());
  }

  /// Implements tvm::IstTONs::getDetails()
  __always_inline
  stTONsDetails getDetails() {
    auto [filtered_lend_array, lend_balance] = filter_lend_ownerhip_array();
    return {
      owner_pubkey_,
      get_owner_addr(),
      depool_.get(),
      depool_pubkey_,
      balance_,
      filtered_lend_array,
      lend_balance,
      transferring_stake_back_,
      last_depool_error_
      };
  }

  /// Implements tvm::IstTONs::calcStTONsAddr()
  __always_inline
  address calcStTONsAddr(
    cell code,
    uint256 owner_pubkey,
    std::optional<address> owner_address,
    address depool,
    uint256 depool_pubkey
  ) {
    DstTONs data {
      .owner_pubkey_ = owner_pubkey,
      .owner_address_ = owner_address,
      .depool_ = depool,
      .depool_pubkey_ = depool_pubkey,
      .balance_ = 0u128,
      .lend_ownership_ = {},
      .transferring_stake_back_ = false,
      .last_depool_error_ = 0u8
    };
    auto [init, hash] = prepare_sttons_state_init_and_addr(data, code);
    auto workchain_id = std::get<addr_std>(address{tvm_myaddr()}.val()).workchain_id;
    return address::make_std(workchain_id, hash);
  }

  __always_inline bool is_internal_owner() const { return owner_address_.has_value(); }

  // original_owner_only - methods only allowed to call by original owner (no lend)
  // allowed_for_original_owner_in_lend_state - methods allowed to call by original owner in lend state
  __always_inline
  uint128 check_internal_owner(bool original_owner_only, bool allowed_for_original_owner_in_lend_state) {
    auto [filtered_map, actual_lend_balance] = filter_lend_ownerhip_map();
    if (actual_lend_balance > 0) {
      if (allowed_for_original_owner_in_lend_state) {
        require(is_internal_owner(), error_code::internal_owner_disabled);
        if (*owner_address_ == int_sender())
          return balance_ - actual_lend_balance;
      }
      require(!original_owner_only, error_code::only_original_owner_allowed);
      auto elem = filtered_map.lookup(int_sender());
      require(!!elem, error_code::sender_is_not_my_owner);
      return std::min(balance_, elem->lend_balance);
    } else {
      require(is_internal_owner(), error_code::internal_owner_disabled);
      require(*owner_address_ == int_sender(),
              error_code::sender_is_not_my_owner);
      return balance_;
    }
  }

  __always_inline
  uint128 check_external_owner(bool allowed_for_original_owner_in_lend_state) {
    require(!is_internal_owner(), error_code::internal_owner_enabled);
    require(msg_pubkey() == owner_pubkey_, error_code::sender_is_not_my_owner);
    tvm_accept();
    auto [filtered_map, actual_lend_balance] = filter_lend_ownerhip_map();
    if (actual_lend_balance > 0 && allowed_for_original_owner_in_lend_state)
      return balance_ - actual_lend_balance;
    require(actual_lend_balance == 0, error_code::wallet_in_lend_owneship);
    return balance_;
  }

  __always_inline
  uint128 check_owner(bool original_owner_only, bool allowed_in_lend_state) {
    if constexpr (Internal)
      return check_internal_owner(original_owner_only, allowed_in_lend_state);
    else
      return check_external_owner(allowed_in_lend_state);
  }

  __always_inline
  unsigned prepare_transfer_message_flags(uint128 &evers) {
    unsigned msg_flags = 0;
    if constexpr (Internal) {
      tvm_rawreserve(tvm_balance() - int_value().get(), rawreserve_flag::up_to);
      msg_flags = SEND_ALL_GAS;
      evers = 0;
    }
    return msg_flags;
  }

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

  // Filter lend ownership map to keep only actual (unexpired) records and common lend balance
  __always_inline
  std::pair<lend_ownership_map, uint128> filter_lend_ownerhip_map() {
    if (lend_ownership_.empty())
      return {};
    auto now_v = tvm_now();
    lend_ownership_map rv;
    uint128 lend_balance;
    for (auto v : lend_ownership_) {
      if (now_v < v.second.lend_finish_time) {
        rv.insert(v);
        lend_balance += v.second.lend_balance;
      }
    }
    lend_ownership_ = rv;
    return { rv, lend_balance };
  }

  __always_inline
  std::pair<lend_ownership_array, uint128> filter_lend_ownerhip_array() {
    if (lend_ownership_.empty())
      return {};
    auto now_v = tvm_now();
    lend_ownership_array rv;
    uint128 lend_balance;
    for (auto v : lend_ownership_) {
      if (now_v < v.second.lend_finish_time) {
        rv.push_back({v.first, v.second.lend_balance, v.second.lend_finish_time});
        lend_balance += v.second.lend_balance;
      }
    }
    return { rv, lend_balance };
  }

  __always_inline
  address get_owner_addr() {
    return owner_address_ ? *owner_address_ :
                            address::make_std(int8(0), uint256(0));
  }

  // =============== Support functions ==================
  DEFAULT_SUPPORT_FUNCTIONS(IstTONs, sttons_replay_protection_t)

  /// Received bounced message back
  __always_inline static int _on_bounced(cell msg, slice msg_body) {
    tvm_accept();

    parser p(msg_body);
    require(p.ldi(32) == -1, error_code::wrong_bounced_header);
    auto [opt_hdr, =p] = parse_continue<abiv2::internal_msg_header>(p);
    require(!!opt_hdr, error_code::wrong_bounced_header);
    auto [hdr, persist] = load_persistent_data<IstTONs, sttons_replay_protection_t, DstTONs>();

    // If it is bounced onTip3LendOwnership, then we need to reset lend ownership
    if (opt_hdr->function_id == id_v<&IstTONsNotify::onLendOwnership>) {
      auto parsed_msg = parse<int_msg_info>(parser(msg), error_code::bad_incoming_msg);
      auto sender = incoming_msg(parsed_msg).int_sender();

      [[maybe_unused]] auto [answer_id, =p] = parse_continue<uint32>(p);
      // Parsing only first `balance` variable of onLendOwnership, other arguments won't fit into bounced response
      auto bounced_val = parse<uint128>(p, error_code::wrong_bounced_args);

      auto v = persist.lend_ownership_[sender];
      if (v.lend_balance <= bounced_val) {
        persist.lend_ownership_.erase(sender);
      } else {
        v.lend_balance -= bounced_val;
        persist.lend_ownership_.set_at(sender, v);
      }
      save_persistent_data<IstTONs, sttons_replay_protection_t>(hdr, persist);
    }
    return 0;
  }

  /// Default processing of unknown messages
  __always_inline static int _fallback(cell /*msg*/, slice /*msg_body*/) {
    return 0;
  }
};

DEFINE_JSON_ABI(IstTONs, DstTONs, EstTONs);

// ----------------------------- Main entry functions ---------------------- //
DEFAULT_MAIN_ENTRY_FUNCTIONS_TMPL(stTONs, IstTONs, DstTONs, STTONS_TIMESTAMP_DELAY)

