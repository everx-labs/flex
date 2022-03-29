if [[ -z "$SRC" ]]; then
    echo "Must provide SRC in environment" 1>&2
    exit 1
fi

cp $SRC/calc_wrapper_reserve_wallet.hpp ./
cp $SRC/FlexLendPayloadArgs.hpp ./
cp $SRC/Tip3Creds.hpp ./
cp $SRC/FlexTokenRoot.abi ./
cp $SRC/FlexTokenRoot.tvc ./
cp $SRC/FlexWallet.abi ./
cp $SRC/FlexWallet.hashes ./
cp $SRC/FlexWallet.hpp ./
cp $SRC/FlexWallet.tvc ./
cp $SRC/RootTokenContract.abi ./
cp $SRC/RootTokenContract.cpp ./
cp $SRC/RootTokenContract.hpp ./
cp $SRC/RootTokenContract.tvc ./
cp $SRC/TONTokenWallet.abi ./
cp $SRC/TONTokenWallet.cpp ./
cp $SRC/TONTokenWallet.hpp ./
cp $SRC/TONTokenWallet.hashes ./
cp $SRC/TONTokenWallet.tvc ./
cp $SRC/WrapperCommon.hpp ./
cp $SRC/Wrapper.abi ./
cp $SRC/Wrapper.cpp ./
cp $SRC/Wrapper.hpp ./
cp $SRC/Wrapper.tvc ./
cp $SRC/WrapperEver.abi ./
cp $SRC/WrapperEver.cpp ./
cp $SRC/WrapperEver.hpp ./
cp $SRC/WrapperEver.tvc ./
