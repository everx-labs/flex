#include "XchgPair.hpp"
#include <tvm/contract.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/default_support_functions.hpp>

using namespace tvm;
using namespace schema;

class XchgPair final : public smart_interface<IXchgPair>, public DXchgPair {
public:
  struct error_code : tvm::error_code {
    static constexpr unsigned not_enough_tons = 101;
    static constexpr unsigned double_deploy   = 102;
    static constexpr unsigned zero_min_amount = 103;
  };

  __always_inline
  bool_t onDeploy(uint128 min_amount, uint128 deploy_value, address notify_addr) {
    require(int_value().get() > deploy_value, error_code::not_enough_tons);
    require(!min_amount_, error_code::double_deploy);
    require(min_amount > 0, error_code::zero_min_amount);

    min_amount_ = min_amount;
    notify_addr_ = notify_addr;
    tvm_rawreserve(deploy_value.get(), rawreserve_flag::up_to);
    set_int_return_flag(SEND_ALL_GAS);
    return bool_t{true};
  }

  __always_inline
  address getFlexAddr() {
    return flex_addr_;
  }

  __always_inline
  address getTip3MajorRoot() {
    return tip3_major_root_;
  }

  __always_inline
  address getTip3MinorRoot() {
    return tip3_minor_root_;
  }

  __always_inline
  uint128 getMinAmount() {
    return min_amount_;
  }

  __always_inline
  address getNotifyAddr() {
    return notify_addr_;
  }

  // =============== Support functions ==================
  DEFAULT_SUPPORT_FUNCTIONS(IXchgPair, void)

  // default processing of unknown messages
  __always_inline static int _fallback(cell /*msg*/, slice /*msg_body*/) {
    return 0;
  }
};

DEFINE_JSON_ABI(IXchgPair, DXchgPair, EXchgPair);

// ----------------------------- Main entry functions ---------------------- //
MAIN_ENTRY_FUNCTIONS_NO_REPLAY(XchgPair, IXchgPair, DXchgPair)
