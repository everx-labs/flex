#pragma once

namespace tvm { inline namespace schema {

using address_t = address; // for sparse packing in solidity debots

struct Tip3Config {
  string name;
  string symbol;
  uint8 decimals;
  uint256 root_public_key;
  address_t root_address;
};

struct OrderRet {
  uint32 err_code;
  uint128 processed;
  uint128 enqueued;
};

__interface IPriceCallback {
  [[internal, noaccept]]
  void onOrderFinished(OrderRet ret, bool_t sell) = 300;
};
using IPriceCallbackPtr = handle<IPriceCallback>;

}} // namespace tvm::schema
