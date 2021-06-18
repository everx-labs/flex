#include "FLeX.hpp"
#include "TradingPair.hpp"
#include <tvm/contract.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/default_support_functions.hpp>

using namespace tvm;
using namespace schema;

static constexpr unsigned TIMESTAMP_DELAY = 1800;

class FLeX final : public smart_interface<IFLeX>, public DFLeX {
  using replay_protection_t = replay_attack_protection::timestamp<TIMESTAMP_DELAY>;
public:
  struct error_code : tvm::error_code {
    static constexpr unsigned sender_is_not_deployer        = 100;
    static constexpr unsigned unexpected_refs_count_in_code = 101;
    static constexpr unsigned cant_override_code            = 102;
  };

  __always_inline
  void constructor(uint256 deployer_pubkey,
      uint128 transfer_tip3, uint128 return_ownership, uint128 trading_pair_deploy,
      uint128 order_answer, uint128 process_queue, uint128 send_notify,
      uint128 min_amount, uint8 deals_limit, address notify_addr) {
    deployer_pubkey_ = deployer_pubkey;
    min_amount_ = min_amount;
    deals_limit_ = deals_limit;
    notify_addr_ = notify_addr;
    tons_cfg_ = {
      transfer_tip3, return_ownership, trading_pair_deploy, order_answer, process_queue, send_notify
    };
  }

  __always_inline
  void setPairCode(cell code) {
    require(!isFullyInitialized().get(), error_code::cant_override_code);
    require(msg_pubkey() == deployer_pubkey_, error_code::sender_is_not_deployer);
    tvm_accept();
    require(!pair_code_, error_code::cant_override_code);
    // adding flex address to ref#2 in this code cell
    require(code.ctos().srefs() == 2, error_code::unexpected_refs_count_in_code);
    pair_code_ = builder().stslice(code.ctos()).stref(build(address{tvm_myaddr()}).endc()).endc();
  }

  __always_inline
  void setPriceCode(cell code) {
    require(!isFullyInitialized().get(), error_code::cant_override_code);
    require(msg_pubkey() == deployer_pubkey_, error_code::sender_is_not_deployer);
    tvm_accept();
    require(!price_code_, error_code::cant_override_code);
    price_code_ = code;
  }

  __always_inline
  bool_t isFullyInitialized() {
    return bool_t(pair_code_ && price_code_);
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
  cell getSellPriceCode(address tip3_addr) {
    // adding tip3 root address to ref#3 in this code cell
    require(price_code_->ctos().srefs() == 2, error_code::unexpected_refs_count_in_code);
    cell salt = builder().stslice(tvm_myaddr()).stslice(tip3_addr.sl()).endc();
    return builder().stslice(price_code_->ctos()).stref(salt).endc();
  }

  __always_inline
  cell getXchgPriceCode(address tip3_addr1, address tip3_addr2) {
    require(price_code_->ctos().srefs() == 3, error_code::unexpected_refs_count_in_code);
    auto keys = std::make_tuple(tip3_addr1, tip3_addr2);
    return builder().stslice(price_code_->ctos()).stref(build(keys).endc()).endc();
  }

  __always_inline
  address getSellTradingPair(address tip3_root) {
    address myaddr{tvm_myaddr()};
    DTradingPair pair_data {
      .flex_addr_ = myaddr,
      .tip3_root_ = tip3_root,
      .deploy_value_ = tons_cfg_.trading_pair_deploy
    };
    auto std_addr = prepare_trading_pair_state_init_and_addr(pair_data, *pair_code_).second;
    auto workchain_id = std::get<addr_std>(myaddr.val()).workchain_id;
    return address::make_std(workchain_id, std_addr);
  }

  __always_inline
  uint128 getMinAmount() {
    return min_amount_;
  }

  __always_inline
  uint8 getDealsLimit() {
    return deals_limit_;
  }

  __always_inline
  address getNotifyAddr() {
    return notify_addr_;
  }

  // =============== Support functions ==================
  DEFAULT_SUPPORT_FUNCTIONS(IFLeX, replay_protection_t)

  // default processing of unknown messages
  __always_inline static int _fallback(cell /*msg*/, slice /*msg_body*/) {
    return 0;
  }
};

DEFINE_JSON_ABI(IFLeX, DFLeX, EFLeX);

// ----------------------------- Main entry functions ---------------------- //
DEFAULT_MAIN_ENTRY_FUNCTIONS(FLeX, IFLeX, DFLeX, TIMESTAMP_DELAY)

