pragma ton-solidity >=0.47.0;
import "../structures/EversConfig.sol";

interface IFlexTip3Tip3Debot {
    //function tradeTip3Tip3(address fc, uint32 signingBox, address notifyAddr, uint128 minAmount, uint8 dealsLimit, EversConfig crystalCfg, address stock, address tradingPair, address wt3major, address wt3minor) external;
    function tradeTip3Tip3(address flex, address fc, uint32 signingBox, uint256 pubkey, uint128 minAmount, address tradingPair, address wt3major, address wt3minor) external;
    //function someTestCall() external;
    //function instantTradeTip3Tip3(address fc, uint32 signingBox, address notifyAddr, uint128 minAmount, uint8 dealsLimit, FlexTonCfg tonCfg, address stock, address t3major, address t3minor, address wt3major, address wt3minor, bool bBuy, uint128 price, uint128 volume) external;
}