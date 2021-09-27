/*  */
pragma ton-solidity ^0.47.0;
pragma AbiHeader expire;
pragma AbiHeader pubkey;
pragma AbiHeader time;
import "ProxyNotify.sol";
import "Upgradable.sol";


interface Notify {
    function AskMoney() external functionID(0xf);
    function SetLastPrice(uint128 price) external functionID(0xfa) ;
    function EndPeriod() external functionID(0xfb) ;
    function StartPeriod() external functionID(0xfc) ;
}

contract StockNotify is Upgradable {
    bool period = false;
    uint128 _lastPrice;
    uint128 _lastAmount;
    address _lastTip3Root;
    address _Stock;
    address _proxy;
    mapping(address => uint256) _SubsFull; 
    mapping(address => uint256) _Subs; 
    uint128[] _SFull;
    address[] _SFulladdr;
    uint128[] _S;
    address[] _Saddr;
    modifier alwaysAccept {
            tvm.accept();
            _;
    }

    constructor(address Stock, address proxy) public alwaysAccept {
        _Stock = Stock;
        _proxy = proxy;
    }

    function setData (address Stock, address proxy) public functionID(0xbaaf) alwaysAccept {
        _Stock = Stock;
        _proxy = proxy;
    }
    
    function addSub(address addr) public functionID(0xbaa) alwaysAccept{
        if (_SubsFull.exists(addr) == false) { _SubsFull[addr] = _SFull.length; _SFull.push(20); _SFulladdr.push(addr); }
    }
    
    function addsmallSub(address addr) public functionID(0xbab) alwaysAccept{
        if (_Subs.exists(addr) == false) { _Subs[addr] = _S.length; _S.push(20); _Saddr.push(addr); }
    }
    
    function endPeriod() external functionID(0xbadc) {
        end1(0);
    }
    
    function end1(uint128 i) public functionID(0xc) alwaysAccept {
        uint128 z = 0;
        for (uint128 j = i; j < _SFull.length; j++){
            if (_SFull[j] <= 10) {
                Notify(_SFulladdr[j]).AskMoney{value: 0.1 ton}();
            } 
            if (_SFull[j] > 0){
                _SFull[j]--;
                Notify(_SFulladdr[j]).EndPeriod{value: 0.1 ton}();
            }
            z++;
            if (z == 5) { ProxyNotify(_proxy).EndPeriod1{value: 0.1 ton}(j); return; } 
        }
        end2(0);
    }
    
    function end2 (uint128 i) public functionID(0xd) alwaysAccept {
        uint128 z = 0;
        for (uint128 j = i; j < _S.length; j++){
            Notify(_Saddr[j]).EndPeriod{value: 0.1 ton}();
            if (_S[j] <= 10) {
                Notify(_Saddr[j]).AskMoney{value: 0.1 ton}();
            } 
            if (_S[j] > 0){
                _S[j]--;
                Notify(_Saddr[j]).EndPeriod{value: 0.1 ton}();
            } 
            z++;
            if (z == 5) { ProxyNotify(_proxy).EndPeriod2{value: 0.1 ton}(j); return; } 
        }
    }
    
    function startPeriod() external functionID(0xdadc) {
        start1(0);
    }
    
    function start1(uint128 i) public functionID(0xe) alwaysAccept {
        uint128 z = 0;
        for (uint128 j = i; j < _SFull.length; j++){
            if (_SFull[j] <= 10) {
                Notify(_SFulladdr[j]).AskMoney{value: 0.1 ton}();
            } 
            if (_SFull[j] > 0){
                _SFull[j]--;
                Notify(_SFulladdr[j]).StartPeriod{value: 0.1 ton}();
            }
            z++;
            if (z == 10) { ProxyNotify(_proxy).StartPeriod1{value: 0.1 ton}(j); return; } 
        }
        start2(0);
    }
    
    function start2(uint128 i) public functionID(0xf) alwaysAccept {
        uint128 z = 0;
        for (uint128 j = i; j < _S.length; j++){
            if (_S[j] <= 10) {
                Notify(_Saddr[j]).AskMoney{value: 0.1 ton}();
            } 
            if (_S[j] > 0){
                _S[j]--;
                Notify(_Saddr[j]).StartPeriod{value: 0.1 ton}();
            }
            z++;
            if (z == 10) { ProxyNotify(_proxy).StartPeriod2{value: 0.1 ton}(j); return; } 
        }
    }        
    
    function onDealCompleted(address tip3root, uint128 price, uint128 amount) external functionID(0xa) {
         _lastPrice = price;
         _lastAmount = amount;
         _lastTip3Root = tip3root;
         tryNotify1(0);
    }    
    
    function tryNotify1(uint128 i) public functionID(0xfa) alwaysAccept {
        uint128 z = 0;
        for (uint128 j = i; j < _SFull.length; j++){
            if (_SFull[j] <= 10) {
                Notify(_SFulladdr[j]).AskMoney{value: 0.1 ton}();
            } 
            if (_SFull[j] > 0){
                _SFull[j]--;
                Notify(_SFulladdr[j]).SetLastPrice{value: 0.1 ton}(_lastPrice);
            }
            z++;
            if (z == 10) { ProxyNotify(_proxy).tryN{value: 0.1 ton}(j); return; } 
        }
    }

    /* fallback/receive */
    receive() external {
        if ((_SubsFull.exists(msg.sender)) && (msg.value >= 1)){
            _SFull[_SubsFull[msg.sender]] += msg.value * 10 - 1;
        }
    }
    
    function getInfo(address check) external functionID(0xbae) view returns(uint128) {
        return _S[_Subs[check]];
    }
    
    function getInfoFull(address check) external functionID(0xbaf) view returns(uint128) {
        return _SFull[_SubsFull[check]];
    }
    
    function onCodeUpgrade() internal override {
        tvm.resetStorage();
    }
}

