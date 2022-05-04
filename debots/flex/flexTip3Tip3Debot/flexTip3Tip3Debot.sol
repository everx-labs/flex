pragma ton-solidity >=0.40.0;
pragma AbiHeader expire;
pragma AbiHeader time;
pragma AbiHeader pubkey;
// import required DeBot interfaces and basic DeBot contract.
import "https://raw.githubusercontent.com/tonlabs/debots/main/Debot.sol";
import "DeBotInterfaces.sol";
import "../abstract/Upgradable.sol";
import "../abstract/AFlex.sol";
import "../abstract/AXchgPair.sol";
import "../abstract/AFlexClient.sol";
import "../abstract/AFlexWallet.sol";
import "../interface/IFlexTip3Tip3Debot.sol";
import "../interface/IFlexDebot.sol";
import "../interface/Msg.sol";

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
    lend_owner_array_record[] lend_owners;
}

contract FlexTip3Tip3Debot is IFlexTip3Tip3Debot, Debot, Upgradable {
    using JsonLib for JsonLib.Value;
    using JsonLib for mapping(uint256 => TvmCell);

    uint8 constant MAJOR_DETAIL = 0;
    uint8 constant MINOR_DETAIL = 1;

    address m_sender;
    uint256 m_masterPubkey;
    address m_tradingPair;
    uint32 m_signingBox;
    uint128 m_flexClientBalance;
    TvmCell m_sendMsg;
    TvmCell m_xchgPriceCode;
    TvmCell m_xchgPriceCodeUnsalted;
    TvmCell m_xchgPriceSalt;
    address m_flexClient;
    address m_tip3majorWallet;
    address m_tip3minorWallet;
    address m_flex;
    mapping(uint8 => T3WDetails) m_tip3walletsDetails;
    uint128 m_dealPrice;
    uint128 m_dealAmount;
    uint32 m_dealTime;
    uint128 m_stockMinAmount;
    uint32 m_getT3WDetailsCallback;
    mapping(uint128 => PriceInfo) m_prices;
    uint128[] m_arPrices;
    AccData[] m_arAccounts;
    bool m_bSell;
    uint128 m_orderLendBalance;
    uint128 m_priceDenom;
    uint8 m_priceDecimals;

    function setABIBytes(bytes dabi) public {
        require(tvm.pubkey() == msg.pubkey(), 100);
        tvm.accept();
        m_options |= DEBOT_ABI;
        m_debotAbi = dabi;
    }

    function start() public override {
        Terminal.print(0,"Invoke me!");
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

    function denomToDecimal(uint128 denom) public pure returns(uint8) {
        uint8 dec = 0;
        while (denom>0) {
            (uint128 d, uint128 r) = math.divmod(denom, uint128(10));
            denom = d;
            if (r==0) {
                dec++;
            } else if (r!=1) {
                Terminal.print(0,"[ERROR] unexpected price denominator");
            }
        }
        return dec;
    }

    function tradeTip3Tip3(address flex, address fc, uint32 signingBox, uint256 pubkey, uint128 minAmount,/*address notifyAddr, uint8 dealsLimit, EversConfig crystalCfg, address stock,*/ address tradingPair, address wt3major, address wt3minor) public override {
        m_flex = flex;
        m_flexClient = fc;
        m_signingBox = signingBox;
        m_masterPubkey = pubkey;
        m_stockMinAmount = minAmount;
        m_tradingPair = tradingPair;
        m_tip3majorWallet = wt3major;
        m_tip3minorWallet = wt3minor;
        m_sender = msg.sender;
        m_getT3WDetailsCallback = tvm.functionId(setT3WDetails);
        optional(uint256) none;
        AXchgPair(m_tradingPair).getPriceDenum{
            callbackId: tvm.functionId(setPriceDenum),
            onErrorId: tvm.functionId(onGetMethodError),
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }().extMsg;
    }

    function setPriceDenum(uint128 value0) public   {
        m_priceDenom = value0;
        m_priceDecimals = denomToDecimal(m_priceDenom);
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
            callbackId: getT3WMajorDetails,
            onErrorId: onGetMethodError,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }().extMsg;
    }
    //get tip3 minor wallet token balance
    function getT3WMajorDetails(string name, string symbol, uint8 decimals, uint128 balance,
            uint256 root_public_key, address root_address, uint256 wallet_public_key,
            address owner_address,
            optional(lend_pubkey_array_record) lend_pubkey,
            lend_owner_array_record[] lend_owners,
            optional(restriction_info) restriction,
            uint128 lend_balance,
            uint256 code_hash,
            uint16 code_depth,
            Allowance allowance,
            int8 workchain_id) public{
        wallet_public_key;owner_address;lend_balance;code_hash;allowance;workchain_id;//disable compile warnings
        T3WDetails tip3walletDetails;
        tip3walletDetails.decimals = decimals;
        tip3walletDetails.balance = balance - lend_balance;
        tip3walletDetails.lend_owners = lend_owners;
        tip3walletDetails.symbol = symbol;
        tip3walletDetails.rootPubkey = root_public_key;
        tip3walletDetails.root = root_address;
        tip3walletDetails.name = name;
        m_tip3walletsDetails[MINOR_DETAIL] = tip3walletDetails;
        optional(uint256) none;
        AFlexWallet(m_tip3majorWallet).getDetails{
            callbackId: m_getT3WDetailsCallback,
            onErrorId: onGetMethodError,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }().extMsg;
    }

    function setT3WDetails(string name, string symbol, uint8 decimals, uint128 balance,
            uint256 root_public_key, address root_address, uint256 wallet_public_key,
            address owner_address,
            optional(lend_pubkey_array_record) lend_pubkey,
            lend_owner_array_record[] lend_owners,
            optional(restriction_info) restriction,
            uint128 lend_balance,
            uint256 code_hash,
            uint16 code_depth,
            Allowance allowance,
            int8 workchain_id) public{
        wallet_public_key;owner_address;lend_balance;code_hash;allowance;workchain_id;//disable compile warnings
        T3WDetails tip3walletDetails;
        tip3walletDetails.decimals = decimals;
        tip3walletDetails.balance = balance - lend_balance;
        tip3walletDetails.lend_owners = lend_owners;
        tip3walletDetails.symbol = symbol;
        tip3walletDetails.rootPubkey = root_public_key;
        tip3walletDetails.root = root_address;
        tip3walletDetails.name = name;
        m_tip3walletsDetails[MAJOR_DETAIL] = tip3walletDetails;
        Terminal.print(0,format("Major tip3 wallet is"));
        Terminal.print(0, format("{}",m_tip3majorWallet));
        Terminal.print(0,format("Minor tip3 wallet is"));
        Terminal.print(0, format("{}",m_tip3minorWallet));

        optional(uint256) none;
        AXchgPair(m_tradingPair).getPriceXchgCode{
            callbackId: tvm.functionId(setPriceCode),
            onErrorId: tvm.functionId(onGetMethodError),
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }(true).extMsg;
    }

    function onGetMethodError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("Get method error. Sdk error = {}, Error code = {}",sdkError, exitCode));
    }

    function setPriceCode(TvmCell value0) public {
        m_xchgPriceCode = value0;
        optional(uint256) none;
        AXchgPair(m_tradingPair).getPriceXchgCode{
            callbackId: tvm.functionId(setPriceCodeUnsalted),
            onErrorId: tvm.functionId(onGetMethodError),
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }(false).extMsg;
    }

    function setPriceCodeUnsalted(TvmCell value0) public {
        m_xchgPriceCodeUnsalted = value0;
        optional(uint256) none;
        AXchgPair(m_tradingPair).getPriceXchgSalt{
            callbackId: tvm.functionId(setPriceSalt),
            onErrorId: tvm.functionId(onGetMethodError),
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }().extMsg;
    }

    function setPriceSalt(TvmCell value0) public {
        m_xchgPriceSalt = value0;
        getPricesDataByHash();
    }

    function setUpdateT3WDetails(string name, string symbol, uint8 decimals, uint128 balance,
            uint256 root_public_key, address root_address, uint256 wallet_public_key,
            address owner_address,
            optional(lend_pubkey_array_record) lend_pubkey,
            lend_owner_array_record[] lend_owners,
            optional(restriction_info) restriction,
            uint128 lend_balance,
            uint256 code_hash,
            uint16 code_depth,
            Allowance allowance,
            int8 workchain_id) public{
        wallet_public_key;owner_address;lend_balance;code_hash;allowance;workchain_id;//disable compile warnings
        T3WDetails tip3walletDetails;
        tip3walletDetails.decimals = decimals;
        tip3walletDetails.balance = balance - lend_balance;
        tip3walletDetails.lend_owners = lend_owners;
        tip3walletDetails.symbol = symbol;
        tip3walletDetails.rootPubkey = root_public_key;
        tip3walletDetails.root = root_address;
        tip3walletDetails.name = name;
        m_tip3walletsDetails[MAJOR_DETAIL] = tip3walletDetails;
        getPricesDataByHash();
    }

    function getPricesDataByHash() public {
        uint256 h = tvm.hash(m_xchgPriceCode);
        m_arAccounts = new AccData[](0);
        //next
        Sdk.getAccountsDataByHash(tvm.functionId(onPricesByHash),h,address(0x0));
    }

    function onPricesByHash(AccData[] accounts) public {
        for (AccData e : accounts){
            m_arAccounts.push(e);
        }

        this.onPricesByHash1(accounts.length);
    }

    function onPricesByHash1(uint len) public {
        if (len==50) {
            uint256 h = tvm.hash(m_xchgPriceCode);
            Sdk.getAccountsDataByHash(tvm.functionId(onPricesByHash),h,m_arAccounts[m_arAccounts.length-1].id);
        }else{
            this.onPricesByHashMenu(m_arAccounts);
        }
    }

    function onPricesByHashMenu(AccData[] accounts) public {
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
            (uint128 pn, uint128 s, uint128 b) = sl.decode(uint128,uint128,uint128);
            m_prices[pn]=PriceInfo(s,b,accounts[i].id);
        }

        optional(uint128 , PriceInfo) registryPair = m_prices.max();
        while (registryPair.hasValue()) {
            (uint128 key, PriceInfo pi) = registryPair.get();

            string str = tokensToStr(key,m_priceDecimals);
            str += " | sell "+tokensToStr(pi.sell,m_tip3walletsDetails[MAJOR_DETAIL].decimals)+" | buy "+tokensToStr(pi.buy,m_tip3walletsDetails[MAJOR_DETAIL].decimals);
            item.push(MenuItem(str,"",tvm.functionId(menuQuickDeal)));
            m_arPrices.push(key);
            registryPair = m_prices.prev(key);
        }

        if (m_tip3walletsDetails[MINOR_DETAIL].balance>0)
            item.push(MenuItem(format("Buy {} for {}",m_tip3walletsDetails[MAJOR_DETAIL].symbol,m_tip3walletsDetails[MINOR_DETAIL].symbol),"",tvm.functionId(menuBuyTip3)));
        if (m_tip3walletsDetails[MAJOR_DETAIL].balance>0)
            item.push( MenuItem(format("Sell {} for {}",m_tip3walletsDetails[MAJOR_DETAIL].symbol,m_tip3walletsDetails[MINOR_DETAIL].symbol),"",tvm.functionId(menuSellTip3)));
        if (m_tip3walletsDetails[MINOR_DETAIL].lend_owners.length>0)
            item.push(MenuItem("Cancel Buy order","",tvm.functionId(menuCancelBuyTip3)));
        if (m_tip3walletsDetails[MAJOR_DETAIL].lend_owners.length>0)
            item.push(MenuItem("Cancel Sell order","",tvm.functionId(menuCancelSellTip3)));

        item.push(MenuItem("Update order book","",tvm.functionId(menuOrderBookUpdate)));
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
            for (uint i = 0; i < m_tip3walletsDetails[wallet].lend_owners.length; ++i){
                address addrStd = address.makeAddrStd(m_tip3walletsDetails[wallet].lend_owners[i].lend_key.dest.workchain_id, m_tip3walletsDetails[wallet].lend_owners[i].lend_key.dest.addr);
                if (addrStd == pi.addr){
                    m_arPrices.push(key);
                    item.push(MenuItem(format("Price {} Volume {}",tokensToStr(key,m_priceDecimals),tokensToStr(m_tip3walletsDetails[wallet].lend_owners[i].lend_balance, m_tip3walletsDetails[wallet].decimals)),"",tvm.functionId(menuCancelSellTip3Order)));
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

    function cancelSellOrder(string name, string symbol, uint8 decimals, uint128 balance,
            uint256 root_public_key, address root_address, uint256 wallet_public_key,
            address owner_address,
            optional(lend_pubkey_array_record) lend_pubkey,
            lend_owner_array_record[] lend_owners,
            optional(restriction_info) restriction,
            uint128 lend_balance,
            uint256 code_hash,
            uint16 code_depth,
            Allowance allowance,
            int8 workchain_id) public{
        wallet_public_key;owner_address;lend_balance;code_hash;allowance;workchain_id;//disable compile warnings

        uint128 payout = 1000000000;
        if (payout>m_flexClientBalance) {
            Terminal.print(0,"Error: Flex client balance too low!\nPlease send evers to your flex client address:");
            Terminal.print(tvm.functionId(getPricesDataByHash),format("{}",m_flexClient));
            return;
        }
        uint128 priceNum = m_dealPrice;
        uint128 priceDenum = uint128(10)**uint128(m_tip3walletsDetails[MAJOR_DETAIL].decimals);
        bool bSell = true;
        if (m_dealTime==1) {bSell=false;}

        optional(uint256) userId;
        userId.set(m_masterPubkey);
        optional(uint256) none;
        m_sendMsg =  tvm.buildExtMsg({
            dest: m_flexClient,
            callbackId: tvm.functionId(onSellOrderCancelSuccess),
            onErrorId: tvm.functionId(onSellOrderCancelError),
            time: 0,
            expire: 0,
            sign: true,
            signBoxHandle: m_signingBox,
            pubkey: none,
            call: {AFlexClient.cancelXchgOrder, bSell, priceNum, payout,m_xchgPriceCode,userId,none}
        });
        string pr = tokensToStr(m_dealPrice,m_tip3walletsDetails[MINOR_DETAIL].decimals);
        string str = "sell";
        if (m_dealTime==1) {str="buy";}
        ConfirmInput.get(tvm.functionId(confirmCancelSellOrder),format("Cancel {} order at price {}\nProcessing value {:t} ever. Unused part will be returned. Continue?",str ,pr, payout));
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
                AmountInput.get(tvm.functionId(getBuyQuickDealAmount), format("Amount of {} tip3 to buy for {} tip3:",m_tip3walletsDetails[MAJOR_DETAIL].symbol,m_tip3walletsDetails[MINOR_DETAIL].symbol),m_tip3walletsDetails[MAJOR_DETAIL].decimals,m_stockMinAmount,max_val);
            } else {Terminal.print(tvm.functionId(getPricesDataByHash),"You haven't tip3 tokens to buy.");}
        }else if (pi.buy!=0) {
            if (m_tip3walletsDetails[MAJOR_DETAIL].balance>0){
                uint128 max_val = math.min(pi.buy,m_tip3walletsDetails[MAJOR_DETAIL].balance);
                AmountInput.get(tvm.functionId(getSellQuickDealAmount), format("Amount of {} tip3 to sell for {} tip3:",m_tip3walletsDetails[MAJOR_DETAIL].symbol,m_tip3walletsDetails[MINOR_DETAIL].symbol),m_tip3walletsDetails[MAJOR_DETAIL].decimals,m_stockMinAmount,max_val);
            } else {Terminal.print(tvm.functionId(getPricesDataByHash),"You haven't tip3 tokens to sell.");}
        }
    }

    function getBuyQuickDealAmount(uint128 value) public {
        m_dealAmount = value;
        //1 hour
        m_dealTime =uint32(now + 3600);
        m_bSell = false;
        optional(uint256) none;
        AFlex(m_flex).calcLendTokensForOrder{
            callbackId: setLendTokensForOrder,
            onErrorId: onGetMethodError,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }(m_bSell,m_dealAmount,PriceTuple(m_dealPrice,m_priceDenom)).extMsg;
    }

    function getSellQuickDealAmount(uint128 value) public {
        m_dealAmount = value;
        //1 hour
        m_dealTime =uint32(now + 3600);
        m_bSell = true;
        optional(uint256) none;
        AFlex(m_flex).calcLendTokensForOrder{
            callbackId: setLendTokensForOrder,
            onErrorId: onGetMethodError,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }(m_bSell,m_dealAmount,PriceTuple(m_dealPrice,m_priceDenom)).extMsg;
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

 //sell done
    function menuSellTip3(uint32 index) public {
        index;//disable compile warnings
        m_bSell = true;
        AmountInput.get(tvm.functionId(getDealAmmount), format("Amount of {} tip3 to sell for {} tip3:",m_tip3walletsDetails[MAJOR_DETAIL].symbol,m_tip3walletsDetails[MINOR_DETAIL].symbol),m_tip3walletsDetails[MAJOR_DETAIL].decimals,m_stockMinAmount,m_tip3walletsDetails[MAJOR_DETAIL].balance);
    }

    //buy
    function menuBuyTip3(uint32 index) public {
        index;//disable compile warnings
        m_bSell = false;
        uint128 max = 50000*(10**uint128(m_tip3walletsDetails[MAJOR_DETAIL].decimals));

        AmountInput.get(tvm.functionId(getDealAmmount), format("Amount of {} tip3 to buy for {} tip3:",m_tip3walletsDetails[MAJOR_DETAIL].symbol,m_tip3walletsDetails[MINOR_DETAIL].symbol),m_tip3walletsDetails[MAJOR_DETAIL].decimals,m_stockMinAmount,max);
    }
    function getDealAmmount(uint128 value) public{
       m_dealAmount = value;
       AmountInput.get(tvm.functionId(getDealPrice), format("Price in {} tip3 for 1 {} tip3:",m_tip3walletsDetails[MINOR_DETAIL].symbol,m_tip3walletsDetails[MAJOR_DETAIL].symbol),m_priceDecimals,1,0xFFFFFFFF);
    }
    function getDealPrice(uint128 value) public{
        m_dealPrice = value;
        AmountInput.get(tvm.functionId(getBuyDealTime), "Order time in hours:",0,1,8640);
    }

    function getBuyDealTime(uint128 value) public{
        m_dealTime =  uint32(value);
        m_dealTime =uint32(now + m_dealTime * 60 * 60);
        optional(uint256) none;
        AFlex(m_flex).calcLendTokensForOrder{
            callbackId: setLendTokensForOrder,
            onErrorId: onGetMethodError,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }(m_bSell,m_dealAmount,PriceTuple(m_dealPrice,m_priceDenom)).extMsg;
    }

    function setLendTokensForOrder(uint128 value0) public {
        m_orderLendBalance = value0;
        m_getT3WDetailsCallback = tvm.functionId(deployPriceOperation);
        getFCT3WBalances();
    }

    function deployPriceOperation(string name, string symbol, uint8 decimals, uint128 balance,
            uint256 root_public_key, address root_address, uint256 wallet_public_key,
            address owner_address,
            optional(lend_pubkey_array_record) lend_pubkey,
            lend_owner_array_record[] lend_owners,
            optional(restriction_info) restriction,
            uint128 lend_balance,
            uint256 code_hash,
            uint16 code_depth,
            Allowance allowance,
            int8 workchain_id) public{
        wallet_public_key;owner_address;lend_balance;code_hash;allowance;workchain_id;//disable compile warnings
        T3WDetails tip3walletDetails;
        tip3walletDetails.decimals = decimals;
         tip3walletDetails.balance = balance - lend_balance;
        tip3walletDetails.lend_owners = lend_owners;
        tip3walletDetails.symbol = symbol;
        tip3walletDetails.rootPubkey = root_public_key;
        tip3walletDetails.root = root_address;
        tip3walletDetails.name = name;
        m_tip3walletsDetails[MAJOR_DETAIL] = tip3walletDetails;
        uint128 priceNum = m_dealPrice;
        uint128 minorAmount;

        if (m_bSell){
            minorAmount = m_dealAmount;
            if (m_orderLendBalance>m_tip3walletsDetails[MAJOR_DETAIL].balance)
            {
                Terminal.print(tvm.functionId(getPricesDataByHash),format("Error: you need {} tokens on your major balance! Cancelled!",m_dealAmount));
                return;
            }
        }else {
            minorAmount = math.muldivr(m_dealAmount, priceNum, m_priceDenom);
            if (m_orderLendBalance>m_tip3walletsDetails[MINOR_DETAIL].balance)
            {
                Terminal.print(tvm.functionId(getPricesDataByHash),format("Error: you need {} tokens on your minor balance! Cancelled!",minorAmount));
                return;
            }
        }

        uint128 payout = 3 ever;
        uint256 orderId = now;
        //mock
        uint256 userId = m_masterPubkey;

        address fromWallet;
        address toWallet;
        if (m_bSell){
            fromWallet = m_tip3majorWallet;
            toWallet = m_tip3minorWallet;
        }else{
            fromWallet = m_tip3minorWallet;
            toWallet = m_tip3majorWallet;
        }

        optional(uint256) none;
        m_sendMsg =  tvm.buildExtMsg({
            dest: m_flexClient,
            callbackId: tvm.functionId(onTradeOperationSuccess),
            onErrorId: tvm.functionId(onTradeOperationError),
            time: 0,
            expire: 0,
            sign: true,
            signBoxHandle: m_signingBox,
            pubkey: none,
            call: {AFlexClient.deployPriceXchg, m_bSell, true, true, priceNum,m_dealAmount,m_orderLendBalance,m_dealTime,payout,
                            m_xchgPriceCodeUnsalted,m_xchgPriceSalt,fromWallet,userId,orderId}
        });
        ConfirmInput.get(tvm.functionId(confirmDeployPriceOperation),format("It will cost {:t} EVERs. Unused part will be returned. Continue?",payout));

    }

    function confirmDeployPriceOperation(bool value) public {
        if (value) {
            onSendPriceDeploy();
        }else {
            getPricesDataByHash();
        }
    }

    function onSendPriceDeploy() public view {
        Msg.sendAsync(m_sendMsg);
    }

    function onTradeOperationSuccess(uint256 id) public {
        this.waitForCollection(tvm.functionId(waitFlexClientTransactionResult),format("{:064x}",id));
    }

    function waitForCollection(uint32 answerId, string msg_id) public {
        Query.waitForCollection(
            answerId,
            QueryCollection.Transactions,
            format("{\"in_msg\": {\"eq\": \"{}\"}}",msg_id),
            "out_messages {id msg_type}",
            40000
        );
    }

    function waitFlexClientTransactionResult(QueryStatus status, JsonLib.Value object) public {
        if(status == QueryStatus.Success) {
            mapping(uint256 => TvmCell) jsonObj;
            optional(JsonLib.Value) jsonv;

            jsonObj = object.as_object().get();

            jsonv = jsonObj.get("out_messages");
            JsonLib.Cell[] array = jsonv.get().as_array().get();
            string msg_id;
            for (JsonLib.Cell e: array) {
                optional(JsonLib.Value) val = JsonLib.decodeArrayValue(e.cell);
                mapping(uint256 => TvmCell) obj = val.get().as_object().get();
                val = obj.get("msg_type");
                int msg_type = val.get().as_number().get();
                if(msg_type==0) {
                    val = obj.get("id");
                    msg_id = val.get().as_string().get();
                    break;
                }
            }
            this.waitForCollection(tvm.functionId(waitTip3WalletTransactionResult),msg_id);
        } else
            Terminal.print(0,"Wait FlexClient error");
    }

    function waitTip3WalletTransactionResult(QueryStatus status, JsonLib.Value object) public {
        if(status == QueryStatus.Success) {
            mapping(uint256 => TvmCell) jsonObj;
            optional(JsonLib.Value) jsonv;
            jsonObj = object.as_object().get();
            jsonv = jsonObj.get("out_messages");
            JsonLib.Cell[] array = jsonv.get().as_array().get();
            string msg_id;
            for (JsonLib.Cell e: array) {
                optional(JsonLib.Value) val = JsonLib.decodeArrayValue(e.cell);
                mapping(uint256 => TvmCell) obj = val.get().as_object().get();
                val = obj.get("msg_type");
                int msg_type = val.get().as_number().get();
                if(msg_type==0) {
                    val = obj.get("id");
                    msg_id = val.get().as_string().get();
                    break;
                }
            }
            this.waitForCollection(tvm.functionId(waitPriceTransactionResult),msg_id);
        } else
            Terminal.print(0,"Wait Tip3Wallet error");
    }

    function waitPriceTransactionResult(QueryStatus status, JsonLib.Value object) public {
        Terminal.print(0,"Order sent!");
        getPricesDataByHash();
    }

    function onTradeOperationError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("Operation failed. Sdk error = {}, Error code = {}",sdkError, exitCode));
        ConfirmInput.get(tvm.functionId(confirmDeployPriceOperation),"Do you want ot retry?");
    }

    /*
    *  Implementation of DeBot
    */
    function getDebotInfo() public functionID(0xDEB) override view returns(
        string name, string version, string publisher, string key, string author,
        address support, string hello, string language, string dabi, bytes icon
    ) {
        name = "Flex TIP3/TIP3 Debot";
        version = "0.4.0";
        publisher = "";
        key = "Trade TIP3 for TIP3";
        author = "";
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