if [[ -z "$SRC" ]]; then
    echo "Must provide SRC in environment" 1>&2
    exit 1
fi

set -x

cp $SRC/EversConfig.hpp ./
cp $SRC/ListingConfig.hpp ./
cp $SRC/Flex.abi ./
cp $SRC/Flex.cpp ./
cp $SRC/Flex.hpp ./
cp $SRC/Flex.tvc ./
cp $SRC/FlexClient.abi ./
cp $SRC/FlexClient.cpp ./
cp $SRC/FlexClient.hpp ./
cp $SRC/FlexClient.tvc ./
cp $SRC/FlexLendPayloadArgs.hpp ./
cp $SRC/FlexTransferPayloadArgs.hpp ./
cp $SRC/PriceCommon.hpp ./
cp $SRC/PriceXchg.abi ./
cp $SRC/PriceXchg.cpp ./
cp $SRC/PriceXchg.hpp ./
cp $SRC/PriceXchg.tvc ./
cp -r $SRC/xchg ./
cp $SRC/PriceXchgSalt.hpp ./
cp $SRC/RationalValue.hpp ./
cp $SRC/stTONs.abi ./
cp $SRC/stTONs.cpp ./
cp $SRC/stTONs.hpp ./
cp $SRC/stTONs.tvc ./
cp $SRC/XchgPair.abi ./
cp $SRC/XchgPair.cpp ./
cp $SRC/XchgPair.hpp ./
cp $SRC/XchgPair.tvc ./
cp $SRC/XchgPairSalt.hpp ./

rm -r Flex.doc
cp -r $SRC/Flex.doc ./

