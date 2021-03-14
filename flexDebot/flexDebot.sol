pragma ton-solidity >=0.35.0;
pragma AbiHeader expire;
pragma AbiHeader time;
pragma AbiHeader pubkey;
// import required DeBot interfaces and basic DeBot contract.
import "../interfaces/Debot.sol";
import "../interfaces/Upgradable.sol";
import "../interfaces/Sdk.sol";
import "../interfaces/Terminal.sol";
import "../interfaces/Menu.sol";
import "../interfaces/ConfirmInput.sol";
import "../interfaces/Msg.sol";
import "../interfaces/AddressInput.sol";
import "../interfaces/AmountInput.sol";

struct StockTonCfg {
    uint128 transfer_tip3; uint128 return_ownership; uint128 trading_pair_deploy;
    uint128 order_answer; uint128 process_queue; uint128 send_notify;
}


abstract contract AMSig {
    function submitTransaction(
        address dest,
        uint128 value,
        bool bounce,
        bool allBalance,
        TvmCell payload)
    public returns (uint64 transId) {}
}

abstract contract AStock {
    function getTradingPairCode() public returns(TvmCell value0) {}
    function getSellPriceCode(address tip3_addr) public returns(TvmCell value0) {}
    function getMinAmount() public returns(uint128 value0) {}
    function getDealsLimit() public returns(uint8 value0) {}
    function getTonsCfg() public returns(uint128 transfer_tip3, uint128 return_ownership, uint128 trading_pair_deploy,
                             uint128 order_answer, uint128 process_queue, uint128 send_notify) {}
}

abstract contract ATradingPair {
    function getTip3Root() public returns(address value0) {}
    function getStockAddr() public returns(address value0) {}
}

abstract contract ATip3Root {
    function getSymbol() public returns(bytes value0){}
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
    uint8 decimals;
    uint128 balance;
    LendOwnership lend_ownership;
}

abstract contract ATip3Wallet {
    function getDetails() public returns (bytes name, bytes symbol, uint8 decimals, uint128 balance, uint256 root_public_key, uint256 wallet_public_key, address root_address, address owner_address, LendOwnership lend_ownership, TvmCell code, Allowance allowance, int8 workchain_id) {}
}

abstract contract AFlexClient {
    constructor(uint256 pubkey, TvmCell trading_pair_code) public {}
    function deployPriceWithBuy(TvmCell args_cl) public returns(address value0) {}
    function deployPriceWithSell(TvmCell args_cl) public returns(address value0) {}
    function cancelBuyOrder(TvmCell args_cl) public {}
    function cancelSellOrder(TvmCell args_cl) public {}
}

interface IFlexHelperDebot {
    function getFCAddressAndKeys(address stock) external;
    function getTip3WalletAddress(address tip3root, address flexClient) external;
    function deployTradigPair(address stock, address fclient, uint128 tpd, uint256 pk, uint256 sk) external;
}

struct PriceInfo {
       uint128 sell;
       uint128 buy;
       address addr;
    }
    
struct Tip3MenuInfo{
        address tp;
        address t3r;
        string symbol;
    }

