/*  */
pragma ton-solidity ^0.47.0;
pragma AbiHeader expire;
pragma AbiHeader time;
pragma AbiHeader pubkey;

import "Upgradable.sol";

interface MarketMaker {
    function ReturnMoney (uint128 count, address tip3) external functionID(0xffa);
    function AskData() external functionID(0xffb);
    function GetLastPrice() external functionID(0xffc) ;
}

abstract contract ATip3Wallet {
    function transferWithNotify(uint32 _answer_id, address answer_addr, address to, 
                                uint128 tokens, uint128 grams, bool return_ownership, 
                                TvmCell payload) public functionID(0xb) {}
}

abstract contract TonTokenWallet {
    function requestBalance() external responsible functionID(0xe) returns(uint128) {}
}

contract Pool is Upgradable{
    bool _period;
    bool _prolongation = false;
    uint128 _stake;
    uint256 _pubkey;
    address _notify;
    address _MM;
    address _tip3wallet;
    address _tip3amm;
    uint128 _count;
    uint128 _price;
    modifier alwaysAccept {
            tvm.accept();
            _;
    }
    
    constructor(uint256 value0, address notify, address MM, address tip3wallet, address tip3amm) public alwaysAccept {
        _pubkey = value0;
        _notify = notify;
        _tip3wallet = tip3wallet;
        _tip3amm = tip3amm;
        _MM = MM;
    }

    function AskMoney() public view functionID(0xf) alwaysAccept {
        if (msg.sender == _notify) {
            _notify.transfer(1e10, false, 3);
        }
    }
    
    function StartPeriod() public functionID(0xfc) alwaysAccept {
        if (msg.sender == _notify) {
                _period = true;
        }
    }
    
    function EndPeriod() public functionID(0xfb) alwaysAccept {
        if (msg.sender == _notify) {
            _period = false;
            MarketMaker(_MM).AskData();
        }
    } 
    
    function writeData(uint128 balancestart, uint128 balanceend) public functionID(0xffa) alwaysAccept {
        uint128 part = _stake * 100 / balancestart;
        _stake = _stake + part * (balanceend - balancestart);
        if (_prolongation == false) { ReturnMoney(); }
    }
    
    function ReturnMoney () public alwaysAccept functionID(0xffc) {
        MarketMaker(_MM).ReturnMoney{value: 0.1 ton}(_stake, _tip3wallet);
        _stake = 0;
    }
    
    function AddStake(uint128 count) public alwaysAccept functionID(0xffd) {
        if (_stake != 0) { return; }
        _count = count;
        MarketMaker(_MM).GetLastPrice{value: 0.1 ton}();
    }
    
    function writePrice(uint128 price) public alwaysAccept functionID(0xffb) {
        _price = price;
        TonTokenWallet(_tip3wallet).requestBalance{value: 0.1 ton, callback: Pool.tryStake}();
    }
    
    function tryStake(uint128 balance) public functionID(0xffe) alwaysAccept {
        uint128 value = balance * _price;
        if (value > address(this).balance * 95 / 100) {
            value = address(this).balance * 95 / 100;
        }
        value *= 2;
        _MM.transfer(value / 2, false, 3);
        uint32 _answer_id = 0;
        uint256 pubkey = 0;
        uint128 grams = 1.5 ton;
        uint128 grams_payload =1 ton;
        TvmBuilder bFLeXDeployWalletArgs;
        bool bFalse = false;
        bFLeXDeployWalletArgs.store(pubkey,_tip3wallet,grams_payload);
        ATip3Wallet(_tip3wallet).transferWithNotify(_answer_id , _tip3wallet, _tip3amm, value / 2 / _price, grams, bFalse, bFLeXDeployWalletArgs.toCell());
        _stake = value;
    }    
        
    function setProlongation(bool prol) public functionID(0xb) alwaysAccept {
        _prolongation = prol;
    } 
    
    function sendMoney(uint128 count, address a1) public view alwaysAccept functionID(0xfff) {
        require(msg.pubkey() == tvm.pubkey(), 101);
        if (count < address(this).balance) { a1.transfer(count, false, 3); }
    }

    /* Setters */
    
    function onCodeUpgrade() internal override {
        tvm.resetStorage();
    }

    /* fallback/receive */
    receive() external {
    }
}

