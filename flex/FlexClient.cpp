/** \file
 *  \brief FlexClient contract implementation
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#include "FlexClient.hpp"
#include "XchgPair.hpp"
#include "Wrapper.hpp"

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
  };

  __always_inline
  void constructor(uint256 pubkey, uint256 user_id) {
    require(pubkey != 0, error_code::zero_owner_pubkey);
    owner_ = pubkey;
    workchain_id_ = std::get<addr_std>(address{tvm_myaddr()}.val()).workchain_id;
    flex_ = address::make_std(0i8, 0u256);
    user_id_ = user_id;
  }

  __always_inline
  void setFlexCfg(EversConfig ev_cfg, address flex) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    ev_cfg_ = ev_cfg;
    flex_ = flex;
  }

  __always_inline
  void setFlexWalletCode(cell flex_wallet_code) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    flex_wallet_code_ = flex_wallet_code;
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
    lend_user_ids_.set_at(user_id, {
      .pubkey = pubkey,
      .lend_finish_time = lend_finish_time
    });
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
    require(lend_user_ids_.contains(user_id), error_code::user_id_not_found);
    tvm_accept();
    if (auto existing = lend_user_ids_.lookup(user_id)) {
      ITONTokenWalletPtr my_tip3(my_tip3_addr);
      my_tip3(Evers(evers.get())).lendOwnershipPubkey(new_key, {});
    }
  }

  __always_inline
  void forgetUserId(
    uint256      user_id
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    lend_user_ids_.erase(user_id);
  }

  __always_inline
  void cancelXchgOrder(
    bool       sell,
    uint128    price_num,
    uint128    price_denum,
    uint128    value,
    cell       xchg_price_code
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();

    auto [state_init, addr, std_addr] = preparePriceXchg(price_num, price_denum, xchg_price_code);
    IPriceXchgPtr price_addr(addr);
    if (sell)
      price_addr(Evers(value.get())).cancelSell();
    else
      price_addr(Evers(value.get())).cancelBuy();
  }

  __always_inline
  void transfer(address dest, uint128 value, bool bounce) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    tvm_transfer(dest, value.get(), bounce);
  }

  __always_inline
  address deployPriceXchg(
    bool    sell,
    bool    immediate_client,
    bool    post_order,
    uint128 price_num,
    uint128 price_denum,
    uint128 amount,
    uint128 lend_amount,
    uint32  lend_finish_time,
    uint128 evers,
    cell    xchg_price_code,
    address my_tip3_addr,
    address receive_wallet,
    uint256 order_id
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    require(price_num != 0, error_code::zero_num_in_price);
    require(price_denum != 0, error_code::zero_denum_in_price);
    tvm_accept();

    auto [state_init, addr, std_addr] = preparePriceXchg(price_num, price_denum, xchg_price_code);
    auto price_addr = IPriceXchgPtr(addr);
    cell deploy_init_cl = build(state_init).endc();
    FlexLendPayloadArgs payload_args = {
      .sell = sell,
      .immediate_client = immediate_client,
      .post_order = post_order,
      .amount = amount,
      .receive_tip3_wallet = receive_wallet,
      .client_addr = address{tvm_myaddr()},
      .user_id = user_id_,
      .order_id = order_id
    };
    cell payload = build(payload_args).endc();

    ITONTokenWalletPtr my_tip3(my_tip3_addr);
    my_tip3(Evers(evers.get())).
      lendOwnership(address{tvm_myaddr()}, 0u128, addr, lend_amount,
                    lend_finish_time, deploy_init_cl, payload);
    return price_addr.get();
  }

  __always_inline
  void registerWrapper(
    uint256 wrapper_pubkey,
    uint128 value,
    Tip3Config tip3cfg
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    IFlexPtr(flex_)(Evers(value.get())).registerWrapper(wrapper_pubkey, tip3cfg);
  }

  __always_inline
  void registerXchgPair(
    uint256 request_pubkey,
    uint128 value,
    address tip3_major_root,
    address tip3_minor_root,
    Tip3Config major_tip3cfg,
    Tip3Config minor_tip3cfg,
    uint128 min_amount,
    address notify_addr
  ) {
    require(msg_pubkey() == owner_, error_code::message_sender_is_not_my_owner);
    tvm_accept();
    IFlexPtr(flex_)(Evers(value.get())).
      registerXchgPair(request_pubkey, tip3_major_root, tip3_minor_root,
                       major_tip3cfg, minor_tip3cfg, min_amount, notify_addr);
  }

  __always_inline
  address deployEmptyFlexWallet(
    uint256    pubkey,
    uint128    evers_to_wallet,
    Tip3Config tip3cfg
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
    new_wallet.deploy_noop(init, Evers(evers_to_wallet.get()));
    return new_wallet.get();
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
  uint256 getUserId() {
    return user_id_;
  }

  __always_inline
  lend_user_ids_array getLendUserIds() {
    return lend_user_ids_array(lend_user_ids_.begin(), lend_user_ids_.end());
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
  cell getPayloadForPriceXchg(
    bool    sell,                ///< Sell order if true, buy order if false.
    bool    immediate_client,    ///< Should this order try to be executed as a client order first
                                 ///<  (find existing corresponding orders).
    bool    post_order,          ///< Should this order be enqueued if it doesn't already have corresponding orders.
    uint128 amount,              ///< Amount of major tokens to buy or sell.
    address receive_tip3_wallet, ///< Client tip3 wallet to receive tokens (minor for sell or major for buy)
    address client_addr,         ///< Client contract address. PriceXchg will execute cancels from this address,
                                 ///<  send notifications and return the remaining native funds (evers) to this address.
    uint256 user_id,             ///< User id for client purposes.
    uint256 order_id             ///< Order id for client purposes.
  ) {
    return build_chain_static(FlexLendPayloadArgs{
      sell, immediate_client, post_order, amount, receive_tip3_wallet, client_addr, user_id, order_id
      });
  }

  __always_inline
  std::pair<cell, address> getStateInitForPriceXchg(
    uint128    price_num,            ///< Price numerator for rational price value
    uint128    price_denum,          ///< Price denominator for rational price value
    cell       xchg_price_code       ///< Code of PriceXchg contract
  ) {
    [[maybe_unused]] auto [state_init, addr, std_addr] =
      preparePriceXchg(price_num, price_denum, xchg_price_code);
    cell deploy_init_cl = build(state_init).endc();
    return { deploy_init_cl, addr };
  }

  // =============== Support functions ==================
  DEFAULT_SUPPORT_FUNCTIONS(IFlexClient, replay_protection_t)

  // default processing of unknown messages
  __always_inline static int _fallback(cell /*msg*/, slice /*msg_body*/) {
    return 0;
  }

private:
  __always_inline
  std::tuple<StateInit, address, uint256> preparePriceXchg(
      uint128 price_num, uint128 price_denum, cell price_code) const {

    DPriceXchg price_data {
      .price_ = { price_num, price_denum },
      .sells_amount_ = 0u128,
      .buys_amount_  = 0u128,
      .sells_ = {},
      .buys_  = {}
    };
    auto [state_init, std_addr] = prepare_price_xchg_state_init_and_addr(price_data, price_code);
    auto addr = address::make_std(workchain_id_, std_addr);
    return { state_init, addr, std_addr };
  }
};

DEFINE_JSON_ABI(IFlexClient, DFlexClient, EFlexClient, FlexClient::replay_protection_t);

// ----------------------------- Main entry functions ---------------------- //
DEFAULT_MAIN_ENTRY_FUNCTIONS(FlexClient, IFlexClient, DFlexClient, TIMESTAMP_DELAY)


