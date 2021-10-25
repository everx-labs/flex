#include "Flex.hpp"
#include "TradingPair.hpp"
#include "XchgPair.hpp"
#include "Wrapper.hpp"
#include <tvm/contract.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/default_support_functions.hpp>

using namespace tvm;
using namespace schema;

__always_inline //  __attribute__((noinline))
static std::pair<StateInit, uint256> prepare_trading_pair(address flex, address tip3_root, cell pair_code) {
  DTradingPair pair_data {
    .flex_addr_ = address{tvm_myaddr()},
    .tip3_root_ = tip3_root,
    .min_amount_ = uint128(0),
    .notify_addr_ = address::make_std(int8(0), uint256(0))
  };
  return prepare_trading_pair_state_init_and_addr(pair_data, pair_code);
}

__attribute__((noinline))
std::pair<address, DFlex::trading_pairs_map> approveTradingPairImpl(
  uint256 pubkey,
  DFlex::trading_pairs_map trading_pair_listing_requests,
  cell pair_code,
  int8 workchain_id,
  ListingConfig listing_cfg);

__attribute__((noinline))
DFlex::trading_pairs_map rejectTradingPairImpl(
  uint256 pubkey,
  DFlex::trading_pairs_map trading_pair_listing_requests,
  ListingConfig listing_cfg);

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
DFlex::wrappers_map rejectWrapperImpl(
  uint256 pubkey,
  DFlex::wrappers_map wrapper_listing_requests,
  ListingConfig listing_cfg);

template<bool Internal>
class Flex final : public smart_interface<IFlex>, public DFlex {
public:
  struct error_code : tvm::error_code {
    static constexpr unsigned sender_is_not_deployer        = 100;
    static constexpr unsigned unexpected_refs_count_in_code = 101;
    static constexpr unsigned cant_override_code            = 102;
    static constexpr unsigned sender_is_not_my_owner        = 103;
    static constexpr unsigned not_enough_funds              = 104;
    static constexpr unsigned wrapper_not_requested         = 105;
    static constexpr unsigned trading_pair_not_requested    = 106;
    static constexpr unsigned xchg_pair_not_requested       = 107;
    static constexpr unsigned costs_inconsistency           = 108;

    static constexpr unsigned wrapper_with_such_pubkey_already_requested = 109;
    static constexpr unsigned trading_pair_with_such_pubkey_already_requested = 110;
    static constexpr unsigned xchg_pair_with_such_pubkey_already_requested = 111;
  };

  __always_inline
  void constructor(
      uint256 deployer_pubkey,
      string ownership_description, std::optional<address> owner_address,
      TonsConfig tons_cfg,
      uint8 deals_limit,
      ListingConfig listing_cfg
  ) {
    deployer_pubkey_ = deployer_pubkey;
    ownership_description_ = ownership_description;
    owner_address_ = owner_address;
    deals_limit_ = deals_limit;
    tons_cfg_ = tons_cfg;
    listing_cfg_ = listing_cfg;
    workchain_id_ = std::get<addr_std>(address{tvm_myaddr()}.val()).workchain_id;

    require(listing_cfg.register_pair_cost >= listing_cfg.reject_pair_cost + listing_cfg.register_return_value,
            error_code::costs_inconsistency);
  }

  __always_inline
  void setSpecificCode(uint8 type, cell code) {
    switch (static_cast<code_type>(type.get())) {
    case code_type::trade_pair_code: setPairCode(code); return;
    case code_type::xchg_pair_code: setXchgPairCode(code); return;
    case code_type::wrapper_code: setWrapperCode(code); return;
    case code_type::ext_wallet_code: setExtWalletCode(code); return;
    case code_type::flex_wallet_code: setFlexWalletCode(code); return;
    case code_type::price_code: setPriceCode(code); return;
    case code_type::xchg_price_code: setXchgPriceCode(code); return;
    }
  }

  __always_inline
  void setPairCode(cell code) {
    require(!pair_code_, error_code::cant_override_code);
    require(msg_pubkey() == deployer_pubkey_, error_code::sender_is_not_deployer);
    tvm_accept();

    // adding flex address to ref#2 in this code cell
    require(code.ctos().srefs() == 2, error_code::unexpected_refs_count_in_code);
    pair_code_ = builder().stslice(code.ctos()).stref(build(address{tvm_myaddr()}).endc()).endc();
  }

  __always_inline
  void setXchgPairCode(cell code) {
    require(!xchg_pair_code_, error_code::cant_override_code);
    require(msg_pubkey() == deployer_pubkey_, error_code::sender_is_not_deployer);
    tvm_accept();

    // adding flex address to ref#2 in this code cell
    require(code.ctos().srefs() == 2, error_code::unexpected_refs_count_in_code);
    xchg_pair_code_ = builder().stslice(code.ctos()).stref(build(address{tvm_myaddr()}).endc()).endc();
  }

