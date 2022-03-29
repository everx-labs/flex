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

  struct error_code : tvm::error_code {
    static constexpr unsigned message_sender_is_not_my_owner = 100; ///< Message sender is not my owner
    static constexpr unsigned missed_flex_wallet_code        = 101; ///< Missed flex wallet code
    static constexpr unsigned zero_owner_pubkey              = 102; ///< Owner's public key can't be zero
    static constexpr unsigned zero_num_in_price              = 103; ///< Zero numerator in price
    static constexpr unsigned zero_denum_in_price            = 104; ///< Zero denominator in price
    static constexpr unsigned user_id_not_found              = 105; ///< User id not found
    static constexpr unsigned auth_index_must_be_defined     = 106; ///< AuthIndex code must be defined before
    static constexpr unsigned missed_user_id_index_code      = 107; ///< Missed UserIdIndex code
  };

  __always_inline
  void constructor(uint256 pubkey) {
    require(pubkey != 0, error_code::zero_owner_pubkey);
    owner_ = pubkey;
    workchain_id_ = std::get<addr_std>(address{tvm_myaddr()}.val()).workchain_id;
    flex_ = address::make_std(0i8, 0u256);
  }

  __always_inline
  void setFlexCfg(address flex) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    flex_ = flex;
  }

  __always_inline
  void setFlexWalletCode(cell flex_wallet_code) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    flex_wallet_code_ = flex_wallet_code;
  }

  __always_inline
  void setAuthIndexCode(cell auth_index_code) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();

    auth_index_code_ = tvm_add_code_salt(AuthIndexSalt{.flex = flex_}, auth_index_code);
  }

  __always_inline
  void setUserIdIndexCode(cell user_id_index_code) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    require(auth_index_code_, error_code::auth_index_must_be_defined);
    tvm_accept();

    user_id_index_code_ = tvm_add_code_salt(
      UserIdIndexSalt{.owner = tvm_myaddr(), .auth_index_code = auth_index_code_.get()}, user_id_index_code
      );
  }

  __always_inline
  void lendOwnershipPubkey(
    address my_tip3_addr,
    uint256 pubkey,
    uint256 user_id,
    uint128 evers,
    uint32  lend_finish_time
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    ITONTokenWalletPtr my_tip3(my_tip3_addr);
    my_tip3(Evers(evers.get())).lendOwnershipPubkey(pubkey, lend_finish_time);
  }

  __always_inline
  void reLendOwnershipPubkey(
    address      my_tip3_addr,
    uint256      user_id,
    opt<uint256> new_key,
    uint128      evers
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    ITONTokenWalletPtr my_tip3(my_tip3_addr);
    my_tip3(Evers(evers.get())).lendOwnershipPubkey(new_key, {});
  }

  __always_inline
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

    auto [state_init, addr, std_addr] = preparePriceXchg(price_num, salted_price_code);
    IPriceXchgPtr price_addr(addr);
    price_addr(Evers(value.get())).cancelOrder(sell, user_id, order_id);
  }

  __always_inline
  void transfer(address dest, uint128 value, bool bounce) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    tvm_transfer(dest, value.get(), bounce);
  }

  __always_inline
  void transferTokens(
    address src,
    address dst,
    uint128 tokens,
    uint128 evers
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    ITONTokenWalletPtr(src)(Evers(evers.get())).transfer(tvm_myaddr(), dst, tokens, 0u128, 0u128, {});
  }

  __always_inline
  address deployPriceXchg(
    bool      sell,
    bool      immediate_client,
    bool      post_order,
    uint128   price_num,
    uint128   amount,
    uint128   lend_amount,
    uint32    lend_finish_time,
    uint128   evers,
    cell      unsalted_price_code,
    cell      price_salt,
    address   my_tip3_addr,
    uint256   user_id,
    uint256   order_id
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    require(price_num != 0, error_code::zero_num_in_price);
    tvm_accept();

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

  __always_inline
  void registerWrapper(
    uint256    wrapper_pubkey,
    uint128    value,
    Tip3Config tip3cfg
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    IFlexPtr(flex_)(Evers(value.get())).registerWrapper(wrapper_pubkey, tip3cfg);
  }

  __always_inline
  void registerWrapperEver(
    uint256 wrapper_pubkey,
    uint128 value
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    IFlexPtr(flex_)(Evers(value.get())).registerWrapperEver(wrapper_pubkey);
  }

  __always_inline
  void registerXchgPair(
    uint256    request_pubkey,
    uint128    value,
    address    tip3_major_root,
    address    tip3_minor_root,
    Tip3Config major_tip3cfg,
    Tip3Config minor_tip3cfg,
    uint128    min_amount,
    uint128    minmove,
    uint128    price_denum,
    address    notify_addr
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    IFlexPtr(flex_)(Evers(value.get())).
      registerXchgPair(request_pubkey, tip3_major_root, tip3_minor_root,
                       major_tip3cfg, minor_tip3cfg, min_amount, minmove, price_denum, notify_addr);
  }

  __always_inline
  address deployEmptyFlexWallet(
    uint256               pubkey,
    uint128               evers_to_wallet,
    Tip3Config            tip3cfg,
    opt<client_lend_info> lend_info
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    require(flex_wallet_code_, error_code::missed_flex_wallet_code);
    tvm_accept();

    auto [init, hash_addr] = prepare_internal_wallet_state_init_and_addr(
      tip3cfg.name, tip3cfg.symbol, tip3cfg.decimals,
      tip3cfg.root_pubkey, tip3cfg.root_address,
      pubkey, address{tvm_myaddr()},
      uint256(tvm_hash(flex_wallet_code_.get())), uint16(flex_wallet_code_.get().cdepth()),
      workchain_id_, flex_wallet_code_.get()
      );
    ITONTokenWalletPtr new_wallet(address::make_std(workchain_id_, hash_addr));
    if (lend_info)
      new_wallet.deploy(init, Evers(evers_to_wallet.get())).lendOwnershipPubkey(lend_info->pubkey, lend_info->lend_finish_time);
    else
      new_wallet.deploy_noop(init, Evers(evers_to_wallet.get()));
    return new_wallet.get();
  }

  __always_inline
  void deployIndex(
    uint256 user_id,
    uint256 lend_pubkey,
    string  name,
    uint128 evers_all,
    uint128 evers_to_auth_idx
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    require(user_id_index_code_, error_code::missed_flex_wallet_code);
    tvm_accept();
    auto [init, std_addr] = prepare<IUserIdIndex, DUserIdIndex>({.user_id_=user_id}, user_id_index_code_.get());
    IUserIdIndexPtr ptr(address::make_std(workchain_id_, std_addr));
    ptr.deploy(init, Evers(evers_all.get())).onDeploy(lend_pubkey, name, evers_to_auth_idx);
  }

  __always_inline
  void reLendIndex(
    uint256             user_id,
    uint256             new_lend_pubkey,
    dict_array<address> wallets,
    uint128             evers_relend_call,
    uint128             evers_each_wallet_call,
    uint128             evers_to_remove,
    uint128             evers_to_auth_idx
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    IUserIdIndexPtr(getUserIdIndex(user_id))(Evers(evers_relend_call.get())).reLendPubkey(new_lend_pubkey, evers_to_remove, evers_to_auth_idx);
    for (auto addr : wallets) {
      ITONTokenWalletPtr(addr)(Evers(evers_each_wallet_call.get())).lendOwnershipPubkey(new_lend_pubkey, {});
    }
  }

  __always_inline
  void destroyIndex(
    uint256 user_id,
    uint128 evers
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    IUserIdIndexPtr(getUserIdIndex(user_id))(Evers(evers.get())).remove();
  }

  __always_inline
  void burnWallet(
    uint128     evers_value,
    uint256     out_pubkey,
    address_opt out_owner,
    address     my_tip3_addr
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();

    ITONTokenWalletPtr my_tip3(my_tip3_addr);
    my_tip3(Evers(evers_value.get())).
      burn(out_pubkey, out_owner);
  }

  __always_inline
  void setTradeRestriction(
    uint128 evers,
    address my_tip3_addr,
    address flex,
    uint256 unsalted_price_code_hash
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();

    ITONTokenWalletPtr my_tip3(my_tip3_addr);
    my_tip3(Evers(evers.get())).
      setTradeRestriction(flex, unsalted_price_code_hash);
  }

  /// Implementation of ITONTokenWalletNotify::onTip3Transfer.
  /// Notification from tip3 wallet to its owner contract about received tokens transfer.
  __always_inline
  void onTip3Transfer(
    uint128     balance,       ///< New balance of the wallet.
    uint128     new_tokens,    ///< Amount of tokens received in transfer.
    uint256     sender_pubkey, ///< Sender wallet pubkey.
    address_opt sender_owner,  ///< Sender wallet internal owner.
    cell        payload,       ///< Payload (must be FlexTransferPayloadArgs).
    address     answer_addr    ///< Answer address (to receive answer and the remaining processing evers).
  ) {
  }

  __always_inline
  uint256 getOwner() {
    return owner_;
  }

  __always_inline
  address getFlex() {
    return flex_;
  }

  __always_inline
  bool hasFlexWalletCode() {
    return flex_wallet_code_;
  }

  __always_inline
  bool hasAuthIndexCode() {
    return auth_index_code_;
  }

  __always_inline
  bool hasUserIdIndexCode() {
    return user_id_index_code_;
  }

  __always_inline
  cell getPayloadForDeployInternalWallet(
    uint256     owner_pubkey,
    address_opt owner_addr,
    uint128     evers
  ) {
    return build(FlexDeployWalletArgs{owner_pubkey, owner_addr, evers}).endc();
  }

  __always_inline
  address getPriceXchgAddress(
    uint128 price_num,        ///< Price numerator for rational price value
    cell    salted_price_code ///< Code of PriceXchg contract (salted!).
  ) {
    [[maybe_unused]] auto [state_init, addr, std_addr] = preparePriceXchg(price_num, salted_price_code);
    return addr;
  }

  __always_inline
  address getUserIdIndex(
    uint256 user_id ///< User id
  ) {
    require(user_id_index_code_, error_code::missed_user_id_index_code);
    [[maybe_unused]] auto [init, std_addr] = prepare<IUserIdIndex, DUserIdIndex>({.user_id_=user_id}, user_id_index_code_.get());
    return address::make_std(workchain_id_, std_addr);
  }

  // =============== Support functions ==================
  DEFAULT_SUPPORT_FUNCTIONS(IFlexClient, replay_protection_t)

  // default processing of unknown messages
  __always_inline static int _fallback(cell /*msg*/, slice /*msg_body*/) {
    return 0;
  }
  // default processing of empty messages
  __always_inline static int _receive(cell /*msg*/, slice /*msg_body*/) {
    return 0;
  }

private:
  __always_inline
  std::tuple<StateInit, address, uint256> preparePriceXchg(
      uint128 price_num, cell price_code) const {

    DPriceXchg price_data {
      .price_num_    = price_num,
      .sells_amount_ = 0u128,
      .buys_amount_  = 0u128,
      .sells_        = {},
      .buys_         = {}
    };
    auto [state_init, std_addr] = prepare<IPriceXchg>(price_data, price_code);
    auto addr = address::make_std(workchain_id_, std_addr);
    return { state_init, addr, std_addr };
  }
};

DEFINE_JSON_ABI(IFlexClient, DFlexClient, EFlexClient, FlexClient::replay_protection_t);

// ----------------------------- Main entry functions ---------------------- //
DEFAULT_MAIN_ENTRY_FUNCTIONS(FlexClient, IFlexClient, DFlexClient, TIMESTAMP_DELAY)
