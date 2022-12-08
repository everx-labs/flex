/** \file
 *  \brief WIC clone evers configuration structure
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) EverFlex Inc
 */

#pragma once

namespace tvm {

/// Evers configuration for clone procedure
struct WICCloneEvers {
  uint128 deploy;   ///< Evers to send in WIC deploy
  uint128 setnext;  ///< Evers to send in setNext call
  uint128 wic_keep; ///< Evers to keep in WIC contract
};

} // namespace tvm
