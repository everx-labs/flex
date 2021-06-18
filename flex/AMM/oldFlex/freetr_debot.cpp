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

__interface [[no_pubkey]] IFreetrDebot {

  [[external]]
  void constructor() = 1;

  [[external]]
  resumable<void> start() = 2;
};

struct DFreetrDebot {
  uint256 deployer_pubkey_;
  std::optional<cell> flex_code_;
  std::optional<IFLeXPtr> flex_;
};

__interface EFreetrDebot {
};

class FreetrDebot final : public smart_interface<IFreetrDebot>, public DFreetrDebot {
  struct error_code : tvm::error_code {
    static constexpr unsigned unexpected_pair_address = 101;
  };

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
    co_await cout().print("Hello, I am FLeX Debot!");
    do {
      co_await cout().iAmHome();
      if (flex_) {
        co_await printf("flex: {address}", flex_->get());
      }
      co_await printf("1). Buy tip3 tokens\n"
                      "2). Sell tip3 tokens\n"
                      "3). Exit");
      auto select = co_await cout().inputUint256("");
      switch (select.get()) {
      case 1: {
        if (!flex_) {
          flex_ = IFLeXPtr(co_await cout().inputAddress("FLeX address:", "FLEX"));
          co_await printf("FLeX: {address}", flex_->get());
        }
        auto my_tip3 = ITONTokenWalletPtr(co_await cout().inputAddress("Token wallet address (to receive tokens):", "TIP3WALLET"));
        auto tip3info = co_await my_tip3.run().getDetails();

        co_await cout().print(tip3info.name);
        co_await printf("Tip3 root address = {address}", tip3info.root_address);
        auto trade_pair = ITradingPairPtr(co_await flex_->run().getSellTradingPair(tip3info.root_address));
        co_await printf("Sell pair is {address}", trade_pair.get());

        TonsConfig tonsCfg = co_await flex_->run().getTonsCfg();

        cell price_code = co_await flex_->run().getSellPriceCode(tip3info.root_address);
        if (1 != co_await cout().getAccountType(trade_pair.get())) {
          co_await printf("Sell Pair is not yet exists and need to be deployed:");
          co_await printf("Deploying Sell Pair for:");
          co_await cout().print(tip3info.name);
          auto deploy_value = co_await cout().inputTONs("Deploy value (extra will return): ");
          DTradingPair pair_data {
            .flex_addr_ = flex_->get(),
            .tip3_root_ = tip3info.root_address,
            .deploy_value_ = tonsCfg.trading_pair_deploy
          };

          auto [state_init, std_addr] = prepare_trading_pair_state_init_and_addr(pair_data, co_await flex_->run().getTradingPairCode());
          auto pair_addr = address::make_std(int8(0), std_addr);
          require(pair_addr == trade_pair.get(), error_code::unexpected_pair_address);
          co_await trade_pair.deploy(state_init, Grams(deploy_value.get())).onDeploy();
          co_await printf("Sell pair deployed: {address}", trade_pair.get());
        }
        Tip3Config tip3Cfg {
          .name = tip3info.name,
          .symbol = tip3info.symbol,
          .decimals = tip3info.decimals,
          .root_public_key = tip3info.root_public_key,
          .root_address = tip3info.root_address,
          .code = tip3info.code
        };

        auto price = co_await cout().inputTONs("Price in TONs:");
        auto amount = uint128((co_await cout().inputUint256("Amount:")).get());
        auto hours = co_await cout().inputUint256("Order time in hours:");
        uint32 order_finish_time((tvm_now() + hours * 60 * 60).get());
        DPrice price_data {
          .price_ = uint128(price.get()),
          .sells_amount_ = uint128(0),
          .buys_amount_ = uint128(0),
          .flex_ = flex_->get(),
          .min_amount_ = uint128(1),
          .deals_limit_ = uint8(10),
          .workchain_id_ = int8(0),
          .tons_cfg_ = tonsCfg,
          .tip3cfg_ = tip3Cfg,
          .sells_ = {},
          .buys_ = {}
        };
        auto [state_init, std_addr] = prepare_price_state_init_and_addr(price_data, price_code);
        auto price_addr = address::make_std(int8(0), std_addr);
        co_await printf("price_addr: {address}", price_addr);
        IPricePtr price_ptr(price_addr);
        auto ret = co_await price_ptr.deploy(state_init, Grams(10000000000)).
          buyTip3(amount, my_tip3.get(), order_finish_time);

        co_await printf("ret.err_code: {uint32}", ret.err_code);
        co_await printf("ret.processed: {uint128}", ret.processed);
        co_await printf("ret.enqueued: {uint128}", ret.enqueued);
        // auto price_info = co_await price_ptr.run().getDetails();
        // co_await printf("price getter: {value}", uint256(price_info.price.get()));
        // co_await printf("sell amount in price: {uint128}", price_info.sell_amount);
        // co_await printf("buy amount in price: {uint128}", price_info.buy_amount);
      } break;
      case 2: {
        if (!flex_) {
          flex_ = IFLeXPtr(co_await cout().inputAddress("FLeX address:", "FLEX"));
          co_await printf("FLeX: {address}", flex_->get());
          bool_t initialized = co_await flex_->run().isFullyInitialized();
          if (initialized) {
            co_await printf("FLeX is fully initialized: {address}\n", flex_->get());
          } else {
            co_await printf("FLeX is NOT fully initialized: {address}\n", flex_->get());
            continue;
          }
        }
        auto my_tip3 = ITONTokenWalletPtr(co_await cout().inputAddress("Token wallet address:", "TIP3WALLET"));
        auto tip3info = co_await my_tip3.run().getDetails();
        co_await cout().print(tip3info.name);
        co_await printf("Tip3 root address = {address}", tip3info.root_address);
        // co_await printf("Token \"{bytes}\"", tip3info.name);
        auto trade_pair = ITradingPairPtr(co_await flex_->run().getSellTradingPair(tip3info.root_address));
        co_await printf("Sell pair is {address}", trade_pair.get());
        auto receive_wallet = co_await cout().inputAddress("TON wallet address:", "TONWALLET");

        TonsConfig tonsCfg = co_await flex_->run().getTonsCfg();

        cell price_code = co_await flex_->run().getSellPriceCode(tip3info.root_address);
        if (1 != co_await cout().getAccountType(trade_pair.get())) {
          co_await printf("Deploying Sell Pair for:");
          co_await cout().print(tip3info.name);
          auto deploy_value = co_await cout().inputTONs("Deploy value (extra will return): ");
          DTradingPair pair_data {
            .flex_addr_ = flex_->get(),
            .tip3_root_ = tip3info.root_address,
            .deploy_value_ = tonsCfg.trading_pair_deploy
          };

          auto [state_init, std_addr] = prepare_trading_pair_state_init_and_addr(pair_data, co_await flex_->run().getTradingPairCode());
          auto pair_addr = address::make_std(int8(0), std_addr);
          require(pair_addr == trade_pair.get(), error_code::unexpected_pair_address);
          co_await trade_pair.deploy(state_init, Grams(deploy_value.get())).onDeploy();
          co_await printf("Sell pair deployed: {address}", trade_pair.get());
        }

        Tip3Config tip3Cfg {
          .name = tip3info.name,
          .symbol = tip3info.symbol,
          .decimals = tip3info.decimals,
          .root_public_key = tip3info.root_public_key,
          .root_address = tip3info.root_address,
          .code = tip3info.code
        };

        auto price = co_await cout().inputTONs("Price in TONs:");
        auto amount = uint128((co_await cout().inputUint256("Amount:")).get());
        auto hours = co_await cout().inputUint256("Order time in hours:");
        DPrice price_data {
          .price_ = uint128(price.get()),
          .sells_amount_ = uint128(0),
          .buys_amount_ = uint128(0),
          .flex_ = flex_->get(),
          .min_amount_ = uint128(1),
          .deals_limit_ = uint8(10),
          .workchain_id_ = int8(0),
          .tons_cfg_ = tonsCfg,
          .tip3cfg_ = tip3Cfg,
          .sells_ = {},
          .buys_ = {}
        };
        auto [state_init, std_addr] = prepare_price_state_init_and_addr(price_data, price_code);
        auto price_addr = address::make_std(int8(0), std_addr);
        co_await printf("price_addr: {address}", price_addr);
        cell deploy_init_cl = build(state_init).endc();
        SellArgs args = {
          .amount = amount,
          .receive_wallet = fixed_addr(receive_wallet)
        };
        cell payload = build(args).endc();
        uint32 cur_time(tvm_now());
        uint32 lend_finish_time((tvm_now() + hours * 60 * 60).get());
        co_await printf("[dbg] cur_time={uint32}, lend_finish_time = {uint32}", cur_time, lend_finish_time);
        co_await printf("The order will ends at {datetime}", lend_finish_time);

        auto grams = tonsCfg.transfer_tip3 + tonsCfg.process_queue + tonsCfg.return_ownership + 1000000000;
        co_await printf("my_tip3: {address}", my_tip3.get());
        auto ret = co_await my_tip3.tail_call<OrderRet>(price_addr, Grams(6000000000)).
          lendOwnership(std::get<addr_std>(price_addr.val()).address, amount, lend_finish_time, deploy_init_cl, payload);
        co_await printf("ret.err_code: {uint32}", ret.err_code);
        co_await printf("ret.processed: {uint128}", ret.processed);
        co_await printf("ret.enqueued: {uint128}", ret.enqueued);

        IPricePtr price_ptr(price_addr);
        // auto price_info = co_await price_ptr.run().getDetails();
        // co_await printf("price getter: {value}", uint256(price_info.price.get()));
        // co_await printf("sell amount in price: {uint128}", price_info.sell_amount);
        // co_await printf("buy amount in price: {uint128}", price_info.buy_amount);
      } break;
      case 3:
        co_await cout().print("You choose to exit, bye");
        co_return;
      }
    } while(true);
  }
  DEFAULT_SUPPORT_FUNCTIONS(IFreetrDebot, void)
  
  // default processing of unknown messages
  __always_inline static int _fallback(cell msg, slice msg_body) {
    return 0;
  }
private:
  __always_inline
  addr_std_fixed fixed_addr(address addr) const {
    auto unpacked = std::get<addr_std>(addr.val());
    return { unpacked.workchain_id, unpacked.address };
  }
};

DEFINE_JSON_ABI(IFreetrDebot, DFreetrDebot, EFreetrDebot);

// ----------------------------- Main entry functions ---------------------- //
MAIN_ENTRY_FUNCTIONS_NO_REPLAY(FreetrDebot, IFreetrDebot, DFreetrDebot)


