/** \file
 *  \brief Flex listing configuration (processing costs)
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#pragma once

namespace tvm {

/// Native funds (evers) configuration for listing procedures
struct ListingConfig {
  /// Funds requirement (from client) to deploy wrapper
  uint128 register_wrapper_cost;
  /// \brief Funds to be taken in case of rejected wrapper.
  /// Rest will be returned to client.
  uint128 reject_wrapper_cost;
  /// Funds to send in wrapper deploy message
  uint128 wrapper_deploy_value;
  /// Wrapper will try to keep this balance (should be less then wrapper_deploy_value)
  uint128 wrapper_keep_balance;
  /// Funds to send in external wallet deploy message
  uint128 ext_wallet_balance;
  /// Funds to send with reserve wallet deploy in Wrapper
  uint128 reserve_wallet_value;
  /// Funds requirement (from client) to deploy xchg/trading pair
  uint128 register_pair_cost;
  /// \brief Funds to be taken in case of rejected xchg/trading pair.
  /// Rest will be returned to client.
  uint128 reject_pair_cost;
  /// Funds to send in pair deploy message
  uint128 pair_deploy_value;
  /// Pair will try to keep this balance
  uint128 pair_keep_balance;
  /// Value for answer
  uint128 register_return_value;
};

} // namespace tvm