contract FlexDebot is Debot, Upgradable {   

    address m_flexHelperDebot;
    address m_stockAddr = address(0x16e1ef2dbd1c1fa8efa696ff812d319e4577bd24ac5bd9512a0c841288c12443);
    address m_tip3root;
    address m_tpa;
    address[] m_tpaddrs;
    uint m_curTP;
    Tip3MenuInfo[] m_tip3Menu;
    uint256 m_masterPubKey;
    uint256 m_masterSecKey;
    TvmCell m_sendMsg;
    TvmCell m_tradingPairCode;
    TvmCell m_sellPriceCode;
    address m_flexClient;
    address m_tip3wallet;
    T3WDetails m_tip3walletDetails;
    uint128 m_dealTons;
    uint128 m_dealAmount;
    uint32 m_dealTime;
    uint128 m_stockMinAmount;
    uint8 m_stockDealsLimit;
    StockTonCfg m_stockTonCfg;
    uint32 m_getT3WDetailsCallback;
    mapping(uint128 => PriceInfo) m_prices;
    uint128[] m_arPrices;


    function setStockAddr(address addr) public {
        require(msg.pubkey() == tvm.pubkey(), 101);
        tvm.accept();
        m_stockAddr = addr;
    }

    function setFlexHelperDebot(address addr) public {
        require(msg.pubkey() == tvm.pubkey(), 101);
        tvm.accept();
        m_flexHelperDebot = addr;
    }

    function start() public override {
        getStockTonsCfg();
    }

    function splitTokens(uint128 tokens, uint8 decimals) private pure returns (uint64, uint64) {
        uint128 val = 1;
        for(int i = 0; i < decimals; i++) {
            val *= 10;
        }
        uint64 integer = uint64(tokens / val);
        uint64 float = uint64(tokens - (integer * val));
        return (integer, float);
    }

    function tokensToStr(uint128 tokens, uint8 decimals) public returns (string) {
        if (decimals==0) return format("{}", tokens);
        (uint64 dec, uint64 float) = splitTokens(tokens, decimals);
        string floatStr = format("{}", float);
        while (floatStr.byteLength() < decimals) {
            floatStr = "0" + floatStr;
        }
        string result = format("{}.{}", dec, floatStr);
        return result;
    }

    //collect stock data
    function getStockTonsCfg() view public {
        optional(uint256) none;
        AStock(m_stockAddr).getTonsCfg{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setStockTonsCfg),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setStockTonsCfg(uint128 transfer_tip3, uint128 return_ownership, uint128 trading_pair_deploy,
                             uint128 order_answer, uint128 process_queue, uint128 send_notify) public {
        m_stockTonCfg.transfer_tip3 = transfer_tip3;
        m_stockTonCfg.return_ownership = return_ownership;
        m_stockTonCfg.trading_pair_deploy = trading_pair_deploy;
        m_stockTonCfg.order_answer = order_answer;
        m_stockTonCfg.process_queue = process_queue;
        m_stockTonCfg.send_notify = send_notify;

        optional(uint256) none;
        AStock(m_stockAddr).getMinAmount{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setStockMinAmount),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setStockMinAmount(uint128 value0) public {
        m_stockMinAmount = value0;

        optional(uint256) none;
        AStock(m_stockAddr).getDealsLimit{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(getTradingPairCode),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function getTradingPairCode(uint8 value0) public {
        m_stockDealsLimit = value0;
        optional(uint256) none;
        AStock(m_stockAddr).getTradingPairCode{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setTradingPairCode),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setTradingPairCode(TvmCell value0) public {
        m_tradingPairCode = value0;
        IFlexHelperDebot(m_flexHelperDebot).getFCAddressAndKeys(m_stockAddr);
    }

    function onGetFCAddressAndKeys(address fc, uint256 pk, uint256 sk) public{
        m_flexClient = fc;
        m_masterPubKey = pk;
        m_masterSecKey = sk;
        getTradingPairCodeHash();
    }

    //show trading pairs
    function getTradingPairCodeHash() public {
        uint256 h = tvm.hash(m_tradingPairCode);
        Sdk.getAccountsDataByHash(tvm.functionId(onAccountsByHash),h,address(0x0));
    }

    function onAccountsByHash(ISdk.AccData[] accounts) public {
        m_tpaddrs = new address[](0);
        m_tip3Menu = new  Tip3MenuInfo[](0);
        for (uint i=0; i<accounts.length;i++)
        {
            m_tpaddrs.push(accounts[i].id);
        }

        if (m_tpaddrs.length>0)
        {
            m_curTP = 0;
            getTradingPairStock(m_tpaddrs[m_curTP]);
        }
        else
            Terminal.print(tvm.functionId(showTip3Menu),"no trading pairs!");
    }

    function getTradingPairStock(address tpa) public {
        m_tpa = tpa;
        optional(uint256) none;
        ATradingPair(m_tpa).getStockAddr{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setTradingPairStock),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setTradingPairStock(address value0) public {
        m_tip3root = value0;
        if (value0 == m_stockAddr)
        {
            getTradingPairTip3Root();
        }else
        {
            getNextTP();
        }
    }

    function getTradingPairTip3Root() public view {
        optional(uint256) none;
        ATradingPair(m_tpa).getTip3Root{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setTip3Root),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setTip3Root(address value0) public {
        m_tip3root = value0;
        getTip3RootSymbol();
    }

    function getTip3RootSymbol() public view {
        optional(uint256) none;
        ATip3Root(m_tip3root).getSymbol{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setTip3RootSymbol),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setTip3RootSymbol(bytes value0) public {
        m_tip3Menu.push(Tip3MenuInfo(m_tpa,m_tip3root,value0));
        getNextTP();
    }

    function getNextTP() public {
        m_curTP += 1;
        if (m_curTP<m_tpaddrs.length)
            getTradingPairStock(m_tpaddrs[m_curTP]);
        else
            showTip3Menu();
    }

    function showTip3Menu() public {
        MenuItem[] mi;
        for (uint i=0; i<m_tip3Menu.length;i++)
        {
            mi.push( MenuItem(m_tip3Menu[i].symbol,"",tvm.functionId(onSelectSymbolMenu)));
        }
        mi.push(MenuItem("Deploy trading pair","",tvm.functionId(menuDeployTradigPair)));
        mi.push(MenuItem("Update list","",tvm.functionId(menuUpdateTradigPair)));
        mi.push(MenuItem("Add funds to flex client","",tvm.functionId(refillBalance)));
        Menu.select("","Tip3 buy/sell",mi);
    }

    function refillBalance(uint32 index) public {
        AmountInput.get(tvm.functionId(setAmount), "How many tokens to transfer?", 9, 0.1 ton, 1000 ton);
        AddressInput.get(tvm.functionId(refillAddress), "Which wallet to transfer tokens from?");
    }

    function setAmount(uint128 value) public {
        m_dealTons = value;
    }

    function refillAddress(address value) public {
        optional(uint256) none;
        TvmCell empty;
        AMSig(value).submitTransaction{
            abiVer: 2,
            extMsg: true,
            callbackId: 0,
            onErrorId: tvm.functionId(refillFailed),
            time: 0,
            expire: 0,
            sign: true,
            pubkey: none
        }(m_flexClient, m_dealTons, true, false, empty);
        Terminal.print(tvm.functionId(getTradingPairCodeHash), "Completed");
    }

    function refillFailed(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(tvm.functionId(getTradingPairCodeHash), "Failed.");
    }

    //tp update
    function menuUpdateTradigPair(uint32 index) public {
        getTradingPairCodeHash();
    }

    // deploy trading pair
    function menuDeployTradigPair(uint32 index) public
    {
        IFlexHelperDebot(m_flexHelperDebot).deployTradigPair(m_stockAddr,m_flexClient,m_stockTonCfg.trading_pair_deploy,m_masterPubKey,m_masterSecKey);
        //response to getTradingPairCodeHash
    }

    //show order book menu
    function onSelectSymbolMenu(uint32 index) public
    {
        m_tpa = m_tip3Menu[index].tp;
        m_tip3root = m_tip3Menu[index].t3r;
        getTip3WalletAddress();
    }

    //find or deploy tip3 wallet
    function getTip3WalletAddress() public view {
        IFlexHelperDebot(m_flexHelperDebot).getTip3WalletAddress(m_tip3root, m_flexClient);
    }

    function onGetTip3WalletAddress(address t3w) public {
        m_tip3wallet = t3w;
        m_getT3WDetailsCallback = tvm.functionId(setT3WDetails);
        Terminal.print(tvm.functionId(getT3WDetails), format("tip3 wallet is {}",m_tip3wallet));
    }

    //get tip3 wallet token balance
    function getT3WDetails() view public {
        optional(uint256) none;
        ATip3Wallet(m_tip3wallet).getDetails{
            abiVer: 2,
            extMsg: true,
            callbackId: m_getT3WDetailsCallback,
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setT3WDetails(bytes name, bytes symbol, uint8 decimals, uint128 balance, uint256 root_public_key, uint256 wallet_public_key, address root_address, address owner_address, LendOwnership lend_ownership, TvmCell code, Allowance allowance, int8 workchain_id) public{
        m_tip3walletDetails.decimals = decimals;
        m_tip3walletDetails.balance = balance;
        m_tip3walletDetails.lend_ownership = lend_ownership;
        Terminal.print(0,"tip3 balance "+tokensToStr(balance,decimals));
        if (lend_ownership.owner!=address(0)) {
            Terminal.print(0,format("  tip3 lend balance {}", lend_ownership.lend_balance));
        }
        Terminal.print(tvm.functionId(getPriceCodeHash),"");
    }

    //get order book
    function getPriceCodeHash() public view {
        optional(uint256) none;
        AStock(m_stockAddr).getSellPriceCode{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setPriceCode),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }(m_tip3root);
    }
    
    function setPriceCode(TvmCell value0) public {
        m_sellPriceCode = value0;
        getPricesDataByHash();
    }

    function setUpdateT3WDetails(bytes name, bytes symbol, uint8 decimals, uint128 balance, uint256 root_public_key, uint256 wallet_public_key, address root_address, address owner_address, LendOwnership lend_ownership, TvmCell code, Allowance allowance, int8 workchain_id) public{
        m_tip3walletDetails.decimals = decimals;
        m_tip3walletDetails.balance = balance;
        m_tip3walletDetails.lend_ownership = lend_ownership;
        getPricesDataByHash();
    }

    function getPricesDataByHash() public {
        uint256 h = tvm.hash(m_sellPriceCode);
        Sdk.getAccountsDataByHash(tvm.functionId(onPricesByHash),h,address(0x0));
    }

    function onPricesByHash(ISdk.AccData[] accounts) public {
        MenuItem[] item;
        mapping (uint128 => PriceInfo) empty;
        m_prices = empty;
        m_arPrices = new uint128[](0);
        for (uint i=0; i<accounts.length;i++)
        {
            TvmSlice sl = accounts[i].data.toSlice();
            sl.decode(bool);
            (uint128 p, uint128 s, uint128 b) = sl.decode(uint128,uint128,uint128);
            m_prices[p]=PriceInfo(s,b,accounts[i].id);
        }

        optional(uint128 , PriceInfo) registryPair = m_prices.min();
        while (registryPair.hasValue()) {
            (uint128 key, PriceInfo pi) = registryPair.get();
            string str = format("{:t} | ",key);
            str += "sell "+tokensToStr(pi.sell,m_tip3walletDetails.decimals)+" | buy "+tokensToStr(pi.buy,m_tip3walletDetails.decimals);
            item.push(MenuItem(str,"",tvm.functionId(menuQuickDeal)));
            m_arPrices.push(key);
            registryPair = m_prices.next(key);
        }

        string comment = "";
        item.push(MenuItem("Buy tip3","",tvm.functionId(menuBuyTip3)));
        if (m_tip3walletDetails.lend_ownership.owner==address(0)) {item.push( MenuItem("Sell tip3","",tvm.functionId(menuSellTip3)));}
        else {
            comment = format("Your tip3 wallet is lended until {} (epoch). You can't sell tip3 tokens now.", m_tip3walletDetails.lend_ownership.lend_finish_time);
            item.push(MenuItem("Cancel Sell tip3 order","",tvm.functionId(menuCancelSellTip3)));
        }
        item.push(MenuItem("Cancel Buy tip3 order","",tvm.functionId(menuCancelBuyTip3)));
        item.push(MenuItem("Update order book","",tvm.functionId(menuOrderBookUpdate)));
        item.push(MenuItem("Back","",tvm.functionId(menuOrderBookBack)));
        Menu.select("Order Book",comment, item);
    }

    //cancel sell order
    function menuCancelSellTip3(uint32 index) public {
        m_dealTons = 0;
        optional(uint128 , PriceInfo) registryPair = m_prices.min();
        while (registryPair.hasValue()) {
            (uint128 key, PriceInfo pi) = registryPair.get();
            if (m_tip3walletDetails.lend_ownership.owner == pi.addr){
                m_dealTons = key;
                break;
            }
            registryPair = m_prices.next(key);
        }

        if (m_dealTons!=0){
            m_getT3WDetailsCallback = tvm.functionId(cancelSellOrder);
            getT3WDetails();
        } else {
            Terminal.print(tvm.functionId(getPricesDataByHash),format("Error: tip owner address not found ({})",m_tip3walletDetails.lend_ownership.owner));
        }
    }

    function cancelSellOrder(bytes name, bytes symbol, uint8 decimals, uint128 balance, uint256 root_public_key, uint256 wallet_public_key, address root_address, address owner_address, LendOwnership lend_ownership, TvmCell code, Allowance allowance, int8 workchain_id) public{
        m_tip3walletDetails.balance = balance;
        if(m_tip3walletDetails.lend_ownership.owner != lend_ownership.owner)
        {
            m_tip3walletDetails.decimals = decimals;
            m_tip3walletDetails.lend_ownership = lend_ownership;
            Terminal.print(tvm.functionId(getPricesDataByHash),"Error: state was changed");
        }
        TvmBuilder bTip3Cfg;
        bTip3Cfg.store(name,symbol,decimals,root_public_key,root_address,code);
        TvmBuilder bCancelArg;
        uint128 payout = 1000000000;
        bCancelArg.store(m_dealTons,m_stockMinAmount,m_stockDealsLimit,payout,m_sellPriceCode);
        bCancelArg.storeRef(bTip3Cfg);

        optional(uint256) none;
        m_sendMsg =  tvm.buildExtMsg({
            abiVer: 2,
            dest: m_flexClient,
            callbackId: tvm.functionId(onSellOrderCancelSuccess),
            onErrorId: tvm.functionId(onSellOrderCancelError),
            time: 0,
            expire: 0,
            sign: true,
            pubkey: none,
            call: {AFlexClient.cancelSellOrder, bCancelArg.toCell()}
        });
        string pr = tokensToStr(m_dealTons,9);
        string t3amount = tokensToStr(m_tip3walletDetails.lend_ownership.lend_balance,m_tip3walletDetails.decimals);
        ConfirmInput.get(tvm.functionId(confirmCancelSellOrder),format("Cancel sell order at price {} ammount {}\nProcessing value {:t} ton. Unused part will be returned. Continue?",pr, t3amount, payout));
    }

    function confirmCancelSellOrder(bool value) public {
        if (value) {
            onSendCancelSellOrder();
        }else {
            getPricesDataByHash();
        }
    }

    function onSendCancelSellOrder() public {
        Msg.sendWithKeypair(tvm.functionId(onSellOrderCancelSuccess),m_sendMsg,m_masterPubKey,m_masterSecKey);
    }

    function onSellOrderCancelSuccess ()  public  {
        Terminal.print(0,"Cancel order send!");
        getPricesDataByHash();
    }
    
    function onSellOrderCancelError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("Transaction failed. Sdk error = {}, Error code = {}",sdkError, exitCode));
        ConfirmInput.get(tvm.functionId(confirmCancelSellOrder),"Do you want ot retry?");
    }

    //cancel buy order
    function menuCancelBuyTip3(uint32 index) public {
        AmountInput.get(tvm.functionId(getCancelBuyPrice), "Price in TONs where you want to cancel your buy order",9,0,0xFFFFFFFFFFFFFFFF);
    }

    function getCancelBuyPrice(uint128 value) public {
        m_dealTons = value;
        m_getT3WDetailsCallback = tvm.functionId(cancelBuyOrder);
        getT3WDetails();
    }

    function cancelBuyOrder(bytes name, bytes symbol, uint8 decimals, uint128 balance, uint256 root_public_key, uint256 wallet_public_key, address root_address, address owner_address, LendOwnership lend_ownership, TvmCell code, Allowance allowance, int8 workchain_id) public{
        m_tip3walletDetails.decimals = decimals;
        m_tip3walletDetails.balance = balance;
        m_tip3walletDetails.lend_ownership = lend_ownership;
        TvmBuilder bTip3Cfg;
        bTip3Cfg.store(name,symbol,decimals,root_public_key,root_address,code);
        TvmBuilder bCancelArg;
        uint128 payout = 1000000000;
        bCancelArg.store(m_dealTons,m_stockMinAmount,m_stockDealsLimit,payout,m_sellPriceCode);
        bCancelArg.storeRef(bTip3Cfg);
        optional(uint256) none;
        m_sendMsg =  tvm.buildExtMsg({
            abiVer: 2,
            dest: m_flexClient,
            callbackId: tvm.functionId(onBuyOrderCancelSuccess),
            onErrorId: tvm.functionId(onBuyOrderCancelError),
            time: 0,
            expire: 0,
            sign: true,
            pubkey: none,
            call: {AFlexClient.cancelBuyOrder, bCancelArg.toCell()}
        });
        ConfirmInput.get(tvm.functionId(confirmCancelBuyOrder),format("Processing value {:t} ton. Unused part will be returned. Continue?",payout));
    }

    function confirmCancelBuyOrder(bool value) public {
        if (value) {
            onSendCancelBuyOrder();
        }else {
            getPricesDataByHash();
        }
    }

    function onSendCancelBuyOrder() public {
        Msg.sendWithKeypair(tvm.functionId(onBuyOrderCancelSuccess),m_sendMsg,m_masterPubKey,m_masterSecKey);
    }

    function onBuyOrderCancelSuccess ()  public  {
        Terminal.print(0,"Cancel order send!");
        getPricesDataByHash();
    }

    function onBuyOrderCancelError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("Transaction failed. Sdk error = {}, Error code = {}",sdkError, exitCode));
        ConfirmInput.get(tvm.functionId(confirmCancelBuyOrder),"Do you want ot retry?");
    }

    //Quick Deal
    function menuQuickDeal(uint32 index) public {
        m_dealTons = m_arPrices[index];
        PriceInfo pi = m_prices[m_dealTons];
        if (pi.sell!=0 && pi.buy!=0) {
            Terminal.print(tvm.functionId(getPricesDataByHash),"Fast trade is not supported at volatile price levels");
        }else if (pi.sell!=0) {
            AmountInput.get(tvm.functionId(getBuyQuickDealAmount), "Amount of tip3 to buy:",m_tip3walletDetails.decimals,m_stockMinAmount,pi.sell);
        }else if (pi.buy!=0) {
            if (m_tip3walletDetails.lend_ownership.owner==address(0)) {
                uint128 max_val;
                if (m_tip3walletDetails.balance>pi.buy) {max_val = pi.buy;} else {max_val = m_tip3walletDetails.balance;}
                AmountInput.get(tvm.functionId(getSellQuickDealAmount), "Amount of tip3 to sell:",m_tip3walletDetails.decimals,m_stockMinAmount,max_val);
            } else {
                Terminal.print(tvm.functionId(getPricesDataByHash),format("Your tip3 wallet is lended until {} (epoch). You can't sell tip3 tokens now.", m_tip3walletDetails.lend_ownership.lend_finish_time));
            }
        }else Terminal.print(tvm.functionId(getPricesDataByHash),"Fast trade is not supported at this price level");
    }

    function getBuyQuickDealAmount(uint128 value) public {
        m_dealAmount = value;
        m_getT3WDetailsCallback = tvm.functionId(deployPriceWithBuy);
        getT3WDetails();
    }

    function getSellQuickDealAmount(uint128 value) public {
        m_dealAmount = value;
        //6 min
        m_dealTime =uint32(now + 360);
        m_getT3WDetailsCallback = tvm.functionId(deployPriceWithSell);
        getT3WDetails();
    }

    //update
    function menuOrderBookUpdate(uint32 index) public {
        m_getT3WDetailsCallback = tvm.functionId(setUpdateT3WDetails);
        getT3WDetails();
    }

    //back
    function menuOrderBookBack(uint32 index) public {
        getTradingPairCodeHash();
    }
    //buy
    function menuBuyTip3(uint32 index) public {
        AmountInput.get(tvm.functionId(getDealAmmount), "Amount of tip3 to buy:",m_tip3walletDetails.decimals,m_stockMinAmount,0xFFFFFFFF);
    }
    function getDealAmmount(uint128 value) public{
       m_dealAmount = value;
       AmountInput.get(tvm.functionId(getDealPrice), "Price in TONs:",9,0,0xFFFFFFFFFFFFFFFF);
    }
    function getDealPrice(uint128 value) public{
        m_dealTons = value;
        m_getT3WDetailsCallback = tvm.functionId(deployPriceWithBuy);
        getT3WDetails();
    }

    function deployPriceWithBuy(bytes name, bytes symbol, uint8 decimals, uint128 balance, uint256 root_public_key, uint256 wallet_public_key, address root_address, address owner_address, LendOwnership lend_ownership, TvmCell code, Allowance allowance, int8 workchain_id) public{
        m_tip3walletDetails.decimals = decimals;
        m_tip3walletDetails.balance = balance;
        m_tip3walletDetails.lend_ownership = lend_ownership;
        TvmBuilder bTip3Cfg;
        bTip3Cfg.store(name,symbol,decimals,root_public_key,root_address,code);
        TvmBuilder bBuyArg;
        uint128 payout = m_dealTons * m_dealAmount + m_stockTonCfg.process_queue + m_stockTonCfg.order_answer + 1000000000;
        bBuyArg.store(m_dealTons,m_dealAmount,m_stockMinAmount,m_stockDealsLimit,payout,m_sellPriceCode,m_tip3wallet);
        bBuyArg.storeRef(bTip3Cfg);

        optional(uint256) none;
        m_sendMsg =  tvm.buildExtMsg({
            abiVer: 2,
            dest: m_flexClient,
            callbackId: tvm.functionId(onBuySuccess),
            onErrorId: tvm.functionId(onBuyError),
            time: 0,
            expire: 0,
            sign: true,
            pubkey: none,
            call: {AFlexClient.deployPriceWithBuy, bBuyArg.toCell()}
        });
        ConfirmInput.get(tvm.functionId(confirmDeployPriceWithBuy),format("It will cost {:t}. Unused part will be returned. Continue?",payout));
    }

    function confirmDeployPriceWithBuy(bool value) public {
        if (value) {
            onSendDeployBuy();
        }else {
            getPricesDataByHash();
        }
    }

    function onSendDeployBuy() public {
        Msg.sendWithKeypair(tvm.functionId(onBuySuccess),m_sendMsg,m_masterPubKey,m_masterSecKey);
    }

    function onBuySuccess (address value0)  public  {
        Terminal.print(0,"Buy order send!");
        getPricesDataByHash();
    }
    function onBuyError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("Buy failed. Sdk error = {}, Error code = {}",sdkError, exitCode));
        ConfirmInput.get(tvm.functionId(confirmDeployPriceWithBuy),"Do you want ot retry?");
    }

    //sell
    function menuSellTip3(uint32 index) public {
        AmountInput.get(tvm.functionId(getSellDealAmmount), "Amount of tip3 to sell:",m_tip3walletDetails.decimals,m_stockMinAmount,m_tip3walletDetails.balance);
    }

    function getSellDealAmmount(uint128 value) public{
       m_dealAmount = value;
       AmountInput.get(tvm.functionId(getSellDealPrice), "Price in TONs:",9,0,0xFFFFFFFFFFFFFFFF);
    }
    function getSellDealPrice(uint128 value) public{
        m_dealTons = value;
        AmountInput.get(tvm.functionId(getSellDealTime), "Order time in hours:",0,1,0xFF);
    }

    function getSellDealTime(uint128 value) public{
       m_dealTime =  uint32(value);
       m_dealTime =uint32(now + m_dealTime * 60 * 60);
       m_getT3WDetailsCallback = tvm.functionId(deployPriceWithSell);
       getT3WDetails();
    }

    function deployPriceWithSell(bytes name, bytes symbol, uint8 decimals, uint128 balance, uint256 root_public_key, uint256 wallet_public_key, address root_address, address owner_address, LendOwnership lend_ownership, TvmCell code, Allowance allowance, int8 workchain_id) public{
        m_tip3walletDetails.decimals = decimals;
        m_tip3walletDetails.balance = balance;
        m_tip3walletDetails.lend_ownership = lend_ownership;
        TvmBuilder bArgsAddrs;
        bArgsAddrs.store(m_tip3wallet,m_flexClient);
        TvmBuilder bTip3Cfg;
        bTip3Cfg.store(name,symbol,decimals,root_public_key,root_address,code);

        uint128 payout = m_stockTonCfg.process_queue + m_stockTonCfg.transfer_tip3 +
        m_stockTonCfg.return_ownership + m_stockTonCfg.order_answer + 1000000000;

        TvmBuilder bSellArg;
        bSellArg.store(m_dealTons,m_dealAmount,m_dealTime,m_stockMinAmount,m_stockDealsLimit,payout,m_sellPriceCode);
        bSellArg.storeRef(bArgsAddrs);
        bSellArg.storeRef(bTip3Cfg);

        optional(uint256) none;
        m_sendMsg = tvm.buildExtMsg({
            abiVer: 2,
            dest: m_flexClient,
            callbackId: tvm.functionId(onSellSuccess),
            onErrorId: tvm.functionId(onSellError),
            time: 0,
            expire: 0,
            sign: true,
            pubkey: none,
            call: {AFlexClient.deployPriceWithSell, bSellArg.toCell()}
        });
         ConfirmInput.get(tvm.functionId(confirmDeployPriceWithSell),format("It will cost {:t}. Unused part will be returned. Continue?",payout));
    }

    function confirmDeployPriceWithSell(bool value) public {
        if (value) {
            onSendDeploySell();
        }else {
            getPricesDataByHash();
        }
    }

    function onSendDeploySell() public {
        Msg.sendWithKeypair(tvm.functionId(onSellSuccess),m_sendMsg,m_masterPubKey,m_masterSecKey);
    }

    function onSellSuccess (address value0)  public  {
        Terminal.print(0,"Sell order send!");
        getPricesDataByHash();
    }
    function onSellError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("sell failed. Sdk error = {}, Error code = {}",sdkError, exitCode));
        ConfirmInput.get(tvm.functionId(confirmDeployPriceWithSell),"Do you want ot retry?");
    }

    /*
    *  Implementation of DeBot
    */

    function getVersion() public override returns (string name, uint24 semver) {
        (name, semver) = ("Fex DeBot", _version(0,0,5));
    }

    function _version(uint24 major, uint24 minor, uint24 fix) private pure inline returns (uint24) {
        return (major << 16) | (minor << 8) | (fix);
    }

    /*
    *  Implementation of Upgradable
    */
    function onCodeUpgrade() internal override {
        tvm.resetStorage();
    }

}