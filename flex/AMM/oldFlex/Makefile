all: Proxy.tvc FLeX.tvc TradingPair.tvc Price.tvc freetr_debot.tvc tip3info.tvc FLeXClient.tvc FLeXDebot.tvc

Proxy.tvc: Proxy.cpp Proxy.hpp
	clang -o Proxy.tvc Proxy.cpp -I ../tokens

FLeX.tvc: FLeX.cpp FLeX.hpp TradingPair.hpp Price.hpp
	clang -o FLeX.tvc FLeX.cpp -I ../tokens

TradingPair.tvc: TradingPair.cpp FLeX.hpp TradingPair.hpp Price.hpp
	clang -o TradingPair.tvc TradingPair.cpp -I ../tokens

Price.tvc: Price.cpp FLeX.hpp TradingPair.hpp Price.hpp
	clang -mtvm-refunc -o Price.tvc Price.cpp -I ../tokens

freetr_debot.tvc: freetr_debot.cpp FLeX.hpp TradingPair.hpp Price.hpp
	clang -o freetr_debot.tvc freetr_debot.cpp -I ../tokens

tip3info.tvc: tip3info.cpp ../tokens/TONTokenWallet.hpp ../tokens/RootTokenContract.hpp
	clang -o tip3info.tvc tip3info.cpp -I ../tokens

FLeXClient.tvc: FLeXClient.cpp ../tokens/TONTokenWallet.hpp ../tokens/RootTokenContract.hpp
	clang -o FLeXClient.tvc FLeXClient.cpp -I ../tokens

FLeXDebot.tvc: FLeXDebot.cpp ../tokens/TONTokenWallet.hpp ../tokens/RootTokenContract.hpp
	clang -o FLeXDebot.tvc FLeXDebot.cpp -I ../tokens

