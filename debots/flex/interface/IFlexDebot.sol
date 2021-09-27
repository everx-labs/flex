pragma ton-solidity >=0.47.0;
import "../structures/FlexTonCfg.sol";

interface IFlexDebot {
    function onGetFCAddressAndKeys(address fc, uint32 signingBox, uint8 dealsLimit, FlexTonCfg tonCfg, TvmCell tpcode, TvmCell xchgpcode) external;
    function onGetTip3WalletAddress(address t3w) external;
    function updateTradingPairs() external;
    function invokeWithBuyOrder(address tradingPair, uint128 price, uint128 volume) external;
    function invokeWithSellOrder(address tradingPair, uint128 price, uint128 volume) external;
    function burnTip3Wallet(address fclient, uint32 signingBox, address t3Wallet) external;
}