pragma ton-solidity >=0.47.0;

struct LendOwnership{
   address lend_addr;
   uint128 lend_balance;
   uint32 lend_finish_time;
}

struct Allowance{
    address spender;
    uint128 remainingTokens;
}