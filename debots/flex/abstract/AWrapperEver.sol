pragma ton-solidity >=0.53.0;

abstract contract AWrapperEver {
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
    function deployEmptyWallet(
        uint32 _answer_id,
        uint256 pubkey,
        optional(address) owner,
        uint128 evers) public functionID(0xc) returns(address value0) {}
    function getWalletAddress(uint256 pubkey, optional(address) owner) public functionID(0x12) returns(address value0) {}
 }
