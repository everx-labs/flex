/** \file
 *  \brief Exchange pair contract implementation
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
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
  static constexpr bool _checked_deploy = true; /// To allow deploy message only with `onDeploy` call
  struct error_code : tvm::error_code {
    static constexpr unsigned not_enough_crystals           = 101; ///< Not enough crystals to process
    static constexpr unsigned double_deploy                 = 102; ///< Double call of `onDeploy` method
    static constexpr unsigned zero_min_amount               = 103; ///< Zero minimum amount
    static constexpr unsigned bad_incoming_msg              = 104; ///< Bad incoming message
    static constexpr unsigned only_flex_may_deploy_me       = 105; ///< Only Flex may deploy this contract
    static constexpr unsigned not_initialized               = 106; ///< Is not correctly initialized
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
  bool onDeploy(uint128 min_amount, uint128 minmove, uint128 price_denum, uint128 deploy_value, address notify_addr,
                Tip3Config major_tip3cfg, Tip3Config minor_tip3cfg) {
    require(int_value().get() > deploy_value, error_code::not_enough_crystals);
    require(!min_amount_, error_code::double_deploy);
    require(min_amount > 0, error_code::zero_min_amount);

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

    major_reserve_wallet_ = calc_wrapper_reserve_wallet(major_tip3cfg);
    minor_reserve_wallet_ = calc_wrapper_reserve_wallet(minor_tip3cfg);
    major_tip3cfg_ = major_tip3cfg;
    minor_tip3cfg_ = minor_tip3cfg;

    min_amount_ = min_amount;
    minmove_ = minmove;
    price_denum_ = price_denum;
    notify_addr_ = notify_addr;
    tvm_rawreserve(deploy_value.get(), rawreserve_flag::up_to);
    set_int_return_flag(SEND_ALL_GAS);
    return true;
  }

  __always_inline
  address getFlexAddr() {
    return  getConfig().flex;
  }

  __always_inline
  address getTip3MajorRoot() {
    return tip3_major_root_;
  }

  __always_inline
  address getTip3MinorRoot() {
    return tip3_minor_root_;
  }

  __always_inline
  uint128 getMinAmount() {
    return min_amount_;
  }

  __always_inline
  uint128 getMinmove() {
    return minmove_;
  }

  __always_inline
  uint128 getPriceDenum() {
    return price_denum_;
  }

  __always_inline
  address getNotifyAddr() {
    return notify_addr_;
  }

  __always_inline
  address getMajorReserveWallet() {
    return major_reserve_wallet_;
  }

  __always_inline
  address getMinorReserveWallet() {
    return minor_reserve_wallet_;
  }

  __always_inline
  XchgPairSalt getConfig() {
    return parse_chain_static<XchgPairSalt>(parser(tvm_code_salt()));
  }

  __always_inline
  cell getPriceXchgCode(bool salted) {
    auto cfg = getConfig();
    require(major_tip3cfg_ && minor_tip3cfg_, error_code::not_initialized);
    if (salted)
      return tvm_add_code_salt(preparePriceXchgSalt(cfg), cfg.xchg_price_code);
    else
      return cfg.xchg_price_code;
  }

  __always_inline
  cell getPriceXchgSalt() {
    auto cfg = getConfig();
    require(major_tip3cfg_ && minor_tip3cfg_, error_code::not_initialized);

    return build_chain_static(preparePriceXchgSalt(cfg));
  }

  __always_inline
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

  // =============== Support functions ==================
  DEFAULT_SUPPORT_FUNCTIONS(IXchgPair, void)

  // default processing of unknown messages
  __always_inline static int _fallback(cell /*msg*/, slice /*msg_body*/) {
    return 0;
  }
  __always_inline static int _receive(cell /*msg*/, slice /*msg_body*/) {
    return 0;
  }
};

DEFINE_JSON_ABI(IXchgPair, DXchgPair, EXchgPair);

// ----------------------------- Main entry functions ---------------------- //
MAIN_ENTRY_FUNCTIONS_NO_REPLAY(XchgPair, IXchgPair, DXchgPair)
