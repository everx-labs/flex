pragma ton-solidity >=0.47.0;

abstract contract AWrapper {
    function getDetails() public functionID(0xf) returns(bytes name, bytes symbol, uint8 decimals, uint256 root_public_key, uint128 total_granted, TvmCell wallet_code, address owner_address, address external_wallet) {}
    function deployEmptyWallet(uint32 _answer_id, uint256 pubkey, address internal_owner, uint128 grams) public functionID(0xc) returns(address value0) {}
    function getWalletAddress(uint256 pubkey, address owner) public functionID(0x11) returns(address value0) {}
 }