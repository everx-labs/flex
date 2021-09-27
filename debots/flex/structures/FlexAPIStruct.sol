pragma ton-solidity >=0.47.0;

//it is record at order book
//can contain multiple orders
struct OrderBookRow {
    string price;
    string buyVolume;
    string sellVolume;
}

struct TradeData {
    string price;
    string volume;
}

struct TradeDataUint {
    uint128 price;
    uint128 volume;
}

struct Candlestick {
    string open;
    string close;
    string high;
    string low;
    string volume;
}

struct Tip3Tip3PairInfo {
    string symbolMajor;
    uint8 decimalsMajor;
    string symbolMinor;
    uint8 decimalsMinor;
    address xchgTradingPair;
}

struct Tip3TonPairInfo {
    string symbol;
    uint8 decimals;
    address tradingPair;
}