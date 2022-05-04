pragma ton-solidity >=0.47.0;

import "../structures/Tip3Structures.sol";

abstract contract AFlexWallet {
    function getDetails() public functionID(0x14)
        returns (
            string name, string symbol, uint8 decimals, uint128 balance,
            uint256 root_public_key, address root_address, uint256 wallet_public_key,
            address owner_address,
            optional(lend_pubkey_array_record) lend_pubkey,
            lend_owner_array_record[] lend_owners,
            optional(restriction_info) restriction,
            uint128 lend_balance,
            uint256 code_hash,
            uint16 code_depth,
            Allowance allowance,
            int8 workchain_id) {}

    /*function lendOwnership(
        uint32 _answer_id,
        address answer_addr,
        uint128 evers,
        address dest,
        uint128 lend_balance,
        uint32 lend_finish_time,
        TvmCell deploy_init_cl,
        TvmCell payload
    ) public functionID(0x10) {} //del me*/

    function cancelOrder(
        uint128 evers,
        address price,
        bool sell,
        optional(uint256) order_id
    ) public functionID(0x11) {}

    function makeOrder(
        uint32 _answer_id,
        optional(address) answer_addr,
        uint128 evers,
        uint128 lend_balance,
        uint32 lend_finish_time,
        uint128 price_num,
        TvmCell unsalted_price_code,
        TvmCell salt,
        FlexLendPayloadArgs args
    ) public functionID(0xf) {}


}