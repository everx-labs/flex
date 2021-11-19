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

}} // namespace tvm::schema
