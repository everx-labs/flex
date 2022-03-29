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

namespace tvm {

/// Lend info about user
struct client_lend_info {
  uint256 pubkey;           ///< User pubkey.
  uint32  lend_finish_time; ///< Last lend finish time. Will be used for lendOwnershipPubkey for bought tokens.
};

/// \brief FlexClient is client contract for Flex
/** FlexClient keeps ownership of flex token wallets. Keeps/spends evers balance for processing.
 *  Makes orders.
 **/
__interface IFlexClient {

  /// Constructor of the FlexClient
  [[external]]
  void constructor(
    uint256 pubkey ///< Owner's public key
  ) = 10;

  /// Set flex address and evers configuration
  [[external, noaccept]]
  void setFlexCfg(
    address        flex    ///< Address of Flex root contract
  ) = 11;

  /// Set flex wallet code for deployEmptyFlexWallet calls
  [[external, noaccept]]
  void setFlexWalletCode(cell flex_wallet_code) = 12;

  /// Set AuthIndex code
  [[external, noaccept]]
  void setAuthIndexCode(cell auth_index_code) = 13;

  /// Set UserIdIndex code
  [[external, noaccept]]
  void setUserIdIndexCode(cell user_id_index_code) = 14;

  /// Deploy tip3-tip3 PriceXchg contract with sell or buy order
  [[external, noaccept]]
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
  ) = 15;

  /// Lend ownership to an external owner (pubkey)
  [[external, noaccept]]
  void lendOwnershipPubkey(
    address      my_tip3_addr,    ///< Address of flex tip3 token wallet to lend tokens
    uint256      pubkey,          ///< Public key of the lend owner
    uint256      user_id,         ///< User id of the lend owner
    uint128      evers,           ///< Processing evers
    uint32       lend_finish_time ///< Lend finish time
  ) = 16;

  /// Re-lend ownership to an external owner (pubkey), or revoke.
  [[external, noaccept]]
  void reLendOwnershipPubkey(
    address      my_tip3_addr, ///< Address of flex tip3 token wallet to re-lend tokens
    uint256      user_id,      ///< User id to be re-lend or revoke
    opt<uint256> new_key,      ///< New public key. Revoke if empty.
    uint128      evers         ///< Processing evers
  ) = 17;

  /// Cancel tip3-tip sell or buy order
  [[external, noaccept]]
  void cancelXchgOrder(
    bool         sell,              ///< Is it a sell order
    uint128      price_num,         ///< Price numerator for rational price value
    uint128      value,             ///< Processing evers
    cell         salted_price_code, ///< Code of PriceXchg contract (salted)
    opt<uint256> user_id,           ///< Is user_id is specified, only orders with this user_id will be canceled
    opt<uint256> order_id           ///< Is order_id is specified, only orders with this order_id will be canceled
  ) = 18;

  /// Transfer evers
  [[external, noaccept]]
  void transfer(
    address dest,  ///< Destination address
    uint128 value, ///< Amount of evers
    bool    bounce ///< Bounce flag
  ) = 19;

  /// Transfer tokens
  [[external, noaccept]]
  void transferTokens(
    address src,    ///< Source address
    address dst,    ///< Destination address
    uint128 tokens, ///< Amount of tokens
    uint128 evers   ///< Amount of processing evers
  ) = 20;

  /// Request wrapper registration from Flex Root
  [[external, noaccept]]
  void registerWrapper(
    uint256    wrapper_pubkey, ///< Wrapper's public key
    uint128    value,          ///< Processing evers
    Tip3Config tip3cfg         ///< Tip3 token configuration
  ) = 21;

  /// Request wrapper registration from Flex Root
  [[external, noaccept]]
  void registerWrapperEver(
    uint256    wrapper_pubkey, ///< Wrapper's public key
    uint128    value           ///< Processing evers
  ) = 211;

  /// Request xchg pair registration from Flex Root
  [[external, noaccept]]
  void registerXchgPair(
    uint256 request_pubkey,       ///< Request public key (for identification)
    uint128 value,                ///< Processing evers
    address tip3_major_root,      ///< Address of major tip3 token root
    address tip3_minor_root,      ///< Address of minor tip3 token root
    Tip3Config major_tip3cfg,     ///< Major tip3 configuration
    Tip3Config minor_tip3cfg,     ///< Minor tip3 configuration
    uint128 min_amount,           ///< Minimum amount of major tokens to buy or sell
    uint128 minmove,              ///< Minimum move for price
    uint128 price_denum,          ///< Price denominator for the pair
    address notify_addr           ///< Notification address (AMM)
  ) = 22;

  /// Deploy an empty flex tip3 token wallet, owned by FlexClient contract
  [[external, noaccept]]
  address deployEmptyFlexWallet(
    uint256               pubkey,          ///< Public key (for identification only)
    uint128               evers_to_wallet, ///< Evers to the wallet
    Tip3Config            tip3cfg,         ///< Tip3 token configuration
    opt<client_lend_info> lend_info        ///< Info for lendOwnershipPubkey call if specified.
  ) = 23;

