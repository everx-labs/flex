pragma ton-solidity >=0.47.0;

struct Order {
    uint128 value;
    uint128 timestamp;
    uint128 returnToken;
}

abstract contract AUserToken {
    constructor() public {}
    function getAirdrop() public functionID(0xaa) {}
    function getStake() external view functionID(0xd) returns (Order[] value0) {}
    function getBalance() external view functionID(0xdd) returns (uint128 value0, uint128 value1, uint128 value2, uint128 value3) {}
    function update() external  functionID(0xccc) {}
    function sendMoney(uint128 count, address dest) external functionID(0x11b) {}
}
