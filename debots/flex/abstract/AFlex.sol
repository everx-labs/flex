pragma ton-solidity >=0.47.0;

abstract contract AFlex {
    function getTonsCfg() public functionID(0x11)
        returns(
            uint128 transfer_tip3, uint128 return_ownership, uint128 trading_pair_deploy,
            uint128 order_answer, uint128 process_queue, uint128 send_notify) {}
    function getTradingPairCode() public functionID(0x12) returns(TvmCell value0) {}
    function getXchgPairCode() public functionID(0x13) returns(TvmCell value0) {}
    function getSellPriceCode(address tip3_addr) public functionID(0x14) returns(address value0) {}
    function getXchgPriceCode(address tip3_addr1, address tip3_addr2) public functionID(0x15) returns(address value0) {}
    function getDealsLimit() public functionID(0x18) returns(uint8 value0) {}
}