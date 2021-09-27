#include "Flex.hpp"
#include "TradingPair.hpp"
#include "XchgPair.hpp"
#include <tvm/contract.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/default_support_functions.hpp>

using namespace tvm;
using namespace schema;

template<bool Internal>
class Flex final : public smart_interface<IFlex>, public DFlex {
public:
  struct error_code : tvm::error_code {
    static constexpr unsigned sender_is_not_deployer        = 100;
    static constexpr unsigned unexpected_refs_count_in_code = 101;
    static constexpr unsigned cant_override_code            = 102;
    static constexpr unsigned sender_is_not_my_owner        = 103;
  };

  __always_inline
  void constructor(
      uint256 deployer_pubkey, string ownership_description, std::optional<address> owner_address,
      uint128 transfer_tip3, uint128 return_ownership, uint128 trading_pair_deploy,
      uint128 order_answer, uint128 process_queue, uint128 send_notify,
      uint8 deals_limit) {
    deployer_pubkey_ = deployer_pubkey;
    ownership_description_ = ownership_description;
    owner_address_ = owner_address;
    deals_limit_ = deals_limit;
    tons_cfg_ = {
      transfer_tip3, return_ownership, trading_pair_deploy, order_answer, process_queue, send_notify
    };
  }

  __always_inline
  void setPairCode(cell code) {
    require(!pair_code_, error_code::cant_override_code);
    require(msg_pubkey() == deployer_pubkey_, error_code::sender_is_not_deployer);
    tvm_accept();

    // adding flex address to ref#2 in this code cell
    require(code.ctos().srefs() == 2, error_code::unexpected_refs_count_in_code);
    pair_code_ = builder().stslice(code.ctos()).stref(build(address{tvm_myaddr()}).endc()).endc();
  }

  __always_inline
  void setXchgPairCode(cell code) {
    require(!xchg_pair_code_, error_code::cant_override_code);
    require(msg_pubkey() == deployer_pubkey_, error_code::sender_is_not_deployer);
    tvm_accept();

    // adding flex address to ref#2 in this code cell
    require(code.ctos().srefs() == 2, error_code::unexpected_refs_count_in_code);
    xchg_pair_code_ = builder().stslice(code.ctos()).stref(build(address{tvm_myaddr()}).endc()).endc();
  }

  __always_inline
  void setPriceCode(cell code) {
    require(!price_code_, error_code::cant_override_code);
    require(msg_pubkey() == deployer_pubkey_, error_code::sender_is_not_deployer);
    tvm_accept();

    price_code_ = code;
  }

  __always_inline
  void setXchgPriceCode(cell code) {
    require(!xchg_price_code_, error_code::cant_override_code);
    require(msg_pubkey() == deployer_pubkey_, error_code::sender_is_not_deployer);
    tvm_accept();

    xchg_price_code_ = code;
  }

  __always_inline
  void transfer(address to, uint128 tons) {
    bool internal_ownership = !!owner_address_;
    if constexpr (Internal)
      require(internal_ownership && int_sender() == *owner_address_, error_code::sender_is_not_my_owner);
    else
      require(!internal_ownership && msg_pubkey() == deployer_pubkey_, error_code::sender_is_not_my_owner);

    tvm_accept();
    tvm_transfer(to, tons.get(), true);
  }

  __always_inline
  bool_t isFullyInitialized() {
    return bool_t(pair_code_ && price_code_ && xchg_pair_code_ && xchg_price_code_);
  }

  __always_inline
  TonsConfig getTonsCfg() {
    return tons_cfg_;
  }

  __always_inline
  cell getTradingPairCode() {
    return *pair_code_;
  }

  __always_inline
  cell getXchgPairCode() {
    return *xchg_pair_code_;
  }

  __always_inline
  cell getSellPriceCode(address tip3_addr) {
    // adding tip3 root address to ref#3 in this code cell
    require(price_code_->ctos().srefs() == 2, error_code::unexpected_refs_count_in_code);
    cell salt = builder().stslice(tvm_myaddr()).stslice(tip3_addr.sl()).endc();
    return builder().stslice(price_code_->ctos()).stref(salt).endc();
  }

  __always_inline
  cell getXchgPriceCode(address tip3_addr1, address tip3_addr2) {
    require(price_code_->ctos().srefs() == 2, error_code::unexpected_refs_count_in_code);
    auto keys = std::make_tuple(tip3_addr1, tip3_addr2);
    return builder().stslice(xchg_price_code_->ctos()).stref(build(keys).endc()).endc();
  }

  __always_inline
  address getSellTradingPair(address tip3_root) {
    address myaddr{tvm_myaddr()};
    DTradingPair pair_data {
      .flex_addr_ = myaddr,
      .tip3_root_ = tip3_root,
      .min_amount_ = uint128(0),
      .notify_addr_ = address::make_std(int8(0), uint256(0))
    };
    auto std_addr = prepare_trading_pair_state_init_and_addr(pair_data, *pair_code_).second;
    auto workchain_id = std::get<addr_std>(myaddr.val()).workchain_id;
    return address::make_std(workchain_id, std_addr);
  }

  __always_inline
  address getXchgTradingPair(address tip3_major_root, address tip3_minor_root) {
    address myaddr{tvm_myaddr()};
    DXchgPair pair_data {
      .flex_addr_ = myaddr,
      .tip3_major_root_ = tip3_major_root,
      .tip3_minor_root_ = tip3_minor_root,
      .min_amount_ = uint128(0),
      .notify_addr_ = address::make_std(int8(0), uint256(0))
    };
    auto std_addr = prepare_xchg_pair_state_init_and_addr(pair_data, *xchg_pair_code_).second;
    auto workchain_id = std::get<addr_std>(myaddr.val()).workchain_id;
    return address::make_std(workchain_id, std_addr);
  }

  __always_inline
  uint8 getDealsLimit() {
    return deals_limit_;
  }

  __always_inline
  FlexOwnershipInfo getOwnershipInfo() {
    return { deployer_pubkey_, ownership_description_, owner_address_ };
  }

  // =============== Support functions ==================
  DEFAULT_SUPPORT_FUNCTIONS(IFlex, flex_replay_protection_t)

  // default processing of unknown messages
  __always_inline static int _fallback(cell /*msg*/, slice /*msg_body*/) {
    return 0;
  }
};

DEFINE_JSON_ABI(IFlex, DFlex, EFlex);

// ----------------------------- Main entry functions ---------------------- //
DEFAULT_MAIN_ENTRY_FUNCTIONS_TMPL(Flex, IFlex, DFlex, FLEX_TIMESTAMP_DELAY)

