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
  return tvm_now() + safe_delay_period < order_finish_time.get();
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
  bool    immediate_client; ///< Should this order try to be executed as a client order first (find existing corresponding orders).
  bool    post_order;       ///< Should this order be enqueued if it doesn't already have corresponding orders.
  uint128 original_amount;  ///< Original amount of major tokens to buy or sell.
  uint128 amount;           ///< Current remaining amount of major tokens to buy or sell.
  uint128 account;          ///< Remaining native funds from client to pay for processing.
  uint128 lend_amount;      ///< Current remaining amount of lend tokens (major tokens for sell, minor tokens for buy).
  addr_std_fixed tip3_wallet_provide; ///< Client tip3 wallet to provide tokens (major for sell or minor for buy).
  addr_std_fixed tip3_wallet_receive; ///< Client tip3 wallet to receive tokens (minor for sell or major for buy).
  addr_std_fixed client_addr;         ///< Client contract address. PriceXchg will execute cancels from this address,
                                      ///<  send notifications and return the remaining native funds (evers) to this address.
  uint32  order_finish_time;          ///< Order finish time
  uint256 user_id;                    ///< User id
  uint256 order_id;                   ///< Order id
  uint64  ltime;                      ///< Logical time of starting transaction for the order
};
using OrderInfoXchgWithIdx = std::pair<unsigned, OrderInfoXchg>;

/** \interface IPriceXchg
 *  \brief PriceXchg contract interface.
 *
 *  PriceXchg - contract to enqueue and process tip3-tip3 exchange orders at a specific price.
 */
__interface IPriceXchg {

  /// \brief Implementation of ITONTokenWalletNotify::onTip3LendOwnership().
  /** Tip3 wallet notifies PriceXchg contract about lent token balance. **/
  [[internal, noaccept, answer_id]]
  OrderRet onTip3LendOwnership(
    uint128     balance,          ///< Lend token balance (amount of tokens to participate in a deal)
    uint32      lend_finish_time, ///< Lend ownership finish time
    uint256     pubkey,           ///< Public key of the tip3 wallet
    address_opt owner,            ///< Internal owner address of the tip3 wallet
    cell        payload,          ///< Payload, must be PayloadArgs struct
    address     answer_addr       ///< Answer address
  ) = 201;

  /// \brief Process enqueued orders.
  /** This function is called from the PriceXchg itself when onTip3LendOwnership processing hits deals limit.
      Or when processQueue processing hits deals limit also. **/
  [[internal, noaccept]]
  void processQueue() = 203;

  /// Will cancel all sell orders with this sender's client_addr.
  [[internal, noaccept]]
  void cancelSell() = 204;

  /// Will cancel all buy orders with this sender's client_addr.
  [[internal, noaccept]]
  void cancelBuy() = 205;

  /// Get sells queue
  [[getter]]
  dict_array<OrderInfoXchg> getSells() = 206;

  /// Get buys queue
  [[getter]]
  dict_array<OrderInfoXchg> getBuys() = 207;

  /// Get Price configuration from code salt (common for prices of one pair).
  [[getter]]
  PriceXchgSalt getConfig() = 208;
};
using IPriceXchgPtr = handle<IPriceXchg>;

/// PriceXchg persistent data struct
struct DPriceXchg {
  price_t price_;        ///< Price in minor tokens for one minor token - rational number
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
__always_inline
std::pair<StateInit, uint256> prepare_price_xchg_state_init_and_addr(DPriceXchg price_data, cell price_code) {
  cell price_data_cl = prepare_persistent_data<IPriceXchg, void, DPriceXchg>({}, price_data);
  StateInit price_init {
    /*split_depth*/{}, /*special*/{},
    price_code, price_data_cl, /*library*/{}
  };
  cell price_init_cl = build(price_init).make_cell();
  return { price_init, uint256(tvm_hash(price_init_cl)) };
}

} // namespace tvm

