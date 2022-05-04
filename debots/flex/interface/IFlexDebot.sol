pragma ton-solidity >=0.47.0;
import "../structures/EversConfig.sol";

interface IFlexDebot {
    function onGetFCAddressAndKeys(address fc, uint32 signingBox, uint256 pubkey, uint8 dealsLimit, EversConfig crystalCfg, TvmCell xchgpcode) external;
    function onGetTip3WalletAddress(address t3w) external;
    function updateTradingPairs() external;
    function onGetWrapperSymbol(string symbol) external;
    //function invokeWithBuyOrder(address tradingPair, uint128 price, uint128 volume) external;
    //function invokeWithSellOrder(address tradingPair, uint128 price, uint128 volume) external;
    //function burnTip3Wallet(address fclient, uint32 signingBox, address t3Wallet) external;
}