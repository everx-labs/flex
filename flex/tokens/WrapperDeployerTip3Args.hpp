/** \file
 *  \brief WrapperDeployerTip3 arguments structure (for `deploy` call)
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#pragma once

namespace tvm {

/// WrapperDeployerTip3 arguments structure (for `deploy` call)
struct WrapperDeployerTip3Args {
  Tip3Config tip3cfg; ///< Tip3 configuration for the external wallet
};

} // namespace tvm
