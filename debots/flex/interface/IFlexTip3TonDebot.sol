pragma ton-solidity >=0.47.0;
import "../structures/FlexTonCfg.sol";

interface IFlexTip3TonDebot {
    function tradeTip3Ton(address fc, uint32 signingBox, address notifyAddr, uint128 minAmount, uint8 dealsLimit, FlexTonCfg tonCfg, address stock, address t3root, address wt3) external;
    function instantTradeTip3Ton(address fc, uint32 signingBox, address notifyAddr, uint128 minAmount, uint8 dealsLimit, FlexTonCfg tonCfg, address stock, address t3root, address wt3, bool bBuy, uint128 price, uint128 volume) external;
}