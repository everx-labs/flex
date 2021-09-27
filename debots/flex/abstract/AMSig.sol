pragma ton-solidity >=0.47.0;

abstract contract AMSig {
    function sendTransaction(
        address dest,
        uint128 value,
        bool bounce,
        bool allBalance,
        TvmCell payload)
    public {}
    function submitTransaction(
        address dest,
        uint128 value,
        bool bounce,
        bool allBalance,
        TvmCell payload)
    public returns (uint64 transId) {}
}