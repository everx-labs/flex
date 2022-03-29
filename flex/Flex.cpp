/** \file
 *  \brief Flex root contract implementation
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
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

__attribute__((noinline))
std::pair<address, DFlex::xchg_pairs_map> approveXchgPairImpl(
  uint256 pubkey,
  DFlex::xchg_pairs_map xchg_pair_listing_requests,
  cell xchg_pair_code,
  int8 workchain_id,
  ListingConfig listing_cfg);

__attribute__((noinline))
DFlex::xchg_pairs_map rejectXchgPairImpl(
  uint256 pubkey,
  DFlex::xchg_pairs_map xchg_pair_listing_requests,
  ListingConfig listing_cfg);

__attribute__((noinline))
std::pair<address, DFlex::wrappers_map> approveWrapperImpl(
  uint256 pubkey,
  DFlex::wrappers_map wrapper_listing_requests,
  cell wrapper_code,
  cell ext_wallet_code,
  cell flex_wallet_code,
  int8 workchain_id,
  ListingConfig listing_cfg);

__attribute__((noinline))
std::pair<address, DFlex::wrappers_map> approveWrapperEverImpl(
  uint256 pubkey,
  DFlex::wrappers_map wrapper_listing_requests,
  cell wrapper_code,
  cell flex_wallet_code,
  int8 workchain_id,
  ListingConfig listing_cfg);

__attribute__((noinline))
DFlex::wrappers_map rejectWrapperImpl(
  uint256 pubkey,
  DFlex::wrappers_map wrapper_listing_requests,
  ListingConfig listing_cfg);

/// Implements tvm::IFlex interface
template<bool Internal>
class Flex final : public smart_interface<IFlex>, public DFlex {
  using data = DFlex;
public:
  struct error_code : tvm::error_code {
    static constexpr unsigned sender_is_not_deployer        = 100; ///< Sender is not deployer
    static constexpr unsigned cant_override_code            = 101; ///< Can't override code
    static constexpr unsigned xchg_price_code_undefined     = 102; ///< Code of PriceXchg should be defined before
    static constexpr unsigned sender_is_not_my_owner        = 103; ///< Sender is not my owner
    static constexpr unsigned not_enough_funds              = 104; ///< Not enough evers to process
    static constexpr unsigned wrapper_not_requested         = 105; ///< Wrapper is not requested
    static constexpr unsigned xchg_pair_not_requested       = 106; ///< Exchange pair is not requested
    static constexpr unsigned costs_inconsistency           = 107; ///< Costs inconsistency
    static constexpr unsigned incorrect_config              = 108; ///< Incorrect config

    /// Wrapper with such public key already requested
    static constexpr unsigned wrapper_with_such_pubkey_already_requested = 109;
    /// Exchange pair with such public key already requested
    static constexpr unsigned xchg_pair_with_such_pubkey_already_requested = 110;
  };

  __always_inline
  void constructor(
      uint256       deployer_pubkey,
      string        ownership_description,
      address_opt   owner_address,
      EversConfig   ev_cfg,
      uint8         deals_limit,
      ListingConfig listing_cfg
  ) {
    deployer_pubkey_ = deployer_pubkey;
    ownership_description_ = ownership_description;
    owner_address_ = owner_address;
    deals_limit_ = deals_limit;
    ev_cfg_ = ev_cfg;
    listing_cfg_ = listing_cfg;
    workchain_id_ = std::get<addr_std>(tvm_myaddr().val()).workchain_id;

    require(listing_cfg.register_pair_cost >= listing_cfg.reject_pair_cost + listing_cfg.register_return_value,
            error_code::costs_inconsistency);
  }

  __always_inline
  void setSpecificCode(uint8 type, cell code) {
    switch (static_cast<code_type>(type.get())) {
      case code_type::xchg_pair_code: setXchgPairCode(code); return;
      case code_type::wrapper_code: setWrapperCode(code); return;
      case code_type::ext_wallet_code: setExtWalletCode(code); return;
      case code_type::flex_wallet_code: setFlexWalletCode(code); return;
      case code_type::xchg_price_code: setXchgPriceCode(code); return;
      case code_type::wrapper_ever_code: setWrapperEverCode(code); return;
    }
  }

  __always_inline
  void setXchgPairCode(cell code) {
    require(!xchg_pair_code_, error_code::cant_override_code);
    require(xchg_price_code_, error_code::xchg_price_code_undefined);
    require(msg_pubkey() == deployer_pubkey_, error_code::sender_is_not_deployer);
    require(code.ctos().srefs() == 2, error_code::unexpected_refs_count_in_code);
    tvm_accept();

    // Adding pair configuration in salt
    XchgPairSalt pair_cfg {
      .flex            = tvm_myaddr(),
      .ev_cfg          = ev_cfg_,
      .deals_limit     = deals_limit_,
      .xchg_price_code = xchg_price_code_.get()
    };
    cell salt = build_chain_static(pair_cfg);
    xchg_pair_code_ = builder().stslice(code.ctos()).stref(salt).endc();

    // TMP - DELETE
    parser p(xchg_pair_code_.get().ctos());
    p.ldref();
    p.ldref();
    auto decoded_salt = parse_chain_static<XchgPairSalt>(parser(p.ldrefrtos()));
    require(decoded_salt.xchg_price_code.ctos().srefs() == 2, 88);
  }

  __always_inline
  void setWrapperCode(cell code) {
    require(!wrapper_code_, error_code::cant_override_code);
    require(msg_pubkey() == deployer_pubkey_, error_code::sender_is_not_deployer);
    tvm_accept();

    require(code.ctos().srefs() == 2, error_code::unexpected_refs_count_in_code);
    wrapper_code_ = builder().stslice(code.ctos()).stref(build(tvm_myaddr()).endc()).endc();
  }

  __always_inline
  void setWrapperEverCode(cell code) {
    require(!wrapper_ever_code_, error_code::cant_override_code);
    require(msg_pubkey() == deployer_pubkey_, error_code::sender_is_not_deployer);
    tvm_accept();

    require(code.ctos().srefs() == 2, error_code::unexpected_refs_count_in_code);
    wrapper_ever_code_ = builder().stslice(code.ctos()).stref(build(tvm_myaddr()).endc()).endc();
  }

  __always_inline
  void setXchgPriceCode(cell code) {
    require(!xchg_price_code_, error_code::cant_override_code);
    require(msg_pubkey() == deployer_pubkey_, error_code::sender_is_not_deployer);
    require(code.ctos().srefs() == 2, error_code::unexpected_refs_count_in_code);
    tvm_accept();

    xchg_price_code_ = code;
  }

  __always_inline
  void setExtWalletCode(cell code) {
    require(!ext_wallet_code_, error_code::cant_override_code);
    require(msg_pubkey() == deployer_pubkey_, error_code::sender_is_not_deployer);
    tvm_accept();

    ext_wallet_code_ = code;
  }

  __always_inline
  void setFlexWalletCode(cell code) {
    require(!flex_wallet_code_, error_code::cant_override_code);
    require(msg_pubkey() == deployer_pubkey_, error_code::sender_is_not_deployer);
    tvm_accept();

    flex_wallet_code_ = code;
  }

  __always_inline
  void transfer(address to, uint128 evers) {
    check_owner();
    tvm_accept();
    tvm_transfer(to, evers.get(), true);
  }

  __always_inline
  void transferReserveTokens(address wrapper, uint128 tokens, address to, uint128 evers) {
    check_owner();
    tvm_accept();
    address_opt answer_addr;
    if constexpr (Internal)
      answer_addr = int_sender();
    IWrapperPtr(wrapper)(Evers(evers.get())).transferFromReserveWallet(answer_addr, to, tokens);
  }

  // Listing procedures
  __always_inline
  address registerXchgPair(
    uint256    pubkey,
    address    tip3_major_root,
    address    tip3_minor_root,
    Tip3Config major_tip3cfg,
    Tip3Config minor_tip3cfg,
    uint128    min_amount,
    uint128    minmove,
    uint128    price_denum,
    address    notify_addr
  ) {
    require(int_value().get() > listing_cfg_.register_pair_cost, error_code::not_enough_funds);
    require(!xchg_pair_listing_requests_.contains(pubkey.get()),
            error_code::xchg_pair_with_such_pubkey_already_requested);
    require(min_amount > 0 && minmove > 0, error_code::incorrect_config);

    xchg_pair_listing_requests_.set_at(pubkey.get(), {
      int_sender(), uint128(int_value().get()) - listing_cfg_.register_return_value,
      tip3_major_root, tip3_minor_root, major_tip3cfg, minor_tip3cfg,
      min_amount, minmove, price_denum, notify_addr
    });

    auto pair_data = prepare_pair(tip3_major_root, tip3_minor_root);

    set_int_return_value(listing_cfg_.register_return_value.get());

    auto [state_init, std_addr] = prepare<IXchgPair>(pair_data, xchg_pair_code_.get());
    return address::make_std(workchain_id_, std_addr);
  }

  __always_inline
  address approveXchgPair(uint256 pubkey) {
    check_owner();
    tvm_accept();

    auto [xchg_pair, xchg_pair_listing_requests] =
      approveXchgPairImpl(pubkey, xchg_pair_listing_requests_, xchg_pair_code_.get(), workchain_id_, listing_cfg_);
    xchg_pair_listing_requests_ = xchg_pair_listing_requests;

    if constexpr (Internal) {
      auto value_gr = int_value();
      tvm_rawreserve(tvm_balance() - value_gr(), rawreserve_flag::up_to);
      set_int_return_flag(SEND_ALL_GAS);
    }
    return xchg_pair;
  }

  __always_inline
  bool rejectXchgPair(uint256 pubkey) {
    check_owner();
    xchg_pair_listing_requests_ = rejectXchgPairImpl(pubkey, xchg_pair_listing_requests_, listing_cfg_);
    if constexpr (Internal) {
      auto value_gr = int_value();
      tvm_rawreserve(tvm_balance() - value_gr(), rawreserve_flag::up_to);
      set_int_return_flag(SEND_ALL_GAS);
    }
    return true;
  }

  __always_inline
  address registerWrapper(uint256 pubkey, Tip3Config tip3cfg) {
    require(int_value().get() > listing_cfg_.register_wrapper_cost, error_code::not_enough_funds);
    require(!wrapper_listing_requests_.contains(pubkey.get()),
            error_code::wrapper_with_such_pubkey_already_requested);

    wrapper_listing_requests_.set_at(pubkey.get(), {
      int_sender(), uint128(int_value().get()) - listing_cfg_.register_return_value,
      tip3cfg
    });

    DWrapper wrapper_data {
      .name_ = tip3cfg.name,
      .symbol_ = tip3cfg.symbol,
      .decimals_ = tip3cfg.decimals,
      .workchain_id_ = workchain_id_,
      .root_pubkey_ = pubkey,
      .total_granted_ = {},
      .internal_wallet_code_ = {},
      .start_balance_ = Evers(listing_cfg_.wrapper_keep_balance.get()),
      .flex_ = {},
      .wallet_ = {}
    };

    set_int_return_value(listing_cfg_.register_return_value.get());

    auto [wrapper_init, wrapper_hash_addr] = prepare_wrapper_state_init_and_addr(wrapper_code_.get(), wrapper_data);
    return address::make_std(workchain_id_, wrapper_hash_addr);
  }

  __always_inline
  address registerWrapperEver(uint256 pubkey) {
    require(int_value().get() > listing_cfg_.register_wrapper_cost, error_code::not_enough_funds);
    require(!wrapper_listing_requests_.contains(pubkey.get()),
            error_code::wrapper_with_such_pubkey_already_requested);

    Tip3Config tip3cfg {
      .name = { 'E', 'V', 'E', 'R' },
      .symbol = { 'E', 'V', 'E', 'R' },
      .decimals = 9u8,
      .root_pubkey = 0u256,
      .root_address = address::make_std(int8(0), uint256(0))
    };

    wrapper_listing_requests_.set_at(pubkey.get(), {
      int_sender(), uint128(int_value().get()) - listing_cfg_.register_return_value,
      tip3cfg
    });

    DWrapper wrapper_data {
      .name_ = tip3cfg.name,
      .symbol_ = tip3cfg.symbol,
      .decimals_ = tip3cfg.decimals,
      .workchain_id_ = workchain_id_,
      .root_pubkey_ = pubkey,
      .total_granted_ = {},
      .internal_wallet_code_ = {},
      .start_balance_ = Evers(listing_cfg_.wrapper_keep_balance.get()),
      .flex_ = {},
      .wallet_ = {}
    };

    set_int_return_value(listing_cfg_.register_return_value.get());

    auto [wrapper_init, wrapper_hash_addr] = prepare_wrapper_ever_state_init_and_addr(wrapper_ever_code_.get(), wrapper_data);
    return address::make_std(workchain_id_, wrapper_hash_addr);
  }

  __always_inline
  address approveWrapper(uint256 pubkey) {
    check_owner();
    tvm_accept();

    auto [wrapper_addr, new_wrapper_listing_requests] =
      approveWrapperImpl(pubkey, wrapper_listing_requests_,
                         wrapper_code_.get(), ext_wallet_code_.get(), flex_wallet_code_.get(),
                         workchain_id_, listing_cfg_);
    wrapper_listing_requests_ = new_wrapper_listing_requests;

    if constexpr (Internal) {
      auto value_gr = int_value();
      tvm_rawreserve(tvm_balance() - value_gr(), rawreserve_flag::up_to);
      set_int_return_flag(SEND_ALL_GAS);
    }
    return wrapper_addr;
  }

  __always_inline
  address approveWrapperEver(uint256 pubkey) {
      check_owner();
      tvm_accept();

      auto [wrapper_addr, new_wrapper_listing_requests] = approveWrapperEverImpl(
          pubkey,
          wrapper_listing_requests_,
          wrapper_ever_code_.get(),
          flex_wallet_code_.get(),
          workchain_id_,
          listing_cfg_
      );
      wrapper_listing_requests_ = new_wrapper_listing_requests;

      if constexpr (Internal) {
          auto value_gr = int_value();
          tvm_rawreserve(tvm_balance() - value_gr(), rawreserve_flag::up_to);
          set_int_return_flag(SEND_ALL_GAS);
      }

      return wrapper_addr;
  }

  __always_inline
  bool rejectWrapper(uint256 pubkey) {
    check_owner();
    tvm_accept();
    wrapper_listing_requests_ = rejectWrapperImpl(pubkey, wrapper_listing_requests_, listing_cfg_);
    if constexpr (Internal) {
      auto value_gr = int_value();
      tvm_rawreserve(tvm_balance() - value_gr(), rawreserve_flag::up_to);
      set_int_return_flag(SEND_ALL_GAS);
    }
    return true;
  }

  __always_inline
  bool isFullyInitialized() {
    return xchg_pair_code_ && xchg_price_code_
      && ext_wallet_code_ && flex_wallet_code_
      && wrapper_code_ && wrapper_ever_code_;
  }

  __always_inline
  FlexDetails getDetails() {
    return {
      isFullyInitialized(),
      getEversCfg(),
      getListingCfg(),
      getXchgPairCode(),
      getWrapperCode(),
      getWrapperEverCode(),
      getDealsLimit(),
      getUnsaltedPriceCodeHash(),
      getOwnershipInfo(),
      getWrapperListingRequests(),
      getXchgPairListingRequests()
      };
  }

  __always_inline
  EversConfig getEversCfg() {
    return ev_cfg_;
  }

  __always_inline
  ListingConfig getListingCfg() {
    return listing_cfg_;
  }

  __always_inline
  cell getXchgPairCode() {
    return xchg_pair_code_.get();
  }

  __always_inline
  cell getWrapperCode() {
    return wrapper_code_.get();
  }

  __always_inline
  cell getWrapperEverCode() {
    return wrapper_ever_code_.get();
  }

  __always_inline
  address getXchgTradingPair(address tip3_major_root, address tip3_minor_root) {
    auto pair_data = prepare_pair(tip3_major_root, tip3_minor_root);
    auto std_addr = prepare<IXchgPair>(pair_data, xchg_pair_code_.get()).second;
    return address::make_std(workchain_id_, std_addr);
  }

  __always_inline
  uint128 calcLendTokensForOrder(bool sell, uint128 major_tokens, price_t price) {
    return calc_lend_tokens_for_order(sell, major_tokens, price);
  }

  __always_inline
  uint8 getDealsLimit() {
    return deals_limit_;
  }

  __always_inline
  uint256 getUnsaltedPriceCodeHash() {
    require(xchg_price_code_, error_code::xchg_price_code_undefined);
    return uint256{tvm_hash(xchg_price_code_.get())};
  }

  __always_inline
  FlexOwnershipInfo getOwnershipInfo() {
    return { deployer_pubkey_, ownership_description_, owner_address_ };
  }

  __always_inline
  dict_array<WrapperListingRequestWithPubkey> getWrapperListingRequests() {
    dict_array<WrapperListingRequestWithPubkey> rv;
    for (const auto &v : wrapper_listing_requests_) {
      rv.push_back({v.first, v.second});
    }
    return rv;
  }

  __always_inline
  dict_array<XchgPairListingRequestWithPubkey> getXchgPairListingRequests() {
    dict_array<XchgPairListingRequestWithPubkey> rv;
    for (const auto &v : xchg_pair_listing_requests_) {
      rv.push_back({v.first, v.second});
    }
    return rv;
  }

  __always_inline
  void check_owner() {
    bool internal_ownership = !!owner_address_;
    if constexpr (Internal)
      require(internal_ownership && (int_sender() == *owner_address_), error_code::sender_is_not_my_owner);
    else
      require(!internal_ownership && (msg_pubkey() == deployer_pubkey_), error_code::sender_is_not_my_owner);
  }

  // =============== Support functions ==================
  DEFAULT_SUPPORT_FUNCTIONS(IFlex, flex_replay_protection_t)

  // default processing of unknown messages
  __always_inline static int _fallback(cell /*msg*/, slice /*msg_body*/) {
    return 0;
  }
};

