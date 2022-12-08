/** \file
 *  \brief Flex root contract implementation
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) EverFlex Inc
 */

#include "Flex.hpp"
#include "XchgPair.hpp"
#include "Wrapper.hpp"
#include "WrapperEver.hpp"
#include <tvm/contract.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/default_support_functions.hpp>
#include <tvm/suffixes.hpp>
#include <tvm/schema/parse_chain_static.hpp>
#include <tvm/schema/build_chain_static.hpp>

using namespace tvm;

__always_inline
DXchgPair prepare_pair(address tip3_major_root, address tip3_minor_root) {
  auto null_addr = address::make_std(0i8, 0u256);
  return {
    .tip3_major_root_      = tip3_major_root,
    .tip3_minor_root_      = tip3_minor_root,
    .major_tip3cfg_        = {},
    .minor_tip3cfg_        = {},
    .min_amount_           = 0u128,
    .notify_addr_          = null_addr,
    .major_reserve_wallet_ = null_addr,
    .minor_reserve_wallet_ = null_addr
  };
}

/// Implements tvm::IFlex interface
class Flex final : public smart_interface<IFlex>, public DFlex {
  using data = DFlex;
public:
  DEFAULT_SUPPORT_FUNCTIONS(IFlex, void)
  static constexpr bool _checked_deploy = true; /// To allow deploy message only with `onDeploy` call

  struct error_code : tvm::error_code {
    static constexpr unsigned sender_is_not_my_owner        = 100; ///< Sender is not my owner
    static constexpr unsigned uninitialized                 = 101; ///< Uninitialized
    static constexpr unsigned incorrect_config              = 102; ///< Incorrect config
  };

  resumable<void> onDeploy(
    uint128        flex_keep_evers,
    PairCloneEvers evers,
    address_opt    old_flex
  ) {
    auto cfg = getConfig();
    require(int_sender() == cfg.super_root, error_code::sender_is_not_my_owner);
    flex_keep_evers_ = flex_keep_evers;
    {
      XchgPairSalt pair_salt = {
        .flex            = tvm_myaddr(),
        .ev_cfg          = cfg.ev_cfg,
        .deals_limit     = cfg.deals_limit,
        .xchg_price_code = cfg.xchg_price_code
      };
      xchg_pair_code_ = tvm_add_code_salt<XchgPairSalt>(pair_salt, cfg.xchg_pair_code);
      workchain_id_ = std::get<addr_std>(tvm_myaddr().val()).workchain_id;
    }
    if (!old_flex)
      co_return;

    // If we are cloning from previous Flex:
    //  * Requesting first/last pairs in linked list from previous Flex
    //  * For each old pair in the list:
    //    * Requesting details from an old pair
    //    * Deploying clone with new code
    //    * Linking previous clone with the next clone using `setNext` call

    auto [first_pair, last_pair] = co_await IFlexPtr(*old_flex)(_all_except(flex_keep_evers)).requestPairs();

    if (first_pair) {
      it_ = first_pair;
      while (it_) {
        IXchgPairPtr pair(*it_);
        const auto details = co_await pair(_all_except(flex_keep_evers)).requestDetails();
        if (!details.unlisted) {
          next_ = details.next;
          notify_addr_ = details.notify_addr;
          min_amount_ = details.min_amount;
          minmove_ = details.minmove;
          price_denum_ = details.price_denum;
          major_tip3cfg_ = details.major_tip3cfg;
          minor_tip3cfg_ = details.minor_tip3cfg;

          IWrapperPtr major_wrapper(major_tip3cfg_->root_address);
          auto [cloned_major_root, cloned_major_pubkey] = co_await major_wrapper(_all_except(flex_keep_evers)).cloned();
          if (cloned_major_root) {
            major_tip3cfg_->root_address = *cloned_major_root;
            major_tip3cfg_->root_pubkey = cloned_major_pubkey;
          }

          IWrapperPtr minor_wrapper(minor_tip3cfg_->root_address);
          auto [cloned_minor_root, cloned_minor_pubkey] = co_await minor_wrapper(_all_except(flex_keep_evers)).cloned();
          if (cloned_minor_root) {
            minor_tip3cfg_->root_address = *cloned_minor_root;
            major_tip3cfg_->root_pubkey = cloned_minor_pubkey;
          }

          auto [init, hash] = prepare<IXchgPair>(
            prepare_pair(major_tip3cfg_->root_address, minor_tip3cfg_->root_address),
            xchg_pair_code_.get()
          );

          IXchgPairPtr clone_ptr(address::make_std(workchain_id_, hash));
          clone_ptr.deploy(init, Evers(evers.deploy.get())).onDeploy(
            min_amount_, minmove_, price_denum_,
            evers.pair_keep,
            *notify_addr_, *major_tip3cfg_, *minor_tip3cfg_
          );
          last_pair_ = clone_ptr.get();
          if (!first_pair_)
            first_pair_ = last_pair_;
          if (prev_clone_)
            IXchgPairPtr(*prev_clone_)(Evers(evers.setnext.get())).setNext(clone_ptr.get());
          prev_clone_ = clone_ptr.get();
          ++pairs_count_;
        }

        it_ = next_;
      }
    }
    prev_clone_.reset();
    next_.reset();
    notify_addr_.reset();
    major_tip3cfg_.reset();
    minor_tip3cfg_.reset();
    co_return;
  }

