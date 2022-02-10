#pragma once

/** \file
 *  \brief RootTokenContract contract interfaces and data-structs.
 *  Compiles into two contract versions: RootTokenContract.tvc (for external wallets) and FlexTokenRoot.tvc (for internal wallets).
 *  With different macroses.
 *  Also, Wrapper contract may be internal wallets root and perform conversion external->internal and back.
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#include "TONTokenWallet.hpp"

namespace tvm {

static constexpr unsigned ROOT_TIMESTAMP_DELAY = 1800;
using root_replay_protection_t = replay_attack_protection::timestamp<ROOT_TIMESTAMP_DELAY>;

// ===== Root Token Contract ===== //
/** \interface IRootTokenContract
 *  \brief Tip3 token root contract interface.
 *  RootTokenContract is a root contract for tip3 token.
 */
__interface IRootTokenContract {

  /// Constructor of RootTokenContract.
  /// Requires next additional initialization call IRootTokenContract::setWalletCode().
  [[internal, external]]
  void constructor(
    string      name,        ///< Token name.
    string      symbol,      ///< Token short symbol.
    uint8       decimals,    ///< Decimals for ui purposes. ex: balance 100 with decimals 2 will be printed as 1.00.
    uint256     root_pubkey, ///< Root public key
    address_opt root_owner,  ///< Owner contract address for internal ownership.
    uint128     total_supply ///< Total supply. Originally allocated tokens.
                             ///< Tokens may be also allocated later using IRootTokenContract::mint() call.
                             ///< If you want to have fixed tokens pool, you need to implement such an owner contract,
                             ///<  that will never call IRootTokenContract::mint().
  ) = 10;

  /// Set wallet code.
  /// Additional initialization method required to fit external message size limits (16k).
  /// Contract will check code hash to be TIP3_WALLET_CODE_HASH, defined at compilation.
  [[internal, external, noaccept, answer_id]]
  bool setWalletCode(cell wallet_code) = 11;

  /// Deploy a new token wallet and grant some \p tokens.
  /// Maybe only called by Root owner.
  /// Should be provided \p pubkey (for external owned wallet) or \p owner (for internal owned wallet).
  [[internal, external, noaccept, answer_id]]
  address deployWallet(
    uint256     pubkey,  ///< New wallet's public key.
    address_opt owner,   ///< New wallet's internal owner address.
    uint128     tokens,  ///< Amount of tokens to be granted to the wallet.
    uint128     evers    ///< Evers to be sent to the deployable wallet.
  ) = 12;

  /// Deploy a new empty token wallet.
  /// Anyone may request to deploy an empty wallet (using internal message with attached evers).
  [[internal, noaccept, answer_id]]
  address deployEmptyWallet(
    uint256     pubkey,  ///< New wallet's public key.
    address_opt owner,   ///< New wallet's internal owner address.
    uint128     evers    ///< Evers to be sent to the deployable wallet.
                         ///< Must be less that attached (to the call) evers value.
  ) = 13;

  /// Grant tokens to \p dest token wallet.
  /// Tokens must be first allocated using IRootTokenContract::mint() call.
  /// Maybe only called by Root owner.
  [[internal, external, noaccept, answer_id]]
  void grant(
    address dest,    ///< Token wallet address
    uint128 tokens,  ///< Amount of tokens to be granted.
    uint128 evers    ///< Amount of evers to be attached to message.
                     ///< If the method called by internal message, will be ignored
                     ///<  (rest attached evers will be transmitted).
  ) = 14;

  /// Mint tokens. Allocates new tokens (increases total_supply_).
  [[internal, external, noaccept, answer_id]]
  bool mint(uint128 tokens) = 15;

  /// Request total granted value to be called from other contracts.
  [[internal, noaccept, answer_id]]
  uint128 requestTotalGranted() = 16;

  /// Get token name
  [[getter]]
  string getName() = 17;

  /// Get token symbol
  [[getter]]
  string getSymbol() = 18;

  /// Get token decimals
  [[getter]]
  uint8 getDecimals() = 19;

  /// Get public key of the Root
  [[getter]]
  uint256 getRootKey() = 20;

  /// Get total supply (allocated tokens)
  [[getter]]
  uint128 getTotalSupply() = 21;

  /// Get total granted tokens
  [[getter]]
  uint128 getTotalGranted() = 22;

  /// Is wallet code already initialized (by IRootTokenContract::setWalletCode())
  [[getter]]
  bool hasWalletCode() = 23;

  /// Get wallet code
  [[getter]]
  cell getWalletCode() = 24;

  /// Calculate wallet address using (pubkey, owner) pair.
  [[getter]]
  address getWalletAddress(
    uint256     pubkey, ///< Public key of the wallet.
    address_opt owner   ///< Internal owner address of the wallet.
  ) = 25;

  /// Get wallet code hash.
  [[getter]]
  uint256 getWalletCodeHash() = 26;
};
using IRootTokenContractPtr = handle<IRootTokenContract>;

/// RootTokenContract persistent data struct
struct DRootTokenContract {
  string      name_;          ///< Token name.
  string      symbol_;        ///< Token short symbol.
  uint8       decimals_;      ///< Decimals for ui purposes. ex: balance 100 with decimals 2 will be printed as 1.00.
  uint256     root_pubkey_;   ///< Root public key.
  address_opt root_owner_;    ///< Root internal owner (owner contract).
  uint128     total_supply_;  ///< Total supply of allocated tokens (in the pool).
  uint128     total_granted_; ///< Total granted tokens (to the wallets).
  optcell     wallet_code_;   ///< Token wallet code.
};

/// \interface ERootTokenContract
/// \brief RootTokenContract events interface
struct ERootTokenContract {
};

/// Prepare Root StateInit structure and expected contract address (hash from StateInit)
inline
std::pair<StateInit, uint256> prepare_root_state_init_and_addr(cell root_code, DRootTokenContract root_data) {
  cell root_data_cl =
    prepare_persistent_data<IRootTokenContract, root_replay_protection_t>(
      root_replay_protection_t::init(), root_data);
  StateInit root_init {
    /*split_depth*/{}, /*special*/{},
    root_code, root_data_cl, /*library*/{}
  };
  cell root_init_cl = build(root_init).make_cell();
  return { root_init, uint256(tvm_hash(root_init_cl)) };
}

} // namespace tvm

