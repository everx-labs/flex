pragma ton-solidity >=0.47.0;

abstract contract AXchgPair {
    function getFlexAddr() public functionID(0xb) returns(address value0) {}
    function getTip3MajorRoot() public functionID(0xc) returns(address value0) {}
    function getTip3MinorRoot() public functionID(0xd) returns(address value0) {}
    function getMinAmount() public functionID(0xe) returns(uint128 value0) {}
    function getMinmove() public functionID(0xf) returns(address value0) {}
    function getPriceDenum() public functionID(0x10) returns(uint128 value0) {}
    function getNotifyAddr() public functionID(0x11) returns(address value0) {}
    //function getXchgPriceCode() public functionID(0x14) returns(TvmCell value0) {}
    function getPriceXchgCode(bool salted) public functionID(0x15) returns(TvmCell value0) {}
    function getPriceXchgSalt() public functionID(0x16) returns(TvmCell value0) {}
}
