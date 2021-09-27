pragma ton-solidity >=0.47.0;
import "../structures/FlexTonCfg.sol";

interface IFlexHelperDebot {
    function getFCAddressAndKeys(address flex) external;
    function getTip3WalletAddress(address flex, uint32 signingBox, address tip3root, address flexClient, FlexTonCfg tonCfg) external;
    function withdrawTons(address fclient, uint32 signingBox) external;
}