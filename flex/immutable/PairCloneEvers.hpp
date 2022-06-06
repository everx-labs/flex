/** \file
 *  \brief Pair clone evers configuration structure
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#pragma once

namespace tvm {

/// Evers configuration for pair clone procedure
struct PairCloneEvers {
  uint128 deploy;    ///< Evers to send in Pair deploy
  uint128 setnext;   ///< Evers to send in setNext call
  uint128 pair_keep; ///< Evers to keep in Pair contract
};

} // namespace tvm
