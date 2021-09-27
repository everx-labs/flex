/*  */
pragma ton-solidity ^0.47.0;
import "Upgradable.sol";

interface SNotify {
    function end1(uint128 counter) external functionID(0xc);
    function end2(uint128 counter) external functionID(0xd);
    function start1(uint128 counter) external functionID(0xe);
    function start2(uint128 counter) external functionID(0xf);
    function tryNotify1(uint128 counter) external functionID(0xfa);
}

contract ProxyNotify is Upgradable{
    uint128 ver = 3;
    address _Notify;
    
    modifier alwaysAccept {
            tvm.accept();
            _;
    }

    constructor() public alwaysAccept {
    }

    function EndPeriod1(uint128 counter) view external functionID(0xc) {
        SNotify(_Notify).end1{value: 0.1 ton}(counter + 1);
    }

    function EndPeriod2(uint128 counter) view external functionID(0xd){
        SNotify(_Notify).end2{value: 0.1 ton}(counter + 1);
    }
    
    function StartPeriod1(uint128 counter) view external functionID(0xe) {
        SNotify(_Notify).start1{value: 0.1 ton}(counter + 1);
    }
    
    function StartPeriod2(uint128 counter) view external functionID(0xf) {
        SNotify(_Notify).start2{value: 0.1 ton}(counter + 1);
    }
    
    function tryN (uint128 counter) view external functionID(0xfa){
        SNotify(_Notify).tryNotify1{value: 0.1 ton}(counter + 1);
    }        
    
    function setNotify(address notify) public functionID(0xfb) alwaysAccept {
        _Notify = notify;
    }


    /* fallback/receive */
    receive() external {
    }
    
    function onCodeUpgrade() internal override {
        tvm.resetStorage();
    }
}
