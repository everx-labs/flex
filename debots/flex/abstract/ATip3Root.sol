pragma ton-solidity >=0.47.0;

abstract contract ATip3Root {
    constructor(bytes name, bytes symbol, uint8 decimals, uint256 root_public_key, address root_owner, uint128 total_supply) public functionID(0xa) {}
    function getName() public  functionID(0x11) returns(string value0) {}
    function getSymbol() public  functionID(0x12) returns(bytes value0){}
    function getRootKey() public functionID(0x14) returns(uint256 value0){}
    function setWalletCode(uint32 _answer_id, TvmCell wallet_code) public  functionID(0xb) returns(bool value0)  {}
    function deployWallet(uint32 _answer_id, uint256 pubkey, uint256 internal_owner,
        uint128 tokens, uint128 grams) public  functionID(0xc) returns(address value0) {}
    function grant(uint32 _answer_id, address dest, uint128 tokens, uint128 grams) public  functionID(0xe) {}
    function getWalletAddress(uint256 pubkey, address owner) public functionID(0x19) returns(address value0) {}
    function getDecimals() public  functionID(0x13) returns (uint8 value0) {}
    function getTotalSupply() public functionID(0x15) returns (uint128 value0) {}
    function getTotalGranted() public functionID(0x16) returns (uint128 value0) {}
    function getWalletCode() public functionID(0x18) returns (TvmCell value0) {}
    function getWalletCodeHash() public functionID(0x1a) returns(uint256 value0) {}
    function deployEmptyWallet(uint32 _answer_id, uint256 pubkey, address internal_owner, uint128 grams) public functionID(0xd) returns(address value0) {}

}