#pragma once

#include <tvm/schema/message.hpp>
#include <tvm/sequence.hpp>

#include <tvm/replay_attack_protection/timestamp.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>

namespace tvm { namespace schema {

using WalletGramsType = uint128;
using TokensType = uint128;

static constexpr unsigned TOKEN_WALLET_TIMESTAMP_DELAY = 1800;
using wallet_replay_protection_t = replay_attack_protection::timestamp<TOKEN_WALLET_TIMESTAMP_DELAY>;

struct allowance_info {
  address spender;
  TokensType remainingTokens;
};

struct lend_ownership_info {
  address owner;
  TokensType lend_balance;
  uint32 lend_finish_time;
};

struct details_info {
  bytes name;
  bytes symbol;
  uint8 decimals;
  TokensType balance;
  uint256 root_public_key;
  uint256 wallet_public_key;
  address root_address;
  address owner_address;
  lend_ownership_info lend_ownership;
  cell code;
  allowance_info allowance;
  int8 workchain_id;
};

// ===== TON Token wallet notification callback interface ===== //
__interface ITONTokenWalletNotify {
  [[internal, noaccept, answer_id]]
  bool_t onTip3LendOwnership(uint128 balance, uint32 lend_finish_time,
    uint256 pubkey, uint256 internal_owner, cell payload, address answer_addr) = 201;
};
using ITONTokenWalletNotifyPtr = handle<ITONTokenWalletNotify>;

// ===== TON Token wallet ===== //
__interface ITONTokenWallet {

  [[internal, noaccept]]
  void transfer(address dest, TokensType tokens, bool_t return_ownership, address answer_addr);

  //[[internal, noaccept]]
  //void transferToRecipient(uint256 recipient_public_key, uint256 recipient_internal_owner,
  //  uint128 tokens, bool_t deploy, address answer_addr);

  [[internal, noaccept, answer_id]]
  TokensType getBalance_InternalOwner();

  // Receive tokens from root
  [[internal, noaccept]]
  void accept(TokensType tokens);

  // Receive tokens from other wallet
  [[internal, noaccept]]
  void internalTransfer(TokensType tokens, uint256 pubkey, uint256 my_owner_addr, address answer_addr);

  // Send rest !native! funds to `dest` and destroy the wallet.
  // balance must be zero. Not allowed for lend ownership.
  [[internal, noaccept]]
  void destroy(address dest);

  // lend ownership to some contract until 'lend_finish_time'
  // allowance is cleared and is not permited to set up by temporary owner.
  [[internal, noaccept, answer_id]]
  bool_t lendOwnership(uint256 std_dest, TokensType lend_balance, uint32 lend_finish_time,
    cell deploy_init_cl, cell payload) = 64;

  // return ownership back to the original owner
  [[internal, noaccept]]
  void returnOwnership();

  // getters
  [[getter]]
  details_info getDetails();

  //[[getter]]
  //bytes getName();

  //[[getter]]
  //bytes getSymbol();

  //[[getter]]
  //uint8 getDecimals();

  //[[getter]]
  //TokensType getBalance();

  //[[getter]]
  //uint256 getWalletKey();

  //[[getter]]
  //address getRootAddress();

  //[[getter]]
  //address getOwnerAddress();

  //[[getter]]
  //lend_ownership_info getLendOwnership();

  //[[getter]]
  //allowance_info allowance();

  // allowance interface
  // [[internal, noaccept]]
  // void approve(address spender, TokensType remainingTokens, TokensType tokens);

  // [[internal, noaccept]]
  // void transferFrom(address dest, address to, TokensType tokens,
  //                   WalletGramsType grams);

  // [[internal]]
  // void internalTransferFrom(address to, TokensType tokens);

  // [[internal, noaccept]]
  // void disapprove();
};
using ITONTokenWalletPtr = handle<ITONTokenWallet>;

struct DTONTokenWallet {
  bytes name_;
  bytes symbol_;
  uint8 decimals_;
  TokensType balance_;
  uint256 root_public_key_;
  uint256 wallet_public_key_;
  address root_address_;
  std::optional<address> owner_address_;
  std::optional<lend_ownership_info> lend_ownership_;
  cell code_;
  std::optional<allowance_info> allowance_;
  int8 workchain_id_;
};

struct ETONTokenWallet {
};

// Prepare Token Wallet StateInit structure and expected contract address (hash from StateInit)
inline
std::pair<StateInit, uint256> prepare_wallet_state_init_and_addr(DTONTokenWallet wallet_data) {
  cell wallet_data_cl =
    prepare_persistent_data<ITONTokenWallet, wallet_replay_protection_t, DTONTokenWallet>(
      wallet_replay_protection_t::init(), wallet_data);
  StateInit wallet_init {
    /*split_depth*/{}, /*special*/{},
    wallet_data.code_, wallet_data_cl, /*library*/{}
  };
  cell wallet_init_cl = build(wallet_init).make_cell();
  return { wallet_init, uint256(tvm_hash(wallet_init_cl)) };
}

}} // namespace tvm::schema

