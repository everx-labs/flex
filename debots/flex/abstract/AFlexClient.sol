pragma ton-solidity >=0.47.0;

import "../structures/FlexTonCfg.sol";

struct Tip3Cfg {
    string name;
    string symbol;
    uint8 decimals;
    uint256 root_public_key;
    address root_address;
}

abstract contract AFlexClient {
    constructor(uint256 pubkey, TvmCell trading_pair_code, TvmCell xchg_pair_code) public functionID(0xa) {}
    function setFlexCfg(FlexTonCfg tons_cfg, address flex) public functionID(0xb) {}
    function setExtWalletCode(TvmCell ext_wallet_code) public functionID(0xc) {}

    function setFlexWalletCode(TvmCell flex_wallet_code) public functionID(0xd) {}
    function setFlexWrapperCode(TvmCell flex_wrapper_code) public functionID(0xe) {}

    function deployTradingPair(address tip3_root, uint128 deploy_min_value, uint128 deploy_value, uint128 min_trade_amount, address notify_addr) public functionID(0xf) returns(address value0) {}
    function deployXchgPair(address tip3_major_root, address tip3_minor_root, uint128 deploy_min_value, uint128 deploy_value, uint128 min_trade_amount, address notify_addr) public functionID(0x10) returns(address value0) {}

    function deployPriceWithSell(uint128 price, uint128 amount, uint32 order_finish_time, uint128 min_amount, uint8 deals_limit,
                                uint128 tons_value, TvmCell price_code, address my_tip3_addr, address receive_wallet, Tip3Cfg tip3cfg, address notify_addr) public functionID(0x11) returns(address value0) {}

    function deployPriceWithBuy(uint128 price, uint128 amount, uint32 order_finish_time, uint128 min_amount, uint8 deals_limit,
                                uint128 deploy_value, TvmCell price_code, address my_tip3_addr, Tip3Cfg tip3cfg, address notify_addr) public functionID(0x12) returns(address value0) {}

    function deployPriceXchg(
        bool  sell,
        uint128 price_num,
        uint128 price_denum,
        uint128 amount,
        uint128 lend_amount,
        uint32 lend_finish_time,
        uint128 min_amount,
        uint8 deals_limit,
        uint128 tons_value,
        TvmCell xchg_price_code,
        address my_tip3_addr,
        address receive_wallet,
        Tip3Cfg major_tip3cfg,
        Tip3Cfg minor_tip3cfg,
        address notify_addr
    ) public functionID(0x13) returns(address value0) {}

    function cancelSellOrder(uint128 price, uint128 min_amount, uint8 deals_limit, uint128 value, TvmCell price_code, Tip3Cfg tip3cfg, address notify_addr)  public functionID(0x14) {}
    function cancelBuyOrder(uint128 price, uint128 min_amount, uint8 deals_limit, uint128 value, TvmCell price_code, Tip3Cfg tip3cfg, address notify_addr)  public functionID(0x15) {}

    function cancelXchgOrder(
        bool sell,
        uint128 price_num,
        uint128 price_denum,
        uint128 min_amount,
        uint8 deals_limit,
        uint128 value,
        TvmCell xchg_price_code,
        Tip3Cfg major_tip3cfg,
        Tip3Cfg minor_tip3cfg,
        address notify_addr
    ) public functionID(0x16) returns(address value0) {}

    function transfer(address dest, uint128 value, bool bounce) public functionID(0x17) {}
    function deployWrapperWithWallet(uint256 wrapper_pubkey, uint128 wrapper_deploy_value, uint128 wrapper_keep_balance, uint128 ext_wallet_balance, uint128 set_internal_wallet_value,Tip3Cfg tip3cfg) public functionID(0x18) returns(address value0) {}
    function deployEmptyFlexWallet(uint256 pubkey, uint128 tons_to_wallet, Tip3Cfg tip3cfg) public functionID(0x19) returns(address value0) {}
    function burnWallet(uint128 tons_value, uint256 out_pubkey, address out_internal_owner, address my_tip3_addr)public functionID(0x1a) {}

    function getFlex() public functionID(0x1c) returns(address value0) {}
    function hasFlexWalletCode() public functionID(0x1e) returns(bool value) {}

    function getPayloadForDeployInternalWallet(uint256 owner_pubkey, address owner_addr, uint128 tons) public functionID(0x20) returns(TvmCell value0) {}
}