/*  */
pragma solidity >= 0.35.0;
pragma AbiHeader expire;
pragma AbiHeader time;
pragma AbiHeader pubkey;

struct OrderRet {
  uint32 err_code;
  uint128 processed;
  uint128 enqueued;
}

interface MMClient {
    function deployPriceWithBuy(TvmCell args_cl) external returns(address value0);
    function deployPriceWithSell(TvmCell args_cl) external returns(address value0);
    function cancelBuyOrder(TvmCell args_cl) external;
    function cancelSellOrder(TvmCell args_cl) external;
}

struct StockTonCfg {
    uint128 transfer_tip3; uint128 return_ownership; uint128 trading_pair_deploy;
    uint128 order_answer; uint128 process_queue; uint128 send_notify;
}

abstract contract AStock {
    function getTradingPairCode() public returns(TvmCell value0) {}
    function getSellPriceCode(address tip3_addr) public returns(TvmCell value0) {}
    function getMinAmount() public returns(uint128 value0) {}
    function getDealsLimit() public returns(uint8 value0) {}
    function getTonsCfg() public returns(uint128 transfer_tip3, uint128 return_ownership, uint128 trading_pair_deploy,
                             uint128 order_answer, uint128 process_queue, uint128 send_notify) {}
}


struct LendOwnership{
   address owner;
   uint128 lend_balance;
   uint32 lend_finish_time;
}

struct Allowance{
    address spender;
    uint128 remainingTokens;
}

struct T3WDetails {
    bytes name;
    bytes symbol;
    uint256 root_public_key;
    uint256 wallet_public_key;
    address root_address;
    address owner_address;
    TvmCell code;
    uint8 decimals;
    uint128 balance;
    LendOwnership lend_ownership;
} 

abstract contract ATip3Wallet {
    function getDetails() public returns (bytes name, bytes symbol, uint8 decimals, uint128 balance, uint256 root_public_key, uint256 wallet_public_key, address root_address, address owner_address, LendOwnership lend_ownership, TvmCell code, Allowance allowance, int8 workchain_id) {}
}

abstract contract AFlexClient {
    constructor(uint256 pubkey, TvmCell trading_pair_code, uint128 transfer_tip3, uint128 return_ownership, uint128 trading_pair_deploy, uint128 order_answer, uint128 process_queue, uint128 send_notify, address flex, address notify_addr) public {}
    function deployPriceWithBuy(TvmCell args_cl) public returns(address value0) {}
    function deployPriceWithSell(TvmCell args_cl) public returns(address value0) {}
    function cancelBuyOrder(TvmCell args_cl) public {}
    function cancelSellOrder(TvmCell args_cl) public {}
}

