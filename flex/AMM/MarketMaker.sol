/*  */
pragma ton-solidity ^0.47.0;
pragma AbiHeader expire;
pragma AbiHeader time;
pragma AbiHeader pubkey;

import "Upgradable.sol";

interface MMClient {
    function deployPriceWithBuy(uint128 price, 
      uint128 amount, 
      uint32 order_finish_time, 
      uint128 min_amount, 
      uint8 deals_limit, 
      uint128 deploy_value, 
      TvmCell price_code, 
      address my_tip3_addr, 
      T3WDetails tip3cfg, 
      address notify_addr) external functionID(0x12) returns(address value0);
    function deployPriceWithSell(uint128 price,
      uint128 amount,
      uint32  lend_finish_time,
      uint128 min_amount,
      uint8   deals_limit,
      uint128 tons_value,
      TvmCell    price_code,
      address my_tip3_addr,
      address receive_wallet,
      T3WDetails tip3cfg,
      address notify_addr) external functionID(0x11) returns(address value0);
    function cancelBuyOrder(uint128 price,
      uint128 min_amount,
      uint8   deals_limit,
      uint128 value,
      TvmCell    price_code,
      T3WDetails tip3cfg,
      address notify_addr) external functionID(0x15);
    function cancelSellOrder(uint128 price,
      uint128 min_amount,
      uint8   deals_limit,
      uint128 value,
      TvmCell    price_code,
      T3WDetails tip3cfg,
      address notify_addr) external functionID(0x14);
}

interface LiquidityPool {
    function writeData(uint128 _start, uint128 _end) external functionID(0xffa);
    function writePrice(uint128 price) external  functionID(0xffb);
}

struct StockTonCfg {
    uint128 transfer_tip3; uint128 return_ownership; uint128 trading_pair_deploy;
    uint128 order_answer; uint128 process_queue; uint128 send_notify;
}

abstract contract TonTokenWallet {
    function requestBalance(uint32 _answer_id) external returns(uint128) {}
}

struct T3WDetails {
    bytes name;
    bytes symbol;
    uint8 decimals;
    uint256 root_public_key;
    address root_address;
} 

abstract contract ATip3Wallet {
    function transferWithNotify(uint32 _answer_id, address answer_addr, address to, 
                                uint128 tokens, uint128 grams, bool return_ownership, 
                                TvmCell payload) public functionID(0xb) {}
}

abstract contract AFlexClient {
    constructor(uint256 pubkey, TvmCell trading_pair_code, TvmCell xchg_pair_code) public functionID(0xa) {}
    function setFlexCfg(StockTonCfg tons_cfg, address flex, address notify_addr) public functionID(0xb) {}
    function setFlexWalletCode(TvmCell flex_wallet_code) public functionID(0xd) {}
}

