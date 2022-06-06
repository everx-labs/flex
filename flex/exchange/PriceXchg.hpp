/** \file
 *  \brief PriceXchg contract interfaces and data-structs
 *  PriceXchg - contract to enqueue and process tip3-tip3 exchange orders at a specific price
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#pragma once

#include <tvm/schema/message.hpp>

#include <tvm/replay_attack_protection/timestamp.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/small_dict_map.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/big_queue.hpp>
#include <tvm/string.hpp>
#include <tvm/numeric_limits.hpp>
#include <tvm/suffixes.hpp>

#include "Flex.hpp"
#include "PriceCommon.hpp"
#include "FlexWallet.hpp"
#include "FlexTransferPayloadArgs.hpp"
#include "FlexLendPayloadArgs.hpp"
#include "PriceXchgSalt.hpp"
#include "RationalValue.hpp"

namespace tvm {

using ITONTokenWalletPtr = handle<ITONTokenWallet>;

/// \brief Safe delay period for lend ownership (in seconds).
/// For tip3 we have lend_finish_time, when wallet ownership will return back to its original owner.
/// And we need some safe period to not process orders with soon expiring tip3 ownership.
static constexpr unsigned safe_delay_period = 5 * 60;

__always_inline
bool is_active_time(uint32 order_finish_time) {
  return tvm_now() < static_cast<int>(order_finish_time.get());
}

/// Calculate cost (amount of minor tip3 tokens to match the "amount" of major tip3 tokens).
/// Zero or 128-bit overflowing value means error.
__always_inline
std::optional<uint128> minor_cost(uint128 amount, price_t price) {
  unsigned cost = __builtin_tvm_muldivr(amount.get(), price.numerator().get(), price.denominator().get());
  if ((cost >> 128) || (cost == 0))
    return {};
  return uint128{cost};
}

/// tip3-tip3 exchange order info
struct OrderInfoXchg {
  bool      immediate_client;         ///< Should this order try to be executed as a client order first (find existing corresponding orders).
  bool      post_order;               ///< Should this order be enqueued if it doesn't already have corresponding orders.
  uint128   original_amount;          ///< Original amount of major tokens to buy or sell.
  uint128   amount;                   ///< Current remaining amount of major tokens to buy or sell.
  uint128   account;                  ///< Remaining native funds from client to pay for processing.
  uint128   lend_amount;              ///< Current remaining amount of lend tokens (major tokens for sell, minor tokens for buy).
  addr_std_fixed tip3_wallet_provide; ///< Client tip3 wallet to provide tokens (major for sell or minor for buy).
  addr_std_fixed client_addr;         ///< Client contract address. PriceXchg will execute cancels from this address,
                                      ///<  send notifications and return the remaining native funds (evers) to this address.
  uint32    order_finish_time;        ///< Order finish time
  uint256   user_id;                  ///< User id
  uint256   order_id;                 ///< Order id
  uint64    ltime;                    ///< Logical time of starting transaction for the order
};
using OrderInfoXchgWithIdx = std::pair<unsigned, OrderInfoXchg>;

/// PriceXchg contract details (for getter)
struct PriceXchgDetails {
  uint128                   price_num; ///< Price numerator in minor tokens for one minor token - rational number, denominator kept in config.
  dict_array<OrderInfoXchg> sells;     ///< Sell orders.
  dict_array<OrderInfoXchg> buys;      ///< Buy orders.
  PriceXchgSalt             salt;      ///< Configuration from code salt
};

/** \interface IPriceXchg
 *  \brief PriceXchg contract interface.
 *
 *  PriceXchg - contract to enqueue and process tip3-tip3 exchange orders at a specific price.
 */
__interface IPriceXchg {

  /// \brief Implementation of ITONTokenWalletNotify::onTip3LendOwnership().
  /** Tip3 wallet notifies PriceXchg contract about lent token balance. **/
  [[internal, noaccept, answer_id, deploy]]
  OrderRet onTip3LendOwnership(
    uint128     balance,          ///< Lend token balance (amount of tokens to participate in a deal)
    uint32      lend_finish_time, ///< Lend ownership finish time
    Tip3Creds   creds,            ///< Wallet's credentials (pubkey + owner)
    cell        payload,          ///< Payload, must be PayloadArgs struct
    address     answer_addr       ///< Answer address
  ) = 201;

  /// \brief Process enqueued orders.
  /** This function is called from the PriceXchg itself when onTip3LendOwnership processing hits deals limit.
      Or when processQueue processing hits deals limit also. **/
  [[internal, noaccept]]
  void processQueue() = 202;

  /// Will cancel all sell/buy orders with this sender's client_addr.
  [[internal, noaccept]]
  void cancelOrder(
    bool         sell,    ///< Cancel sell order(s)
    opt<uint256> user_id, ///< Is user_id is specified, only orders with this user_id will be canceled
    opt<uint256> order_id ///< Is order_id is specified, only orders with this order_id will be canceled
  ) = 203;

  /// Will cancel orders, may be requested only from FlexWallet
  [[internal, noaccept]]
  void cancelWalletOrder(
    bool         sell,    ///< Cancel sell order(s)
    address      owner,   ///< FlexWallet's owner (FlexClient)
    uint256      user_id, ///< FlexWallet's public key (also, it is User Id)
    opt<uint256> order_id ///< Is order_id is specified, only orders with this order_id will be canceled
  ) = 205;

  /// Get contract details
  [[getter]]
  PriceXchgDetails getDetails() = 206;
};
using IPriceXchgPtr = handle<IPriceXchg>;

/// PriceXchg persistent data struct
struct DPriceXchg {
  uint128 price_num_;    ///< Price numerator in minor tokens for one minor token - rational number, denominator kept in config.
  uint128 sells_amount_; ///< Common amount of major tokens to sell.
                         /// \warning May be not strictly actual because of possible expired orders in the queue.
  uint128 buys_amount_;  /// Common amount of major tokens to buy.
                         /// \warning May be not strictly actual because of possible expired orders in the queue.

  big_queue<OrderInfoXchg> sells_; ///< Queue of sell orders.
  big_queue<OrderInfoXchg> buys_;  ///< Queue of buy orders.
};

/// \interface EPriceXchg
/// \brief PriceXchg events interface
__interface EPriceXchg {
};

/// Prepare StateInit struct and std address to deploy PriceXchg contract
template<>
struct preparer<IPriceXchg, DPriceXchg> {
  __always_inline
  static std::pair<StateInit, uint256> execute(DPriceXchg data, cell code) {
    cell data_cl = prepare_persistent_data<IPriceXchg, void>({}, data);
    StateInit init { {}, {}, code, data_cl, {} };
    cell init_cl = build(init).make_cell();
    return { init, uint256(tvm_hash(init_cl)) };
  }
};

} // namespace tvm

