/** \file
 *  \brief FlexClient contract interfaces and data-structs
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#pragma once

#include <tvm/schema/message.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>

#include "PriceXchg.hpp"
#include "FlexVersion.hpp"
#include "FlexClientStub.hpp"
#include "EverReTransferArgs.hpp"

namespace tvm {

/// Initial persistent data structure
using DFlexClient0 = DFlexClientStub;

/// FlexClient details (for getter)
struct FlexClientDetails {
  uint256          owner;              ///< Owner's public key
  FlexVersion      triplet;            ///< Version triplet
  opt<FlexVersion> ex_triplet;         ///< Ex-version triplet (initialized during code upgrade)
  cell             auth_index_code;    ///< AuthIndex code
  cell             user_id_index_code; ///< UserIdIndex code
};

/// Burn parameters for each wallet in `burnThemAll`
struct BurnInfo {
  uint256     out_pubkey; ///< Public key for external wallet (out)
  address_opt out_owner;  ///< Internal (contract) owner for external wallet (out)
  address     wallet;     ///< Wallet address
  opt<cell>   notify;     ///< Notification payload to the destination wallet's owner
                          ///<  (or EverReTransferArgs for (old WrapperEver)->(new WrapperEver) transfer)
};

/// \brief FlexClient is client contract for Flex
/** FlexClient keeps ownership of flex token wallets. Keeps/spends evers balance for processing.
 *  Makes orders.
 **/