__attribute__((noinline))
std::pair<address, DFlex::xchg_pairs_map> approveXchgPairImpl(
  uint256 pubkey,
  DFlex::xchg_pairs_map xchg_pair_listing_requests,
  cell xchg_pair_code,
  int8 workchain_id,
  ListingConfig listing_cfg
) {
  auto opt_req_info = xchg_pair_listing_requests.extract(pubkey.get());
  require(!!opt_req_info, Flex<true>::error_code::xchg_pair_not_requested);
  auto req_info = *opt_req_info;

  auto pair_data = prepare_pair(req_info.tip3_major_root, req_info.tip3_minor_root);

  auto [state_init, std_addr] = prepare<IXchgPair>(pair_data, xchg_pair_code);
  auto xchg_pair = IXchgPairPtr(address::make_std(workchain_id, std_addr));
  xchg_pair.deploy(state_init, Evers(listing_cfg.pair_deploy_value.get())).
    onDeploy(req_info.min_amount, req_info.minmove, req_info.price_denum,
             listing_cfg.pair_keep_balance, req_info.notify_addr,
             req_info.major_tip3cfg, req_info.minor_tip3cfg);

  auto remaining_funds = req_info.client_funds - listing_cfg.register_pair_cost;
  IListingAnswerPtr(req_info.client_addr)(Evers(remaining_funds.get())).
    onXchgPairApproved(pubkey, xchg_pair.get());
  return { xchg_pair.get(), xchg_pair_listing_requests };
}

