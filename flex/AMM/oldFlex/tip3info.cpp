#include <tvm/contract.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>
#include <tvm/string.hpp>
#include <tvm/debot.hpp>
#include <tvm/default_support_functions.hpp>
#include <tvm/Console.hpp>
#include "Price.hpp"
#include "TradingPair.hpp"
#include "FLeX.hpp"
#include "TONTokenWallet.hpp"

using namespace tvm;
using namespace schema;
using namespace debot;

__interface [[no_pubkey]] ITip3InfoDebot {

  [[external]]
  void constructor() = 1;

  [[external]]
  resumable<void> start() = 2;
};

struct DTip3InfoDebot {
  std::optional<ITONTokenWalletPtr> tip3_;
};

__interface ETip3InfoDebot {
};

class Tip3InfoDebot final : public smart_interface<ITip3InfoDebot>, public DTip3InfoDebot {

  IConsolePtr cout_ptr_ = IConsolePtr(address::make_std(schema::int8{0}, schema::uint256{0}));

  __always_inline IConsolePtr::proxy cout() {
    return cout_ptr_();
  }
  __always_inline
  auto printf(string fmt) {
    cell params = builder().endc();
    return cout().printf(fmt, params);
  }
  template<typename... Args>
  __always_inline
  auto printf(string fmt, Args... args) {
    auto tup = std::make_tuple(args...);
    auto chain = make_chain_tuple(tup);
    cell params = build(chain).endc();
    return cout().printf(fmt, params);
  }
public:
  __always_inline
  void constructor() override {
  }

  __always_inline
  resumable<void> start() override {
    co_await cout().print("Hello, I am Tip3Info Debot!");
    tip3_ = ITONTokenWalletPtr(co_await cout().inputAddress("Tip3 token wallet address:", "TIP3WALLET"));
    do {
      auto tip3info = co_await tip3_->run().getDetails();
      co_await printf("Name:");
      co_await cout().print(tip3info.name);
      co_await printf("Symbol:");
      co_await cout().print(tip3info.symbol);
      co_await printf("Decimals: {uint8}", tip3info.decimals);
      co_await printf("Balance: {uint128}", tip3info.balance);
      if (tip3info.lend_ownership.owner == address::make_std(int8(0), uint256(0))) {
        co_await cout().print("Lend owner: None");
      } else {
        co_await printf("Lend owner: {address}", tip3info.lend_ownership.owner);
        co_await printf("Lend balance: {uint128}", tip3info.lend_ownership.lend_balance);
        co_await printf("Lend finish time: {datetime}", uint32(tip3info.lend_ownership.lend_finish_time.get()));
      }

      // uint256 root_public_key;
      // uint256 wallet_public_key;
      // address root_address;
      // address owner_address;
      // cell code;
      // allowance_info allowance;
      // int8 workchain_id;
    } while (co_await cout().inputYesOrNo("Update? (y/n)"));
  }
  DEFAULT_SUPPORT_FUNCTIONS(ITip3InfoDebot, void)

  // default processing of unknown messages
  __always_inline static int _fallback(cell msg, slice msg_body) {
    return 0;
  }
};

DEFINE_JSON_ABI(ITip3InfoDebot, DTip3InfoDebot, ETip3InfoDebot);

// ----------------------------- Main entry functions ---------------------- //
MAIN_ENTRY_FUNCTIONS_NO_REPLAY(Tip3InfoDebot, ITip3InfoDebot, DTip3InfoDebot)


