pragma ton-solidity >=0.47.0;

abstract contract Utility {
    function splitTokens(uint128 tokens, uint8 decimals) private pure returns (uint64, uint64) {
        uint128 val = 1;
        for(int i = 0; i < decimals; i++) {
            val *= 10;
        }
        uint64 integer = uint64(tokens / val);
        uint64 float = uint64(tokens - (integer * val));
        return (integer, float);
    }

    function tokensToStr(uint128 tokens, uint8 decimals) public pure returns (string) {
        (uint64 dec, uint64 float) = splitTokens(tokens, decimals);
        string floatStr = format("{}", float);
        while (floatStr.byteLength() < decimals) {
            floatStr = "0" + floatStr;
        }
        string result = format("{}.{}", dec, floatStr);
        return result;
    }
}