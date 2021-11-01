all: Proxy.tvc Flex.tvc TradingPair.tvc XchgPair.tvc Price.tvc PriceXchg.tvc tip3info.tvc FlexClient.tvc TestTradeDebot.tvc stTONs.tvc DePoolMock.tvc stTONsClientMock.tvc

Proxy.tvc: Proxy.cpp Proxy.hpp
	clang -o Proxy.tvc Proxy.cpp -I ../tokens/fungible -Wall

Flex.tvc: Flex.cpp Flex.hpp TradingPair.hpp XchgPair.hpp Price.hpp PriceXchg.hpp PriceCommon.hpp
	clang -mtvm-refunc -o Flex.tvc Flex.cpp -I ../tokens/fungible -Wall

TradingPair.tvc: TradingPair.cpp Flex.hpp TradingPair.hpp Price.hpp PriceCommon.hpp
	clang -o TradingPair.tvc TradingPair.cpp -I ../tokens/fungible -Wall

XchgPair.tvc: XchgPair.cpp Flex.hpp XchgPair.hpp PriceXchg.hpp PriceCommon.hpp
	clang -o XchgPair.tvc XchgPair.cpp -I ../tokens/fungible -Wall

Price.tvc: Price.cpp Flex.hpp TradingPair.hpp Price.hpp PriceCommon.hpp
	clang -mtvm-refunc -o Price.tvc Price.cpp -I ../tokens/fungible -Wall

PriceXchg.tvc: PriceXchg.cpp Flex.hpp XchgPair.hpp PriceXchg.hpp PriceCommon.hpp
	clang -mtvm-refunc -o PriceXchg.tvc PriceXchg.cpp -I ../tokens/fungible -Wall

freetr_debot.tvc: freetr_debot.cpp Flex.hpp TradingPair.hpp XchgPair.hpp Price.hpp PriceXchg.hpp PriceCommon.hpp
	clang -o freetr_debot.tvc freetr_debot.cpp -I ../tokens/fungible -Wall

tip3info.tvc: tip3info.cpp ../tokens/fungible/TONTokenWallet.hpp ../tokens/fungible/FlexWallet.hpp ../tokens/fungible/RootTokenContract.hpp
	clang -o tip3info.tvc tip3info.cpp -I ../tokens/fungible -Wall

FlexClient.tvc: FlexClient.cpp FlexClient.hpp ../tokens/fungible/TONTokenWallet.hpp ../tokens/fungible/FlexWallet.hpp ../tokens/fungible/RootTokenContract.hpp
	clang -o FlexClient.tvc FlexClient.cpp -I ../tokens/fungible -Wall

FlexDebot.tvc: FlexDebot.cpp ../tokens/fungible/TONTokenWallet.hpp ../tokens/fungible/FlexWallet.hpp ../tokens/fungible/RootTokenContract.hpp
	clang -o FlexDebot.tvc FlexDebot.cpp -I ../tokens/fungible -Wall

TestTradeDebot.tvc: TestTradeDebot.cpp Flex.hpp TradingPair.hpp XchgPair.hpp Price.hpp PriceXchg.hpp \
    ../tokens/fungible/TONTokenWallet.hpp ../tokens/fungible/FlexWallet.hpp ../tokens/fungible/RootTokenContract.hpp
	clang -o TestTradeDebot.tvc TestTradeDebot.cpp -I ../tokens/fungible -Wall

FlexClientTest.tvc: FlexClientTest.cpp FlexClient.hpp TradingPair.hpp XchgPair.hpp Price.hpp PriceXchg.hpp \
    ../tokens/fungible/TONTokenWallet.hpp ../tokens/fungible/FlexWallet.hpp ../tokens/fungible/RootTokenContract.hpp
	clang -o FlexClientTest.tvc FlexClientTest.cpp -I ../tokens/fungible -Wall \
	-DTIP3_ENABLE_EXTERNAL -DTIP3_ENABLE_ALLOWANCE -DTIP3_IMPROVED_TRANSFER

stTONs.tvc: stTONs.cpp stTONs.hpp DePool.hpp PriceCommon.hpp ../tokens/fungible/RootTokenContract.hpp DePool.hpp
	clang -o stTONs.tvc stTONs.cpp -I ../tokens/fungible -Wall

DePoolMock.tvc: DePoolMock.cpp DePoolMock.hpp
	clang -o DePoolMock.tvc DePoolMock.cpp -I ../tokens/fungible -Wall

stTONsClientMock.tvc: stTONsClientMock.cpp stTONsClientMock.hpp
	clang -o stTONsClientMock.tvc stTONsClientMock.cpp -I ../tokens/fungible -Wall

clean:
	rm -f *.abi
	rm -f *.tvc

