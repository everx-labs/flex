pragma ton-solidity >=0.40.0;
pragma AbiHeader expire;
pragma AbiHeader time;
pragma AbiHeader pubkey;
// import required DeBot interfaces and basic DeBot contract.
import "https://raw.githubusercontent.com/tonlabs/debots/main/Debot.sol";
import "DeBotInterfaces.sol";
import "../abstract/Upgradable.sol";
import "../abstract/AMSig.sol";
import "../abstract/AFlex.sol";
import "../abstract/ATradingPair.sol";
import "../abstract/AFlexClient.sol";
import "../abstract/AFlexWallet.sol";
import "../interface/IFlexTip3TonDebot.sol";
import "../interface/IFlexDebot.sol";

struct T3WDetails {
    uint8 decimals;
    uint128 balance;
    string symbol;
    LendOwnership[] lend_ownership;
}

struct PriceInfo {
       uint128 sell;
       uint128 buy;
       address addr;
    }

contract FlexTip3TonDebot is IFlexTip3TonDebot, Debot, Upgradable{
    address m_flexAddr;
    address m_tip3root;
    uint32 m_signingBox;
    uint128 m_flexClientBalance;
    TvmCell m_sendMsg;
    TvmCell m_sellPriceCode;
    address m_flexClient;
    address m_tip3wallet;
    T3WDetails m_tip3walletDetails;
    uint128 m_dealTons;
    uint128 m_dealAmount;
    uint32 m_dealTime;
    uint128 m_flexMinAmount;
    uint8 m_flexDealsLimit;
    FlexTonCfg m_flexTonCfg;
    uint32 m_getT3WDetailsCallback;
    mapping(uint128 => PriceInfo) m_prices;
    uint128[] m_arPrices;
    address m_sender;
    address m_tpNotifyAddress;
    bool m_bInstantBuy;

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

    function tokensToStr(uint128 tokens, uint8 decimals) public pure returns (string) {
        if (decimals==0) return format("{}", tokens);
        uint128 val = uint128(10) ** uint128(decimals);
        (uint128 dec, uint128 float) = math.divmod(tokens, val);
        string floatStr = format("{}", float);
        while (floatStr.byteLength() < decimals) {
            floatStr = "0" + floatStr;
        }
        string result = format("{}.{}", dec, floatStr);
        return result;
    }

    function tokensToStr6(uint128 tokens) public pure returns (string) {
        (uint64 dec, uint64 float) = splitTokens(tokens, 9);
        float = uint64(float / 1000);
        string result = format("{}.{:06}", dec, float);
        return result;
    }

    function price2Flex(uint128 tokens, uint8 decimals) public pure returns (uint128) {
        uint128 k = uint128(10) ** uint128(decimals);
        return uint128(tokens / k);
    }

    function instantTradeTip3Ton(address fc, uint32 signingBox, address notifyAddr, uint128 minAmount, uint8 dealsLimit, FlexTonCfg tonCfg, address stock, address t3root, address wt3, bool bBuy, uint128 price, uint128 volume) public override{
        m_flexClient = fc;
        m_signingBox = signingBox;
        m_tpNotifyAddress = notifyAddr;
        m_flexMinAmount = minAmount;
        m_flexDealsLimit = dealsLimit;
        m_flexTonCfg = tonCfg;
        m_flexAddr = stock;
        m_tip3root = t3root;
        m_tip3wallet = wt3;
        m_sender = msg.sender;
        m_dealTime =uint32(now + 86400);//24 hour
        m_dealAmount = volume;
        m_dealTons = price;
        m_bInstantBuy = bBuy;

        optional(uint256) none;
        AFlex(m_flexAddr).getSellPriceCode{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setInstantPriceCode),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }(m_tip3root);
    }

    function setInstantPriceCode(TvmCell value0) public {
        m_sellPriceCode = value0;

        if (m_bInstantBuy)
            m_getT3WDetailsCallback = tvm.functionId(deployPriceWithBuy);
        else
            m_getT3WDetailsCallback = tvm.functionId(deployPriceWithSell);
        getFCT3WBalances();
    }


    function tradeTip3Ton(address fc, uint32 signingBox, address notifyAddr, uint128 minAmount, uint8 dealsLimit, FlexTonCfg tonCfg, address stock, address t3root, address wt3) public override{
        m_flexClient = fc;
        m_signingBox = signingBox;
        m_tpNotifyAddress = notifyAddr;
        m_flexMinAmount = minAmount;
        m_flexDealsLimit = dealsLimit;
        m_flexTonCfg = tonCfg;
        m_flexAddr = stock;
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
        AFlexWallet(m_tip3wallet).getDetails{
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

    function setT3WDetails(bytes name, bytes symbol, uint8 decimals, uint128 balance, uint256 root_public_key, uint256 wallet_public_key, address root_address, address owner_address, LendOwnership[] lend_ownership, uint128 lend_balance, TvmCell code, Allowance allowance, int8 workchain_id) public{
        name;root_public_key;wallet_public_key;root_address;owner_address;lend_balance;code;allowance;workchain_id;//disable compile warnings
        m_tip3walletDetails.decimals = decimals;
        m_tip3walletDetails.balance = balance - lend_balance;
        m_tip3walletDetails.lend_ownership = lend_ownership;
        m_tip3walletDetails.symbol = symbol;
        Terminal.print(tvm.functionId(getPriceCodeHash),format("Your tip3 {} wallet is {}",m_tip3walletDetails.symbol,m_tip3wallet));
    }

    //get order book
    function getPriceCodeHash() public view {
        optional(uint256) none;
        AFlex(m_flexAddr).getSellPriceCode{
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

    function setUpdateT3WDetails(bytes name, bytes symbol, uint8 decimals, uint128 balance, uint256 root_public_key, uint256 wallet_public_key, address root_address, address owner_address, LendOwnership[] lend_ownership, uint128 lend_balance, TvmCell code, Allowance allowance, int8 workchain_id) public{
        name;root_public_key;wallet_public_key;root_address;owner_address;lend_balance;code;allowance;workchain_id;//disable compile warnings
        m_tip3walletDetails.decimals = decimals;
        m_tip3walletDetails.balance = balance - lend_balance;
        m_tip3walletDetails.lend_ownership = lend_ownership;
        m_tip3walletDetails.symbol = symbol;
        getPricesDataByHash();
    }

    function getPricesDataByHash() public {
        uint256 h = tvm.hash(m_sellPriceCode);
        Sdk.getAccountsDataByHash(tvm.functionId(onPricesByHash),h,address(0x0));
    }

    function onPricesByHash(AccData[] accounts) public {
        MenuItem[] item;
        mapping (uint128 => PriceInfo) empty;
        m_prices = empty;
        m_arPrices = new uint128[](0);

        uint128 priceToUser = uint128(10) ** uint128(m_tip3walletDetails.decimals);
        for (uint i=0; i<accounts.length;i++)
        {
            TvmSlice sl = accounts[i].data.toSlice();
            sl.decode(bool);
            (uint128 p, uint128 s, uint128 b) = sl.decode(uint128,uint128,uint128);
            p*=priceToUser;
            m_prices[p]=PriceInfo(s,b,accounts[i].id);
        }

        optional(uint128 , PriceInfo) registryPair = m_prices.max();
        while (registryPair.hasValue()) {
            (uint128 key, PriceInfo pi) = registryPair.get();
            string str = tokensToStr6(key);
            str += " | sell "+tokensToStr(pi.sell,m_tip3walletDetails.decimals)+" | buy "+tokensToStr(pi.buy,m_tip3walletDetails.decimals);
            item.push(MenuItem(str,"",tvm.functionId(menuQuickDeal)));
            m_arPrices.push(key);
            registryPair = m_prices.prev(key);
        }

        string comment = format("Flex client balance: {:t}",m_flexClientBalance);
        comment.append(format("\nTip3 {} balance: ",m_tip3walletDetails.symbol));
        comment.append(tokensToStr(m_tip3walletDetails.balance,m_tip3walletDetails.decimals));
        item.push(MenuItem(format("Buy tip3 {}",m_tip3walletDetails.symbol),"",tvm.functionId(menuBuyTip3)));
        if (m_tip3walletDetails.balance>0) {item.push( MenuItem(format("Sell tip3 {}",m_tip3walletDetails.symbol),"",tvm.functionId(menuSellTip3)));}
        if (m_tip3walletDetails.lend_ownership.length>0) {
            item.push(MenuItem("Cancel Sell tip3 order","",tvm.functionId(menuCancelSellTip3)));
        }
        item.push(MenuItem("Cancel Buy tip3 order","",tvm.functionId(menuCancelBuyTip3)));
        item.push(MenuItem("Update order book","",tvm.functionId(menuOrderBookUpdate)));
        item.push(MenuItem(format("Withdraw {} tip3",m_tip3walletDetails.symbol),"",tvm.functionId(menuOrderBookBurnTip3)));
        item.push(MenuItem("Back","",tvm.functionId(menuOrderBookBack)));

        Terminal.print(0,comment);
        Menu.select("Order Book","", item);
    }

    //cancel sell order
    function menuCancelSellTip3(uint32 index) public {
        index;//disable compile warning
        MenuItem[] item;
        m_arPrices = new uint128[](0);
        optional(uint128 , PriceInfo) registryPair = m_prices.max();
        while (registryPair.hasValue()) {
            (uint128 key, PriceInfo pi) = registryPair.get();
            for (uint i = 0; i < m_tip3walletDetails.lend_ownership.length; ++i){
                if (m_tip3walletDetails.lend_ownership[i].lend_addr == pi.addr){
                    m_arPrices.push(key);
                    item.push(MenuItem(format("Price {} Volume {}",tokensToStr6(key),tokensToStr(m_tip3walletDetails.lend_ownership[i].lend_balance, m_tip3walletDetails.decimals)),"",tvm.functionId(menuCancelSellTip3Order)));
                    break;
                }
            }
            registryPair = m_prices.prev(key);
        }
        item.push(MenuItem("Back","",tvm.functionId(menuOrderBookUpdate)));

        Menu.select("Cancel order","", item);
    }

     function menuCancelSellTip3Order(uint32 index) public {
        m_dealTons = m_arPrices[index];
        m_getT3WDetailsCallback = tvm.functionId(cancelSellOrder);
        getFCT3WBalances();
     }

    function cancelSellOrder(bytes name, bytes symbol, uint8 decimals, uint128 balance, uint256 root_public_key, uint256 wallet_public_key, address root_address, address owner_address, LendOwnership[] lend_ownership, uint128 lend_balance, TvmCell code, Allowance allowance, int8 workchain_id) public{
        name;symbol;decimals;root_public_key;wallet_public_key;root_address;owner_address;lend_ownership;lend_balance;code;allowance;workchain_id;//disable compile warnings
        m_tip3walletDetails.balance = balance - lend_balance;
        uint128 payout = 1000000000;
        if (payout+100000000>m_flexClientBalance) {
            Terminal.print(0,"Error: Flex client balance too low!\nPlease send tons to your flex client address:");
            Terminal.print(tvm.functionId(getPricesDataByHash),format("{}",m_flexClient));
            return;
        }

        uint128 flexTons = price2Flex(m_dealTons, m_tip3walletDetails.decimals);
        Tip3Cfg tip3cfg;
        tip3cfg.name=name;
        tip3cfg.symbol=symbol;
        tip3cfg.decimals=decimals;
        tip3cfg.root_public_key=root_public_key;
        tip3cfg.root_address=root_address;

        optional(uint256) none;
        m_sendMsg =  tvm.buildExtMsg({
            abiVer: 2,
            dest: m_flexClient,
            callbackId: tvm.functionId(onSellOrderCancelSuccess),
            onErrorId: tvm.functionId(onSellOrderCancelError),
            time: 0,
            expire: 0,
            sign: true,
            signBoxHandle: m_signingBox,
            pubkey: none,
            call: {AFlexClient.cancelSellOrder, flexTons,m_flexMinAmount,m_flexDealsLimit,payout,m_sellPriceCode,tip3cfg,m_tpNotifyAddress}
        });
        string pr = tokensToStr6(m_dealTons);
        ConfirmInput.get(tvm.functionId(confirmCancelSellOrder),format("Cancel sell order at price {}\nProcessing value {:t} ton. Unused part will be returned. Continue?",pr, payout));
    }

    function confirmCancelSellOrder(bool value) public {
        if (value) {
            onSendCancelSellOrder();
        }else {
            getPricesDataByHash();
        }
    }

    function onSendCancelSellOrder() public view {
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
        index;//disable compile warning
        AmountInput.get(tvm.functionId(getCancelBuyPrice), "Price in TONs where you want to cancel your buy order",6,1,0xFFFFFFFF);
    }

    function getCancelBuyPrice(uint128 value) public {
        m_dealTons = value*1000;
        m_getT3WDetailsCallback = tvm.functionId(cancelBuyOrder);
        getFCT3WBalances();
    }

    function cancelBuyOrder(bytes name, bytes symbol, uint8 decimals, uint128 balance, uint256 root_public_key, uint256 wallet_public_key, address root_address, address owner_address, LendOwnership[] lend_ownership, uint128 lend_balance, TvmCell code, Allowance allowance, int8 workchain_id) public{
        name;symbol;root_public_key;wallet_public_key;root_address;owner_address;lend_balance;code;allowance;workchain_id;//disable compile warnings
        m_tip3walletDetails.decimals = decimals;
        m_tip3walletDetails.balance = balance - lend_balance;
        m_tip3walletDetails.lend_ownership = lend_ownership;
        uint128 payout = 1000000000;
        if (payout+100000000>m_flexClientBalance) {
            Terminal.print(0,"Error: Flex client balance too low!\nPlease send tons to your flex client address:");
            Terminal.print(tvm.functionId(getPricesDataByHash),format("{}",m_flexClient));
            return;
        }
        m_dealTons = price2Flex(m_dealTons, m_tip3walletDetails.decimals);
        Tip3Cfg tip3cfg;
        tip3cfg.name=name;
        tip3cfg.symbol=symbol;
        tip3cfg.decimals=decimals;
        tip3cfg.root_public_key=root_public_key;
        tip3cfg.root_address=root_address;

        optional(uint256) none;
        m_sendMsg =  tvm.buildExtMsg({
            abiVer: 2,
            dest: m_flexClient,
            callbackId: tvm.functionId(onBuyOrderCancelSuccess),
            onErrorId: tvm.functionId(onBuyOrderCancelError),
            time: 0,
            expire: 0,
            sign: true,
            signBoxHandle: m_signingBox,
            pubkey: none,
            call: {AFlexClient.cancelBuyOrder, m_dealTons, m_flexMinAmount, m_flexDealsLimit ,payout, m_sellPriceCode, tip3cfg, m_tpNotifyAddress}
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

    function onSendCancelBuyOrder() public view {
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
        index;//disable compile warning
        m_dealTons = m_arPrices[index];
        PriceInfo pi = m_prices[m_dealTons];
        if (pi.sell!=0 && pi.buy!=0) {
            Terminal.print(tvm.functionId(getPricesDataByHash),"Fast trade is not supported at volatile price levels");
        }else if (pi.sell!=0) {
            AmountInput.get(tvm.functionId(getBuyQuickDealAmount), "Amount of tip3 to buy:",m_tip3walletDetails.decimals,m_flexMinAmount,pi.sell);
        }else if (pi.buy!=0) {
            if (m_tip3walletDetails.balance>0){
                uint128 max_val;
                if (m_tip3walletDetails.balance>pi.buy) {max_val = pi.buy;} else {max_val = m_tip3walletDetails.balance;}
                AmountInput.get(tvm.functionId(getSellQuickDealAmount), "Amount of tip3 to sell:",m_tip3walletDetails.decimals,m_flexMinAmount,max_val);
            } else {Terminal.print(tvm.functionId(getPricesDataByHash),"You haven't tip3 tokens to sell.");}
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
        index;//disable compile warning
        m_getT3WDetailsCallback = tvm.functionId(setUpdateT3WDetails);
        getFCT3WBalances();
    }

    //back
    function menuOrderBookBack(uint32 index) public {
        index;//disable compile warning
        IFlexDebot(m_sender).updateTradingPairs();
        m_sender = address(0);
    }

    //withdraw tip3
    function menuOrderBookBurnTip3(uint32 index) public {
        index;//disable compile warning
        IFlexDebot(m_sender).burnTip3Wallet(m_flexClient,m_signingBox,m_tip3wallet);
        m_sender = address(0);
    }
    //buy
    function menuBuyTip3(uint32 index) public {
        index;//disable compile warning
        AmountInput.get(tvm.functionId(getDealAmmount), "Amount of tip3 to buy:",m_tip3walletDetails.decimals,m_flexMinAmount,0xFFFFFFFF);
    }
    function getDealAmmount(uint128 value) public{
       m_dealAmount = value;
       AmountInput.get(tvm.functionId(getDealPrice), "Price in TONs:",6,1,0xFFFFFFFF);
    }
    function getDealPrice(uint128 value) public{
        m_dealTons = value*1000;
        AmountInput.get(tvm.functionId(getBuyDealTime), "Order time in hours:",0,1,8640);
    }

    function getBuyDealTime(uint128 value) public{
       m_dealTime =  uint32(value);
       m_dealTime =uint32(now + m_dealTime * 60 * 60);
       m_getT3WDetailsCallback = tvm.functionId(deployPriceWithBuy);
       getFCT3WBalances();
    }

    function deployPriceWithBuy(bytes name, bytes symbol, uint8 decimals, uint128 balance, uint256 root_public_key, uint256 wallet_public_key, address root_address, address owner_address, LendOwnership[] lend_ownership, uint128 lend_balance, TvmCell code, Allowance allowance, int8 workchain_id) public{
        name;symbol;root_public_key;wallet_public_key;root_address;owner_address;lend_balance;code;allowance;workchain_id;//disable compile warnings
        m_tip3walletDetails.decimals = decimals;
        m_tip3walletDetails.balance = balance - lend_balance;
        m_tip3walletDetails.lend_ownership = lend_ownership;
        m_dealTons = price2Flex(m_dealTons, m_tip3walletDetails.decimals);
        uint128 payout = m_dealTons * m_dealAmount + m_flexTonCfg.process_queue + m_flexTonCfg.order_answer + 1 ton;
        if (payout+0.1 ton>m_flexClientBalance) {
            Terminal.print(0,"Error: Flex client balance too low!\nPlease send tons to your flex client address:");
            Terminal.print(tvm.functionId(getPricesDataByHash),format("{}",m_flexClient));
            return;
        }
        Tip3Cfg tip3cfg;
        tip3cfg.name=name;
        tip3cfg.symbol=symbol;
        tip3cfg.decimals=decimals;
        tip3cfg.root_public_key=root_public_key;
        tip3cfg.root_address=root_address;

        optional(uint256) none;
        m_sendMsg =  tvm.buildExtMsg({
            abiVer: 2,
            dest: m_flexClient,
            callbackId: tvm.functionId(onBuySuccess),
            onErrorId: tvm.functionId(onBuyError),
            time: 0,
            expire: 0,
            sign: true,
            signBoxHandle: m_signingBox,
            pubkey: none,
            call: {AFlexClient.deployPriceWithBuy, m_dealTons,m_dealAmount,m_dealTime,m_flexMinAmount,m_flexDealsLimit,payout,m_sellPriceCode,m_tip3wallet,tip3cfg, m_tpNotifyAddress}
        });
        ConfirmInput.get(tvm.functionId(confirmDeployPriceWithBuy),format("Buy will cost {:t} TONs. Unused part will be returned. Continue?",payout));
    }

    function confirmTestBuy() public {
        Terminal.print(0,"Success!!!");
    }


    function confirmDeployPriceWithBuy(bool value) public {
        if (value) {
            onSendDeployBuy();
        }else {
            getPricesDataByHash();
        }
    }

    function onSendDeployBuy() public view {
        tvm.sendrawmsg(m_sendMsg, 1);
    }

    function onBuySuccess (address value0)  public  {
        value0;//disable compile warning
        Terminal.print(0,"Buy order send!");
        getPricesDataByHash();
    }
    function onBuyError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("Buy failed. Sdk error = {}, Error code = {}",sdkError, exitCode));
        ConfirmInput.get(tvm.functionId(confirmDeployPriceWithBuy),"Do you want ot retry?");
    }

    //sell
    function menuSellTip3(uint32 index) public {
        index;//disable compile warning
        AmountInput.get(tvm.functionId(getSellDealAmmount), "Amount of tip3 to sell:",m_tip3walletDetails.decimals,m_flexMinAmount,m_tip3walletDetails.balance);
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

    function deployPriceWithSell(bytes name, bytes symbol, uint8 decimals, uint128 balance, uint256 root_public_key, uint256 wallet_public_key, address root_address, address owner_address, LendOwnership[] lend_ownership, uint128 lend_balance, TvmCell code, Allowance allowance, int8 workchain_id) public{
        name;symbol;root_public_key;wallet_public_key;root_address;owner_address;lend_balance;code;allowance;workchain_id;//disable compile warnings
        m_tip3walletDetails.decimals = decimals;
        m_tip3walletDetails.balance = balance - lend_balance;
        m_tip3walletDetails.lend_ownership = lend_ownership;
        m_dealTons = price2Flex(m_dealTons, m_tip3walletDetails.decimals);
        uint128 payout = m_flexTonCfg.process_queue + m_flexTonCfg.transfer_tip3 +
        m_flexTonCfg.return_ownership + m_flexTonCfg.order_answer + 1000000000;
        if (payout+100000000>m_flexClientBalance) {
            Terminal.print(0,"Error: Flex client balance too low!\nPlease send tons to your flex client address:");
            Terminal.print(tvm.functionId(getPricesDataByHash),format("{}",m_flexClient));
            return;
        }
        if (m_dealAmount>m_tip3walletDetails.balance) {
            Terminal.print(0,"Error: Not enough tip3 tokens on your tip3 wallet: ");
            Terminal.print(tvm.functionId(getPricesDataByHash),format("{}",m_tip3wallet));
            return;
        }
        Tip3Cfg tip3cfg;
        tip3cfg.name=name;
        tip3cfg.symbol=symbol;
        tip3cfg.decimals=decimals;
        tip3cfg.root_public_key=root_public_key;
        tip3cfg.root_address=root_address;

        optional(uint256) none;
        m_sendMsg = tvm.buildExtMsg({
            abiVer: 2,
            dest: m_flexClient,
            callbackId: tvm.functionId(onSellSuccess),
            onErrorId: tvm.functionId(onSellError),
            time: 0,
            expire: 0,
            sign: true,
            signBoxHandle: m_signingBox,
            pubkey: none,
            call: {AFlexClient.deployPriceWithSell, m_dealTons,m_dealAmount,m_dealTime,m_flexMinAmount,m_flexDealsLimit,payout,m_sellPriceCode,m_tip3wallet,m_flexClient,tip3cfg,m_tpNotifyAddress}
        });
         ConfirmInput.get(tvm.functionId(confirmDeployPriceWithSell),format("It will cost {:t} TONs. Unused part will be returned. Continue?",payout));
    }

    function confirmDeployPriceWithSell(bool value) public {
        if (value) {
            onSendDeploySell();
        }else {
            getPricesDataByHash();
        }
    }

    function onSendDeploySell() public view {
        tvm.sendrawmsg(m_sendMsg, 1);
    }

    function onSellSuccess (address value0)  public  {
        value0;//disable compile warning
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
        string name, string version, string publisher, string key, string author,
        address support, string hello, string language, string dabi, bytes icon
    ) {
        name = "Flex Tip3Ton Debot";
        version = "0.2.3";
        publisher = "TON Labs";
        key = "Work with flex";
        author = "TON Labs";
        support = address.makeAddrStd(0, 0);
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
    function onCodeUpgrade() internal override {
        tvm.resetStorage();
    }

}