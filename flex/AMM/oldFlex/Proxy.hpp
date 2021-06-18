#pragma once

#include <tvm/schema/message.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>

namespace tvm { namespace schema {

__interface IProxy {

  [[external, dyn_chain_parse]]
  void constructor(uint256 pubkey) = 1;

  [[external, noaccept, dyn_chain_parse]]
  void sendMessage(cell msg, uint8 flags) = 2;
};
using IProxyPtr = handle<IProxy>;

struct DProxy {
  uint256 owner_;
};

__interface EProxy {
};

}} // namespace tvm::schema

