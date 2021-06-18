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
#include "FLeXClient.hpp"

using namespace tvm;
using namespace schema;
using namespace debot;

__interface [[no_pubkey]] IFLeXDebot {

  [[external]]
  void constructor() = 1;

  [[external]]
  resumable<void> start() = 2;
};

struct DFLeXDebot {
  uint256 deployer_pubkey_;
  std::optional<IFLeXPtr> flex_;
  std::optional<IFLeXClientPtr> client_;
};

__interface EFLeXDebot {
};

class FLeXDebot final : public smart_interface<IFLeXDebot>, public DFLeXDebot {
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
      if (!flex_) {
        flex_ = IFLeXPtr(co_await cout().inputAddress("FLeX address:", "FLEX"));
        bool_t initialized = co_await flex_->run().isFullyInitialized();
        if (initialized) {
          co_await printf("FLeX is fully initialized: {address}\n", flex_->get());
        } else {
          co_await printf("FLeX is NOT fully initialized: {address}\n", flex_->get());
          flex_.reset();
          continue;
        }
      }
      auto tons_cfg = co_await flex_->run().getTonsCfg();
      auto notify_addr = co_await flex_->run().getNotifyAddr();
      if (!client_) {
        client_ = IFLeXClientPtr(co_await cout().inputAddress("FLeX client address:", "FLEX_CLIENT"));
        co_await client_->debot_ext_in().setFLeXCfg(tons_cfg, flex_->get(), notify_addr);
      }
      co_await cout().iAmHome();
      co_await printf("FLeX: {address}", flex_->get());
      co_await printf("client: {address}", client_->get());

