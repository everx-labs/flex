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
import "../abstract/AXchgPair.sol";
import "../abstract/AFlexClient.sol";
import "../abstract/AFlexWallet.sol";
import "../interface/IFlexTip3Tip3Debot.sol";
import "../interface/IFlexDebot.sol";

struct PriceInfo {
       uint128 sell;
       uint128 buy;
       address addr;
    }

struct T3WDetails {
    uint256 rootPubkey;
    address root;
    string name;
    uint8 decimals;
    uint128 balance;
    string symbol;
    LendOwnership[] lend_ownership;
}

contract FlexTip3TonDebot is IFlexTip3Tip3Debot, Debot, Upgradable {

    uint8 constant MAJOR_DETAIL = 0;
    uint8 constant MINOR_DETAIL = 1;

    address m_sender;
    address m_stockAddr;
    address m_tip3majorRoot;
    address m_tip3minorRoot;
    string m_tip3tip3Name;
    uint32 m_signingBox;
    uint128 m_flexClientBalance;
    TvmCell m_sendMsg;
    TvmCell m_xchgPriceCode;
    address m_flexClient;
    address m_tip3majorWallet;
    address m_tip3minorWallet;
    address m_notifyAddr;
    mapping(uint8 => T3WDetails) m_tip3walletsDetails;
    uint128 m_dealPrice;
    uint128 m_dealAmount;
    uint32 m_dealTime;
    uint128 m_stockMinAmount;
    uint8 m_stockDealsLimit;
    FlexTonCfg m_flexTonCfg;
    uint32 m_getT3WDetailsCallback;
    bool m_bInstantBuy;
    mapping(uint128 => PriceInfo) m_prices;
    uint128[] m_arPrices;

    function start() public override {
        Terminal.print(0,"Please invoke me");
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

    function instantTradeTip3Tip3(address fc, uint32 signingBox, address notifyAddr, uint128 minAmount, uint8 dealsLimit, FlexTonCfg tonCfg, address stock, address t3major, address t3minor, address wt3major, address wt3minor, bool bBuy, uint128 price, uint128 volume) public override {
        m_flexClient = fc;
        m_signingBox = signingBox;
        m_notifyAddr = notifyAddr;
        m_stockMinAmount = minAmount;
        m_stockDealsLimit = dealsLimit;
        m_flexTonCfg = tonCfg;
        m_stockAddr = stock;
        m_tip3majorRoot = t3major;
        m_tip3minorRoot = t3minor;
        m_tip3majorWallet = wt3major;
        m_tip3minorWallet = wt3minor;
        m_sender = msg.sender;
        m_dealAmount = volume;
        m_dealPrice = price;
        m_bInstantBuy = bBuy;

        optional(uint256) none;
        AFlex(m_stockAddr).getXchgPriceCode{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setInstantPriceCode),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }(m_tip3majorRoot,m_tip3minorRoot);
    }

    function setInstantPriceCode(TvmCell value0) public {
        m_xchgPriceCode = value0;

        if (m_bInstantBuy)
            m_getT3WDetailsCallback = tvm.functionId(deployPriceWithBuy);
        else
            m_getT3WDetailsCallback = tvm.functionId(deployPriceWithSell);
        getFCT3WBalances();
    }

    function tradeTip3Tip3(address fc, uint32 signingBox, address notifyAddr, uint128 minAmount, uint8 dealsLimit, FlexTonCfg tonCfg, address stock, address t3major, address t3minor, address wt3major, address wt3minor) public override {
        m_flexClient = fc;
        m_signingBox = signingBox;
        m_notifyAddr = notifyAddr;
        m_stockMinAmount = minAmount;
        m_stockDealsLimit = dealsLimit;
        m_flexTonCfg = tonCfg;
        m_stockAddr = stock;
        m_tip3majorRoot = t3major;
        m_tip3minorRoot = t3minor;
        m_tip3majorWallet = wt3major;
        m_tip3minorWallet = wt3minor;
        m_sender = msg.sender;
        m_getT3WDetailsCallback = tvm.functionId(setT3WDetails);
        getFCT3WBalances();
    }

    //get flexclient balance
    function getFCT3WBalances() public {
        Sdk.getBalance(tvm.functionId(getT3WDetails), m_flexClient);
    }

    //get tip3 major wallet token balance
    function getT3WDetails(uint128 nanotokens) public {
        m_flexClientBalance = nanotokens;
        optional(uint256) none;
        AFlexWallet(m_tip3minorWallet).getDetails{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(getT3WMajorDetails),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }
    //get tip3 minor wallet token balance
    function getT3WMajorDetails(bytes name, bytes symbol, uint8 decimals, uint128 balance,
            uint256 root_public_key, uint256 wallet_public_key, address root_address,
            address owner_address, LendOwnership[] lend_ownership, uint128 lend_balance,
            TvmCell code, Allowance allowance, int8 workchain_id) public{
        wallet_public_key;owner_address;lend_balance;code;allowance;workchain_id;//disable compile warnings
        T3WDetails tip3walletDetails;
        tip3walletDetails.decimals = decimals;
        tip3walletDetails.balance = balance - lend_balance;
        tip3walletDetails.lend_ownership = lend_ownership;
        tip3walletDetails.symbol = symbol;
        tip3walletDetails.rootPubkey = root_public_key;
        tip3walletDetails.root = root_address;
        tip3walletDetails.name = name;
        m_tip3walletsDetails[MINOR_DETAIL] = tip3walletDetails;
        optional(uint256) none;
        AFlexWallet(m_tip3majorWallet).getDetails{
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

    function setT3WDetails(bytes name, bytes symbol, uint8 decimals, uint128 balance,
            uint256 root_public_key, uint256 wallet_public_key, address root_address,
            address owner_address, LendOwnership[] lend_ownership, uint128 lend_balance,
            TvmCell code, Allowance allowance, int8 workchain_id) public{
        wallet_public_key;owner_address;lend_balance;code;allowance;workchain_id;//disable compile warnings
        T3WDetails tip3walletDetails;
        tip3walletDetails.decimals = decimals;
        tip3walletDetails.balance = balance - lend_balance;
        tip3walletDetails.lend_ownership = lend_ownership;
        tip3walletDetails.symbol = symbol;
        tip3walletDetails.rootPubkey = root_public_key;
        tip3walletDetails.root = root_address;
        tip3walletDetails.name = name;
        m_tip3walletsDetails[MAJOR_DETAIL] = tip3walletDetails;
        Terminal.print(0,format("Major tip3 wallet is"));
        Terminal.print(0, format("{}",m_tip3majorWallet));
        Terminal.print(0,format("Minor tip3 wallet is"));
        Terminal.print(0, format("{}",m_tip3minorWallet));
        //get order book
        optional(uint256) none;
        AFlex(m_stockAddr).getXchgPriceCode{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setPriceCode),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }(m_tip3majorRoot,m_tip3minorRoot);
    }

    function setPriceCode(TvmCell value0) public {
        m_xchgPriceCode = value0;
        getPricesDataByHash();
    }

    function setUpdateT3WDetails(bytes name, bytes symbol, uint8 decimals, uint128 balance,
            uint256 root_public_key, uint256 wallet_public_key, address root_address,
            address owner_address, LendOwnership[] lend_ownership, uint128 lend_balance,
            TvmCell code, Allowance allowance, int8 workchain_id) public{
        wallet_public_key;owner_address;lend_balance;code;allowance;workchain_id;//disable compile warnings
        T3WDetails tip3walletDetails;
        tip3walletDetails.decimals = decimals;
        tip3walletDetails.balance = balance - lend_balance;
        tip3walletDetails.lend_ownership = lend_ownership;
        tip3walletDetails.symbol = symbol;
        tip3walletDetails.rootPubkey = root_public_key;
        tip3walletDetails.root = root_address;
        tip3walletDetails.name = name;
        m_tip3walletsDetails[MAJOR_DETAIL] = tip3walletDetails;
        getPricesDataByHash();
    }

    function getPricesDataByHash() public {
        uint256 h = tvm.hash(m_xchgPriceCode);
        Sdk.getAccountsDataByHash(tvm.functionId(onPricesByHash),h,address(0x0));
    }

    function onPricesByHash(AccData[] accounts) public {
        MenuItem[] item;
        mapping (uint128 => PriceInfo) empty;
        m_prices = empty;
        m_arPrices = new uint128[](0);
        string comment = format("Flex client balance: {:t}",m_flexClientBalance);
        comment.append(format("\nTip3 {} balance: ",m_tip3walletsDetails[MAJOR_DETAIL].symbol));
        comment.append(tokensToStr(m_tip3walletsDetails[MAJOR_DETAIL].balance,m_tip3walletsDetails[MAJOR_DETAIL].decimals));
        comment.append(format("\nTip3 {} balance: ",m_tip3walletsDetails[MINOR_DETAIL].symbol));
        comment.append(tokensToStr(m_tip3walletsDetails[MINOR_DETAIL].balance,m_tip3walletsDetails[MINOR_DETAIL].decimals));

        for (uint i=0; i<accounts.length;i++)
        {
            TvmSlice sl = accounts[i].data.toSlice();
            sl.decode(bool);
            (uint128 pn, uint128 pd, uint128 s, uint128 b) = sl.decode(uint128,uint128,uint128,uint128);
            if (pd!=(uint128(10)**uint128(m_tip3walletsDetails[MAJOR_DETAIL].decimals))) continue;
            m_prices[pn]=PriceInfo(s,b,accounts[i].id);
        }

        optional(uint128 , PriceInfo) registryPair = m_prices.max();
        while (registryPair.hasValue()) {
            (uint128 key, PriceInfo pi) = registryPair.get();
            string str = tokensToStr(key,m_tip3walletsDetails[MINOR_DETAIL].decimals);
            str += " | sell "+tokensToStr(pi.sell,m_tip3walletsDetails[MAJOR_DETAIL].decimals)+" | buy "+tokensToStr(pi.buy,m_tip3walletsDetails[MAJOR_DETAIL].decimals);
            item.push(MenuItem(str,"",tvm.functionId(menuQuickDeal)));
            m_arPrices.push(key);
            registryPair = m_prices.prev(key);
        }
        if (m_tip3walletsDetails[MINOR_DETAIL].balance>0)
            item.push(MenuItem(format("Buy {} for {}",m_tip3walletsDetails[MAJOR_DETAIL].symbol,m_tip3walletsDetails[MINOR_DETAIL].symbol),"",tvm.functionId(menuBuyTip3)));
        if (m_tip3walletsDetails[MAJOR_DETAIL].balance>0)
            item.push( MenuItem(format("Sell {} for {}",m_tip3walletsDetails[MAJOR_DETAIL].symbol,m_tip3walletsDetails[MINOR_DETAIL].symbol),"",tvm.functionId(menuSellTip3)));
        if (m_tip3walletsDetails[MINOR_DETAIL].lend_ownership.length>0)
            item.push(MenuItem("Cancel Buy order","",tvm.functionId(menuCancelBuyTip3)));
        if (m_tip3walletsDetails[MAJOR_DETAIL].lend_ownership.length>0)
            item.push(MenuItem("Cancel Sell order","",tvm.functionId(menuCancelSellTip3)));

        item.push(MenuItem("Update order book","",tvm.functionId(menuOrderBookUpdate)));
        item.push(MenuItem("Withdraw tip3","",tvm.functionId(menuOrderBookBurnTip3)));
        item.push(MenuItem("Back","",tvm.functionId(menuOrderBookBack)));

        Terminal.print(0,comment);

        Menu.select("Order Book","", item);
    }

    //cancel buy order
    function menuCancelBuyTip3(uint32 index) public {
        index;//disable compile warnings
        m_dealPrice = 0;
        m_dealTime = 1;
        showCancelOrderMenu(MINOR_DETAIL);
    }

    //cancel sell order
    function menuCancelSellTip3(uint32 index) public {
        index;//disable compile warnings
        m_dealPrice = 0;
        m_dealTime = 0;
        showCancelOrderMenu(MAJOR_DETAIL);
    }
    function showCancelOrderMenu(uint8 wallet) public{
        m_arPrices = new uint128[](0);
        MenuItem[] item;
        optional(uint128 , PriceInfo) registryPair = m_prices.max();
        while (registryPair.hasValue()) {
            (uint128 key, PriceInfo pi) = registryPair.get();
            for (uint i = 0; i < m_tip3walletsDetails[wallet].lend_ownership.length; ++i){
                if (m_tip3walletsDetails[wallet].lend_ownership[i].lend_addr == pi.addr){
                    m_arPrices.push(key);
                    item.push(MenuItem(format("Price {} Volume {}",tokensToStr(key,m_tip3walletsDetails[MINOR_DETAIL].decimals),tokensToStr(m_tip3walletsDetails[wallet].lend_ownership[i].lend_balance, m_tip3walletsDetails[wallet].decimals)),"",tvm.functionId(menuCancelSellTip3Order)));
                    break;
                }
            }
            registryPair = m_prices.prev(key);
        }
        item.push(MenuItem("Back","",tvm.functionId(menuOrderBookUpdate)));
        Menu.select("Cancel order","", item);
    }

     function menuCancelSellTip3Order(uint32 index) public {
        m_dealPrice = m_arPrices[index];
        m_getT3WDetailsCallback = tvm.functionId(cancelSellOrder);
        getFCT3WBalances();
     }

    function cancelSellOrder(bytes name, bytes symbol, uint8 decimals, uint128 balance,
            uint256 root_public_key, uint256 wallet_public_key, address root_address,
            address owner_address, LendOwnership[] lend_ownership, uint128 lend_balance,
            TvmCell code, Allowance allowance, int8 workchain_id) public{
        wallet_public_key;owner_address;lend_balance;code;allowance;workchain_id;//disable compile warnings
        T3WDetails tip3walletDetails;
        tip3walletDetails.decimals = decimals;
        tip3walletDetails.balance = balance - lend_balance;
        tip3walletDetails.lend_ownership = lend_ownership;
        tip3walletDetails.symbol = symbol;
        tip3walletDetails.rootPubkey = root_public_key;
        tip3walletDetails.root = root_address;
        tip3walletDetails.name = name;

        uint128 payout = 1000000000;
        if (payout>m_flexClientBalance) {
            Terminal.print(0,"Error: Flex client balance too low!\nPlease send tons to your flex client address:");
            Terminal.print(tvm.functionId(getPricesDataByHash),format("{}",m_flexClient));
            return;
        }
        uint128 priceNum = m_dealPrice;
        uint128 priceDenum = uint128(10)**uint128(m_tip3walletsDetails[MAJOR_DETAIL].decimals);
        bool bSell = true;
        if (m_dealTime==1) {bSell=false;}

        Tip3Cfg majoorTip3cfg;
        majoorTip3cfg.name=m_tip3walletsDetails[MAJOR_DETAIL].name;
        majoorTip3cfg.symbol=m_tip3walletsDetails[MAJOR_DETAIL].symbol;
        majoorTip3cfg.decimals=m_tip3walletsDetails[MAJOR_DETAIL].decimals;
        majoorTip3cfg.root_public_key=m_tip3walletsDetails[MAJOR_DETAIL].rootPubkey;
        majoorTip3cfg.root_address=m_tip3walletsDetails[MAJOR_DETAIL].root;

        Tip3Cfg minorTip3cfg;
        minorTip3cfg.name=m_tip3walletsDetails[MINOR_DETAIL].name;
        minorTip3cfg.symbol=m_tip3walletsDetails[MINOR_DETAIL].symbol;
        minorTip3cfg.decimals=m_tip3walletsDetails[MINOR_DETAIL].decimals;
        minorTip3cfg.root_public_key=m_tip3walletsDetails[MINOR_DETAIL].rootPubkey;
        minorTip3cfg.root_address=m_tip3walletsDetails[MINOR_DETAIL].root;

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
            call: {AFlexClient.cancelXchgOrder, bSell, priceNum, priceDenum, m_stockMinAmount,m_stockDealsLimit,payout,m_xchgPriceCode,majoorTip3cfg,minorTip3cfg,m_notifyAddr}
        });
        string pr = tokensToStr(m_dealPrice,m_tip3walletsDetails[MINOR_DETAIL].decimals);
        string str = "sell";
        if (m_dealTime==1) {str="buy";}
        ConfirmInput.get(tvm.functionId(confirmCancelSellOrder),format("Cancel {} order at price {}\nProcessing value {:t} ton. Unused part will be returned. Continue?",str ,pr, payout));
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

    //Quick Deal
    function menuQuickDeal(uint32 index) public {
        index;//disable compile warnings
        m_dealPrice = m_arPrices[index];
        PriceInfo pi = m_prices[m_dealPrice];
        if (pi.sell!=0 && pi.buy!=0) {
            Terminal.print(tvm.functionId(getPricesDataByHash),"Fast trade is not supported at volatile price levels");
        }else if (pi.sell!=0) {
            if (m_tip3walletsDetails[MINOR_DETAIL].balance>0){
                uint128 max_val = math.min(pi.sell,m_tip3walletsDetails[MINOR_DETAIL].balance);
                AmountInput.get(tvm.functionId(getBuyQuickDealAmount), format("Amount of {} tip3 to 2 for {} tip3:",m_tip3walletsDetails[MAJOR_DETAIL].symbol,m_tip3walletsDetails[MINOR_DETAIL].symbol),m_tip3walletsDetails[MAJOR_DETAIL].decimals,m_stockMinAmount,max_val);
                //AmountInput.get(tvm.functionId(getSellQuickDealAmount), "Amount of tip3 to sell:",m_tip3walletDetails.decimals,m_stockMinAmount,max_val);
            } else {Terminal.print(tvm.functionId(getPricesDataByHash),"You haven't tip3 tokens to buy.");}
        }else if (pi.buy!=0) {
            if (m_tip3walletsDetails[MAJOR_DETAIL].balance>0){
                uint128 max_val = math.min(pi.buy,m_tip3walletsDetails[MAJOR_DETAIL].balance);
                AmountInput.get(tvm.functionId(getSellQuickDealAmount), format("Amount of {} tip3 to sell for {} tip3:",m_tip3walletsDetails[MAJOR_DETAIL].symbol,m_tip3walletsDetails[MINOR_DETAIL].symbol),m_tip3walletsDetails[MAJOR_DETAIL].decimals,m_stockMinAmount,max_val);
                //AmountInput.get(tvm.functionId(getSellQuickDealAmount), "Amount of tip3 to sell:",m_tip3walletDetails.decimals,m_stockMinAmount,max_val);
            } else {Terminal.print(tvm.functionId(getPricesDataByHash),"You haven't tip3 tokens to sell.");}
        }
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
        index;//disable compile warnings
        m_getT3WDetailsCallback = tvm.functionId(setUpdateT3WDetails);
        getFCT3WBalances();
    }

    //back
    function menuOrderBookBack(uint32 index) public {
        index;//disable compile warnings
        IFlexDebot(m_sender).updateTradingPairs();
        m_sender = address(0);
    }

    //withdraw tip3
    function menuOrderBookBurnTip3(uint32 index) public {
        index;//disable compile warning
        MenuItem[] item;
        item.push(MenuItem(format("Withdraw {} tip3",m_tip3walletsDetails[MAJOR_DETAIL].symbol),"",tvm.functionId(menuOrderBookBurnMajor)));
        item.push(MenuItem(format("Withdraw {} tip3",m_tip3walletsDetails[MINOR_DETAIL].symbol),"",tvm.functionId(menuOrderBookBurnMinor)));
        item.push(MenuItem("Back","",tvm.functionId(menuOrderBookUpdate)));
        Menu.select("Withdraw tip3","", item);
    }

    function menuOrderBookBurnMajor(uint32 index) public {
        index;//disable compile warning
        IFlexDebot(m_sender).burnTip3Wallet(m_flexClient,m_signingBox,m_tip3majorWallet);
        m_sender = address(0);
    }

    function menuOrderBookBurnMinor(uint32 index) public {
        index;//disable compile warning
        IFlexDebot(m_sender).burnTip3Wallet(m_flexClient,m_signingBox,m_tip3minorWallet);
        m_sender = address(0);
    }

    //buy
    function menuBuyTip3(uint32 index) public {
        index;//disable compile warnings
        AmountInput.get(tvm.functionId(getDealAmmount), format("Amount of {} tip3 to buy for {} tip3:",m_tip3walletsDetails[MAJOR_DETAIL].symbol,m_tip3walletsDetails[MINOR_DETAIL].symbol),m_tip3walletsDetails[MAJOR_DETAIL].decimals,m_stockMinAmount,0xFFFFFFFF);
    }
    function getDealAmmount(uint128 value) public{
       m_dealAmount = value;
       AmountInput.get(tvm.functionId(getDealPrice), format("Price in {} tip3 for 1 {} tip3:",m_tip3walletsDetails[MINOR_DETAIL].symbol,m_tip3walletsDetails[MAJOR_DETAIL].symbol),m_tip3walletsDetails[MINOR_DETAIL].decimals,1,0xFFFFFFFF);
    }
    function getDealPrice(uint128 value) public{
        m_dealPrice = value;
        AmountInput.get(tvm.functionId(getBuyDealTime), "Order time in hours:",0,1,8640);
    }

    function getBuyDealTime(uint128 value) public{
       m_dealTime =  uint32(value);
       m_dealTime =uint32(now + m_dealTime * 60 * 60);
       m_getT3WDetailsCallback = tvm.functionId(deployPriceWithBuy);
       getFCT3WBalances();
    }

    function deployPriceWithBuy(bytes name, bytes symbol, uint8 decimals, uint128 balance,
            uint256 root_public_key, uint256 wallet_public_key, address root_address,
            address owner_address, LendOwnership[] lend_ownership, uint128 lend_balance,
            TvmCell code, Allowance allowance, int8 workchain_id) public{
        wallet_public_key;owner_address;lend_balance;code;allowance;workchain_id;//disable compile warnings
        T3WDetails tip3walletDetails;
        tip3walletDetails.decimals = decimals;
         tip3walletDetails.balance = balance - lend_balance;
        tip3walletDetails.lend_ownership = lend_ownership;
        tip3walletDetails.symbol = symbol;
        tip3walletDetails.rootPubkey = root_public_key;
        tip3walletDetails.root = root_address;
        tip3walletDetails.name = name;
        m_tip3walletsDetails[MAJOR_DETAIL] = tip3walletDetails;
        //m_dealPrice = price2Flex(m_dealTons, m_tip3walletDetails.decimals);
        uint128 priceNum = m_dealPrice;
        uint128 priceDenum = uint128(10)**uint128(m_tip3walletsDetails[MAJOR_DETAIL].decimals);
        uint128 minorAmount = math.muldivr(1, priceNum, priceDenum);
        if (minorAmount<1)
        {
            Terminal.print(tvm.functionId(getPricesDataByHash),"Error: too low price! Cancelled!");
            return;
        }
        minorAmount = math.muldivr(m_dealAmount, priceNum, priceDenum);
        if (minorAmount>m_tip3walletsDetails[MINOR_DETAIL].balance)
        {
            Terminal.print(tvm.functionId(getPricesDataByHash),format("Error: you need {} tokens on your minor balance! Cancelled!",minorAmount));
            return;
        }
        uint128 payout = m_flexTonCfg.process_queue + m_flexTonCfg.transfer_tip3 + m_flexTonCfg.send_notify +
          m_flexTonCfg.return_ownership + m_flexTonCfg.order_answer + 1000000000;

        Tip3Cfg majoorTip3cfg;
        majoorTip3cfg.name=m_tip3walletsDetails[MAJOR_DETAIL].name;
        majoorTip3cfg.symbol=m_tip3walletsDetails[MAJOR_DETAIL].symbol;
        majoorTip3cfg.decimals=m_tip3walletsDetails[MAJOR_DETAIL].decimals;
        majoorTip3cfg.root_public_key=m_tip3walletsDetails[MAJOR_DETAIL].rootPubkey;
        majoorTip3cfg.root_address=m_tip3walletsDetails[MAJOR_DETAIL].root;

        Tip3Cfg minorTip3cfg;
        minorTip3cfg.name=m_tip3walletsDetails[MINOR_DETAIL].name;
        minorTip3cfg.symbol=m_tip3walletsDetails[MINOR_DETAIL].symbol;
        minorTip3cfg.decimals=m_tip3walletsDetails[MINOR_DETAIL].decimals;
        minorTip3cfg.root_public_key=m_tip3walletsDetails[MINOR_DETAIL].rootPubkey;
        minorTip3cfg.root_address=m_tip3walletsDetails[MINOR_DETAIL].root;

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
            call: {AFlexClient.deployPriceXchg, false,priceNum,priceDenum,m_dealAmount,minorAmount,m_dealTime,m_stockMinAmount,m_stockDealsLimit,payout,
                            m_xchgPriceCode,m_tip3minorWallet,m_tip3majorWallet,majoorTip3cfg,minorTip3cfg,m_notifyAddr}
        });
        ConfirmInput.get(tvm.functionId(confirmDeployPriceWithBuy),format("It will cost {:t} TONs. Unused part will be returned. Continue?",payout));
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

    //sell done
    function menuSellTip3(uint32 index) public {
        index;//disable compile warnings
        AmountInput.get(tvm.functionId(getSellDealAmmount), format("Amount of {} tip3 to sell for {} tip3:",m_tip3walletsDetails[MAJOR_DETAIL].symbol,m_tip3walletsDetails[MINOR_DETAIL].symbol),m_tip3walletsDetails[MAJOR_DETAIL].decimals,m_stockMinAmount,m_tip3walletsDetails[MAJOR_DETAIL].balance);
    }

    function getSellDealAmmount(uint128 value) public{
       m_dealAmount = value;
       AmountInput.get(tvm.functionId(getSellDealPrice), format("Price in {} tip3 for 1 {} tip3:",m_tip3walletsDetails[MINOR_DETAIL].symbol,m_tip3walletsDetails[MAJOR_DETAIL].symbol),m_tip3walletsDetails[MINOR_DETAIL].decimals,1,0xFFFFFFFF);
    }
    function getSellDealPrice(uint128 value) public{
        m_dealPrice = value;
        AmountInput.get(tvm.functionId(getSellDealTime), "Order time in hours:",0,1,0xFF);
    }

    function getSellDealTime(uint128 value) public{
       m_dealTime =  uint32(value);
       m_dealTime =uint32(now + m_dealTime * 60 * 60);
       m_getT3WDetailsCallback = tvm.functionId(deployPriceWithSell);
       getFCT3WBalances();
    }

    function deployPriceWithSell(bytes name, bytes symbol, uint8 decimals, uint128 balance,
            uint256 root_public_key, uint256 wallet_public_key, address root_address,
            address owner_address, LendOwnership[] lend_ownership, uint128 lend_balance,
            TvmCell code, Allowance allowance, int8 workchain_id) public{
        wallet_public_key;owner_address;lend_balance;code;allowance;workchain_id;//disable compile warnings
        T3WDetails tip3walletDetails;
        tip3walletDetails.decimals = decimals;
        tip3walletDetails.balance = balance - lend_balance;
        tip3walletDetails.lend_ownership = lend_ownership;
        tip3walletDetails.symbol = symbol;
        tip3walletDetails.rootPubkey = root_public_key;
        tip3walletDetails.root = root_address;
        tip3walletDetails.name = name;
        m_tip3walletsDetails[MAJOR_DETAIL] = tip3walletDetails;
        uint128 priceNum = m_dealPrice;
        uint128 priceDenum = uint128(10)**uint128(m_tip3walletsDetails[MAJOR_DETAIL].decimals);
        uint128 minorAmount = math.muldivr(1, priceNum, priceDenum);
        if (minorAmount<1)
        {
            Terminal.print(tvm.functionId(getPricesDataByHash),"Error: too low price! Cancelled!");
            return;
        }

        if (m_dealAmount>m_tip3walletsDetails[MAJOR_DETAIL].balance)
        {
            Terminal.print(tvm.functionId(getPricesDataByHash),format("Error: you need {} tokens on your major balance! Cancelled!",m_dealAmount));
            return;
        }

        uint128 payout = m_flexTonCfg.process_queue + m_flexTonCfg.transfer_tip3 + m_flexTonCfg.send_notify +
          m_flexTonCfg.return_ownership + m_flexTonCfg.order_answer + 1000000000;

        Tip3Cfg majoorTip3cfg;
        majoorTip3cfg.name=m_tip3walletsDetails[MAJOR_DETAIL].name;
        majoorTip3cfg.symbol=m_tip3walletsDetails[MAJOR_DETAIL].symbol;
        majoorTip3cfg.decimals=m_tip3walletsDetails[MAJOR_DETAIL].decimals;
        majoorTip3cfg.root_public_key=m_tip3walletsDetails[MAJOR_DETAIL].rootPubkey;
        majoorTip3cfg.root_address=m_tip3walletsDetails[MAJOR_DETAIL].root;

        Tip3Cfg minorTip3cfg;
        minorTip3cfg.name=m_tip3walletsDetails[MINOR_DETAIL].name;
        minorTip3cfg.symbol=m_tip3walletsDetails[MINOR_DETAIL].symbol;
        minorTip3cfg.decimals=m_tip3walletsDetails[MINOR_DETAIL].decimals;
        minorTip3cfg.root_public_key=m_tip3walletsDetails[MINOR_DETAIL].rootPubkey;
        minorTip3cfg.root_address=m_tip3walletsDetails[MINOR_DETAIL].root;

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
            call: {AFlexClient.deployPriceXchg, true,priceNum,priceDenum,m_dealAmount,m_dealAmount,m_dealTime,m_stockMinAmount,m_stockDealsLimit,payout,
                            m_xchgPriceCode,m_tip3majorWallet,m_tip3minorWallet,majoorTip3cfg,minorTip3cfg,m_notifyAddr}
        });
        ConfirmInput.get(tvm.functionId(confirmDeployPriceWithBuy),format("It will cost {:t} TONs. Unused part will be returned. Continue?",payout));
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
        value0;//disable compile warnings
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
        name = "Flex TIP3/TIP3 Debot";
        version = "0.2.3";
        publisher = "TON Labs";
        key = "Trade TIP3 for TIP3";
        author = "TON Labs";
        support = address.makeAddrStd(0, 0x0);
        hello = "Hello, i'am a Flex Tip3Tip3 DeBot.";
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