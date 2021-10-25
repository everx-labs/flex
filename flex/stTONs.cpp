#include "stTONs.hpp"
#include <tvm/contract.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/default_support_functions.hpp>

#include "DePool.hpp"

using namespace tvm;
using namespace schema;

template<bool Internal>
class stTONs final : public smart_interface<IstTONs>, public DstTONs {
public:
  struct error_code : tvm::error_code {
    static constexpr unsigned sender_is_not_my_owner        = 100;
    static constexpr unsigned crystalls_inconsistency       = 101;
    static constexpr unsigned not_enough_crystalls          = 102;
    static constexpr unsigned store_crystalls_first         = 103;
    static constexpr unsigned small_tokens_value            = 104;
    static constexpr unsigned not_good_wallet               = 105;
    static constexpr unsigned too_big_amount_to_return_back = 106;
  };

  __always_inline
  void constructor(
    uint256 owner_pubkey,
    Tip3Config tip3cfg,
    address depool,
    stTONsCosts costs,
    cell tip3code
  ) {
    require(costs.receive_stake_transfer_costs >
              costs.store_crystals_costs + costs.mint_costs + costs.deploy_wallet_costs,
            error_code::crystalls_inconsistency);

    owner_pubkey_ = owner_pubkey;
    tip3cfg_ = tip3cfg;
    depool_ = depool;
    costs_ = costs;
    tip3code_ = tip3code;
    workchain_id_ = std::get<addr_std>(address{tvm_myaddr()}.val()).workchain_id;
  }

  __always_inline
  IRootTokenContractPtr tip3root() const { return IRootTokenContractPtr(tip3cfg_.root_address); }

  __always_inline
  void storeCrystalls(address client_addr) {
    require(int_value().get() > costs_.receive_stake_transfer_costs, error_code::not_enough_crystalls);

    auto std_addr = std::get<addr_std>(client_addr.val()).address;
    if (auto opt_exist = stored_crystals_.lookup(std_addr.get()))
      stored_crystals_.set_at(std_addr.get(), *opt_exist + int_value().get() - costs_.store_crystals_costs);
    else
      stored_crystals_.set_at(std_addr.get(), int_value().get() - costs_.store_crystals_costs);
  }

  __always_inline
  void receiveStakeTransfer(address source, uint128 amount) {
    // transferStake has uint64 amount argument, so we will not be able to return back stakes > (1 << 64)
    require((amount >> 64) == 0, error_code::too_big_amount_to_return_back);
    auto std_addr = std::get<addr_std>(source.val()).address;
    auto opt_account = stored_crystals_.extract(std_addr.get());
    require(!!opt_account, error_code::store_crystalls_first);
    tvm_accept();

    tip3root()(Grams(costs_.mint_costs.get())).mint(amount);

    auto rest_crystals = *opt_account - costs_.process_receive_stake_costs - costs_.mint_costs;
    tip3root()(Grams(rest_crystals.get())).
      deployWallet(uint256(0), source, amount, rest_crystals - costs_.deploy_wallet_costs);
  }

  __always_inline
  void internalTransfer(
    uint128 tokens,
    address answer_addr,
    uint256 sender_pubkey,
    address sender_owner,
    bool_t  notify_receiver,
    cell    payload
  ) {
    require(tokens >= costs_.min_transfer_tokens, error_code::small_tokens_value);

    uint256 expected_address = expected_sender_address(sender_pubkey, sender_owner);
    auto [sender, value_gr] = int_sender_and_value();
    require(std::get<addr_std>(sender()).address == expected_address,
            error_code::not_good_wallet);

    depool_(Grams(costs_.transfer_stake_costs.get())).transferStake(sender_owner, uint64(tokens.get()));
  }

  __always_inline
  stTONsDetails getDetails() {
    return {
      owner_pubkey_,
      owner_address_,
      tip3root().get(),
      depool_.get(),
      getStoredCrystals(),
      costs_
      };
  }

  __always_inline
  dict_array<StoredCrystalsPair> getStoredCrystals() const {
    dict_array<StoredCrystalsPair> rv;
    for (auto v : stored_crystals_) {
      rv.push_back({v.first, v.second});
    }
    return rv;
  }

  __always_inline
  void check_owner() {
    bool internal_ownership = !!owner_address_;
    if constexpr (Internal)
      require(internal_ownership && (int_sender() == *owner_address_), error_code::sender_is_not_my_owner);
    else
      require(!internal_ownership && (msg_pubkey() == owner_pubkey_), error_code::sender_is_not_my_owner);
  }

  // transform x:0000...0000 address into empty optional<address>
  __always_inline
  std::optional<address> optional_owner(address owner) {
    return std::get<addr_std>(owner()).address ?
      std::optional<address>(owner) : std::optional<address>();
  }

  __always_inline
  std::pair<StateInit, uint256> calc_wallet_init_hash(uint256 pubkey, address internal_owner) {
    return prepare_internal_wallet_state_init_and_addr(
      tip3cfg_.name, tip3cfg_.symbol, tip3cfg_.decimals, tip3cfg_.root_public_key, pubkey, tip3cfg_.root_address,
      optional_owner(internal_owner), tip3code_, workchain_id_);
  }

  __always_inline
  uint256 expected_sender_address(uint256 sender_public_key, address sender_owner) {
    return calc_wallet_init_hash(sender_public_key, sender_owner).second;
  }

  // =============== Support functions ==================
  DEFAULT_SUPPORT_FUNCTIONS(IstTONs, sttons_replay_protection_t)

  // default processing of unknown messages
  __always_inline static int _fallback(cell /*msg*/, slice /*msg_body*/) {
    return 0;
  }
};

DEFINE_JSON_ABI(IstTONs, DstTONs, EstTONs);

// ----------------------------- Main entry functions ---------------------- //
DEFAULT_MAIN_ENTRY_FUNCTIONS_TMPL(stTONs, IstTONs, DstTONs, STTONS_TIMESTAMP_DELAY)

