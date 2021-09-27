pragma ton-solidity >=0.47.0;
pragma AbiHeader expire;
pragma AbiHeader time;
pragma AbiHeader pubkey;
// import required DeBot interfaces and basic DeBot contract.
import "https://raw.githubusercontent.com/tonlabs/debots/main/Debot.sol";
import "DeBotInterfaces.sol";
import "../../abstract/Upgradable.sol";
import "../../interface/IFlexAPI.sol";
import "../../interface/IFlexDebot.sol";
import "../../interface/IFlexNotify.sol";
import "../../interface/IFlexAPIDebot.sol";
import "../../interface/IFlexTip3Tip3APIDebot.sol";
import "../../abstract/AFlex.sol";
import "../../abstract/ATradingPair.sol";
import "../../abstract/AWrapper.sol";
import "../../abstract/AXchgPair.sol";


contract FlexDebot is Debot, IFlexAPI, IFlexAPIDebot, Upgradable {

    using JsonLib for JsonLib.Value;
    using JsonLib for mapping(uint256 => TvmCell);

    uint256 constant TIP3WRAPPER_CODEHASH = 0x86920201c74856975aa9bc7cb07b746209dd36834c35b9fe1aca459933b010ff;

    address m_flexAddr;
    address m_sender;
    address m_tip3tip3APIDebot;
    TvmCell m_tradingPairCode;
    Tip3TonPairInfo[] m_tip3tonPairsInfo;
    Tip3TonPairInfo m_curInfo;
    Tip3Tip3PairInfo[] m_tip3tip3PairsInfo;
    Tip3Tip3PairInfo m_curTip3Tip3Info;
    uint m_curTP;
    address[] m_tpaddrs;
    uint8 m_tip3Decimals;
    address m_tip3Root;
    uint32 m_startTime;
    uint32 m_endTime;
    uint32 m_stepTime;
    address m_notifyAddr;
    address m_tradingPair;
    uint32 constant LIMIT = 50;
    mapping(uint32=>TradeData[]) m_history;
    mapping(uint32=>TradeDataUint[]) m_historyUint;

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

    function price2User(uint128 tokens, uint8 decimals) public pure returns (uint128) {
        uint128 k = uint128(10) ** uint128(decimals);
        return uint128(tokens * k);
    }

    function setFlexAddr(address addr) public {
        require(msg.pubkey() == tvm.pubkey(), 101);
        tvm.accept();
        m_flexAddr = addr;
    }

    function setTip3Tip3APIDebotAddr(address addr) public {
        require(msg.pubkey() == tvm.pubkey(), 101);
        tvm.accept();
        m_tip3tip3APIDebot = addr;
    }

    function start() public override {
        Terminal.print(0, "Invoke this DeBot!");
    }
    ////////////////////////////////////////////
    //getPairs
    ////////////////////////////////////////////
    function getPairs() public override {
        require(msg.sender!=address(0),101);
        m_sender=msg.sender;
        optional(uint256) none;
        AFlex(m_flexAddr).getTradingPairCode{
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
        m_tip3tonPairsInfo = new Tip3TonPairInfo[](0);
        m_tpaddrs = new address[](0);
        Sdk.getAccountsDataByHash(tvm.functionId(onAccountsByHash),tvm.hash(m_tradingPairCode),address.makeAddrStd(-1, 0));
    }

    function onAccountsByHash(AccData[] accounts) public {

        for (uint i=0; i<accounts.length;i++)
        {
            m_tpaddrs.push(accounts[i].id);
        }

        if (accounts.length != 0) {
            Sdk.getAccountsDataByHash(
                tvm.functionId(onAccountsByHash),
                tvm.hash(m_tradingPairCode),
                accounts[accounts.length - 1].id
            );
        } else {
            if (m_tpaddrs.length>0)
            {
                m_curTP = 0;
                getTradingPairFlex();
            }
            else
                getXchgPairs();
        }
    }

    function getTradingPairFlex() public {
        m_curInfo = Tip3TonPairInfo("",0,m_tpaddrs[m_curTP]);
        optional(uint256) none;
        ATradingPair(m_tpaddrs[m_curTP]).getFlexAddr{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setTradingPairFlex),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setTradingPairFlex(address value0) public {
        if (value0 == m_flexAddr)
        {
            getTradingPairTip3Root();
        }else
        {
            getNextTP();
        }
    }

    function getTradingPairTip3Root() public view {
        optional(uint256) none;
        ATradingPair(m_tpaddrs[m_curTP]).getTip3Root{
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
        m_tip3Root = value0;
        Sdk.getAccountCodeHash(tvm.functionId(getTip3RootCodeHash), value0);
    }

    function getTip3RootCodeHash(uint256 code_hash) public {
        if (code_hash == TIP3WRAPPER_CODEHASH){
            getTip3RootSymbol();
        }else{
            getNextTP();
        }
    }

    function getTip3RootSymbol() public view {
        optional(uint256) none;
        AWrapper(m_tip3Root).getDetails{
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

    function setTip3RootSymbol(bytes name, bytes symbol, uint8 decimals, uint256 root_public_key, uint128 total_granted, TvmCell wallet_code, address owner_address, address external_wallet ) public {
        name;decimals;root_public_key;total_granted;wallet_code;owner_address;external_wallet;//disable compile warnings
        m_curInfo.symbol = symbol;
        m_curInfo.decimals = decimals;
        m_tip3tonPairsInfo.push(m_curInfo);
        getNextTP();
    }

    function getNextTP() public {
        m_curTP += 1;
        if (m_curTP<m_tpaddrs.length)
            getTradingPairFlex();
        else
            getXchgPairs();
    }

    function getXchgPairs() public view {
        optional(uint256) none;
        AFlex(m_flexAddr).getXchgPairCode{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setXchgPairCode),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setXchgPairCode(TvmCell value0) public {
        m_tradingPairCode = value0;
        m_tip3tip3PairsInfo = new Tip3Tip3PairInfo[](0);
        m_tpaddrs = new address[](0);
        Sdk.getAccountsDataByHash(tvm.functionId(onXchgAccountsByHash),tvm.hash(m_tradingPairCode),address.makeAddrStd(-1, 0));
    }

    function onXchgAccountsByHash(AccData[] accounts) public {

        for (uint i=0; i<accounts.length;i++)
        {
            m_tpaddrs.push(accounts[i].id);
        }

        if (accounts.length != 0) {
            Sdk.getAccountsDataByHash(
                tvm.functionId(onXchgAccountsByHash),
                tvm.hash(m_tradingPairCode),
                accounts[accounts.length - 1].id
            );
        } else {
            if (m_tpaddrs.length>0)
            {
                m_curTP = 0;
                getXchgPairFlex();
            }
            else
                onGetPairs();
        }
    }

    function getXchgPairFlex() public {
        m_curTip3Tip3Info = Tip3Tip3PairInfo("",0,"",0,m_tpaddrs[m_curTP]);
        optional(uint256) none;
        AXchgPair(m_tpaddrs[m_curTP]).getFlexAddr{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setXchgPairFlex),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setXchgPairFlex(address value0) public {
        if (value0 == m_flexAddr)
        {
            optional(uint256) none;
            AXchgPair(m_tpaddrs[m_curTP]).getTip3MajorRoot{
                abiVer: 2,
                extMsg: true,
                callbackId: tvm.functionId(setTip3MajorRoot),
                onErrorId: 0,
                time: 0,
                expire: 0,
                sign: false,
                pubkey: none
            }();
        }else
        {
            getNextXchgTP();
        }
    }

    function setTip3MajorRoot(address value0) public {
        m_tip3Root = value0;
        Sdk.getAccountCodeHash(tvm.functionId(getTip3MajorRootCodeHash), value0);
    }

    function getTip3MajorRootCodeHash(uint256 code_hash) public {
        if (code_hash == TIP3WRAPPER_CODEHASH){
            optional(uint256) none;
            AWrapper(m_tip3Root).getDetails{
                abiVer: 2,
                extMsg: true,
                callbackId: tvm.functionId(setTip3MajorRootSymbol),
                onErrorId: 0,
                time: 0,
                expire: 0,
                sign: false,
                pubkey: none
            }();
        }else{
            getNextXchgTP();
        }
    }


    function setTip3MajorRootSymbol(bytes name, bytes symbol, uint8 decimals, uint256 root_public_key, uint128 total_granted, TvmCell wallet_code, address owner_address, address external_wallet ) public {
        name;decimals;root_public_key;total_granted;wallet_code;owner_address;external_wallet;//disable compile warnings
        m_curTip3Tip3Info.symbolMajor = symbol;
        m_curTip3Tip3Info.decimalsMajor = decimals;

        optional(uint256) none;
        AXchgPair(m_tpaddrs[m_curTP]).getTip3MinorRoot{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setTip3MinorRoot),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }
    function setTip3MinorRoot(address value0) public {
        m_tip3Root = value0;
        Sdk.getAccountCodeHash(tvm.functionId(getTip3MinorRootCodeHash), value0);
    }

    function getTip3MinorRootCodeHash(uint256 code_hash) public {
        if (code_hash == TIP3WRAPPER_CODEHASH){
            optional(uint256) none;
            AWrapper(m_tip3Root).getDetails{
                abiVer: 2,
                extMsg: true,
                callbackId: tvm.functionId(setTip3MinorRootSymbol),
                onErrorId: 0,
                time: 0,
                expire: 0,
                sign: false,
                pubkey: none
            }();
        }else{
            getNextXchgTP();
        }
    }

    function setTip3MinorRootSymbol(bytes name, bytes symbol, uint8 decimals, uint256 root_public_key, uint128 total_granted, TvmCell wallet_code, address owner_address, address external_wallet ) public {
        name;decimals;root_public_key;total_granted;wallet_code;owner_address;external_wallet;//disable compile warnings
        m_curTip3Tip3Info.symbolMinor = symbol;
        m_curTip3Tip3Info.decimalsMinor = decimals;
        m_tip3tip3PairsInfo.push(m_curTip3Tip3Info);
        getNextXchgTP();
    }

    function getNextXchgTP() public {
        m_curTP += 1;
        if (m_curTP<m_tpaddrs.length)
            getXchgPairFlex();
        else
            onGetPairs();
    }

    function onGetPairs() public view {
        IFlexAPIOnGetPairs(m_sender).onGetPairs(m_tip3tip3PairsInfo, m_tip3tonPairsInfo);
    }

    ////////////////////////////////////////////
    //END getPairs
    ////////////////////////////////////////////

    ////////////////////////////////////////////
    //getOrderBook
    ////////////////////////////////////////////
    function getOrderBook(address tradingPair) public override{
        require(msg.sender!=address(0),101);
        m_sender=msg.sender;
        optional(uint256) none;
        ATradingPair(tradingPair).getTip3Root{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setOrderBookTip3Root),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();

    }

    function setOrderBookTip3Root(address value0) public {
        m_tip3Root = value0;
        optional(uint256) none;
        AWrapper(m_tip3Root).getDetails{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setOrderBookTip3Details),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setOrderBookTip3Details(bytes name, bytes symbol, uint8 decimals, uint256 root_public_key, uint128 total_granted, TvmCell wallet_code, address owner_address, address external_wallet ) public {
        name;symbol;decimals;root_public_key;total_granted;wallet_code;owner_address;external_wallet;//disable compile warnings
        m_tip3Decimals = decimals;
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
        }(m_tip3Root);
    }

    function setPriceCode(TvmCell value0) public {
        Sdk.getAccountsDataByHash(tvm.functionId(onGetOrderBook),tvm.hash(value0),address(0x0));
    }

    function onGetOrderBook(AccData[] accounts) public view {
        OrderBookRow[] orderBook;

        for (uint i=0; i<accounts.length;i++)
        {
            TvmSlice sl = accounts[i].data.toSlice();
            sl.decode(bool);
            (uint128 p, uint128 s, uint128 b) = sl.decode(uint128,uint128,uint128);
            string price = format("{:t}",price2User(p,m_tip3Decimals));
            string buy = tokensToStr(b,m_tip3Decimals);
            string sell = tokensToStr(s,m_tip3Decimals);
            orderBook.push(OrderBookRow(price,buy,sell));
        }

        IFlexAPIOnGetOrderBook(m_sender).onGetOrderBook(orderBook);
    }

    ////////////////////////////////////////////
    //END getOrderBook
    ////////////////////////////////////////////

    ////////////////////////////////////////////
    //getXchgOrderBook
    ////////////////////////////////////////////
    function getXchgOrderBook(address xchgTradingPair) public override{
        xchgTradingPair;//disable compile warnings
        require(msg.sender!=address(0),101);
        m_sender=msg.sender;
        IFlexTip3Tip3APIDebot(m_tip3tip3APIDebot).getXchgOrderBook(xchgTradingPair,m_flexAddr);
    }

    function onGetXchgOrderBook(OrderBookRow[] orderBook)  public override {
        IFlexAPIOnGetOrderBook(m_sender).onGetXchgOrderBook(orderBook);
    }

    ////////////////////////////////////////////
    //END getXchgOrderBook
    ////////////////////////////////////////////

    ////////////////////////////////////////////
    //getPriceTickHistory
    ////////////////////////////////////////////
    function getPriceTickHistory(address tradingPair, uint32 startTime) public override{
        require(msg.sender!=address(0),101);
        m_sender=msg.sender;
        optional(uint256) none;
        m_startTime = startTime;
        m_endTime = now - 1;
        m_tradingPair = tradingPair;
        ATradingPair(tradingPair).getTip3Root{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setPriceTickHistoryTip3Root),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();

    }

    function setPriceTickHistoryTip3Root(address value0) public {
        m_tip3Root = value0;

        optional(uint256) none;
        AWrapper(m_tip3Root).getDetails{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setPriceTickHistoryTip3Details),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setPriceTickHistoryTip3Details(bytes name, bytes symbol, uint8 decimals, uint256 root_public_key, uint128 total_granted, TvmCell wallet_code, address owner_address, address external_wallet ) public {
        name;symbol;decimals;root_public_key;total_granted;wallet_code;owner_address;external_wallet;//disable compile warnings
        m_tip3Decimals = decimals;

        optional(uint256) none;
        ATradingPair(m_tradingPair).getNotifyAddr{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setPriceTickHistoryNotifyAddress),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setPriceTickHistoryNotifyAddress(address value0) public
    {
        m_notifyAddr = value0;
        mapping(uint32=>TradeData[]) history;
        m_history = history;

        Query.collection(
            tvm.functionId(setPriceTickHistoryQuery1Result),
            QueryCollection.Messages,
            format("{\"dst\":{\"eq\":\"{}\"},\"created_at\":{\"ge\":{},\"le\":{}}}",m_notifyAddr,m_startTime,m_endTime),
            "id created_at body",
            LIMIT,
            QueryOrderBy("id", SortDirection.Ascending)
        );
    }

    function setPriceTickHistoryQuery1Result(QueryStatus status, JsonLib.Value[] objects) public {
        if (status != QueryStatus.Success) {
            onGetPriceTickHistory();
        }

        if (objects.length != 0) {
            mapping(uint256 => TvmCell) jsonObj;
            optional(JsonLib.Value) jsonv;
            for (JsonLib.Value obj: objects) {
                jsonObj = obj.as_object().get();

                jsonv = jsonObj.get("created_at");
                int ca = jsonv.get().as_number().get();
                uint32 creationAt = uint32(ca);

                jsonv = jsonObj.get("body");
                TvmSlice s = jsonv.get().as_cell().get().toSlice();
                uint32 fid = s.decode(uint32);
                if (fid == tvm.functionId(IFlexNotify.onDealCompleted)) {
                    (address tip3root, uint128 price, uint128 amount) = s.decodeFunctionParams(IFlexNotify.onDealCompleted);
                    if (tip3root==m_tip3Root)
                        m_history[creationAt].push(TradeData(format("{:t}",price2User(price,m_tip3Decimals)),tokensToStr(amount,m_tip3Decimals)));
                }
            }
        }

        if (objects.length == LIMIT) {
            optional(JsonLib.Value) jsonv;
            mapping(uint256 => TvmCell) jsonObj;
            jsonObj = objects[LIMIT-1].as_object().get();
            jsonv = jsonObj.get("id");
            string idStr = jsonv.get().as_string().get();
            Query.collection(
                tvm.functionId(setPriceTickHistoryQuery1Result),
                QueryCollection.Messages,
                format("{\"dst\":{\"eq\":\"{}\"},\"created_at\":{\"ge\":{},\"le\":{}},\"id\":{\"gt\":\"{}\"}}",m_notifyAddr,m_startTime,m_endTime,idStr),
                "id created_at body",
                LIMIT,
                QueryOrderBy("id", SortDirection.Ascending)
            );
        }else {
            onGetPriceTickHistory();
        }
    }

    function onGetPriceTickHistory() public view {
        IFlexAPIOnGetPriceTickHistory(m_sender).onGetPriceTickHistory(m_history);
    }

    ////////////////////////////////////////////
    //END getPriceTickHistory
    ////////////////////////////////////////////

    ////////////////////////////////////////////
    //getXchgPriceTickHistory
    ////////////////////////////////////////////
    function getXchgPriceTickHistory(address xchgTradingPair, uint32 startTime) public override{
        require(msg.sender!=address(0),101);
        m_sender=msg.sender;
        IFlexTip3Tip3APIDebot(m_tip3tip3APIDebot).getXchgPriceTickHistory(xchgTradingPair,startTime);
    }

    function onGetXchgPriceTickHistory(mapping(uint32=>TradeData[]) history) public override {
        IFlexAPIOnGetPriceTickHistory(m_sender).onGetXchgPriceTickHistory(history);
    }

    ////////////////////////////////////////////
    //END getXchgPriceTickHistory
    ////////////////////////////////////////////

    ////////////////////////////////////////////
    //getCandlestickChart
    ////////////////////////////////////////////
    function getCandlestickChart(address tradingPair, uint32 startTime, uint32 endTime, uint32 stepTime) public override{
        require(msg.sender!=address(0),101);
        m_sender=msg.sender;
        optional(uint256) none;
        m_startTime = startTime;
        m_endTime = endTime;
        m_stepTime = stepTime;
        m_tradingPair = tradingPair;
        ATradingPair(tradingPair).getTip3Root{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setCandlestickChartTip3Root),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();

    }

    function setCandlestickChartTip3Root(address value0) public {
        m_tip3Root = value0;

        optional(uint256) none;
        AWrapper(m_tip3Root).getDetails{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setCandlestickChartTip3Details),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setCandlestickChartTip3Details(bytes name, bytes symbol, uint8 decimals, uint256 root_public_key, uint128 total_granted, TvmCell wallet_code, address owner_address, address external_wallet ) public {
        name;symbol;decimals;root_public_key;total_granted;wallet_code;owner_address;external_wallet;//disable compile warnings
        m_tip3Decimals = decimals;

        optional(uint256) none;
        ATradingPair(m_tradingPair).getNotifyAddr{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setCandlestickChartNotifyAddress),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setCandlestickChartNotifyAddress(address value0) public
    {
        m_notifyAddr = value0;
        mapping(uint32=>TradeDataUint[]) history;
        m_historyUint = history;

        Query.collection(
            tvm.functionId(setCandlestickChartQuery1Result),
            QueryCollection.Messages,
            format("{\"dst\":{\"eq\":\"{}\"},\"created_at\":{\"ge\":{},\"le\":{}}}",m_notifyAddr,m_startTime,m_endTime),
            "id created_at body",
            LIMIT,
            QueryOrderBy("id", SortDirection.Ascending)
        );
    }

    function setCandlestickChartQuery1Result(QueryStatus status, JsonLib.Value[] objects) public {
        if (status != QueryStatus.Success) {
            onGetCandlestickChartHistoryUint();
        }

        if (objects.length != 0) {
            mapping(uint256 => TvmCell) jsonObj;
            optional(JsonLib.Value) jsonv;
            for (JsonLib.Value obj: objects) {
                jsonObj = obj.as_object().get();

                jsonv = jsonObj.get("created_at");
                int ca = jsonv.get().as_number().get();
                uint32 creationAt = uint32(ca);

                jsonv = jsonObj.get("body");
                TvmSlice s = jsonv.get().as_cell().get().toSlice();
                uint32 fid = s.decode(uint32);
                if (fid == tvm.functionId(IFlexNotify.onDealCompleted)) {
                    (address tip3root, uint128 price, uint128 amount) = s.decodeFunctionParams(IFlexNotify.onDealCompleted);
                    if (tip3root==m_tip3Root)
                        m_historyUint[creationAt].push(TradeDataUint(price,amount));
                }
            }
        }

        if (objects.length == LIMIT) {
            optional(JsonLib.Value) jsonv;
            mapping(uint256 => TvmCell) jsonObj;
            jsonObj = objects[LIMIT-1].as_object().get();
            jsonv = jsonObj.get("id");
            string idStr = jsonv.get().as_string().get();
            Query.collection(
                tvm.functionId(setCandlestickChartQuery1Result),
                QueryCollection.Messages,
                format("{\"dst\":{\"eq\":\"{}\"},\"created_at\":{\"ge\":{},\"le\":{}},\"id\":{\"gt\":\"{}\"}}",m_notifyAddr,m_startTime,m_endTime,idStr),
                "id created_at body",
                LIMIT,
                QueryOrderBy("id", SortDirection.Ascending)
            );
        }else {
            onGetCandlestickChartHistoryUint();
        }
    }

    function onGetCandlestickChartHistoryUint() public view {
        mapping(uint32=>Candlestick) candles;
        uint32 curTime = m_startTime;
        uint32 endTime = curTime+m_stepTime;
        uint128 volume = 0;
        uint128 open = 0;
        uint128 close = 0;
        uint128 high = 0;
        uint128 low = 0;
        optional(uint32 , TradeDataUint[]) registryPair = m_historyUint.min();
        while (registryPair.hasValue()) {
            (uint32 key, TradeDataUint[] pi) = registryPair.get();
            while (endTime<key) {
                candles[curTime]=Candlestick(
                    format("{:t}",price2User(open,m_tip3Decimals)),
                    format("{:t}",price2User(close,m_tip3Decimals)),
                    format("{:t}",price2User(high,m_tip3Decimals)),
                    format("{:t}",price2User(low,m_tip3Decimals)),
                    tokensToStr(volume,m_tip3Decimals));
                volume = 0;
                open = 0;
                close = 0;
                high = 0;
                low = 0;
                curTime = endTime;
                endTime = curTime+m_stepTime;
            }

            for (TradeDataUint val : pi) {
                volume+=val.volume;
                if (open==0) open = val.price;
                close = val.price;
                if (high==0 || high<val.price) high = val.price;
                if (low==0 || low>val.price) low = val.price;
            }

            registryPair = m_historyUint.next(key);
        }

        while (endTime<m_endTime) {
            candles[curTime]=Candlestick(
                format("{:t}",price2User(open,m_tip3Decimals)),
                format("{:t}",price2User(close,m_tip3Decimals)),
                format("{:t}",price2User(high,m_tip3Decimals)),
                format("{:t}",price2User(low,m_tip3Decimals)),
                tokensToStr(volume,m_tip3Decimals));
            volume = 0;
            open = 0;
            close = 0;
            high = 0;
            low = 0;
            curTime = endTime;
            endTime = curTime+m_stepTime;
        }

        IFlexAPIOnGetCandlestickChart(m_sender).onGetCandlestickChart(candles);
    }

    ////////////////////////////////////////////
    //END getCandlestickChart
    ////////////////////////////////////////////

    ////////////////////////////////////////////
    //getXchgCandlestickChart
    ////////////////////////////////////////////
    function getXchgCandlestickChart(address xchgTradingPair, uint32 startTime, uint32 endTime, uint32 stepTime) public override{
        require(msg.sender!=address(0),101);
        m_sender=msg.sender;
        IFlexTip3Tip3APIDebot(m_tip3tip3APIDebot).getXchgCandlestickChart(xchgTradingPair,startTime,endTime,stepTime);
    }

    function onGetXchgCandlestickChart(mapping(uint32=>Candlestick) candles)  public override {
        IFlexAPIOnGetCandlestickChart(m_sender).onGetXchgCandlestickChart(candles);
    }

    ////////////////////////////////////////////
    //END getXchgCandlestickChart
    ////////////////////////////////////////////

    ////////////////////////////////////////////
    //getLinearChart
    ////////////////////////////////////////////
    function getLinearChart(address tradingPair, uint32 startTime, uint32 endTime, uint32 stepTime) public override{
        require(msg.sender!=address(0),101);
        m_sender=msg.sender;
        optional(uint256) none;
        m_startTime = startTime;
        m_endTime = endTime;
        m_stepTime = stepTime;
        m_tradingPair = tradingPair;
        ATradingPair(tradingPair).getTip3Root{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setLinearChartTip3Root),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();

    }

    function setLinearChartTip3Root(address value0) public {
        m_tip3Root = value0;

        optional(uint256) none;
        AWrapper(m_tip3Root).getDetails{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setLinearChartTip3Details),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setLinearChartTip3Details(bytes name, bytes symbol, uint8 decimals, uint256 root_public_key, uint128 total_granted, TvmCell wallet_code, address owner_address, address external_wallet ) public {
        name;symbol;decimals;root_public_key;total_granted;wallet_code;owner_address;external_wallet;//disable compile warnings
        m_tip3Decimals = decimals;

        optional(uint256) none;
        ATradingPair(m_tradingPair).getNotifyAddr{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setLinearChartNotifyAddress),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setLinearChartNotifyAddress(address value0) public
    {
        m_notifyAddr = value0;
        mapping(uint32=>TradeDataUint[]) history;
        m_historyUint = history;

        Query.collection(
            tvm.functionId(setLinearChartQuery1Result),
            QueryCollection.Messages,
            format("{\"dst\":{\"eq\":\"{}\"},\"created_at\":{\"ge\":{},\"le\":{}}}",m_notifyAddr,m_startTime,m_endTime),
            "id created_at body",
            LIMIT,
            QueryOrderBy("id", SortDirection.Ascending)
        );
    }

    function setLinearChartQuery1Result(QueryStatus status, JsonLib.Value[] objects) public {
        if (status != QueryStatus.Success) {
            onGetLinearChartHistoryUint();
        }

        if (objects.length != 0) {
            mapping(uint256 => TvmCell) jsonObj;
            optional(JsonLib.Value) jsonv;
            for (JsonLib.Value obj: objects) {
                jsonObj = obj.as_object().get();

                jsonv = jsonObj.get("created_at");
                int ca = jsonv.get().as_number().get();
                uint32 creationAt = uint32(ca);

                jsonv = jsonObj.get("body");
                TvmSlice s = jsonv.get().as_cell().get().toSlice();
                uint32 fid = s.decode(uint32);
                if (fid == tvm.functionId(IFlexNotify.onDealCompleted)) {
                    (address tip3root, uint128 price, uint128 amount) = s.decodeFunctionParams(IFlexNotify.onDealCompleted);
                    if (tip3root==m_tip3Root)
                        m_historyUint[creationAt].push(TradeDataUint(price,amount));
                }
            }
        }

        if (objects.length == LIMIT) {
            optional(JsonLib.Value) jsonv;
            mapping(uint256 => TvmCell) jsonObj;
            jsonObj = objects[LIMIT-1].as_object().get();
            jsonv = jsonObj.get("id");
            string idStr = jsonv.get().as_string().get();
            Query.collection(
                tvm.functionId(setLinearChartQuery1Result),
                QueryCollection.Messages,
                format("{\"dst\":{\"eq\":\"{}\"},\"created_at\":{\"ge\":{},\"le\":{}},\"id\":{\"gt\":\"{}\"}}",m_notifyAddr,m_startTime,m_endTime,idStr),
                "id created_at body",
                LIMIT,
                QueryOrderBy("id", SortDirection.Ascending)
            );
        }else {
            onGetLinearChartHistoryUint();
        }
    }

    function onGetLinearChartHistoryUint() public view {

        mapping(uint32=>TradeData) prices;
        uint32 curTime = m_startTime;
        uint32 endTime = curTime+m_stepTime;
        uint128 volume = 0;
        uint128 price = 0;
        optional(uint32 , TradeDataUint[]) registryPair = m_historyUint.min();
        while (registryPair.hasValue()) {
            (uint32 key, TradeDataUint[] pi) = registryPair.get();
            while (endTime<key) {
                prices[curTime]=TradeData(format("{:t}",price2User(price,m_tip3Decimals)), tokensToStr(volume,m_tip3Decimals));
                volume = 0;
                curTime = endTime;
                endTime = curTime+m_stepTime;
            }

            for (TradeDataUint val : pi) {
                volume+=val.volume;
                price = val.price;
            }

            registryPair = m_historyUint.next(key);
        }

        while (endTime<m_endTime) {
            prices[curTime]=TradeData(format("{:t}",price2User(price,m_tip3Decimals)), tokensToStr(volume,m_tip3Decimals));
            volume = 0;
            curTime = endTime;
            endTime = curTime+m_stepTime;
        }

        IFlexAPIOnGetLinearChart(m_sender).onGetLinearChart(prices);
    }

    ////////////////////////////////////////////
    //END getLinearChart
    ////////////////////////////////////////////

    ////////////////////////////////////////////
    //getXchgLinearChart
    ////////////////////////////////////////////
    function getXchgLinearChart(address xchgTradingPair, uint32 startTime, uint32 endTime, uint32 stepTime) public override{
        xchgTradingPair;startTime;endTime;stepTime;//disable compile warnings
        require(msg.sender!=address(0),101);
        m_sender=msg.sender;
        IFlexTip3Tip3APIDebot(m_tip3tip3APIDebot).getXchgLinearChart(xchgTradingPair,startTime,endTime,stepTime);
    }

    function onGetXchgLinearChart(mapping(uint32=>TradeData) prices) public override {
        IFlexAPIOnGetLinearChart(m_sender).onGetXchgLinearChart(prices);
    }

    ////////////////////////////////////////////
    //END getXchgLinearChart
    ////////////////////////////////////////////

    ////////////////////////////////////////////
    //getBuyOrderMsg
    ////////////////////////////////////////////
    function getBuyOrderMsg(address tradingPair, uint128 price, uint128 volume) public override{
        require(msg.sender!=address(0),101);
        m_sender=msg.sender;

        TvmCell body = tvm.encodeBody(IFlexDebot.invokeWithBuyOrder, tradingPair, price, volume);
        TvmBuilder message_;
        message_.store(false, true, true, false, address(0), address(this));
        message_.storeTons(0);
        message_.storeUnsigned(0, 1);
        message_.storeTons(0);
        message_.storeTons(0);
        message_.store(uint64(0));
        message_.store(uint32(0));
        message_.storeUnsigned(0, 1); //init: nothing$0
        message_.storeUnsigned(1, 1); //body: right$1
        message_.store(body);

        IFlexAPIOnGetOrderMsg(m_sender).onGetBuyOrderMsg(message_.toCell());
    }

    ////////////////////////////////////////////
    //END getBuyOrderMsg
    ////////////////////////////////////////////

    ////////////////////////////////////////////
    //getSellOrderMsg
    ////////////////////////////////////////////
    function getSellOrderMsg(address tradingPair, uint128 price, uint128 volume) public override{
        require(msg.sender!=address(0),101);
        m_sender=msg.sender;

        TvmCell body = tvm.encodeBody(IFlexDebot.invokeWithSellOrder, tradingPair, price, volume);
        TvmBuilder message_;
        message_.store(false, true, true, false, address(0), address(this));
        message_.storeTons(0);
        message_.storeUnsigned(0, 1);
        message_.storeTons(0);
        message_.storeTons(0);
        message_.store(uint64(0));
        message_.store(uint32(0));
        message_.storeUnsigned(0, 1); //init: nothing$0
        message_.storeUnsigned(1, 1); //body: right$1
        message_.store(body);

        IFlexAPIOnGetOrderMsg(m_sender).onGetSellOrderMsg(message_.toCell());
    }
    ////////////////////////////////////////////
    //END getSellOrderMsg
    ////////////////////////////////////////////


    /*
    *  Implementation of DeBot
    */
    function getDebotInfo() public functionID(0xDEB) override view returns(
        string name, string version, string publisher, string caption, string author,
        address support, string hello, string language, string dabi, bytes icon
    ) {
        name = "Flex API";
        version = "0.2.0";
        publisher = "TON Labs";
        caption = "API for flex";
        author = "TON Labs";
        support = address.makeAddrStd(0, 0x0);
        hello = "";
        language = "en";
        dabi = m_debotAbi.get();
        icon = "";
    }

    function getRequiredInterfaces() public view override returns (uint256[] interfaces) {
        return [ Sdk.ID, Terminal.ID, Query.ID ];
    }

    /*
    *  Implementation of Upgradable
    */
    function onCodeUpgrade() internal override {
        tvm.resetStorage();
    }

}
