all: Flex.tvc TradingPair.tvc XchgPair.tvc Price.tvc PriceXchg.tvc FlexClient.tvc

Flex.tvc: Flex.cpp Flex.hpp TradingPair.hpp XchgPair.hpp Price.hpp PriceXchg.hpp PriceCommon.hpp
	clang -o Flex.tvc Flex.cpp -I tokens -Wall

TradingPair.tvc: TradingPair.cpp Flex.hpp TradingPair.hpp Price.hpp PriceCommon.hpp
	clang -o TradingPair.tvc TradingPair.cpp -I tokens -Wall

XchgPair.tvc: XchgPair.cpp Flex.hpp XchgPair.hpp PriceXchg.hpp PriceCommon.hpp
	clang -o XchgPair.tvc XchgPair.cpp -I tokens -Wall

Price.tvc: Price.cpp Flex.hpp TradingPair.hpp Price.hpp PriceCommon.hpp
	clang -mtvm-refunc -o Price.tvc Price.cpp -I tokens -Wall

PriceXchg.tvc: PriceXchg.cpp Flex.hpp XchgPair.hpp PriceXchg.hpp PriceCommon.hpp
	clang -mtvm-refunc -o PriceXchg.tvc PriceXchg.cpp -I tokens -Wall

FlexClient.tvc: FlexClient.cpp tokens/TONTokenWallet.hpp tokens/RootTokenContract.hpp
	clang -o FlexClient.tvc FlexClient.cpp -I tokens -Wall

clean:
	rm -f *.abi
	rm -f *.tvc

