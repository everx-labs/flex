ifeq (, $(shell which tonos-cli))
 $(error "No tonos-cli in $(PATH)")
endif

ifeq (, $(shell clang -v 2>&1 | grep "TON Labs"))
 $(error "Wrong clang version in $(PATH)")
endif

RED="\033[31m"
CLEAR="\033[0m"
SHELL := /bin/bash
INCLUDES += -I ./ -I ../immutable -I ../exchange

all : | tvcs check_size
tvcs : FlexWallet.tvc FlexTokenRoot.tvc TONTokenWallet.tvc RootTokenContract.tvc \
     Wrapper.tvc WrapperEver.tvc WrapperBroxus.tvc \
     WrapperDeployerTip3.tvc WrapperDeployerEver.tvc WrapperDeployerBroxus.tvc

rebuild:
	$(MAKE) clean
	$(MAKE) all

FlexWallet.tvc: TONTokenWallet.cpp TONTokenWallet.hpp ../exchange/FlexTransferPayloadArgs.hpp
	clang TONTokenWallet.cpp -DTIP3_ENABLE_EXTERNAL -DTIP3_ENABLE_LEND_OWNERSHIP \
        -DTIP3_ENABLE_BURN -DTIP3_ONLY_FLEX_EXTERNAL -mtvm-refunc -o FlexWallet.tvc $(INCLUDES)

FlexWallet.hashes: FlexWallet.tvc Makefile
	tonos-cli decode stateinit --tvc FlexWallet.tvc | \
    grep -E "(code_hash|code_depth)" | \
    sed -e 's/"code_hash": "/-DTIP3_WALLET_CODE_HASH=0x/' \
        -e 's/"code_depth": "/-DTIP3_WALLET_CODE_DEPTH=/; s/",//g' | \
    tr -d '\n' | awk '{$$1=$$1};1' > FlexWallet.hashes

TONTokenWallet.hashes: TONTokenWallet.tvc Makefile
	tonos-cli decode stateinit --tvc TONTokenWallet.tvc | \
    grep -E "(code_hash|code_depth)" | \
    sed -e 's/"code_hash": "/-DTIP3_WALLET_CODE_HASH=0x/' \
        -e 's/"code_depth": "/-DTIP3_WALLET_CODE_DEPTH=/; s/",//g' | \
    tr -d '\n' | awk '{$$1=$$1};1' > TONTokenWallet.hashes

TONTokenWallet.tvc: TONTokenWallet.cpp TONTokenWallet.hpp
	clang TONTokenWallet.cpp -DTIP3_ENABLE_EXTERNAL -DTIP3_ENABLE_ALLOWANCE -DTIP3_DEPLOY_TRANSFER -DTIP3_ENABLE_DESTROY \
        -mtvm-refunc -o TONTokenWallet.tvc $(INCLUDES)

RootTokenContract.tvc: RootTokenContract.cpp RootTokenContract.hpp TONTokenWallet.hpp TONTokenWallet.hashes
	clang RootTokenContract.cpp $(file < TONTokenWallet.hashes) \
	-DTIP3_ENABLE_EXTERNAL -DTIP3_ENABLE_ALLOWANCE -DTIP3_DEPLOY_TRANSFER -DTIP3_ENABLE_DESTROY -o RootTokenContract.tvc $(INCLUDES)

FlexTokenRoot.tvc: RootTokenContract.cpp RootTokenContract.hpp TONTokenWallet.hpp FlexWallet.hashes
	clang RootTokenContract.cpp $(file < FlexWallet.hashes) -DTIP3_ENABLE_EXTERNAL -DTIP3_ENABLE_LEND_OWNERSHIP \
	-DTIP3_ENABLE_BURN -DTIP3_ONLY_FLEX_EXTERNAL -o FlexTokenRoot.tvc $(INCLUDES)

Wrapper.tvc: Wrapper.cpp Wrapper.hpp TONTokenWallet.hpp FlexWallet.hashes
	clang Wrapper.cpp -o Wrapper.tvc $(file < FlexWallet.hashes) \
        -DTIP3_ENABLE_EXTERNAL -DTIP3_ENABLE_ALLOWANCE -DTIP3_DEPLOY_TRANSFER -DTIP3_ENABLE_DESTROY $(INCLUDES)

WrapperEver.tvc: WrapperEver.cpp WrapperEver.hpp TONTokenWallet.hpp FlexWallet.hashes
	clang WrapperEver.cpp -o WrapperEver.tvc $(file < FlexWallet.hashes) \
        -DTIP3_ENABLE_EXTERNAL -DTIP3_ENABLE_ALLOWANCE -DTIP3_DEPLOY_TRANSFER -DTIP3_ENABLE_DESTROY $(INCLUDES)

WrapperBroxus.tvc: WrapperBroxus.cpp WrapperBroxus.hpp TONTokenWallet.hpp FlexWallet.hashes
	clang WrapperBroxus.cpp -o WrapperBroxus.tvc $(file < FlexWallet.hashes) \
        -DTIP3_ENABLE_EXTERNAL -DTIP3_ENABLE_ALLOWANCE -DTIP3_DEPLOY_TRANSFER -DTIP3_ENABLE_DESTROY $(INCLUDES)

WrapperDeployerTip3.tvc: WrapperDeployerTip3.cpp WrapperDeployerTip3.hpp Wrapper.hpp TONTokenWallet.hpp
	clang WrapperDeployerTip3.cpp -o WrapperDeployerTip3.tvc $(file < FlexWallet.hashes) \
        -DTIP3_ENABLE_EXTERNAL -DTIP3_ENABLE_ALLOWANCE -DTIP3_DEPLOY_TRANSFER -DTIP3_ENABLE_DESTROY $(INCLUDES)

WrapperDeployerEver.tvc: WrapperDeployerEver.cpp WrapperDeployerEver.hpp Wrapper.hpp TONTokenWallet.hpp
	clang WrapperDeployerEver.cpp -o WrapperDeployerEver.tvc $(file < FlexWallet.hashes) \
        -DTIP3_ENABLE_EXTERNAL -DTIP3_ENABLE_ALLOWANCE -DTIP3_DEPLOY_TRANSFER -DTIP3_ENABLE_DESTROY $(INCLUDES)

WrapperDeployerBroxus.tvc: WrapperDeployerBroxus.cpp WrapperDeployerBroxus.hpp Wrapper.hpp TONTokenWallet.hpp
	clang WrapperDeployerBroxus.cpp -o WrapperDeployerBroxus.tvc $(file < FlexWallet.hashes) \
        -DTIP3_ENABLE_EXTERNAL -DTIP3_ENABLE_ALLOWANCE -DTIP3_DEPLOY_TRANSFER -DTIP3_ENABLE_DESTROY $(INCLUDES)

check_size: tvcs
	@ls -l *.tvc | awk '{ if ($$5 > 16000) { print ${RED} "CONTRACT BIG SIZE" ${CLEAR} " | " $$5 " | " $$9; err = 1; } } END {exit err}'

clean:
	rm -f *.tvc
	rm -f *.abi
	rm -f *.hashes

