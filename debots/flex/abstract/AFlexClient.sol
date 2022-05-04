pragma ton-solidity >=0.47.0;

import "../structures/EversConfig.sol";
import "../structures/Tip3Config.sol";

struct LendInfo {
  uint256 pubkey;
  uint32 lend_finish_time;
}

abstract contract AFlexClient {
    constructor(uint256 pubkey) public functionID(0xa) {}
    function setFlexCfg(address flex) public functionID(0xb) {}
    function setFlexWalletCode(TvmCell flex_wallet_code) public functionID(0xc) {}
    function setAuthIndexCode(TvmCell auth_index_code) public functionID(0xd) {}
    function setUserIdIndexCode(TvmCell user_id_index_code) public functionID(0xe) {}

    function deployPriceXchg(
        bool  sell,
        bool immediate_client,
        bool post_order,
        uint128 price_num,
        //uint128 price_denum,
        uint128 amount,
        uint128 lend_amount,
        uint32 lend_finish_time,
        uint128 evers,
        TvmCell unsalted_price_code,
        TvmCell price_salt,
        //TvmCell xchg_price_code,
        address my_tip3_addr,
        //address receive_wallet,
        uint256 user_id,
        uint256 order_id
    ) public functionID(0xf) returns(address value0) {}

    function lendOwnershipPubkey(address my_tip3_addr, uint256 pubkey, uint256 user_id, uint128 evers, uint32 lend_finish_time) public functionID(0x10) {}

    function cancelXchgOrder(
        bool sell,
        uint128 price_num,
        //uint128 price_denum,
        uint128 value,
        TvmCell salted_price_code,
        optional(uint256) user_id,
        optional(uint256) order_id
    ) public functionID(0x12) returns(address value0) {}

    function transfer(address dest, uint128 value, bool bounce) public functionID(0x13) {}

    function transferTokens(address src, address dst, uint128 tokens, uint128 evers) public functionID(0x14) {}
    //function deployEmptyFlexWallet(uint256 pubkey, uint128 crystals_to_wallet, Tip3Config tip3cfg) public functionID(0x17) returns(address value0) {}
    //function burnWallet(uint128 crystals_value, uint256 out_pubkey, optional(address) out_owner, address my_tip3_addr)public functionID(0x18) {}
    function deployEmptyFlexWallet(
        uint256 pubkey,
        uint128 evers_to_wallet,
        Tip3Config tip3cfg,
        optional(LendInfo) lend_info
    ) public functionID(0x17) returns(address value0) {}

    function deployIndex(uint256 user_id, uint256 lend_pubkey, string name, uint128 evers_all, uint128 evers_to_auth_idx) public functionID(0x18) {}
    function reLendIndex(
        uint256 user_id,
        uint256 new_lend_pubkey,
        address[] wallets,
        uint128 evers_relend_call,
        uint128 evers_each_wallet_call,
        uint128 evers_to_remove,
        uint128 evers_to_auth_idx
    ) public functionID(0x19) {}

    function setTradeRestriction(uint128 evers, address my_tip3_addr, address flex, uint256 unsalted_price_code_hash) public functionID(0x1c) {}
    function getFlex() public functionID(0x1e) returns(address value0) {}
    function hasFlexWalletCode() public functionID(0x1f) returns(bool value0) {}
    function hasAuthIndexCode() public functionID(0x20) returns(bool value0) {}
    function hasUserIdIndexCode() public functionID(0x21) returns(bool value0) {}
    function getPriceXchgAddress(uint128 price_num, TvmCell salted_price_code) public functionID(0x23) returns(address value0) {}

    /*function getPayloadForPriceXchg(
        bool sell,
        bool immediate_client,
        bool post_order,
        uint128 amount,
        address receive_tip3_wallet,
        address client_addr,
        uint256 user_id,
        uint256 order_id
    ) public functionID(0x22) returns(TvmCell value0) {}
*/
    /*function getStateInitForPriceXchg(
        uint128 price_num,
        TvmCell xchg_price_code
    ) public functionID(0x23) returns(TvmCell first, address second) {}
*/
    //function getPayloadForDeployInternalWallet(uint256 owner_pubkey, address owner_addr, uint128 crystals) public functionID(0x1e) returns(TvmCell value0) {}

/*
    function setExtWalletCode(TvmCell ext_wallet_code) public functionID(0xc) {}

    function setFlexWalletCode(TvmCell flex_wallet_code) public functionID(0xd) {}
    function setFlexWrapperCode(TvmCell flex_wrapper_code) public functionID(0xe) {}

    function deployTradingPair(address tip3_root, uint128 deploy_min_value, uint128 deploy_value, uint128 min_trade_amount, address notify_addr) public functionID(0xf) returns(address value0) {}
    function deployXchgPair(address tip3_major_root, address tip3_minor_root, uint128 deploy_min_value, uint128 deploy_value, uint128 min_trade_amount, address notify_addr) public functionID(0x10) returns(address value0) {}

    function deployPriceWithSell(uint128 price, uint128 amount, uint32 order_finish_time, uint128 min_amount, uint8 deals_limit,
                                uint128 tons_value, TvmCell price_code, address my_tip3_addr, address receive_wallet, Tip3Cfg tip3cfg, address notify_addr) public functionID(0x11) returns(address value0) {}

    function deployPriceWithBuy(uint128 price, uint128 amount, uint32 order_finish_time, uint128 min_amount, uint8 deals_limit,
                                uint128 deploy_value, TvmCell price_code, address my_tip3_addr, Tip3Cfg tip3cfg, address notify_addr) public functionID(0x12) returns(address value0) {}



    function cancelSellOrder(uint128 price, uint128 min_amount, uint8 deals_limit, uint128 value, TvmCell price_code, Tip3Cfg tip3cfg, address notify_addr)  public functionID(0x14) {}
    function cancelBuyOrder(uint128 price, uint128 min_amount, uint8 deals_limit, uint128 value, TvmCell price_code, Tip3Cfg tip3cfg, address notify_addr)  public functionID(0x15) {}




    function deployWrapperWithWallet(uint256 wrapper_pubkey, uint128 wrapper_deploy_value, uint128 wrapper_keep_balance, uint128 ext_wallet_balance, uint128 set_internal_wallet_value,Tip3Cfg tip3cfg) public functionID(0x18) returns(address value0) {}
*/
}