contract MarketMaker {
    uint128 _stepPrice = 1e5;
    uint128  _countLiquidity = 1e10;
    uint128 _countLiquidityT;
    uint128 _numberOrders = 10;
    uint128[] fibNumber;
    uint128[] sumFib;
    bool _fibWrote = false;
    bool _notified = false;
    address[] _tip3wallets;
    address[] flexClient;
    T3WDetails[] m_tip3walletDetails;    
    modifier alwaysAccept {
            tvm.accept();
            _;
    }
    uint128 _oldPrice = 0;
    StockTonCfg m_stockTonCfg;
    address m_stockaddress;
    uint128 m_stockMinAmount;
    uint8 m_stockDealsLimit;
    TvmCell m_sellPriceCode;
    TvmCell m_tradingPairCode;
    uint128[] pricebuy; 
    uint128[] pricesell;
    uint128[] pricesellnum;
    uint256[] pubkey;
    TvmCell _codeflex;
    TvmCell _dataflex;
    address notify_;
    uint128 rapt = 0;

    constructor() public alwaysAccept {
    }

    function AddPubKey (uint256 value0) external alwaysAccept {
        pubkey.push(value0);
    }

    function AddTip3Wallet (T3WDetails value1, address value2) external alwaysAccept {
        _tip3wallets.push(value2);
        m_tip3walletDetails.push(value1);
    }


    function setCodeflex(TvmCell c) public alwaysAccept {
	    _codeflex = c;
    }

    function setDataflex(TvmCell c) public alwaysAccept {
	    _dataflex = c;
    }

    function DeployFlex() public alwaysAccept {
        TvmCell _contractflex = tvm.buildStateInit(_codeflex, _dataflex);
        for (uint256 i = 0; i < pubkey.length; i++){
            address a1;
	    TvmCell s1;
	    s1 = tvm.insertPubkey(_contractflex, pubkey[i]);
            TvmCell payload = tvm.encodeBody(AFlexClient, pubkey[i], m_tradingPairCode, m_stockTonCfg.transfer_tip3, m_stockTonCfg.return_ownership, m_stockTonCfg.trading_pair_deploy, m_stockTonCfg.order_answer, m_stockTonCfg.process_queue, m_stockTonCfg.send_notify, m_stockaddress, notify_); 
            a1 = tvm.deploy(s1, payload, 100000000000000, 0);
            flexClient.push(a1);
        }
    }

    function DelFlex() public alwaysAccept {
        for (int128 i = int128(flexClient.length) - 1; i >= 0; i--){
            flexClient.pop();
        }
    }
     
    function DelPubKey() public alwaysAccept {
        for (int128 i = int128(pubkey.length) - 1; i >= 0; i--){
            pubkey.pop();
        }
    } 
    function DelTip3() public alwaysAccept {
        for (int128 i = int128(_tip3wallets.length) - 1; i >= 0; i--){
            _tip3wallets.pop();
        }
        for (int128 i = int128(m_tip3walletDetails.length) - 1; i >= 0; i--){
            m_tip3walletDetails.pop();
        }
    } 

    function UpdateInfo(StockTonCfg value0, uint128 value1, uint8 value2, TvmCell value3) external alwaysAccept {
        m_stockTonCfg = value0;
        m_stockMinAmount = value1;
        m_stockDealsLimit = value2;
        m_tradingPairCode = value3; 
    }

    function setPriceCode (TvmCell value0) external alwaysAccept {
        m_sellPriceCode = value0;
    }

    function init(address stockaddress, uint128 step, uint128 number, uint128 liquidity, uint128 liquidityTIP3, address notify) public alwaysAccept {
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
        rapt = 0;
        _stepPrice = step;
        _numberOrders = number;
        _countLiquidity = liquidity;
        _countLiquidityT = liquidityTIP3;
        m_stockaddress = stockaddress;
    }

    function SetLastPrice(address tip3root, uint128 price, uint128 amount) public alwaysAccept {
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
        _oldPrice = price;
        changeOrders();
    }

    function removeOrders() public alwaysAccept {
        if ((int128(pricebuy.length) == 0) && (int128(pricesell.length) == 0)){
            return;
        }
        for (int128 i = int128(pricebuy.length) - 1; i >= 0; i--){
            CancelBuyOrder(pricebuy[uint128(i)]);
            pricebuy.pop();
        }
        for (int128 i = int128(pricesell.length) - 1; i >= 0; i--){
            CancelSellOrder(pricesell[uint128(i)], pricesellnum[uint128(i)]);
            pricesellnum.pop();
            pricesell.pop();
        }
    }

    function setOrders() public alwaysAccept {
        rapt = (rapt + _numberOrders) % (2 * _numberOrders);
        for (uint128 i = _numberOrders; i >= 1; i--){
            uint128 nowPrice = _oldPrice + _stepPrice * i;
            uint128 nowOrder = fibNumber[i - 1] * _countLiquidityT;
            nowOrder /= sumFib[_numberOrders - 1];
            deploySellOrder(nowPrice, nowOrder, i);
            pricesellnum.push(i);
            pricesell.push(nowPrice);
        }
        for (uint128 i = _numberOrders; i >= 1; i--){
            int128 nowPrice_t = int128(_oldPrice) - int128(_stepPrice) * int128(i);
            if (nowPrice_t < 0){
                continue;
            }
            uint128 nowPrice = uint128(nowPrice_t);
            uint128 nowOrder = fibNumber[i - 1] * _countLiquidity;
            nowOrder /= sumFib[_numberOrders - 1];
            nowOrder /= nowPrice;
            deployBuyOrder(nowPrice, nowOrder);
            pricebuy.push(nowPrice);
        }
    }

    function price2Flex(uint128 tokens, uint8 decimals) public returns (uint128) {
        uint128 k = 1;
        for (uint i=0; i<decimals;i++)
        {
            k*=10;
        }
        return uint128(tokens / k);
    }

    function deployBuyOrder(uint128 m_dealTons, uint128 m_dealAmount) public alwaysAccept {
        TvmBuilder bTip3Cfg;
        m_dealTons = price2Flex(m_dealTons, m_tip3walletDetails[0].decimals);
        uint32 m_dealTime =  uint32(100);
        m_dealTime =uint32(now + m_dealTime * 60 * 60);
        bTip3Cfg.store(m_tip3walletDetails[0].name,m_tip3walletDetails[0].symbol,m_tip3walletDetails[0].decimals,m_tip3walletDetails[0].root_public_key,m_tip3walletDetails[0].root_address,m_tip3walletDetails[0].code);
        TvmBuilder bBuyArg;
        uint128 payout = m_dealTons * m_dealAmount + m_stockTonCfg.process_queue + m_stockTonCfg.order_answer + 1000000000;
        bBuyArg.store(m_dealTons,m_dealAmount, m_dealTime, m_stockMinAmount,m_stockDealsLimit,payout,m_sellPriceCode,_tip3wallets[0]);
        bBuyArg.storeRef(bTip3Cfg);
        MMClient(flexClient[0]).deployPriceWithBuy{value: payout, bounce: true, flag: 1}(bBuyArg.toCell());
    }

    
    function getInfo() external returns(T3WDetails) {
        return m_tip3walletDetails[0];
    }

    function getInfo2() external returns(uint128, uint128) {
        return (m_stockMinAmount, m_stockDealsLimit);
    }

    function getInfo3() external returns (TvmCell) {
        return m_sellPriceCode;
    }

    function getFlex(uint128 i) external returns (address) {
        return flexClient[i];
    }


    function deploySellOrder(uint128 m_dealTons, uint128 m_dealAmount, uint128 i) public alwaysAccept {
        uint128 _numberOrders1 =  _numberOrders + rapt;
        TvmBuilder bArgsAddrs;
        bArgsAddrs.store(_tip3wallets[_numberOrders1 - i],flexClient[_numberOrders1 - i]);
        TvmBuilder bTip3Cfg;
        bTip3Cfg.store(m_tip3walletDetails[_numberOrders1 - i].name,m_tip3walletDetails[_numberOrders1 - i].symbol,m_tip3walletDetails[_numberOrders1 - i].decimals,m_tip3walletDetails[_numberOrders1 - i].root_public_key,m_tip3walletDetails[_numberOrders1 - i].root_address,m_tip3walletDetails[_numberOrders1 - i].code);
        m_dealTons = price2Flex(m_dealTons, m_tip3walletDetails[_numberOrders1 - i].decimals);
        uint128 payout = m_stockTonCfg.process_queue + m_stockTonCfg.transfer_tip3 +
        m_stockTonCfg.return_ownership + m_stockTonCfg.order_answer + 1000000000;
        uint32 m_dealTime =  uint32(100);
        m_dealTime =uint32(now + m_dealTime * 60 * 60);
        TvmBuilder bSellArg;
        bSellArg.store(m_dealTons,m_dealAmount,m_dealTime,m_stockMinAmount,m_stockDealsLimit,payout,m_sellPriceCode);
        bSellArg.storeRef(bArgsAddrs);
        bSellArg.storeRef(bTip3Cfg);

        MMClient(flexClient[_numberOrders1 - i]).deployPriceWithSell{value: 3.1 ton, bounce: true, flag: 1}(bSellArg.toCell());
    }

    function CancelBuyOrder(uint128 m_dealTons) public alwaysAccept {
        TvmBuilder bTip3Cfg;
        bTip3Cfg.store(m_tip3walletDetails[0].name,m_tip3walletDetails[0].symbol,m_tip3walletDetails[0].decimals,m_tip3walletDetails[0].root_public_key,m_tip3walletDetails[0].root_address,m_tip3walletDetails[0].code);
        TvmBuilder bCancelArg;
        m_dealTons = price2Flex(m_dealTons, m_tip3walletDetails[0].decimals);
        uint128 payout = 1000000000;
        bCancelArg.store(m_dealTons,m_stockMinAmount,m_stockDealsLimit,payout,m_sellPriceCode);
        bCancelArg.storeRef(bTip3Cfg);
        MMClient(flexClient[0]).cancelBuyOrder{value: 3 ton, bounce: true, flag: 1}(bCancelArg.toCell());
    }

    function CancelSellOrder(uint128 m_dealTons, uint128 i) public alwaysAccept {
        uint128 _numberOrders1 =  _numberOrders + rapt; 
        TvmBuilder bArgsAddrs;
        bArgsAddrs.store(_tip3wallets[_numberOrders1 - i],flexClient[_numberOrders1 - i]);
        TvmBuilder bTip3Cfg;        
        m_dealTons = price2Flex(m_dealTons, m_tip3walletDetails[0].decimals);
        bTip3Cfg.store(m_tip3walletDetails[_numberOrders1 - i].name,m_tip3walletDetails[_numberOrders1 - i].symbol,m_tip3walletDetails[_numberOrders1 - i].decimals,m_tip3walletDetails[_numberOrders1 - i].root_public_key,m_tip3walletDetails[_numberOrders1 - i].root_address,m_tip3walletDetails[_numberOrders1 - i].code);

        TvmBuilder bCancelArg;
        uint128 payout = 1000000000;
        bCancelArg.store(m_dealTons,m_stockMinAmount,m_stockDealsLimit,payout,m_sellPriceCode);
        bCancelArg.storeRef(bTip3Cfg);

        MMClient(flexClient[_numberOrders1 - i]).cancelSellOrder{value: 3.2 ton, bounce: true, flag: 1}(bCancelArg.toCell());
    }

    function changeOrders() public alwaysAccept {
        removeOrders();
        setOrders();
    }

    /* Setters */

    function setStep(uint128 step) public alwaysAccept {
        if (_stepPrice != step){
            removeOrders();
            _stepPrice = step;
            setOrders();
        }
    }

    function setNumber(uint128 number) public alwaysAccept {
        if (number > 20){
            number = 20;
        }
        if (number < 0){
            number = 0;
        }
        if (_numberOrders != number){
            removeOrders();
            _numberOrders = number;
            setOrders();
        }
    }

    function setLiquidity(uint128 liquidity) public alwaysAccept {
        if (_countLiquidity != liquidity){
            removeOrders();
            _countLiquidity = liquidity;
            setOrders();
        }
    }

    function setLiquidityTIP3(uint128 liquidityTIP3) public alwaysAccept {
        if (_countLiquidity != liquidityTIP3){
            removeOrders();
            _countLiquidityT = liquidityTIP3;
            setOrders();
        }
    }

    /* fallback/receive */
    receive() external {
    }
}