  __always_inline
  void setWrapperCode(cell code) {
    require(!wrapper_code_, error_code::cant_override_code);
    require(msg_pubkey() == deployer_pubkey_, error_code::sender_is_not_deployer);
    tvm_accept();

    require(code.ctos().srefs() == 2, error_code::unexpected_refs_count_in_code);
    wrapper_code_ = builder().stslice(code.ctos()).stref(build(address{tvm_myaddr()}).endc()).endc();
  }

  __always_inline
  void setPriceCode(cell code) {
    require(!price_code_, error_code::cant_override_code);
    require(msg_pubkey() == deployer_pubkey_, error_code::sender_is_not_deployer);
    tvm_accept();

    price_code_ = code;
  }

  __always_inline
  void setXchgPriceCode(cell code) {
    require(!xchg_price_code_, error_code::cant_override_code);
    require(msg_pubkey() == deployer_pubkey_, error_code::sender_is_not_deployer);
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
  void transfer(address to, uint128 tons) {
    check_owner();
    tvm_accept();
    tvm_transfer(to, tons.get(), true);
  }

  // Listing procedures
  __always_inline
  address registerTradingPair(
    uint256 pubkey,
    address tip3_root,
    uint128 min_amount,
    address notify_addr
  ) {
    require(int_value().get() > listing_cfg_.register_pair_cost, error_code::not_enough_funds);
    require(!trading_pair_listing_requests_.contains(pubkey.get()),
            error_code::trading_pair_with_such_pubkey_already_requested);

    trading_pair_listing_requests_.set_at(pubkey.get(), {
      int_sender(), uint128(int_value().get()) - listing_cfg_.register_return_value,
      tip3_root, min_amount, notify_addr
    });

    set_int_return_value(listing_cfg_.register_return_value.get());

    auto [state_init, std_addr] = prepare_trading_pair(address{tvm_myaddr()}, tip3_root, pair_code_.get());
    return address::make_std(workchain_id_, std_addr);
  }

  __always_inline
  address approveTradingPair(uint256 pubkey) {
    check_owner();
    tvm_accept();

    auto [trade_pair, new_trading_pair_listing_requests] =
      approveTradingPairImpl(pubkey, trading_pair_listing_requests_, pair_code_.get(), workchain_id_, listing_cfg_);
    trading_pair_listing_requests_ = new_trading_pair_listing_requests;

    if constexpr (Internal) {
      auto value_gr = int_value();
      tvm_rawreserve(tvm_balance() - value_gr(), rawreserve_flag::up_to);
      set_int_return_flag(SEND_ALL_GAS);
    }
    return trade_pair;
  }

  __always_inline
  bool_t rejectTradingPair(uint256 pubkey) {
    check_owner();
    trading_pair_listing_requests_ =
      rejectTradingPairImpl(pubkey, trading_pair_listing_requests_, listing_cfg_);
    if constexpr (Internal) {
      auto value_gr = int_value();
      tvm_rawreserve(tvm_balance() - value_gr(), rawreserve_flag::up_to);
      set_int_return_flag(SEND_ALL_GAS);
    }
    return bool_t{true};
  }

  __always_inline
  address registerXchgPair(
    uint256 pubkey,
    address tip3_major_root,
    address tip3_minor_root,
    uint128 min_amount,
    address notify_addr
  ) {
    require(int_value().get() > listing_cfg_.register_pair_cost, error_code::not_enough_funds);
    require(!xchg_pair_listing_requests_.contains(pubkey.get()),
            error_code::xchg_pair_with_such_pubkey_already_requested);

    xchg_pair_listing_requests_.set_at(pubkey.get(), {
      int_sender(), uint128(int_value().get()) - listing_cfg_.register_return_value,
      tip3_major_root, tip3_minor_root, min_amount, notify_addr
    });

    DXchgPair pair_data {
      .flex_addr_ = address{tvm_myaddr()},
      .tip3_major_root_ = tip3_major_root,
      .tip3_minor_root_ = tip3_minor_root,
      .min_amount_ = uint128(0),
      .notify_addr_ = address::make_std(int8(0), uint256(0))
    };

    set_int_return_value(listing_cfg_.register_return_value.get());

    auto [state_init, std_addr] = prepare_xchg_pair_state_init_and_addr(pair_data, xchg_pair_code_.get());
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
  bool_t rejectXchgPair(uint256 pubkey) {
    check_owner();
    xchg_pair_listing_requests_ = rejectXchgPairImpl(pubkey, xchg_pair_listing_requests_, listing_cfg_);
    if constexpr (Internal) {
      auto value_gr = int_value();
      tvm_rawreserve(tvm_balance() - value_gr(), rawreserve_flag::up_to);
      set_int_return_flag(SEND_ALL_GAS);
    }
    return bool_t{true};
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
      .root_public_key_ = pubkey,
      .total_granted_ = {},
      .internal_wallet_code_ = {},
      .owner_address_ = address{tvm_myaddr()},
      .start_balance_ = Grams(listing_cfg_.wrapper_keep_balance.get()),
      .external_wallet_ = {}
    };

    set_int_return_value(listing_cfg_.register_return_value.get());

    auto [wrapper_init, wrapper_hash_addr] = prepare_wrapper_state_init_and_addr(wrapper_code_.get(), wrapper_data);
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
  bool_t rejectWrapper(uint256 pubkey) {
    check_owner();
    tvm_accept();
    wrapper_listing_requests_ = rejectWrapperImpl(pubkey, wrapper_listing_requests_, listing_cfg_);
    if constexpr (Internal) {
      auto value_gr = int_value();
      tvm_rawreserve(tvm_balance() - value_gr(), rawreserve_flag::up_to);
      set_int_return_flag(SEND_ALL_GAS);
    }
    return bool_t{true};
  }

  __always_inline
  bool_t isFullyInitialized() {
    return bool_t(pair_code_ && price_code_ && xchg_pair_code_ && xchg_price_code_ &&
                  ext_wallet_code_ && flex_wallet_code_ && wrapper_code_);
  }

  __always_inline
  FlexDetails getDetails() {
    return {
      isFullyInitialized(),
      getTonsCfg(),
      getListingCfg(),
      getTradingPairCode(),
      getXchgPairCode(),
      getDealsLimit(),
      getOwnershipInfo(),
      getWrapperListingRequests(),
      getTradingPairListingRequests(),
      getXchgPairListingRequests()
      };
  }

  __always_inline
  TonsConfig getTonsCfg() {
    return tons_cfg_;
  }

  __always_inline
  ListingConfig getListingCfg() {
    return listing_cfg_;
  }

  __always_inline
  cell getTradingPairCode() {
    return pair_code_.get();
  }

  __always_inline
  cell getXchgPairCode() {
    return xchg_pair_code_.get();
  }

  __always_inline
  cell getSellPriceCode(address tip3_addr) {
    // adding tip3 root address to ref#3 in this code cell
    auto price_code_sl = price_code_.get().ctos();
    require(price_code_sl.srefs() == 2, error_code::unexpected_refs_count_in_code);
    cell salt = builder().stslice(tvm_myaddr()).stslice(tip3_addr.sl()).endc();
    return builder().stslice(price_code_sl).stref(salt).endc();
  }

  __always_inline
  cell getXchgPriceCode(address tip3_addr1, address tip3_addr2) {
    auto price_code_sl = xchg_price_code_.get().ctos();
    require(price_code_sl.srefs() == 2, error_code::unexpected_refs_count_in_code);
    auto keys = std::make_tuple(tip3_addr1, tip3_addr2);
    return builder().stslice(price_code_sl).stref(build(keys).endc()).endc();
  }

  __always_inline
  address getSellTradingPair(address tip3_root) {
    auto std_addr = prepare_trading_pair(address{tvm_myaddr()}, tip3_root, pair_code_.get()).second;
    return address::make_std(workchain_id_, std_addr);
  }

  __always_inline
  address getXchgTradingPair(address tip3_major_root, address tip3_minor_root) {
    address myaddr{tvm_myaddr()};
    DXchgPair pair_data {
      .flex_addr_ = myaddr,
      .tip3_major_root_ = tip3_major_root,
      .tip3_minor_root_ = tip3_minor_root,
      .min_amount_ = uint128(0),
      .notify_addr_ = address::make_std(int8(0), uint256(0))
    };
    auto std_addr = prepare_xchg_pair_state_init_and_addr(pair_data, xchg_pair_code_.get()).second;
    return address::make_std(workchain_id_, std_addr);
  }

  __always_inline
  uint8 getDealsLimit() {
    return deals_limit_;
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
  dict_array<TradingPairListingRequestWithPubkey> getTradingPairListingRequests() {
    dict_array<TradingPairListingRequestWithPubkey> rv;
    for (const auto &v : trading_pair_listing_requests_) {
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
std::pair<address, DFlex::trading_pairs_map> approveTradingPairImpl(
  uint256 pubkey,
  DFlex::trading_pairs_map trading_pair_listing_requests,
  cell pair_code,
  int8 workchain_id,
  ListingConfig listing_cfg
) {
  auto opt_req_info = trading_pair_listing_requests.extract(pubkey.get());
  require(!!opt_req_info, Flex<true>::error_code::trading_pair_not_requested);
  auto req_info = *opt_req_info;

  auto [state_init, std_addr] = prepare_trading_pair(address{tvm_myaddr()}, req_info.tip3_root, pair_code);
  auto trade_pair = ITradingPairPtr(address::make_std(workchain_id, std_addr));
  trade_pair.deploy(state_init, Grams(listing_cfg.pair_deploy_value.get()), DEFAULT_MSG_FLAGS, false).
    onDeploy(req_info.min_amount, listing_cfg.pair_keep_balance, req_info.notify_addr);

  auto remaining_funds = req_info.client_funds - listing_cfg.register_pair_cost;
  IListingAnswerPtr(req_info.client_addr)(Grams(remaining_funds.get())).
    onTradingPairApproved(pubkey, trade_pair.get());
  return { trade_pair.get(), trading_pair_listing_requests };
}

__attribute__((noinline))
DFlex::trading_pairs_map rejectTradingPairImpl(
  uint256 pubkey,
  DFlex::trading_pairs_map trading_pair_listing_requests,
  ListingConfig listing_cfg
) {
  auto opt_req_info = trading_pair_listing_requests.extract(pubkey.get());
  require(!!opt_req_info, Flex<true>::error_code::trading_pair_not_requested);
  auto req_info = *opt_req_info;

  auto remaining_funds = req_info.client_funds - listing_cfg.reject_pair_cost;
  IListingAnswerPtr(req_info.client_addr)(Grams(remaining_funds.get())).
    onTradingPairRejected(pubkey);
  return trading_pair_listing_requests;
}

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

  DXchgPair pair_data {
    .flex_addr_ = address{tvm_myaddr()},
    .tip3_major_root_ = req_info.tip3_major_root,
    .tip3_minor_root_ = req_info.tip3_minor_root,
    .min_amount_ = uint128(0),
    .notify_addr_ = address::make_std(int8(0), uint256(0))
  };

  auto [state_init, std_addr] = prepare_xchg_pair_state_init_and_addr(pair_data, xchg_pair_code);
  auto xchg_pair = IXchgPairPtr(address::make_std(workchain_id, std_addr));
  xchg_pair.deploy(state_init, Grams(listing_cfg.pair_deploy_value.get()), DEFAULT_MSG_FLAGS, false).
    onDeploy(req_info.min_amount, listing_cfg.pair_keep_balance, req_info.notify_addr);

  auto remaining_funds = req_info.client_funds - listing_cfg.register_pair_cost;
  IListingAnswerPtr(req_info.client_addr)(Grams(remaining_funds.get())).
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
  IListingAnswerPtr(req_info.client_addr)(Grams(remaining_funds.get())).
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
    .root_public_key_ = pubkey,
    .total_granted_ = {},
    .internal_wallet_code_ = {},
    .owner_address_ = address{tvm_myaddr()},
    .start_balance_ = Grams(listing_cfg.wrapper_keep_balance.get()),
    .external_wallet_ = {}
  };
  auto [wrapper_init, wrapper_hash_addr] = prepare_wrapper_state_init_and_addr(wrapper_code, wrapper_data);
  IWrapperPtr wrapper_addr(address::make_std(workchain_id, wrapper_hash_addr));

  // ============= Deploying external wallet for Flex wrapper ============ //
  auto [wallet_init, wallet_hash_addr] = prepare_external_wallet_state_init_and_addr(
    tip3cfg.name, tip3cfg.symbol, tip3cfg.decimals,
    tip3cfg.root_public_key, pubkey, tip3cfg.root_address,
    wrapper_addr.get(), ext_wallet_code.get(), workchain_id);
  ITONTokenWalletPtr wallet_addr(address::make_std(workchain_id, wallet_hash_addr));
  wallet_addr.deploy_noop(wallet_init, Grams(listing_cfg.ext_wallet_balance.get()));

  // ================== Deploying Flex wrapper ================== //
  wrapper_addr.deploy(wrapper_init, Grams(listing_cfg.wrapper_deploy_value.get())).init(wallet_addr.get());

  wrapper_addr(Grams(listing_cfg.set_internal_wallet_value.get())).setInternalWalletCode(flex_wallet_code);

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
  IListingAnswerPtr(req_info.client_addr)(Grams(remaining_funds.get())).
    onWrapperRejected(pubkey);
  return wrapper_listing_requests;
}

DEFINE_JSON_ABI(IFlex, DFlex, EFlex);

// ----------------------------- Main entry functions ---------------------- //
DEFAULT_MAIN_ENTRY_FUNCTIONS_TMPL(Flex, IFlex, DFlex, FLEX_TIMESTAMP_DELAY)