__attribute__((noinline))
DFlex::xchg_pairs_map rejectXchgPairImpl(
  uint256 pubkey,
  DFlex::xchg_pairs_map xchg_pair_listing_requests,
  ListingConfig listing_cfg
) {
  auto opt_req_info = xchg_pair_listing_requests.extract(pubkey.get());
  require(!!opt_req_info, Flex<true>::error_code::xchg_pair_not_requested);
  auto req_info = *opt_req_info;

  auto remaining_funds = req_info.client_funds - listing_cfg.reject_pair_cost;
  IListingAnswerPtr(req_info.client_addr)(Evers(remaining_funds.get())).
    onXchgPairRejected(pubkey);
  return xchg_pair_listing_requests;
}

__attribute__((noinline))
std::pair<address, DFlex::wrappers_map> approveWrapperImpl(
  uint256 pubkey,
  DFlex::wrappers_map wrapper_listing_requests,
  cell wrapper_code,
  cell ext_wallet_code,
  cell flex_wallet_code,
  int8 workchain_id,
  ListingConfig listing_cfg
) {
  auto opt_req_info = wrapper_listing_requests.extract(pubkey.get());
  require(!!opt_req_info, Flex<true>::error_code::wrapper_not_requested);
  auto req_info = *opt_req_info;
  auto tip3cfg = req_info.tip3cfg;
  DWrapper wrapper_data {
    .name_ = tip3cfg.name,
    .symbol_ = tip3cfg.symbol,
    .decimals_ = tip3cfg.decimals,
    .workchain_id_ = workchain_id,
    .root_pubkey_ = pubkey,
    .total_granted_ = {},
    .internal_wallet_code_ = {},
    .start_balance_ = Evers(listing_cfg.wrapper_keep_balance.get()),
    .flex_ = address{tvm_myaddr()},
    .wallet_ = {}
  };
  auto [wrapper_init, wrapper_hash_addr] = prepare_wrapper_state_init_and_addr(wrapper_code, wrapper_data);
  IWrapperPtr wrapper_addr(address::make_std(workchain_id, wrapper_hash_addr));

  // ============= Deploying external wallet for Flex wrapper ============ //
  auto [wallet_init, wallet_hash_addr] = prepare_external_wallet_state_init_and_addr(
    tip3cfg.name, tip3cfg.symbol, tip3cfg.decimals,
    tip3cfg.root_pubkey, tip3cfg.root_address,
    pubkey, wrapper_addr.get(),
    uint256(tvm_hash(ext_wallet_code)), uint16(ext_wallet_code.cdepth()),
    workchain_id, ext_wallet_code.get());
  ITONTokenWalletPtr wallet_addr(address::make_std(workchain_id, wallet_hash_addr));
  wallet_addr.deploy_noop(wallet_init, Evers(listing_cfg.ext_wallet_balance.get()));

  // ================== Deploying Flex wrapper ================== //
  wrapper_addr.deploy(wrapper_init, Evers(listing_cfg.wrapper_deploy_value.get()))
    .init(wallet_addr.get(), listing_cfg.reserve_wallet_value, flex_wallet_code);
  return { wrapper_addr.get(), wrapper_listing_requests };
}

