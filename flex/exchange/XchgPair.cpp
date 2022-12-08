/** \file
 *  \brief Exchange pair contract implementation
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) EverFlex Inc
 */

#include "XchgPair.hpp"
#include "calc_wrapper_reserve_wallet.hpp"
#include "PriceXchgSalt.hpp"
#include <tvm/contract.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/default_support_functions.hpp>
#include <tvm/schema/build_chain_static.hpp>

using namespace tvm;

/// Implements tvm::IXchgPair interface
class XchgPair final : public smart_interface<IXchgPair>, public DXchgPair {
  using data = DXchgPair;
public:
  DEFAULT_SUPPORT_FUNCTIONS(IXchgPair, void)

  static constexpr bool _checked_deploy = true; /// To allow deploy message only with `onDeploy` call
  struct error_code : tvm::error_code {
    static constexpr unsigned message_sender_is_not_my_owner = 100; ///< Message sender is not my owner
    static constexpr unsigned not_enough_evers               = 101; ///< Not enough evers to process
    static constexpr unsigned double_deploy                  = 102; ///< Double call of `onDeploy` method
    static constexpr unsigned zero_min_amount                = 103; ///< Zero minimum amount
    static constexpr unsigned only_flex_may_deploy_me        = 104; ///< Only Flex may deploy this contract
    static constexpr unsigned not_initialized                = 105; ///< Is not correctly initialized
  };

  void onDeploy(
    uint128    min_amount,
    uint128    minmove,
    uint128    price_denum,
    uint128    deploy_value,
    address    notify_addr,
    Tip3Config major_tip3cfg,
    Tip3Config minor_tip3cfg
  ) {
    require(int_value().get() > deploy_value, error_code::not_enough_evers);
    require(!min_amount_, error_code::double_deploy);
    require(min_amount > 0, error_code::zero_min_amount);

    auto cfg = getConfig();
    require(cfg.flex == int_sender(), error_code::only_flex_may_deploy_me);

    major_reserve_wallet_ = calc_wrapper_reserve_wallet(major_tip3cfg);
    minor_reserve_wallet_ = calc_wrapper_reserve_wallet(minor_tip3cfg);
    major_tip3cfg_ = major_tip3cfg;
    minor_tip3cfg_ = minor_tip3cfg;

    min_amount_ = min_amount;
    minmove_ = minmove;
    price_denum_ = price_denum;
    notify_addr_ = notify_addr;

    return _all_except(deploy_value).with_void();
  }

  XchgPairDetails requestDetails() {
    return _remaining_ev() & getDetails();
  }

  XchgPairDetails getDetails() {
    require(major_tip3cfg_ && minor_tip3cfg_, error_code::not_initialized);
    return {
      tip3_major_root_,
      tip3_minor_root_,
      min_amount_,
      minmove_,
      price_denum_,
      notify_addr_,
      major_reserve_wallet_,
      minor_reserve_wallet_,
      *major_tip3cfg_,
      *minor_tip3cfg_,
      next_,
      unlisted_.get()
    };
  }

  void setNext(address next) {
    require(int_sender() == getConfig().flex, error_code::message_sender_is_not_my_owner);
    next_ = next;
    return _remaining_ev().with_void();
  }

  void unlist() {
    auto flex = getConfig().flex;
    require(int_sender() == flex, error_code::message_sender_is_not_my_owner);
    unlisted_ = true;
    return _remaining_ev().with_void();
  }

  address getFlexAddr() {
    return  getConfig().flex;
  }

  address getTip3MajorRoot() {
    return tip3_major_root_;
  }

  address getTip3MinorRoot() {
    return tip3_minor_root_;
  }

  uint128 getMinAmount() {
    return min_amount_;
  }

  uint128 getMinmove() {
    return minmove_;
  }

  uint128 getPriceDenum() {
    return price_denum_;
  }

  address getNotifyAddr() {
    return notify_addr_;
  }

  address getMajorReserveWallet() {
    return major_reserve_wallet_;
  }

  address getMinorReserveWallet() {
    return minor_reserve_wallet_;
  }

  XchgPairSalt getConfig() {
    return parse_chain_static<XchgPairSalt>(parser(tvm_code_salt()));
  }

  cell getPriceXchgCode(bool salted) {
    auto cfg = getConfig();
    require(major_tip3cfg_ && minor_tip3cfg_, error_code::not_initialized);
    if (salted)
      return tvm_add_code_salt(preparePriceXchgSalt(cfg), cfg.xchg_price_code);
    else
      return cfg.xchg_price_code;
  }

  cell getPriceXchgSalt() {
    auto cfg = getConfig();
    require(major_tip3cfg_ && minor_tip3cfg_, error_code::not_initialized);

    return build_chain_static(preparePriceXchgSalt(cfg));
  }

  PriceXchgSalt preparePriceXchgSalt(XchgPairSalt cfg) {
    return {
      .flex                 = cfg.flex,
      .pair                 = tvm_myaddr(),
      .notify_addr          = getNotifyAddr(),
      .major_tip3cfg        = *major_tip3cfg_,
      .minor_tip3cfg        = *minor_tip3cfg_,
      .major_reserve_wallet = getMajorReserveWallet(),
      .minor_reserve_wallet = getMinorReserveWallet(),
      .ev_cfg               = cfg.ev_cfg,
      .min_amount           = getMinAmount(),
      .minmove              = getMinmove(),
      .price_denum          = getPriceDenum(),
      .deals_limit          = cfg.deals_limit,
      .workchain_id         = std::get<addr_std>(tvm_myaddr().val()).workchain_id
    };
  }

  // default processing of unknown messages
  static int _fallback([[maybe_unused]] cell msg, [[maybe_unused]] slice msg_body) {
    return 0;
  }
  static int _receive([[maybe_unused]] cell msg, [[maybe_unused]] slice msg_body) {
    return 0;
  }
};

DEFINE_JSON_ABI(IXchgPair, DXchgPair, EXchgPair);

// ----------------------------- Main entry functions ---------------------- //
MAIN_ENTRY_FUNCTIONS_NO_REPLAY(XchgPair, IXchgPair, DXchgPair)
