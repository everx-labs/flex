#pragma once

#include <tvm/schema/message.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/replay_attack_protection/timestamp.hpp>
#include "PriceCommon.hpp"

namespace tvm { inline namespace schema {

static constexpr unsigned FLEX_TIMESTAMP_DELAY = 1800;
using flex_replay_protection_t = replay_attack_protection::timestamp<FLEX_TIMESTAMP_DELAY>;

// Processing native funds value ...
struct TonsConfig {
  // ... for executing tip3 transfer
  uint128 transfer_tip3;
  // ... to return tip3 ownership
  uint128 return_ownership;
  // ... to deploy tip3 trading pair
  uint128 trading_pair_deploy;
  // .. to send answer message from Price
  uint128 order_answer;
  // ... to process processQueue function
  //  (also is used for buyTip3/onTip3LendOwnership/cancelSell/cancelBuy estimations)
  uint128 process_queue;
  // ... to send notification about completed deal (IFlexNotify)
  uint128 send_notify;
};

struct ListingConfig {
  // Funds requirement (from client) to deploy wrapper
  uint128 register_wrapper_cost;
  // Funds to be taken in case of rejected wrapper
  // rest will be returned to client
  uint128 reject_wrapper_cost;
  // Funds to send in wrapper deploy message
  uint128 wrapper_deploy_value;
  // Wrapper will try to keep this balance (should be less then wrapper_deploy_value)
  uint128 wrapper_keep_balance;
  // Funds to send in external wallet deploy message
  uint128 ext_wallet_balance;
  // Funds to send with Wrapper::setInternalWalletCode message
  uint128 set_internal_wallet_value;
  // Funds requirement (from client) to deploy xchg/trading pair
  uint128 register_pair_cost;
  // Funds to be taken in case of rejected xchg/trading pair
  // rest will be returned to client
  uint128 reject_pair_cost;
  // Funds to send in pair deploy message
  uint128 pair_deploy_value;
  // Pair will try to keep this balance
  uint128 pair_keep_balance;
  // Value for answer
  uint128 register_return_value;
};

// Answers to client about approved/rejected listings
__interface IListingAnswer {
  [[internal, noaccept]]
  void onWrapperApproved(uint256 pubkey, address contract) = 500;
  [[internal, noaccept]]
  void onWrapperRejected(uint256 pubkey) = 501;
  [[internal, noaccept]]
  void onTradingPairApproved(uint256 pubkey, address contract) = 502;
  [[internal, noaccept]]
  void onTradingPairRejected(uint256 pubkey) = 503;
  [[internal, noaccept]]
  void onXchgPairApproved(uint256 pubkey, address contract) = 504;
  [[internal, noaccept]]
  void onXchgPairRejected(uint256 pubkey) = 505;
};
using IListingAnswerPtr = handle<IListingAnswer>;

__interface IFlexNotify {
  [[internal, noaccept]]
  void onDealCompleted(address tip3root, uint128 price, uint128 amount) = 10;
  [[internal, noaccept]]
  void onXchgDealCompleted(address tip3root_sell, address tip3root_buy,
                           uint128 price_num, uint128 price_denum, uint128 amount) = 11;
  [[internal, noaccept]]
  void onOrderAdded(bool_t sell, address tip3root, uint128 price, uint128 amount, uint128 sum_amount) = 12;
  [[internal, noaccept]]
  void onOrderCanceled(bool_t sell, address tip3root, uint128 price, uint128 amount, uint128 sum_amount) = 13;
  [[internal, noaccept]]
  void onXchgOrderAdded(bool_t sell, address tip3root_major, address tip3root_minor,
                        uint128 price_num, uint128 price_denum, uint128 amount, uint128 sum_amount) = 14;
  [[internal, noaccept]]
  void onXchgOrderCanceled(bool_t sell, address tip3root_major, address tip3root_minor,
                           uint128 price_num, uint128 price_denum, uint128 amount, uint128 sum_amount) = 15;
};
using IFlexNotifyPtr = handle<IFlexNotify>;

// Request to deploy wrapper
struct WrapperListingRequest {
  // client address to send notification with the remaining funds
  address client_addr;
  // Native funds, sent by client
  uint128 client_funds;
  // Configuration of external tip3 wallet
  Tip3Config tip3cfg;
};

struct WrapperListingRequestWithPubkey {
  uint256 wrapper_pubkey;
  WrapperListingRequest request;
};

// Request to deploy trading pair
struct TradingPairListingRequest {
  // client address to send notification with the remaining funds
  address client_addr;
  // Native funds, sent by client
  uint128 client_funds;
  address tip3_root;
  uint128 min_amount;
  address notify_addr;
};

struct TradingPairListingRequestWithPubkey {
  uint256 wrapper_pubkey;
  TradingPairListingRequest request;
};

// Request to deploy xchg pair
struct XchgPairListingRequest {
  // client address to send notification with the remaining funds
  address client_addr;
  // Native funds, sent by client
  uint128 client_funds;
  address tip3_major_root;
  address tip3_minor_root;
  uint128 min_amount;
  address notify_addr;
};

struct XchgPairListingRequestWithPubkey {
  uint256 request_pubkey;
  XchgPairListingRequest request;
};

struct FlexOwnershipInfo {
  uint256 deployer_pubkey;
  string ownership_description;
  // If Flex is managed by other contract (deployer will not be able to execute non-deploy methods)
  std::optional<address> owner_contract;
};

struct FlexDetails {
  bool_t initialized;
  TonsConfig tons_cfg;
  ListingConfig listing_cfg;
  cell trading_pair_code;
  cell xchg_pair_code;
  uint8 deals_limit;
  FlexOwnershipInfo ownership;
  dict_array<WrapperListingRequestWithPubkey> wrapper_listing_requests;
  dict_array<TradingPairListingRequestWithPubkey> trading_pair_listing_requests;
  dict_array<XchgPairListingRequestWithPubkey> xchg_pair_listing_requests;
};

enum class code_type {
  trade_pair_code = 1,
  xchg_pair_code,
  wrapper_code,
  ext_wallet_code,
  flex_wallet_code,
  price_code,
  xchg_price_code
};

__interface IFlex {

  [[external]]
  void constructor(
    uint256 deployer_pubkey,
    string ownership_description, std::optional<address> owner_address,
    TonsConfig tons_cfg,
    uint8 deals_limit,
    ListingConfig listing_cfg) = 10;

  // To fit message size limit, set specific code in separate function
  //  (not in constructor). To set pairs, wrapper, wallets, prices contract codes.
  // `type` should be code_type enum
  [[external, noaccept]]
  void setSpecificCode(uint8 type, cell code) = 11;

  [[external, internal, noaccept]]
  void transfer(address to, uint128 tons) = 12;

  // Listing procedures
  // Request to register trading pair (returns pre-calculated address of future trading pair)
  [[internal, noaccept, answer_id]]
  address registerTradingPair(
    uint256 pubkey,
    address tip3_root,
    uint128 min_amount,
    address notify_addr
    ) = 13;
  // Request to register xchg pair (returns pre-calculated address of future xchg pair)
  [[internal, noaccept, answer_id]]
  address registerXchgPair(
    uint256 pubkey,
    address tip3_major_root,
    address tip3_minor_root,
    uint128 min_amount,
    address notify_addr
    ) = 14;
  // Request to register wrapper (returns pre-calculated address of future wrapper)
  [[internal, noaccept, answer_id]]
  address registerWrapper(
    uint256 pubkey,
    Tip3Config tip3cfg
    ) = 15;

  [[external, internal, noaccept, answer_id]]
  address approveTradingPair(
    uint256 pubkey
    ) = 16;
  [[external, internal, noaccept]]
  bool_t rejectTradingPair(
    uint256 pubkey
    ) = 17;
  [[external, internal, noaccept, answer_id]]
  address approveXchgPair(
    uint256 pubkey
    ) = 18;
  [[external, internal, noaccept]]
  bool_t rejectXchgPair(
    uint256 pubkey
    ) = 19;
  [[external, internal, noaccept, answer_id]]
  address approveWrapper(
    uint256 pubkey
    ) = 20;
  [[external, internal, noaccept]]
  bool_t rejectWrapper(
    uint256 pubkey
    ) = 21;
  // ========== getters ==========

  // means setPairCode/setXchgPairCode/setPriceCode/setXchgPriceCode executed
  [[getter]]
  bool_t isFullyInitialized() = 22;

  [[getter]]
  FlexDetails getDetails() = 23;

  [[getter]]
  cell getSellPriceCode(address tip3_addr) = 24;

  [[getter]]
  cell getXchgPriceCode(address tip3_addr1, address tip3_addr2) = 25;

  [[getter]]
  address getSellTradingPair(address tip3_root) = 26;

  [[getter]]
  address getXchgTradingPair(address tip3_major_root, address tip3_minor_root) = 27;
};
using IFlexPtr = handle<IFlex>;

struct DFlex {
  uint256 deployer_pubkey_;
  int8 workchain_id_;
  string ownership_description_;
  std::optional<address> owner_address_;
  TonsConfig tons_cfg_;
  ListingConfig listing_cfg_;
  optcell pair_code_;
  optcell xchg_pair_code_;
  optcell price_code_;
  optcell xchg_price_code_;
  optcell ext_wallet_code_;
  optcell flex_wallet_code_;
  optcell wrapper_code_;
  uint8 deals_limit_; // deals limit in one processing in Price

  // map from wrapper pubkey into listing data
  using wrappers_map = dict_map<uint256, WrapperListingRequest>;
  wrappers_map wrapper_listing_requests_;
  // map from request pubkey into trading pair listing data
  using trading_pairs_map = dict_map<uint256, TradingPairListingRequest>;
  trading_pairs_map trading_pair_listing_requests_;
  // map from request pubkey into xchg pair listing data
  using xchg_pairs_map = dict_map<uint256, XchgPairListingRequest>;
  xchg_pairs_map xchg_pair_listing_requests_;
};

__interface EFlex {
};

// Prepare Flex StateInit structure and expected contract address (hash from StateInit)
inline
std::pair<StateInit, uint256> prepare_flex_state_init_and_addr(DFlex flex_data, cell flex_code) {
  cell flex_data_cl =
    prepare_persistent_data<IFlex, flex_replay_protection_t, DFlex>(
      flex_replay_protection_t::init(), flex_data
    );
  StateInit flex_init {
    /*split_depth*/{}, /*special*/{},
    flex_code, flex_data_cl, /*library*/{}
  };
  cell flex_init_cl = build(flex_init).make_cell();
  return { flex_init, uint256(tvm_hash(flex_init_cl)) };
}

}} // namespace tvm::schema

