pragma ton-solidity >=0.47.0;

interface IFlexTip3Tip3APIDebot {
    function getXchgOrderBook(address xchgTradingPair, address flex) external;
    function getXchgPriceTickHistory(address xchgTradingPair, uint32 startTime) external;
    function getXchgCandlestickChart(address xchgTradingPair, uint32 startTime, uint32 endTime, uint32 stepTime) external;
    function getXchgLinearChart(address xchgTradingPair, uint32 startTime, uint32 endTime, uint32 stepTime) external;
}