/*  */
pragma solidity >= 0.6.0;

interface Notify {
    function SetLastPrice(address tip3root, uint128 price, uint128 amount) external;
}

contract StockNotify  {
    uint128 _lastPrice;
    uint128 _lastAmount;
    address _lastTip3Root;
    bool _ready = false;
    address _Notify;
    modifier alwaysAccept {
            tvm.accept();
            _;
    }

    constructor(address notify) public alwaysAccept {
        _Notify = notify;
    }

    function onDealCompleted(address tip3root, uint128 price, uint128 amount) external {
         _lastPrice = price;
         _lastAmount = amount;
         _lastTip3Root = tip3root;
         _ready = true;
         tryNotify();
    }

    function tryNotify() public alwaysAccept {
        if (_ready == true) {
            Notify(_Notify).SetLastPrice{value: 0.1 ton}(_lastTip3Root, _lastPrice, _lastAmount);
        }
    }

    function setNotify(address notify) public alwaysAccept {
        _Notify = notify;

    }


    /* fallback/receive */
    receive() external {
    }
}

