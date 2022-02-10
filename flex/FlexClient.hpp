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

/// Map user_id => lend_info
using lend_user_ids_map = small_dict_map<uint256, client_lend_info>;
using lend_user_ids_array = dict_array<std::pair<uint256, client_lend_info>>;

/// \brief FlexClient is client contract for Flex
/** FlexClient keeps ownership of flex token wallets. Keeps/spends evers balance for processing.
 *  Makes orders.
 **/
__interface IFlexClient {

  /// Constructor of the FlexClient
  [[external]]
  void constructor(
    uint256 pubkey, ///< Owner's public key
    uint256 user_id ///< User id
  ) = 10;

  /// Set flex address and evers configuration
  [[external, noaccept]]
  void setFlexCfg(
    EversConfig    ev_cfg, ///< Evers configuration of Flex
    address        flex    ///< Address of Flex root contract
  ) = 11;

  /// Set flex wallet code for deployEmptyFlexWallet calls
  [[external, noaccept]]
  void setFlexWalletCode(cell flex_wallet_code) = 12;

  /// Deploy tip3-tip3 PriceXchg contract with sell or buy order
  [[internal, noaccept]]
  address deployPriceXchg(
    bool       sell,                 ///< Is it a sell order
    bool       immediate_client,     ///< Should this order try to be executed as a client order first
                                     ///<  (find existing corresponding orders).
    bool       post_order,           ///< Should this order be enqueued if it doesn't already have corresponding orders.
    uint128    price_num,            ///< Price numerator for rational price value
    uint128    price_denum,          ///< Price denominator for rational price value
    uint128    amount,               ///< Amount of major tip3 tokens to sell or buy
    uint128    lend_amount,          ///< Lend amount. For sell, it should be amount of major tokens, for buy - minor.
    uint32     lend_finish_time,     ///< Lend finish time (order finish time also)
    uint128    evers,                ///< Processing evers
    cell       xchg_price_code,      ///< Code of PriceXchg contract
    address    my_tip3_addr,         ///< Address of flex tip3 token wallet to provide tokens
    address    receive_wallet,       ///< Address of flex tip3 token wallet to receive tokens
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

  /// Forget user id
  [[external, noaccept]]
  void forgetUserId(
    uint256      user_id       ///< User id to be removed. Wallet's revokes should be performed before this call.
  ) = 18;

  /// Cancel tip3-tip sell or buy order
  [[external, noaccept]]
  void cancelXchgOrder(
    bool       sell,                 ///< Is it a sell order
    uint128    price_num,            ///< Price numerator for rational price value
    uint128    price_denum,          ///< Price denominator for rational price value
    uint128    value,                ///< Processing evers
    cell       xchg_price_code       ///< Code of PriceXchg contract
  ) = 19;

  /// Transfer evers
  [[external, noaccept]]
  void transfer(
    address dest,  ///< Destination address
    uint128 value, ///< Amount of evers
    bool    bounce ///< Bounce flag
  ) = 20;

  /// Request wrapper registration from Flex Root
  [[external, noaccept]]
  void registerWrapper(
    uint256    wrapper_pubkey, ///< Wrapper's public key
    uint128    value,          ///< Processing evers
    Tip3Config tip3cfg         ///< Tip3 token configuration
  ) = 21;

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
    address notify_addr           ///< Notification address (AMM)
  ) = 22;

  /// Deploy an empty flex tip3 token wallet, owned by FlexClient contract
  [[external, noaccept]]
  address deployEmptyFlexWallet(
    uint256    pubkey,             ///< Public key (for identification only)
    uint128    evers_to_wallet,    ///< Evers to the wallet
    Tip3Config tip3cfg             ///< Tip3 token configuration
  ) = 23;

  /// To convert flex tip3 tokens back to external tip3 tokens
  [[external, noaccept]]
  void burnWallet(
    uint128     evers_value,    ///< Processing evers
    uint256     out_pubkey,     ///< Public key for external wallet (out)
    address_opt out_owner,      ///< Internal (contract) owner for external wallet (out)
    address     my_tip3_addr    ///< Address of flex tip3 token wallet to burn (convert to external tokens)
  ) = 24;

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
  uint256 getOwner() = 25;

  /// Get Flex root address
  [[getter]]
  address getFlex() = 26;

  /// Is flex token wallet code initialized
  [[getter]]
  bool hasFlexWalletCode() = 27;

  /// Get user id for main user
  [[getter]]
  uint256 getUserId() = 28;

  /// Get lend user ids and lend infos
  [[getter]]
  lend_user_ids_array getLendUserIds() = 29;

  /// Prepare payload for transferWithNotify call from external wallet to wrapper's wallet
  ///  to deploy flex internal wallet
  [[getter]]
  cell getPayloadForDeployInternalWallet(
    uint256     owner_pubkey, ///< Owner's public key
    address_opt owner_addr,   ///< Owner's internal address (contract)
    uint128     evers         ///< Processing evers
  ) = 30;

  /// Prepare payload for lendOwnership to PriceXchg contract
  [[getter]]
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
  ) = 31;

  /// Prepare StateInit cell and destination address for lendOwnership to PriceXchg contract
  [[getter]]
  std::pair<cell, address> getStateInitForPriceXchg(
    uint128 price_num,      ///< Price numerator for rational price value
    uint128 price_denum,    ///< Price denominator for rational price value
    cell    xchg_price_code ///< Code of PriceXchg contract
  ) = 32;
};
using IFlexClientPtr = handle<IFlexClient>;

/// FlexClient persistent data struct
struct DFlexClient {
  uint256           owner_;            ///< Owner's public key
  int8              workchain_id_;     ///< Workchain id
  EversConfig       ev_cfg_;           ///< Evers configuration
  address           flex_;             ///< Address of Flex root
  optcell           flex_wallet_code_; ///< Flex token wallet code
  uint256           user_id_;          ///< User id
  lend_user_ids_map lend_user_ids_;    ///< Lend user ids
};

/// \interface EFlexClient
/// \brief FlexClient events interface
__interface EFlexClient {
};

} // namespace tvm