contract MarketMaker  is Upgradable{
    uint128 _stepPrice = 1e5;
    uint128 _countLiquidityT;
    uint128 _numberOrders = 10;
    uint128[] fibNumber;
    uint128[] sumFib;
    bool _fibWrote = false;
    address _tip3wallet;
    address flexClient;
    T3WDetails  m_tip3walletDetails;    
    uint128 _oldPrice = 0;
    StockTonCfg m_stockTonCfg;
    address m_stockaddress;
    uint128 m_stockMinAmount;
    uint8 m_stockDealsLimit;
    TvmCell m_sellPriceCode;
    TvmCell m_tradingPairCode;
    TvmCell m_xchg_pair_code;
    uint128[] pricebuy; 
    uint128[] pricesell;
    uint256 pubkey;
    TvmCell _codeflex;
    TvmCell _dataflex;
    TvmCell _codepool;
    TvmCell _datapool;
    TvmCell _codeW;
    address notify_;
    uint128 _balance;
    address _pool;
    uint128 _balancestart;
    uint128 _balanceend;
    
    modifier alwaysAccept {
            tvm.accept();
            _;
    }
    
    constructor(uint256 value0) public alwaysAccept {
        pubkey = value0;
    }

    function AddTip3Wallet (T3WDetails value1, address value2) external functionID(0x17) alwaysAccept {
        _tip3wallet = value2;
        m_tip3walletDetails = value1;
    }

    function DeployFlex() external functionID(0xbd) alwaysAccept {
        TvmCell _contractflex = tvm.buildStateInit(_codeflex, _dataflex);
        address a1;
	      TvmCell s1;
	      s1 = tvm.insertPubkey(_contractflex, pubkey);
        TvmCell payload = tvm.encodeBody(AFlexClient, pubkey, m_tradingPairCode, m_xchg_pair_code); 
        a1 = tvm.deploy(s1, payload, 10000000000, 0);
        flexClient = a1;
        AFlexClient(flexClient).setFlexCfg{value: 0.1 ton, bounce: true, flag: 1}(m_stockTonCfg, m_stockaddress, notify_);
        AFlexClient(flexClient).setFlexWalletCode{value: 0.1 ton, bounce: true, flag: 1}(_codeW);      
    }

    function UpdateInfo(StockTonCfg value0, uint128 value1, uint8 value2) external functionID(0xb) alwaysAccept {
        m_stockTonCfg = value0;
        m_stockMinAmount = value1;
        m_stockDealsLimit = value2;
    }
    
    function UpdateInfo1(TvmCell value3) external functionID(0xbb) alwaysAccept {
        m_tradingPairCode = value3; 
    }
    
    function UpdateInfo2(TvmCell value4) external functionID(0xbbb) alwaysAccept {
        m_xchg_pair_code = value4;
    }

    function init(address stockaddress, uint128 step, uint128 number, address notify) external  functionID(0xd) alwaysAccept {
        notify_ = notify;
        if (_fibWrote == false){
            uint128 sum = 2;
            fibNumber.push(1);
            fibNumber.push(1);
            sumFib.push(1);
            sumFib.push(2);
            for (uint128 i = 2; i < 20; i++){
                fibNumber.push(fibNumber[i - 1] + fibNumber[i - 2]);
                sum += fibNumber[i];
                sumFib.push(sum);
            }
            _fibWrote = true;
        }
        _stepPrice = step;
        _numberOrders = number;
        m_stockaddress = stockaddress;
    }

    function SetLastPrice(uint128 price) public functionID(0xfa) alwaysAccept {
        _oldPrice = price;
        if (pricebuy.length > 0){
            if (price < pricebuy[pricebuy.length - 1]){
                return;
            }
        }
        if (pricesell.length > 0){
            if (price > pricesell[pricesell.length - 1]){
                return;
            }
        }
        changeOrders();
    }
    
    function GetLastPrice() public view alwaysAccept functionID(0xffc)  {
        LiquidityPool(msg.sender).writePrice{value: 0.1 ton, bounce: true, flag: 1}(_oldPrice);
    }

    function removeOrders() public alwaysAccept {
        for (int128 i = int128(pricebuy.length) - 1; i >= 0; i--){
            CancelBuyOrder(pricebuy[uint128(i)]);
            pricebuy.pop();
        }
        for (int128 i = int128(pricesell.length) - 1; i >= 0; i--){
            CancelSellOrder(pricesell[uint128(i)]);
            pricesell.pop();
        }
        uint32 answer = 26;
        TonTokenWallet(_tip3wallet).requestBalance{value: 0.1 ton}(answer);
    }

    function setOrders(uint128 value) public alwaysAccept functionID(0x1a){
        _countLiquidityT = value; 
        _countLiquidityT *= 45;
        _countLiquidityT /= 100; 
        for (uint128 i = _numberOrders; i >= 1; i--){
            uint128 nowPrice = _oldPrice + _stepPrice * i;
            uint128 nowOrder = fibNumber[i - 1] * _countLiquidityT;
            nowOrder /= sumFib[_numberOrders - 1];
            deploySellOrder(nowPrice, nowOrder);
            pricesell.push(nowPrice);
        }
        for (uint128 i = _numberOrders; i >= 1; i--){
            int128 nowPrice_t = int128(_oldPrice) - int128(_stepPrice) * int128(i);
            if (nowPrice_t < 0){
                continue;
            }
            uint128 nowPrice = uint128(nowPrice_t);
            uint128 nowOrder = fibNumber[i - 1] * (address(this).balance) * 45 / 100;
            nowOrder /= sumFib[_numberOrders - 1];
            nowOrder /= nowPrice;
            deployBuyOrder(nowPrice, nowOrder);
            pricebuy.push(nowPrice);
        }
    }

    function price2Flex(uint128 tokens, uint8 decimals) public pure returns (uint128) {
        uint128 k = 1;
        for (uint i=0; i<decimals;i++)
        {
            k*=10;
        }
        return uint128(tokens / k);
    }

    function deployBuyOrder(uint128 m_dealTons, uint128 m_dealAmount) public view alwaysAccept {
        m_dealTons = price2Flex(m_dealTons, m_tip3walletDetails.decimals);
        uint32 m_dealTime =  uint32(100);
        m_dealTime =uint32(now + m_dealTime * 60 * 60);
        uint128 payout = m_dealTons * m_dealAmount + m_stockTonCfg.process_queue + m_stockTonCfg.order_answer + 1000000000;
        MMClient(flexClient).deployPriceWithBuy{value: payout, bounce: true, flag: 1}(m_dealTons,m_dealAmount, m_dealTime,  m_stockMinAmount,m_stockDealsLimit,payout,m_sellPriceCode, _tip3wallet, m_tip3walletDetails, notify_);
    }

    function deploySellOrder(uint128 m_dealTons, uint128 m_dealAmount) public view alwaysAccept {
        m_dealTons = price2Flex(m_dealTons, m_tip3walletDetails.decimals);
        uint128 payout = m_stockTonCfg.process_queue + m_stockTonCfg.transfer_tip3 +
        m_stockTonCfg.return_ownership + m_stockTonCfg.order_answer + 1000000000;
        uint32 m_dealTime =  uint32(100);
        m_dealTime =uint32(now + m_dealTime * 60 * 60);
        MMClient(flexClient).deployPriceWithSell{value: 3.1 ton, bounce: true, flag: 1}(m_dealTons,m_dealAmount,m_dealTime,m_stockMinAmount,m_stockDealsLimit,payout,m_sellPriceCode, _tip3wallet, _tip3wallet, m_tip3walletDetails, notify_);
    }

    function CancelBuyOrder(uint128 m_dealTons) public view alwaysAccept {
        m_dealTons = price2Flex(m_dealTons, m_tip3walletDetails.decimals);
        uint128 payout = 1000000000;
        MMClient(flexClient).cancelBuyOrder{value: 3 ton, bounce: true, flag: 1}(m_dealTons, m_stockMinAmount,m_stockDealsLimit, payout, m_sellPriceCode, m_tip3walletDetails, notify_);
    }

    function CancelSellOrder(uint128 m_dealTons) public view alwaysAccept {      
        m_dealTons = price2Flex(m_dealTons, m_tip3walletDetails.decimals);
        uint128 payout = 1000000000;
        MMClient(flexClient).cancelSellOrder{value: 3.2 ton, bounce: true, flag: 1}(m_dealTons, m_stockMinAmount,m_stockDealsLimit, payout, m_sellPriceCode, m_tip3walletDetails, notify_);
    }

    function changeOrders() public alwaysAccept {
        removeOrders();
    }
       
    function AskMoney() public view functionID(0xf) alwaysAccept {
        if (msg.sender == notify_) {
            notify_.transfer(1e10, false, 3);
        }
    }
    
    function StartPeriod() public view alwaysAccept functionID(0xfc) {
        uint32 answer = 65530;
        TonTokenWallet(_tip3wallet).requestBalance{value: 0.1 ton}(answer);
    }
    
    function Periodon(uint128 value) public functionID(0xfffa) alwaysAccept {
        _balancestart = address(this).balance + value * _oldPrice;
    }
    
    function EndPeriod() public view alwaysAccept functionID(0xfb) {
        uint32 answer = 65531;
        TonTokenWallet(_tip3wallet).requestBalance{value: 0.1 ton}(answer);
    } 
    
    function Periodoff(uint128 value) public functionID(0xfffb) alwaysAccept {
        _balanceend = address(this).balance + value * _oldPrice;
    }
    
    function ReturnMoney (uint128 count, address tip3) public view alwaysAccept functionID(0xffa)  {
        TvmCell _contractPool = tvm.buildStateInit(_codepool, _datapool);
        TvmCell s1 = tvm.insertPubkey(_contractPool, msg.pubkey());
        address a1 = address.makeAddrStd(0, tvm.hash(s1));
        if (a1 != msg.sender){ return; }
        a1.transfer(count / 2, false, 3);
        uint32 _answer_id = 0;
        uint128 grams = 1.5 ton;
        uint128 grams_payload = 1 ton;
        TvmBuilder bFLeXDeployWalletArgs;
        bool bFalse = false;
        bFLeXDeployWalletArgs.store(pubkey,_tip3wallet,grams_payload);
        ATip3Wallet(_tip3wallet).transferWithNotify(_answer_id , _tip3wallet, tip3, count / 2 / _oldPrice, grams, bFalse, bFLeXDeployWalletArgs.toCell());
    }
    
    function AskData() public view alwaysAccept functionID(0xffb)  {
        LiquidityPool(msg.sender).writeData{value: 0.1 ton, bounce: true, flag: 1}(_balancestart, _balanceend);
    }
    
    function sendMoney(uint128 count, address a1) public view alwaysAccept {
        require(msg.pubkey() == tvm.pubkey(), 101);
        if (count < address(this).balance) { a1.transfer(count, false, 3); }
    }

    /* Setters */
    
    function setCodeflex(TvmCell c) external  functionID(0xe) alwaysAccept {
        _codeflex = c;
    }

    function setDataflex(TvmCell c) external  functionID(0x11) alwaysAccept {
	      _dataflex = c;
    }
    
    function setCodeW(TvmCell c) external  functionID(0x12) alwaysAccept {
        _codeW = c;
    }
    
    function setCodepool(TvmCell c) external  functionID(0x13) alwaysAccept {
        _codepool = c;
    }

    function setDatapool(TvmCell c) external  functionID(0x14) alwaysAccept {
	      _datapool = c;
    }
    
    function setPriceCode (TvmCell value0) external functionID(0x15) alwaysAccept {
        m_sellPriceCode = value0;
    }

    function setStep(uint128 step) external alwaysAccept {
        if (_stepPrice != step){
            _stepPrice = step;
            removeOrders();
        }
    }

    function setNumber(uint128 number) external alwaysAccept {
        if (number > 10){
            number = 10;
        }
        if (number < 0){
            number = 0;
        }
        if (_numberOrders != number){
            _numberOrders = number;
            removeOrders();
        }
    }
    
    function onCodeUpgrade() internal override {
        
    }

    /* fallback/receive */
    receive() external {
    }
    
    /* Getters */   
    function getT3WDetails() external view functionID(0xbbba) returns(T3WDetails) {
        return m_tip3walletDetails;
    }

    function getStockDetails() external view functionID(0xbbbb) returns(uint128, uint8)    {
        return (m_stockMinAmount, m_stockDealsLimit);
    }

    function getSellPriceCode() external view functionID(0xbbbc) returns (TvmCell) {
        return m_sellPriceCode;
    }
    
    function lastPrice() public view functionID(0xbbbe) returns (uint128) {
        return _oldPrice;
    }

    function getFlex() external view functionID(0xbbbd) returns (address) {
        return flexClient;
    }
}

