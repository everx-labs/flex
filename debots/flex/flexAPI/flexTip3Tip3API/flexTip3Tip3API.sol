pragma ton-solidity >=0.47.0;
pragma AbiHeader expire;
pragma AbiHeader time;
pragma AbiHeader pubkey;
// import required DeBot interfaces and basic DeBot contract.
import "https://raw.githubusercontent.com/tonlabs/debots/main/Debot.sol";
import "DeBotInterfaces.sol";
import "../../abstract/Upgradable.sol";
import "../../interface/IFlexDebot.sol";
import "../../interface/IFlexNotify.sol";
import "../../interface/IFlexAPIDebot.sol";
import "../../interface/IFlexTip3Tip3APIDebot.sol";
import "../../abstract/AFlex.sol";
import "../../abstract/AWrapper.sol";
import "../../abstract/AXchgPair.sol";



contract FlexTip3Tip3APIDebot is Debot,IFlexTip3Tip3APIDebot, Upgradable {

    using JsonLib for JsonLib.Value;
    using JsonLib for mapping(uint256 => TvmCell);

    address m_flexAddr;
    address m_sender;
    TvmCell m_tradingPairCode;
    uint8 m_tip3MajorDecimals;
    uint8 m_tip3MinorDecimals;
    address m_tip3MajorRoot;
    address m_tip3MinorRoot;
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

    function start() public override {
        Terminal.print(0, "Invoke this DeBot!");
    }

    ////////////////////////////////////////////
    //getXchgOrderBook
    ////////////////////////////////////////////
    function getXchgOrderBook(address xchgTradingPair, address flex) public override {
        require(msg.sender!=address(0),101);
        m_sender=msg.sender;
        m_flexAddr = flex;
        m_tradingPair = xchgTradingPair;
        optional(uint256) none;
        AXchgPair(m_tradingPair).getTip3MajorRoot{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setXchgOrderBookMajorRoot),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();

    }

    function setXchgOrderBookMajorRoot(address value0) public {
        m_tip3MajorRoot = value0;
        optional(uint256) none;
        AXchgPair(m_tradingPair).getTip3MinorRoot{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setXchgOrderBookMinorRoot),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setXchgOrderBookMinorRoot(address value0) public {
        m_tip3MinorRoot = value0;
        optional(uint256) none;
        AWrapper(m_tip3MajorRoot).getDetails{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setXchgOrderBookMajorTip3Details),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setXchgOrderBookMajorTip3Details(bytes name, bytes symbol, uint8 decimals, uint256 root_public_key, uint128 total_granted, TvmCell wallet_code, address owner_address, address external_wallet ) public {
        name;symbol;decimals;root_public_key;total_granted;wallet_code;owner_address;external_wallet;//disable compile warnings
        m_tip3MajorDecimals = decimals;
        optional(uint256) none;
        AWrapper(m_tip3MinorRoot).getDetails{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setXchgOrderBookMinorTip3Details),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setXchgOrderBookMinorTip3Details(bytes name, bytes symbol, uint8 decimals, uint256 root_public_key, uint128 total_granted, TvmCell wallet_code, address owner_address, address external_wallet ) public {
        name;symbol;decimals;root_public_key;total_granted;wallet_code;owner_address;external_wallet;//disable compile warnings
        m_tip3MinorDecimals = decimals;
        optional(uint256) none;
        AFlex(m_flexAddr).getXchgPriceCode{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setXchgPriceCode),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }(m_tip3MajorRoot,m_tip3MinorRoot);
    }

    function setXchgPriceCode(TvmCell value0) public {
        Sdk.getAccountsDataByHash(tvm.functionId(onGetXchgOrderBook),tvm.hash(value0),address(0x0));
    }

    function onGetXchgOrderBook(AccData[] accounts)  public view {
        OrderBookRow[] orderBook;

        for (uint i=0; i<accounts.length;i++)
        {
            TvmSlice sl = accounts[i].data.toSlice();
            sl.decode(bool);
            (uint128 pn, uint128 pd, uint128 s, uint128 b) = sl.decode(uint128,uint128,uint128,uint128);
            if (pd!=(uint128(10)**uint128(m_tip3MajorDecimals))) continue;//todo remove
            //if (pd!=priceToUser) continue;//todo uncomment
            //pd
            string price = tokensToStr(pn,m_tip3MinorDecimals);
            string buy = tokensToStr(b,m_tip3MajorDecimals);
            string sell = tokensToStr(s,m_tip3MajorDecimals);
            orderBook.push(OrderBookRow(price,buy,sell));
        }

        IFlexAPIDebot(m_sender).onGetXchgOrderBook(orderBook);
    }

    ////////////////////////////////////////////
    //END getXchgOrderBook
    ////////////////////////////////////////////

    ////////////////////////////////////////////
    //getXchgPriceTickHistory
    ////////////////////////////////////////////
    function getXchgPriceTickHistory(address xchgTradingPair, uint32 startTime) public override{
        xchgTradingPair;startTime;//disable compile warnings
        require(msg.sender!=address(0),101);
        m_sender=msg.sender;
        m_startTime = startTime;
        m_endTime = now - 1;
        m_tradingPair = xchgTradingPair;
        optional(uint256) none;
        AXchgPair(m_tradingPair).getTip3MajorRoot{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setXchgPriceTickHistoryMajorRoot),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();

    }

    function setXchgPriceTickHistoryMajorRoot(address value0) public {
        m_tip3MajorRoot = value0;
        optional(uint256) none;
        AXchgPair(m_tradingPair).getTip3MinorRoot{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setXchgPriceTickHistoryMinorRoot),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setXchgPriceTickHistoryMinorRoot(address value0) public {
        m_tip3MinorRoot = value0;
        /*require(msg.sender!=address(0),101);
        m_sender=msg.sender;*/
        optional(uint256) none;
        AWrapper(m_tip3MajorRoot).getDetails{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setXchgPriceTickHistoryMajorTip3Details),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setXchgPriceTickHistoryMajorTip3Details(bytes name, bytes symbol, uint8 decimals, uint256 root_public_key, uint128 total_granted, TvmCell wallet_code, address owner_address, address external_wallet ) public {
        name;symbol;decimals;root_public_key;total_granted;wallet_code;owner_address;external_wallet;//disable compile warnings
        m_tip3MajorDecimals = decimals;
        optional(uint256) none;
        AWrapper(m_tip3MinorRoot).getDetails{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setXchgPriceTickHistoryMinorTip3Details),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setXchgPriceTickHistoryMinorTip3Details(bytes name, bytes symbol, uint8 decimals, uint256 root_public_key, uint128 total_granted, TvmCell wallet_code, address owner_address, address external_wallet ) public {
        name;symbol;decimals;root_public_key;total_granted;wallet_code;owner_address;external_wallet;//disable compile warnings
        m_tip3MinorDecimals = decimals;
        optional(uint256) none;
        AXchgPair(m_tradingPair).getNotifyAddr{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setXchgPriceTickHistoryNotifyAddress),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setXchgPriceTickHistoryNotifyAddress(address value0) public
    {
        m_notifyAddr = value0;
        mapping(uint32=>TradeData[]) history;
        m_history = history;

        Query.collection(
            tvm.functionId(setXchgPriceTickHistoryQuery1Result),
            QueryCollection.Messages,
            format("{\"dst\":{\"eq\":\"{}\"},\"created_at\":{\"ge\":{},\"le\":{}}}",m_notifyAddr,m_startTime,m_endTime),
            "id created_at body",
            LIMIT,
            QueryOrderBy("id", SortDirection.Ascending)
        );
    }

    function setXchgPriceTickHistoryQuery1Result(QueryStatus status, JsonLib.Value[] objects) public {
        if (status != QueryStatus.Success) {
            onGetXchgPriceTickHistory();
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
                if (fid == tvm.functionId(IFlexNotify.onXchgDealCompleted)) {
                    (address tip3root_sell, address tip3root_buy, uint128 price_num, uint128 price_denum, uint128 amount) = s.decodeFunctionParams(IFlexNotify.onXchgDealCompleted);
                    if ((tip3root_sell==m_tip3MajorRoot)&&(tip3root_buy==m_tip3MinorRoot))
                        if (price_denum!=(uint128(10)**uint128(m_tip3MajorDecimals))) //todo remove
                            m_history[creationAt].push(TradeData(tokensToStr(price_num,m_tip3MinorDecimals),tokensToStr(amount,m_tip3MajorDecimals)));
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
                tvm.functionId(setXchgPriceTickHistoryQuery1Result),
                QueryCollection.Messages,
                format("{\"dst\":{\"eq\":\"{}\"},\"created_at\":{\"ge\":{},\"le\":{}},\"id\":{\"gt\":\"{}\"}}",m_notifyAddr,m_startTime,m_endTime,idStr),
                "id created_at body",
                LIMIT,
                QueryOrderBy("id", SortDirection.Ascending)
            );
        }else {
            onGetXchgPriceTickHistory();
        }
    }

    function onGetXchgPriceTickHistory() public view {
        IFlexAPIDebot(m_sender).onGetXchgPriceTickHistory(m_history);
    }

    ////////////////////////////////////////////
    //END getXchgPriceTickHistory
    ////////////////////////////////////////////

    ////////////////////////////////////////////
    //getXchgCandlestickChart
    ////////////////////////////////////////////
    function getXchgCandlestickChart(address xchgTradingPair, uint32 startTime, uint32 endTime, uint32 stepTime) public override{
        xchgTradingPair;startTime;endTime;stepTime;//disable compile warnings
        require(msg.sender!=address(0),101);
        m_sender=msg.sender;
        m_startTime = startTime;
        m_endTime = endTime;
        m_stepTime = stepTime;
        m_tradingPair = xchgTradingPair;
        optional(uint256) none;
        AXchgPair(m_tradingPair).getTip3MajorRoot{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setXchgCandlestickMajorRoot),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();

    }

    function setXchgCandlestickMajorRoot(address value0) public {
        m_tip3MajorRoot = value0;
        optional(uint256) none;
        AXchgPair(m_tradingPair).getTip3MinorRoot{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setXchgCandlestickMinorRoot),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setXchgCandlestickMinorRoot(address value0) public {
        m_tip3MinorRoot = value0;
        optional(uint256) none;
        AWrapper(m_tip3MajorRoot).getDetails{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setXchgCandlestickMajorTip3Details),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setXchgCandlestickMajorTip3Details(bytes name, bytes symbol, uint8 decimals, uint256 root_public_key, uint128 total_granted, TvmCell wallet_code, address owner_address, address external_wallet ) public {
        name;symbol;decimals;root_public_key;total_granted;wallet_code;owner_address;external_wallet;//disable compile warnings
        m_tip3MajorDecimals = decimals;
        optional(uint256) none;
        AWrapper(m_tip3MinorRoot).getDetails{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setXchgCandlestickMinorTip3Details),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setXchgCandlestickMinorTip3Details(bytes name, bytes symbol, uint8 decimals, uint256 root_public_key, uint128 total_granted, TvmCell wallet_code, address owner_address, address external_wallet ) public {
        name;symbol;decimals;root_public_key;total_granted;wallet_code;owner_address;external_wallet;//disable compile warnings
        m_tip3MinorDecimals = decimals;
        optional(uint256) none;
        AXchgPair(m_tradingPair).getNotifyAddr{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setXchgCandlestickNotifyAddress),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setXchgCandlestickNotifyAddress(address value0) public
    {
        m_notifyAddr = value0;
        mapping(uint32=>TradeData[]) history;
        m_history = history;

        Query.collection(
            tvm.functionId(setXchgCandlestickQuery1Result),
            QueryCollection.Messages,
            format("{\"dst\":{\"eq\":\"{}\"},\"created_at\":{\"ge\":{},\"le\":{}}}",m_notifyAddr,m_startTime,m_endTime),
            "id created_at body",
            LIMIT,
            QueryOrderBy("id", SortDirection.Ascending)
        );
    }

    function setXchgCandlestickQuery1Result(QueryStatus status, JsonLib.Value[] objects) public {
        if (status != QueryStatus.Success) {
            onGetXchgCandlestickChart();
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
                if (fid == tvm.functionId(IFlexNotify.onXchgDealCompleted)) {
                    (address tip3root_sell, address tip3root_buy, uint128 price_num, uint128 price_denum, uint128 amount) = s.decodeFunctionParams(IFlexNotify.onXchgDealCompleted);
                    if ((tip3root_sell==m_tip3MajorRoot)&&(tip3root_buy==m_tip3MinorRoot))
                        if (price_denum!=(uint128(10)**uint128(m_tip3MajorDecimals))) //todo remove
                            m_historyUint[creationAt].push(TradeDataUint(price_num,amount));
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
                tvm.functionId(setXchgCandlestickQuery1Result),
                QueryCollection.Messages,
                format("{\"dst\":{\"eq\":\"{}\"},\"created_at\":{\"ge\":{},\"le\":{}},\"id\":{\"gt\":\"{}\"}}",m_notifyAddr,m_startTime,m_endTime,idStr),
                "id created_at body",
                LIMIT,
                QueryOrderBy("id", SortDirection.Ascending)
            );
        }else {
            onGetXchgCandlestickChart();
        }
    }

    function onGetXchgCandlestickChart() public view {
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
                    format("{:t}",tokensToStr(open,m_tip3MinorDecimals)),
                    format("{:t}",tokensToStr(close,m_tip3MinorDecimals)),
                    format("{:t}",tokensToStr(high,m_tip3MinorDecimals)),
                    format("{:t}",tokensToStr(low,m_tip3MinorDecimals)),
                    tokensToStr(volume,m_tip3MajorDecimals));
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
                format("{:t}",tokensToStr(open,m_tip3MinorDecimals)),
                format("{:t}",tokensToStr(close,m_tip3MinorDecimals)),
                format("{:t}",tokensToStr(high,m_tip3MinorDecimals)),
                format("{:t}",tokensToStr(low,m_tip3MinorDecimals)),
                tokensToStr(volume,m_tip3MajorDecimals));
            volume = 0;
            open = 0;
            close = 0;
            high = 0;
            low = 0;
            curTime = endTime;
            endTime = curTime+m_stepTime;
        }

        IFlexAPIDebot(m_sender).onGetXchgCandlestickChart(candles);
    }

    ////////////////////////////////////////////
    //END getXchgCandlestickChart
    ////////////////////////////////////////////

    ////////////////////////////////////////////
    //getXchgLinearChart
    ////////////////////////////////////////////
    function getXchgLinearChart(address xchgTradingPair, uint32 startTime, uint32 endTime, uint32 stepTime) public override{
        //xchgTradingPair;startTime;endTime;stepTime;//disable compile warnings
        require(msg.sender!=address(0),101);
        m_sender=msg.sender;
        m_startTime = startTime;
        m_endTime = endTime;
        m_stepTime = stepTime;
        m_tradingPair = xchgTradingPair;
        optional(uint256) none;
        AXchgPair(m_tradingPair).getTip3MajorRoot{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setXchgLinearChartMajorRoot),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();

    }

    function setXchgLinearChartMajorRoot(address value0) public {
        m_tip3MajorRoot = value0;
        optional(uint256) none;
        AXchgPair(m_tradingPair).getTip3MinorRoot{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setXchgLinearChartMinorRoot),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setXchgLinearChartMinorRoot(address value0) public {
        m_tip3MinorRoot = value0;
        optional(uint256) none;
        AWrapper(m_tip3MajorRoot).getDetails{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setXchgLinearChartMajorTip3Details),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setXchgLinearChartMajorTip3Details(bytes name, bytes symbol, uint8 decimals, uint256 root_public_key, uint128 total_granted, TvmCell wallet_code, address owner_address, address external_wallet ) public {
        name;symbol;decimals;root_public_key;total_granted;wallet_code;owner_address;external_wallet;//disable compile warnings
        m_tip3MajorDecimals = decimals;
        optional(uint256) none;
        AWrapper(m_tip3MinorRoot).getDetails{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setXchgLinearChartMinorTip3Details),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setXchgLinearChartMinorTip3Details(bytes name, bytes symbol, uint8 decimals, uint256 root_public_key, uint128 total_granted, TvmCell wallet_code, address owner_address, address external_wallet ) public {
        name;symbol;decimals;root_public_key;total_granted;wallet_code;owner_address;external_wallet;//disable compile warnings
        m_tip3MinorDecimals = decimals;
        optional(uint256) none;
        AXchgPair(m_tradingPair).getNotifyAddr{
            abiVer: 2,
            extMsg: true,
            callbackId: tvm.functionId(setXchgLinearChartNotifyAddress),
            onErrorId: 0,
            time: 0,
            expire: 0,
            sign: false,
            pubkey: none
        }();
    }

    function setXchgLinearChartNotifyAddress(address value0) public
    {
        m_notifyAddr = value0;
        mapping(uint32=>TradeData[]) history;
        m_history = history;

        Query.collection(
            tvm.functionId(setXchgLinearChartQuery1Result),
            QueryCollection.Messages,
            format("{\"dst\":{\"eq\":\"{}\"},\"created_at\":{\"ge\":{},\"le\":{}}}",m_notifyAddr,m_startTime,m_endTime),
            "id created_at body",
            LIMIT,
            QueryOrderBy("id", SortDirection.Ascending)
        );
    }

    function setXchgLinearChartQuery1Result(QueryStatus status, JsonLib.Value[] objects) public {
        if (status != QueryStatus.Success) {
            onGetXchgLinearChart();
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
                if (fid == tvm.functionId(IFlexNotify.onXchgDealCompleted)) {
                    (address tip3root_sell, address tip3root_buy, uint128 price_num, uint128 price_denum, uint128 amount) = s.decodeFunctionParams(IFlexNotify.onXchgDealCompleted);
                    if ((tip3root_sell==m_tip3MajorRoot)&&(tip3root_buy==m_tip3MinorRoot))
                        if (price_denum!=(uint128(10)**uint128(m_tip3MajorDecimals))) //todo remove
                            m_historyUint[creationAt].push(TradeDataUint(price_num,amount));
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
                tvm.functionId(setXchgLinearChartQuery1Result),
                QueryCollection.Messages,
                format("{\"dst\":{\"eq\":\"{}\"},\"created_at\":{\"ge\":{},\"le\":{}},\"id\":{\"gt\":\"{}\"}}",m_notifyAddr,m_startTime,m_endTime,idStr),
                "id created_at body",
                LIMIT,
                QueryOrderBy("id", SortDirection.Ascending)
            );
        }else {
            onGetXchgLinearChart();
        }
    }

    function onGetXchgLinearChart() public view {
        mapping(uint32=>TradeData) prices;
        uint32 curTime = m_startTime;
        uint32 endTime = curTime+m_stepTime;
        uint128 volume = 0;
        uint128 price = 0;
        //TradeData
        optional(uint32 , TradeDataUint[]) registryPair = m_historyUint.min();
        while (registryPair.hasValue()) {
            (uint32 key, TradeDataUint[] pi) = registryPair.get();
            while (endTime<key) {
                prices[curTime]=TradeData(format("{:t}",tokensToStr(price,m_tip3MinorDecimals)), tokensToStr(volume,m_tip3MajorDecimals));
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
            prices[curTime]=TradeData(format("{:t}",tokensToStr(price,m_tip3MinorDecimals)), tokensToStr(volume,m_tip3MajorDecimals));
            volume = 0;
            curTime = endTime;
            endTime = curTime+m_stepTime;
        }

        IFlexAPIDebot(m_sender).onGetXchgLinearChart(prices);
    }

    ////////////////////////////////////////////
    //END getXchgLinearChart
    ////////////////////////////////////////////

    /*
    *  Implementation of DeBot
    */
    function getDebotInfo() public functionID(0xDEB) override view returns(
        string name, string version, string publisher, string caption, string author,
        address support, string hello, string language, string dabi, bytes icon
    ) {
        name = "Flex TIP3TIP3 API";
        version = "0.1.9";
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
