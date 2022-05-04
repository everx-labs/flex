pragma ton-solidity >=0.47.0;
//import "../structures/EversConfig.sol";

interface IFlexHelperDebot {
    function getFCAddressAndKeys(address flex) external;
    function getTip3WalletAddress( uint32 signingBox, uint256 pubkey, address tip3root, address flexClient) external;
    function getWrapperSymbol(address wrapper) external;
    /*function withdrawTons(address fclient, uint32 signingBox) external;*/
}