  address addXchgPair(
    PairCloneEvers evers,
    Tip3Config     major_tip3cfg,
    Tip3Config     minor_tip3cfg,
    uint128        min_amount,
    uint128        minmove,
    uint128        price_denum,
    address        notify_addr
  ) {
    auto cfg = getConfig();
    require(min_amount > 0 && minmove > 0 && price_denum > 0, error_code::incorrect_config);
    require(int_sender() == cfg.super_root, error_code::sender_is_not_my_owner);
    require(xchg_pair_code_, error_code::uninitialized);

    tvm_rawreserve(tvm_balance() - int_value().get(), rawreserve_flag::up_to);

    auto pair_data = prepare_pair(major_tip3cfg.root_address, minor_tip3cfg.root_address);
    auto [init, hash] = prepare<IXchgPair>(pair_data, xchg_pair_code_.get());
    IXchgPairPtr ptr(address::make_std(workchain_id_, hash));
    if (!last_pair_) {
      ptr.deploy(init, Evers(evers.deploy.get())).onDeploy(
        min_amount, minmove, price_denum,
        evers.pair_keep,
        notify_addr, major_tip3cfg, minor_tip3cfg
      );
      first_pair_ = ptr.get();
      last_pair_ = ptr.get();
      pairs_count_ = 1;
    } else {
      ptr.deploy(init, Evers(evers.deploy.get())).onDeploy(
        min_amount, minmove, price_denum,
        evers.pair_keep,
        notify_addr, major_tip3cfg, minor_tip3cfg
      );
      IXchgPairPtr(*last_pair_)(Evers(evers.setnext.get())).setNext(ptr.get());
      last_pair_ = ptr.get();
      ++pairs_count_;
    }
    set_int_return_flag(SEND_ALL_GAS);
    return ptr.get();
  }

  void unlistXchgPair(address pair) {
    check_owner();
    IXchgPairPtr(pair)(_remaining_ev()).unlist();
  }

  PairsRange requestPairs() {
    return _remaining_ev() & PairsRange{ first_pair_, last_pair_ };
  }

  FlexSalt getConfig() {
    return parse_chain_static<FlexSalt>(parser(tvm_code_salt()));
  }

  FlexDetails getDetails() {
    require(xchg_pair_code_, error_code::uninitialized);
    return {
      xchg_pair_code_.get(),
      uint256{tvm_hash(getConfig().xchg_price_code)},
      first_pair_, last_pair_, pairs_count_
      };
  }

  address getXchgTradingPair(address tip3_major_root, address tip3_minor_root) {
    auto pair_data = prepare_pair(tip3_major_root, tip3_minor_root);
    auto std_addr = prepare<IXchgPair>(pair_data, xchg_pair_code_.get()).second;
    return address::make_std(workchain_id_, std_addr);
  }

  uint128 calcLendTokensForOrder(bool sell, uint128 major_tokens, price_t price) {
    return calc_lend_tokens_for_order(sell, major_tokens, price);
  }

  void check_owner() {
    require(int_sender() == getConfig().super_root, error_code::sender_is_not_my_owner);
  }

  // default processing of unknown messages
  static int _fallback([[maybe_unused]] cell msg, [[maybe_unused]] slice msg_body) {
    return 0;
  }
  // processing of empty messages
  static int _receive([[maybe_unused]] cell msg, [[maybe_unused]] slice msg_body) {
    auto cfg = parse_chain_static<FlexSalt>(parser(tvm_code_salt()));
    auto [hdr, persist] = load_persistent_data<IFlex, void, DFlex>();
    if (tvm_balance() > static_cast<int>(persist.flex_keep_evers_.get())) {
      tvm_rawreserve(persist.flex_keep_evers_.get(), rawreserve_flag::up_to);
      tvm_transfer(cfg.super_root, 0, false, SEND_ALL_GAS | IGNORE_ACTION_ERRORS);
    }
    return 0;
  }
};

DEFINE_JSON_ABI(IFlex, DFlex, EFlex);

// ----------------------------- Main entry functions ---------------------- //
MAIN_ENTRY_FUNCTIONS_NO_REPLAY(Flex, IFlex, DFlex)
