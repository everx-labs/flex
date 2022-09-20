/** \file
 *  \brief FlexClient contract implementation
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#include "FlexClient.hpp"
#include "XchgPair.hpp"
#include "Wrapper.hpp"
#include "UserIdIndex.hpp"
#include "AuthIndex.hpp"
#include "UserDataConfig.hpp"

#include <tvm/contract.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/replay_attack_protection/timestamp.hpp>
#include <tvm/default_support_functions.hpp>
#include <tvm/suffixes.hpp>

using namespace tvm;

static constexpr unsigned TIMESTAMP_DELAY = 1800;

/// FlexClient contract. Implements IFlexClient.
class FlexClient final : public smart_interface<IFlexClient>, public DFlexClient {
  using data = DFlexClient;
public:
  using replay_protection_t = replay_attack_protection::timestamp<TIMESTAMP_DELAY>;
  DEFAULT_SUPPORT_FUNCTIONS(IFlexClient, replay_protection_t)

  struct error_code : tvm::error_code {
    static constexpr unsigned message_sender_is_not_my_owner = 100; ///< Message sender is not my owner
    static constexpr unsigned zero_num_in_price              = 101; ///< Zero numerator in price
    static constexpr unsigned uninitialized                  = 102; ///< Not fully initialized
    static constexpr unsigned upgrade_version_must_be_higher = 103; ///< Upgrade version must be higher than the current version
    static constexpr unsigned unverified_tip3_wallet         = 104; ///< Unverified tip3 token wallet
    static constexpr unsigned too_many_wallets               = 105; ///< Too many wallets
    static constexpr unsigned not_enough_balance             = 106; ///< Not enough balance
    static constexpr unsigned only_one_packet_burning_may_be_processed_at_a_time = 107; ///< Only one packet burning may be processed at a time
    static constexpr unsigned only_one_packet_canceling_may_be_processed_at_a_time = 108; ///< Only one packet canceling may be processed at a time
  };

  __attribute__((noinline, noreturn))
  static void onCodeUpgrade(cell state) {
    auto [hdr, vals] = parse_persistent_data<IFlexClientStub, replay_protection_t, DFlexClient0>(state);
    DFlexClient data {
      .owner_              = vals.owner_,
      .triplet_            = vals.triplet_,
      .auth_index_code_    = vals.auth_index_code_,
      .user_id_index_code_ = vals.user_id_index_code_,
      .binding_            = vals.binding_
    };
    // TODO: Consider vals.ex_triplet_ for FlexClient(N) -> FlexClient(K) upgrades
    save_persistent_data<IFlexClient, replay_protection_t, DFlexClient>(hdr, data);
    tvm_throw(0);
  }

  resumable<void> upgrade(
    uint128 request_evers,
    address user_data_cfg
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    tvm_commit();
    Evers request_ev(request_evers.get());

    auto details = co_await IUserDataConfigPtr(user_data_cfg)(request_ev).requestDetails();
    require(details.triplet.user > triplet_.user, error_code::upgrade_version_must_be_higher);

    ex_triplet_         = triplet_;
    binding_            = details.binding;
    auth_index_code_    = details.auth_index_code;
    user_id_index_code_ = details.user_id_index_code;
    triplet_            = details.triplet;

    cell state = prepare_persistent_data<IFlexClient, replay_protection_t, data>(header_, static_cast<data&>(*this));
    tvm_setcode(details.flex_client_code);
    tvm_setcurrentcode(parser(details.flex_client_code).skipref().ldref());

    onCodeUpgrade(state);
  }

  void cancelXchgOrder(
    bool         sell,
    uint128      price_num,
    uint128      value,
    cell         salted_price_code,
    opt<uint256> user_id,
    opt<uint256> order_id
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    tvm_commit();

    auto [state_init, addr, std_addr] = preparePriceXchg(price_num, salted_price_code);
    IPriceXchgPtr price_addr(addr);
    price_addr(Evers(value.get())).cancelOrder(sell, user_id, order_id);
  }

  void transfer(address dest, uint128 value, bool bounce) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    tvm_commit();
    tvm_transfer(dest, value.get(), bounce);
  }

  void transferTokens(
    address   src,
    Tip3Creds dst,
    uint128   tokens,
    uint128   evers,
    uint128   keep_evers
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    tvm_commit();
    ITONTokenWalletPtr(src)(Evers(evers.get())).
      transferToRecipient(tvm_myaddr(), dst, tokens, 0u128, keep_evers, true, 0u128, builder().endc());
  }

  address deployPriceXchg(
    bool    sell,
    bool    immediate_client,
    bool    post_order,
    uint128 price_num,
    uint128 amount,
    uint128 lend_amount,
    uint32  lend_finish_time,
    uint128 evers,
    cell    unsalted_price_code,
    cell    price_salt,
    address my_tip3_addr,
    uint256 user_id,
    uint256 order_id
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    require(price_num != 0, error_code::zero_num_in_price);
    tvm_accept();
    tvm_commit();

    FlexLendPayloadArgs args = {
      .sell                = sell,
      .immediate_client    = immediate_client,
      .post_order          = post_order,
      .amount              = amount,
      .client_addr         = address{tvm_myaddr()},
      .user_id             = user_id,
      .order_id            = order_id
    };

    ITONTokenWalletPtr my_tip3(my_tip3_addr);
    my_tip3(Evers(evers.get())).
      makeOrder(address{tvm_myaddr()}, 0u128, lend_amount, lend_finish_time, price_num, unsalted_price_code, price_salt, args);

    auto [state_init, addr, std_addr] = preparePriceXchg(price_num, tvm_add_code_salt_cell(price_salt, unsalted_price_code));
    auto price_addr = IPriceXchgPtr(addr);
    return price_addr.get();
  }

  address deployEmptyFlexWallet(
    uint256        pubkey,
    uint128        evers_to_wallet,
    Tip3Config     tip3cfg,
    uint256        trader,
    cell           flex_wallet_code
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    tvm_commit();
    auto workchain_id = std::get<addr_std>(tvm_myaddr().val()).workchain_id;

    auto [init, hash_addr] = prepare_internal_wallet_state_init_and_addr(
      tip3cfg.name, tip3cfg.symbol, tip3cfg.decimals,
      tip3cfg.root_pubkey, tip3cfg.root_address,
      pubkey, address{tvm_myaddr()},
      uint256(tvm_hash(flex_wallet_code)), uint16(flex_wallet_code.cdepth()),
      workchain_id, flex_wallet_code
      );
    ITONTokenWalletPtr new_wallet(address::make_std(workchain_id, hash_addr));
    new_wallet.deploy(init, Evers(evers_to_wallet.get())).bind(true, binding_, true, trader);
    return new_wallet.get();
  }

  void deployIndex(
    uint256 user_id,
    uint256 lend_pubkey,
    string  name,
    uint128 evers_all,
    uint128 evers_to_auth_idx,
    uint128 refill_wallet,
    uint128 min_refill
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    require(user_id_index_code_, error_code::uninitialized);
    tvm_accept();
    tvm_commit();
    auto workchain_id = std::get<addr_std>(tvm_myaddr().val()).workchain_id;
    auto salted_code = tvm_add_code_salt(UserIdIndexSalt{tvm_myaddr(), auth_index_code_.get()}, user_id_index_code_.get());
    auto [init, hash] = prepare<IUserIdIndex, DUserIdIndex>({.user_id_=user_id}, salted_code);
    IUserIdIndexPtr ptr(address::make_std(workchain_id, hash));
    ptr.deploy(init, Evers(evers_all.get())).onDeploy(lend_pubkey, name, evers_to_auth_idx, refill_wallet, min_refill);
  }

  void reBindWallets(
    uint256             user_id,
    bool                set_binding,
    opt<bind_info>      binding,
    bool                set_trader,
    opt<uint256>        trader,
    dict_array<address> wallets,
    uint128             evers_relend_call,
    uint128             evers_each_wallet_call,
    uint128             evers_to_remove,
    uint128             evers_to_auth_idx
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    tvm_commit();
    require(wallets.size() < (255 / 2), error_code::too_many_wallets);
    if (set_trader && trader) {
      IUserIdIndexPtr(getUserIdIndex(user_id))(Evers(evers_relend_call.get())).
        reLendPubkey(*trader, evers_to_remove, evers_to_auth_idx);
    }
    for (auto addr : wallets) {
      ITONTokenWalletPtr(addr)(Evers(evers_each_wallet_call.get())).bind(set_binding, binding, set_trader, trader);
    }
  }

  void destroyIndex(
    uint256 user_id,
    uint128 evers
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    tvm_commit();
    IUserIdIndexPtr(getUserIdIndex(user_id))(Evers(evers.get())).remove();
  }

  void burnWallet(
    uint128     evers_value,
    uint256     out_pubkey,
    address_opt out_owner,
    address     my_tip3_addr,
    opt<cell>   notify
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    tvm_commit();

    ITONTokenWalletPtr my_tip3(my_tip3_addr);
    my_tip3(Evers(evers_value.get())).
      burn(out_pubkey, out_owner, notify);
  }

  // Returns updated (packet_burning, burns_ev, burns)
  static __attribute__((noinline))
  std::tuple<bool_t, uint128, dict_array<BurnInfo>>
  burn_them_all_impl(uint128              burn_ev,
                     dict_array<BurnInfo> burns
  ) {
    auto sz = burns.size();
    if (!sz) return {};
    unsigned msgs_sent = 0;
    do {
      auto [idx, info, succ] = burns.rem_min();
      if (!succ) return {};
      ITONTokenWalletPtr(info.wallet)(Evers(burn_ev.get())).burn(info.out_pubkey, info.out_owner, info.notify);
      ++msgs_sent;
      // We need to reserve one last message for internal call to myaddr, until we have only one extra wallet.
    } while (msgs_sent < 254 || (msgs_sent == 254 && sz == 255));

    if (msgs_sent < sz) {
      IFlexClientPtr(tvm_myaddr())(0_ev, SEND_ALL_GAS).continueBurnThemAll();
      return {bool_t{true}, burn_ev, burns};
    } else {
      return {};
    }
  }

  void burnThemAll(
    uint128              burn_ev,
    dict_array<BurnInfo> burns
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    tvm_commit();

    require(burn_ev * burns.size() < tvm_balance(), error_code::not_enough_balance);
    require(!packet_burning_.get(), error_code::only_one_packet_burning_may_be_processed_at_a_time);

    auto [packet_burning, burn_ev_new, burns_new] = burn_them_all_impl(burn_ev, burns);
    packet_burning_ = packet_burning;
    burn_ev_ = burn_ev_new;
    burns_ = burns_new;
  }

  void continueBurnThemAll() {
    require(int_sender() == tvm_myaddr(), error_code::message_sender_is_not_my_owner);
    tvm_accept();
    auto [packet_burning, burn_ev, burns] = burn_them_all_impl(burn_ev_, burns_);
    packet_burning_ = packet_burning;
    burn_ev_ = burn_ev;
    burns_ = burns;
  }

  // Returns updated (packet_burning, burns_ev, burns)
  static __attribute__((noinline))
  std::tuple<bool_t, uint128, dict_array<address>>
  cancel_them_all_impl(uint128             cancel_ev,
                       dict_array<address> prices
  ) {
    auto sz = prices.size();
    if (!sz) return {};
    unsigned msgs_sent = 0;
    do {
      auto [idx, price, succ] = prices.rem_min();
      if (!succ) return {};
      IPriceXchgPtr ptr(price);
      ptr(Evers(cancel_ev.get())).cancelOrder(true, {}, {});
      ptr(Evers(cancel_ev.get())).cancelOrder(false, {}, {});
      msgs_sent += 2;
    } while (msgs_sent < 253);

    if (msgs_sent < sz * 2) {
      IFlexClientPtr(tvm_myaddr())(0_ev, SEND_ALL_GAS).continueCancelThemAll();
      return {bool_t{true}, cancel_ev, prices};
    } else {
      return {};
    }
  }

  void cancelThemAll(
    uint128             cancel_ev,
    dict_array<address> prices
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    tvm_commit();

    require(cancel_ev * 2 * prices.size() < tvm_balance(), error_code::not_enough_balance);
    require(!packet_canceling_.get(), error_code::only_one_packet_canceling_may_be_processed_at_a_time);

    auto [packet_canceling, cancel_ev_new, prices_new] = cancel_them_all_impl(cancel_ev, prices);
    packet_canceling_ = packet_canceling;
    cancel_ev_ = cancel_ev_new;
    prices_ = prices_new;
  }

  void continueCancelThemAll() {
    require(int_sender() == tvm_myaddr(), error_code::message_sender_is_not_my_owner);
    tvm_accept();
    auto [packet_canceling, cancel_ev, prices] = cancel_them_all_impl(cancel_ev_, prices_);
    packet_canceling_ = packet_canceling;
    cancel_ev_ = cancel_ev;
    prices_ = prices;
  }

  void unwrapWallet(
    uint128     evers_value,
    uint256     out_pubkey,
    address_opt out_owner,
    address     my_tip3_addr,
    uint128     tokens,
    opt<cell>   notify
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    tvm_commit();

    ITONTokenWalletPtr my_tip3(my_tip3_addr);
    my_tip3(Evers(evers_value.get())).
      unwrap(out_pubkey, out_owner, tokens, notify);
  }

  void bindWallet(
    uint128        evers,
    address        my_tip3_addr,
    bool           set_binding,
    opt<bind_info> binding,
    bool           set_trader,
    opt<uint256>   trader
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    tvm_commit();

    ITONTokenWalletPtr my_tip3(my_tip3_addr);
    my_tip3(Evers(evers.get())).
      bind(set_binding, binding, set_trader, trader);
  }

  /// Implementation of ITONTokenWalletNotify::onTip3Transfer.
  /// Notification from tip3 wallet to its owner contract about received tokens transfer.
  resumable<void> onTip3Transfer(
    [[maybe_unused]] uint128        balance,
    [[maybe_unused]] uint128        new_tokens,
                     uint128        evers_balance,
                     Tip3Config     tip3cfg,
                     opt<Tip3Creds> sender,
                     Tip3Creds      receiver,
    [[maybe_unused]] cell           payload,
    [[maybe_unused]] address        answer_addr
  ) {
    require(user_id_index_code_, error_code::uninitialized);
    auto workchain_id = std::get<addr_std>(tvm_myaddr().val()).workchain_id;
    auto expected_hash = calc_int_wallet_init_hash(
      tip3cfg, receiver.pubkey, receiver.owner,
      uint256(TIP3_WALLET_CODE_HASH), uint16(TIP3_WALLET_CODE_DEPTH), workchain_id
      );
    require(int_sender() == address::make_std(workchain_id, expected_hash), error_code::unverified_tip3_wallet);
    tvm_accept();

    auto user_id = receiver.pubkey;
    auto salted_code = tvm_add_code_salt(UserIdIndexSalt{tvm_myaddr(), auth_index_code_.get()}, user_id_index_code_.get());
    [[maybe_unused]] auto [init, hash] = prepare<IUserIdIndex>(DUserIdIndex{.user_id_ = user_id}, salted_code);
    IUserIdIndexPtr ptr(address::make_std(workchain_id, hash));
    auto lend_pubkey = co_await ptr(_remaining_ev()).requestLendPubkey(evers_balance);

    ITONTokenWalletPtr wallet(address::make_std(workchain_id, expected_hash));
    wallet(_remaining_ev()).bind(true, binding_, true, lend_pubkey);
  }

  cell getPayloadForDeployInternalWallet(
    uint256     owner_pubkey,
    address_opt owner_addr,
    uint128     evers,
    uint128     keep_evers
  ) {
    return build(FlexDeployWalletArgs{owner_pubkey, owner_addr, evers, keep_evers}).endc();
  }

  cell getPayloadForEverReTransferArgs(
    uint128 wallet_deploy_evers, ///< Evers to be sent to the deployable wallet.
    uint128 wallet_keep_evers    ///< Evers to be kept in the deployable wallet.
  ) {
    return build(EverReTransferArgs{wallet_deploy_evers, wallet_keep_evers}).endc();
  }

  address getPriceXchgAddress(
    uint128 price_num,        ///< Price numerator for rational price value
    cell    salted_price_code ///< Code of PriceXchg contract (salted!).
  ) {
    [[maybe_unused]] auto [state_init, addr, std_addr] = preparePriceXchg(price_num, salted_price_code);
    return addr;
  }

  address getUserIdIndex(
    uint256 user_id
  ) {
    require(user_id_index_code_, error_code::uninitialized);
    auto workchain_id = std::get<addr_std>(tvm_myaddr().val()).workchain_id;
    auto salted_code = tvm_add_code_salt(UserIdIndexSalt{tvm_myaddr(), auth_index_code_.get()}, user_id_index_code_.get());
    [[maybe_unused]] auto [init, hash] = prepare<IUserIdIndex, DUserIdIndex>({.user_id_=user_id}, salted_code);
    return address::make_std(workchain_id, hash);
  }

  FlexClientDetails getDetails() {
    require(auth_index_code_ && user_id_index_code_, error_code::uninitialized);
    return { owner_, triplet_, ex_triplet_, auth_index_code_.get(), user_id_index_code_.get() };
  }

  // default processing of unknown messages
  static int _fallback([[maybe_unused]] cell msg, [[maybe_unused]] slice msg_body) {
    return 0;
  }
  // default processing of empty messages
  static int _receive([[maybe_unused]] cell msg, [[maybe_unused]] slice msg_body) {
    return 0;
  }

private:
  std::tuple<StateInit, address, uint256> preparePriceXchg(
      uint128 price_num, cell price_code) const {

    DPriceXchg price_data {
      .price_num_    = price_num,
      .sells_amount_ = 0u128,
      .buys_amount_  = 0u128,
      .sells_        = {},
      .buys_         = {}
    };
    auto workchain_id = std::get<addr_std>(tvm_myaddr().val()).workchain_id;
    auto [state_init, std_addr] = prepare<IPriceXchg>(price_data, price_code);
    auto addr = address::make_std(workchain_id, std_addr);
    return { state_init, addr, std_addr };
  }
};

DEFINE_JSON_ABI(IFlexClient, DFlexClient, EFlexClient, FlexClient::replay_protection_t);

// ----------------------------- Main entry functions ---------------------- //
DEFAULT_MAIN_ENTRY_FUNCTIONS(FlexClient, IFlexClient, DFlexClient, TIMESTAMP_DELAY)
