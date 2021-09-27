pragma ton-solidity >=0.47.0;

interface IFlexNotify {
    function onDealCompleted(address tip3root, uint128 price, uint128 amount) functionID(0xa) external;
    function onXchgDealCompleted(address tip3root_sell, address tip3root_buy,
                           uint128 price_num, uint128 price_denum, uint128 amount) functionID(0xb) external;
}