__interface IFlexClient {

  /// Deploy tip3-tip3 PriceXchg contract with sell or buy order
  [[external]]
  address deployPriceXchg(
    bool       sell,                 ///< Is it a sell order
    bool       immediate_client,     ///< Should this order try to be executed as a client order first
                                     ///<  (find existing corresponding orders).
    bool       post_order,           ///< Should this order be enqueued if it doesn't already have corresponding orders.
    uint128    price_num,            ///< Price numerator for rational price value
    uint128    amount,               ///< Amount of major tip3 tokens to sell or buy
    uint128    lend_amount,          ///< Lend amount. For sell, it should be amount of major tokens, for buy - minor.
    uint32     lend_finish_time,     ///< Lend finish time (order finish time also will be lend_finish_time - safe_period)
    uint128    evers,                ///< Processing evers
    cell       unsalted_price_code,  ///< Unsalted PriceXchg code
    cell       price_salt,           ///< PriceXchg code salt (configuration)
    address    my_tip3_addr,         ///< Address of flex tip3 token wallet to provide tokens
    uint256    user_id,              ///< User id
    uint256    order_id              ///< Order id
  ) = 10;

  /// Cancel tip3-tip sell or buy order
  [[external]]
  void cancelXchgOrder(
    bool         sell,              ///< Is it a sell order
    uint128      price_num,         ///< Price numerator for rational price value
    uint128      value,             ///< Processing evers
    cell         salted_price_code, ///< Code of PriceXchg contract (salted)
    opt<uint256> user_id,           ///< Is user_id is specified, only orders with this user_id will be canceled
    opt<uint256> order_id           ///< Is order_id is specified, only orders with this order_id will be canceled
  ) = 11;

  /// Transfer evers
  [[external]]
  void transfer(
    address dest,  ///< Destination address
    uint128 value, ///< Amount of evers
    bool    bounce ///< Bounce flag
  ) = 12;

  /// Transfer tokens
  [[external]]
  void transferTokens(
    address   src,       ///< Source address
    Tip3Creds dst,       ///< Destination credentials (pubkey + owner)
    uint128   tokens,    ///< Amount of tokens
    uint128   evers,     ///< Amount of processing evers
    uint128   keep_evers ///< Evers to keep in destination wallet
  ) = 13;

  /// Deploy an empty flex tip3 token wallet, owned by FlexClient contract
  [[external]]
  address deployEmptyFlexWallet(
    uint256        pubkey,          ///< Public key (for identification only)
    uint128        evers_to_wallet, ///< Evers to the wallet
    Tip3Config     tip3cfg,         ///< Tip3 token configuration
    uint256        trader,          ///< Trader (lend pubkey) info for `bind` call
    cell           flex_wallet_code ///< Flex wallet code
  ) = 14;

  /// Deploy UserIdIndex contract
  [[external]]
  void deployIndex(
    uint256 user_id,           ///< User id
    uint256 lend_pubkey,       ///< Lend public key
    string  name,              ///< User name (encoded)
    uint128 evers_all,         ///< Evers for all processing: UserIdIndex deploy and included AuthIndex deploy
    uint128 evers_to_auth_idx, ///< Evers to AuthIndex deploy
    uint128 refill_wallet,     ///< Re-fill wallet on transfer received
    uint128 min_refill         ///< Minimum refill value
  ) = 15;

  /// Change lend_pubkey into UserIdIndex contract(call UserIdIndex.reLendPubkey) if set_trader & trader.
  /// Call bind(set_binding, binding, set_trader, trader) for every wallet from wallets.
  [[external]]
  void reBindWallets(
    uint256             user_id,                ///< User id
    bool                set_binding,            ///< Set binding
    opt<bind_info>      binding,                ///< If `set_binding` is true, binding will be set into wallets
    bool                set_trader,             ///< Set trader
    opt<uint256>        trader,                 ///< If `set_trader` is true, trader (lend pubkey) will be set into wallets and UserIdIndex
    dict_array<address> wallets,                ///< Array of wallet addresses
    uint128             evers_relend_call,      ///< Evers for `UserIdIndex->reLendPubkey` call
    uint128             evers_each_wallet_call, ///< Evers for each `FlexWallet->lendOwnershipPubkey` call
    uint128             evers_to_remove,        ///< Evers to send in `AuthIndex->remove` call inside `UserIdIndex->reLendPubkey`
    uint128             evers_to_auth_idx       ///< Evers to send in new AuthIndex deploy call inside `UserIdIndex->reLendPubkey`
  ) = 16;

  /// Remove UserIdIndex contract
  [[external]]
  void destroyIndex(
    uint256 user_id, ///< User id
    uint128 evers    ///< Evers to send to `UserIdIndex->remove`, inside - to `AuthIndex->remove`. The remaining will return.
  ) = 17;

  /// To convert flex tip3 tokens back to external tip3 tokens (with destruction of the wallet).
  [[external]]
  void burnWallet(
    uint128     evers_value,    ///< Processing evers
    uint256     out_pubkey,     ///< Public key for external wallet (out)
    address_opt out_owner,      ///< Internal (contract) owner for external wallet (out)
    address     my_tip3_addr,   ///< Address of flex tip3 token wallet to burn (convert to external tokens)
    opt<cell>   notify          ///< Notification payload to the destination wallet's owner
                                ///<  (or EverReTransferArgs for (old WrapperEver)->(new WrapperEver) transfer)
  ) = 18;

  /// To convert many flex tip3 tokens back to external tip3 tokens (with destruction of the wallets).
  [[external]]
  void burnThemAll(
    uint128              burn_ev, ///< Processing evers for each wallet `burn` call
    dict_array<BurnInfo> burns    ///< Array of burn parameters for each wallet
  ) = 19;

  /// Will send to itself from burnThemAll if wallets are more than 255
  [[internal]]
  void continueBurnThemAll();

  /// To cancel all orders for the provided wallets
  [[external]]
  void cancelThemAll(
    uint128             cancel_ev, ///< Processing evers for each price `cancelOrder` call.
                                   ///< Two such calls for each price (cancel sells and cancel buys).
    dict_array<address> prices     ///< Array of PriceXchg addresses
  ) = 20;

  /// Will send to itself from cancelThemAll if prices are more than 255 / 2 (we need two messages per price)
  [[internal]]
  void continueCancelThemAll();

  /// To convert some flex tip3 tokens back to external tip3 tokens
  [[external]]
  void unwrapWallet(
    uint128     evers_value,    ///< Processing evers
    uint256     out_pubkey,     ///< Public key for external wallet (out)
    address_opt out_owner,      ///< Internal (contract) owner for external wallet (out)
    address     my_tip3_addr,   ///< Address of flex tip3 token wallet to burn (convert to external tokens)
    uint128     tokens,         ///< Tokens amount to unwrap (withdraw)
    opt<cell>   notify          ///< Notification payload to the destination wallet's owner
                                ///<  (or EverReTransferArgs for (old WrapperEver)->(new WrapperEver) transfer)
  ) = 21;

  /// Bind trading wallet to a specific flex and PriceXchg code hash and set trader (lend pubkey)
  [[external]]
  void bindWallet(
    uint128        evers,        ///< Processing evers
    address        my_tip3_addr, ///< Address of flex tip3 token wallet
    bool           set_binding,  ///< Set binding
    opt<bind_info> binding,      ///< if `set_binding` is true, binding will be set
    bool           set_trader,   ///< Set trader
    opt<uint256>   trader        ///< if `set_trader` is true, trader (lend pubkey) will be set
  ) = 22;

  /// Implementation of ITONTokenWalletNotify::onTip3Transfer.
  /// Notification from tip3 wallet to its owner contract about received tokens transfer.
  [[internal, answer_id]]
  resumable<void> onTip3Transfer(
    uint128        balance,       ///< New balance of the wallet.
    uint128        new_tokens,    ///< Amount of tokens received in transfer.
    uint128        evers_balance, ///< Evers balance of the wallet.
    Tip3Config     tip3cfg,       ///< Tip3 config.
    opt<Tip3Creds> sender,        ///< Sender wallet's credentials (pubkey + owner). Empty if mint received from root/wrapper.
    Tip3Creds      receiver,      ///< Receiver wallet's credentials (pubkey + owner).
    cell           payload,       ///< Payload (must be FlexTransferPayloadArgs).
    address        answer_addr    ///< Answer address (to receive answer and the remaining processing evers).
  ) = 202;

  /// Upgrade FlexClient to the specific UserDataConfig
  [[external]]
  resumable<void> upgrade(
    uint128 request_evers, ///< Evers to send in UserDataConfig::requestDetails()
    address user_data_cfg  ///< UserDataConfig address
  ) = 23;

  /// Prepare payload for transferWithNotify call from external wallet to wrapper's wallet
  ///  to deploy flex internal wallet
  [[getter]]
  cell getPayloadForDeployInternalWallet(
    uint256     owner_pubkey, ///< Owner's public key
    address_opt owner_addr,   ///< Owner's internal address (contract)
    uint128     evers,        ///< Processing evers
    uint128     keep_evers    ///< Evers to keep in the wallet
  ) = 24;

  /// Prepare payload for burn for WrapperEver -> WrapperEver transfer (EverReTransferArgs struct)
  [[getter]]
  cell getPayloadForEverReTransferArgs(
    uint128 wallet_deploy_evers, ///< Evers to be sent to the deployable wallet.
    uint128 wallet_keep_evers    ///< Evers to be kept in the deployable wallet.
  ) = 25;

  /// Get PriceXchg address
  [[getter]]
  address getPriceXchgAddress(
    uint128 price_num,        ///< Price numerator for rational price value
    cell    salted_price_code ///< Code of PriceXchg contract (salted!).
  ) = 26;

  /// Return UserIdIndex address
  [[getter]]
  address getUserIdIndex(
    uint256 user_id ///< User id
  ) = 27;

  /// Get contract state details
  [[getter]]
  FlexClientDetails getDetails() = 28;
};
using IFlexClientPtr = handle<IFlexClient>;

/// FlexClient persistent data struct
struct DFlexClient1 {
  uint256              owner_;              ///< Owner's public key
  FlexVersion          triplet_;            ///< Version triplet.
  FlexVersion          ex_triplet_;         ///< Ex-version triplet (initialized during code upgrade)
  optcell              auth_index_code_;    ///< AuthIndex code
  optcell              user_id_index_code_; ///< UserIdIndex code
  opt<bind_info>       binding_;            ///< Binding info for exchange
  bool_t               packet_burning_;     ///< When burnThemAll was postponed into continueBurnThemAll call
  uint128              burn_ev_;            ///< Processing evers for each wallet `burn` call
  dict_array<BurnInfo> burns_;              ///< Array of burn parameters for each wallet
  bool_t               packet_canceling_;   ///< When cancelThemAll was postponed into continueCancelThemAll call
  uint128              cancel_ev_;          ///< Processing evers for each wallet `cancelOrder` call
  dict_array<address>  prices_;             ///< Array of PriceXchg addresses
};

using DFlexClient = DFlexClient1;

/// \interface EFlexClient
/// \brief FlexClient events interface
__interface EFlexClient {
};

} // namespace tvm

