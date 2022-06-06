/** \file
 *  \brief WrapperDeployer minimized interface (to call from WIC).
 *  WrapperDeployer deploys Wrapper contracts
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#pragma once

#include <tvm/schema/basics.hpp>
#include <tvm/schema/message.hpp>
#include <tvm/contract_handle.hpp>
#include "immutable_ids.hpp"

namespace tvm {

/** \interface IWrapperDeployer
 *  \brief WrapperDeployer minimized interface to call from WIC.
 */
__interface IWrapperDeployer {
  /// Deploy Wrapper. Returns Wrapper's address and pubkey
  [[internal, answer_id]]
  std::pair<address, uint256> deploy(cell init_args, cell wic_unsalted_code) = immutable_ids::wrapper_deployer_deploy_id;
};
using IWrapperDeployerPtr = handle<IWrapperDeployer>;

} // namespace tvm
