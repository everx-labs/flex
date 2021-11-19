all: Flex.tvc TradingPair.tvc XchgPair.tvc Price.tvc PriceXchg.tvc FlexClient.tvc

TOKENS_DIR = ../tokens-fungible
TOKENS_INCLUDE = -I $(TOKENS_DIR)

Flex.tvc: Flex.cpp Flex.hpp TradingPair.hpp XchgPair.hpp Price.hpp PriceXchg.hpp PriceCommon.hpp
	clang -mtvm-refunc -o Flex.tvc Flex.cpp $(TOKENS_INCLUDE) -Wall

TradingPair.tvc: TradingPair.cpp Flex.hpp TradingPair.hpp Price.hpp PriceCommon.hpp
	clang -o TradingPair.tvc TradingPair.cpp $(TOKENS_INCLUDE) -Wall

XchgPair.tvc: XchgPair.cpp Flex.hpp XchgPair.hpp PriceXchg.hpp PriceCommon.hpp
	clang -o XchgPair.tvc XchgPair.cpp $(TOKENS_INCLUDE) -Wall

Price.tvc: Price.cpp Flex.hpp TradingPair.hpp Price.hpp PriceCommon.hpp
	clang -mtvm-refunc -o Price.tvc Price.cpp $(TOKENS_INCLUDE) -Wall

PriceXchg.tvc: PriceXchg.cpp Flex.hpp XchgPair.hpp PriceXchg.hpp PriceCommon.hpp
	clang -mtvm-refunc -o PriceXchg.tvc PriceXchg.cpp $(TOKENS_INCLUDE) -Wall

FlexClient.tvc: FlexClient.cpp FlexClient.hpp $(TOKENS_DIR)/TONTokenWallet.hpp $(TOKENS_DIR)/FlexWallet.hpp $(TOKENS_DIR)/RootTokenContract.hpp
	clang -o FlexClient.tvc FlexClient.cpp $(TOKENS_INCLUDE) -Wall

stTONs.tvc: stTONs.cpp stTONs.hpp DePool.hpp PriceCommon.hpp $(TOKENS_DIR)/RootTokenContract.hpp DePool.hpp
	clang -o stTONs.tvc stTONs.cpp $(TOKENS_INCLUDE) -Wall

clean:
	rm -f *.abi
	rm -f *.tvc

