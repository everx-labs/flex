pragma ton-solidity >=0.47.0;
import "../structures/FlexAPIStruct.sol";

interface IFlexAPIDebot {
    function onGetXchgOrderBook(OrderBookRow[] orderBook) external;
    function onGetXchgPriceTickHistory(mapping(uint32=>TradeData[]) history) external;
    function onGetXchgCandlestickChart(mapping(uint32=>Candlestick) candles) external;
    function onGetXchgLinearChart(mapping(uint32=>TradeData) prices) external;
}