/** \file
 *  \brief stTONs contract interfaces and data-structs
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#pragma once

#include <tvm/schema/message.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/replay_attack_protection/timestamp.hpp>

#include "DePool.hpp"

namespace tvm {

static constexpr unsigned STTONS_TIMESTAMP_DELAY = 1800;
using sttons_replay_protection_t = replay_attack_protection::timestamp<STTONS_TIMESTAMP_DELAY>;

/// Lend ownership record (for usage in `address->lend_record` map)
struct lend_record {
  uint128 lend_balance;    ///< Lend ownership balance
  uint32 lend_finish_time; ///< Lend ownership finish time
};
/// Lend ownership map
using lend_ownership_map = small_dict_map<addr_std_fixed, lend_record>;

/// Lend ownership array record (for usage in getter)
struct lend_array_record {
  address lend_addr;       ///< Lend ownership destination address
  uint128 lend_balance;    ///< Lend ownership balance
  uint32 lend_finish_time; ///< Lend ownership finish time
};
/// Lend ownership array
using lend_ownership_array = dict_array<lend_array_record>;

/** \interface IstTONsNotify
 *  \brief Notification about lend ownership to destination contracts.
 *
 *  Must be implemented in contracts receiving lend ownership.
 */
__interface IstTONsNotify {
  /** \fn onLendOwnership
   * \brief Lend ownership notification.
   *
   * Will be called by IstTONs::lendOwnership() as a notification about lent token balance.
   */
  [[internal, noaccept, answer_id]]
  bool onLendOwnership(
    uint128 balance,          ///< Lend ownership balance
    uint32  lend_finish_time, ///< Lend ownership finish time
    uint256 pubkey,           ///< stTONs contract owner's public key
    std::optional<address> internal_owner, ///< stTONs contract owner contract's address
    address depool,           ///< Address of DePool
    uint256 depool_pubkey,    ///< DePool's public key
    cell    payload,          ///< Payload (arbitrary cell)
    address answer_addr       ///< Answer address
  ) = 201;
};
using IstTONsNotifyPtr = handle<IstTONsNotify>;

/// stTONs contract details (for getDetails getter)
struct stTONsDetails {
  uint256 owner_pubkey;  ///< Owner's public key
  address owner_address; ///< Owner contract's address (will be 0:00..0 if the contract has external ownership)
  address depool;        ///< Address of DePool contract
  uint256 depool_pubkey; ///< DePool's public key
  uint128 balance;       ///< Full token balance of the contract
  lend_ownership_array lend_ownership; ///< All lend ownership records of the contract
  uint128 lend_balance; ///< Summarized lend balance to all targets (actual active balance will be `balance - lend_balance`)
  bool    transferring_stake_back; ///< If the contract is in "transferring stake back" state
  uint8 last_depool_error; ///< Last error sent by DePool
};

/** \interface IstTONs
 *  \brief stTONs contract interface
 */
__interface IstTONs {

  /// Empty noop function for deploying
  [[external, internal, noaccept]]
  void onDeploy() = 10;

  /// Lend ownership to some contract for \p lend_balance until \p lend_finish_time
  [[external, internal, noaccept, answer_id]]
  void lendOwnership(
    address answer_addr,      ///< Answer address
    uint128 evers,            ///< Evers to send with the message (ignored for internal-ownership requests)
    address dest,             ///< Lend ownership destination address
    uint128 lend_balance,     ///< Lend ownership balance
    uint32  lend_finish_time, ///< Lend ownership finish time
    cell    deploy_init_cl,   ///< StateInit cell in case if IstTONsNotify::onLendOwnership() notification should be deploy message
                              ///<  (empty cell otherwise)
    cell    payload           ///< Payload to be transmitted in IstTONsNotify::onLendOwnership() notification
  ) = 11;

  /// Return ownership back to the original owner (for the provided amount of tokens)
  [[internal, noaccept]]
  void returnOwnership(uint128 tokens) = 12;

  /** Return stake to the provided \p dst.
   * Only works when contract is not in "lend mode".
   * Will return all stake (calling IDePool::transferStake(\p dst, 0_u64))
   * \param dst Destination contract address
   * \param processing_evers Value will be attached to the message
   */
  [[external, internal, noaccept]]
  void returnStake(address dst, uint128 processing_evers) = 13;

  /** Eliminate contract and return all the remaining evers to \p dst
   * \param ignore_errors Ignore error returned by depool for transferStake.
   * `finalize` will not work if the contract is in "lend mode" or `last_depool_error_ != 0 && !ignore_errors`.
   * \warning Do not use `ignore_errors=true`, unless you are really sure that
   *   your stake in DePool is empty or insignificant.
   * > `STATUS_DEPOOL_CLOSED` / `STATUS_NO_PARTICIPANT` are not considered as an errors forbidding `finalize`
   */
  [[external, internal, noaccept]]
  void finalize(address dst, bool ignore_errors) = 14;

  /// Receive stake transfer notify (from solidity IParticipant::onTransfer(address source, uint128 amount))
  [[internal, noaccept]]
  void receiveStakeTransfer(address source, uint128 amount) = 0x23c4771d; // IParticipant::onTransfer

  /// If an error occured while transferring stake back
  [[internal, noaccept]]
  void receiveAnswer(uint32 errcode, uint64 comment) = 0x3f109e44; // IParticipant::receiveAnswer

  // ========== getters ==========

  /// Returns contract status details
  [[getter]]
  stTONsDetails getDetails() = 15;

  /// Calculate stTONs contract address using configuration parameters
  [[getter, no_persistent]]
  address calcStTONsAddr(
    cell code,
    uint256 owner_pubkey,
    std::optional<address> owner_address,
    address depool,
    uint256 depool_pubkey
  ) = 16;
};
using IstTONsPtr = handle<IstTONs>;

/// stTONs persistent data struct
struct DstTONs {
  uint256 owner_pubkey_;                 ///< Owner's public key
  std::optional<address> owner_address_; ///< Owner contract's address
  IDePoolPtr depool_;                    ///< Address of DePool contract
  uint256 depool_pubkey_;                ///< DePool's public key
  uint128 balance_;                      ///< Full token balance of the contract
  lend_ownership_map lend_ownership_;    ///< All lend ownership records of the contract
  bool transferring_stake_back_;         ///< If the contract is in "transferring stake back" state
  uint8 last_depool_error_;              ///< Last error sent by DePool
};

/// \interface EstTONs
/// \brief stTONs events interface
__interface EstTONs {
};

/// Prepare stTONs StateInit structure and expected contract address (hash from StateInit)
inline
std::pair<StateInit, uint256> prepare_sttons_state_init_and_addr(DstTONs data, cell code) {
  cell data_cl =
    prepare_persistent_data<IstTONs, sttons_replay_protection_t, DstTONs>(
      sttons_replay_protection_t::init(), data
    );
  StateInit sttons_init {
    /*split_depth*/{}, /*special*/{},
    code, data_cl, /*library*/{}
  };
  cell init_cl = build(sttons_init).make_cell();
  return { sttons_init, uint256(tvm_hash(init_cl)) };
}

} // namespace tvm

