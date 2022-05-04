pragma ton-solidity >=0.53.0;

struct WalletArgs {
    uint256 pubkey;
    optional(address) owner;
    uint128 evers;
}

abstract contract AEverWrapper {
    function getDetails() public functionID(0x10) returns(
        bytes name,
        bytes symbol,
        uint8 decimals,
        uint256 root_pubkey,
        uint128 total_granted,
        TvmCell wallet_code,
        address reserve_wallet,
        address flex
        ) {}

    function onEverTransfer(
        uint32 _answer_id,
        uint128 tokens,
        WalletArgs args,
        address answer_addr) public functionID(0xca) {}

}