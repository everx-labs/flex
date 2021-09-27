pragma ton-solidity >=0.47.0;

import "../structures/Tip3Structures.sol";

abstract contract AFlexWallet {
    function getDetails() public functionID(0x15)
        returns (
            bytes name, bytes symbol, uint8 decimals, uint128 balance,
            uint256 root_public_key, uint256 wallet_public_key, address root_address,
            address owner_address, LendOwnership[] lend_ownership, uint128 lend_balance,
            TvmCell code, Allowance allowance, int8 workchain_id) {}
}