__attribute__((noinline))
std::pair<address, DFlex::wrappers_map> approveWrapperEverImpl(
  uint256 pubkey,
  DFlex::wrappers_map wrapper_listing_requests,
  cell wrapper_code,
  cell flex_wallet_code,
  int8 workchain_id,
  ListingConfig listing_cfg
) {
  auto opt_req_info = wrapper_listing_requests.extract(pubkey.get());
  require(!!opt_req_info, Flex<true>::error_code::wrapper_not_requested);
  auto req_info = *opt_req_info;
  auto tip3cfg = req_info.tip3cfg;
  DWrapper wrapper_data {
    .name_ = tip3cfg.name,
    .symbol_ = tip3cfg.symbol,
    .decimals_ = tip3cfg.decimals,
    .workchain_id_ = workchain_id,
    .root_pubkey_ = pubkey,
    .total_granted_ = {},
    .internal_wallet_code_ = {},
    .start_balance_ = Evers(listing_cfg.wrapper_keep_balance.get()),
    .flex_ = address{tvm_myaddr()},
    .wallet_ = {}
  };
  auto [wrapper_init, wrapper_hash_addr] = prepare_wrapper_ever_state_init_and_addr(wrapper_code, wrapper_data);
  IWrapperEverPtr wrapper_addr(address::make_std(workchain_id, wrapper_hash_addr));

  // ================== Deploying Flex wrapper ================== //
  wrapper_addr.deploy(wrapper_init, Evers(listing_cfg.wrapper_deploy_value.get()))
    .init(listing_cfg.reserve_wallet_value, flex_wallet_code);
  return { wrapper_addr.get(), wrapper_listing_requests };
}

__attribute__((noinline))
DFlex::wrappers_map rejectWrapperImpl(
  uint256 pubkey,
  DFlex::wrappers_map wrapper_listing_requests,
  ListingConfig listing_cfg
) {
  auto opt_req_info = wrapper_listing_requests.extract(pubkey.get());
  require(!!opt_req_info, Flex<true>::error_code::wrapper_not_requested);
  auto req_info = *opt_req_info;

  auto remaining_funds = req_info.client_funds - listing_cfg.reject_wrapper_cost;
  IListingAnswerPtr(req_info.client_addr)(Evers(remaining_funds.get())).
    onWrapperRejected(pubkey);
  return wrapper_listing_requests;
}

DEFINE_JSON_ABI(IFlex, DFlex, EFlex, flex_replay_protection_t);

// ----------------------------- Main entry functions ---------------------- //
DEFAULT_MAIN_ENTRY_FUNCTIONS_TMPL(Flex, IFlex, DFlex, FLEX_TIMESTAMP_DELAY)

