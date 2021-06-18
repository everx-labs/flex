pragma ton-solidity >=0.40.0;
pragma AbiHeader expire;
pragma AbiHeader time;
pragma AbiHeader pubkey;
// import required DeBot interfaces and basic DeBot contract.
import "../interfaces/Debot.sol";
import "../interfaces/Sdk.sol";
import "../interfaces/Terminal.sol";
import "../interfaces/Menu.sol";
import "../interfaces/ConfirmInput.sol";
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
    function getSellPriceCode(address tip3_addr) public returns(TvmCell value0) {}
}

abstract contract ATradingPair {
    function getTip3Root() public returns(address value0) {}
    function getFLeXAddr() public returns(address value0) {}
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
    string symbol;
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
    function getTip3WalletAddress(address stock, address tip3root, address flexClient, StockTonCfg tonCfg) external;
    function deployTradigPair(address stock, address fclient, uint128 tpd, uint256 pk) external;
    function withdrawTons(address fclient) external;
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

interface IFlexDebot {
    function updateTradingPairs() external;
}

contract FlexDebot is Debot{

    address m_flexHelperDebot;
    address m_stockAddr;
    address m_tip3root;
    address m_tpa;
    address[] m_tpaddrs;
    uint m_curTP;
    Tip3MenuInfo[] m_tip3Menu;
    uint256 m_masterPubKey;
    uint128 m_flexClientBalance;
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
    address m_sender;

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
        Terminal.print(0,"Please invoke me");
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
        uint128 val = 10 ** uint128(decimals);
        (uint128 dec, uint128 float) = math.divmod(tokens, val);
        string floatStr = format("{}", float);
        while (floatStr.byteLength() < decimals) {
            floatStr = "0" + floatStr;
        }
        string result = format("{}.{}", dec, floatStr);
        return result;
    }

    function tokensToStr6(uint128 tokens) public returns (string) {
        (uint64 dec, uint64 float) = splitTokens(tokens, 9);
        float = uint64(float / 1000);
        string result = format("{}.{:06}", dec, float);
        return result;
    }

    function price2Flex(uint128 tokens, uint8 decimals) public returns (uint128) {
        uint128 k = 10 ** uint128(decimals);
        return uint128(tokens / k);
    }

    function tradeTip3Ton(address fc, uint256 pk, uint128 minAmount, uint8 dealsLimit, StockTonCfg tonCfg, address stock, address t3root, address wt3) public{
        m_flexClient = fc;
        m_masterPubKey = pk;
        m_stockMinAmount = minAmount;
        m_stockDealsLimit = dealsLimit;
        m_stockTonCfg = tonCfg;
        m_stockAddr = stock;
        m_tip3root = t3root;
        m_tip3wallet = wt3;
        m_sender = msg.sender;
        m_getT3WDetailsCallback = tvm.functionId(setT3WDetails);
        getFCT3WBalances();
    }

    //get flexclient balance
    function getFCT3WBalances() public {
        Sdk.getBalance(tvm.functionId(getT3WDetails), m_flexClient);
    }

    //get tip3 wallet token balance
    function getT3WDetails(uint128 nanotokens) public {
        m_flexClientBalance = nanotokens;
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
        m_tip3walletDetails.symbol = symbol;
        getPriceCodeHash();
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
        m_tip3walletDetails.symbol = symbol;
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

        uint128 priceToUser = 1;
        for (uint i=0; i<m_tip3walletDetails.decimals;i++)
        {
            priceToUser*=10;
        }
        for (uint i=0; i<accounts.length;i++)
        {
            TvmSlice sl = accounts[i].data.toSlice();
            sl.decode(bool);
            (uint128 p, uint128 s, uint128 b) = sl.decode(uint128,uint128,uint128);
            p*=priceToUser;
            m_prices[p]=PriceInfo(s,b,accounts[i].id);
        }

        optional(uint128 , PriceInfo) registryPair = m_prices.min();
        while (registryPair.hasValue()) {
            (uint128 key, PriceInfo pi) = registryPair.get();
            string str = tokensToStr6(key);
            str += " | sell "+tokensToStr(pi.sell,m_tip3walletDetails.decimals)+" | buy "+tokensToStr(pi.buy,m_tip3walletDetails.decimals);
            item.push(MenuItem(str,"",tvm.functionId(menuQuickDeal)));
            m_arPrices.push(key);
            registryPair = m_prices.next(key);
        }

        string comment = format("Flex client balance: {:t}",m_flexClientBalance);
        comment.append(format("\nTip3 {} balance: ",m_tip3walletDetails.symbol));
        comment.append(tokensToStr(m_tip3walletDetails.balance,m_tip3walletDetails.decimals));
        item.push(MenuItem(format("Buy tip3 {}",m_tip3walletDetails.symbol),"",tvm.functionId(menuBuyTip3)));
        
        if (m_tip3walletDetails.lend_ownership.owner==address(0)) {
            if (m_tip3walletDetails.balance>0) {item.push( MenuItem(format("Sell tip3 {}",m_tip3walletDetails.symbol),"",tvm.functionId(menuSellTip3)));}
        } else {
            comment.append("\n  tip3 lend balance: "+tokensToStr(m_tip3walletDetails.lend_ownership.lend_balance,m_tip3walletDetails.decimals));
            comment.append("\nYour tip3 wallet is lent. You can't sell tip3 tokens now.");
            item.push(MenuItem("Cancel Sell tip3 order","",tvm.functionId(menuCancelSellTip3)));
        }
        item.push(MenuItem("Cancel Buy tip3 order","",tvm.functionId(menuCancelBuyTip3)));
        item.push(MenuItem("Update order book","",tvm.functionId(menuOrderBookUpdate)));
        item.push(MenuItem("Back","",tvm.functionId(menuOrderBookBack)));

        Terminal.print(0,comment);
        Menu.select("Order Book","", item);
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
            getFCT3WBalances();
        } else {
            Terminal.print(tvm.functionId(getPricesDataByHash),format("Error: tip3 owner address not found ({})",m_tip3walletDetails.lend_ownership.owner));
        }
    }

    function cancelSellOrder(bytes name, bytes symbol, uint8 decimals, uint128 balance, uint256 root_public_key, uint256 wallet_public_key, address root_address, address owner_address, LendOwnership lend_ownership, TvmCell code, Allowance allowance, int8 workchain_id) public{
        m_tip3walletDetails.balance = balance;
        if(m_tip3walletDetails.lend_ownership.owner != lend_ownership.owner){
            m_tip3walletDetails.decimals = decimals;
            m_tip3walletDetails.lend_ownership = lend_ownership;
            Terminal.print(tvm.functionId(getPricesDataByHash),"Error: state was changed");
            return;
        }
        uint128 payout = 1000000000;
        if (payout+100000000>m_flexClientBalance) {
            Terminal.print(0,"Error: Flex client balance too low!\nPlease send tons to your flex client address:");
            Terminal.print(tvm.functionId(getPricesDataByHash),format("{}",m_flexClient));
            return;
        }
        
        uint128 flexTons = price2Flex(m_dealTons, m_tip3walletDetails.decimals);
        TvmBuilder bTip3Cfg;
        bTip3Cfg.store(name,symbol,decimals,root_public_key,root_address);
        TvmBuilder bCancelArg;
        bCancelArg.store(flexTons,m_stockMinAmount,m_stockDealsLimit,payout,m_sellPriceCode,code);
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
        ConfirmInput.get(tvm.functionId(confirmCancelSellOrder),format("Cancel sell order at price {} amount {}\nProcessing value {:t} ton. Unused part will be returned. Continue?",pr, t3amount, payout));
    }

    function confirmCancelSellOrder(bool value) public {
        if (value) {
            onSendCancelSellOrder();
        }else {
            getPricesDataByHash();
        }
    }

    function onSendCancelSellOrder() public {
        tvm.sendrawmsg(m_sendMsg, 1);
    }

    function onSellOrderCancelSuccess ()  public  {
        Terminal.print(0,"Order cancellation sent!");
        getPricesDataByHash();
    }

    function onSellOrderCancelError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("Transaction failed. Sdk error = {}, Error code = {}",sdkError, exitCode));
        ConfirmInput.get(tvm.functionId(confirmCancelSellOrder),"Do you want ot retry?");
    }

    //cancel buy order
    function menuCancelBuyTip3(uint32 index) public {
        AmountInput.get(tvm.functionId(getCancelBuyPrice), "Price in TONs where you want to cancel your buy order",6,1,0xFFFFFFFF);
    }

    function getCancelBuyPrice(uint128 value) public {
        m_dealTons = value*1000;
        m_getT3WDetailsCallback = tvm.functionId(cancelBuyOrder);
        getFCT3WBalances();
    }

    function cancelBuyOrder(bytes name, bytes symbol, uint8 decimals, uint128 balance, uint256 root_public_key, uint256 wallet_public_key, address root_address, address owner_address, LendOwnership lend_ownership, TvmCell code, Allowance allowance, int8 workchain_id) public{
        m_tip3walletDetails.decimals = decimals;
        m_tip3walletDetails.balance = balance;
        m_tip3walletDetails.lend_ownership = lend_ownership;
        uint128 payout = 1000000000;
        if (payout+100000000>m_flexClientBalance) {
            Terminal.print(0,"Error: Flex client balance too low!\nPlease send tons to your flex client address:");
            Terminal.print(tvm.functionId(getPricesDataByHash),format("{}",m_flexClient));
            return;
        }
        m_dealTons = price2Flex(m_dealTons, m_tip3walletDetails.decimals);
        TvmBuilder bTip3Cfg;
        bTip3Cfg.store(name,symbol,decimals,root_public_key,root_address);
        TvmBuilder bCancelArg;
        bCancelArg.store(m_dealTons,m_stockMinAmount,m_stockDealsLimit,payout,m_sellPriceCode,code);
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
        tvm.sendrawmsg(m_sendMsg, 1);
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
                if (m_tip3walletDetails.balance>0){
                    uint128 max_val;
                    if (m_tip3walletDetails.balance>pi.buy) {max_val = pi.buy;} else {max_val = m_tip3walletDetails.balance;}
                    AmountInput.get(tvm.functionId(getSellQuickDealAmount), "Amount of tip3 to sell:",m_tip3walletDetails.decimals,m_stockMinAmount,max_val);
                } else {Terminal.print(tvm.functionId(getPricesDataByHash),"You haven't tip3 tokens to sell.");}
            } else {
                Terminal.print(tvm.functionId(getPricesDataByHash),"Your tip3 wallet is lent. You can't sell tip3 tokens now.");
              }
        }else Terminal.print(tvm.functionId(getPricesDataByHash),"Fast trade is not supported at this price level");
    }

    function getBuyQuickDealAmount(uint128 value) public {
        m_dealAmount = value;
        //1 hour
        m_dealTime =uint32(now + 3600);
        m_getT3WDetailsCallback = tvm.functionId(deployPriceWithBuy);
        getFCT3WBalances();
    }

    function getSellQuickDealAmount(uint128 value) public {
        m_dealAmount = value;
        //1 hour
        m_dealTime =uint32(now + 3600);
        m_getT3WDetailsCallback = tvm.functionId(deployPriceWithSell);
        getFCT3WBalances();
    }

    //update
    function menuOrderBookUpdate(uint32 index) public {
        m_getT3WDetailsCallback = tvm.functionId(setUpdateT3WDetails);
        getFCT3WBalances();
    }

    //back
    function menuOrderBookBack(uint32 index) public {
        IFlexDebot(m_sender).updateTradingPairs();
        m_sender = address(0);
    }
    //buy
    function menuBuyTip3(uint32 index) public {
        AmountInput.get(tvm.functionId(getDealAmmount), "Amount of tip3 to buy:",m_tip3walletDetails.decimals,m_stockMinAmount,0xFFFFFFFF);
    }
    function getDealAmmount(uint128 value) public{
       m_dealAmount = value;
       AmountInput.get(tvm.functionId(getDealPrice), "Price in TONs:",6,1,0xFFFFFFFF);
    }
    function getDealPrice(uint128 value) public{
        m_dealTons = value*1000;
        AmountInput.get(tvm.functionId(getBuyDealTime), "Order time in hours:",0,1,0xFF);
    }

    function getBuyDealTime(uint128 value) public{
       m_dealTime =  uint32(value);
       m_dealTime =uint32(now + m_dealTime * 60 * 60);
       m_getT3WDetailsCallback = tvm.functionId(deployPriceWithBuy);
       getFCT3WBalances();
    }

    function deployPriceWithBuy(bytes name, bytes symbol, uint8 decimals, uint128 balance, uint256 root_public_key, uint256 wallet_public_key, address root_address, address owner_address, LendOwnership lend_ownership, TvmCell code, Allowance allowance, int8 workchain_id) public{
        m_tip3walletDetails.decimals = decimals;
        m_tip3walletDetails.balance = balance;
        m_tip3walletDetails.lend_ownership = lend_ownership;
        m_dealTons = price2Flex(m_dealTons, m_tip3walletDetails.decimals);
        uint128 payout = m_dealTons * m_dealAmount + m_stockTonCfg.process_queue + m_stockTonCfg.order_answer + 1000000000;
        if (payout+100000000>m_flexClientBalance) {
            Terminal.print(0,"Error: Flex client balance too low!\nPlease send tons to your flex client address:");
            Terminal.print(tvm.functionId(getPricesDataByHash),format("{}",m_flexClient));
            return;
        }
        TvmBuilder bTip3Cfg;
        bTip3Cfg.store(name,symbol,decimals,root_public_key,root_address);
        TvmBuilder bBuyArg;
        bBuyArg.store(m_dealTons,m_dealAmount,m_dealTime,m_stockMinAmount,m_stockDealsLimit,payout,m_sellPriceCode,m_tip3wallet,code);
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
        tvm.sendrawmsg(m_sendMsg, 1);
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
       AmountInput.get(tvm.functionId(getSellDealPrice), "Price in TONs:",6,1,0xFFFFFFFF);
    }
    function getSellDealPrice(uint128 value) public{
        m_dealTons = value*1000;
        AmountInput.get(tvm.functionId(getSellDealTime), "Order time in hours:",0,1,0xFF);
    }

    function getSellDealTime(uint128 value) public{
       m_dealTime =  uint32(value);
       m_dealTime =uint32(now + m_dealTime * 60 * 60);
       m_getT3WDetailsCallback = tvm.functionId(deployPriceWithSell);
       getFCT3WBalances();
    }

    function deployPriceWithSell(bytes name, bytes symbol, uint8 decimals, uint128 balance, uint256 root_public_key, uint256 wallet_public_key, address root_address, address owner_address, LendOwnership lend_ownership, TvmCell code, Allowance allowance, int8 workchain_id) public{
        m_tip3walletDetails.decimals = decimals;
        m_tip3walletDetails.balance = balance;
        m_tip3walletDetails.lend_ownership = lend_ownership;
        m_dealTons = price2Flex(m_dealTons, m_tip3walletDetails.decimals);
        uint128 payout = m_stockTonCfg.process_queue + m_stockTonCfg.transfer_tip3 +
        m_stockTonCfg.return_ownership + m_stockTonCfg.order_answer + 1000000000;
        if (payout+100000000>m_flexClientBalance) {
            Terminal.print(0,"Error: Flex client balance too low!\nPlease send tons to your flex client address:");
            Terminal.print(tvm.functionId(getPricesDataByHash),format("{}",m_flexClient));
            return;
        }
        TvmBuilder bArgsAddrs;
        bArgsAddrs.store(m_tip3wallet,m_flexClient);
        TvmBuilder bTip3Cfg;
        bTip3Cfg.store(name,symbol,decimals,root_public_key,root_address);
        TvmBuilder bSellArg;
        bSellArg.store(m_dealTons,m_dealAmount,m_dealTime,m_stockMinAmount,m_stockDealsLimit,payout,m_sellPriceCode);
        bSellArg.storeRef(bArgsAddrs);
        bSellArg.store(code);
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
        tvm.sendrawmsg(m_sendMsg, 1);
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
    function getDebotInfo() public functionID(0xDEB) override view returns(
        string name, string version, string publisher, string caption, string author,
        address support, string hello, string language, string dabi, bytes icon
    ) {
        name = "Flex Tip3Ton Debot";
        version = "0.1.2";
        publisher = "TON Labs";
        caption = "Work with flex";
        author = "TON Labs";
        support = address.makeAddrStd(0, 0x0);
        hello = "Hello, i'am a Flex DeBot.";
        language = "en";
        dabi = m_debotAbi.get();
        icon = "";
    }

    function getRequiredInterfaces() public view override returns (uint256[] interfaces) {
        return [ AddressInput.ID, AmountInput.ID, ConfirmInput.ID, Menu.ID, Sdk.ID, Terminal.ID ];
    }

    /*
    *  Implementation of Upgradable
    */
    function upgrade(TvmCell state) public functionID(2) {
        require(msg.pubkey() == tvm.pubkey(), 100);
        TvmCell newcode = state.toSlice().loadRef();
        tvm.accept();
        tvm.commit();
        tvm.setcode(newcode);
        tvm.setCurrentCode(newcode);
        onCodeUpgrade();
    }

    function onCodeUpgrade() internal {
        tvm.resetStorage();
    }

}