      co_await printf("1). Buy tip3 tokens\n"
                      "2). Sell tip3 tokens\n"
                      "3). Cancel order\n"
                      "4). Exit");
      auto select = co_await cout().inputUint256("");
      switch (select.get()) {
      case 1: {
        auto my_tip3 = ITONTokenWalletPtr(co_await cout().inputAddress("Token wallet address (to receive tokens):", "TIP3WALLET"));
        auto tip3info = co_await my_tip3.run().getDetails();

        co_await cout().print(tip3info.name);
        co_await printf("Tip3 root address = {address}", tip3info.root_address);
        auto trade_pair = ITradingPairPtr(co_await flex_->run().getSellTradingPair(tip3info.root_address));
        co_await printf("Sell pair is {address}", trade_pair.get());

        cell price_code = co_await flex_->run().getSellPriceCode(tip3info.root_address);
        if (1 != co_await cout().getAccountType(trade_pair.get())) {
          co_await printf("Sell Pair is not yet exists and need to be deployed:");
          co_await printf("Deploying Sell Pair for:");
          co_await cout().print(tip3info.name);
          auto deploy_value = uint128((co_await cout().inputTONs("Deploy value (extra will return): ")).get());

          auto deployed_pair = co_await client_->debot_ext_in().deployTradingPair(
            flex_->get(), tip3info.root_address, tons_cfg.trading_pair_deploy, deploy_value);
          co_await printf("Sell pair deployed: {address}", deployed_pair);
        }
        Tip3Config tip3cfg {
          .name = tip3info.name,
          .symbol = tip3info.symbol,
          .decimals = tip3info.decimals,
          .root_public_key = tip3info.root_public_key,
          .root_address = tip3info.root_address,
          .code = tip3info.code
        };

        auto price = uint128((co_await cout().inputTONs("Price in TONs:")).get());
        auto amount = uint128((co_await cout().inputUint256("Amount:")).get());
        auto minutes = co_await cout().inputUint256("Order time in minutes:");
        uint32 order_finish_time((tvm_now() + minutes * 60).get());

        auto min_amount = co_await flex_->run().getMinAmount();
        auto deals_limit = co_await flex_->run().getDealsLimit();

        uint128 deploy_value;
        uint128 min_deploy_value = price * amount + tons_cfg.process_queue + tons_cfg.order_answer;
        do {
          co_await printf("Required value is {value}", uint256(min_deploy_value.get()));
          deploy_value = uint128((co_await cout().inputTONs("Send value (extra will return): ")).get());
        } while (deploy_value < min_deploy_value);

        FLeXBuyArgs buy_args {
          price, amount, order_finish_time, min_amount, deals_limit, deploy_value, price_code, my_tip3.get(), tip3cfg
          };

        IPricePtr price_addr(co_await client_->debot_ext_in().deployPriceWithBuy(build(buy_args).endc()));

        //auto price_info = co_await price_addr.run().getDetails();
        //co_await printf("price getter: {value}", uint256(price_info.price.get()));
        //co_await printf("sell amount in price: {uint128}", price_info.sell_amount);
        //co_await printf("buy amount in price: {uint128}", price_info.buy_amount);
      } break;
      case 2: {
        if (!flex_) {
          flex_ = IFLeXPtr(co_await cout().inputAddress("FLeX address:", "FLEX"));
          co_await printf("FLeX: {address}", flex_->get());
        }
        auto my_tip3 = ITONTokenWalletPtr(co_await cout().inputAddress("Token wallet address:", "TIP3WALLET"));
        auto tip3info = co_await my_tip3.run().getDetails();
        co_await cout().print(tip3info.name);
        co_await printf("Tip3 root address = {address}", tip3info.root_address);
        // co_await printf("Token \"{bytes}\"", tip3info.name);
        auto trade_pair = ITradingPairPtr(co_await flex_->run().getSellTradingPair(tip3info.root_address));
        co_await printf("Sell pair is {address}", trade_pair.get());
        address receive_wallet = client_->get();

        cell price_code = co_await flex_->run().getSellPriceCode(tip3info.root_address);
        if (1 != co_await cout().getAccountType(trade_pair.get())) {
          co_await printf("Deploying Sell Pair for:");
          co_await cout().print(tip3info.name);
          auto deploy_value = uint128((co_await cout().inputTONs("Deploy value (extra will return): ")).get());
          auto deployed_pair = co_await client_->debot_ext_in().deployTradingPair(
            flex_->get(), tip3info.root_address, tons_cfg.trading_pair_deploy, deploy_value);
          co_await printf("Sell pair deployed: {address}", deployed_pair);
        }

        Tip3Config tip3cfg {
          .name = tip3info.name,
          .symbol = tip3info.symbol,
          .decimals = tip3info.decimals,
          .root_public_key = tip3info.root_public_key,
          .root_address = tip3info.root_address,
          .code = tip3info.code
        };

        auto price = uint128((co_await cout().inputTONs("Price in TONs:")).get());
        auto amount = uint128((co_await cout().inputUint256("Amount:")).get());
        auto minutes = co_await cout().inputUint256("Order time in minutes:");
        uint32 lend_finish_time((tvm_now() + minutes * 60).get());

        auto min_amount = co_await flex_->run().getMinAmount();
        auto deals_limit = co_await flex_->run().getDealsLimit();

        uint128 tons_value;
        auto min_value = tons_cfg.process_queue + tons_cfg.transfer_tip3 +
          tons_cfg.return_ownership + tons_cfg.order_answer;
        do {
          co_await printf("Required value is {value}", uint256(min_value.get()));
          tons_value = uint128((co_await cout().inputTONs("Processing value (extra will return):")).get());
        } while (tons_value < min_value);

        FLeXSellArgs sell_args {
          price, amount, lend_finish_time, min_amount, deals_limit, tons_value, price_code,
          FLeXSellArgsAddrs { my_tip3.get(), receive_wallet }, tip3cfg
          };

        IPricePtr price_addr(co_await client_->debot_ext_in().deployPriceWithSell(build(sell_args).endc()));

        // auto price_info = co_await price_addr.run().getDetails();
        // co_await printf("price getter: {value}", uint256(price_info.price.get()));
        // co_await printf("sell amount in price: {uint128}", price_info.sell_amount);
        // co_await printf("buy amount in price: {uint128}", price_info.buy_amount);
      } break;
      case 3: {
        co_await printf("1). Cancel sell\n"
                        "2). Cancel buy\n");
        auto select = co_await cout().inputUint256("");
        if (select == 1) {
          auto my_tip3 = ITONTokenWalletPtr(co_await cout().inputAddress("Token wallet address:", "TIP3WALLET"));
          auto tip3info = co_await my_tip3.run().getDetails();
          co_await cout().print(tip3info.name);
          Tip3Config tip3cfg {
            .name = tip3info.name,
            .symbol = tip3info.symbol,
            .decimals = tip3info.decimals,
            .root_public_key = tip3info.root_public_key,
            .root_address = tip3info.root_address,
            .code = tip3info.code
          };
          cell price_code = co_await flex_->run().getSellPriceCode(tip3info.root_address);
          auto min_amount = co_await flex_->run().getMinAmount();
          auto deals_limit = co_await flex_->run().getDealsLimit();
          auto price = uint128((co_await cout().inputTONs("Price in TONs:")).get());
          auto tons_value = uint128((co_await cout().inputTONs("Processing value (extra will return):")).get());
          FLeXCancelArgs cancel_args {
            price, min_amount, deals_limit, tons_value, price_code, tip3cfg
            };
          co_await client_->debot_ext_in().cancelSellOrder(build(cancel_args).endc());
        } else {
          auto my_tip3 = ITONTokenWalletPtr(co_await cout().inputAddress("Token wallet address:", "TIP3WALLET"));
          auto tip3info = co_await my_tip3.run().getDetails();
          co_await cout().print(tip3info.name);
          Tip3Config tip3cfg {
            .name = tip3info.name,
            .symbol = tip3info.symbol,
            .decimals = tip3info.decimals,
            .root_public_key = tip3info.root_public_key,
            .root_address = tip3info.root_address,
            .code = tip3info.code
          };
          cell price_code = co_await flex_->run().getSellPriceCode(tip3info.root_address);
          auto min_amount = co_await flex_->run().getMinAmount();
          auto deals_limit = co_await flex_->run().getDealsLimit();
          auto price = uint128((co_await cout().inputTONs("Price in TONs:")).get());
          auto tons_value = uint128((co_await cout().inputTONs("Processing value (extra will return):")).get());
          FLeXCancelArgs cancel_args {
            price, min_amount, deals_limit, tons_value, price_code, tip3cfg
            };
          co_await client_->debot_ext_in().cancelBuyOrder(build(cancel_args).endc());
        } break;
      }
      case 4:
        co_await cout().print("You choose to exit, bye");
        co_return;
      }
    } while(true);
  }
  DEFAULT_SUPPORT_FUNCTIONS(IFLeXDebot, void)

  // default processing of unknown messages
  __always_inline static int _fallback(cell /*msg*/, slice /*msg_body*/) {
    return 0;
  }
};

DEFINE_JSON_ABI(IFLeXDebot, DFLeXDebot, EFLeXDebot);

// ----------------------------- Main entry functions ---------------------- //
MAIN_ENTRY_FUNCTIONS_NO_REPLAY(FLeXDebot, IFLeXDebot, DFLeXDebot)