  /// Deploy UserIdIndex contract
  [[external, noaccept]]
  void deployIndex(
    uint256 user_id,          ///< User id
    uint256 lend_pubkey,      ///< Lend public key
    string  name,             ///< User name (encoded)
    uint128 evers_all,        ///< Evers for all processing: UserIdIndex deploy and included AuthIndex deploy
    uint128 evers_to_auth_idx ///< Evers to AuthIndex deploy
  ) = 24;

  /// Change lend_pubkey into UserIdIndex contract(call UserIdIndex.reLendPubkey). Set new_lend_pubkey for every wallet from wallets.
  [[external, noaccept]]
  void reLendIndex(
    uint256             user_id,                ///< User id
    uint256             new_lend_pubkey,        ///< New lend public key
    dict_array<address> wallets,                ///< Array of wallet addresses
    uint128             evers_relend_call,      ///< Evers for `UserIdIndex->reLendPubkey` call
    uint128             evers_each_wallet_call, ///< Evers for each `FlexWallet->lendOwnershipPubkey` call
    uint128             evers_to_remove,        ///< Evers to send in `AuthIndex->remove` call inside `UserIdIndex->reLendPubkey`
    uint128             evers_to_auth_idx       ///< Evers to send in new AuthIndex deploy call inside `UserIdIndex->reLendPubkey`
  ) = 25;

  /// Remove UserIdIndex contract
  [[external, noaccept]]
  void destroyIndex(
    uint256 user_id, ///< User id
    uint128 evers    ///< Evers to send to `UserIdIndex->remove`, inside - to `AuthIndex->remove`. The remaining will return.
  ) = 26;

  /// To convert flex tip3 tokens back to external tip3 tokens
  [[external, noaccept]]
  void burnWallet(
    uint128     evers_value,    ///< Processing evers
    uint256     out_pubkey,     ///< Public key for external wallet (out)
    address_opt out_owner,      ///< Internal (contract) owner for external wallet (out)
    address     my_tip3_addr    ///< Address of flex tip3 token wallet to burn (convert to external tokens)
  ) = 27;

  [[external, noaccept]]
  void setTradeRestriction(
    uint128 evers,                   ///< Processing evers
    address my_tip3_addr,            ///< Address of flex tip3 token wallet
    address flex,                    ///< Flex root address
    uint256 unsalted_price_code_hash ///< PriceXchg unsalted code hash
  ) = 28;

  /// Implementation of ITONTokenWalletNotify::onTip3Transfer.
  /// Notification from tip3 wallet to its owner contract about received tokens transfer.
  [[internal, noaccept, answer_id]]
  void onTip3Transfer(
    uint128     balance,       ///< New balance of the wallet.
    uint128     new_tokens,    ///< Amount of tokens received in transfer.
    uint256     sender_pubkey, ///< Sender wallet pubkey.
    address_opt sender_owner,  ///< Sender wallet internal owner.
    cell        payload,       ///< Payload (must be FlexTransferPayloadArgs).
    address     answer_addr    ///< Answer address (to receive answer and the remaining processing evers).
  ) = 202;

  /// Get owner's public key
  [[getter]]
  uint256 getOwner() = 29;

  /// Get Flex root address
  [[getter]]
  address getFlex() = 30;

  /// Is flex token wallet code initialized
  [[getter]]
  bool hasFlexWalletCode() = 31;

  /// Is AuthIndex code initialized
  [[getter]]
  bool hasAuthIndexCode() = 32;

  /// Is UserIdIndex code initialized
  [[getter]]
  bool hasUserIdIndexCode() = 33;

  /// Prepare payload for transferWithNotify call from external wallet to wrapper's wallet
  ///  to deploy flex internal wallet
  [[getter]]
  cell getPayloadForDeployInternalWallet(
    uint256     owner_pubkey, ///< Owner's public key
    address_opt owner_addr,   ///< Owner's internal address (contract)
    uint128     evers         ///< Processing evers
  ) = 34;

  /// Get PriceXchg address
  [[getter]]
  address getPriceXchgAddress(
    uint128 price_num,        ///< Price numerator for rational price value
    cell    salted_price_code ///< Code of PriceXchg contract (salted!).
  ) = 35;

  /// Return UserIdIndex address.
  [[getter]]
  address getUserIdIndex(
    uint256 user_id ///< User id
  ) = 36;
};
using IFlexClientPtr = handle<IFlexClient>;

/// FlexClient persistent data struct
struct DFlexClient {
  uint256           owner_;              ///< Owner's public key
  int8              workchain_id_;       ///< Workchain id
  address           flex_;               ///< Address of Flex root
  optcell           flex_wallet_code_;   ///< Flex token wallet code
  optcell           auth_index_code_;    ///< AuthIndex code
  optcell           user_id_index_code_; ///< UserIdIndex code
};

/// \interface EFlexClient
/// \brief FlexClient events interface
__interface EFlexClient {
};

} // namespace tvm

