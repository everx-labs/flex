#pragma once

#include <tvm/schema/message.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/replay_attack_protection/timestamp.hpp>

#include "PriceCommon.hpp"
#include "RootTokenContract.hpp"
#include "DePool.hpp"

namespace tvm { inline namespace schema {

static constexpr unsigned STTONS_TIMESTAMP_DELAY = 1800;
using sttons_replay_protection_t = replay_attack_protection::timestamp<STTONS_TIMESTAMP_DELAY>;

struct stTONsCosts {
  uint128 receive_stake_transfer_costs; // full costs of receiveStakeTransfer processing
  uint128 store_crystals_costs; // costs of storeCrystalls processing
  uint128 mint_costs; // costs of `mint` call
  uint128 process_receive_stake_costs; // costs of receiveStakeTransfer function processing itself
  uint128 deploy_wallet_costs; // costs of deployWallet (without crystals stored in the wallet)
  uint128 min_transfer_tokens;
  uint128 transfer_stake_costs;
};

struct StoredCrystalsPair {
  uint256 std_addr;
  uint128 account;
};

struct stTONsDetails {
  uint256 owner_pubkey;
  std::optional<address> owner_address;
  address tip3root;
  address depool;
  dict_array<StoredCrystalsPair> accounts;
  stTONsCosts costs;
};

__interface IstTONs {

  [[external]]
  void constructor(
    uint256 owner_pubkey,
    Tip3Config tip3cfg,
    address depool,
    stTONsCosts costs,
    cell tip3code
  ) = 10;

  // store crystalls in account for sender (to be used in next receiveStakeTransfer call)
  [[internal, noaccept]]
  void storeCrystalls(address client_addr) = 11;

  // Receive stake transfer notify (from solidity IParticipant::onTransfer(address source, uint128 amount))
  [[internal, noaccept]]
  void receiveStakeTransfer(address source, uint128 amount) = 0x23c4771d; // = hash_v<"onTransfer(address,uint128)()v2">

  // Receive tokens from other wallet
  [[internal, noaccept, answer_id]]
  void internalTransfer(
    uint128 tokens,
    address answer_addr,
    uint256 sender_pubkey,
    address sender_owner,
    bool_t  notify_receiver,
    cell    payload
  ) = 16; // value must be the same as in TONTokenWallet::internalTransfer

  // ========== getters ==========
  [[getter]]
  stTONsDetails getDetails() = 17;
};
using IstTONsPtr = handle<IstTONs>;

struct DstTONs {
  uint256 owner_pubkey_;
  std::optional<address> owner_address_;
  Tip3Config tip3cfg_;
  IDePoolPtr depool_;
  dict_map<uint256, uint128> stored_crystals_;
  stTONsCosts costs_;
  int8 workchain_id_;
  cell tip3code_;
};

__interface EstTONs {
};

// Prepare stTONs StateInit structure and expected contract address (hash from StateInit)
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

}} // namespace tvm::schema

