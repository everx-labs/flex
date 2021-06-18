pragma ton-solidity >=0.40.0;
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

abstract contract AFlex {
    function getSellPriceCode(address tip3_addr) public returns(TvmCell value0) {}
    function getXchgPriceCode(address tip3_addr1, address tip3_addr2) public returns(TvmCell value0) {}
}

abstract contract ATradingPair {
    function getTip3Root() public returns(address value0) {}
    function getFLeXAddr() public returns(address value0) {}
}

abstract contract AXchgPair {
    function getTip3MajorRoot() public returns(address value0) {}
    function getTip3MinorRoot() public returns(address value0) {}
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
    uint256 rootPubkey;
    address root;
    string name;
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
    function deployPriceXchg(TvmCell args_cl) public returns(address value0) {}
    function cancelXchgOrder(TvmCell args) public {}
}

interface IFlexDebot {
    function updateTradingPairs() external;
}

struct PriceInfo {
       uint128 sell;
       uint128 buy;
       address addr;
    }

struct Tip3MenuInfo{
        address tp;
        address t3rMajor;
        address t3rMinor;
        string symbol;
    }

contract FlexDebot is Debot, Upgradable {

    uint256 constant TIP3ROOT_CODEHASH = 0x9e4be18bf6a7f77d5267dde7afa144f905f46a7aa82854df8846ea21f71701c9;

    uint8 constant MAJOR_DETAIL = 0;
    uint8 constant MINOR_DETAIL = 1;

    address m_sender;
    address m_stockAddr;
    address m_tip3majorRoot;
    address m_tip3minorRoot;
    string m_tip3tip3Name;
    uint256 m_masterPubKey;
    uint128 m_flexClientBalance;
    TvmCell m_sendMsg;
    TvmCell m_xchgPriceCode;
    address m_flexClient;
    address m_tip3wallet;
    address m_tip3majorWallet;
    address m_tip3minorWallet;
    mapping(uint8 => T3WDetails) m_tip3walletsDetails;
    uint128 m_dealPrice;
    uint128 m_dealAmount;
    uint32 m_dealTime;
    uint128 m_stockMinAmount;
    uint8 m_stockDealsLimit;
    StockTonCfg m_stockTonCfg;
    uint32 m_getT3WDetailsCallback;
    mapping(uint128 => PriceInfo) m_prices;

    function start() public override {
        Terminal.print(0,"Please invoke me");
    }

    function tokensToStr(uint128 tokens, uint8 decimals) public returns (string) {
        if (decimals==0) return format("{}", tokens);
        uint128 val = uint128(10 ** decimals);
        (uint128 dec, uint128 float) = math.divmod(tokens, val);
        string floatStr = format("{}", float);
        while (floatStr.byteLength() < decimals) {
            floatStr = "0" + floatStr;
        }
        string result = format("{}.{}", dec, floatStr);
        return result;
    }

    function tradeTip3Tip3(address fc, uint256 pk, uint128 minAmount, uint8 dealsLimit, StockTonCfg tonCfg, address stock, address t3major, address t3minor, address wt3major, address wt3minor) public{
        m_flexClient = fc;
        m_masterPubKey = pk;
        m_stockMinAmount = minAmount;
        m_stockDealsLimit = dealsLimit;
        m_stockTonCfg = tonCfg;
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
        ATip3Wallet(m_tip3minorWallet).getDetails{
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
    function getT3WMajorDetails(bytes name, bytes symbol, uint8 decimals, uint128 balance, uint256 root_public_key, uint256 wallet_public_key, address root_address, address owner_address, LendOwnership lend_ownership, TvmCell code, Allowance allowance, int8 workchain_id) public{
        T3WDetails tip3walletDetails;
        tip3walletDetails.decimals = decimals;
        tip3walletDetails.balance = balance;
        tip3walletDetails.lend_ownership = lend_ownership;
        tip3walletDetails.symbol = symbol;
        tip3walletDetails.rootPubkey = root_public_key;
        tip3walletDetails.root = root_address;
        tip3walletDetails.name = name;
        m_tip3walletsDetails[MINOR_DETAIL] = tip3walletDetails;
        optional(uint256) none;
        ATip3Wallet(m_tip3majorWallet).getDetails{
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
        T3WDetails tip3walletDetails;
        tip3walletDetails.decimals = decimals;
        tip3walletDetails.balance = balance;
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
        getPriceCodeHash();
    }

    //get order book
    function getPriceCodeHash() public view {
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

    function setUpdateT3WDetails(bytes name, bytes symbol, uint8 decimals, uint128 balance, uint256 root_public_key, uint256 wallet_public_key, address root_address, address owner_address, LendOwnership lend_ownership, TvmCell code, Allowance allowance, int8 workchain_id) public{
        T3WDetails tip3walletDetails;
        tip3walletDetails.decimals = decimals;
        tip3walletDetails.balance = balance;
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

    function onPricesByHash(ISdk.AccData[] accounts) public {
        MenuItem[] item;
        mapping (uint128 => PriceInfo) empty;
        m_prices = empty;

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
            m_prices[pn]=PriceInfo(s,b,accounts[i].id);
        }

        optional(uint128 , PriceInfo) registryPair = m_prices.min();
        while (registryPair.hasValue()) {
            (uint128 key, PriceInfo pi) = registryPair.get();
            string str = tokensToStr(key,m_tip3walletsDetails[MINOR_DETAIL].decimals);
            str += " | sell "+tokensToStr(pi.sell,m_tip3walletsDetails[MAJOR_DETAIL].decimals)+" | buy "+tokensToStr(pi.buy,m_tip3walletsDetails[MAJOR_DETAIL].decimals);
            item.push(MenuItem(str,"",tvm.functionId(menuQuickDeal)));
            registryPair = m_prices.next(key);
        }

        if (m_tip3walletsDetails[MINOR_DETAIL].lend_ownership.owner==address(0))
            item.push(MenuItem(format("Buy {} for {}",m_tip3walletsDetails[MAJOR_DETAIL].symbol,m_tip3walletsDetails[MINOR_DETAIL].symbol),"",tvm.functionId(menuBuyTip3)));
        else 
            comment.append(format("\nYour {} tip3 wallet is lent. You can't buy now.",m_tip3walletsDetails[MINOR_DETAIL].symbol));
        if (m_tip3walletsDetails[MAJOR_DETAIL].lend_ownership.owner==address(0))
            item.push( MenuItem(format("Sell {} for {}",m_tip3walletsDetails[MAJOR_DETAIL].symbol,m_tip3walletsDetails[MINOR_DETAIL].symbol),"",tvm.functionId(menuSellTip3)));
        else 
            comment.append(format("\nYour {} tip3 wallet is lent. You can't sell now.",m_tip3walletsDetails[MAJOR_DETAIL].symbol));
        if (m_tip3walletsDetails[MINOR_DETAIL].lend_ownership.owner!=address(0))
            item.push(MenuItem("Cancel Buy order","",tvm.functionId(menuCancelBuyTip3)));
        if (m_tip3walletsDetails[MAJOR_DETAIL].lend_ownership.owner!=address(0))
            item.push(MenuItem("Cancel Sell order","",tvm.functionId(menuCancelSellTip3)));
        
        
        item.push(MenuItem("Update order book","",tvm.functionId(menuOrderBookUpdate)));
        item.push(MenuItem("Back","",tvm.functionId(menuOrderBookBack)));
        
        Terminal.print(0,comment);
        
        Menu.select("Order Book","", item);

    }

    //cancel buy order
    function menuCancelBuyTip3(uint32 index) public {
        m_dealPrice = 0;
        m_dealTime = 1;
        onCancelSellTip3();
    }

    //cancel sell order
    function menuCancelSellTip3(uint32 index) public {
        m_dealPrice = 0;
        m_dealTime = 0;
        onCancelSellTip3();
    }

    function onCancelSellTip3() public {
        optional(uint128 , PriceInfo) registryPair = m_prices.min();
        while (registryPair.hasValue()) {
            (uint128 key, PriceInfo pi) = registryPair.get();
            if (m_tip3walletsDetails[uint8(m_dealTime)].lend_ownership.owner == pi.addr){
                m_dealPrice = key;
                m_dealAmount = pi.sell;
                if (m_dealTime==1) {m_dealAmount = pi.buy;}
                break;
            }
            registryPair = m_prices.next(key);
        }

        if (m_dealPrice!=0){
            m_getT3WDetailsCallback = tvm.functionId(cancelSellOrder);
            getFCT3WBalances();
        } else {
            string str;
            if (m_dealTime==0) {str="major";}else {str="minor";}
            Terminal.print(tvm.functionId(getPricesDataByHash),format("Error: {} tip3 lend owner address not found in price contracts ({})",str,m_tip3walletsDetails[uint8(m_dealTime)].lend_ownership.owner));
        }
    }

    function cancelSellOrder(bytes name, bytes symbol, uint8 decimals, uint128 balance, uint256 root_public_key, uint256 wallet_public_key, address root_address, address owner_address, LendOwnership lend_ownership, TvmCell code, Allowance allowance, int8 workchain_id) public{
        T3WDetails tip3walletDetails;
        tip3walletDetails.decimals = decimals;
        tip3walletDetails.balance = balance;
        tip3walletDetails.lend_ownership = lend_ownership;
        tip3walletDetails.symbol = symbol;
        tip3walletDetails.rootPubkey = root_public_key;
        tip3walletDetails.root = root_address;
        tip3walletDetails.name = name;
        m_tip3walletsDetails[MAJOR_DETAIL] = tip3walletDetails;
        
        uint128 payout = 1000000000;
        if (payout+100000000>m_flexClientBalance) {
            Terminal.print(0,"Error: Flex client balance too low!\nPlease send tons to your flex client address:");
            Terminal.print(tvm.functionId(getPricesDataByHash),format("{}",m_flexClient));
            return;
        }
        

        uint128 priceNum = m_dealPrice;
        uint128 priceDenum = 10**uint128(m_tip3walletsDetails[MINOR_DETAIL].decimals);
        TvmBuilder bTip3CfgMajor;
        bTip3CfgMajor.store(m_tip3walletsDetails[MAJOR_DETAIL].name,m_tip3walletsDetails[MAJOR_DETAIL].symbol,m_tip3walletsDetails[MAJOR_DETAIL].decimals,m_tip3walletsDetails[MAJOR_DETAIL].rootPubkey,m_tip3walletsDetails[MAJOR_DETAIL].root);
        TvmBuilder bTip3CfgMinor;
        bTip3CfgMinor.store(m_tip3walletsDetails[MINOR_DETAIL].name,m_tip3walletsDetails[MINOR_DETAIL].symbol,m_tip3walletsDetails[MINOR_DETAIL].decimals,m_tip3walletsDetails[MINOR_DETAIL].rootPubkey,m_tip3walletsDetails[MINOR_DETAIL].root);
        TvmBuilder bFlexXchgCfgs;
        bFlexXchgCfgs.storeRef(bTip3CfgMajor);
        bFlexXchgCfgs.storeRef(bTip3CfgMinor);

        TvmBuilder bFlexXchgCancelArgs;
        bool bSell = true;
        if (m_dealTime==1) {bSell=false;}
        bFlexXchgCancelArgs.store(bSell, priceNum, priceDenum, m_stockMinAmount,m_stockDealsLimit,payout,m_xchgPriceCode,code);
        bFlexXchgCancelArgs.storeRef(bFlexXchgCfgs);
          
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
            call: {AFlexClient.cancelXchgOrder, bFlexXchgCancelArgs.toCell()}
        });
        string pr = tokensToStr(m_dealPrice,m_tip3walletsDetails[MINOR_DETAIL].decimals);
        string t3amount;
        if (m_dealTime==1) {
            t3amount = tokensToStr(m_dealAmount,m_tip3walletsDetails[uint8(MAJOR_DETAIL)].decimals);
        } else {
            t3amount = tokensToStr(m_dealAmount,m_tip3walletsDetails[uint8(MINOR_DETAIL)].decimals);
        }
         
        string str = "sell";
        if (m_dealTime==1) {str="buy";}
        ConfirmInput.get(tvm.functionId(confirmCancelSellOrder),format("Cancel {} order at price {} amount {}\nProcessing value {:t} ton. Unused part will be returned. Continue?",str ,pr, t3amount, payout));
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

    //Quick Deal
    function menuQuickDeal(uint32 index) public {
        PriceInfo pi;
        optional(uint128 , PriceInfo) registryPair = m_prices.min();
        for (uint i=0; i<=index; i++){
            (m_dealPrice, pi) = registryPair.get();
            registryPair = m_prices.next(m_dealPrice);
        }
        if (pi.sell!=0 && pi.buy!=0) {
            Terminal.print(tvm.functionId(getPricesDataByHash),"Fast trade is not supported at volatile price levels");
        }else if (pi.sell!=0) {
            if (m_tip3walletsDetails[MINOR_DETAIL].lend_ownership.owner==address(0)) {
                if (m_tip3walletsDetails[MINOR_DETAIL].balance>0){
                    uint128 max_val = math.min(pi.sell,m_tip3walletsDetails[MINOR_DETAIL].balance);
                    AmountInput.get(tvm.functionId(getBuyQuickDealAmount), format("Amount of {} tip3 to buy for {} tip3:",m_tip3walletsDetails[MAJOR_DETAIL].symbol,m_tip3walletsDetails[MINOR_DETAIL].symbol),m_tip3walletsDetails[MAJOR_DETAIL].decimals,m_stockMinAmount,max_val);
                    //AmountInput.get(tvm.functionId(getSellQuickDealAmount), "Amount of tip3 to sell:",m_tip3walletDetails.decimals,m_stockMinAmount,max_val);
                } else {Terminal.print(tvm.functionId(getPricesDataByHash),"You haven't tip3 tokens to buy.");}
            } else {
                Terminal.print(tvm.functionId(getPricesDataByHash),"Your tip3 wallet is lent. You can't buy tip3 tokens now.");
             }
        }else if (pi.buy!=0) {
            if (m_tip3walletsDetails[MAJOR_DETAIL].lend_ownership.owner==address(0)) {
                if (m_tip3walletsDetails[MAJOR_DETAIL].balance>0){
                    uint128 max_val = math.min(pi.buy,m_tip3walletsDetails[MAJOR_DETAIL].balance);
                    AmountInput.get(tvm.functionId(getSellQuickDealAmount), format("Amount of {} tip3 to sell for {} tip3:",m_tip3walletsDetails[MAJOR_DETAIL].symbol,m_tip3walletsDetails[MINOR_DETAIL].symbol),m_tip3walletsDetails[MAJOR_DETAIL].decimals,m_stockMinAmount,max_val);
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
        AmountInput.get(tvm.functionId(getDealAmmount), format("Amount of {} tip3 to buy for {} tip3:",m_tip3walletsDetails[MAJOR_DETAIL].symbol,m_tip3walletsDetails[MINOR_DETAIL].symbol),m_tip3walletsDetails[MAJOR_DETAIL].decimals,m_stockMinAmount,0xFFFFFFFF);
    }
    function getDealAmmount(uint128 value) public{
       m_dealAmount = value;
       AmountInput.get(tvm.functionId(getDealPrice), format("Price in {} tip3 for 1 {} tip3:",m_tip3walletsDetails[MINOR_DETAIL].symbol,m_tip3walletsDetails[MAJOR_DETAIL].symbol),m_tip3walletsDetails[MINOR_DETAIL].decimals,1,0xFFFFFFFF);
    }
    function getDealPrice(uint128 value) public{
        m_dealPrice = value;
        AmountInput.get(tvm.functionId(getBuyDealTime), "Order time in hours:",0,1,0xFF);
    }

    function getBuyDealTime(uint128 value) public{
       m_dealTime =  uint32(value);
       m_dealTime =uint32(now + m_dealTime * 60 * 60);
       m_getT3WDetailsCallback = tvm.functionId(deployPriceWithBuy);
       getFCT3WBalances();
    }

    function deployPriceWithBuy(bytes name, bytes symbol, uint8 decimals, uint128 balance, uint256 root_public_key, uint256 wallet_public_key, address root_address, address owner_address, LendOwnership lend_ownership, TvmCell code, Allowance allowance, int8 workchain_id) public{
        T3WDetails tip3walletDetails;
        tip3walletDetails.decimals = decimals;
        tip3walletDetails.balance = balance;
        tip3walletDetails.lend_ownership = lend_ownership;
        tip3walletDetails.symbol = symbol;
        tip3walletDetails.rootPubkey = root_public_key;
        tip3walletDetails.root = root_address;
        tip3walletDetails.name = name;
        m_tip3walletsDetails[MAJOR_DETAIL] = tip3walletDetails;
        uint128 priceNum = m_dealPrice;
        uint128 priceDenum = 10**uint128(m_tip3walletsDetails[MINOR_DETAIL].decimals);
        uint128 minorAmount = math.muldivr(m_dealAmount, priceNum, priceDenum);
        if (minorAmount<1) 
        {
            Terminal.print(tvm.functionId(getPricesDataByHash),"Error: too low price! Cancelled!");
            return;
        }

        if (minorAmount>m_tip3walletsDetails[MINOR_DETAIL].balance) 
        {
            Terminal.print(tvm.functionId(getPricesDataByHash),format("Error: you need {} tokens on your minor balance! Cancelled!",minorAmount));
            return;
        }
    
        uint128 payout = m_stockTonCfg.process_queue + m_stockTonCfg.transfer_tip3 + m_stockTonCfg.send_notify +
          m_stockTonCfg.return_ownership + m_stockTonCfg.order_answer + 1000000000;
          
        TvmBuilder bTip3CfgMajor;
        bTip3CfgMajor.store(m_tip3walletsDetails[MAJOR_DETAIL].name,m_tip3walletsDetails[MAJOR_DETAIL].symbol,m_tip3walletsDetails[MAJOR_DETAIL].decimals,m_tip3walletsDetails[MAJOR_DETAIL].rootPubkey,m_tip3walletsDetails[MAJOR_DETAIL].root);
        TvmBuilder bTip3CfgMinor;
        bTip3CfgMinor.store(m_tip3walletsDetails[MINOR_DETAIL].name,m_tip3walletsDetails[MINOR_DETAIL].symbol,m_tip3walletsDetails[MINOR_DETAIL].decimals,m_tip3walletsDetails[MINOR_DETAIL].rootPubkey,m_tip3walletsDetails[MINOR_DETAIL].root);
        
        TvmBuilder bFlexXchgCfgs;
        bFlexXchgCfgs.storeRef(bTip3CfgMajor);
        bFlexXchgCfgs.storeRef(bTip3CfgMinor);

        TvmBuilder bFlexSellArgsAddrs;
        bFlexSellArgsAddrs.store(m_tip3minorWallet,m_tip3majorWallet);
        TvmBuilder bFlexXchgArgs;
        bFlexXchgArgs.store(false,priceNum,priceDenum,m_dealAmount,minorAmount,m_dealTime,m_stockMinAmount,m_stockDealsLimit,payout,
                            m_xchgPriceCode);
        bFlexXchgArgs.storeRef(bFlexSellArgsAddrs);
        bFlexXchgArgs.store(code);
        bFlexXchgArgs.storeRef(bFlexXchgCfgs);

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
            call: {AFlexClient.deployPriceXchg, bFlexXchgArgs.toCell()}
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
        Terminal.print(0,format("debug price {}!",value0));
        Terminal.print(0,"Buy order send!");
        getPricesDataByHash();
    }
    function onBuyError(uint32 sdkError, uint32 exitCode) public {
        Terminal.print(0, format("Buy failed. Sdk error = {}, Error code = {}",sdkError, exitCode));
        ConfirmInput.get(tvm.functionId(confirmDeployPriceWithBuy),"Do you want ot retry?");
    }

    //sell done
    function menuSellTip3(uint32 index) public {
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

    function deployPriceWithSell(bytes name, bytes symbol, uint8 decimals, uint128 balance, uint256 root_public_key, uint256 wallet_public_key, address root_address, address owner_address, LendOwnership lend_ownership, TvmCell code, Allowance allowance, int8 workchain_id) public{
        T3WDetails tip3walletDetails;
        tip3walletDetails.decimals = decimals;
        tip3walletDetails.balance = balance;
        tip3walletDetails.lend_ownership = lend_ownership;
        tip3walletDetails.symbol = symbol;
        tip3walletDetails.rootPubkey = root_public_key;
        tip3walletDetails.root = root_address;
        tip3walletDetails.name = name;
        m_tip3walletsDetails[MAJOR_DETAIL] = tip3walletDetails;
        uint128 priceNum = m_dealPrice;
        uint128 priceDenum = 10**uint128(m_tip3walletsDetails[MINOR_DETAIL].decimals);
        uint128 minorAmount = math.muldivr(m_dealAmount, priceNum, priceDenum);
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
        
        uint128 payout = m_stockTonCfg.process_queue + m_stockTonCfg.transfer_tip3 + m_stockTonCfg.send_notify +
          m_stockTonCfg.return_ownership + m_stockTonCfg.order_answer + 1000000000;
          
        TvmBuilder bTip3CfgMajor;
        bTip3CfgMajor.store(m_tip3walletsDetails[MAJOR_DETAIL].name,m_tip3walletsDetails[MAJOR_DETAIL].symbol,m_tip3walletsDetails[MAJOR_DETAIL].decimals,m_tip3walletsDetails[MAJOR_DETAIL].rootPubkey,m_tip3walletsDetails[MAJOR_DETAIL].root);
        TvmBuilder bTip3CfgMinor;
        bTip3CfgMinor.store(m_tip3walletsDetails[MINOR_DETAIL].name,m_tip3walletsDetails[MINOR_DETAIL].symbol,m_tip3walletsDetails[MINOR_DETAIL].decimals,m_tip3walletsDetails[MINOR_DETAIL].rootPubkey,m_tip3walletsDetails[MINOR_DETAIL].root);
        
        TvmBuilder bFlexXchgCfgs;
        bFlexXchgCfgs.storeRef(bTip3CfgMajor);
        bFlexXchgCfgs.storeRef(bTip3CfgMinor);

        TvmBuilder bFlexSellArgsAddrs;
        bFlexSellArgsAddrs.store(m_tip3majorWallet,m_tip3minorWallet);
        TvmBuilder bFlexXchgArgs;
        bFlexXchgArgs.store(true,priceNum,priceDenum,m_dealAmount,m_dealAmount,m_dealTime,m_stockMinAmount,m_stockDealsLimit,payout,
                            m_xchgPriceCode);
        bFlexXchgArgs.storeRef(bFlexSellArgsAddrs);
        bFlexXchgArgs.store(code);
        bFlexXchgArgs.storeRef(bFlexXchgCfgs);
 
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
            call: {AFlexClient.deployPriceXchg, bFlexXchgArgs.toCell()}
        });
        ConfirmInput.get(tvm.functionId(confirmDeployPriceWithBuy),format("It will cost {:t}. Unused part will be returned. Continue?",payout));
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
        string name, string version, string publisher, string key, string author,
        address support, string hello, string language, string dabi, bytes icon
    ) {
        name = "Flex TIP3/TIP3 Debot";
        version = "0.1.0";
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